#include <QtTest/QtTest>

#include <memory>

#include "analytics/RuleEngine.h"
#include "analytics/RuleFactory.h"
#include "analytics/RuleGeometry.h"
#include "analytics/RuleSpec.h"
#include "core/Track.h"
#include "core/VisionEvent.h"

using visionlab::EventType;
using visionlab::RuleContext;
using visionlab::RuleEngine;
using visionlab::RuleKind;
using visionlab::RuleSpec;
using visionlab::Track;
using visionlab::makeRule;
using visionlab::polygonFromRect;

namespace {

Track makeTrack(std::uint64_t trackId, const cv::Rect& box)
{
    Track track;
    track.trackId = trackId;
    track.classId = 0;
    track.label = "person";
    track.confidence = 0.9F;
    track.box = box;
    return track;
}

cv::Rect boxWithFoot(int fx, int fy)
{
    return {fx - 10, fy - 20, 20, 20};
}

RuleSpec validRoi(std::string id = "roi")
{
    RuleSpec spec;
    spec.ruleId = std::move(id);
    spec.kind = RuleKind::RoiIntrusion;
    spec.polygon = polygonFromRect(cv::Rect(0, 0, 100, 100));
    return spec;
}

} // namespace

class RuleSpecTest : public QObject
{
    Q_OBJECT

private slots:
    void validRoiMakeRuleEmitsEnter();
    void invalidGeometryAndEmptyIdReturnNull();
    void unknownKindReturnsNull();
    void clearDropsRulesAndRestartsEventIds();
};

void RuleSpecTest::validRoiMakeRuleEmitsEnter()
{
    auto rule = makeRule(validRoi());
    QVERIFY(rule != nullptr);
    QCOMPARE(rule->id(), std::string("roi"));
    QCOMPARE(rule->eventType(), EventType::RoiIntrusion);

    RuleContext context;
    context.sourceId = "cam:0";
    QVERIFY(rule->evaluate({makeTrack(1, boxWithFoot(150, 50))}, context).empty());
    const auto events = rule->evaluate({makeTrack(1, boxWithFoot(50, 50))}, context);
    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events[0].eventId, std::uint64_t{0});
    QCOMPARE(events[0].trackId, std::uint64_t{1});
    QCOMPARE(events[0].message, std::string("intrusion"));
}

void RuleSpecTest::invalidGeometryAndEmptyIdReturnNull()
{
    RuleSpec fewVerts = validRoi();
    fewVerts.polygon = {{0.0F, 0.0F}, {1.0F, 0.0F}};
    QVERIFY(makeRule(fewVerts) == nullptr);

    RuleSpec emptyId = validRoi();
    emptyId.ruleId.clear();
    QVERIFY(makeRule(emptyId) == nullptr);

    RuleSpec line;
    line.ruleId = "line";
    line.kind = RuleKind::LineCrossing;
    line.a = {0.0F, 0.0F};
    line.b = {0.0F, 0.0F};
    QVERIFY(makeRule(line) == nullptr);

    RuleSpec count;
    count.ruleId = "count";
    count.kind = RuleKind::Counting;
    count.a = {1.0F, 1.0F};
    count.b = {1.0F, 1.0F};
    QVERIFY(makeRule(count) == nullptr);

    RuleSpec loiter = validRoi("loiter");
    loiter.kind = RuleKind::Loitering;
    loiter.loiterSeconds = 0.0;
    QVERIFY(makeRule(loiter) == nullptr);
}

void RuleSpecTest::unknownKindReturnsNull()
{
    RuleSpec spec = validRoi();
    spec.kind = static_cast<RuleKind>(255);
    QVERIFY(makeRule(spec) == nullptr);
}

void RuleSpecTest::clearDropsRulesAndRestartsEventIds()
{
    RuleEngine engine;
    QVERIFY(engine.addRule(makeRule(validRoi("roi"))));
    QVERIFY(engine.addRule(makeRule(validRoi("roi-2"))));
    QCOMPARE(engine.ruleIds(), (std::vector<std::string>{"roi", "roi-2"}));
    QCOMPARE(engine.size(), std::size_t{2});

    engine.clear();
    QCOMPARE(engine.size(), std::size_t{0});
    QVERIFY(engine.ruleIds().empty());
    QCOMPARE(engine.stats().ruleCount, std::size_t{0});
    QCOMPARE(engine.stats().eventsEmitted, std::uint64_t{0});

    QVERIFY(engine.addRule(makeRule(validRoi("roi"))));
    engine.evaluate({makeTrack(1, boxWithFoot(150, 50))}, RuleContext{});
    const auto events = engine.evaluate({makeTrack(1, boxWithFoot(50, 50))}, RuleContext{});
    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events.front().eventId, std::uint64_t{1});
}

QTEST_APPLESS_MAIN(RuleSpecTest)

#include "RuleSpecTest.moc"
