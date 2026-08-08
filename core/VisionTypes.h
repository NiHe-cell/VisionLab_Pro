#ifndef VISIONTYPES_H
#define VISIONTYPES_H

#include <cstdint>

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

} // namespace visionlab

#endif // VISIONTYPES_H
