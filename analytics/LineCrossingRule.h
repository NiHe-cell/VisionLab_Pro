#ifndef LINECROSSINGRULE_H
#define LINECROSSINGRULE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "analytics/IRule.h"

namespace visionlab {

struct LineConfig
{
    std::string ruleId = "line";
    cv::Point2f a{};
    cv::Point2f b{};
    std::vector<int> classIds;
};

class LineCrossingRule final : public IRule
{
public:
    explicit LineCrossingRule(LineConfig config);

    std::string id() const override;
    std::string name() const override;
    EventType eventType() const override;
    std::vector<VisionEvent> evaluate(
        const std::vector<Track>& tracks,
        const RuleContext& context) override;
    void reset() override;

private:
    LineConfig m_config;
    std::unordered_map<std::uint64_t, cv::Point2f> m_lastFoot;
};

} // namespace visionlab

#endif // LINECROSSINGRULE_H
