#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "detectors/IDetector.h"
#include "plugin/DetectorCreateRequest.h"
#include "plugin/PluginMetadata.h"

namespace visionlab {

// 扫描插件目录、校验 IID/版本/重复 id、创建 IDetector。
// 非 QObject、非线程安全：只在 GUI 线程、管线 start() 之前调用。
// V1 不提供 unload()；QPluginLoader 活到本对象析构。scan() 只替换当前扫描结果，
// 不销毁已加载的 loader，避免 CameraManager 重建管线时卸掉仍被 backup 持有的 IDetector。
class PluginManager
{
public:
    PluginManager();
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    void scan(const std::filesystem::path& directory);
    std::vector<PluginMetadata> metadata() const;
    std::vector<std::string> errors() const;

    std::unique_ptr<IDetector> createDetector(const std::string& pluginId,
                                              const DetectorCreateRequest& request);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace visionlab

#endif // PLUGINMANAGER_H
