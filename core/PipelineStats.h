#ifndef PIPELINESTATS_H
#define PIPELINESTATS_H

#include <cstddef>
#include <cstdint>

namespace visionlab {

// 管线计数与耗时指标的聚合。
//
// 由 StatsProbe 在工作线程写入、经 snapshot() 值拷贝交给 UI 抽样。
// 不要每帧向 QML 发信号；编排层用定时器取快照。
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
