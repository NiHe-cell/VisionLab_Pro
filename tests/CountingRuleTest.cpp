#include <QtTest/QtTest>

#include "analytics/CountingRule.h"
#include "core/Track.h"
#include "core/VisionEvent.h"

using visionlab::CountConfig;
using visionlab::CountingRule;
using visionlab::CrossingDirection;
using visionlab::EventType;
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

CountConfig horizontalLine()
{
    CountConfig config;
    config.a = {0.0F, 0.0F};
    config.b = {10.0F, 0.0F};
    return config;
}

// 脚点 (5, 5)，AB 左侧（+）。
cv::Rect plusBox()
{
    return {0, 0, 10, 5};
}

// 脚点 (5, -5)，AB 右侧（−）。
cv::Rect minusBox()
{
    return {0, -15, 10, 10};
}

} // namespace

class CountingRuleTest : public QObject
{
    Q_OBJECT

private slots:
    void forwardCrossingCountsIn();
    void secondForwardWhileInsideIsIgnored();
    void reverseCrossingCountsOut();
    void inOutInIncrementsInTwice();
    void twoTracksOccupyTwo();
    void disappearanceDropsOccupancyWithoutOut();
    void resetZerosCounts();
};

void CountingRuleTest::forwardCrossingCountsIn()
{
    CountingRule rule(horizontalLine());
    QCOMPARE(rule.name(), std::string("counting"));
    QCOMPARE(rule.id(), std::string("count"));
    QCOMPARE(rule.eventType(), EventType::Counting);

    QVERIFY(rule.evaluate({makeTrack(1, plusBox())}, RuleContext{}).empty());
    const auto events = rule.evaluate({makeTrack(1, minusBox())}, RuleContext{});

    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events.front().message, std::string("IN"));
    QCOMPARE(events.front().direction, CrossingDirection::Forward);
    QCOMPARE(events.front().trackId, std::uint64_t{1});
    QCOMPARE(events.front().countIn, std::uint64_t{1});
    QCOMPARE(events.front().countOut, std::uint64_t{0});
    QCOMPARE(events.front().occupancy, std::size_t{1});
    QCOMPARE(rule.countIn(), std::uint64_t{1});
    QCOMPARE(rule.occupancy(), std::size_t{1});
}

void CountingRuleTest::secondForwardWhileInsideIsIgnored()
{
    CountingRule rule(horizontalLine());
    rule.evaluate({makeTrack(1, plusBox())}, RuleContext{});
    rule.evaluate({makeTrack(1, minusBox())}, RuleContext{});

    // (5,-5) → (25,5) 不与 AB 相交，侧向变号但不计穿越；仍在 IN 集。
    QVERIFY(rule.evaluate({makeTrack(1, cv::Rect(20, 0, 10, 5))}, RuleContext{}).empty());
    QCOMPARE(rule.countIn(), std::uint64_t{1});

    // (25,5) → (5,-1) 再次 Forward，已在 IN 集则忽略。
    QVERIFY(rule.evaluate({makeTrack(1, cv::Rect(0, -11, 10, 10))}, RuleContext{}).empty());
    QCOMPARE(rule.countIn(), std::uint64_t{1});
    QCOMPARE(rule.occupancy(), std::size_t{1});
}

void CountingRuleTest::reverseCrossingCountsOut()
{
    CountingRule rule(horizontalLine());
    rule.evaluate({makeTrack(1, plusBox())}, RuleContext{});
    rule.evaluate({makeTrack(1, minusBox())}, RuleContext{});

    const auto events = rule.evaluate({makeTrack(1, plusBox())}, RuleContext{});
    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events.front().message, std::string("OUT"));
    QCOMPARE(events.front().direction, CrossingDirection::Reverse);
    QCOMPARE(events.front().countIn, std::uint64_t{1});
    QCOMPARE(events.front().countOut, std::uint64_t{1});
    QCOMPARE(events.front().occupancy, std::size_t{0});
    QCOMPARE(rule.countOut(), std::uint64_t{1});
    QCOMPARE(rule.occupancy(), std::size_t{0});
}

void CountingRuleTest::inOutInIncrementsInTwice()
{
    CountingRule rule(horizontalLine());
    const auto t = makeTrack(1, plusBox());
    rule.evaluate({t}, RuleContext{});
    rule.evaluate({makeTrack(1, minusBox())}, RuleContext{});
    rule.evaluate({t}, RuleContext{});
    const auto events = rule.evaluate({makeTrack(1, minusBox())}, RuleContext{});

    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events.front().message, std::string("IN"));
    QCOMPARE(rule.countIn(), std::uint64_t{2});
    QCOMPARE(rule.countOut(), std::uint64_t{1});
    QCOMPARE(rule.occupancy(), std::size_t{1});
}

void CountingRuleTest::twoTracksOccupyTwo()
{
    CountingRule rule(horizontalLine());
    rule.evaluate({makeTrack(1, plusBox())}, RuleContext{});
    rule.evaluate({makeTrack(1, minusBox())}, RuleContext{});
    rule.evaluate({makeTrack(1, minusBox()), makeTrack(2, plusBox())}, RuleContext{});
    const auto events = rule.evaluate(
        {makeTrack(1, minusBox()), makeTrack(2, minusBox())}, RuleContext{});

    QCOMPARE(events.size(), std::size_t{1});
    QCOMPARE(events.front().trackId, std::uint64_t{2});
    QCOMPARE(rule.countIn(), std::uint64_t{2});
    QCOMPARE(rule.occupancy(), std::size_t{2});
}

void CountingRuleTest::disappearanceDropsOccupancyWithoutOut()
{
    CountingRule rule(horizontalLine());
    rule.evaluate({makeTrack(1, plusBox())}, RuleContext{});
    rule.evaluate({makeTrack(1, minusBox())}, RuleContext{});

    const auto events = rule.evaluate({}, RuleContext{});
    QVERIFY(events.empty());
    QCOMPARE(rule.occupancy(), std::size_t{0});
    QCOMPARE(rule.countOut(), std::uint64_t{0});
    QCOMPARE(rule.countIn(), std::uint64_t{1});
}

void CountingRuleTest::resetZerosCounts()
{
    CountingRule rule(horizontalLine());
    rule.evaluate({makeTrack(1, plusBox())}, RuleContext{});
    rule.evaluate({makeTrack(1, minusBox())}, RuleContext{});
    rule.reset();

    QCOMPARE(rule.countIn(), std::uint64_t{0});
    QCOMPARE(rule.countOut(), std::uint64_t{0});
    QCOMPARE(rule.occupancy(), std::size_t{0});
    QVERIFY(rule.evaluate({makeTrack(1, plusBox())}, RuleContext{}).empty());
}

QTEST_APPLESS_MAIN(CountingRuleTest)

#include "CountingRuleTest.moc"
