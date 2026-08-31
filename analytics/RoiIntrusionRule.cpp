#include "RoiIntrusionRule.h"

#include <algorithm>
#include <unordered_set>

#include "analytics/RuleGeometry.h"

namespace visionlab {
namespace {

bool classAllowed(const std::vector<int>& classIds, int classId)
{
    return classIds.empty()
        || std::find(classIds.begin(), classIds.end(), classId) != classIds.end();
}

VisionEvent makeIntrusionEvent(const Track& track,
                               const RuleContext& context,
                               const std::string& ruleId)
{
    VisionEvent event;
    event.type = EventType::RoiIntrusion;
    event.ruleId = ruleId;
    event.sourceId = context.sourceId;
    event.trackId = track.trackId;
    event.classId = track.classId;
    event.label = track.label;
    event.confidence = track.confidence;
    event.box = track.box;
    event.frameId = context.frameId;
    event.timestamp = context.timestamp;
    event.message = "intrusion";
    return event;
}

} // namespace

RoiIntrusionRule::RoiIntrusionRule(RoiConfig config)
    : m_config(std::move(config))
{
}

std::string RoiIntrusionRule::id() const
{
    return m_config.ruleId;
}

std::string RoiIntrusionRule::name() const
{
    return "roi-intrusion";
}

EventType RoiIntrusionRule::eventType() const
{
    return EventType::RoiIntrusion;
}

std::vector<VisionEvent> RoiIntrusionRule::evaluate(
    const std::vector<Track>& tracks,
    const RuleContext& context)
{
    if (m_config.polygon.size() < 3)
        return {};

    std::unordered_set<std::uint64_t> seen;
    std::vector<VisionEvent> events;
    for (const Track& track : tracks)
    {
        seen.insert(track.trackId);
        const bool inside = classAllowed(m_config.classIds, track.classId)
            && pointInPolygon(footPoint(track.box), m_config.polygon);
        if (!inside)
        {
            m_inside.erase(track.trackId);
            continue;
        }
        if (m_inside.insert(track.trackId).second)
            events.push_back(makeIntrusionEvent(track, context, m_config.ruleId));
    }

    for (auto it = m_inside.begin(); it != m_inside.end();)
    {
        if (seen.contains(*it))
            ++it;
        else
            it = m_inside.erase(it);
    }
    return events;
}

void RoiIntrusionRule::reset()
{
    m_inside.clear();
}

} // namespace visionlab
