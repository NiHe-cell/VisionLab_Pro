#ifndef DETECTION_H
#define DETECTION_H

#include <string>

#include <opencv2/core.hpp>

namespace visionlab {

// 单条结构化检测结果，坐标为原始帧像素坐标。
// 纯领域类型：不依赖 Qt，不涉及渲染。
struct Detection
{
    // 检测器类别表中的索引；运动区域使用 kMotionClassId，无类别语义。
    int classId = -1;

    // 可读的类别标签（如 "person"、"face"、"motion"）。
    std::string label;

    // 置信度，取值 [0, 1]（检测器能提供时）。
    float confidence = 0.0F;

    // 包围框，原始帧像素坐标。
    cv::Rect box;
};

} // namespace visionlab

#endif // DETECTION_H
