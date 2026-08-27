#ifndef INFERENCEWORKER_H
#define INFERENCEWORKER_H

#include <functional>
#include <optional>
#include <stop_token>

#include "core/BoundedQueue.h"
#include "core/FramePacket.h"
#include "core/LatestResult.h"
#include "core/PresentedFrame.h"
#include "core/VisionTypes.h"
#include "detectors/IDetector.h"
#include "pipeline/StatsProbe.h"
#include "rendering/DetectionRenderer.h"
#include "tracking/ITracker.h"

namespace visionlab {

// 推理工作线程体：从有界队列取帧、调用 IDetector、可选 ITracker，同线程绘制并发布。
// 不拥有 jthread。检测器与模式由获取器注入。跟踪在 detect 之后、render 之前，不另开线程。
class InferenceWorker
{
public:
    using DetectorProvider = std::function<IDetector*()>;
    using ModeProvider = std::function<DetectionMode()>;

    InferenceWorker(BoundedQueue<FramePacket>& in,
                    LatestResult<PresentedFrame>& out,
                    DetectorProvider detector,
                    StatsProbe& stats,
                    std::function<void()> onPresented = {},
                    DetectionRenderer renderer = {},
                    ITracker* tracker = nullptr,
                    ModeProvider mode = {});

    void run(std::stop_token stop);

private:
    BoundedQueue<FramePacket>& m_in;
    LatestResult<PresentedFrame>& m_out;
    DetectorProvider m_detector;
    StatsProbe& m_stats;
    std::function<void()> m_onPresented;
    DetectionRenderer m_renderer;
    ITracker* m_tracker;
    ModeProvider m_mode;
    std::optional<DetectionMode> m_lastMode;
};

} // namespace visionlab

#endif // INFERENCEWORKER_H
