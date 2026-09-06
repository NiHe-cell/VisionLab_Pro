#ifndef VISIONPIPELINE_H
#define VISIONPIPELINE_H

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "core/BoundedQueue.h"
#include "core/FramePacket.h"
#include "core/LatestResult.h"
#include "core/PipelineStats.h"
#include "core/PresentedFrame.h"
#include "core/VisionTypes.h"
#include "detectors/IDetector.h"
#include "pipeline/CaptureWorker.h"
#include "pipeline/EventLog.h"
#include "pipeline/InferenceWorker.h"
#include "pipeline/StatsProbe.h"
#include "analytics/RuleEngine.h"
#include "tracking/ITracker.h"
#include "video/IVideoSource.h"

namespace visionlab {

// 纯 C++ 管线编排器：拥有视频源、检测器、可选跟踪器、可选规则引擎、有界队列和两条 jthread。
// 非 QObject，不依赖 Qt。检测器由调用方注入。跟踪与规则在推理线程、detect 与 render 之间。
class VisionPipeline
{
public:
    static constexpr std::size_t kDefaultQueueCapacity = 2;

    VisionPipeline(std::unique_ptr<IVideoSource> source,
                   std::map<DetectionMode, std::unique_ptr<IDetector>> detectors,
                   std::size_t queueCapacity = kDefaultQueueCapacity,
                   std::unique_ptr<ITracker> tracker = {},
                   std::unique_ptr<RuleEngine> rules = {});

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
    std::vector<VisionEvent> recentEvents() const;
    RuleEngine* ruleEngine();

    // 仅在 !isRunning() 时有效。取出视频源供宿主按新会话设置重建管线。
    std::unique_ptr<IVideoSource> releaseSource();

    // 在推理线程、publish 之后调用。回调不得做 GUI 工作；编排层应 QueuedConnection 切回 GUI。
    void setPresentedCallback(std::function<void()> callback);

private:
    IDetector* currentDetector();
    void joinWorkers();

    std::unique_ptr<IVideoSource> m_source;
    std::map<DetectionMode, std::unique_ptr<IDetector>> m_detectors;
    std::unique_ptr<ITracker> m_tracker;
    std::unique_ptr<RuleEngine> m_rules;
    EventLog m_eventLog;
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
    std::function<void()> m_onPresented;
};

} // namespace visionlab

#endif // VISIONPIPELINE_H
