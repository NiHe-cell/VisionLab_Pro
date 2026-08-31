#include <QtTest/QtTest>

#include <thread>
#include <vector>

#include "core/VisionEvent.h"
#include "pipeline/EventLog.h"

using visionlab::EventLog;
using visionlab::VisionEvent;

namespace {

VisionEvent makeEvent(std::uint64_t eventId)
{
    VisionEvent event;
    event.eventId = eventId;
    return event;
}

} // namespace

class EventLogTest : public QObject
{
    Q_OBJECT

private slots:
    void pushKeepsInsertionOrder();
    void dropOldestBeyondCapacity();
    void resetClears();
    void concurrentPushAndSnapshot();
};

void EventLogTest::pushKeepsInsertionOrder()
{
    EventLog log(256);
    log.push({makeEvent(1), makeEvent(2), makeEvent(3)});
    QCOMPARE(log.size(), std::size_t{3});
    const auto snap = log.snapshot();
    QCOMPARE(snap.size(), std::size_t{3});
    QCOMPARE(snap.front().eventId, std::uint64_t{1});
    QCOMPARE(snap.back().eventId, std::uint64_t{3});
}

void EventLogTest::dropOldestBeyondCapacity()
{
    EventLog log(256);
    std::vector<VisionEvent> batch;
    batch.reserve(300);
    for (std::uint64_t i = 1; i <= 300; ++i)
        batch.push_back(makeEvent(i));
    log.push(batch);

    QCOMPARE(log.size(), std::size_t{256});
    const auto snap = log.snapshot();
    QCOMPARE(snap.front().eventId, std::uint64_t{45});
    QCOMPARE(snap.back().eventId, std::uint64_t{300});
}

void EventLogTest::resetClears()
{
    EventLog log;
    log.push({makeEvent(1)});
    log.reset();
    QCOMPARE(log.size(), std::size_t{0});
    QVERIFY(log.snapshot().empty());
}

void EventLogTest::concurrentPushAndSnapshot()
{
    EventLog log(256);
    std::thread producer([&] {
        for (int i = 0; i < 1000; ++i)
            log.push({makeEvent(static_cast<std::uint64_t>(i + 1))});
    });
    std::thread reader([&] {
        for (int i = 0; i < 1000; ++i)
            (void)log.snapshot();
    });
    producer.join();
    reader.join();
    QVERIFY(log.size() <= 256);
    QVERIFY(log.size() > 0);
}

QTEST_APPLESS_MAIN(EventLogTest)

#include "EventLogTest.moc"
