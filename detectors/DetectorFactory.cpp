#include "DetectorFactory.h"

#include <filesystem>

#include "FaceDetector.h"
#include "MotionDetector.h"
#include "ObjectDetector.h"

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
        return std::make_unique<ObjectDetector>(
            joinPath(modelDir, "yolov4-tiny.cfg"),
            joinPath(modelDir, "yolov4-tiny.weights"),
            joinPath(modelDir, "coco.names"));
    case DetectionMode::Motion:
        return std::make_unique<MotionDetector>();
    case DetectionMode::None:
    default:
        return nullptr;
    }
}

} // namespace visionlab
