#ifndef MOTIONDETECTOR_H
#define MOTIONDETECTOR_H

#include <string>
#include <vector>

#include <opencv2/video/background_segm.hpp>

#include "IDetector.h"

namespace visionlab {

// 基于 MOG2 背景减除的运动检测器。
//
// Detection 泛化约定：运动区域不是分类目标，
//   classId    = kMotionClassId（保留常量，不与真实类别冲突）
//   label      = "In Motion"（沿用旧 UI 文本）
//   confidence = 1.0（运动检测无置信度语义）
//
// 背景模型是本实现的状态，随 detect 调用持续学习；
// 同一实例不承诺并发调用（由单管线线程驱动）。
class MotionDetector : public IDetector
{
public:
    MotionDetector();

    std::string name() const override { return "MOG2 Motion"; }
    DetectionMode mode() const override { return DetectionMode::Motion; }
    bool isReady() const override { return true; } // 无外部模型，始终就绪

    std::vector<Detection> detect(const FramePacket& frame) override;

private:
    cv::Ptr<cv::BackgroundSubtractor> m_bgSubtractor;
    cv::Mat m_fgMask;
    int m_minArea = 500;
};

} // namespace visionlab

#endif // MOTIONDETECTOR_H
