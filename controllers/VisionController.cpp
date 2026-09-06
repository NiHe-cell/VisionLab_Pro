#include "VisionController.h"

#include <QImage>
#include <QTimer>

#include "core/VisionTypes.h"
#include "rendering/Letterbox.h"

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

void VisionController::setFrameSize(int width, int height)
{
    if (width == m_frameWidth && height == m_frameHeight)
        return;
    m_frameWidth = width;
    m_frameHeight = height;
    emit frameSizeChanged();
}

QVariantList VisionController::draftPoints() const
{
    QVariantList points;
    points.reserve(static_cast<int>(m_draft.size()));
    for (const cv::Point2f& point : m_draft)
        points.append(QPointF(point.x, point.y));
    return points;
}

QPointF VisionController::itemToFrame(qreal x, qreal y, qreal itemW, qreal itemH) const
{
    const visionlab::Letterbox box = visionlab::computeLetterbox(
        static_cast<float>(itemW), static_cast<float>(itemH), m_frameWidth, m_frameHeight);
    cv::Point2f out;
    if (!visionlab::itemToFrame(box, static_cast<float>(x), static_cast<float>(y),
                                m_frameWidth, m_frameHeight, out))
    {
        return {};
    }
    return QPointF(out.x, out.y);
}

QPointF VisionController::frameToItem(qreal fx, qreal fy, qreal itemW, qreal itemH) const
{
    const visionlab::Letterbox box = visionlab::computeLetterbox(
        static_cast<float>(itemW), static_cast<float>(itemH), m_frameWidth, m_frameHeight);
    cv::Point2f out;
    if (!visionlab::frameToItem(box, static_cast<float>(fx), static_cast<float>(fy), out))
        return {};
    return QPointF(out.x, out.y);
}

bool VisionController::itemPointInVideo(qreal x, qreal y, qreal itemW, qreal itemH) const
{
    const visionlab::Letterbox box = visionlab::computeLetterbox(
        static_cast<float>(itemW), static_cast<float>(itemH), m_frameWidth, m_frameHeight);
    cv::Point2f out;
    return visionlab::itemToFrame(box, static_cast<float>(x), static_cast<float>(y),
                                  m_frameWidth, m_frameHeight, out);
}

void VisionController::beginDraw(DrawTool tool)
{
    if (m_running)
        return;
    if (tool == m_drawTool && tool != None)
        return;
    m_drawTool = tool;
    m_draft.clear();
    emit drawToolChanged();
    emit draftPointsChanged();
}

void VisionController::addDrawPoint(qreal itemX, qreal itemY, qreal itemW, qreal itemH)
{
    if (m_running || m_drawTool == None)
        return;
    if (!itemPointInVideo(itemX, itemY, itemW, itemH))
        return;

    const QPointF frame = itemToFrame(itemX, itemY, itemW, itemH);
    m_draft.emplace_back(static_cast<float>(frame.x()), static_cast<float>(frame.y()));
    emit draftPointsChanged();

    if ((m_drawTool == Line || m_drawTool == Count) && m_draft.size() >= 2)
        finishDraw();
}

void VisionController::finishDraw()
{
    if (m_drawTool == None)
        return;

    const bool polygon = m_drawTool == Roi || m_drawTool == Loiter;
    const bool enough = polygon ? m_draft.size() >= 3 : m_draft.size() >= 2;
    if (enough)
    {
        visionlab::RuleSpec spec;
        spec.kind = kindForTool(m_drawTool);
        if (polygon)
        {
            spec.polygon = m_draft;
        }
        else
        {
            spec.a = m_draft[0];
            spec.b = m_draft[1];
        }
        m_ruleModel->addSpec(std::move(spec));
    }
    clearDraft();
}

void VisionController::cancelDraw()
{
    if (m_drawTool == None && m_draft.empty())
        return;
    clearDraft();
}

void VisionController::commitRulesToEngine()
{
    if (!m_camera)
        return;
    m_camera->applyRuleSpecs(m_ruleModel->specs());
}

visionlab::RuleKind VisionController::kindForTool(DrawTool tool) const
{
    switch (tool)
    {
    case Line:
        return visionlab::RuleKind::LineCrossing;
    case Loiter:
        return visionlab::RuleKind::Loitering;
    case Count:
        return visionlab::RuleKind::Counting;
    case Roi:
    case None:
        break;
    }
    return visionlab::RuleKind::RoiIntrusion;
}

void VisionController::clearDraft()
{
    const bool toolChanged = m_drawTool != None;
    m_drawTool = None;
    m_draft.clear();
    if (toolChanged)
        emit drawToolChanged();
    emit draftPointsChanged();
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
    setFrameSize(frame.width(), frame.height());
}

void VisionController::onStatsTick()
{
    if (!m_camera)
        return;
    m_performanceModel->update(m_camera->statsSnapshot());
}
