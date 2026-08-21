#include "InferenceEngineFactory.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "inference/InferenceSelection.h"
#include "inference/OnnxRuntimeCudaEngine.h"
#include "inference/OnnxRuntimeEngine.h"

namespace visionlab {

namespace {

std::string asciiLower(std::string_view text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

class UnavailableInferenceEngine final : public IInferenceEngine
{
public:
    UnavailableInferenceEngine(std::string backendId, std::string displayName)
        : m_backendId(std::move(backendId))
        , m_error(std::move(displayName) + " is not available")
    {
    }

    bool initialize(const ModelConfig&) override
    {
        return false;
    }

    bool isReady() const override { return false; }
    std::string lastError() const override { return m_error; }
    std::string backendId() const override { return m_backendId; }

    DeviceInfo device() const override
    {
        DeviceInfo info;
        info.name = m_backendId;
        info.gpu = true;
        return info;
    }

    TensorMetadata inputMetadata() const override { return {}; }
    std::vector<TensorMetadata> outputMetadata() const override { return {}; }

    InferResult infer(const TensorView&) override
    {
        InferResult result;
        result.error = m_error;
        return result;
    }

    void warmup(int) override {}

private:
    std::string m_backendId;
    std::string m_error;
};

} // namespace

std::unique_ptr<IInferenceEngine> createInferenceEngine(const ModelConfig& config)
{
    switch (config.backend)
    {
    case InferenceBackend::OnnxRuntimeCpu:
        return std::make_unique<OnnxRuntimeEngine>();
    case InferenceBackend::OnnxRuntimeCuda:
        return std::make_unique<OnnxRuntimeCudaEngine>();
    case InferenceBackend::TensorRT:
        return std::make_unique<UnavailableInferenceEngine>("tensorrt", "TensorRT");
    }

    return std::make_unique<UnavailableInferenceEngine>("unknown", "requested backend");
}

std::optional<InferenceBackend> parseInferenceBackend(std::string_view text)
{
    const std::string key = asciiLower(text);
    if (key == "onnx-cpu" || key == "onnxruntime-cpu")
        return InferenceBackend::OnnxRuntimeCpu;
    if (key == "onnx-cuda" || key == "onnxruntime-cuda")
        return InferenceBackend::OnnxRuntimeCuda;
    if (key == "tensorrt" || key == "trt")
        return InferenceBackend::TensorRT;
    return std::nullopt;
}

std::optional<InferencePrecision> parseInferencePrecision(std::string_view text)
{
    const std::string key = asciiLower(text);
    if (key == "fp32")
        return InferencePrecision::Fp32;
    if (key == "fp16")
        return InferencePrecision::Fp16;
    return std::nullopt;
}

InferenceSelection inferenceSelectionFromEnv()
{
    InferenceSelection selection;

    if (const char* backend = std::getenv("VISIONLAB_INFERENCE_BACKEND"))
    {
        if (const auto parsed = parseInferenceBackend(backend))
            selection.backend = *parsed;
    }
    if (const char* precision = std::getenv("VISIONLAB_INFERENCE_PRECISION"))
    {
        if (const auto parsed = parseInferencePrecision(precision))
            selection.precision = *parsed;
    }
    if (const char* device = std::getenv("VISIONLAB_INFERENCE_DEVICE"))
    {
        char* end = nullptr;
        const long parsed = std::strtol(device, &end, 10);
        if (end != device && *end == '\0' && parsed >= 0 && parsed <= 1'000'000)
            selection.deviceId = static_cast<int>(parsed);
    }

    return selection;
}

} // namespace visionlab
