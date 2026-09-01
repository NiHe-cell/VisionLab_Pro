#ifndef RULEFACTORY_H
#define RULEFACTORY_H

#include <memory>

#include "analytics/IRule.h"
#include "analytics/RuleSpec.h"

namespace visionlab {

// 空 id、未知 kind、ROI/Loiter 顶点 <3、Line/Count 的 A==B、loiterSeconds<=0 → nullptr。
std::unique_ptr<IRule> makeRule(const RuleSpec& spec);

} // namespace visionlab

#endif // RULEFACTORY_H
