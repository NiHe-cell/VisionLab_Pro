#ifndef FAKERULE_H
#define FAKERULE_H

#include <string>
#include <vector>

#include "analytics/IRule.h"
#include "core/Track.h"
#include "core/VisionEvent.h"

// 仅测试：按当前帧 Track 下标各产一条事件，不做几何判定。
class FakeRule : public visionlab::IRule
{
public:
    explicit FakeRule(std::string id = "fake")
        : m_id(std::move(id))
    {
    }

    std::string id() const override { return m_id; }

    std::string name() const override { return "fake"; }

    visionlab::EventType eventType() const override
    {
        return visionlab::EventType::RoiIntrusion;
    }

    std::vector<visionlab::VisionEvent> evaluate(
        const std::vector<visionlab::Track>& tracks,
        const visionlab::RuleContext& context) override
    {
        std::vector<visionlab::VisionEvent> events;
        events.reserve(tracks.size());
        for (const visionlab::Track& track : tracks)
        {
            visionlab::VisionEvent event;
            event.ruleId = m_id;
            event.sourceId = context.sourceId;
            event.trackId = track.trackId;
            event.classId = track.classId;
            event.label = track.label;
            event.confidence = track.confidence;
            event.box = track.box;
            event.frameId = context.frameId;
            event.timestamp = context.timestamp;
            event.message = "fake";
            events.push_back(event);
        }
        return events;
    }

    void reset() override { ++m_resetCount; }

    int resetCount() const { return m_resetCount; }

private:
    std::string m_id;
    int m_resetCount = 0;
};

#endif // FAKERULE_H
