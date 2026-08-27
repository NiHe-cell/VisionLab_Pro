#include "InferenceWorker.h"

#include <chrono>
#include <iostream>

#include <opencv2/imgproc.hpp>

namespace visionlab {

InferenceWorker::InferenceWorker(BoundedQueue<FramePacket>& in,
                                 LatestResult<PresentedFrame>& out,
                                 DetectorProvider detector,
                                 StatsProbe& stats,
                                 std::function<void()> onPresented,
                                 DetectionRenderer renderer,
                                 ITracker* tracker,
                                 ModeProvider mode)
    : m_in(in)
    , m_out(out)
    , m_detector(std::move(detector))
    , m_stats(stats)
    , m_onPresented(std::move(onPresented))
    , m_renderer(std::move(renderer))
    , m_tracker(tracker)
    , m_mode(std::move(mode))
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

        if (m_tracker)
        {
            if (m_mode)
            {
                const DetectionMode current = m_mode();
                if (m_lastMode.has_value() && *m_lastMode != current)
                    m_tracker->reset();
                m_lastMode = current;
            }
            try
            {
                TrackUpdateContext context;
                context.frameId = packet.frameId;
                context.timestamp = packet.captureTimestamp;
                presented.tracks = m_tracker->update(presented.detections, context);
            }
            catch (const cv::Exception& e)
            {
                std::cerr << "[InferenceWorker] track 异常: " << e.what() << '\n';
                presented.tracks.clear();
            }
        }

        try
        {
            if (!packet.image.empty())
            {
                cv::Mat annotated = packet.image.clone();
                if (!presented.tracks.empty())
                    m_trackRenderer.render(annotated, presented.tracks);
                else
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

        const double inferenceMs = presented.inferenceLatencyMs;
        const double endToEndMs = std::chrono::duration<double, std::milli>(
                                      std::chrono::steady_clock::now() - packet.captureTimestamp)
                                      .count();
        m_out.publish(std::move(presented));
        m_stats.onInferred(inferenceMs, endToEndMs);
        m_stats.setQueueDepth(m_in.size());
        if (m_onPresented)
            m_onPresented();
    }
}

} // namespace visionlab
