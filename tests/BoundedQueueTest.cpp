#include <QtTest/QtTest>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include "core/BoundedQueue.h"

using visionlab::BoundedQueue;
using visionlab::OverflowPolicy;

namespace {

bool waitFlag(const std::atomic<bool>& flag, int timeoutMs = 1000)
{
    const auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::milliseconds(timeoutMs);
    while (!flag.load())
    {
        if (std::chrono::steady_clock::now() >= deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

struct JoinGuard
{
    std::thread& thread;
    ~JoinGuard()
    {
        if (thread.joinable())
            thread.join();
    }
};

} // namespace

class BoundedQueueTest : public QObject
{
    Q_OBJECT

private slots:
    void capacityMustBeAtLeastOne();
    void pushPopPreservesOrderAndMoves();
    void dropOldestDiscardsFrontAndCounts();
    void dropNewestRejectsIncomingAndCounts();
    void blockProducerWaitsUntilPop();
    void closeWakesBlockedProducer();
    void closeWakesBlockedConsumer();
    void closeDrainsThenPopFails();
    void closeIsIdempotentAndEmptyPopFails();
    void multiProducerConsumerNeverExceedsCapacity();
};

void BoundedQueueTest::capacityMustBeAtLeastOne()
{
    QVERIFY_EXCEPTION_THROWN(
        BoundedQueue<int>(0, OverflowPolicy::DropOldest),
        std::invalid_argument);
}

void BoundedQueueTest::pushPopPreservesOrderAndMoves()
{
    BoundedQueue<std::unique_ptr<int>> q(2, OverflowPolicy::DropOldest);

    QVERIFY(q.push(std::make_unique<int>(1)));
    QVERIFY(q.push(std::make_unique<int>(2)));
    QCOMPARE(q.size(), std::size_t(2));
    QCOMPARE(q.capacity(), std::size_t(2));
    QVERIFY(!q.closed());
    QCOMPARE(q.droppedCount(), std::size_t(0));

    std::unique_ptr<int> a;
    std::unique_ptr<int> b;
    QVERIFY(q.pop(a));
    QVERIFY(q.pop(b));
    QVERIFY(a);
    QVERIFY(b);
    QCOMPARE(*a, 1);
    QCOMPARE(*b, 2);
    QCOMPARE(q.size(), std::size_t(0));
}

void BoundedQueueTest::dropOldestDiscardsFrontAndCounts()
{
    BoundedQueue<int> q(2, OverflowPolicy::DropOldest);

    QVERIFY(q.push(1));
    QVERIFY(q.push(2));
    QVERIFY(q.push(3)); // 丢掉 1，留下 2, 3

    QCOMPARE(q.size(), std::size_t(2));
    QCOMPARE(q.droppedCount(), std::size_t(1));

    int a = 0;
    int b = 0;
    QVERIFY(q.pop(a));
    QVERIFY(q.pop(b));
    QCOMPARE(a, 2);
    QCOMPARE(b, 3);
}

void BoundedQueueTest::dropNewestRejectsIncomingAndCounts()
{
    BoundedQueue<int> q(2, OverflowPolicy::DropNewest);

    QVERIFY(q.push(1));
    QVERIFY(q.push(2));
    QVERIFY(!q.push(3)); // 拒绝 3，队列仍为 1, 2

    QCOMPARE(q.size(), std::size_t(2));
    QCOMPARE(q.droppedCount(), std::size_t(1));

    int a = 0;
    int b = 0;
    QVERIFY(q.pop(a));
    QVERIFY(q.pop(b));
    QCOMPARE(a, 1);
    QCOMPARE(b, 2);
}

void BoundedQueueTest::blockProducerWaitsUntilPop()
{
    BoundedQueue<int> q(1, OverflowPolicy::BlockProducer);
    QVERIFY(q.push(1));

    std::atomic<bool> entered{false};
    std::atomic<bool> returned{false};
    std::atomic<bool> accepted{false};

    std::thread producer([&] {
        entered = true;
        accepted = q.push(2);
        returned = true;
    });
    JoinGuard join{producer};

    QVERIFY(waitFlag(entered));
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    QVERIFY(!returned.load());

    int out = 0;
    QVERIFY(q.pop(out));
    QCOMPARE(out, 1);

    QVERIFY(waitFlag(returned));
    QVERIFY(accepted.load());

    QVERIFY(q.pop(out));
    QCOMPARE(out, 2);
}

void BoundedQueueTest::closeWakesBlockedProducer()
{
    BoundedQueue<int> q(1, OverflowPolicy::BlockProducer);
    QVERIFY(q.push(1));

    std::atomic<bool> entered{false};
    std::atomic<bool> returned{false};
    std::atomic<bool> accepted{true};

    std::thread producer([&] {
        entered = true;
        accepted = q.push(2);
        returned = true;
    });
    JoinGuard join{producer};

    QVERIFY(waitFlag(entered));
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    QVERIFY(!returned.load());

    q.close();
    QVERIFY(waitFlag(returned));
    QVERIFY(!accepted.load());
    QVERIFY(q.closed());
}

void BoundedQueueTest::closeWakesBlockedConsumer()
{
    BoundedQueue<int> q(1, OverflowPolicy::DropOldest);

    std::atomic<bool> entered{false};
    std::atomic<bool> returned{false};
    std::atomic<bool> gotItem{true};

    std::thread consumer([&] {
        entered = true;
        int out = 0;
        gotItem = q.pop(out);
        returned = true;
    });
    JoinGuard join{consumer};

    QVERIFY(waitFlag(entered));
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    QVERIFY(!returned.load());

    q.close();
    QVERIFY(waitFlag(returned));
    QVERIFY(!gotItem.load());
}

void BoundedQueueTest::closeDrainsThenPopFails()
{
    BoundedQueue<int> q(4, OverflowPolicy::DropOldest);
    QVERIFY(q.push(10));
    QVERIFY(q.push(20));
    q.close();
    QVERIFY(q.closed());
    QVERIFY(!q.push(30));

    int a = 0;
    int b = 0;
    int c = 0;
    QVERIFY(q.pop(a));
    QVERIFY(q.pop(b));
    QVERIFY(!q.pop(c));
    QCOMPARE(a, 10);
    QCOMPARE(b, 20);
}

void BoundedQueueTest::closeIsIdempotentAndEmptyPopFails()
{
    BoundedQueue<int> q(1, OverflowPolicy::DropOldest);
    q.close();
    q.close();
    QVERIFY(q.closed());

    int out = 99;
    QVERIFY(!q.pop(out));
    QCOMPARE(out, 99);
    QVERIFY(!q.push(1));
    QCOMPARE(q.size(), std::size_t(0));
}

void BoundedQueueTest::multiProducerConsumerNeverExceedsCapacity()
{
    constexpr std::size_t kCapacity = 4;
    constexpr int kPerProducer = 250;
    BoundedQueue<int> q(kCapacity, OverflowPolicy::BlockProducer);

    std::atomic<bool> overflow{false};
    std::atomic<bool> stopWatch{false};
    std::thread watchdog([&] {
        while (!stopWatch.load())
        {
            if (q.size() > q.capacity())
                overflow = true;
            std::this_thread::yield();
        }
    });

    std::mutex seenMutex;
    std::vector<int> seen;
    seen.reserve(kPerProducer * 2);

    std::thread consumer1([&] {
        int v = 0;
        while (q.pop(v))
        {
            std::lock_guard lock(seenMutex);
            seen.push_back(v);
        }
    });
    std::thread consumer2([&] {
        int v = 0;
        while (q.pop(v))
        {
            std::lock_guard lock(seenMutex);
            seen.push_back(v);
        }
    });

    std::atomic<bool> pushFailed{false};
    std::thread producer1([&] {
        for (int i = 0; i < kPerProducer; ++i)
        {
            if (!q.push(i))
                pushFailed = true;
        }
    });
    std::thread producer2([&] {
        for (int i = kPerProducer; i < kPerProducer * 2; ++i)
        {
            if (!q.push(i))
                pushFailed = true;
        }
    });

    producer1.join();
    producer2.join();
    q.close();
    consumer1.join();
    consumer2.join();
    stopWatch = true;
    watchdog.join();

    QVERIFY(!overflow.load());
    QVERIFY(!pushFailed.load());
    QCOMPARE(static_cast<int>(seen.size()), kPerProducer * 2);

    std::sort(seen.begin(), seen.end());
    for (int i = 0; i < kPerProducer * 2; ++i)
        QCOMPARE(seen[static_cast<std::size_t>(i)], i);
}

QTEST_APPLESS_MAIN(BoundedQueueTest)

#include "BoundedQueueTest.moc"
