#include "EventLog.h"

namespace visionlab {

EventLog::EventLog(std::size_t capacity)
    : m_capacity(capacity == 0 ? 1 : capacity)
{
}

void EventLog::push(const std::vector<VisionEvent>& events)
{
    std::lock_guard lock(m_mutex);
    for (const VisionEvent& event : events)
    {
        if (m_events.size() >= m_capacity)
            m_events.pop_front();
        m_events.push_back(event);
    }
}

std::vector<VisionEvent> EventLog::snapshot() const
{
    std::lock_guard lock(m_mutex);
    return {m_events.begin(), m_events.end()};
}

void EventLog::reset()
{
    std::lock_guard lock(m_mutex);
    m_events.clear();
}

std::size_t EventLog::size() const
{
    std::lock_guard lock(m_mutex);
    return m_events.size();
}

} // namespace visionlab
