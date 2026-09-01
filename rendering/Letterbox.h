#ifndef LETTERBOX_H
#define LETTERBOX_H

#include <opencv2/core.hpp>

namespace visionlab {

// PreserveAspectFit 几何：帧在 item 内均匀缩放并居中，不足处为黑边。
// 无 Qt。坐标为 float，调用方画多边形时再 round。
struct Letterbox
{
    float offsetX = 0.F;
    float offsetY = 0.F;
    float contentW = 0.F;
    float contentH = 0.F;
    float scale = 1.F;
};

Letterbox computeLetterbox(float itemW, float itemH, int frameW, int frameH);

bool itemToFrame(const Letterbox& box, float itemX, float itemY,
                 int frameW, int frameH, cv::Point2f& out);

bool frameToItem(const Letterbox& box, float frameX, float frameY,
                 cv::Point2f& out);

} // namespace visionlab

#endif // LETTERBOX_H
