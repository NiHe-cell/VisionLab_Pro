#include <QtTest/QtTest>

#include <chrono>

#include "analytics/LoiteringRule.h"
#include "analytics/RuleGeometry.h"
#include "core/Track.h"
#include "core/VisionEvent.h"

using visionlab::EventType;
using visionlab::LoiterConfig;
using visionlab::LoiteringRule;
using visionlab::RuleContext;
using visionlab::Track;
using visionlab::TrackState;
using visionlab::polygonFromRect;

using namespace std::chrono_literals;

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

LoiteringRule makeRule()
{
    LoiterConfig config;
    config.polygon = polygonFromRect(cv::Rect(0, 0, 100, 100));
    config.loiterSeconds = 5.0;
    return LoiteringRule{std::move(config)};
}

const std::chrono::steady_clock::time_point kOrigin{};

RuleContext at(std::chrono::seconds elapsed)
{
    RuleContext context;
    context.frameId = elapsed.count();
    context.timestamp = kOrigin + elapsed;
    context.sourceId = "cam:0";
    return context;
}

const cv::Rect kInside = boxWithFoot(50, 50);
const cv::Rect kOutside = boxWithFoot(150, 50);

} // namespace

class LoiteringRuleTest : public QObject
{
    Q_OBJECT

private slots:
    void belowThresholdEmitsNothing();
    void thresholdFrameEmitsOnce();
    void stayAfterEmitDoesNotRepeat();
    void leaveBeforeThresholdResetsTimer();
    void reenterCanEmitAgain();
    void disappearClearsTimer();
    void resetClearsTimer();
    void lostInsideContinuesTimer();
    void invalidConfigEmitsNothing();
};

void LoiteringRuleTest::belowThresholdEmitsNothing()
{
    auto rule = makeRule();
    QCOMPARE(rule.id(), std::string("loiter"));
    QCOMPARE(rule.name(), std::string("loitering"));
    QCOMPARE(rule.eventType(), EventType::Loitering);

    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(0s)).empty());
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(4s)).empty());
}

void LoiteringRuleTest::thresholdFrameEmitsOnce()
{
    auto rule = makeRule();
    rule.evaluate({makeTrack(1, kInside)}, at(0s));
    const auto events = rule.evaluate({makeTrack(1, kInside)}, at(5s));
    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events[0].eventId, std::uint64_t{0});
    QCOMPARE(events[0].type, EventType::Loitering);
    QCOMPARE(events[0].trackId, std::uint64_t{1});
    QCOMPARE(events[0].message, std::string("loitering"));
    QCOMPARE(events[0].sourceId, std::string("cam:0"));
}

void LoiteringRuleTest::stayAfterEmitDoesNotRepeat()
{
    auto rule = makeRule();
    rule.evaluate({makeTrack(1, kInside)}, at(0s));
    rule.evaluate({makeTrack(1, kInside)}, at(5s));
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(6s)).empty());
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(8s)).empty());
}

void LoiteringRuleTest::leaveBeforeThresholdResetsTimer()
{
    auto rule = makeRule();
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(0s)).empty());
    QVERIFY(rule.evaluate({makeTrack(1, kOutside)}, at(3s)).empty());
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(3s)).empty());
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(7s)).empty());
    QCOMPARE(rule.evaluate({makeTrack(1, kInside)}, at(8s)).size(), std::size_t{1});
}

void LoiteringRuleTest::reenterCanEmitAgain()
{
    auto rule = makeRule();
    rule.evaluate({makeTrack(1, kInside)}, at(0s));
    QCOMPARE(rule.evaluate({makeTrack(1, kInside)}, at(5s)).size(), std::size_t{1});
    QVERIFY(rule.evaluate({makeTrack(1, kOutside)}, at(6s)).empty());
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(6s)).empty());
    QCOMPARE(rule.evaluate({makeTrack(1, kInside)}, at(11s)).size(), std::size_t{1});
}

void LoiteringRuleTest::disappearClearsTimer()
{
    auto rule = makeRule();
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(0s)).empty());
    QVERIFY(rule.evaluate({}, at(4s)).empty());
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(4s)).empty());
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(8s)).empty());
    QCOMPARE(rule.evaluate({makeTrack(1, kInside)}, at(9s)).size(), std::size_t{1});
}

void LoiteringRuleTest::resetClearsTimer()
{
    auto rule = makeRule();
    rule.evaluate({makeTrack(1, kInside)}, at(0s));
    rule.reset();
    QVERIFY(rule.evaluate({makeTrack(1, kInside)}, at(4s)).empty());
    QCOMPARE(rule.evaluate({makeTrack(1, kInside)}, at(9s)).size(), std::size_t{1});
}

void LoiteringRuleTest::lostInsideContinuesTimer()
{
    auto rule = makeRule();
    rule.evaluate({makeTrack(1, kInside)}, at(0s));
    Track lost = makeTrack(1, kInside);
    lost.state = TrackState::Lost;
    QCOMPARE(rule.evaluate({lost}, at(5s)).size(), std::size_t{1});
}

void LoiteringRuleTest::invalidConfigEmitsNothing()
{
    LoiterConfig badPolygon;
    badPolygon.polygon = {{0.0F, 0.0F}, {1.0F, 0.0F}};
    LoiteringRule ruleA{badPolygon};
    QVERIFY(ruleA.evaluate({makeTrack(1, kInside)}, at(5s)).empty());

    LoiterConfig badSeconds;
    badSeconds.polygon = polygonFromRect(cv::Rect(0, 0, 100, 100));
    badSeconds.loiterSeconds = 0.0;
    LoiteringRule ruleB{badSeconds};
    QVERIFY(ruleB.evaluate({makeTrack(1, kInside)}, at(0s)).empty());
    QVERIFY(ruleB.evaluate({makeTrack(1, kInside)}, at(10s)).empty());
}

QTEST_APPLESS_MAIN(LoiteringRuleTest)

#include "LoiteringRuleTest.moc"
