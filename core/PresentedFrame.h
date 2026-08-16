#ifndef PRESENTEDFRAME_H
#define PRESENTEDFRAME_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "core/Detection.h"

namespace visionlab {

// 推理线程产出、交给 UI 的一帧结果。
//
// rgb 为独立连续的 RGB888 缓冲（已从 BGR 转换），可供 QImage 深拷贝。
// detections 保留结构化结果，供后续在 detect 与 render 之间插入 Tracking。
struct PresentedFrame
{
    std::int64_t frameId = 0;
    std::chrono::steady_clock::time_point captureTimestamp{};
    std::string sourceId;
    cv::Mat rgb;
    std::vector<Detection> detections;
    double inferenceLatencyMs = 0.0;
};

} // namespace visionlab

#endif // PRESENTEDFRAME_H
