#ifndef IINFERENCEENGINE_H
#define IINFERENCEENGINE_H

#include <string>
#include <vector>

#include "inference/InferenceTypes.h"
#include "inference/ModelConfig.h"

namespace visionlab {

// 推理运行时端口：加载模型、执行张量、报告后端与错误。
//
// 约定：
// - 不暴露 ONNX Runtime / TensorRT / cv::dnn 类型。
// - initialize 失败时 isReady()==false，lastError() 非空。
// - 同一实例不承诺可并发 infer；仅由一条推理工作线程调用（与 IDetector 相同）。
// - InferResult.outputs 为自有缓冲，返回后不依赖 Session 内部指针。
class IInferenceEngine
{
public:
    virtual ~IInferenceEngine() = default;

    virtual bool initialize(const ModelConfig& config) = 0;
    virtual bool isReady() const = 0;
    virtual std::string lastError() const = 0;

    virtual std::string backendId() const = 0;
    virtual DeviceInfo device() const = 0;
    virtual TensorMetadata inputMetadata() const = 0;
    virtual std::vector<TensorMetadata> outputMetadata() const = 0;

    virtual InferResult infer(const TensorView& input) = 0;
    virtual void warmup(int iterations) = 0;
};

} // namespace visionlab

#endif // IINFERENCEENGINE_H
