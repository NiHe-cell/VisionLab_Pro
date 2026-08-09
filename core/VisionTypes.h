#ifndef VISIONTYPES_H
#define VISIONTYPES_H

#include <cstdint>
#include <string_view>

namespace visionlab {

// 处理管线的检测算法选择。
// 用于替代旧的字符串模式切换（"Face Detection" 等）。
// 枚举值保持稳定；只在应用边界处与 UI 字符串互转。
enum class DetectionMode : std::uint8_t
{
    None,
    Face,
    Object,
    Motion,
};

// MotionDetector 运动区域的保留类别 id：运动区域不是分类目标，
// 有意取负数，避免与 COCO 等真实类别索引冲突。
inline constexpr int kMotionClassId = -1;

// UI 展示字符串 ↔ 模式枚举的映射。
// 仅在应用边界（控制器层）使用；核心与检测器不感知这些字符串。
inline constexpr std::string_view kFaceModeLabel = "Face Detection";
inline constexpr std::string_view kObjectModeLabel = "Object Detection";
inline constexpr std::string_view kMotionModeLabel = "Motion Detection";

inline std::string_view labelForDetectionMode(DetectionMode mode)
{
    switch (mode)
    {
    case DetectionMode::Face:
        return kFaceModeLabel;
    case DetectionMode::Object:
        return kObjectModeLabel;
    case DetectionMode::Motion:
        return kMotionModeLabel;
    case DetectionMode::None:
    default:
        return "None";
    }
}

// 未识别的标签安全落到 None（不产生任何检测行为）。
inline DetectionMode detectionModeFromLabel(std::string_view label)
{
    if (label == kFaceModeLabel)
        return DetectionMode::Face;
    if (label == kObjectModeLabel)
        return DetectionMode::Object;
    if (label == kMotionModeLabel)
        return DetectionMode::Motion;
    return DetectionMode::None;
}

} // namespace visionlab

#endif // VISIONTYPES_H
