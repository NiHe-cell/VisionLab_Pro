#ifndef IRULE_H
#define IRULE_H

#include <string>
#include <vector>

#include "core/Track.h"
#include "core/VisionEvent.h"

namespace visionlab {

// 智能分析规则扩展点：一帧 Track 列表进，本帧新 VisionEvent 列表出。
//
// 约定：
// - 实现不得读取或写入像素；只使用 Track 框与 RuleContext。
// - evaluate 不得修改入参 tracks。
// - 只发过渡 / 阈值事件，不每帧重复同一状态。
// - 启用/禁用由 RuleEngine 持有，不放进本接口。
// - 同一实例的 evaluate / reset 不承诺可并发调用；只应由推理线程驱动
//   （VisionPipeline::start() 在启动 jthread 之前的 reset 除外）。
class IRule
{
public:
    virtual ~IRule() = default;

    virtual std::string id() const = 0;
    virtual std::string name() const = 0;
    virtual EventType eventType() const = 0;

    virtual std::vector<VisionEvent> evaluate(
        const std::vector<Track>& tracks,
        const RuleContext& context) = 0;

    virtual void reset() = 0;
};

} // namespace visionlab

#endif // IRULE_H
