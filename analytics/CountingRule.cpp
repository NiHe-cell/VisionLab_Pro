#include "CountingRule.h"

#include <algorithm>
#include <unordered_set>

#include "analytics/RuleGeometry.h"

namespace visionlab {

CountingRule::CountingRule(CountConfig config)
    : m_config(std::move(config))
{
}

std::string CountingRule::id() const
{
    return m_config.ruleId;
}

std::string CountingRule::name() const
{
    return "counting";
}

EventType CountingRule::eventType() const
{
    return EventType::Counting;
}

bool CountingRule::acceptsClass(int classId) const
{
    if (m_config.classIds.empty())
        return true;
    return std::find(m_config.classIds.begin(), m_config.classIds.end(), classId)
           != m_config.classIds.end();
}

VisionEvent CountingRule::makeEvent(const Track& track,
                                    const RuleContext& context,
                                    CrossingDirection direction,
                                    const std::string& message) const
{
    VisionEvent event;
    event.type = EventType::Counting;
    event.ruleId = m_config.ruleId;
    event.sourceId = context.sourceId;
    event.trackId = track.trackId;
    event.classId = track.classId;
    event.label = track.label;
    event.confidence = track.confidence;
    event.box = track.box;
    event.frameId = context.frameId;
    event.timestamp = context.timestamp;
    event.message = message;
    event.direction = direction;
    event.countIn = m_countIn;
    event.countOut = m_countOut;
    event.occupancy = m_inside.size();
    return event;
}

std::vector<VisionEvent> CountingRule::evaluate(
    const std::vector<Track>& tracks,
    const RuleContext& context)
{
    if (m_config.a == m_config.b)
        return {};

    std::vector<VisionEvent> events;
    std::unordered_set<std::uint64_t> seen;

    for (const Track& track : tracks)
    {
        seen.insert(track.trackId);
        if (!acceptsClass(track.classId))
        {
            m_lastFoot.erase(track.trackId);
            m_inside.erase(track.trackId);
            continue;
        }

        const cv::Point2f foot = footPoint(track.box);
        const auto previous = m_lastFoot.find(track.trackId);
        if (previous != m_lastFoot.end())
        {
            const CrossingDirection direction =
                classifyCrossing(previous->second, foot, m_config.a, m_config.b);
            if (direction == CrossingDirection::Forward && !m_inside.contains(track.trackId))
            {
                ++m_countIn;
                m_inside.insert(track.trackId);
                events.push_back(makeEvent(track, context, direction, "IN"));
            }
            else if (direction == CrossingDirection::Reverse && m_inside.contains(track.trackId))
            {
                ++m_countOut;
                m_inside.erase(track.trackId);
                events.push_back(makeEvent(track, context, direction, "OUT"));
            }
        }
        m_lastFoot[track.trackId] = foot;
    }

    for (auto it = m_lastFoot.begin(); it != m_lastFoot.end();)
    {
        if (!seen.contains(it->first))
        {
            m_inside.erase(it->first);
            it = m_lastFoot.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return events;
}

void CountingRule::reset()
{
    m_lastFoot.clear();
    m_inside.clear();
    m_countIn = 0;
    m_countOut = 0;
}

std::uint64_t CountingRule::countIn() const
{
    return m_countIn;
}

std::uint64_t CountingRule::countOut() const
{
    return m_countOut;
}

std::size_t CountingRule::occupancy() const
{
    return m_inside.size();
}

} // namespace visionlab
