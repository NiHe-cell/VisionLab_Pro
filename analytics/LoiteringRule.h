#ifndef LOITERINGRULE_H
#define LOITERINGRULE_H

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include "analytics/IRule.h"

namespace visionlab {

struct LoiterConfig
{
    std::string ruleId = "loiter";
    std::vector<cv::Point2f> polygon;
    std::vector<int> classIds;
    double loiterSeconds = 5.0;
};

class LoiteringRule final : public IRule
{
public:
    explicit LoiteringRule(LoiterConfig config);

    std::string id() const override;
    std::string name() const override;
    EventType eventType() const override;
    std::vector<VisionEvent> evaluate(
        const std::vector<Track>& tracks,
        const RuleContext& context) override;
    void reset() override;

private:
    struct Stay
    {
        std::chrono::steady_clock::time_point entered{};
        bool emitted = false;
    };

    LoiterConfig m_config;
    std::unordered_map<std::uint64_t, Stay> m_stays;
};

} // namespace visionlab

#endif // LOITERINGRULE_H
