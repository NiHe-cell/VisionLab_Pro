#include "LineCrossingRule.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "analytics/RuleGeometry.h"

namespace visionlab {
namespace {

bool classAllowed(const std::vector<int>& classIds, int classId)
{
    return classIds.empty()
        || std::find(classIds.begin(), classIds.end(), classId) != classIds.end();
}

bool samePoint(const cv::Point2f& a, const cv::Point2f& b)
{
    return std::fabs(a.x - b.x) < 1e-6F && std::fabs(a.y - b.y) < 1e-6F;
}

VisionEvent makeCrossingEvent(const Track& track,
                              const RuleContext& context,
                              const std::string& ruleId,
                              CrossingDirection direction)
{
    VisionEvent event;
    event.type = EventType::LineCrossing;
    event.ruleId = ruleId;
    event.sourceId = context.sourceId;
    event.trackId = track.trackId;
    event.classId = track.classId;
    event.label = track.label;
    event.confidence = track.confidence;
    event.box = track.box;
    event.frameId = context.frameId;
    event.timestamp = context.timestamp;
    event.direction = direction;
    event.message = direction == CrossingDirection::Forward ? "A→B" : "B→A";
    return event;
}

} // namespace

LineCrossingRule::LineCrossingRule(LineConfig config)
    : m_config(std::move(config))
{
}

std::string LineCrossingRule::id() const
{
    return m_config.ruleId;
}

std::string LineCrossingRule::name() const
{
    return "line-crossing";
}

EventType LineCrossingRule::eventType() const
{
    return EventType::LineCrossing;
}

std::vector<VisionEvent> LineCrossingRule::evaluate(
    const std::vector<Track>& tracks,
    const RuleContext& context)
{
    if (samePoint(m_config.a, m_config.b))
        return {};

    std::unordered_set<std::uint64_t> seen;
    std::vector<VisionEvent> events;
    for (const Track& track : tracks)
    {
        seen.insert(track.trackId);
        if (!classAllowed(m_config.classIds, track.classId))
        {
            m_lastFoot.erase(track.trackId);
            continue;
        }

        const cv::Point2f foot = footPoint(track.box);
        const auto previous = m_lastFoot.find(track.trackId);
        if (previous != m_lastFoot.end())
        {
            const CrossingDirection direction = classifyCrossing(
                previous->second, foot, m_config.a, m_config.b);
            if (direction != CrossingDirection::None)
                events.push_back(makeCrossingEvent(
                    track, context, m_config.ruleId, direction));
        }
        m_lastFoot[track.trackId] = foot;
    }

    for (auto it = m_lastFoot.begin(); it != m_lastFoot.end();)
    {
        if (seen.contains(it->first))
            ++it;
        else
            it = m_lastFoot.erase(it);
    }
    return events;
}

void LineCrossingRule::reset()
{
    m_lastFoot.clear();
}

} // namespace visionlab
