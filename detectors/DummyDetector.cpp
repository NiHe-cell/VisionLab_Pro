#include "DummyDetector.h"

namespace visionlab {

std::vector<Detection> DummyDetector::detect(const FramePacket& frame)
{
    if (frame.image.empty())
        return {};

    Detection detection;
    detection.classId = 0;
    detection.label = "dummy";
    detection.confidence = 1.0F;
    detection.box = cv::Rect(10, 10, 20, 20);
    return {detection};
}

} // namespace visionlab
