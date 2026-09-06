#ifndef VISIONCONTROLLER_H
#define VISIONCONTROLLER_H

#include <QObject>

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
public:
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

signals:
    void modeChanged();
    void runningChanged();
    void currentPageChanged();
    void frameSizeChanged();

private slots:
    void onFrameChanged();
    void onStatsTick();

private:
    QString m_mode;
    CameraManager* m_camera;
    bool m_running = false;
    int m_currentPage = 0;
    int m_frameWidth = 0;
    int m_frameHeight = 0;

    DetectionModel* m_detectionModel = nullptr;
    TrackModel* m_trackModel = nullptr;
    EventModel* m_eventModel = nullptr;
    EventTypeFilterModel* m_eventFilterModel = nullptr;
    PerformanceModel* m_performanceModel = nullptr;
    PluginModel* m_pluginModel = nullptr;
    RuleModel* m_ruleModel = nullptr;
};

#endif // VISIONCONTROLLER_H
