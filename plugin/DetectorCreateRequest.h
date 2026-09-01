#ifndef DETECTORCREATEREQUEST_H
#define DETECTORCREATEREQUEST_H

#include <filesystem>

#include "inference/InferenceTypes.h"

namespace visionlab {

// 插件创建 IDetector 时需要的宿主上下文。
// YOLO 插件把 backend / precision / deviceId / 阈值写入 ModelConfig 后交给
// createInferenceEngine；Face / Motion / Dummy 忽略后端与阈值。
struct DetectorCreateRequest
{
    std::filesystem::path modelDir;
    InferenceBackend backend = InferenceBackend::OnnxRuntimeCpu;
    InferencePrecision precision = InferencePrecision::Fp32;
    int deviceId = 0;
    float confidenceThreshold = 0.25F;
    float nmsThreshold = 0.45F;
};

} // namespace visionlab

#endif // DETECTORCREATEREQUEST_H
