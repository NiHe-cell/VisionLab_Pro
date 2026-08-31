#ifndef VISIONEVENT_H
#define VISIONEVENT_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

#include <opencv2/core.hpp>

namespace visionlab {

// 规则产生的事件类型。V1 四条规则各对应一项。
enum class EventType : std::uint8_t
{
    RoiIntrusion,
    LineCrossing,
    Loitering,
    Counting,
};

// 有向越线：AB 左侧（叉积 > 0）为 +，右侧为 −。
// Forward：+ → −，文案 "A→B"；Reverse：− → +，文案 "B→A"。
enum class CrossingDirection : std::uint8_t
{
    None,
    Forward,
    Reverse,
};

// 一次规则求值的帧上下文。规则不接收像素。
struct RuleContext
{
    std::int64_t frameId = 0;
    std::chrono::steady_clock::time_point timestamp{};
    std::string sourceId;
};

// 智能分析事件。纯领域类型：不依赖 Qt，不含像素缓冲。
// snapshotRef 留给 Phase 8；V1 保持空字符串。
struct VisionEvent
{
    std::uint64_t eventId = 0;
    EventType type = EventType::RoiIntrusion;
    std::string ruleId;
    std::string sourceId;
    std::uint64_t trackId = 0;
    int classId = -1;
    std::string label;
    float confidence = 0.0F;
    cv::Rect box;
    std::int64_t frameId = 0;
    std::chrono::steady_clock::time_point timestamp{};
    std::string message;
    std::string snapshotRef;
    CrossingDirection direction = CrossingDirection::None;
    std::uint64_t countIn = 0;
    std::uint64_t countOut = 0;
    std::size_t occupancy = 0;
};

struct RuleEngineStats
{
    std::size_t ruleCount = 0;
    std::size_t enabledRules = 0;
    std::uint64_t eventsEmitted = 0;
    double lastEvaluateLatencyMs = 0.0;
};

} // namespace visionlab

#endif // VISIONEVENT_H
