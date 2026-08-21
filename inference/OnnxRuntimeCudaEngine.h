#ifndef ONNXRUNTIMECUDAENGINE_H
#define ONNXRUNTIMECUDAENGINE_H

#include <memory>
#include <string>
#include <vector>

#include "inference/IInferenceEngine.h"

namespace visionlab {

// ONNX Runtime CUDA Execution Provider 后端。ORT / CUDA 类型藏在 Impl 里。
// 无 GPU 或无 CUDA EP 时 initialize 失败，不得改走 CPU Session。
class OnnxRuntimeCudaEngine final : public IInferenceEngine
{
public:
    OnnxRuntimeCudaEngine();
    ~OnnxRuntimeCudaEngine() override;

    OnnxRuntimeCudaEngine(const OnnxRuntimeCudaEngine&) = delete;
    OnnxRuntimeCudaEngine& operator=(const OnnxRuntimeCudaEngine&) = delete;

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

#endif // ONNXRUNTIMECUDAENGINE_H
