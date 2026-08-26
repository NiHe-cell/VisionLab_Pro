#ifndef FACEVISIONPLUGIN_H
#define FACEVISIONPLUGIN_H

#include <memory>

#include <QObject>

#include "plugin/IVisionPlugin.h"

class FaceVisionPlugin : public QObject, public visionlab::IVisionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID VisionLab_IVisionPlugin_iid)
    Q_INTERFACES(visionlab::IVisionPlugin)

public:
    visionlab::PluginMetadata metadata() const override;
    std::unique_ptr<visionlab::IDetector> createDetector(
        const visionlab::DetectorCreateRequest& request) override;
};

#endif // FACEVISIONPLUGIN_H
