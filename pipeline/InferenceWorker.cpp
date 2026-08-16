#include "InferenceWorker.h"

#include <chrono>
#include <iostream>

#include <opencv2/imgproc.hpp>

namespace visionlab {

InferenceWorker::InferenceWorker(BoundedQueue<FramePacket>& in,
                                 LatestResult<PresentedFrame>& out,
                                 DetectorProvider detector,
                                 DetectionRenderer renderer)
    : m_in(in)
    , m_out(out)
    , m_detector(std::move(detector))
    , m_renderer(std::move(renderer))
{
}

void InferenceWorker::run(std::stop_token stop)
{
    while (!stop.stop_requested())
    {
        FramePacket packet;
        if (!m_in.pop(packet))
            break;

        PresentedFrame presented;
        presented.frameId = packet.frameId;
        presented.captureTimestamp = packet.captureTimestamp;
        presented.sourceId = packet.sourceId;

        const auto detectBegin = std::chrono::steady_clock::now();
        try
        {
            IDetector* detector = m_detector ? m_detector() : nullptr;
            if (detector && detector->isReady())
                presented.detections = detector->detect(packet);
        }
        catch (const cv::Exception& e)
        {
            std::cerr << "[InferenceWorker] detect 异常: " << e.what() << '\n';
            presented.detections.clear();
        }
        const auto detectEnd = std::chrono::steady_clock::now();
        presented.inferenceLatencyMs =
            std::chrono::duration<double, std::milli>(detectEnd - detectBegin).count();

        // Tracking / Analytics 预留点：此处已有 packet + detections，尚未绘制。
        try
        {
            if (!packet.image.empty())
            {
                cv::Mat annotated = packet.image.clone();
                m_renderer.render(annotated, presented.detections);
                cv::Mat rgb;
                cv::cvtColor(annotated, rgb, cv::COLOR_BGR2RGB);
                if (!rgb.isContinuous())
                    rgb = rgb.clone();
                presented.rgb = std::move(rgb);
            }
        }
        catch (const cv::Exception& e)
        {
            std::cerr << "[InferenceWorker] 呈现异常: " << e.what() << '\n';
        }

        m_out.publish(std::move(presented));
    }
}

} // namespace visionlab
