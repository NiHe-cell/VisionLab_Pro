#include <QtTest/QtTest>

#include "core/VisionEvent.h"

using visionlab::CrossingDirection;
using visionlab::EventType;
using visionlab::RuleContext;
using visionlab::RuleEngineStats;
using visionlab::VisionEvent;

class VisionEventTest : public QObject
{
    Q_OBJECT

private slots:
    void visionEventDefaults();
    void ruleContextDefaults();
    void ruleEngineStatsDefaults();
};

void VisionEventTest::visionEventDefaults()
{
    const VisionEvent event;
    QCOMPARE(event.eventId, std::uint64_t{0});
    QCOMPARE(event.type, EventType::RoiIntrusion);
    QVERIFY(event.ruleId.empty());
    QVERIFY(event.sourceId.empty());
    QCOMPARE(event.trackId, std::uint64_t{0});
    QCOMPARE(event.classId, -1);
    QVERIFY(event.label.empty());
    QCOMPARE(event.confidence, 0.0F);
    QVERIFY(event.box.empty());
    QCOMPARE(event.frameId, std::int64_t{0});
    QVERIFY(event.message.empty());
    QVERIFY(event.snapshotRef.empty());
    QCOMPARE(event.direction, CrossingDirection::None);
    QCOMPARE(event.countIn, std::uint64_t{0});
    QCOMPARE(event.countOut, std::uint64_t{0});
    QCOMPARE(event.occupancy, std::size_t{0});
}

void VisionEventTest::ruleContextDefaults()
{
    const RuleContext context;
    QCOMPARE(context.frameId, std::int64_t{0});
    QVERIFY(context.sourceId.empty());
}

void VisionEventTest::ruleEngineStatsDefaults()
{
    const RuleEngineStats stats;
    QCOMPARE(stats.ruleCount, std::size_t{0});
    QCOMPARE(stats.enabledRules, std::size_t{0});
    QCOMPARE(stats.eventsEmitted, std::uint64_t{0});
    QCOMPARE(stats.lastEvaluateLatencyMs, 0.0);
}

QTEST_APPLESS_MAIN(VisionEventTest)

#include "VisionEventTest.moc"
