#ifndef ONNXRUNTIMEENGINE_H
#define ONNXRUNTIMEENGINE_H

#include <memory>
#include <string>
#include <vector>

#include "inference/IInferenceEngine.h"

namespace visionlab {

// ONNX Runtime CPU 后端。ORT 类型藏在 Impl 里，公共头不 include onnxruntime。
class OnnxRuntimeEngine final : public IInferenceEngine
{
public:
    OnnxRuntimeEngine();
    ~OnnxRuntimeEngine() override;

    OnnxRuntimeEngine(const OnnxRuntimeEngine&) = delete;
    OnnxRuntimeEngine& operator=(const OnnxRuntimeEngine&) = delete;

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

#endif // ONNXRUNTIMEENGINE_H
