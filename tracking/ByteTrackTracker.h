#ifndef BYTETRACKTRACKER_H
#define BYTETRACKTRACKER_H

#include <chrono>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "core/Track.h"
#include "tracking/ITracker.h"
#include "tracking/KalmanBoxFilter.h"

namespace visionlab {

class ByteTrackTracker final : public ITracker
{
public:
    explicit ByteTrackTracker(TrackerConfig config = {});

    std::string name() const override;
    std::vector<Track> update(
        const std::vector<Detection>& detections,
        const TrackUpdateContext& context) override;
    void reset() override;
    TrackerStats stats() const override;
    TrackerConfig config() const override;

private:
    struct InternalTrack
    {
        std::uint64_t id = 0;
        KalmanBoxFilter kalman;
        int classId = -1;
        std::string label;
        float confidence = 0.0F;
        TrackState state = TrackState::Tentative;
        std::chrono::steady_clock::time_point firstSeen{};
        std::chrono::steady_clock::time_point lastSeen{};
        int age = 1;
        int hits = 1;
        int timeSinceUpdate = 0;
        std::deque<TrackPoint> trajectory;

        explicit InternalTrack(const cv::Rect& box)
            : kalman(box)
        {
        }
    };

    Track toPublic(const InternalTrack& track) const;
    void appendTrajectory(InternalTrack& track, const TrackUpdateContext& context);
    void applyDetection(InternalTrack& track, const Detection& detection,
                        const TrackUpdateContext& context);

    TrackerConfig m_config;
    TrackerStats m_stats;
    std::vector<InternalTrack> m_tracks;
    std::uint64_t m_nextId = 1;
    std::optional<std::chrono::steady_clock::time_point> m_lastTimestamp;
};

} // namespace visionlab

#endif // BYTETRACKTRACKER_H
