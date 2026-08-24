#include "core/LatencyWindow.h"
#include "inference/InferenceEngineFactory.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef VISIONLAB_HAS_TENSORRT
#include <cuda_runtime_api.h>
#endif

namespace {

struct Options
{
    int warmup = 5;
    int iters = 50;
    std::filesystem::path model{VISIONLAB_DEFAULT_MODEL};
    visionlab::InferenceBackend backend = visionlab::InferenceBackend::OnnxRuntimeCpu;
    visionlab::InferencePrecision precision = visionlab::InferencePrecision::Fp32;
    int deviceId = 0;
    int inputWidth = 320;
    int inputHeight = 320;
};

void printUsage(std::ostream& out)
{
    out << "Usage: bench_onnx_cpu [--backend onnx-cpu|onnx-cuda|tensorrt]\n"
           "                      [--precision fp32|fp16] [--device N]\n"
           "                      [--input-width W] [--input-height H]\n"
           "                      [--warmup N] [--iters N] [--model path.onnx] [path.onnx]\n"
           "Compare backends with four separate processes; do not run all four in one process.\n"
           "Optional gpu_mem_mb is device memory used (total-free from cudaMemGetInfo) in MiB\n"
           "after the timed loop; omitted on CPU and if the CUDA query fails.\n";
}

bool parseNonNegativeInt(const char* text, int& value)
{
    if (text == nullptr || *text == '\0')
        return false;
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (end == text || *end != '\0')
        return false;
    if (parsed < 0 || parsed > 1'000'000)
        return false;
    value = static_cast<int>(parsed);
    return true;
}

bool parseArgs(int argc, char** argv, Options& options, std::string& error)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        const auto requireValue = [&](const char* name) -> const char* {
            if (i + 1 >= argc)
            {
                error = std::string(name) + " requires a value";
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h")
        {
            printUsage(std::cout);
            std::exit(0);
        }
        if (arg == "--warmup")
        {
            const char* value = requireValue("--warmup");
            if (value == nullptr)
                return false;
            if (!parseNonNegativeInt(value, options.warmup))
            {
                error = "invalid --warmup";
                return false;
            }
            continue;
        }
        if (arg == "--iters")
        {
            const char* value = requireValue("--iters");
            if (value == nullptr)
                return false;
            if (!parseNonNegativeInt(value, options.iters) || options.iters < 1)
            {
                error = "invalid --iters";
                return false;
            }
            continue;
        }
        if (arg == "--model")
        {
            const char* value = requireValue("--model");
            if (value == nullptr)
                return false;
            options.model = value;
            continue;
        }
        if (arg == "--backend")
        {
            const char* value = requireValue("--backend");
            if (value == nullptr)
                return false;
            const auto parsed = visionlab::parseInferenceBackend(value);
            if (!parsed)
            {
                error = "unknown --backend: " + std::string(value);
                return false;
            }
            options.backend = *parsed;
            continue;
        }
        if (arg == "--precision")
        {
            const char* value = requireValue("--precision");
            if (value == nullptr)
                return false;
            const auto parsed = visionlab::parseInferencePrecision(value);
            if (!parsed)
            {
                error = "invalid --precision: " + std::string(value);
                return false;
            }
            options.precision = *parsed;
            continue;
        }
        if (arg == "--device")
        {
            const char* value = requireValue("--device");
            if (value == nullptr)
                return false;
            if (!parseNonNegativeInt(value, options.deviceId))
            {
                error = "invalid --device";
                return false;
            }
            continue;
        }
        if (arg == "--input-width")
        {
            const char* value = requireValue("--input-width");
            if (value == nullptr)
                return false;
            if (!parseNonNegativeInt(value, options.inputWidth) || options.inputWidth < 1)
            {
                error = "invalid --input-width";
                return false;
            }
            continue;
        }
        if (arg == "--input-height")
        {
            const char* value = requireValue("--input-height");
            if (value == nullptr)
                return false;
            if (!parseNonNegativeInt(value, options.inputHeight) || options.inputHeight < 1)
            {
                error = "invalid --input-height";
                return false;
            }
            continue;
        }
        if (!arg.empty() && arg.front() == '-')
        {
            error = "unknown option: " + arg;
            return false;
        }
        options.model = arg;
    }

    if (options.iters < 1)
    {
        error = "--iters must be >= 1";
        return false;
    }
    return true;
}

std::size_t elementCount(const std::vector<std::int64_t>& shape)
{
    std::size_t count = 1;
    for (const std::int64_t dim : shape)
    {
        if (dim <= 0)
            return 0;
        count *= static_cast<std::size_t>(dim);
    }
    return count;
}

bool isGpuBackend(visionlab::InferenceBackend backend)
{
    return backend == visionlab::InferenceBackend::OnnxRuntimeCuda
           || backend == visionlab::InferenceBackend::TensorRT;
}

std::optional<double> usedGpuMemMb(int deviceId)
{
#ifdef VISIONLAB_HAS_TENSORRT
    if (cudaSetDevice(deviceId) != cudaSuccess)
        return std::nullopt;
    std::size_t freeBytes = 0;
    std::size_t totalBytes = 0;
    if (cudaMemGetInfo(&freeBytes, &totalBytes) != cudaSuccess)
        return std::nullopt;
    if (totalBytes < freeBytes)
        return std::nullopt;
    return static_cast<double>(totalBytes - freeBytes) / (1024.0 * 1024.0);
#else
    (void)deviceId;
    return std::nullopt;
#endif
}

} // namespace

int main(int argc, char** argv)
{
    Options options;
    std::string error;
    if (!parseArgs(argc, argv, options, error))
    {
        std::cerr << error << '\n';
        printUsage(std::cerr);
        return 2;
    }

    if (!std::filesystem::exists(options.model))
    {
        std::cerr << "model file not found: " << options.model.string() << '\n';
        return 1;
    }

    visionlab::ModelConfig config;
    config.modelPath = options.model;
    config.backend = options.backend;
    config.precision = options.precision;
    config.deviceId = options.deviceId;
    config.inputWidth = options.inputWidth;
    config.inputHeight = options.inputHeight;

    const std::unique_ptr<visionlab::IInferenceEngine> engine =
        visionlab::createInferenceEngine(config);
    if (!engine->initialize(config) || !engine->isReady())
    {
        std::cerr << "initialize failed: " << engine->lastError() << '\n';
        return 1;
    }

    visionlab::TensorView input;
    input.shape = engine->inputMetadata().shape;
    const std::size_t count = elementCount(input.shape);
    if (count == 0)
    {
        std::cerr << "invalid input shape from model metadata\n";
        return 1;
    }
    input.data.assign(count, 0.0F);

    engine->warmup(options.warmup);

    visionlab::LatencyWindow window(static_cast<std::size_t>(options.iters));
    for (int i = 0; i < options.iters; ++i)
    {
        const visionlab::InferResult result = engine->infer(input);
        if (!result.ok)
        {
            std::cerr << "infer failed: " << result.error << '\n';
            return 1;
        }
        window.record(result.latencyMs);
    }

    const double meanMs = window.mean();
    const double throughputFps = (meanMs > 0.0) ? (1000.0 / meanMs) : 0.0;

    std::cout << "build: " << VISIONLAB_BUILD_TYPE << '\n';
    std::cout << "model: " << options.model.string() << '\n';
    std::cout << "backend: " << engine->backendId() << '\n';
    std::cout << "warmup: " << options.warmup << '\n';
    std::cout << "iters: " << options.iters << '\n';
    std::cout << "mean_ms: " << meanMs << '\n';
    std::cout << "p50_ms: " << window.percentile(50.0) << '\n';
    std::cout << "p95_ms: " << window.percentile(95.0) << '\n';
    std::cout << "p99_ms: " << window.percentile(99.0) << '\n';
    std::cout << "throughput_fps: " << throughputFps << '\n';
    if (isGpuBackend(options.backend))
    {
        if (const std::optional<double> gpuMemMb = usedGpuMemMb(options.deviceId))
            std::cout << "gpu_mem_mb: " << *gpuMemMb << '\n';
    }
    if (std::string_view{VISIONLAB_BUILD_TYPE} == "Debug")
        std::cout << "note: Debug builds are much slower than Release\n";

    return 0;
}
