#ifndef TRACKRENDERER_H
#define TRACKRENDERER_H

#include <vector>

#include <opencv2/core.hpp>

#include "core/Track.h"

namespace visionlab {

// 把结构化 Track 绘制到帧上：包围框、`label #id`、可选质心折线。
//
// 所有权约定与 DetectionRenderer 相同：frame 必须由调用方拥有且可写。
class TrackRenderer
{
public:
    void render(cv::Mat& frame, const std::vector<Track>& tracks) const;

private:
    static cv::Scalar colorFor(const Track& track);
};

} // namespace visionlab

#endif // TRACKRENDERER_H
