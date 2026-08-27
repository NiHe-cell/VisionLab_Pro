#include "ByteTrackTracker.h"

#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

#include "tracking/IouMatching.h"

namespace visionlab {
namespace {

constexpr double kDefaultDt = 1.0 / 30.0;

double deltaSeconds(
    const std::optional<std::chrono::steady_clock::time_point>& previous,
    std::chrono::steady_clock::time_point current)
{
    if (!previous.has_value())
        return kDefaultDt;
    const double dt = std::chrono::duration<double>(current - *previous).count();
    return dt > 0.0 ? dt : kDefaultDt;
}

cv::Point2f centroidOf(const cv::Rect& box)
{
    return {
        static_cast<float>(box.x) + static_cast<float>(box.width) * 0.5F,
        static_cast<float>(box.y) + static_cast<float>(box.height) * 0.5F,
    };
}

} // namespace

ByteTrackTracker::ByteTrackTracker(TrackerConfig config)
    : m_config(config)
{
}

std::string ByteTrackTracker::name() const
{
    return "ByteTrack";
}

Track ByteTrackTracker::toPublic(const InternalTrack& track) const
{
    Track published;
    published.trackId = track.id;
    published.classId = track.classId;
    published.label = track.label;
    published.confidence = track.confidence;
    published.box = track.kalman.box();
    published.state = track.state;
    published.firstSeen = track.firstSeen;
    published.lastSeen = track.lastSeen;
    published.age = track.age;
    published.hits = track.hits;
    published.timeSinceUpdate = track.timeSinceUpdate;
    published.trajectory = track.trajectory;
    return published;
}

void ByteTrackTracker::appendTrajectory(InternalTrack& track, const TrackUpdateContext& context)
{
    TrackPoint point;
    point.frameId = context.frameId;
    point.timestamp = context.timestamp;
    point.box = track.kalman.box();
    point.centroid = centroidOf(point.box);
    track.trajectory.push_back(point);
    while (track.trajectory.size() > m_config.maxTrajectoryPoints)
        track.trajectory.pop_front();
}

void ByteTrackTracker::applyDetection(InternalTrack& track, const Detection& detection,
                                      const TrackUpdateContext& context)
{
    track.kalman.update(detection.box);
    track.label = detection.label;
    track.confidence = detection.confidence;
    track.lastSeen = context.timestamp;
    track.hits += 1;
    track.timeSinceUpdate = 0;
    if (track.hits >= m_config.minHits)
        track.state = TrackState::Confirmed;
}

std::vector<Track> ByteTrackTracker::update(const std::vector<Detection>& detections,
                                            const TrackUpdateContext& context)
{
    const auto started = std::chrono::steady_clock::now();
    const double dt = deltaSeconds(m_lastTimestamp, context.timestamp);
    m_lastTimestamp = context.timestamp;

    for (InternalTrack& track : m_tracks)
    {
        track.kalman.predict(dt);
        track.age += 1;
    }

    std::vector<int> highIndices;
    std::vector<int> lowIndices;
    highIndices.reserve(detections.size());
    lowIndices.reserve(detections.size());
    for (int i = 0; i < static_cast<int>(detections.size()); ++i)
    {
        const float confidence = detections[static_cast<std::size_t>(i)].confidence;
        if (confidence > m_config.highThresh)
            highIndices.push_back(i);
        else if (confidence > m_config.lowThresh)
            lowIndices.push_back(i);
    }

    std::vector<char> highUsed(highIndices.size(), 0);
    std::vector<char> trackUsed(m_tracks.size(), 0);

    const auto associate = [&](const std::vector<int>& detIndices, std::vector<char>& detUsed) {
        std::vector<int> unmatchedDetPos;
        std::vector<int> unmatchedTrackPos;
        std::vector<cv::Rect> unmatchedDetBoxes;
        std::vector<int> unmatchedDetClasses;
        std::vector<cv::Rect> unmatchedTrackBoxes;
        std::vector<int> unmatchedTrackClasses;
        for (std::size_t i = 0; i < detIndices.size(); ++i)
        {
            if (detUsed[i])
                continue;
            unmatchedDetPos.push_back(static_cast<int>(i));
            unmatchedDetBoxes.push_back(detections[static_cast<std::size_t>(detIndices[i])].box);
            unmatchedDetClasses.push_back(
                detections[static_cast<std::size_t>(detIndices[i])].classId);
        }
        for (std::size_t t = 0; t < m_tracks.size(); ++t)
        {
            if (trackUsed[t])
                continue;
            unmatchedTrackPos.push_back(static_cast<int>(t));
            unmatchedTrackBoxes.push_back(m_tracks[t].kalman.box());
            unmatchedTrackClasses.push_back(m_tracks[t].classId);
        }

        const auto matches = greedyIouAssociate(
            unmatchedDetBoxes, unmatchedDetClasses, unmatchedTrackBoxes, unmatchedTrackClasses,
            m_config.matchIou);
        for (const Association& match : matches)
        {
            const int detPos = unmatchedDetPos[static_cast<std::size_t>(match.detectionIndex)];
            const int trackPos = unmatchedTrackPos[static_cast<std::size_t>(match.trackIndex)];
            detUsed[static_cast<std::size_t>(detPos)] = 1;
            trackUsed[static_cast<std::size_t>(trackPos)] = 1;
            applyDetection(m_tracks[static_cast<std::size_t>(trackPos)],
                           detections[static_cast<std::size_t>(detIndices[static_cast<std::size_t>(detPos)])],
                           context);
        }
    };

    associate(highIndices, highUsed);

    std::vector<char> lowUsed(lowIndices.size(), 0);
    associate(lowIndices, lowUsed);

    std::vector<InternalTrack> surviving;
    surviving.reserve(m_tracks.size());
    for (std::size_t t = 0; t < m_tracks.size(); ++t)
    {
        if (trackUsed[t])
        {
            surviving.push_back(std::move(m_tracks[t]));
            continue;
        }

        InternalTrack& track = m_tracks[t];
        if (track.state == TrackState::Tentative)
        {
            ++m_stats.removedTracks;
            continue;
        }
        if (track.state != TrackState::Lost)
            ++m_stats.lostTracks;
        track.state = TrackState::Lost;
        track.timeSinceUpdate += 1;
        if (track.timeSinceUpdate > m_config.maxLostFrames)
        {
            ++m_stats.removedTracks;
            continue;
        }
        surviving.push_back(std::move(track));
    }
    m_tracks = std::move(surviving);

    for (std::size_t i = 0; i < highIndices.size(); ++i)
    {
        if (highUsed[i])
            continue;
        const Detection& detection = detections[static_cast<std::size_t>(highIndices[i])];
        InternalTrack created(detection.box);
        created.id = m_nextId++;
        created.classId = detection.classId;
        created.label = detection.label;
        created.confidence = detection.confidence;
        created.firstSeen = context.timestamp;
        created.lastSeen = context.timestamp;
        created.age = 1;
        created.hits = 1;
        created.state = created.hits >= m_config.minHits ? TrackState::Confirmed
                                                         : TrackState::Tentative;
        m_tracks.push_back(std::move(created));
        ++m_stats.createdTracks;
    }

    std::vector<Track> published;
    published.reserve(m_tracks.size());
    for (InternalTrack& track : m_tracks)
    {
        appendTrajectory(track, context);
        if (track.state == TrackState::Tentative)
            continue;
        published.push_back(toPublic(track));
    }

    m_stats.activeTracks = published.size();
    m_stats.lastUpdateLatencyMs = std::chrono::duration<double, std::milli>(
                                      std::chrono::steady_clock::now() - started)
                                      .count();
    return published;
}

void ByteTrackTracker::reset()
{
    m_tracks.clear();
    m_nextId = 1;
    m_stats = {};
    m_lastTimestamp.reset();
}

TrackerStats ByteTrackTracker::stats() const
{
    return m_stats;
}

TrackerConfig ByteTrackTracker::config() const
{
    return m_config;
}

} // namespace visionlab
