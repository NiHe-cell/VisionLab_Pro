#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include "detectors/FaceDetector.h"
#include "detectors/ObjectDetector.h"
#include "detectors/MotionDetector.h"
#include "rendering/DetectionRenderer.h"
#include <QTimer>
#include <QImage>

class CameraManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QImage frame READ frame NOTIFY frameChanged)
public:
    explicit CameraManager();

    bool start();
    bool stop();
    void setMode(const QString& m);

    QImage frame() const;

signals:
    void frameChanged();
    void frameCleared();

private:
    void processFrame();

    QString currentMode;
    cv::VideoCapture cap;
    bool running = false;

    visionlab::FaceDetector m_faceDetector;
    visionlab::ObjectDetector m_objectDetector;
    visionlab::MotionDetector m_motionDetector;

    // 检测标注的唯一写入方；Object 模式已改走结构化结果 + 渲染器。
    visionlab::DetectionRenderer m_renderer;
    // 捕获循环内的帧序号，用于组装 FramePacket 与保持原有跳帧节奏。
    std::int64_t m_frameId = 0;

    QTimer* timer;
    QImage m_frame;
};

#endif // CAMERAMANAGER_H
