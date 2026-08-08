#ifndef PIPELINESTATS_H
#define PIPELINESTATS_H

#include <cstddef>
#include <cstdint>

namespace visionlab {

// Aggregated pipeline counters and timing values.
//
// Phase 1 defines the data shape only; the capture/inference workers that
// populate these fields arrive with the Phase 2 pipeline. Values will be
// written by worker threads and sampled by the UI, so population code must
// use atomics or a snapshot hand-off (decided in Phase 2).
struct PipelineStats
{
    std::uint64_t capturedFrames = 0;
    std::uint64_t processedFrames = 0;
    std::uint64_t droppedFrames = 0;

    std::size_t captureQueueDepth = 0;

    double captureFps = 0.0;
    double inferenceFps = 0.0;
    double renderFps = 0.0;

    double avgInferenceLatencyMs = 0.0;
    double p50InferenceLatencyMs = 0.0;
    double p95InferenceLatencyMs = 0.0;
    double endToEndLatencyMs = 0.0;
};

} // namespace visionlab

#endif // PIPELINESTATS_H
