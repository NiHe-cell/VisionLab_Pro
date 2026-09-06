#ifndef VISIONCONTROLLER_H
#define VISIONCONTROLLER_H

#include <QObject>
#include <QPointF>
#include <QVariantList>
#include <vector>

#include <opencv2/core.hpp>

#include "analytics/RuleSpec.h"
#include "models/DetectionModel.h"
#include "models/EventModel.h"
#include "models/EventTypeFilterModel.h"
#include "models/PerformanceModel.h"
#include "models/PluginModel.h"
#include "models/RuleModel.h"
#include "models/TrackModel.h"
#include "utilities/CameraManager.h"

// QML 只绑本对象与其上的 ListModel。禁止从 QML 直接碰 pipeline /
// RuleEngine / 检测器 / EventWriter。
class VisionController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(DetectionModel* detectionModel READ detectionModel CONSTANT)
    Q_PROPERTY(TrackModel* trackModel READ trackModel CONSTANT)
    Q_PROPERTY(EventModel* eventModel READ eventModel CONSTANT)
    Q_PROPERTY(EventTypeFilterModel* eventFilterModel READ eventFilterModel CONSTANT)
    Q_PROPERTY(PerformanceModel* performanceModel READ performanceModel CONSTANT)
    Q_PROPERTY(PluginModel* pluginModel READ pluginModel CONSTANT)
    Q_PROPERTY(RuleModel* ruleModel READ ruleModel CONSTANT)
    Q_PROPERTY(int frameWidth READ frameWidth NOTIFY frameSizeChanged)
    Q_PROPERTY(int frameHeight READ frameHeight NOTIFY frameSizeChanged)
    Q_PROPERTY(int drawTool READ drawTool NOTIFY drawToolChanged)
    Q_PROPERTY(QVariantList draftPoints READ draftPoints NOTIFY draftPointsChanged)
    Q_PROPERTY(int eventTypeFilter READ eventTypeFilter WRITE setEventTypeFilter NOTIFY eventTypeFilterChanged)
public:
    enum DrawTool
    {
        None = 0,
        Roi,
        Line,
        Loiter,
        Count,
    };
    Q_ENUM(DrawTool)

    explicit VisionController(CameraManager* cam, QObject* parent = nullptr);

    QString mode() const;
    Q_INVOKABLE void setMode(QString newMode);
    Q_INVOKABLE void startCamera();
    Q_INVOKABLE void stopCamera();

    bool running() const;
    void setRunning(bool newRunning);

    int currentPage() const;
    void setCurrentPage(int page);

    DetectionModel* detectionModel() const { return m_detectionModel; }
    TrackModel* trackModel() const { return m_trackModel; }
    EventModel* eventModel() const { return m_eventModel; }
    EventTypeFilterModel* eventFilterModel() const { return m_eventFilterModel; }
    PerformanceModel* performanceModel() const { return m_performanceModel; }
    PluginModel* pluginModel() const { return m_pluginModel; }
    RuleModel* ruleModel() const { return m_ruleModel; }

    int frameWidth() const { return m_frameWidth; }
    int frameHeight() const { return m_frameHeight; }
    void setFrameSize(int width, int height);

    int drawTool() const { return static_cast<int>(m_drawTool); }
    QVariantList draftPoints() const;

    Q_INVOKABLE QPointF itemToFrame(qreal x, qreal y, qreal itemW, qreal itemH) const;
    Q_INVOKABLE QPointF frameToItem(qreal fx, qreal fy, qreal itemW, qreal itemH) const;
    Q_INVOKABLE bool itemPointInVideo(qreal x, qreal y, qreal itemW, qreal itemH) const;
    Q_INVOKABLE void beginDraw(DrawTool tool);
    Q_INVOKABLE void addDrawPoint(qreal itemX, qreal itemY, qreal itemW, qreal itemH);
    Q_INVOKABLE void finishDraw();
    Q_INVOKABLE void cancelDraw();
    Q_INVOKABLE void commitRulesToEngine();

    int eventTypeFilter() const;
    void setEventTypeFilter(int filter);

signals:
    void modeChanged();
    void runningChanged();
    void currentPageChanged();
    void frameSizeChanged();
    void drawToolChanged();
    void draftPointsChanged();
    void eventTypeFilterChanged();

private slots:
    void onFrameChanged();
    void onStatsTick();

private:
    visionlab::RuleKind kindForTool(DrawTool tool) const;
    void clearDraft();

    QString m_mode;
    CameraManager* m_camera;
    bool m_running = false;
    int m_currentPage = 0;
    int m_frameWidth = 0;
    int m_frameHeight = 0;
    DrawTool m_drawTool = None;
    std::vector<cv::Point2f> m_draft;

    DetectionModel* m_detectionModel = nullptr;
    TrackModel* m_trackModel = nullptr;
    EventModel* m_eventModel = nullptr;
    EventTypeFilterModel* m_eventFilterModel = nullptr;
    PerformanceModel* m_performanceModel = nullptr;
    PluginModel* m_pluginModel = nullptr;
    RuleModel* m_ruleModel = nullptr;
};

#endif // VISIONCONTROLLER_H
