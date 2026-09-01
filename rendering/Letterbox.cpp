#include "rendering/Letterbox.h"

#include <algorithm>

namespace visionlab {

Letterbox computeLetterbox(float itemW, float itemH, int frameW, int frameH)
{
    if (itemW <= 0.F || itemH <= 0.F || frameW <= 0 || frameH <= 0)
        return Letterbox{0.F, 0.F, 0.F, 0.F, 0.F};

    Letterbox box;
    box.scale = std::min(itemW / static_cast<float>(frameW),
                         itemH / static_cast<float>(frameH));
    box.contentW = static_cast<float>(frameW) * box.scale;
    box.contentH = static_cast<float>(frameH) * box.scale;
    box.offsetX = (itemW - box.contentW) * 0.5F;
    box.offsetY = (itemH - box.contentH) * 0.5F;
    return box;
}

bool itemToFrame(const Letterbox& box, float itemX, float itemY,
                 int frameW, int frameH, cv::Point2f& out)
{
    if (box.scale <= 0.F || frameW <= 0 || frameH <= 0)
        return false;
    if (itemX < box.offsetX || itemY < box.offsetY)
        return false;
    if (itemX > box.offsetX + box.contentW || itemY > box.offsetY + box.contentH)
        return false;

    out.x = (itemX - box.offsetX) / box.scale;
    out.y = (itemY - box.offsetY) / box.scale;
    return true;
}

bool frameToItem(const Letterbox& box, float frameX, float frameY,
                 cv::Point2f& out)
{
    if (box.scale == 0.F)
        return false;
    out.x = box.offsetX + frameX * box.scale;
    out.y = box.offsetY + frameY * box.scale;
    return true;
}

} // namespace visionlab
