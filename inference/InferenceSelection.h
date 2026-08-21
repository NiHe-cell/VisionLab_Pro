#ifndef INFERENCESELECTION_H
#define INFERENCESELECTION_H

#include "inference/InferenceTypes.h"

namespace visionlab {

// 应用层推理后端选择。非法环境变量保持默认 CPU / Fp32 / device 0。
struct InferenceSelection
{
    InferenceBackend backend = InferenceBackend::OnnxRuntimeCpu;
    InferencePrecision precision = InferencePrecision::Fp32;
    int deviceId = 0;
};

// 读取 VISIONLAB_INFERENCE_BACKEND / PRECISION / DEVICE。不读 Qt，不做日志。
InferenceSelection inferenceSelectionFromEnv();

} // namespace visionlab

#endif // INFERENCESELECTION_H
