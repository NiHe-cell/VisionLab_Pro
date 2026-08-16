#include <QtTest/QtTest>

#include <atomic>
#include <chrono>
#include <thread>

#include "core/BoundedQueue.h"
#include "core/FramePacket.h"
#include "core/LatestResult.h"
#include "core/PresentedFrame.h"
#include "fakes/FakeDetector.h"
#include "fakes/SlowDetector.h"
#include "pipeline/InferenceWorker.h"

using visionlab::BoundedQueue;
using visionlab::FramePacket;
using visionlab::InferenceWorker;
using visionlab::LatestResult;
using visionlab::OverflowPolicy;
using visionlab::PresentedFrame;

namespace {

FramePacket makeBgrPacket(std::int64_t frameId, const cv::Scalar& bgr = cv::Scalar(255, 0, 0))
{
    FramePacket packet;
    packet.frameId = frameId;
    packet.sourceId = "fake:0";
    packet.captureTimestamp = std::chrono::steady_clock::now();
    packet.image = cv::Mat(16, 20, CV_8UC3, bgr);
    return packet;
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

class InferenceWorkerTest : public QObject
{
    Q_OBJECT

private slots:
    void closeOnEmptyQueueExits();
    void publishesDetectionsAndConvertsToRgb();
    void notReadyStillPublishesEmptyDetections();
    void emptyDetectionsKeepFrameSize();
    void slowDetectCanStopAfterQueueClose();
};

void InferenceWorkerTest::closeOnEmptyQueueExits()
{
    BoundedQueue<FramePacket> in(2, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    FakeDetector detector;
    InferenceWorker worker(in, out, [&] { return &detector; });

    in.close();
    worker.run(std::stop_token{});
    QVERIFY(!out.snapshot().has_value());
}

void InferenceWorkerTest::publishesDetectionsAndConvertsToRgb()
{
    BoundedQueue<FramePacket> in(4, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    FakeDetector detector;
    InferenceWorker worker(in, out, [&] { return &detector; });

    QVERIFY(in.push(makeBgrPacket(7)));
    in.close();
    worker.run(std::stop_token{});

    const std::optional<PresentedFrame> view = out.snapshot();
    QVERIFY(view.has_value());
    QCOMPARE(view->frameId, std::int64_t(7));
    QCOMPARE(view->sourceId, std::string("fake:0"));
    QCOMPARE(view->detections.size(), std::size_t(1));
    QCOMPARE(view->detections.front().label, std::string("fake-object"));
    QVERIFY(view->inferenceLatencyMs >= 0.0);

    QCOMPARE(view->rgb.cols, 20);
    QCOMPARE(view->rgb.rows, 16);
    QCOMPARE(view->rgb.type(), CV_8UC3);
    QVERIFY(view->rgb.isContinuous());
    // 输入 BGR 蓝 (255,0,0) → RGB (0,0,255)。(0,0) 不在检测框上。
    QCOMPARE(view->rgb.at<cv::Vec3b>(0, 0), cv::Vec3b(0, 0, 255));
}

void InferenceWorkerTest::notReadyStillPublishesEmptyDetections()
{
    BoundedQueue<FramePacket> in(2, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    FakeDetector detector;
    detector.setReady(false);
    InferenceWorker worker(in, out, [&] { return &detector; });

    QVERIFY(in.push(makeBgrPacket(3)));
    in.close();
    worker.run(std::stop_token{});

    const std::optional<PresentedFrame> view = out.snapshot();
    QVERIFY(view.has_value());
    QVERIFY(view->detections.empty());
    QVERIFY(!view->rgb.empty());
    QCOMPARE(view->rgb.at<cv::Vec3b>(0, 0), cv::Vec3b(0, 0, 255));
}

void InferenceWorkerTest::emptyDetectionsKeepFrameSize()
{
    BoundedQueue<FramePacket> in(2, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    SlowDetector detector{std::chrono::milliseconds(0)};
    InferenceWorker worker(in, out, [&] { return &detector; });

    QVERIFY(in.push(makeBgrPacket(1)));
    in.close();
    worker.run(std::stop_token{});

    const std::optional<PresentedFrame> view = out.snapshot();
    QVERIFY(view.has_value());
    QVERIFY(view->detections.empty());
    QCOMPARE(view->rgb.cols, 20);
    QCOMPARE(view->rgb.rows, 16);
}

void InferenceWorkerTest::slowDetectCanStopAfterQueueClose()
{
    BoundedQueue<FramePacket> in(2, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    SlowDetector detector{std::chrono::milliseconds(40)};
    InferenceWorker worker(in, out, [&] { return &detector; });

    for (int i = 0; i < 8; ++i)
        QVERIFY(in.push(makeBgrPacket(i + 1)));

    std::atomic<bool> finished{false};
    std::thread thread([&] {
        worker.run(std::stop_token{});
        finished = true;
    });
    JoinGuard join{thread};

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    in.close();
    QVERIFY(waitFlag(finished, 3000));
    QVERIFY(detector.callCount() >= 1);
    QVERIFY(out.snapshot().has_value());
}

QTEST_APPLESS_MAIN(InferenceWorkerTest)

#include "InferenceWorkerTest.moc"
