#include "IouMatching.h"

#include <cstddef>
#include <vector>

namespace visionlab {

float intersectionOverUnion(const cv::Rect& a, const cv::Rect& b)
{
    if (a.empty() || b.empty())
        return 0.0F;

    const cv::Rect overlap = a & b;
    const int inter = overlap.area();
    if (inter <= 0)
        return 0.0F;

    const int uni = a.area() + b.area() - inter;
    if (uni <= 0)
        return 0.0F;
    return static_cast<float>(inter) / static_cast<float>(uni);
}

std::vector<Association> greedyIouAssociate(
    const std::vector<cv::Rect>& detectionBoxes,
    const std::vector<int>& detectionClassIds,
    const std::vector<cv::Rect>& predictedTrackBoxes,
    const std::vector<int>& trackClassIds,
    float iouThreshold)
{
    const std::size_t detCount = detectionBoxes.size();
    const std::size_t trackCount = predictedTrackBoxes.size();
    if (detCount != detectionClassIds.size() || trackCount != trackClassIds.size())
        return {};
    if (detCount == 0 || trackCount == 0)
        return {};

    std::vector<char> detUsed(detCount, 0);
    std::vector<char> trackUsed(trackCount, 0);
    std::vector<Association> matches;

    while (true)
    {
        Association best;
        for (std::size_t d = 0; d < detCount; ++d)
        {
            if (detUsed[d])
                continue;
            for (std::size_t t = 0; t < trackCount; ++t)
            {
                if (trackUsed[t] || detectionClassIds[d] != trackClassIds[t])
                    continue;
                const float iou = intersectionOverUnion(detectionBoxes[d], predictedTrackBoxes[t]);
                if (iou < iouThreshold || iou <= best.iou)
                    continue;
                best.detectionIndex = static_cast<int>(d);
                best.trackIndex = static_cast<int>(t);
                best.iou = iou;
            }
        }
        if (best.detectionIndex < 0)
            break;
        detUsed[static_cast<std::size_t>(best.detectionIndex)] = 1;
        trackUsed[static_cast<std::size_t>(best.trackIndex)] = 1;
        matches.push_back(best);
    }
    return matches;
}

} // namespace visionlab
