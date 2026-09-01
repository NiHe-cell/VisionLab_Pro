#include <QtTest/QtTest>

#include <memory>

#include "analytics/RuleEngine.h"
#include "core/Track.h"
#include "fakes/FakeRule.h"

using visionlab::RuleContext;
using visionlab::RuleEngine;
using visionlab::Track;

namespace {

Track oneTrack()
{
    Track track;
    track.trackId = 1;
    track.classId = 0;
    track.label = "person";
    track.confidence = 0.9F;
    track.box = cv::Rect(1, 2, 3, 4);
    return track;
}

} // namespace

class RuleEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyEngineEmitsNothing();
    void assignsMonotonicEventIds();
    void twoRulesEmitInRegistrationOrder();
    void disableSkipsRuleWithoutChangingSize();
    void duplicateIdIsRejected();
    void resetRestartsIdsAndCallsRuleReset();
    void clearDropsRulesAndRestartsIds();
    void ruleIdsFollowRegistrationOrder();
};

void RuleEngineTest::emptyEngineEmitsNothing()
{
    RuleEngine engine;
    QVERIFY(engine.evaluate({oneTrack()}, RuleContext{}).empty());
    QCOMPARE(engine.size(), std::size_t{0});
    QCOMPARE(engine.stats().ruleCount, std::size_t{0});
    QCOMPARE(engine.stats().eventsEmitted, std::uint64_t{0});
    QCOMPARE(engine.stats().lastEvaluateLatencyMs, 0.0);
}

void RuleEngineTest::assignsMonotonicEventIds()
{
    RuleEngine engine;
    QVERIFY(engine.addRule(std::make_unique<FakeRule>()));

    const auto first = engine.evaluate({oneTrack()}, RuleContext{});
    QCOMPARE(first.size(), std::size_t{1});
    QCOMPARE(first.front().eventId, std::uint64_t{1});
    QCOMPARE(engine.stats().eventsEmitted, std::uint64_t{1});

    const auto second = engine.evaluate({oneTrack()}, RuleContext{});
    QCOMPARE(second.front().eventId, std::uint64_t{2});
    QCOMPARE(engine.stats().eventsEmitted, std::uint64_t{2});
}

void RuleEngineTest::twoRulesEmitInRegistrationOrder()
{
    RuleEngine engine;
    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake")));
    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake2")));

    const auto events = engine.evaluate({oneTrack()}, RuleContext{});
    QCOMPARE(events.size(), std::size_t{2});
    QCOMPARE(events[0].eventId, std::uint64_t{1});
    QCOMPARE(events[0].ruleId, std::string("fake"));
    QCOMPARE(events[1].eventId, std::uint64_t{2});
    QCOMPARE(events[1].ruleId, std::string("fake2"));
}

void RuleEngineTest::disableSkipsRuleWithoutChangingSize()
{
    RuleEngine engine;
    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake")));
    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake2")));
    QCOMPARE(engine.size(), std::size_t{2});
    QCOMPARE(engine.stats().enabledRules, std::size_t{2});

    QVERIFY(engine.setEnabled("fake", false));
    QVERIFY(!engine.isEnabled("fake"));
    QCOMPARE(engine.size(), std::size_t{2});
    QCOMPARE(engine.stats().enabledRules, std::size_t{1});

    const auto disabled = engine.evaluate({oneTrack()}, RuleContext{});
    QCOMPARE(disabled.size(), std::size_t{1});
    QCOMPARE(disabled.front().ruleId, std::string("fake2"));

    QVERIFY(engine.setEnabled("fake", true));
    QCOMPARE(engine.evaluate({oneTrack()}, RuleContext{}).size(), std::size_t{2});
}

void RuleEngineTest::duplicateIdIsRejected()
{
    RuleEngine engine;
    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake")));
    QVERIFY(!engine.addRule(std::make_unique<FakeRule>("fake")));
    QCOMPARE(engine.size(), std::size_t{1});
    QVERIFY(!engine.addRule(nullptr));
}

void RuleEngineTest::resetRestartsIdsAndCallsRuleReset()
{
    auto rule = std::make_unique<FakeRule>();
    FakeRule* rulePtr = rule.get();
    RuleEngine engine;
    QVERIFY(engine.addRule(std::move(rule)));
    engine.evaluate({oneTrack()}, RuleContext{});
    engine.reset();

    QCOMPARE(rulePtr->resetCount(), 1);
    QCOMPARE(engine.stats().eventsEmitted, std::uint64_t{0});
    QCOMPARE(engine.stats().lastEvaluateLatencyMs, 0.0);
    const auto events = engine.evaluate({oneTrack()}, RuleContext{});
    QCOMPARE(events.front().eventId, std::uint64_t{1});
}

void RuleEngineTest::clearDropsRulesAndRestartsIds()
{
    RuleEngine engine;
    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake")));
    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake2")));
    engine.evaluate({oneTrack()}, RuleContext{});
    engine.clear();

    QCOMPARE(engine.size(), std::size_t{0});
    QVERIFY(engine.ruleIds().empty());
    QCOMPARE(engine.stats().eventsEmitted, std::uint64_t{0});
    QCOMPARE(engine.stats().lastEvaluateLatencyMs, 0.0);

    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake")));
    const auto events = engine.evaluate({oneTrack()}, RuleContext{});
    QCOMPARE(events.front().eventId, std::uint64_t{1});
}

void RuleEngineTest::ruleIdsFollowRegistrationOrder()
{
    RuleEngine engine;
    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake")));
    QVERIFY(engine.addRule(std::make_unique<FakeRule>("fake2")));
    QCOMPARE(engine.ruleIds(), (std::vector<std::string>{"fake", "fake2"}));
}

QTEST_APPLESS_MAIN(RuleEngineTest)

#include "RuleEngineTest.moc"
