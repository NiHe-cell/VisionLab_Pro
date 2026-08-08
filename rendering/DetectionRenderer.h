#ifndef DETECTIONRENDERER_H
#define DETECTIONRENDERER_H

#include <vector>

#include <opencv2/core.hpp>

#include "core/Detection.h"

namespace visionlab {

// 把结构化 Detection 绘制到帧上，使检测器与渲染彻底解耦。
//
// 所有权约定：frame 必须由调用方拥有且可写。FramePacket::image 按契约
// 是只读的，调用方在需要叠加标注时应先 clone()，再把克隆体交给本组件。
class DetectionRenderer
{
public:
    // 在 frame 上绘制包围框与标签；空帧或空列表为无操作。
    void render(cv::Mat& frame, const std::vector<Detection>& detections) const;

private:
    static cv::Scalar colorFor(const Detection& detection);
};

} // namespace visionlab

#endif // DETECTIONRENDERER_H
