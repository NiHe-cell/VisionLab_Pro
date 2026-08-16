#include "CaptureWorker.h"

#include <chrono>
#include <thread>

namespace visionlab {

CaptureWorker::CaptureWorker(IVideoSource& source,
                             BoundedQueue<FramePacket>& out,
                             StatsProbe& stats)
    : m_source(source)
    , m_out(out)
    , m_stats(stats)
{
}

void CaptureWorker::run(std::stop_token stop)
{
    std::int64_t frameId = 0;

    while (!stop.stop_requested() && !m_out.closed())
    {
        cv::Mat image;
        if (!m_source.read(image))
        {
            if (stop.stop_requested() || m_out.closed())
                break;
            // 摄像头偶发空帧或 Fake 流结束：短暂退避，避免空转打满 CPU。
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        FramePacket packet;
        packet.frameId = ++frameId;
        packet.captureTimestamp = std::chrono::steady_clock::now();
        packet.sourceId = m_source.sourceId();
        packet.image = std::move(image);

        const std::size_t droppedBefore = m_out.droppedCount();
        if (!m_out.push(std::move(packet)))
            break;

        m_stats.onCaptured();
        const std::size_t droppedNow = m_out.droppedCount();
        if (droppedNow > droppedBefore)
            m_stats.onDropped(droppedNow - droppedBefore);
        m_stats.setQueueDepth(m_out.size());
    }
}

} // namespace visionlab
