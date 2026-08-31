#ifndef STATSPROBE_H
#define STATSPROBE_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include "core/LatencyWindow.h"
#include "core/PipelineStats.h"
#include "core/Track.h"
#include "core/VisionEvent.h"

namespace visionlab {

// 工作线程写入、任意线程 snapshot 取值拷贝的管线指标探针。
//
// Capture 线程调用 onCaptured / onDropped / setQueueDepth；
// Inference 线程调用 onInferred(推理耗时, E2E)、onTracked(跟踪计数, 跟踪耗时)
// 与 onRuled(规则计数, 规则耗时)。
// 呈现与推理同线程，故 renderFps 与 inferenceFps 相同。不向 QML 发信号。
class StatsProbe
{
public:
    void onCaptured();
    void onDropped(std::uint64_t count = 1);
    void onInferred(double inferenceLatencyMs, double endToEndLatencyMs);
    void onTracked(const TrackerStats& stats, double trackingLatencyMs);
    void onRuled(const RuleEngineStats& stats, double ruleLatencyMs);
    void setQueueDepth(std::size_t depth);
    void reset();
    PipelineStats snapshot() const;

private:
    mutable std::mutex m_mutex;
    PipelineStats m_totals;
    LatencyWindow m_latencies;
    LatencyWindow m_trackingLatencies;
    LatencyWindow m_ruleLatencies;
    std::chrono::steady_clock::time_point m_origin{std::chrono::steady_clock::now()};
};

} // namespace visionlab

#endif // STATSPROBE_H
