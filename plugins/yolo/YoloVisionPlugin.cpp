#include "YoloVisionPlugin.h"

#include <utility>

#include "core/VisionTypes.h"
#include "detectors/YoloDetector.h"
#include "inference/InferenceEngineFactory.h"
#include "inference/ModelConfig.h"

visionlab::PluginMetadata YoloVisionPlugin::metadata() const
{
    visionlab::PluginMetadata meta;
    meta.id = "vision.yolo";
    meta.name = "YOLOv4-tiny";
    meta.version = "1.0.0";
    meta.description = "YOLOv4-tiny object detector via IInferenceEngine";
    meta.capabilities = {"detect"};
    meta.interfaceVersion = 1;
    meta.mode = visionlab::DetectionMode::Object;
    return meta;
}

std::unique_ptr<visionlab::IDetector> YoloVisionPlugin::createDetector(
    const visionlab::DetectorCreateRequest& request)
{
    visionlab::ModelConfig config;
    config.modelPath = request.modelDir / "yolov4-tiny.onnx";
    config.classNamesPath = request.modelDir / "coco.names";
    config.inputWidth = 320;
    config.inputHeight = 320;
    config.backend = request.backend;
    config.precision = request.precision;
    config.deviceId = request.deviceId;
    config.confidenceThreshold = request.confidenceThreshold;
    config.nmsThreshold = request.nmsThreshold;
    return std::make_unique<visionlab::YoloDetector>(
        visionlab::createInferenceEngine(config), std::move(config));
}
