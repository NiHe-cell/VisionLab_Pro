#include <QtTest/QtTest>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

#include "core/BoundedQueue.h"
#include "core/FramePacket.h"
#include "fakes/FakeVideoSource.h"
#include "pipeline/CaptureWorker.h"

using visionlab::BoundedQueue;
using visionlab::CaptureWorker;
using visionlab::FramePacket;
using visionlab::OverflowPolicy;

namespace {

bool waitAtLeast(const std::atomic<std::uint64_t>& value,
                 std::uint64_t expected,
                 int timeoutMs = 2000)
{
    const auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::milliseconds(timeoutMs);
    while (value.load() < expected)
    {
        if (std::chrono::steady_clock::now() >= deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

bool waitFlag(const std::atomic<bool>& flag, int timeoutMs = 2000)
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

class CaptureWorkerTest : public QObject
{
    Q_OBJECT

private slots:
    void stampsMonotonicIdsAndSourceId();
    void stopAndCloseUnblocksRun();
    void dropOldestKeepsQueueBounded();
};

void CaptureWorkerTest::stampsMonotonicIdsAndSourceId()
{
    FakeVideoSource source(5, "fake:cam");
    QVERIFY(source.open());

    BoundedQueue<FramePacket> queue(8, OverflowPolicy::DropOldest);
    std::atomic<std::uint64_t> captured{0};
    std::atomic<std::uint64_t> dropped{0};
    CaptureWorker worker(source, queue, captured, dropped);

    std::stop_source stop;
    std::atomic<bool> finished{false};
    std::thread thread([&] {
        worker.run(stop.get_token());
        finished = true;
    });
    JoinGuard join{thread};

    QVERIFY(waitAtLeast(captured, 5));
    stop.request_stop();
    source.close();
    queue.close();
    QVERIFY(waitFlag(finished));

    QCOMPARE(captured.load(), std::uint64_t(5));
    QCOMPARE(dropped.load(), std::uint64_t(0));

    std::vector<FramePacket> frames;
    FramePacket packet;
    while (queue.pop(packet))
        frames.push_back(std::move(packet));

    QCOMPARE(frames.size(), std::size_t(5));
    for (std::size_t i = 0; i < frames.size(); ++i)
    {
        QCOMPARE(frames[i].frameId, static_cast<std::int64_t>(i + 1));
        QCOMPARE(frames[i].sourceId, std::string("fake:cam"));
        QVERIFY(!frames[i].image.empty());
        if (i > 0)
        {
            QVERIFY(frames[i].captureTimestamp >= frames[i - 1].captureTimestamp);
        }
    }
}

void CaptureWorkerTest::stopAndCloseUnblocksRun()
{
    FakeVideoSource source(100000);
    QVERIFY(source.open());

    BoundedQueue<FramePacket> queue(2, OverflowPolicy::DropOldest);
    std::atomic<std::uint64_t> captured{0};
    std::atomic<std::uint64_t> dropped{0};
    CaptureWorker worker(source, queue, captured, dropped);

    std::stop_source stop;
    std::atomic<bool> finished{false};
    std::thread thread([&] {
        worker.run(stop.get_token());
        finished = true;
    });
    JoinGuard join{thread};

    stop.request_stop();
    source.close();
    queue.close();
    QVERIFY(waitFlag(finished));
}

void CaptureWorkerTest::dropOldestKeepsQueueBounded()
{
    FakeVideoSource source(20);
    QVERIFY(source.open());

    BoundedQueue<FramePacket> queue(2, OverflowPolicy::DropOldest);
    std::atomic<std::uint64_t> captured{0};
    std::atomic<std::uint64_t> dropped{0};
    CaptureWorker worker(source, queue, captured, dropped);

    std::stop_source stop;
    std::atomic<bool> finished{false};
    std::thread thread([&] {
        worker.run(stop.get_token());
        finished = true;
    });
    JoinGuard join{thread};

    QVERIFY(waitAtLeast(captured, 20));
    QVERIFY(queue.size() <= queue.capacity());
    QVERIFY(dropped.load() > 0);
    QCOMPARE(captured.load(), std::uint64_t(20));

    stop.request_stop();
    source.close();
    queue.close();
    QVERIFY(waitFlag(finished));
}

QTEST_APPLESS_MAIN(CaptureWorkerTest)

#include "CaptureWorkerTest.moc"
