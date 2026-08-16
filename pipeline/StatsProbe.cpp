#include "StatsProbe.h"

#include <chrono>

namespace visionlab {

void StatsProbe::onCaptured()
{
    std::lock_guard lock(m_mutex);
    ++m_totals.capturedFrames;
}

void StatsProbe::onDropped(std::uint64_t count)
{
    std::lock_guard lock(m_mutex);
    m_totals.droppedFrames += count;
}

void StatsProbe::onInferred(double inferenceLatencyMs, double endToEndLatencyMs)
{
    std::lock_guard lock(m_mutex);
    ++m_totals.processedFrames;
    m_latencies.record(inferenceLatencyMs);
    m_totals.endToEndLatencyMs = endToEndLatencyMs;
}

void StatsProbe::setQueueDepth(std::size_t depth)
{
    std::lock_guard lock(m_mutex);
    m_totals.captureQueueDepth = depth;
}

void StatsProbe::reset()
{
    std::lock_guard lock(m_mutex);
    m_totals = PipelineStats{};
    m_latencies.reset();
    m_origin = std::chrono::steady_clock::now();
}

PipelineStats StatsProbe::snapshot() const
{
    std::lock_guard lock(m_mutex);
    PipelineStats stats = m_totals;
    stats.avgInferenceLatencyMs = m_latencies.mean();
    stats.p50InferenceLatencyMs = m_latencies.percentile(50.0);
    stats.p95InferenceLatencyMs = m_latencies.percentile(95.0);

    const double elapsed = std::chrono::duration<double>(
                               std::chrono::steady_clock::now() - m_origin)
                               .count();
    if (elapsed > 0.0)
    {
        stats.captureFps = static_cast<double>(m_totals.capturedFrames) / elapsed;
        stats.inferenceFps = static_cast<double>(m_totals.processedFrames) / elapsed;
        stats.renderFps = stats.inferenceFps;
    }
    return stats;
}

} // namespace visionlab
