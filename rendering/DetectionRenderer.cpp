#include "DetectionRenderer.h"

#include <algorithm>

#include <opencv2/imgproc.hpp>

#include "core/VisionTypes.h"

namespace visionlab {

namespace {

// 样式沿用改造前的视觉表现：普通检测绿色、运动区域黄色（BGR）。
constexpr int kBoxThickness = 2;
constexpr double kFontScale = 0.6;
constexpr int kTextThickness = 2;
constexpr int kLabelOffset = 5;

const cv::Scalar kObjectColor{0, 255, 0};
const cv::Scalar kMotionColor{0, 255, 255};

} // namespace

cv::Scalar DetectionRenderer::colorFor(const Detection& detection)
{
    return detection.classId == kMotionClassId ? kMotionColor : kObjectColor;
}

void DetectionRenderer::render(cv::Mat& frame, const std::vector<Detection>& detections) const
{
    if (frame.empty())
        return;

    for (const Detection& detection : detections)
    {
        const cv::Scalar color = colorFor(detection);

        cv::rectangle(frame, detection.box, color, kBoxThickness);

        // 标签放在框上方；框贴近图像顶部时下移到框内，避免画出负坐标。
        const int labelY = detection.box.y - kLabelOffset >= kLabelOffset
                               ? detection.box.y - kLabelOffset
                               : detection.box.y + kLabelOffset + 10;
        const cv::Point origin{std::max(detection.box.x, 0), labelY};
        cv::putText(frame, detection.label, origin, cv::FONT_HERSHEY_SIMPLEX,
                    kFontScale, color, kTextThickness);
    }
}

} // namespace visionlab
