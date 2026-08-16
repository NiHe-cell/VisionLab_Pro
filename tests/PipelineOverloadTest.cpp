#include <QtTest/QtTest>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <thread>

#include "core/PipelineStats.h"
#include "core/VisionTypes.h"
#include "fakes/FakeVideoSource.h"
#include "fakes/SlowDetector.h"
#include "pipeline/VisionPipeline.h"

using visionlab::DetectionMode;
using visionlab::IDetector;
using visionlab::PipelineStats;
using visionlab::VisionPipeline;

namespace {

constexpr std::size_t kQueueCapacity = 2;
constexpr auto kDetectDelay = std::chrono::milliseconds(50);

bool waitUntil(const std::function<bool()>& pred, int timeoutMs = 3000)
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

} // namespace

class PipelineOverloadTest : public QObject
{
    Q_OBJECT

private slots:
    void dropOldestBoundsQueueAndDropsUnderSlowDetect();
};

void PipelineOverloadTest::dropOldestBoundsQueueAndDropsUnderSlowDetect()
{
    auto source = std::make_unique<FakeVideoSource>(32, "fake:overload", true, true);
    auto slow = std::make_unique<SlowDetector>(kDetectDelay);
    std::map<DetectionMode, std::unique_ptr<IDetector>> detectors;
    detectors.emplace(DetectionMode::Object, std::move(slow));

    VisionPipeline pipeline(std::move(source), std::move(detectors), kQueueCapacity);
    pipeline.setMode(DetectionMode::Object);

    const auto started = std::chrono::steady_clock::now();
    QVERIFY(pipeline.start());

    std::size_t peakDepth = 0;
    QVERIFY(waitUntil([&] {
        const auto snap = pipeline.stats();
        peakDepth = std::max(peakDepth, snap.captureQueueDepth);
        return snap.droppedFrames > 0 && snap.processedFrames >= 4;
    }));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    peakDepth = std::max(peakDepth, pipeline.stats().captureQueueDepth);

    const auto elapsedMs = std::chrono::duration<double, std::milli>(
                               std::chrono::steady_clock::now() - started)
                               .count();
    const PipelineStats stats = pipeline.stats();
    pipeline.stop();
    QVERIFY(!pipeline.isRunning());

    QWARN(qPrintable(
        QString("P2-T07 overload measured: captured=%1 processed=%2 dropped=%3 "
                "qpeak=%4 p50=%5 p95=%6 e2e=%7 elapsedMs=%8")
            .arg(static_cast<qulonglong>(stats.capturedFrames))
            .arg(static_cast<qulonglong>(stats.processedFrames))
            .arg(static_cast<qulonglong>(stats.droppedFrames))
            .arg(static_cast<qulonglong>(peakDepth))
            .arg(stats.p50InferenceLatencyMs, 0, 'f', 1)
            .arg(stats.p95InferenceLatencyMs, 0, 'f', 1)
            .arg(stats.endToEndLatencyMs, 0, 'f', 1)
            .arg(elapsedMs, 0, 'f', 0)));

    // 每次运行以 QWARN 实测为准，禁止把某次数字写死为断言。
    // 本机 Debug 数量级：dropped >> processed，qpeak == capacity，
    // p50 ≈ detect delay，e2e << elapsed。
    QVERIFY(stats.droppedFrames > 0);
    QVERIFY(stats.processedFrames < stats.capturedFrames);
    QVERIFY(stats.processedFrames * 5 < stats.capturedFrames);
    QVERIFY(peakDepth <= kQueueCapacity);
    QVERIFY(stats.captureQueueDepth <= kQueueCapacity);
    QVERIFY(stats.p50InferenceLatencyMs >= 30.0);
    QVERIFY(stats.p50InferenceLatencyMs < 250.0);
    QVERIFY(stats.endToEndLatencyMs > 0.0);
    QVERIFY(stats.endToEndLatencyMs < elapsedMs * 0.5);
    QVERIFY(pipeline.latest().has_value());
}

QTEST_APPLESS_MAIN(PipelineOverloadTest)

#include "PipelineOverloadTest.moc"
