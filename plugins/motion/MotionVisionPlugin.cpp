#include "MotionVisionPlugin.h"

#include "core/VisionTypes.h"
#include "detectors/MotionDetector.h"

visionlab::PluginMetadata MotionVisionPlugin::metadata() const
{
    visionlab::PluginMetadata meta;
    meta.id = "vision.motion";
    meta.name = "MOG2 Motion";
    meta.version = "1.0.0";
    meta.description = "OpenCV MOG2 motion regions";
    meta.capabilities = {"detect"};
    meta.interfaceVersion = 1;
    meta.mode = visionlab::DetectionMode::Motion;
    return meta;
}

std::unique_ptr<visionlab::IDetector> MotionVisionPlugin::createDetector(
    const visionlab::DetectorCreateRequest&)
{
    return std::make_unique<visionlab::MotionDetector>();
}
