#ifndef IOUMATCHING_H
#define IOUMATCHING_H

#include <vector>

#include <opencv2/core.hpp>

namespace visionlab {

struct Association
{
    int detectionIndex = -1;
    int trackIndex = -1;
    float iou = 0.0F;
};

// 并集为 0 或任一方为空时返回 0。
float intersectionOverUnion(const cv::Rect& a, const cv::Rect& b);

// 只在同类且 IoU>=threshold 时匹配。greedy：反复取剩余对中 IoU 最大者。
// 每个 detection / track 至多匹配一次。
std::vector<Association> greedyIouAssociate(
    const std::vector<cv::Rect>& detectionBoxes,
    const std::vector<int>& detectionClassIds,
    const std::vector<cv::Rect>& predictedTrackBoxes,
    const std::vector<int>& trackClassIds,
    float iouThreshold);

} // namespace visionlab

#endif // IOUMATCHING_H
