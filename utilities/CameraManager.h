#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include "detectors/FaceDetector.h"
#include "detectors/ObjectDetector.h"
#include "detectors/MotionDetector.h"
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

    FaceDetector faceDetector;
    ObjectDetector objectDetector;
    MotionDetector motionDetector;

    QTimer* timer;
    QImage m_frame;
};

#endif // CAMERAMANAGER_H
