#include <QtTest/QtTest>

#include "analytics/RoiIntrusionRule.h"
#include "analytics/RuleGeometry.h"
#include "core/Track.h"
#include "core/VisionEvent.h"

using visionlab::EventType;
using visionlab::RoiConfig;
using visionlab::RoiIntrusionRule;
using visionlab::RuleContext;
using visionlab::Track;
using visionlab::polygonFromRect;

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

// 宽高 20 的框，脚点落在 (fx, fy)。
cv::Rect boxWithFoot(int fx, int fy)
{
    return {fx - 10, fy - 20, 20, 20};
}

RoiIntrusionRule makeRule(std::vector<int> classIds = {})
{
    RoiConfig config;
    config.polygon = polygonFromRect(cv::Rect(0, 0, 100, 100));
    config.classIds = std::move(classIds);
    return RoiIntrusionRule{std::move(config)};
}

RuleContext ctx()
{
    RuleContext context;
    context.frameId = 3;
    context.sourceId = "cam:0";
    return context;
}

} // namespace

class RoiIntrusionRuleTest : public QObject
{
    Q_OBJECT

private slots:
    void outsideEmitsNothing();
    void enterEmitsOnce();
    void stayEmitsNothing();
    void exitEmitsNothing();
    void reenterEmitsAgain();
    void invalidPolygonEmitsNothing();
    void classFilterSkipsOtherIds();
    void disappearThenReappearIsNewEnter();
    void resetAllowsReenterWhileStillInside();
};

void RoiIntrusionRuleTest::outsideEmitsNothing()
{
    auto rule = makeRule();
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(150, 50))}, ctx()).empty());
}

void RoiIntrusionRuleTest::enterEmitsOnce()
{
    auto rule = makeRule();
    QCOMPARE(rule.id(), std::string("roi"));
    QCOMPARE(rule.name(), std::string("roi-intrusion"));
    QCOMPARE(rule.eventType(), EventType::RoiIntrusion);

    const Track track = makeTrack(17, boxWithFoot(50, 50));
    rule.evaluate({makeTrack(17, boxWithFoot(150, 50))}, ctx());
    const auto events = rule.evaluate({track}, ctx());

    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events[0].eventId, std::uint64_t{0});
    QCOMPARE(events[0].type, EventType::RoiIntrusion);
    QCOMPARE(events[0].ruleId, std::string("roi"));
    QCOMPARE(events[0].sourceId, std::string("cam:0"));
    QCOMPARE(events[0].trackId, std::uint64_t{17});
    QCOMPARE(events[0].classId, 0);
    QCOMPARE(events[0].label, std::string("person"));
    QCOMPARE(events[0].confidence, 0.9F);
    QCOMPARE(events[0].box, track.box);
    QCOMPARE(events[0].frameId, std::int64_t{3});
    QCOMPARE(events[0].message, std::string("intrusion"));
}

void RoiIntrusionRuleTest::stayEmitsNothing()
{
    auto rule = makeRule();
    const Track inside = makeTrack(1, boxWithFoot(50, 50));
    rule.evaluate({inside}, ctx());
    QVERIFY(rule.evaluate({inside}, ctx()).empty());
    QVERIFY(rule.evaluate({inside}, ctx()).empty());
}

void RoiIntrusionRuleTest::exitEmitsNothing()
{
    auto rule = makeRule();
    rule.evaluate({makeTrack(1, boxWithFoot(50, 50))}, ctx());
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(150, 50))}, ctx()).empty());
}

void RoiIntrusionRuleTest::reenterEmitsAgain()
{
    auto rule = makeRule();
    rule.evaluate({makeTrack(1, boxWithFoot(50, 50))}, ctx());
    rule.evaluate({makeTrack(1, boxWithFoot(150, 50))}, ctx());
    QCOMPARE(rule.evaluate({makeTrack(1, boxWithFoot(50, 50))}, ctx()).size(), std::size_t{1});
}

void RoiIntrusionRuleTest::invalidPolygonEmitsNothing()
{
    RoiConfig config;
    config.polygon = {{0.0F, 0.0F}, {10.0F, 0.0F}};
    RoiIntrusionRule rule{config};
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(5, 0))}, ctx()).empty());
}

void RoiIntrusionRuleTest::classFilterSkipsOtherIds()
{
    auto rule = makeRule({0});
    QVERIFY(rule.evaluate({makeTrack(1, boxWithFoot(50, 50), 1)}, ctx()).empty());
}

void RoiIntrusionRuleTest::disappearThenReappearIsNewEnter()
{
    auto rule = makeRule();
    QCOMPARE(rule.evaluate({makeTrack(1, boxWithFoot(50, 50))}, ctx()).size(), std::size_t{1});
    QVERIFY(rule.evaluate({}, ctx()).empty());
    QCOMPARE(rule.evaluate({makeTrack(1, boxWithFoot(50, 50))}, ctx()).size(), std::size_t{1});
}

void RoiIntrusionRuleTest::resetAllowsReenterWhileStillInside()
{
    auto rule = makeRule();
    const Track inside = makeTrack(1, boxWithFoot(50, 50));
    rule.evaluate({inside}, ctx());
    rule.reset();
    QCOMPARE(rule.evaluate({inside}, ctx()).size(), std::size_t{1});
}

QTEST_APPLESS_MAIN(RoiIntrusionRuleTest)

#include "RoiIntrusionRuleTest.moc"
