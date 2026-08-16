#ifndef FAKEDETECTOR_H
#define FAKEDETECTOR_H

#include <atomic>
#include <string>
#include <utility>
#include <vector>

#include "detectors/IDetector.h"

// 确定性假检测器：非空且就绪时返回固定的一条 Detection。
class FakeDetector : public visionlab::IDetector
{
public:
    explicit FakeDetector(std::string label = "fake-object")
        : m_label(std::move(label))
    {
    }

    std::string name() const override { return "fake"; }
    visionlab::DetectionMode mode() const override
    {
        return visionlab::DetectionMode::Object;
    }
    bool isReady() const override { return m_ready; }

    std::vector<visionlab::Detection> detect(const visionlab::FramePacket& frame) override
    {
        m_callCount.fetch_add(1, std::memory_order_relaxed);
        if (!m_ready || frame.image.empty())
            return {};

        visionlab::Detection d;
        d.classId = 1;
        d.label = m_label;
        d.confidence = 0.9F;
        d.box = cv::Rect(1, 2, 3, 4);
        return {d};
    }

    void setReady(bool ready) { m_ready = ready; }
    int callCount() const { return m_callCount.load(std::memory_order_relaxed); }

private:
    std::string m_label;
    bool m_ready = true;
    std::atomic<int> m_callCount{0};
};

#endif // FAKEDETECTOR_H
