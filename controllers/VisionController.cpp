#include "VisionController.h"

#include <QImage>
#include <QTimer>

#include "core/VisionTypes.h"

VisionController::VisionController(CameraManager* cam, QObject* parent)
    : QObject(parent)
    , m_camera(cam)
    , m_detectionModel(new DetectionModel(this))
    , m_trackModel(new TrackModel(this))
    , m_eventModel(new EventModel(this))
    , m_eventFilterModel(new EventTypeFilterModel(this))
    , m_performanceModel(new PerformanceModel(this))
    , m_pluginModel(new PluginModel(this))
    , m_ruleModel(new RuleModel(this))
{
    m_eventFilterModel->setSourceModel(m_eventModel);
    if (m_camera)
    {
        m_pluginModel->setPlugins(m_camera->pluginMetadata(), m_camera->pluginLoadErrors());
        connect(m_camera, &CameraManager::frameChanged, this, &VisionController::onFrameChanged);
    }

    auto* timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, this, &VisionController::onStatsTick);
    timer->start();
}

QString VisionController::mode() const
{
    return m_mode;
}

void VisionController::setMode(QString newMode)
{
    if (m_mode == newMode)
        return;
    m_mode = newMode;
    if (m_camera)
        m_camera->setMode(visionlab::detectionModeFromLabel(newMode.toStdString()));
    emit modeChanged();
}

void VisionController::startCamera()
{
    if (!m_camera)
        return;
    m_camera->applyRuleSpecs(m_ruleModel->specs());
    m_eventModel->beginSession();
    if (m_camera->start())
        setRunning(true);
}

void VisionController::stopCamera()
{
    setRunning(false);
    if (m_camera)
        m_camera->stop();
    m_detectionModel->setDetections({});
    m_trackModel->setTracks({});
}

bool VisionController::running() const
{
    return m_running;
}

void VisionController::setRunning(bool newRunning)
{
    if (m_running == newRunning)
        return;
    m_running = newRunning;
    emit runningChanged();
}

int VisionController::currentPage() const
{
    return m_currentPage;
}

void VisionController::setCurrentPage(int page)
{
    if (page < 0 || page > 3 || page == m_currentPage)
        return;
    m_currentPage = page;
    emit currentPageChanged();
}

void VisionController::onFrameChanged()
{
    if (!m_camera)
        return;
    m_detectionModel->setDetections(m_camera->latestDetections());
    m_trackModel->setTracks(m_camera->latestTracks());
    m_eventModel->ingest(m_camera->recentEvents());

    const QImage frame = m_camera->frame();
    if (frame.isNull())
        return;
    if (m_frameWidth == frame.width() && m_frameHeight == frame.height())
        return;
    m_frameWidth = frame.width();
    m_frameHeight = frame.height();
    emit frameSizeChanged();
}

void VisionController::onStatsTick()
{
    if (!m_camera)
        return;
    m_performanceModel->update(m_camera->statsSnapshot());
}
