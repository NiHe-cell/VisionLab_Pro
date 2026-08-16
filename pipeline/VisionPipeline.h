#ifndef VISIONPIPELINE_H
#define VISIONPIPELINE_H

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include "core/BoundedQueue.h"
#include "core/FramePacket.h"
#include "core/LatestResult.h"
#include "core/PipelineStats.h"
#include "core/PresentedFrame.h"
#include "core/VisionTypes.h"
#include "detectors/IDetector.h"
#include "pipeline/CaptureWorker.h"
#include "pipeline/InferenceWorker.h"
#include "pipeline/StatsProbe.h"
#include "video/IVideoSource.h"

namespace visionlab {

// 纯 C++ 管线编排器：拥有视频源、检测器、有界队列和两条 jthread。
// 非 QObject，不依赖 Qt。检测器由调用方注入，不包含 DetectorFactory。
// Tracking 预留点在 InferenceWorker 的 detect 与 render 之间。
class VisionPipeline
{
public:
    static constexpr std::size_t kDefaultQueueCapacity = 2;

    VisionPipeline(std::unique_ptr<IVideoSource> source,
                   std::map<DetectionMode, std::unique_ptr<IDetector>> detectors,
                   std::size_t queueCapacity = kDefaultQueueCapacity);

    VisionPipeline(const VisionPipeline&) = delete;
    VisionPipeline& operator=(const VisionPipeline&) = delete;

    ~VisionPipeline();

    bool start();
    void stop();
    bool isRunning() const;

    void setMode(DetectionMode mode);
    DetectionMode mode() const;

    std::optional<PresentedFrame> latest() const;
    PipelineStats stats() const;

private:
    IDetector* currentDetector();
    void joinWorkers();

    std::unique_ptr<IVideoSource> m_source;
    std::map<DetectionMode, std::unique_ptr<IDetector>> m_detectors;
    const std::size_t m_queueCapacity;

    LatestResult<PresentedFrame> m_latest;
    StatsProbe m_stats;
    std::unique_ptr<BoundedQueue<FramePacket>> m_queue;
    std::unique_ptr<CaptureWorker> m_captureWorker;
    std::unique_ptr<InferenceWorker> m_inferenceWorker;
    std::jthread m_captureThread;
    std::jthread m_inferenceThread;

    std::atomic<DetectionMode> m_mode{DetectionMode::Face};
    std::atomic<bool> m_running{false};
    std::mutex m_lifecycle;
};

} // namespace visionlab

#endif // VISIONPIPELINE_H
