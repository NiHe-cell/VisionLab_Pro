#ifndef FAKETRACKER_H
#define FAKETRACKER_H

#include <cstdint>
#include <string>
#include <vector>

#include "tracking/ITracker.h"

// 仅测试：按当前帧 Detection 下标分配 Confirmed id，不跨帧保持身份。
class FakeTracker : public visionlab::ITracker
{
public:
    std::string name() const override { return "fake"; }

    std::vector<visionlab::Track> update(
        const std::vector<visionlab::Detection>& detections,
        const visionlab::TrackUpdateContext&) override
    {
        if (detections.empty())
        {
            m_stats.activeTracks = 0;
            return {};
        }

        std::vector<visionlab::Track> tracks;
        tracks.reserve(detections.size());
        for (std::size_t i = 0; i < detections.size(); ++i)
        {
            visionlab::Track track;
            track.trackId = static_cast<std::uint64_t>(i + 1);
            track.classId = detections[i].classId;
            track.label = detections[i].label;
            track.confidence = detections[i].confidence;
            track.box = detections[i].box;
            track.state = visionlab::TrackState::Confirmed;
            tracks.push_back(track);
        }
        m_stats.createdTracks += static_cast<std::uint64_t>(tracks.size());
        m_stats.activeTracks = tracks.size();
        return tracks;
    }

    void reset() override { m_stats = {}; }

    visionlab::TrackerStats stats() const override { return m_stats; }

    visionlab::TrackerConfig config() const override { return {}; }

private:
    visionlab::TrackerStats m_stats;
};

#endif // FAKETRACKER_H
