#include "MotionDetector.h"

#include <opencv2/imgproc.hpp>

#include "core/VisionTypes.h"

namespace visionlab {

MotionDetector::MotionDetector()
    : m_bgSubtractor(cv::createBackgroundSubtractorMOG2(500, 16, true))
{
}

std::vector<Detection> MotionDetector::detect(const FramePacket& frame)
{
    if (frame.image.empty())
        return {};

    // 背景减除：背景→黑，运动区域→白。
    m_bgSubtractor->apply(frame.image, m_fgMask);

    // 去除阴影（MOG2 将阴影标为 127）并降噪。
    cv::threshold(m_fgMask, m_fgMask, 200, 255, cv::THRESH_BINARY);
    cv::erode(m_fgMask, m_fgMask, cv::Mat(), cv::Point(-1, -1), 1);
    cv::dilate(m_fgMask, m_fgMask, cv::Mat(), cv::Point(-1, -1), 2);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(m_fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    const cv::Rect frameRect(0, 0, frame.image.cols, frame.image.rows);

    std::vector<Detection> detections;
    for (const auto& contour : contours)
    {
        if (cv::contourArea(contour) < m_minArea)
            continue;

        const cv::Rect box = cv::boundingRect(contour) & frameRect;
        if (box.empty())
            continue;

        Detection detection;
        detection.classId = kMotionClassId;
        detection.label = "In Motion";
        detection.confidence = 1.0F;
        detection.box = box;
        detections.push_back(detection);
    }
    return detections;
}

} // namespace visionlab
