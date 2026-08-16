#ifndef CAPTUREWORKER_H
#define CAPTUREWORKER_H

#include <stop_token>

#include "core/BoundedQueue.h"
#include "core/FramePacket.h"
#include "pipeline/StatsProbe.h"
#include "video/IVideoSource.h"

namespace visionlab {

// 采集工作线程体：从 IVideoSource 读帧、打帧号/时间戳，按 DropOldest 推入有界队列。
// 不拥有 std::jthread；由编排层持有线程并传入 stop_token。
// 不调用 open()：源由编排层在 start 时打开。
class CaptureWorker
{
public:
    CaptureWorker(IVideoSource& source,
                  BoundedQueue<FramePacket>& out,
                  StatsProbe& stats);

    void run(std::stop_token stop);

private:
    IVideoSource& m_source;
    BoundedQueue<FramePacket>& m_out;
    StatsProbe& m_stats;
};

} // namespace visionlab

#endif // CAPTUREWORKER_H
