#ifndef RULEENGINE_H
#define RULEENGINE_H

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "analytics/IRule.h"
#include "core/Track.h"
#include "core/VisionEvent.h"

namespace visionlab {

class RuleEngine
{
public:
    bool addRule(std::unique_ptr<IRule> rule);
    bool setEnabled(std::string_view ruleId, bool enabled);
    bool isEnabled(std::string_view ruleId) const;
    std::size_t size() const;

    std::vector<VisionEvent> evaluate(
        const std::vector<Track>& tracks,
        const RuleContext& context);

    void reset();
    void clear();
    std::vector<std::string> ruleIds() const;
    RuleEngineStats stats() const;

private:
    struct Entry
    {
        std::unique_ptr<IRule> rule;
        bool enabled = true;
    };

    const Entry* find(std::string_view ruleId) const;
    Entry* find(std::string_view ruleId);

    std::vector<Entry> m_entries;
    std::uint64_t m_nextEventId = 1;
    std::uint64_t m_eventsEmitted = 0;
    double m_lastEvaluateLatencyMs = 0.0;
};

} // namespace visionlab

#endif // RULEENGINE_H
