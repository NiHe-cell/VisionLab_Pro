#ifndef INFERENCETYPES_H
#define INFERENCETYPES_H

#include <cstdint>
#include <string>
#include <vector>

namespace visionlab {

// 推理后端选择。本阶段只实现 OnnxRuntimeCpu；其余枚举留给 Phase 4，
// 不得在 CPU 引擎里假装成功。
enum class InferenceBackend : std::uint8_t
{
    OnnxRuntimeCpu,
    OnnxRuntimeCuda,
    TensorRT,
};

enum class InferencePrecision : std::uint8_t
{
    Fp32,
    Fp16,
    Int8,
};

struct DeviceInfo
{
    std::string name;
    int deviceId = 0;
    bool gpu = false;
};

struct TensorMetadata
{
    std::string name;
    std::vector<std::int64_t> shape;
    std::string dtype;
};

// 引擎无关的自有张量缓冲。infer 返回后调用方不依赖 Session 内部指针。
struct TensorView
{
    std::vector<float> data;
    std::vector<std::int64_t> shape;
};

struct InferResult
{
    bool ok = false;
    std::string error;
    std::vector<TensorView> outputs;
    double latencyMs = 0.0;
};

} // namespace visionlab

#endif // INFERENCETYPES_H
