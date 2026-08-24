#ifndef TENSORRTENGINE_H
#define TENSORRTENGINE_H

#include <memory>
#include <string>
#include <vector>

#include "inference/IInferenceEngine.h"

namespace visionlab {

// TensorRT 10 后端。NvInfer / CUDA 类型藏在 Impl 里，公共头不暴露。
// FP32 / FP16（需 platformHasFastFp16）；序列化引擎写入 ONNX 旁 .trt-cache/。
class TensorRTEngine final : public IInferenceEngine
{
public:
    TensorRTEngine();
    ~TensorRTEngine() override;

    TensorRTEngine(const TensorRTEngine&) = delete;
    TensorRTEngine& operator=(const TensorRTEngine&) = delete;

    bool initialize(const ModelConfig& config) override;
    bool isReady() const override;
    std::string lastError() const override;
    std::string backendId() const override;
    DeviceInfo device() const override;
    TensorMetadata inputMetadata() const override;
    std::vector<TensorMetadata> outputMetadata() const override;
    InferResult infer(const TensorView& input) override;
    void warmup(int iterations) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace visionlab

#endif // TENSORRTENGINE_H
