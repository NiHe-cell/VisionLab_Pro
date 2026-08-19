#include "OnnxRuntimeEngine.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <system_error>

#include <onnxruntime_cxx_api.h>

namespace visionlab {

namespace {

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

} // namespace

struct OnnxRuntimeEngine::Impl
{
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    std::string error;
    bool ready = false;

    std::string inputName;
    std::vector<std::string> outputNames;
    std::vector<const char*> inputNamePtrs;
    std::vector<const char*> outputNamePtrs;

    TensorMetadata inputMeta;
    std::vector<TensorMetadata> outputMeta;

    void reset()
    {
        ready = false;
        session.reset();
        env.reset();
        inputName.clear();
        outputNames.clear();
        inputNamePtrs.clear();
        outputNamePtrs.clear();
        inputMeta = {};
        outputMeta.clear();
    }

    void bindNamePtrs()
    {
        inputNamePtrs = {inputName.c_str()};
        outputNamePtrs.clear();
        outputNamePtrs.reserve(outputNames.size());
        for (const std::string& name : outputNames)
            outputNamePtrs.push_back(name.c_str());
    }
};

OnnxRuntimeEngine::OnnxRuntimeEngine()
    : m_impl(std::make_unique<Impl>())
{
}

OnnxRuntimeEngine::~OnnxRuntimeEngine() = default;

bool OnnxRuntimeEngine::initialize(const ModelConfig& config)
{
    m_impl->reset();

    if (config.backend != InferenceBackend::OnnxRuntimeCpu)
    {
        m_impl->error = "OnnxRuntimeEngine only supports InferenceBackend::OnnxRuntimeCpu";
        return false;
    }
    if (config.precision != InferencePrecision::Fp32)
    {
        m_impl->error = "OnnxRuntimeEngine CPU only supports InferencePrecision::Fp32";
        return false;
    }

    std::error_code existsError;
    if (config.modelPath.empty()
        || !std::filesystem::exists(config.modelPath, existsError))
    {
        m_impl->error = joinError("model file not found: ", config.modelPath);
        return false;
    }

    try
    {
        m_impl->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "visionlab");
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(1);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

        m_impl->session = std::make_unique<Ort::Session>(
            *m_impl->env, config.modelPath.c_str(), options);

        if (m_impl->session->GetInputCount() != 1)
        {
            m_impl->error = "OnnxRuntimeEngine expects exactly one model input";
            m_impl->reset();
            return false;
        }

        Ort::AllocatorWithDefaultOptions allocator;
        const auto inputName = m_impl->session->GetInputNameAllocated(0, allocator);
        m_impl->inputName = inputName.get();

        const auto inputType = m_impl->session->GetInputTypeInfo(0);
        const auto inputShapeInfo = inputType.GetTensorTypeAndShapeInfo();
        if (inputShapeInfo.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
        {
            m_impl->error = "OnnxRuntimeEngine expects float32 input";
            m_impl->reset();
            return false;
        }

        m_impl->inputMeta.name = m_impl->inputName;
        m_impl->inputMeta.shape = inputShapeInfo.GetShape();
        m_impl->inputMeta.dtype = "float32";
        if (elementCount(m_impl->inputMeta.shape) < 0)
        {
            m_impl->error = "dynamic input shape is not supported";
            m_impl->reset();
            return false;
        }

        const size_t outputCount = m_impl->session->GetOutputCount();
        if (outputCount == 0)
        {
            m_impl->error = "model has no outputs";
            m_impl->reset();
            return false;
        }

        m_impl->outputNames.reserve(outputCount);
        m_impl->outputMeta.reserve(outputCount);
        for (size_t i = 0; i < outputCount; ++i)
        {
            const auto outputName = m_impl->session->GetOutputNameAllocated(i, allocator);
            m_impl->outputNames.emplace_back(outputName.get());

            const auto outputType = m_impl->session->GetOutputTypeInfo(i);
            const auto outputShapeInfo = outputType.GetTensorTypeAndShapeInfo();
            TensorMetadata meta;
            meta.name = m_impl->outputNames.back();
            meta.shape = outputShapeInfo.GetShape();
            meta.dtype = outputShapeInfo.GetElementType() == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
                             ? "float32"
                             : "unsupported";
            m_impl->outputMeta.push_back(std::move(meta));
        }

        m_impl->bindNamePtrs();
        m_impl->ready = true;
        m_impl->error.clear();
        return true;
    }
    catch (const Ort::Exception& ex)
    {
        m_impl->reset();
        m_impl->error = ex.what();
        return false;
    }
}

bool OnnxRuntimeEngine::isReady() const
{
    return m_impl->ready;
}

std::string OnnxRuntimeEngine::lastError() const
{
    return m_impl->error;
}

std::string OnnxRuntimeEngine::backendId() const
{
    return "onnxruntime-cpu";
}

DeviceInfo OnnxRuntimeEngine::device() const
{
    DeviceInfo info;
    info.name = "CPU";
    info.deviceId = 0;
    info.gpu = false;
    return info;
}

TensorMetadata OnnxRuntimeEngine::inputMetadata() const
{
    return m_impl->inputMeta;
}

std::vector<TensorMetadata> OnnxRuntimeEngine::outputMetadata() const
{
    return m_impl->outputMeta;
}

InferResult OnnxRuntimeEngine::infer(const TensorView& input)
{
    InferResult result;
    if (!m_impl->ready || !m_impl->session)
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
        std::vector<float> inputCopy = input.data;
        const Ort::MemoryInfo memory =
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memory,
            inputCopy.data(),
            inputCopy.size(),
            input.shape.data(),
            input.shape.size());

        const auto started = std::chrono::steady_clock::now();
        auto outputs = m_impl->session->Run(
            Ort::RunOptions{nullptr},
            m_impl->inputNamePtrs.data(),
            &inputTensor,
            1,
            m_impl->outputNamePtrs.data(),
            m_impl->outputNamePtrs.size());
        const auto elapsed = std::chrono::steady_clock::now() - started;
        result.latencyMs =
            std::chrono::duration<double, std::milli>(elapsed).count();

        result.outputs.reserve(outputs.size());
        for (Ort::Value& output : outputs)
        {
            const auto info = output.GetTensorTypeAndShapeInfo();
            if (info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
            {
                result.ok = false;
                result.error = "non-float32 output is not supported";
                result.outputs.clear();
                return result;
            }

            TensorView view;
            view.shape = info.GetShape();
            const size_t count = info.GetElementCount();
            const float* data = output.GetTensorData<float>();
            view.data.assign(data, data + count);
            result.outputs.push_back(std::move(view));
        }

        result.ok = true;
        return result;
    }
    catch (const Ort::Exception& ex)
    {
        result.ok = false;
        result.error = ex.what();
        result.outputs.clear();
        return result;
    }
}

void OnnxRuntimeEngine::warmup(int iterations)
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
