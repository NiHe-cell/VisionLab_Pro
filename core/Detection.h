#ifndef DETECTION_H
#define DETECTION_H

#include <string>

#include <opencv2/core.hpp>

namespace visionlab {

// A single structured detector output, in original frame pixel coordinates.
// Pure domain type: no Qt, no rendering concerns.
struct Detection
{
    // Index into the detector's class table; kMotionClassId for motion
    // regions, which carry no class semantics.
    int classId = -1;

    // Human-readable class label (e.g. "person", "face", "motion").
    std::string label;

    // Confidence in [0, 1] where the detector provides one.
    float confidence = 0.0F;

    // Bounding box in original frame pixel coordinates.
    cv::Rect box;
};

} // namespace visionlab

#endif // DETECTION_H
