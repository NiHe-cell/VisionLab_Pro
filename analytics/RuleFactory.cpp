#include "analytics/RuleFactory.h"

#include <cmath>

#include "analytics/CountingRule.h"
#include "analytics/LineCrossingRule.h"
#include "analytics/LoiteringRule.h"
#include "analytics/RoiIntrusionRule.h"

namespace visionlab {
namespace {

bool samePoint(const cv::Point2f& a, const cv::Point2f& b)
{
    return std::fabs(a.x - b.x) < 1e-6F && std::fabs(a.y - b.y) < 1e-6F;
}

} // namespace

std::unique_ptr<IRule> makeRule(const RuleSpec& spec)
{
    if (spec.ruleId.empty())
        return nullptr;

    switch (spec.kind)
    {
    case RuleKind::RoiIntrusion:
        if (spec.polygon.size() < 3)
            return nullptr;
        return std::make_unique<RoiIntrusionRule>(
            RoiConfig{spec.ruleId, spec.polygon, spec.classIds});
    case RuleKind::LineCrossing:
        if (samePoint(spec.a, spec.b))
            return nullptr;
        return std::make_unique<LineCrossingRule>(
            LineConfig{spec.ruleId, spec.a, spec.b, spec.classIds});
    case RuleKind::Loitering:
        if (spec.polygon.size() < 3 || spec.loiterSeconds <= 0.0)
            return nullptr;
        return std::make_unique<LoiteringRule>(
            LoiterConfig{spec.ruleId, spec.polygon, spec.classIds, spec.loiterSeconds});
    case RuleKind::Counting:
        if (samePoint(spec.a, spec.b))
            return nullptr;
        return std::make_unique<CountingRule>(
            CountConfig{spec.ruleId, spec.a, spec.b, spec.classIds});
    }
    return nullptr;
}

} // namespace visionlab
