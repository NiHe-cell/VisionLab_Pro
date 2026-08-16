#ifndef THROWINGDETECTOR_H
#define THROWINGDETECTOR_H

#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "detectors/IDetector.h"

// detect 抛出 cv::Exception，用于验证推理线程捕获后进程不 abort。
class ThrowingDetector : public visionlab::IDetector
{
public:
    std::string name() const override { return "throwing"; }
    visionlab::DetectionMode mode() const override
    {
        return visionlab::DetectionMode::Face;
    }
    bool isReady() const override { return true; }

    std::vector<visionlab::Detection> detect(const visionlab::FramePacket&) override
    {
        throw cv::Exception(cv::Error::StsError,
                            "forced detector failure",
                            "ThrowingDetector::detect",
                            __FILE__,
                            __LINE__);
    }
};

#endif // THROWINGDETECTOR_H
