#include <QtTest/QtTest>

#include "core/PipelineStats.h"
#include "models/PerformanceModel.h"

using visionlab::PipelineStats;

class PerformanceModelTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreZero();
    void updateWritesChangedFieldsOnly();
};

void PerformanceModelTest::defaultsAreZero()
{
    PerformanceModel model;
    QCOMPARE(model.capturedFrames(), quint64{0});
    QCOMPARE(model.processedFrames(), quint64{0});
    QCOMPARE(model.droppedFrames(), quint64{0});
    QCOMPARE(model.captureQueueDepth(), 0);
    QCOMPARE(model.captureFps(), 0.0);
    QCOMPARE(model.inferenceFps(), 0.0);
    QCOMPARE(model.renderFps(), 0.0);
    QCOMPARE(model.avgInferenceLatencyMs(), 0.0);
    QCOMPARE(model.p50InferenceLatencyMs(), 0.0);
    QCOMPARE(model.p95InferenceLatencyMs(), 0.0);
    QCOMPARE(model.endToEndLatencyMs(), 0.0);
    QCOMPARE(model.activeTracks(), 0);
    QCOMPARE(model.createdTracks(), quint64{0});
    QCOMPARE(model.lostTracks(), quint64{0});
    QCOMPARE(model.removedTracks(), quint64{0});
    QCOMPARE(model.avgTrackingLatencyMs(), 0.0);
    QCOMPARE(model.eventsEmitted(), quint64{0});
    QCOMPARE(model.enabledRules(), 0);
    QCOMPARE(model.avgRuleLatencyMs(), 0.0);
}

void PerformanceModelTest::updateWritesChangedFieldsOnly()
{
    PerformanceModel model;
    QSignalSpy fpsSpy(&model, &PerformanceModel::captureFpsChanged);
    QSignalSpy eventsSpy(&model, &PerformanceModel::eventsEmittedChanged);
    QSignalSpy tracksSpy(&model, &PerformanceModel::activeTracksChanged);

    PipelineStats stats;
    stats.captureFps = 12.5;
    stats.eventsEmitted = 4;
    stats.activeTracks = 3;
    stats.avgRuleLatencyMs = 1.25;
    model.update(stats);

    QCOMPARE(model.captureFps(), 12.5);
    QCOMPARE(model.eventsEmitted(), quint64{4});
    QCOMPARE(model.activeTracks(), 3);
    QCOMPARE(model.avgRuleLatencyMs(), 1.25);
    QCOMPARE(fpsSpy.count(), 1);
    QCOMPARE(eventsSpy.count(), 1);
    QCOMPARE(tracksSpy.count(), 1);

    model.update(stats);
    QCOMPARE(fpsSpy.count(), 1);
    QCOMPARE(eventsSpy.count(), 1);
    QCOMPARE(tracksSpy.count(), 1);

    stats.captureFps = 13.0;
    model.update(stats);
    QCOMPARE(fpsSpy.count(), 2);
    QCOMPARE(eventsSpy.count(), 1);
    QCOMPARE(model.captureFps(), 13.0);
}

QTEST_GUILESS_MAIN(PerformanceModelTest)

#include "PerformanceModelTest.moc"
