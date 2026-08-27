#include <QtTest/QtTest>

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <thread>

#include "core/VisionTypes.h"
#include "fakes/FakeDetector.h"
#include "fakes/FakeTracker.h"
#include "fakes/FakeVideoSource.h"
#include "pipeline/VisionPipeline.h"
#include "tracking/ByteTrackTracker.h"
#include "tracking/ITracker.h"

using visionlab::ByteTrackTracker;
using visionlab::DetectionMode;
using visionlab::IDetector;
using visionlab::ITracker;
using visionlab::IVideoSource;
using visionlab::VisionPipeline;

namespace {

bool waitLatest(VisionPipeline& pipeline, int timeoutMs = 2000)
{
    const auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::milliseconds(timeoutMs);
    while (!pipeline.latest().has_value())
    {
        if (std::chrono::steady_clock::now() >= deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

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

std::map<DetectionMode, std::unique_ptr<IDetector>> makeDetectors(
    std::unique_ptr<FakeDetector> face = std::make_unique<FakeDetector>("face"),
    std::unique_ptr<FakeDetector> object = std::make_unique<FakeDetector>("object"))
{
    std::map<DetectionMode, std::unique_ptr<IDetector>> detectors;
    detectors.emplace(DetectionMode::Face, std::move(face));
    detectors.emplace(DetectionMode::Object, std::move(object));
    return detectors;
}

} // namespace

class VisionPipelineTest : public QObject
{
    Q_OBJECT

private slots:
    void startProducesLatestThenStopJoins();
    void repeatedStartStop();
    void openFailureDoesNotStart();
    void setModeSwitchesDetector();
    void fakeTrackerFillsLatestTracks();
    void setModeResetsTracker();
    void byteTrackConfirmsStableId();
    void byteTrackRestartAllocatesIdFromOne();
    void byteTrackModeChangeRestartsIds();
};

void VisionPipelineTest::startProducesLatestThenStopJoins()
{
    auto source = std::make_unique<FakeVideoSource>(200, "fake:pipe");
    VisionPipeline pipeline(std::move(source), makeDetectors());
    pipeline.setMode(DetectionMode::Face);

    QVERIFY(pipeline.start());
    QVERIFY(pipeline.isRunning());
    QVERIFY(waitLatest(pipeline));
    QVERIFY(pipeline.stats().capturedFrames > 0);
    QVERIFY(pipeline.latest()->frameId >= 1);

    pipeline.stop();
    QVERIFY(!pipeline.isRunning());
}

void VisionPipelineTest::repeatedStartStop()
{
    auto source = std::make_unique<FakeVideoSource>(500);
    VisionPipeline pipeline(std::move(source), makeDetectors());

    for (int i = 0; i < 3; ++i)
    {
        QVERIFY(pipeline.start());
        QVERIFY(waitLatest(pipeline));
        pipeline.stop();
        QVERIFY(!pipeline.isRunning());
    }
}

void VisionPipelineTest::openFailureDoesNotStart()
{
    auto source = std::make_unique<FakeVideoSource>(10, "fake:0", false);
    VisionPipeline pipeline(std::move(source), makeDetectors());

    QVERIFY(!pipeline.start());
    QVERIFY(!pipeline.isRunning());
    QVERIFY(!pipeline.latest().has_value());
}

void VisionPipelineTest::setModeSwitchesDetector()
{
    auto face = std::make_unique<FakeDetector>("face");
    auto object = std::make_unique<FakeDetector>("object");
    FakeDetector* objectPtr = object.get();

    std::map<DetectionMode, std::unique_ptr<IDetector>> detectors;
    detectors.emplace(DetectionMode::Face, std::move(face));
    detectors.emplace(DetectionMode::Object, std::move(object));

    auto source = std::make_unique<FakeVideoSource>(32, "fake:0", true, true);
    VisionPipeline pipeline(std::move(source), std::move(detectors));
    pipeline.setMode(DetectionMode::Face);
    QVERIFY(pipeline.start());
    QVERIFY(waitLatest(pipeline));

    pipeline.setMode(DetectionMode::Object);
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->detections.empty()
               && frame->detections.front().label == "object";
    }));

    pipeline.stop();
    QVERIFY(objectPtr->callCount() >= 1);
}

void VisionPipelineTest::fakeTrackerFillsLatestTracks()
{
    auto source = std::make_unique<FakeVideoSource>(32, "fake:track", true, true);
    auto tracker = std::make_unique<FakeTracker>();
    VisionPipeline pipeline(std::move(source), makeDetectors(),
                            VisionPipeline::kDefaultQueueCapacity, std::move(tracker));
    pipeline.setMode(DetectionMode::Face);

    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->tracks.empty();
    }));
    QCOMPARE(pipeline.latest()->tracks.front().label, std::string("face"));
    pipeline.stop();
}

void VisionPipelineTest::setModeResetsTracker()
{
    auto tracker = std::make_unique<FakeTracker>();
    FakeTracker* trackerPtr = tracker.get();
    auto source = std::make_unique<FakeVideoSource>(32, "fake:reset", true, true);
    VisionPipeline pipeline(std::move(source), makeDetectors(),
                            VisionPipeline::kDefaultQueueCapacity, std::move(tracker));
    pipeline.setMode(DetectionMode::Face);
    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->tracks.empty();
    }));

    pipeline.setMode(DetectionMode::Object);
    QVERIFY(waitUntil([&] { return trackerPtr->resetCount() >= 1; }));
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->tracks.empty()
               && frame->detections.front().label == "object"
               && frame->tracks.front().trackId == 1;
    }));
    pipeline.stop();
}

void VisionPipelineTest::byteTrackConfirmsStableId()
{
    auto source = std::make_unique<FakeVideoSource>(64, "fake:bt-stable", true, true);
    VisionPipeline pipeline(std::move(source), makeDetectors(),
                            VisionPipeline::kDefaultQueueCapacity,
                            std::make_unique<ByteTrackTracker>());
    pipeline.setMode(DetectionMode::Face);
    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && frame->tracks.size() == 1;
    }));
    const std::uint64_t id = pipeline.latest()->tracks.front().trackId;
    QCOMPARE(id, std::uint64_t{1});
    const std::int64_t seen = pipeline.latest()->frameId;
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && frame->frameId != seen && frame->tracks.size() == 1
               && frame->tracks.front().trackId == id;
    }));
    pipeline.stop();
}

void VisionPipelineTest::byteTrackRestartAllocatesIdFromOne()
{
    auto source = std::make_unique<FakeVideoSource>(64, "fake:bt-restart", true, true);
    VisionPipeline pipeline(std::move(source), makeDetectors(),
                            VisionPipeline::kDefaultQueueCapacity,
                            std::make_unique<ByteTrackTracker>());
    pipeline.setMode(DetectionMode::Face);
    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->tracks.empty();
    }));
    pipeline.stop();

    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->tracks.empty() && frame->tracks.front().trackId == 1;
    }));
    pipeline.stop();
}

void VisionPipelineTest::byteTrackModeChangeRestartsIds()
{
    auto source = std::make_unique<FakeVideoSource>(64, "fake:bt-mode", true, true);
    VisionPipeline pipeline(std::move(source), makeDetectors(),
                            VisionPipeline::kDefaultQueueCapacity,
                            std::make_unique<ByteTrackTracker>());
    pipeline.setMode(DetectionMode::Face);
    QVERIFY(pipeline.start());
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->tracks.empty()
               && frame->detections.front().label == "face";
    }));

    pipeline.setMode(DetectionMode::Object);
    QVERIFY(waitUntil([&] {
        const auto frame = pipeline.latest();
        return frame && !frame->tracks.empty()
               && frame->detections.front().label == "object"
               && frame->tracks.front().trackId == 1;
    }));
    pipeline.stop();
}

QTEST_APPLESS_MAIN(VisionPipelineTest)

#include "VisionPipelineTest.moc"
