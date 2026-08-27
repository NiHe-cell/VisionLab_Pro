#ifndef TRACK_H
#define TRACK_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

#include <opencv2/core.hpp>

namespace visionlab {

// 单条轨迹的生命周期。Tentative 尚未确认，不对外返回；
// Confirmed 为稳定目标；Lost 仍存活但本帧未匹配（可带预测框）。
enum class TrackState : std::uint8_t
{
    Tentative,
    Confirmed,
    Lost,
};

// 轨迹历史中的一个质心样本。容量由 TrackerConfig::maxTrajectoryPoints 限制。
struct TrackPoint
{
    std::int64_t frameId = 0;
    std::chrono::steady_clock::time_point timestamp{};
    cv::Point2f centroid{};
    cv::Rect box;
};

// 持久跟踪目标。坐标为原始帧像素。纯领域类型：不依赖 Qt，不含像素缓冲。
struct Track
{
    std::uint64_t trackId = 0;
    int classId = -1;
    std::string label;
    float confidence = 0.0F;
    cv::Rect box;
    TrackState state = TrackState::Tentative;
    std::chrono::steady_clock::time_point firstSeen{};
    std::chrono::steady_clock::time_point lastSeen{};
    int age = 0;
    int hits = 0;
    int timeSinceUpdate = 0;
    std::deque<TrackPoint> trajectory;
};

struct TrackerConfig
{
    float highThresh = 0.6F;
    float lowThresh = 0.1F;
    float matchIou = 0.5F;
    int maxLostFrames = 30;
    int minHits = 3;
    std::size_t maxTrajectoryPoints = 30;
};

struct TrackerStats
{
    std::size_t activeTracks = 0;
    std::uint64_t createdTracks = 0;
    std::uint64_t lostTracks = 0;
    std::uint64_t removedTracks = 0;
    double lastUpdateLatencyMs = 0.0;
};

// 一次 update 的帧上下文。跟踪器不接收像素。
struct TrackUpdateContext
{
    std::int64_t frameId = 0;
    std::chrono::steady_clock::time_point timestamp{};
};

} // namespace visionlab

#endif // TRACK_H
