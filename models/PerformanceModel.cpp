#include "models/PerformanceModel.h"

PerformanceModel::PerformanceModel(QObject* parent)
    : QObject(parent)
{
}

void PerformanceModel::update(const visionlab::PipelineStats& stats)
{
    const visionlab::PipelineStats old = m_stats;
    m_stats = stats;
    if (old.capturedFrames != m_stats.capturedFrames)
        emit capturedFramesChanged();
    if (old.processedFrames != m_stats.processedFrames)
        emit processedFramesChanged();
    if (old.droppedFrames != m_stats.droppedFrames)
        emit droppedFramesChanged();
    if (old.captureQueueDepth != m_stats.captureQueueDepth)
        emit captureQueueDepthChanged();
    if (old.captureFps != m_stats.captureFps)
        emit captureFpsChanged();
    if (old.inferenceFps != m_stats.inferenceFps)
        emit inferenceFpsChanged();
    if (old.renderFps != m_stats.renderFps)
        emit renderFpsChanged();
    if (old.avgInferenceLatencyMs != m_stats.avgInferenceLatencyMs)
        emit avgInferenceLatencyMsChanged();
    if (old.p50InferenceLatencyMs != m_stats.p50InferenceLatencyMs)
        emit p50InferenceLatencyMsChanged();
    if (old.p95InferenceLatencyMs != m_stats.p95InferenceLatencyMs)
        emit p95InferenceLatencyMsChanged();
    if (old.endToEndLatencyMs != m_stats.endToEndLatencyMs)
        emit endToEndLatencyMsChanged();
    if (old.activeTracks != m_stats.activeTracks)
        emit activeTracksChanged();
    if (old.createdTracks != m_stats.createdTracks)
        emit createdTracksChanged();
    if (old.lostTracks != m_stats.lostTracks)
        emit lostTracksChanged();
    if (old.removedTracks != m_stats.removedTracks)
        emit removedTracksChanged();
    if (old.avgTrackingLatencyMs != m_stats.avgTrackingLatencyMs)
        emit avgTrackingLatencyMsChanged();
    if (old.eventsEmitted != m_stats.eventsEmitted)
        emit eventsEmittedChanged();
    if (old.enabledRules != m_stats.enabledRules)
        emit enabledRulesChanged();
    if (old.avgRuleLatencyMs != m_stats.avgRuleLatencyMs)
        emit avgRuleLatencyMsChanged();
}
