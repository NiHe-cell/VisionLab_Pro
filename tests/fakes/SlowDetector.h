#ifndef SLOWDETECTOR_H
#define SLOWDETECTOR_H

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "detectors/IDetector.h"

// detect 内睡眠，用于验证推理慢于入队时仍可 stop / close。
class SlowDetector : public visionlab::IDetector
{
public:
    explicit SlowDetector(std::chrono::milliseconds delay)
        : m_delay(delay)
    {
    }

    std::string name() const override { return "slow"; }
    visionlab::DetectionMode mode() const override
    {
        return visionlab::DetectionMode::Object;
    }
    bool isReady() const override { return true; }

    std::vector<visionlab::Detection> detect(const visionlab::FramePacket&) override
    {
        std::this_thread::sleep_for(m_delay);
        m_callCount.fetch_add(1, std::memory_order_relaxed);
        return {};
    }

    int callCount() const { return m_callCount.load(std::memory_order_relaxed); }

private:
    std::chrono::milliseconds m_delay;
    std::atomic<int> m_callCount{0};
};

#endif // SLOWDETECTOR_H
