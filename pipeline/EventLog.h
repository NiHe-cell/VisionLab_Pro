#ifndef EVENTLOG_H
#define EVENTLOG_H

#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

#include "core/VisionEvent.h"

namespace visionlab {

// 有界事件邮箱：推理线程 push，任意线程 snapshot。满则 DropOldest。
class EventLog
{
public:
    explicit EventLog(std::size_t capacity = 256);

    void push(const std::vector<VisionEvent>& events);
    std::vector<VisionEvent> snapshot() const;
    void reset();
    std::size_t size() const;

private:
    const std::size_t m_capacity;
    mutable std::mutex m_mutex;
    std::deque<VisionEvent> m_events;
};

} // namespace visionlab

#endif // EVENTLOG_H
