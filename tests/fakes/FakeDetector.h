#ifndef FAKEDETECTOR_H
#define FAKEDETECTOR_H

#include <string>
#include <vector>

#include "detectors/IDetector.h"

// 确定性假检测器：非空且就绪时返回固定的一条 Detection。
class FakeDetector : public visionlab::IDetector
{
public:
    std::string name() const override { return "fake"; }
    visionlab::DetectionMode mode() const override
    {
        return visionlab::DetectionMode::Object;
    }
    bool isReady() const override { return m_ready; }

    std::vector<visionlab::Detection> detect(const visionlab::FramePacket& frame) override
    {
        ++m_callCount;
        if (!m_ready || frame.image.empty())
            return {};

        visionlab::Detection d;
        d.classId = 1;
        d.label = "fake-object";
        d.confidence = 0.9F;
        d.box = cv::Rect(1, 2, 3, 4);
        return {d};
    }

    void setReady(bool ready) { m_ready = ready; }
    int callCount() const { return m_callCount; }

private:
    bool m_ready = true;
    int m_callCount = 0;
};

#endif // FAKEDETECTOR_H
