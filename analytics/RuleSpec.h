#ifndef RULESPEC_H
#define RULESPEC_H

#include <cstdint>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace visionlab {

enum class RuleKind : std::uint8_t
{
    RoiIntrusion,
    LineCrossing,
    Loitering,
    Counting,
};

// UI / 会话持有的规则规格。enabled 由 RuleEngine::setEnabled 应用，不放进 IRule。
struct RuleSpec
{
    std::string ruleId;
    RuleKind kind = RuleKind::RoiIntrusion;
    bool enabled = true;
    std::vector<cv::Point2f> polygon;
    cv::Point2f a{};
    cv::Point2f b{};
    std::vector<int> classIds;
    double loiterSeconds = 5.0;
};

} // namespace visionlab

#endif // RULESPEC_H
