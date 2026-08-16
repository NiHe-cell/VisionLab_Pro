#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <memory>

#include <QImage>
#include <QMutex>
#include <QObject>

#include "core/PipelineStats.h"
#include "core/VisionTypes.h"
#include "pipeline/VisionPipeline.h"

// GUI 边界上的相机编排器：拥有 VisionPipeline，把 PresentedFrame 深拷贝为 QImage。
// QObject 只活在 GUI 线程；采集与推理在 pipeline 的 jthread 上。
// frame() 加锁，供场景图线程上的 ImageProvider 读取。
class CameraManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QImage frame READ frame NOTIFY frameChanged)
public:
    explicit CameraManager();
    explicit CameraManager(std::unique_ptr<visionlab::VisionPipeline> pipeline);

    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    ~CameraManager() override;

    bool start();
    bool stop();
    void setMode(visionlab::DetectionMode mode);

    QImage frame() const;
    visionlab::PipelineStats statsSnapshot() const;

signals:
    void frameChanged();
    void frameCleared();

private slots:
    void notifyFrame();

private:
    void bindPresentedCallback();

    std::unique_ptr<visionlab::VisionPipeline> m_pipeline;
    mutable QMutex m_frameMutex;
    QImage m_frame;
};

#endif // CAMERAMANAGER_H
