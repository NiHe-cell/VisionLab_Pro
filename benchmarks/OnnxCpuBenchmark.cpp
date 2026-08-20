#include "core/LatencyWindow.h"
#include "inference/OnnxRuntimeEngine.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Options
{
    int warmup = 5;
    int iters = 50;
    std::filesystem::path model{VISIONLAB_DEFAULT_MODEL};
};

void printUsage(std::ostream& out)
{
    out << "Usage: bench_onnx_cpu [--warmup N] [--iters N] [--model path.onnx] [path.onnx]\n";
}

bool parsePositiveInt(const char* text, int& value)
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
            if (!parsePositiveInt(value, options.warmup))
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
            if (!parsePositiveInt(value, options.iters) || options.iters < 1)
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
    config.backend = visionlab::InferenceBackend::OnnxRuntimeCpu;
    config.precision = visionlab::InferencePrecision::Fp32;

    visionlab::OnnxRuntimeEngine engine;
    if (!engine.initialize(config) || !engine.isReady())
    {
        std::cerr << "initialize failed: " << engine.lastError() << '\n';
        return 1;
    }

    visionlab::TensorView input;
    input.shape = engine.inputMetadata().shape;
    const std::size_t count = elementCount(input.shape);
    if (count == 0)
    {
        std::cerr << "invalid input shape from model metadata\n";
        return 1;
    }
    input.data.assign(count, 0.0F);

    engine.warmup(options.warmup);

    visionlab::LatencyWindow window(static_cast<std::size_t>(options.iters));
    for (int i = 0; i < options.iters; ++i)
    {
        const visionlab::InferResult result = engine.infer(input);
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
    std::cout << "backend: " << engine.backendId() << '\n';
    std::cout << "warmup: " << options.warmup << '\n';
    std::cout << "iters: " << options.iters << '\n';
    std::cout << "mean_ms: " << meanMs << '\n';
    std::cout << "p50_ms: " << window.percentile(50.0) << '\n';
    std::cout << "p95_ms: " << window.percentile(95.0) << '\n';
    std::cout << "throughput_fps: " << throughputFps << '\n';
    if (std::string_view{VISIONLAB_BUILD_TYPE} == "Debug")
        std::cout << "note: Debug builds are much slower than Release\n";

    return 0;
}
