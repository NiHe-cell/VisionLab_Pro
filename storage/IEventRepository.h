#ifndef IEVENTREPOSITORY_H
#define IEVENTREPOSITORY_H

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "core/VisionEvent.h"
#include "storage/EventQuery.h"

namespace visionlab {

class IEventRepository
{
public:
    virtual ~IEventRepository() = default;

    virtual bool open(const std::filesystem::path& dbPath) = 0;
    virtual bool insert(const VisionEvent& event, std::int64_t wallUtcMs) = 0;
    virtual std::vector<StoredEvent> query(const EventQuery& query) const = 0;
    virtual void prune(std::size_t maxRows) = 0;
    virtual std::size_t count() const = 0;
    virtual void close() = 0;
};

} // namespace visionlab

#endif // IEVENTREPOSITORY_H
