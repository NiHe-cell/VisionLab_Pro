#include "FaceVisionPlugin.h"

#include <filesystem>

#include "core/VisionTypes.h"
#include "detectors/FaceDetector.h"

visionlab::PluginMetadata FaceVisionPlugin::metadata() const
{
    visionlab::PluginMetadata meta;
    meta.id = "vision.face";
    meta.name = "Res10-SSD Face";
    meta.version = "1.0.0";
    meta.description = "OpenCV DNN Res10 SSD face detector";
    meta.capabilities = {"detect"};
    meta.interfaceVersion = 1;
    meta.mode = visionlab::DetectionMode::Face;
    return meta;
}

std::unique_ptr<visionlab::IDetector> FaceVisionPlugin::createDetector(
    const visionlab::DetectorCreateRequest& request)
{
    const auto proto = (request.modelDir / "deploy.prototxt").string();
    const auto model =
        (request.modelDir / "res10_300x300_ssd_iter_140000.caffemodel").string();
    return std::make_unique<visionlab::FaceDetector>(proto, model);
}
