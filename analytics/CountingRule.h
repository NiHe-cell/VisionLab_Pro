#ifndef COUNTINGRULE_H
#define COUNTINGRULE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <opencv2/core.hpp>

#include "analytics/IRule.h"
#include "core/Track.h"
#include "core/VisionEvent.h"

namespace visionlab {

struct CountConfig
{
    std::string ruleId = "count";
    cv::Point2f a{};
    cv::Point2f b{};
    std::vector<int> classIds;
};

class CountingRule final : public IRule
{
public:
    explicit CountingRule(CountConfig config = {});

    std::string id() const override;
    std::string name() const override;
    EventType eventType() const override;
    std::vector<VisionEvent> evaluate(
        const std::vector<Track>& tracks,
        const RuleContext& context) override;
    void reset() override;

    std::uint64_t countIn() const;
    std::uint64_t countOut() const;
    std::size_t occupancy() const;

private:
    bool acceptsClass(int classId) const;
    VisionEvent makeEvent(const Track& track,
                          const RuleContext& context,
                          CrossingDirection direction,
                          const std::string& message) const;

    CountConfig m_config;
    std::unordered_map<std::uint64_t, cv::Point2f> m_lastFoot;
    std::unordered_set<std::uint64_t> m_inside;
    std::uint64_t m_countIn = 0;
    std::uint64_t m_countOut = 0;
};

} // namespace visionlab

#endif // COUNTINGRULE_H
