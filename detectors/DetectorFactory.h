#ifndef DETECTORFACTORY_H
#define DETECTORFACTORY_H

#include <memory>
#include <string>

#include "IDetector.h"
#include "inference/InferenceTypes.h"

namespace visionlab {

// 按模式创建具体检测器实例。modelDir 为模型文件目录（不含文件名）；
// DetectionMode::None 返回 nullptr。
//
// Object 模式的 backend / precision / deviceId 写入 ModelConfig 后交给
// createInferenceEngine。Face / Motion 忽略这三项。默认 CPU / Fp32 / 0。
//
// 这是装配层的临时集中点：Phase 5 起检测器改由插件系统发现与创建，
// 此工厂随之收缩删除。
std::unique_ptr<IDetector> createDetector(
    DetectionMode mode,
    const std::string& modelDir,
    InferenceBackend backend = InferenceBackend::OnnxRuntimeCpu,
    InferencePrecision precision = InferencePrecision::Fp32,
    int deviceId = 0);

} // namespace visionlab

#endif // DETECTORFACTORY_H
