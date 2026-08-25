#ifndef PLUGINMETADATA_H
#define PLUGINMETADATA_H

#include <optional>
#include <string>
#include <vector>

#include "core/VisionTypes.h"

namespace visionlab {

// 检测器插件的自描述信息。与推理后端无关。
//
// interfaceVersion 必须为 1；不匹配的插件由 PluginManager 拒绝加载。
// Dummy 插件的 mode 为空：不进入生产管线的 DetectionMode 映射。
// Face / Object / Motion 插件填写对应枚举，供 CameraManager 按模式装配。
struct PluginMetadata
{
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> capabilities;
    int interfaceVersion = 1;
    std::optional<DetectionMode> mode;
};

} // namespace visionlab

#endif // PLUGINMETADATA_H
