#include "RuleEngine.h"

#include <chrono>
#include <iostream>

#include <opencv2/core.hpp>

namespace visionlab {

const RuleEngine::Entry* RuleEngine::find(std::string_view ruleId) const
{
    for (const Entry& entry : m_entries)
    {
        if (entry.rule && entry.rule->id() == ruleId)
            return &entry;
    }
    return nullptr;
}

RuleEngine::Entry* RuleEngine::find(std::string_view ruleId)
{
    return const_cast<Entry*>(static_cast<const RuleEngine*>(this)->find(ruleId));
}

bool RuleEngine::addRule(std::unique_ptr<IRule> rule)
{
    if (!rule || rule->id().empty())
        return false;
    if (find(rule->id()) != nullptr)
        return false;
    m_entries.push_back(Entry{std::move(rule), true});
    return true;
}

bool RuleEngine::setEnabled(std::string_view ruleId, bool enabled)
{
    Entry* entry = find(ruleId);
    if (!entry)
        return false;
    entry->enabled = enabled;
    return true;
}

bool RuleEngine::isEnabled(std::string_view ruleId) const
{
    const Entry* entry = find(ruleId);
    return entry != nullptr && entry->enabled;
}

std::size_t RuleEngine::size() const
{
    return m_entries.size();
}

std::vector<VisionEvent> RuleEngine::evaluate(
    const std::vector<Track>& tracks,
    const RuleContext& context)
{
    if (m_entries.empty())
    {
        m_lastEvaluateLatencyMs = 0.0;
        return {};
    }

    const auto begin = std::chrono::steady_clock::now();
    std::vector<VisionEvent> events;
    for (Entry& entry : m_entries)
    {
        if (!entry.enabled || !entry.rule)
            continue;
        try
        {
            std::vector<VisionEvent> part = entry.rule->evaluate(tracks, context);
            for (VisionEvent& event : part)
            {
                event.eventId = m_nextEventId++;
                ++m_eventsEmitted;
                events.push_back(std::move(event));
            }
        }
        catch (const cv::Exception& e)
        {
            std::cerr << "[RuleEngine] rule " << entry.rule->id()
                      << " 异常: " << e.what() << '\n';
        }
    }
    m_lastEvaluateLatencyMs = std::chrono::duration<double, std::milli>(
                                  std::chrono::steady_clock::now() - begin)
                                  .count();
    return events;
}

void RuleEngine::reset()
{
    for (Entry& entry : m_entries)
    {
        if (entry.rule)
            entry.rule->reset();
    }
    m_nextEventId = 1;
    m_eventsEmitted = 0;
    m_lastEvaluateLatencyMs = 0.0;
}

RuleEngineStats RuleEngine::stats() const
{
    RuleEngineStats stats;
    stats.ruleCount = m_entries.size();
    for (const Entry& entry : m_entries)
    {
        if (entry.enabled)
            ++stats.enabledRules;
    }
    stats.eventsEmitted = m_eventsEmitted;
    stats.lastEvaluateLatencyMs = m_lastEvaluateLatencyMs;
    return stats;
}

} // namespace visionlab
