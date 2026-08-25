#include "FailCreatePlugin.h"

visionlab::PluginMetadata FailCreatePlugin::metadata() const
{
    visionlab::PluginMetadata meta;
    meta.id = "vision.fail-create";
    meta.name = "Fail Create";
    meta.version = "1.0.0";
    meta.capabilities = {"detect"};
    meta.interfaceVersion = 1;
    return meta;
}

std::unique_ptr<visionlab::IDetector> FailCreatePlugin::createDetector(
    const visionlab::DetectorCreateRequest&)
{
    return nullptr;
}
