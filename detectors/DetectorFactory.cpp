#include "DetectorFactory.h"

#include <filesystem>
#include <utility>

#include "FaceDetector.h"
#include "MotionDetector.h"
#include "YoloDetector.h"
#include "inference/ModelConfig.h"
#include "inference/OnnxRuntimeEngine.h"

namespace visionlab {

namespace {

std::string joinPath(const std::string& dir, const char* file)
{
    return (std::filesystem::path(dir) / file).string();
}

} // namespace

std::unique_ptr<IDetector> createDetector(DetectionMode mode, const std::string& modelDir)
{
    switch (mode)
    {
    case DetectionMode::Face:
        return std::make_unique<FaceDetector>(
            joinPath(modelDir, "deploy.prototxt"),
            joinPath(modelDir, "res10_300x300_ssd_iter_140000.caffemodel"));
    case DetectionMode::Object:
    {
        ModelConfig config;
        config.modelPath = std::filesystem::path(modelDir) / "yolov4-tiny.onnx";
        config.classNamesPath = std::filesystem::path(modelDir) / "coco.names";
        config.inputWidth = 320;
        config.inputHeight = 320;
        config.backend = InferenceBackend::OnnxRuntimeCpu;
        config.precision = InferencePrecision::Fp32;
        return std::make_unique<YoloDetector>(
            std::make_unique<OnnxRuntimeEngine>(), std::move(config));
    }
    case DetectionMode::Motion:
        return std::make_unique<MotionDetector>();
    case DetectionMode::None:
    default:
        return nullptr;
    }
}

} // namespace visionlab
