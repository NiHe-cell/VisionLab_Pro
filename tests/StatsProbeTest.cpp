#include <QtTest/QtTest>

#include <thread>

#include "pipeline/StatsProbe.h"

using visionlab::PipelineStats;
using visionlab::StatsProbe;

class StatsProbeTest : public QObject
{
    Q_OBJECT

private slots:
    void emptySnapshotIsZero();
    void accumulatesCaptureDropAndInference();
    void resetZerosCounters();
    void concurrentRecordAndSnapshot();
};

void StatsProbeTest::emptySnapshotIsZero()
{
    const StatsProbe probe;
    const PipelineStats stats = probe.snapshot();
    QCOMPARE(stats.capturedFrames, std::uint64_t(0));
    QCOMPARE(stats.processedFrames, std::uint64_t(0));
    QCOMPARE(stats.droppedFrames, std::uint64_t(0));
    QCOMPARE(stats.captureQueueDepth, std::size_t(0));
    QCOMPARE(stats.avgInferenceLatencyMs, 0.0);
    QCOMPARE(stats.p50InferenceLatencyMs, 0.0);
    QCOMPARE(stats.p95InferenceLatencyMs, 0.0);
    QCOMPARE(stats.endToEndLatencyMs, 0.0);
}

void StatsProbeTest::accumulatesCaptureDropAndInference()
{
    StatsProbe probe;
    for (int i = 0; i < 5; ++i)
        probe.onCaptured();
    probe.onDropped(2);
    probe.setQueueDepth(3);
    probe.onInferred(10.0, 40.0);
    probe.onInferred(20.0, 50.0);
    probe.onInferred(30.0, 60.0);

    const PipelineStats stats = probe.snapshot();
    QCOMPARE(stats.capturedFrames, std::uint64_t(5));
    QCOMPARE(stats.droppedFrames, std::uint64_t(2));
    QCOMPARE(stats.processedFrames, std::uint64_t(3));
    QCOMPARE(stats.captureQueueDepth, std::size_t(3));
    QCOMPARE(stats.avgInferenceLatencyMs, 20.0);
    QCOMPARE(stats.p50InferenceLatencyMs, 20.0);
    QCOMPARE(stats.endToEndLatencyMs, 60.0);
    QVERIFY(stats.captureFps > 0.0);
    QVERIFY(stats.inferenceFps > 0.0);
    QCOMPARE(stats.renderFps, stats.inferenceFps);
}

void StatsProbeTest::resetZerosCounters()
{
    StatsProbe probe;
    probe.onCaptured();
    probe.onDropped(4);
    probe.onInferred(15.0, 25.0);
    probe.setQueueDepth(9);
    probe.reset();

    const PipelineStats stats = probe.snapshot();
    QCOMPARE(stats.capturedFrames, std::uint64_t(0));
    QCOMPARE(stats.droppedFrames, std::uint64_t(0));
    QCOMPARE(stats.processedFrames, std::uint64_t(0));
    QCOMPARE(stats.captureQueueDepth, std::size_t(0));
    QCOMPARE(stats.avgInferenceLatencyMs, 0.0);
    QCOMPARE(stats.endToEndLatencyMs, 0.0);
    QCOMPARE(stats.captureFps, 0.0);
    QCOMPARE(stats.inferenceFps, 0.0);
    QCOMPARE(stats.renderFps, 0.0);
}

void StatsProbeTest::concurrentRecordAndSnapshot()
{
    StatsProbe probe;
    std::thread producer([&] {
        for (int i = 0; i < 1000; ++i)
        {
            probe.onCaptured();
            if (i % 3 == 0)
                probe.onDropped(1);
            probe.onInferred(1.0, 2.0);
            probe.setQueueDepth(static_cast<std::size_t>(i % 4));
        }
    });
    std::thread reader([&] {
        for (int i = 0; i < 1000; ++i)
            (void)probe.snapshot();
    });
    producer.join();
    reader.join();

    const PipelineStats stats = probe.snapshot();
    QCOMPARE(stats.capturedFrames, std::uint64_t(1000));
    QCOMPARE(stats.processedFrames, std::uint64_t(1000));
}

QTEST_APPLESS_MAIN(StatsProbeTest)

#include "StatsProbeTest.moc"
