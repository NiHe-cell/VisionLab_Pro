#ifndef INFERENCEWORKER_H
#define INFERENCEWORKER_H

#include <functional>
#include <stop_token>

#include "core/BoundedQueue.h"
#include "core/FramePacket.h"
#include "core/LatestResult.h"
#include "core/PresentedFrame.h"
#include "detectors/IDetector.h"
#include "rendering/DetectionRenderer.h"

namespace visionlab {

// 推理工作线程体：从有界队列取帧、调用 IDetector、同线程绘制并发布 PresentedFrame。
// 不拥有 jthread。检测器由获取器注入，便于编排层按 DetectionMode 切换。
// Tracking 的预留插入点在 detect 之后、render 之前（本阶段不增加线程）。
class InferenceWorker
{
public:
    using DetectorProvider = std::function<IDetector*()>;

    InferenceWorker(BoundedQueue<FramePacket>& in,
                    LatestResult<PresentedFrame>& out,
                    DetectorProvider detector,
                    DetectionRenderer renderer = {});

    void run(std::stop_token stop);

private:
    BoundedQueue<FramePacket>& m_in;
    LatestResult<PresentedFrame>& m_out;
    DetectorProvider m_detector;
    DetectionRenderer m_renderer;
};

} // namespace visionlab

#endif // INFERENCEWORKER_H
