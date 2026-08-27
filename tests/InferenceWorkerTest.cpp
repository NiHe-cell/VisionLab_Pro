#include <QtTest/QtTest>

#include <atomic>
#include <chrono>
#include <thread>

#include "core/BoundedQueue.h"
#include "core/FramePacket.h"
#include "core/LatestResult.h"
#include "core/PresentedFrame.h"
#include "core/VisionTypes.h"
#include "fakes/FakeDetector.h"
#include "fakes/FakeTracker.h"
#include "fakes/SlowDetector.h"
#include "pipeline/InferenceWorker.h"
#include "pipeline/StatsProbe.h"

using visionlab::BoundedQueue;
using visionlab::DetectionMode;
using visionlab::FramePacket;
using visionlab::InferenceWorker;
using visionlab::LatestResult;
using visionlab::OverflowPolicy;
using visionlab::PresentedFrame;
using visionlab::StatsProbe;

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
    void publishesTracksWhenTrackerInjected();
    void emptyDetectionsYieldEmptyTracks();
    void modeChangeResetsTrackerOnInferenceThread();
};

void InferenceWorkerTest::closeOnEmptyQueueExits()
{
    BoundedQueue<FramePacket> in(2, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    FakeDetector detector;
    StatsProbe stats;
    InferenceWorker worker(in, out, [&] { return &detector; }, stats);

    in.close();
    worker.run(std::stop_token{});
    QVERIFY(!out.snapshot().has_value());
}

void InferenceWorkerTest::publishesDetectionsAndConvertsToRgb()
{
    BoundedQueue<FramePacket> in(4, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    FakeDetector detector;
    StatsProbe stats;
    InferenceWorker worker(in, out, [&] { return &detector; }, stats);

    QVERIFY(in.push(makeBgrPacket(7)));
    in.close();
    worker.run(std::stop_token{});

    const std::optional<PresentedFrame> view = out.snapshot();
    QVERIFY(view.has_value());
    QCOMPARE(view->frameId, std::int64_t(7));
    QCOMPARE(view->sourceId, std::string("fake:0"));
    QCOMPARE(view->detections.size(), std::size_t(1));
    QCOMPARE(view->detections.front().label, std::string("fake-object"));
    QVERIFY(view->tracks.empty());
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
    StatsProbe stats;
    InferenceWorker worker(in, out, [&] { return &detector; }, stats);

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
    StatsProbe stats;
    InferenceWorker worker(in, out, [&] { return &detector; }, stats);

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
    StatsProbe stats;
    InferenceWorker worker(in, out, [&] { return &detector; }, stats);

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

void InferenceWorkerTest::publishesTracksWhenTrackerInjected()
{
    BoundedQueue<FramePacket> in(4, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    FakeDetector detector;
    FakeTracker tracker;
    StatsProbe stats;
    InferenceWorker worker(in, out, [&] { return &detector; }, stats, {}, {}, &tracker);

    QVERIFY(in.push(makeBgrPacket(7)));
    in.close();
    worker.run(std::stop_token{});

    const std::optional<PresentedFrame> view = out.snapshot();
    QVERIFY(view.has_value());
    QCOMPARE(view->detections.size(), std::size_t(1));
    QCOMPARE(view->tracks.size(), view->detections.size());
    QCOMPARE(view->tracks.front().label, view->detections.front().label);
    QCOMPARE(view->tracks.front().trackId, std::uint64_t{1});
    QCOMPARE(view->tracks.front().box, view->detections.front().box);
    QCOMPARE(view->rgb.cols, 20);
    QCOMPARE(view->rgb.rows, 16);
    QCOMPARE(view->rgb.type(), CV_8UC3);
    QVERIFY(view->rgb.isContinuous());
    QVERIFY(cv::countNonZero(view->rgb.reshape(1)) > 0);
}

void InferenceWorkerTest::emptyDetectionsYieldEmptyTracks()
{
    BoundedQueue<FramePacket> in(2, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    SlowDetector detector{std::chrono::milliseconds(0)};
    FakeTracker tracker;
    StatsProbe stats;
    InferenceWorker worker(in, out, [&] { return &detector; }, stats, {}, {}, &tracker);

    QVERIFY(in.push(makeBgrPacket(1)));
    in.close();
    worker.run(std::stop_token{});

    const std::optional<PresentedFrame> view = out.snapshot();
    QVERIFY(view.has_value());
    QVERIFY(view->detections.empty());
    QVERIFY(view->tracks.empty());
}

void InferenceWorkerTest::modeChangeResetsTrackerOnInferenceThread()
{
    BoundedQueue<FramePacket> in(4, OverflowPolicy::DropOldest);
    LatestResult<PresentedFrame> out;
    FakeDetector detector;
    FakeTracker tracker;
    StatsProbe stats;
    int modeCalls = 0;
    InferenceWorker worker(
        in, out, [&] { return &detector; }, stats, {}, {}, &tracker,
        [&] {
            ++modeCalls;
            return modeCalls == 1 ? DetectionMode::Face : DetectionMode::Object;
        });

    QVERIFY(in.push(makeBgrPacket(1)));
    QVERIFY(in.push(makeBgrPacket(2)));
    in.close();
    worker.run(std::stop_token{});

    QCOMPARE(tracker.resetCount(), 1);
    const std::optional<PresentedFrame> view = out.snapshot();
    QVERIFY(view.has_value());
    QCOMPARE(view->tracks.front().trackId, std::uint64_t{1});
}

QTEST_APPLESS_MAIN(InferenceWorkerTest)

#include "InferenceWorkerTest.moc"
