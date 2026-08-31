#include "LoiteringRule.h"

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

VisionEvent makeLoiterEvent(const Track& track,
                            const RuleContext& context,
                            const std::string& ruleId)
{
    VisionEvent event;
    event.type = EventType::Loitering;
    event.ruleId = ruleId;
    event.sourceId = context.sourceId;
    event.trackId = track.trackId;
    event.classId = track.classId;
    event.label = track.label;
    event.confidence = track.confidence;
    event.box = track.box;
    event.frameId = context.frameId;
    event.timestamp = context.timestamp;
    event.message = "loitering";
    return event;
}

} // namespace

LoiteringRule::LoiteringRule(LoiterConfig config)
    : m_config(std::move(config))
{
}

std::string LoiteringRule::id() const
{
    return m_config.ruleId;
}

std::string LoiteringRule::name() const
{
    return "loitering";
}

EventType LoiteringRule::eventType() const
{
    return EventType::Loitering;
}

std::vector<VisionEvent> LoiteringRule::evaluate(
    const std::vector<Track>& tracks,
    const RuleContext& context)
{
    if (m_config.polygon.size() < 3 || m_config.loiterSeconds <= 0.0)
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
            m_stays.erase(track.trackId);
            continue;
        }

        auto [it, inserted] = m_stays.try_emplace(track.trackId);
        if (inserted)
            it->second.entered = context.timestamp;

        if (it->second.emitted)
            continue;

        const double elapsed = std::chrono::duration<double>(
                                   context.timestamp - it->second.entered)
                                   .count();
        if (elapsed >= m_config.loiterSeconds)
        {
            it->second.emitted = true;
            events.push_back(makeLoiterEvent(track, context, m_config.ruleId));
        }
    }

    for (auto it = m_stays.begin(); it != m_stays.end();)
    {
        if (seen.contains(it->first))
            ++it;
        else
            it = m_stays.erase(it);
    }
    return events;
}

void LoiteringRule::reset()
{
    m_stays.clear();
}

} // namespace visionlab
