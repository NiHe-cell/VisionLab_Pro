#ifndef IVISIONPLUGIN_H
#define IVISIONPLUGIN_H

#include <memory>

#include <QtPlugin>

#include "detectors/IDetector.h"
#include "plugin/DetectorCreateRequest.h"
#include "plugin/PluginMetadata.h"

namespace visionlab {

// 运行时检测器扩展点。实现类多重继承 QObject 与本接口，
// 使用 Q_PLUGIN_METADATA(IID VisionLab_IVisionPlugin_iid) 与 Q_INTERFACES。
//
// 本头不暴露 QPluginLoader / QML。同一实例的 createDetector
// 不承诺可并发调用；由 PluginManager 在管线 start() 之前于 GUI 线程调用。
class IVisionPlugin
{
public:
    virtual ~IVisionPlugin() = default;

    virtual PluginMetadata metadata() const = 0;

    // 允许返回 nullptr（创建失败）；调用方不得把它当成就绪检测器。
    virtual std::unique_ptr<IDetector> createDetector(
        const DetectorCreateRequest& request) = 0;
};

} // namespace visionlab

#define VisionLab_IVisionPlugin_iid "com.visionlab.IVisionPlugin/1.0"
Q_DECLARE_INTERFACE(visionlab::IVisionPlugin, VisionLab_IVisionPlugin_iid)

#endif // IVISIONPLUGIN_H
