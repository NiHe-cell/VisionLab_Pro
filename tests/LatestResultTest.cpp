#include <QtTest/QtTest>

#include <atomic>
#include <optional>
#include <thread>
#include <vector>

#include "core/LatestResult.h"

using visionlab::LatestResult;

class LatestResultTest : public QObject
{
    Q_OBJECT

private slots:
    void emptySnapshotIsNullopt();
    void publishThenSnapshotReturnsValue();
    void laterPublishOverwrites();
    void snapshotCopyIsIndependent();
    void clearEmptiesSlot();
    void concurrentPublishAndSnapshot();
};

void LatestResultTest::emptySnapshotIsNullopt()
{
    const LatestResult<int> slot;
    QVERIFY(!slot.snapshot().has_value());
}

void LatestResultTest::publishThenSnapshotReturnsValue()
{
    LatestResult<int> slot;
    slot.publish(42);
    const std::optional<int> view = slot.snapshot();
    QVERIFY(view.has_value());
    QCOMPARE(*view, 42);
}

void LatestResultTest::laterPublishOverwrites()
{
    LatestResult<int> slot;
    slot.publish(1);
    slot.publish(2);
    slot.publish(3);
    const std::optional<int> view = slot.snapshot();
    QVERIFY(view.has_value());
    QCOMPARE(*view, 3);
}

void LatestResultTest::snapshotCopyIsIndependent()
{
    LatestResult<std::vector<int>> slot;
    slot.publish(std::vector<int>{1, 2});

    std::optional<std::vector<int>> copy = slot.snapshot();
    QVERIFY(copy.has_value());

    slot.publish(std::vector<int>{9});
    QCOMPARE(*copy, (std::vector<int>{1, 2}));
    QCOMPARE(*slot.snapshot(), (std::vector<int>{9}));
}

void LatestResultTest::clearEmptiesSlot()
{
    LatestResult<int> slot;
    slot.publish(7);
    slot.clear();
    QVERIFY(!slot.snapshot().has_value());
    slot.clear();
    QVERIFY(!slot.snapshot().has_value());
}

void LatestResultTest::concurrentPublishAndSnapshot()
{
    LatestResult<int> slot;
    constexpr int kLast = 9999;
    std::atomic<bool> producerDone{false};

    std::thread producer([&] {
        for (int i = 0; i <= kLast; ++i)
            slot.publish(i);
        producerDone = true;
    });
    std::thread consumer([&] {
        while (!producerDone.load())
        {
            (void)slot.snapshot();
        }
        (void)slot.snapshot();
    });

    producer.join();
    consumer.join();

    const std::optional<int> view = slot.snapshot();
    QVERIFY(view.has_value());
    QCOMPARE(*view, kLast);
}

QTEST_APPLESS_MAIN(LatestResultTest)

#include "LatestResultTest.moc"
