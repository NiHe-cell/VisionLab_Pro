#include "DummyVisionPlugin.h"

#include "detectors/DummyDetector.h"

visionlab::PluginMetadata DummyVisionPlugin::metadata() const
{
    visionlab::PluginMetadata meta;
    meta.id = "vision.dummy";
    meta.name = "Dummy Detector";
    meta.version = "1.0.0";
    meta.description = "Deterministic fake detector for plugin discovery tests";
    meta.capabilities = {"detect"};
    meta.interfaceVersion = 1;
    return meta;
}

std::unique_ptr<visionlab::IDetector> DummyVisionPlugin::createDetector(
    const visionlab::DetectorCreateRequest&)
{
    return std::make_unique<visionlab::DummyDetector>();
}
