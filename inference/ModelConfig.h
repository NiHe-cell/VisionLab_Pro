#ifndef MODELCONFIG_H
#define MODELCONFIG_H

#include <filesystem>

#include "inference/InferenceTypes.h"

namespace visionlab {

// 检测器与引擎共用的模型配置。
//
// 引擎 initialize 只消费：modelPath / backend / deviceId / precision。
// confidence、NMS、classNames、input 尺寸由检测器读取。
struct ModelConfig
{
    std::filesystem::path modelPath;
    int inputWidth = 320;
    int inputHeight = 320;
    float confidenceThreshold = 0.25F;
    float nmsThreshold = 0.45F;
    InferenceBackend backend = InferenceBackend::OnnxRuntimeCpu;
    int deviceId = 0;
    std::filesystem::path classNamesPath;
    InferencePrecision precision = InferencePrecision::Fp32;
};

} // namespace visionlab

#endif // MODELCONFIG_H
