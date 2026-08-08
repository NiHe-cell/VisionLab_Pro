#ifndef PIPELINESTATS_H
#define PIPELINESTATS_H

#include <cstddef>
#include <cstdint>

namespace visionlab {

// 管线计数与耗时指标的聚合。
//
// Phase 1 只定义数据形状；真正写入这些字段的采集/推理工作线程将在
// Phase 2 引入。由于字段将由工作线程写入、UI 线程采样，届时写入方
// 必须使用原子或快照交接（具体方案在 Phase 2 决定）。
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
