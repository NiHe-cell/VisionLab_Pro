#ifndef PERFORMANCEMODEL_H
#define PERFORMANCEMODEL_H

#include <QObject>

#include "core/PipelineStats.h"

// PipelineStats 快照。由编排层定时调用 update；本对象不创建 QTimer。
class PerformanceModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint64 capturedFrames READ capturedFrames NOTIFY capturedFramesChanged)
    Q_PROPERTY(quint64 processedFrames READ processedFrames NOTIFY processedFramesChanged)
    Q_PROPERTY(quint64 droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
    Q_PROPERTY(int captureQueueDepth READ captureQueueDepth NOTIFY captureQueueDepthChanged)
    Q_PROPERTY(double captureFps READ captureFps NOTIFY captureFpsChanged)
    Q_PROPERTY(double inferenceFps READ inferenceFps NOTIFY inferenceFpsChanged)
    Q_PROPERTY(double renderFps READ renderFps NOTIFY renderFpsChanged)
    Q_PROPERTY(double avgInferenceLatencyMs READ avgInferenceLatencyMs NOTIFY avgInferenceLatencyMsChanged)
    Q_PROPERTY(double p50InferenceLatencyMs READ p50InferenceLatencyMs NOTIFY p50InferenceLatencyMsChanged)
    Q_PROPERTY(double p95InferenceLatencyMs READ p95InferenceLatencyMs NOTIFY p95InferenceLatencyMsChanged)
    Q_PROPERTY(double endToEndLatencyMs READ endToEndLatencyMs NOTIFY endToEndLatencyMsChanged)
    Q_PROPERTY(int activeTracks READ activeTracks NOTIFY activeTracksChanged)
    Q_PROPERTY(quint64 createdTracks READ createdTracks NOTIFY createdTracksChanged)
    Q_PROPERTY(quint64 lostTracks READ lostTracks NOTIFY lostTracksChanged)
    Q_PROPERTY(quint64 removedTracks READ removedTracks NOTIFY removedTracksChanged)
    Q_PROPERTY(double avgTrackingLatencyMs READ avgTrackingLatencyMs NOTIFY avgTrackingLatencyMsChanged)
    Q_PROPERTY(quint64 eventsEmitted READ eventsEmitted NOTIFY eventsEmittedChanged)
    Q_PROPERTY(int enabledRules READ enabledRules NOTIFY enabledRulesChanged)
    Q_PROPERTY(double avgRuleLatencyMs READ avgRuleLatencyMs NOTIFY avgRuleLatencyMsChanged)
public:
    explicit PerformanceModel(QObject* parent = nullptr);

    void update(const visionlab::PipelineStats& stats);

    quint64 capturedFrames() const { return m_stats.capturedFrames; }
    quint64 processedFrames() const { return m_stats.processedFrames; }
    quint64 droppedFrames() const { return m_stats.droppedFrames; }
    int captureQueueDepth() const { return static_cast<int>(m_stats.captureQueueDepth); }
    double captureFps() const { return m_stats.captureFps; }
    double inferenceFps() const { return m_stats.inferenceFps; }
    double renderFps() const { return m_stats.renderFps; }
    double avgInferenceLatencyMs() const { return m_stats.avgInferenceLatencyMs; }
    double p50InferenceLatencyMs() const { return m_stats.p50InferenceLatencyMs; }
    double p95InferenceLatencyMs() const { return m_stats.p95InferenceLatencyMs; }
    double endToEndLatencyMs() const { return m_stats.endToEndLatencyMs; }
    int activeTracks() const { return static_cast<int>(m_stats.activeTracks); }
    quint64 createdTracks() const { return m_stats.createdTracks; }
    quint64 lostTracks() const { return m_stats.lostTracks; }
    quint64 removedTracks() const { return m_stats.removedTracks; }
    double avgTrackingLatencyMs() const { return m_stats.avgTrackingLatencyMs; }
    quint64 eventsEmitted() const { return m_stats.eventsEmitted; }
    int enabledRules() const { return static_cast<int>(m_stats.enabledRules); }
    double avgRuleLatencyMs() const { return m_stats.avgRuleLatencyMs; }

signals:
    void capturedFramesChanged();
    void processedFramesChanged();
    void droppedFramesChanged();
    void captureQueueDepthChanged();
    void captureFpsChanged();
    void inferenceFpsChanged();
    void renderFpsChanged();
    void avgInferenceLatencyMsChanged();
    void p50InferenceLatencyMsChanged();
    void p95InferenceLatencyMsChanged();
    void endToEndLatencyMsChanged();
    void activeTracksChanged();
    void createdTracksChanged();
    void lostTracksChanged();
    void removedTracksChanged();
    void avgTrackingLatencyMsChanged();
    void eventsEmittedChanged();
    void enabledRulesChanged();
    void avgRuleLatencyMsChanged();

private:
    visionlab::PipelineStats m_stats;
};

#endif // PERFORMANCEMODEL_H
