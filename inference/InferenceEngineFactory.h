#ifndef INFERENCEENGINEFACTORY_H
#define INFERENCEENGINEFACTORY_H

#include <memory>
#include <optional>
#include <string_view>

#include "inference/IInferenceEngine.h"
#include "inference/ModelConfig.h"

namespace visionlab {

// 按 ModelConfig.backend 创建引擎。永不返回 nullptr。
// CUDA / TensorRT 在对应实现落地前返回会 initialize 失败的占位引擎。
std::unique_ptr<IInferenceEngine> createInferenceEngine(const ModelConfig& config);

std::optional<InferenceBackend> parseInferenceBackend(std::string_view text);
std::optional<InferencePrecision> parseInferencePrecision(std::string_view text);

} // namespace visionlab

#endif // INFERENCEENGINEFACTORY_H
