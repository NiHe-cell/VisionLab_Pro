#include <QtTest/QtTest>

#include "analytics/LineCrossingRule.h"
#include "core/Track.h"
#include "core/VisionEvent.h"

using visionlab::CrossingDirection;
using visionlab::EventType;
using visionlab::LineConfig;
using visionlab::LineCrossingRule;
using visionlab::RuleContext;
using visionlab::Track;

namespace {

Track makeTrack(std::uint64_t trackId, const cv::Rect& box, int classId = 0)
{
    Track track;
    track.trackId = trackId;
    track.classId = classId;
    track.label = "person";
    track.confidence = 0.9F;
    track.box = box;
    return track;
}

cv::Rect boxWithFoot(int fx, int fy)
{
    return {fx - 10, fy - 20, 20, 20};
}

LineCrossingRule makeRule(std::vector<int> classIds = {})
{
    LineConfig config;
    config.a = {0.0F, 50.0F};
    config.b = {100.0F, 50.0F};
    config.classIds = std::move(classIds);
    return LineCrossingRule{std::move(config)};
}

RuleContext ctx()
{
    RuleContext context;
    context.frameId = 4;
    context.sourceId = "cam:0";
    return context;
}

} // namespace

class LineCrossingRuleTest : public QObject
{
    Q_OBJECT

private slots:
    void forwardCrossingEmitsAtoB();
    void reverseCrossingEmitsBtoA();
    void parallelDoesNotEmit();
    void stopOnLineDoesNotEmit();
    void stayOnSameSideDoesNotEmit();
    void classFilterSkipsOtherIds();
    void disappearDoesNotInventCrossing();
    void resetRequiresNewPreviousPoint();
    void zeroLengthLineEmitsNothing();
};

void LineCrossingRuleTest::forwardCrossingEmitsAtoB()
{
    auto rule = makeRule();
    QCOMPARE(rule.id(), std::string("line"));
    QCOMPARE(rule.name(), std::string("line-crossing"));
    QCOMPARE(rule.eventType(), EventType::LineCrossing);

    QVERIFY(rule.evaluate({makeTrack(8, boxWithFoot(50, 80))}, ctx()).empty());
    const Track after = makeTrack(8, boxWithFoot(50, 20));
    const auto events = rule.evaluate({after}, ctx());

    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events[0].eventId, std::uint64_t{0});
    QCOMPARE(events[0].type, EventType::LineCrossing);
    QCOMPARE(events[0].trackId, std::uint64_t{8});
    QCOMPARE(events[0].direction, CrossingDirection::Forward);
    QCOMPARE(events[0].message, std::string("A→B"));
    QCOMPARE(events[0].box, after.box);
    QCOMPARE(events[0].sourceId, std::string("cam:0"));
}

void LineCrossingRuleTest::reverseCrossingEmitsBtoA()
{
    auto rule = makeRule();
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(50, 20))}, ctx()).empty());
    const auto events = rule.evaluate({makeTrack(1, boxWithFoot(50, 80))}, ctx());
    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events[0].direction, CrossingDirection::Reverse);
    QCOMPARE(events[0].message, std::string("B→A"));
}

void LineCrossingRuleTest::parallelDoesNotEmit()
{
    auto rule = makeRule();
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(20, 80))}, ctx()).empty());
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(80, 80))}, ctx()).empty());
}

void LineCrossingRuleTest::stopOnLineDoesNotEmit()
{
    auto rule = makeRule();
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(50, 80))}, ctx()).empty());
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(50, 50))}, ctx()).empty());
}

void LineCrossingRuleTest::stayOnSameSideDoesNotEmit()
{
    auto rule = makeRule();
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(40, 80))}, ctx()).empty());
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(60, 80))}, ctx()).empty());
}

void LineCrossingRuleTest::classFilterSkipsOtherIds()
{
    auto rule = makeRule({0});
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(50, 80), 1)}, ctx()).empty());
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(50, 20), 1)}, ctx()).empty());
}

void LineCrossingRuleTest::disappearDoesNotInventCrossing()
{
    auto rule = makeRule();
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(50, 80))}, ctx()).empty());
    QVERIFY(rule.evaluate({}, ctx()).empty());
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(50, 20))}, ctx()).empty());
}

void LineCrossingRuleTest::resetRequiresNewPreviousPoint()
{
    auto rule = makeRule();
    rule.evaluate({makeTrack(1, boxWithFoot(50, 80))}, ctx());
    QCOMPARE(rule.evaluate({makeTrack(1, boxWithFoot(50, 20))}, ctx()).size(), std::size_t{1});
    rule.reset();
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(50, 20))}, ctx()).empty());
    QCOMPARE(rule.evaluate({makeTrack(1, boxWithFoot(50, 80))}, ctx()).size(), std::size_t{1});
}

void LineCrossingRuleTest::zeroLengthLineEmitsNothing()
{
    LineConfig config;
    config.a = {10.0F, 10.0F};
    config.b = {10.0F, 10.0F};
    LineCrossingRule rule{config};
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(10, 20))}, ctx()).empty());
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(10, 0))}, ctx()).empty());
}

QTEST_APPLESS_MAIN(LineCrossingRuleTest)

#include "LineCrossingRuleTest.moc"
