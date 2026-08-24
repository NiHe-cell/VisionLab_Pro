#include "TensorRTEngine.h"

#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <system_error>
#include <utility>
#include <vector>

#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime_api.h>

#include "inference/cuda/CudaDeviceBuffer.h"
#include "inference/cuda/CudaStream.h"

namespace visionlab {

namespace {

struct TrtDeleter
{
    template <typename T>
    void operator()(T* obj) const noexcept
    {
        delete obj;
    }
};

template <typename T>
using TrtPtr = std::unique_ptr<T, TrtDeleter>;

std::int64_t elementCount(const std::vector<std::int64_t>& shape)
{
    std::int64_t count = 1;
    for (const std::int64_t dim : shape)
    {
        if (dim < 0)
            return -1;
        count *= dim;
    }
    return count;
}

std::string joinError(const std::string& prefix, const std::filesystem::path& path)
{
    return prefix + path.generic_string();
}

std::vector<std::int64_t> dimsToShape(const nvinfer1::Dims& dims)
{
    std::vector<std::int64_t> shape;
    if (dims.nbDims < 0)
        return shape;
    shape.reserve(static_cast<std::size_t>(dims.nbDims));
    for (int32_t i = 0; i < dims.nbDims; ++i)
        shape.push_back(dims.d[i]);
    return shape;
}

bool hasDynamicDim(const nvinfer1::Dims& dims)
{
    if (dims.nbDims < 0)
        return true;
    for (int32_t i = 0; i < dims.nbDims; ++i)
    {
        if (dims.d[i] < 0)
            return true;
    }
    return false;
}

std::string parserErrors(nvonnxparser::IParser& parser)
{
    std::string text;
    const int32_t count = parser.getNbErrors();
    for (int32_t i = 0; i < count; ++i)
    {
        const nvonnxparser::IParserError* err = parser.getError(i);
        if (err == nullptr || err->desc() == nullptr)
            continue;
        if (!text.empty())
            text += "; ";
        text += err->desc();
    }
    return text;
}

std::int64_t utcMtimeSeconds(const std::filesystem::path& path)
{
    const auto fileTime = std::filesystem::last_write_time(path);
    const auto sysTime = std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
    return std::chrono::duration_cast<std::chrono::seconds>(sysTime.time_since_epoch()).count();
}

std::string gpuSlug(int deviceId)
{
    cudaDeviceProp prop{};
    if (cudaGetDeviceProperties(&prop, deviceId) != cudaSuccess)
        return "gpu";

    std::string slug;
    for (const char c : std::string(prop.name))
    {
        if (std::isalnum(static_cast<unsigned char>(c)))
            slug.push_back(c);
        else if (!slug.empty() && slug.back() != '-')
            slug.push_back('-');
    }
    while (!slug.empty() && slug.back() == '-')
        slug.pop_back();
    return slug.empty() ? std::string("gpu") : slug;
}

std::string trtVersionString()
{
    return std::to_string(NV_TENSORRT_MAJOR) + "." + std::to_string(NV_TENSORRT_MINOR) + "."
        + std::to_string(NV_TENSORRT_PATCH);
}

std::string precisionTag(InferencePrecision precision)
{
    return precision == InferencePrecision::Fp16 ? "fp16" : "fp32";
}

std::filesystem::path cacheFilePath(const ModelConfig& config)
{
    const auto size = std::filesystem::file_size(config.modelPath);
    std::string name = config.modelPath.stem().string();
    name += "-s" + std::to_string(static_cast<unsigned long long>(size));
    name += "-t" + std::to_string(utcMtimeSeconds(config.modelPath));
    name += "-g" + gpuSlug(config.deviceId);
    name += "-v" + trtVersionString();
    name += "-p" + precisionTag(config.precision);
    name += "-1x3x" + std::to_string(config.inputHeight) + "x"
        + std::to_string(config.inputWidth);
    name += ".engine";
    return config.modelPath.parent_path() / ".trt-cache" / name;
}

struct EngineIo
{
    std::string inputName;
    TensorMetadata inputMeta;
    std::vector<std::string> outputNames;
    std::vector<TensorMetadata> outputMeta;
};

bool readEngineIo(
    nvinfer1::ICudaEngine& engine,
    const ModelConfig& config,
    EngineIo& io,
    std::string& error)
{
    io = {};
    const int32_t ioCount = engine.getNbIOTensors();
    for (int32_t i = 0; i < ioCount; ++i)
    {
        const char* name = engine.getIOTensorName(i);
        if (name == nullptr)
        {
            error = "engine I/O tensor is unnamed";
            return false;
        }
        const nvinfer1::TensorIOMode mode = engine.getTensorIOMode(name);
        if (engine.getTensorDataType(name) != nvinfer1::DataType::kFLOAT)
        {
            error = "deserialized engine tensor is not float32";
            return false;
        }
        const nvinfer1::Dims dims = engine.getTensorShape(name);
        if (hasDynamicDim(dims))
        {
            error = "dynamic engine tensor shape is not supported";
            return false;
        }
        TensorMetadata meta;
        meta.name = name;
        meta.shape = dimsToShape(dims);
        meta.dtype = "float32";
        if (mode == nvinfer1::TensorIOMode::kINPUT)
        {
            if (!io.inputName.empty())
            {
                error = "TensorRTEngine expects exactly one model input";
                return false;
            }
            if (dims.nbDims != 4 || dims.d[0] != 1 || dims.d[1] != 3
                || dims.d[2] != config.inputHeight || dims.d[3] != config.inputWidth)
            {
                error = "deserialized engine input is not static 1x3xHxW float32 matching ModelConfig";
                return false;
            }
            io.inputName = name;
            io.inputMeta = std::move(meta);
        }
        else if (mode == nvinfer1::TensorIOMode::kOUTPUT)
        {
            io.outputNames.push_back(name);
            io.outputMeta.push_back(std::move(meta));
        }
    }
    if (io.inputName.empty())
    {
        error = "deserialized engine has no input";
        return false;
    }
    if (io.outputNames.empty())
    {
        error = "deserialized engine has no outputs";
        return false;
    }
    return true;
}

bool persistCache(
    const std::filesystem::path& dest,
    const void* data,
    std::size_t bytes,
    std::string& error)
{
    std::error_code ec;
    std::filesystem::create_directories(dest.parent_path(), ec);
    if (ec)
    {
        error = "failed to create TensorRT cache directory";
        return false;
    }

    std::filesystem::path tmp = dest;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            error = "failed to open TensorRT cache temp file";
            return false;
        }
        out.write(static_cast<const char*>(data), static_cast<std::streamsize>(bytes));
        if (!out)
        {
            out.close();
            std::filesystem::remove(tmp, ec);
            error = "failed to write TensorRT cache temp file";
            return false;
        }
    }

    std::filesystem::rename(tmp, dest, ec);
    if (ec)
    {
        std::filesystem::remove(dest, ec);
        std::filesystem::rename(tmp, dest, ec);
    }
    if (ec)
    {
        std::filesystem::remove(tmp, ec);
        error = "failed to publish TensorRT cache file";
        return false;
    }
    return true;
}

void discardCache(const std::filesystem::path& dest)
{
    std::error_code ec;
    std::filesystem::remove(dest, ec);
}

std::vector<char> readBinaryFile(const std::filesystem::path& path, std::string& error)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        error = joinError("failed to read: ", path);
        return {};
    }
    in.seekg(0, std::ios::end);
    const auto fileSize = in.tellg();
    if (fileSize <= 0)
    {
        error = joinError("empty file: ", path);
        return {};
    }
    in.seekg(0, std::ios::beg);
    std::vector<char> bytes(static_cast<std::size_t>(fileSize));
    if (!in.read(bytes.data(), static_cast<std::streamsize>(bytes.size())))
    {
        error = joinError("failed to read: ", path);
        return {};
    }
    return bytes;
}

class TrtLogger final : public nvinfer1::ILogger
{
public:
    void log(Severity severity, nvinfer1::AsciiChar const* msg) noexcept override
    {
        if (msg == nullptr || severity > Severity::kWARNING)
            return;
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_messages.empty())
            m_messages += "; ";
        m_messages += msg;
    }

    std::string messages() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_messages;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.clear();
    }

private:
    mutable std::mutex m_mutex;
    std::string m_messages;
};

} // namespace

struct TensorRTEngine::Impl
{
    TrtLogger logger;
    std::string error;
    bool ready = false;
    int deviceId = 0;

    std::string inputName;
    std::vector<std::string> outputNames;
    TensorMetadata inputMeta;
    std::vector<TensorMetadata> outputMeta;

    std::unique_ptr<CudaStream> stream;
    std::unique_ptr<CudaDeviceBuffer> inputBuffer;
    std::vector<std::unique_ptr<CudaDeviceBuffer>> outputBuffers;

    TrtPtr<nvinfer1::IRuntime> runtime;
    TrtPtr<nvinfer1::ICudaEngine> engine;
    TrtPtr<nvinfer1::IExecutionContext> context;

    void reset()
    {
        ready = false;
        context.reset();
        engine.reset();
        runtime.reset();
        outputBuffers.clear();
        inputBuffer.reset();
        stream.reset();
        inputName.clear();
        outputNames.clear();
        inputMeta = {};
        outputMeta.clear();
    }
};

TensorRTEngine::TensorRTEngine()
    : m_impl(std::make_unique<Impl>())
{
}

TensorRTEngine::~TensorRTEngine() = default;

bool TensorRTEngine::initialize(const ModelConfig& config)
{
    m_impl->reset();
    m_impl->logger.clear();
    m_impl->deviceId = config.deviceId;

    if (config.backend != InferenceBackend::TensorRT)
    {
        m_impl->error = "TensorRTEngine only supports InferenceBackend::TensorRT";
        return false;
    }
    if (config.precision != InferencePrecision::Fp32
        && config.precision != InferencePrecision::Fp16)
    {
        m_impl->error = "TensorRTEngine does not support InferencePrecision::Int8";
        return false;
    }

    std::error_code existsError;
    if (config.modelPath.empty()
        || !std::filesystem::exists(config.modelPath, existsError))
    {
        m_impl->error = joinError("model file not found: ", config.modelPath);
        return false;
    }

    int deviceCount = 0;
    const cudaError_t countErr = cudaGetDeviceCount(&deviceCount);
    if (countErr != cudaSuccess)
    {
        m_impl->error = std::string("CUDA device query failed: ") + cudaGetErrorString(countErr);
        return false;
    }
    if (deviceCount < 1)
    {
        m_impl->error = "no CUDA device";
        return false;
    }
    if (config.deviceId < 0 || config.deviceId >= deviceCount)
    {
        m_impl->error = "invalid CUDA device id " + std::to_string(config.deviceId);
        return false;
    }
    const cudaError_t setErr = cudaSetDevice(config.deviceId);
    if (setErr != cudaSuccess)
    {
        m_impl->error = std::string("failed to select CUDA device ")
            + std::to_string(config.deviceId) + ": " + cudaGetErrorString(setErr);
        return false;
    }

    auto fail = [this](std::string message) {
        if (message.empty())
            message = m_impl->logger.messages();
        m_impl->reset();
        m_impl->error = std::move(message);
        return false;
    };

    try
    {
        auto stream = std::make_unique<CudaStream>();
        if (!stream->valid())
            return fail("CUDA stream create failed: " + stream->lastError());

        TrtPtr<nvinfer1::IBuilder> builder{nvinfer1::createInferBuilder(m_impl->logger)};
        if (!builder)
            return fail("createInferBuilder failed");
        if (config.precision == InferencePrecision::Fp16 && !builder->platformHasFastFp16())
            return fail("Fp16 is not supported on this device");

        TrtPtr<nvinfer1::IRuntime> runtime{nvinfer1::createInferRuntime(m_impl->logger)};
        if (!runtime)
            return fail("createInferRuntime failed");

        const std::filesystem::path cachePath = cacheFilePath(config);
        TrtPtr<nvinfer1::ICudaEngine> cudaEngine;
        EngineIo io;

        std::error_code cacheExistsError;
        if (std::filesystem::exists(cachePath, cacheExistsError))
        {
            std::string readError;
            std::vector<char> blob = readBinaryFile(cachePath, readError);
            if (!blob.empty())
            {
                cudaEngine.reset(runtime->deserializeCudaEngine(blob.data(), blob.size()));
                std::string inspectError;
                if (cudaEngine && readEngineIo(*cudaEngine, config, io, inspectError))
                {
                    // cache hit
                }
                else
                {
                    cudaEngine.reset();
                    discardCache(cachePath);
                }
            }
            else
            {
                discardCache(cachePath);
            }
        }

        if (!cudaEngine)
        {
            std::string readError;
            std::vector<char> onnxBytes = readBinaryFile(config.modelPath, readError);
            if (onnxBytes.empty())
                return fail(readError);

            TrtPtr<nvinfer1::INetworkDefinition> network{builder->createNetworkV2(0)};
            if (!network)
                return fail("createNetworkV2 failed");

            TrtPtr<nvonnxparser::IParser> parser{
                nvonnxparser::createParser(*network, m_impl->logger)};
            if (!parser)
                return fail("createParser failed");

            const std::string modelPath = config.modelPath.string();
            if (!parser->parse(onnxBytes.data(), onnxBytes.size(), modelPath.c_str()))
            {
                std::string message = parserErrors(*parser);
                if (message.empty())
                    message = m_impl->logger.messages();
                if (message.empty())
                    message = joinError("ONNX parse failed: ", config.modelPath);
                return fail(std::move(message));
            }

            if (network->getNbInputs() != 1)
                return fail("TensorRTEngine expects exactly one model input");

            nvinfer1::ITensor* inputTensor = network->getInput(0);
            if (inputTensor == nullptr || inputTensor->getName() == nullptr)
                return fail("model input tensor is missing");
            if (inputTensor->getType() != nvinfer1::DataType::kFLOAT)
                return fail("TensorRTEngine expects float32 input");

            const nvinfer1::Dims inputDims = inputTensor->getDimensions();
            if (hasDynamicDim(inputDims))
                return fail("dynamic input shape is not supported");
            if (inputDims.nbDims != 4 || inputDims.d[0] != 1 || inputDims.d[1] != 3
                || inputDims.d[2] != config.inputHeight || inputDims.d[3] != config.inputWidth)
            {
                return fail(
                    "input must be static 1x3xHxW matching ModelConfig.inputHeight/inputWidth");
            }

            if (network->getNbOutputs() < 1)
                return fail("model has no outputs");

            for (int32_t i = 0; i < network->getNbOutputs(); ++i)
            {
                nvinfer1::ITensor* outputTensor = network->getOutput(i);
                if (outputTensor == nullptr || outputTensor->getName() == nullptr)
                    return fail("model output tensor is missing");
                if (outputTensor->getType() != nvinfer1::DataType::kFLOAT)
                    return fail("TensorRTEngine expects float32 outputs");
                if (hasDynamicDim(outputTensor->getDimensions()))
                    return fail("dynamic output shape is not supported");
            }

            TrtPtr<nvinfer1::IBuilderConfig> builderConfig{builder->createBuilderConfig()};
            if (!builderConfig)
                return fail("createBuilderConfig failed");
            if (config.precision == InferencePrecision::Fp16)
                builderConfig->setFlag(nvinfer1::BuilderFlag::kFP16);

            TrtPtr<nvinfer1::IHostMemory> plan{
                builder->buildSerializedNetwork(*network, *builderConfig)};
            if (!plan || plan->data() == nullptr || plan->size() == 0)
            {
                std::string message = m_impl->logger.messages();
                if (message.empty())
                    message = "buildSerializedNetwork failed";
                return fail(std::move(message));
            }

            std::string persistError;
            if (!persistCache(cachePath, plan->data(), plan->size(), persistError))
                return fail(std::move(persistError));

            cudaEngine.reset(runtime->deserializeCudaEngine(plan->data(), plan->size()));
            if (!cudaEngine)
                return fail("deserializeCudaEngine failed");

            std::string inspectError;
            if (!readEngineIo(*cudaEngine, config, io, inspectError))
                return fail(std::move(inspectError));
        }

        TrtPtr<nvinfer1::IExecutionContext> context{cudaEngine->createExecutionContext()};
        if (!context)
            return fail("createExecutionContext failed");

        auto inputBuffer = std::make_unique<CudaDeviceBuffer>(
            static_cast<std::size_t>(elementCount(io.inputMeta.shape)) * sizeof(float));
        if (!inputBuffer->valid())
            return fail("input CUDA buffer: " + inputBuffer->lastError());

        std::vector<std::unique_ptr<CudaDeviceBuffer>> outputBuffers;
        outputBuffers.reserve(io.outputNames.size());
        for (const TensorMetadata& meta : io.outputMeta)
        {
            const std::int64_t count = elementCount(meta.shape);
            if (count <= 0)
                return fail("output element count is invalid");
            auto buffer = std::make_unique<CudaDeviceBuffer>(
                static_cast<std::size_t>(count) * sizeof(float));
            if (!buffer->valid())
                return fail("output CUDA buffer: " + buffer->lastError());
            outputBuffers.push_back(std::move(buffer));
        }

        m_impl->inputName = std::move(io.inputName);
        m_impl->outputNames = std::move(io.outputNames);
        m_impl->inputMeta = std::move(io.inputMeta);
        m_impl->outputMeta = std::move(io.outputMeta);
        m_impl->stream = std::move(stream);
        m_impl->inputBuffer = std::move(inputBuffer);
        m_impl->outputBuffers = std::move(outputBuffers);
        m_impl->context = std::move(context);
        m_impl->engine = std::move(cudaEngine);
        m_impl->runtime = std::move(runtime);
        m_impl->ready = true;
        m_impl->error.clear();
        return true;
    }
    catch (const std::exception& ex)
    {
        m_impl->reset();
        m_impl->error = ex.what();
        return false;
    }
}

bool TensorRTEngine::isReady() const
{
    return m_impl->ready;
}

std::string TensorRTEngine::lastError() const
{
    return m_impl->error;
}

std::string TensorRTEngine::backendId() const
{
    return "tensorrt";
}

DeviceInfo TensorRTEngine::device() const
{
    DeviceInfo info;
    info.name = "CUDA";
    info.deviceId = m_impl->deviceId;
    info.gpu = true;
    return info;
}

TensorMetadata TensorRTEngine::inputMetadata() const
{
    return m_impl->inputMeta;
}

std::vector<TensorMetadata> TensorRTEngine::outputMetadata() const
{
    return m_impl->outputMeta;
}

InferResult TensorRTEngine::infer(const TensorView& input)
{
    InferResult result;
    if (!m_impl->ready || !m_impl->context || !m_impl->stream || !m_impl->inputBuffer)
    {
        result.error = m_impl->error.empty() ? "engine not ready" : m_impl->error;
        return result;
    }

    if (input.shape != m_impl->inputMeta.shape
        || static_cast<std::int64_t>(input.data.size()) != elementCount(input.shape))
    {
        result.error = "input shape does not match model";
        return result;
    }

    try
    {
        const std::size_t inputBytes = input.data.size() * sizeof(float);
        if (!m_impl->inputBuffer->copyFromHost(input.data.data(), inputBytes, m_impl->stream->get()))
        {
            result.error = m_impl->inputBuffer->lastError();
            return result;
        }

        if (!m_impl->context->setTensorAddress(m_impl->inputName.c_str(), m_impl->inputBuffer->data()))
        {
            result.error = "setTensorAddress failed for input " + m_impl->inputName;
            return result;
        }
        for (std::size_t i = 0; i < m_impl->outputNames.size(); ++i)
        {
            if (!m_impl->context->setTensorAddress(
                    m_impl->outputNames[i].c_str(), m_impl->outputBuffers[i]->data()))
            {
                result.error = "setTensorAddress failed for output " + m_impl->outputNames[i];
                return result;
            }
        }

        const auto started = std::chrono::steady_clock::now();
        if (!m_impl->context->enqueueV3(m_impl->stream->get()))
        {
            result.error = "enqueueV3 failed";
            const std::string logged = m_impl->logger.messages();
            if (!logged.empty())
                result.error += ": " + logged;
            return result;
        }

        const cudaError_t computeSync = cudaStreamSynchronize(m_impl->stream->get());
        if (computeSync != cudaSuccess)
        {
            result.error = std::string("cudaStreamSynchronize after enqueue failed: ")
                + cudaGetErrorString(computeSync);
            return result;
        }

        result.outputs.resize(m_impl->outputNames.size());
        for (std::size_t i = 0; i < m_impl->outputNames.size(); ++i)
        {
            TensorView& view = result.outputs[i];
            view.shape = m_impl->outputMeta[i].shape;
            const std::int64_t count = elementCount(view.shape);
            view.data.resize(static_cast<std::size_t>(count));
            if (!m_impl->outputBuffers[i]->copyToHost(
                    view.data.data(), view.data.size() * sizeof(float), m_impl->stream->get()))
            {
                result.error = m_impl->outputBuffers[i]->lastError();
                result.outputs.clear();
                return result;
            }
        }

        const cudaError_t copySync = cudaStreamSynchronize(m_impl->stream->get());
        const auto elapsed = std::chrono::steady_clock::now() - started;
        result.latencyMs = std::chrono::duration<double, std::milli>(elapsed).count();
        if (copySync != cudaSuccess)
        {
            result.error = std::string("cudaStreamSynchronize after D2H failed: ")
                + cudaGetErrorString(copySync);
            result.outputs.clear();
            return result;
        }

        result.ok = true;
        return result;
    }
    catch (const std::exception& ex)
    {
        m_impl->reset();
        m_impl->error = ex.what();
        result.error = m_impl->error;
        result.outputs.clear();
        return result;
    }
}

void TensorRTEngine::warmup(int iterations)
{
    if (!m_impl->ready || iterations <= 0)
        return;

    TensorView zeros;
    zeros.shape = m_impl->inputMeta.shape;
    const std::int64_t count = elementCount(zeros.shape);
    if (count <= 0)
        return;
    zeros.data.assign(static_cast<std::size_t>(count), 0.0F);
    for (int i = 0; i < iterations; ++i)
        infer(zeros);
}

} // namespace visionlab
