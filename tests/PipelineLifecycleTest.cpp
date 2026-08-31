#include <QtTest/QtTest>

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <thread>

#include "core/VisionTypes.h"
#include "analytics/RuleEngine.h"
#include "fakes/FakeDetector.h"
#include "fakes/FakeRule.h"
#include "fakes/FakeTracker.h"
#include "fakes/FakeVideoSource.h"
#include "fakes/SlowDetector.h"
#include "fakes/ThrowingDetector.h"
#include "pipeline/VisionPipeline.h"
#include "tracking/ByteTrackTracker.h"

using visionlab::ByteTrackTracker;
using visionlab::DetectionMode;
using visionlab::IDetector;
using visionlab::RuleEngine;
using visionlab::VisionPipeline;

namespace {

constexpr auto kDetectDelay = std::chrono::milliseconds(80);

bool waitUntil(const std::function<bool()>& pred, int timeoutMs = 2000)
{
    const auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::milliseconds(timeoutMs);
    while (!pred())
    {
        if (std::chrono::steady_clock::now() >= deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

std::map<DetectionMode, std::unique_ptr<IDetector>> makeFaceDetector()
{
    std::map<DetectionMode, std::unique_ptr<IDetector>> detectors;
    detectors.emplace(DetectionMode::Face, std::make_unique<FakeDetector>("face"));
    return detectors;
}

} // namespace

class PipelineLifecycleTest : public QObject
{
    Q_OBJECT

private slots:
    void stopJoinsWhileDetectSleeps();
    void startAfterStop();
    void sourceCloseThenStopJoins();
    void detectExceptionDoesNotAbort();
    void startStopStartWithByteTrack();
    void startStopLeavesEventLogAccessible();
};

void PipelineLifecycleTest::stopJoinsWhileDetectSleeps()
{
    // detect 不可中断：stop 最多等到当前这一次 detect 返回后再 join。
    auto source = std::make_unique<FakeVideoSource>(32, "fake:life", true, true);
    auto slow = std::make_unique<SlowDetector>(kDetectDelay);
    SlowDetector* slowPtr = slow.get();
    std::map<DetectionMode, std::unique_ptr<IDetector>> detectors;
    detectors.emplace(DetectionMode::Object, std::move(slow));

    VisionPipeline pipeline(std::move(source), std::move(detectors));
    pipeline.setMode(DetectionMode::Object);
    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] { return slowPtr->callCount() >= 1; }));

    QElapsedTimer joinTimer;
    joinTimer.start();
    pipeline.stop();
    QVERIFY(!pipeline.isRunning());
    QVERIFY(joinTimer.elapsed() < 1500);
}

void PipelineLifecycleTest::startAfterStop()
{
    auto source = std::make_unique<FakeVideoSource>(64, "fake:restart", true, true);
    VisionPipeline pipeline(std::move(source), makeFaceDetector());
    pipeline.setMode(DetectionMode::Face);

    for (int i = 0; i < 3; ++i)
    {
        QVERIFY(pipeline.start());
        QVERIFY(waitUntil([&] { return pipeline.latest().has_value(); }));
        pipeline.stop();
        QVERIFY(!pipeline.isRunning());
    }
}

void PipelineLifecycleTest::sourceCloseThenStopJoins()
{
    auto source = std::make_unique<FakeVideoSource>(100000, "fake:close", true, true);
    FakeVideoSource* sourcePtr = source.get();
    VisionPipeline pipeline(std::move(source), makeFaceDetector());
    pipeline.setMode(DetectionMode::Face);
    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] { return pipeline.latest().has_value(); }));

    sourcePtr->close();

    QElapsedTimer joinTimer;
    joinTimer.start();
    pipeline.stop();
    QVERIFY(!pipeline.isRunning());
    QVERIFY(joinTimer.elapsed() < 1500);
}

void PipelineLifecycleTest::detectExceptionDoesNotAbort()
{
    auto source = std::make_unique<FakeVideoSource>(8, "fake:throw");
    std::map<DetectionMode, std::unique_ptr<IDetector>> detectors;
    detectors.emplace(DetectionMode::Face, std::make_unique<ThrowingDetector>());

    VisionPipeline pipeline(std::move(source), std::move(detectors));
    pipeline.setMode(DetectionMode::Face);
    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] { return pipeline.latest().has_value(); }));
    QVERIFY(pipeline.stats().processedFrames >= 1);

    pipeline.stop();
    QVERIFY(!pipeline.isRunning());
}

void PipelineLifecycleTest::startStopStartWithByteTrack()
{
    auto source = std::make_unique<FakeVideoSource>(64, "fake:life-bt", true, true);
    VisionPipeline pipeline(std::move(source), makeFaceDetector(),
                            VisionPipeline::kDefaultQueueCapacity,
                            std::make_unique<ByteTrackTracker>());
    pipeline.setMode(DetectionMode::Face);

    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->tracks.empty();
    }));
    pipeline.stop();
    QVERIFY(!pipeline.isRunning());

    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->tracks.empty() && frame->tracks.front().trackId == 1;
    }));
    pipeline.stop();
    QVERIFY(!pipeline.isRunning());
}

void PipelineLifecycleTest::startStopLeavesEventLogAccessible()
{
    auto source = std::make_unique<FakeVideoSource>(64, "fake:life-events", true, true);
    auto engine = std::make_unique<RuleEngine>();
    QVERIFY(engine->addRule(std::make_unique<FakeRule>()));
    VisionPipeline pipeline(std::move(source), makeFaceDetector(),
                            VisionPipeline::kDefaultQueueCapacity,
                            std::make_unique<FakeTracker>(),
                            std::move(engine));
    pipeline.setMode(DetectionMode::Face);

    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] { return !pipeline.recentEvents().empty(); }));
    pipeline.stop();
    QVERIFY(!pipeline.isRunning());
    (void)pipeline.recentEvents();
    QVERIFY(pipeline.recentEvents().size() > 0);

    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] {
        const auto events = pipeline.recentEvents();
        return !events.empty() && events.front().eventId == 1;
    }));
    pipeline.stop();
    QVERIFY(!pipeline.isRunning());
}

QTEST_APPLESS_MAIN(PipelineLifecycleTest)

#include "PipelineLifecycleTest.moc"
