#ifndef EVENTQUERY_H
#define EVENTQUERY_H

#include <cstddef>
#include <cstdint>
#include <optional>

#include "core/VisionEvent.h"

namespace visionlab {

struct EventQuery
{
    std::optional<EventType> type;
    std::uint64_t afterRowId = 0;
    std::size_t limit = 500;
};

struct StoredEvent
{
    std::int64_t rowId = 0;
    std::int64_t wallUtcMs = 0;
    VisionEvent event;
};

} // namespace visionlab

#endif // EVENTQUERY_H
