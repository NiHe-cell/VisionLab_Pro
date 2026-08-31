#ifndef ROIINTRUSIONRULE_H
#define ROIINTRUSIONRULE_H

#include <string>
#include <unordered_set>
#include <vector>

#include "analytics/IRule.h"

namespace visionlab {

struct RoiConfig
{
    std::string ruleId = "roi";
    std::vector<cv::Point2f> polygon;
    std::vector<int> classIds;
};

class RoiIntrusionRule final : public IRule
{
public:
    explicit RoiIntrusionRule(RoiConfig config);

    std::string id() const override;
    std::string name() const override;
    EventType eventType() const override;
    std::vector<VisionEvent> evaluate(
        const std::vector<Track>& tracks,
        const RuleContext& context) override;
    void reset() override;

private:
    RoiConfig m_config;
    std::unordered_set<std::uint64_t> m_inside;
};

} // namespace visionlab

#endif // ROIINTRUSIONRULE_H
