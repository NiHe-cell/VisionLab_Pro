/*
 * Author - Muhammed Suwaneh
*/

#include "CameraManager.h"
#include <QThread>
#include <QDebug>

CameraManager::CameraManager() : currentMode("Face Detection")
{
    //this->timer = new QTimer(this);
}

// convert opencv mat to QImage
inline QImage matToQImage(const cv::Mat& mat)
{
    switch (mat.type())
    {
        case CV_8UC3:
            return QImage(
                       mat.data,
                       mat.cols,
                       mat.rows,
                       mat.step,
                       QImage::Format_BGR888
                       ).copy();

        case CV_8UC1:
            return QImage(
                       mat.data,
                       mat.cols,
                       mat.rows,
                       mat.step,
                       QImage::Format_Grayscale8
                       ).copy();

        default:
            return QImage();
    }
}

bool CameraManager::start()
{
    if (running)
        return true;

    if (!cap.open(0))
        return false;

    running = true;

    // Run capture loop in background thread
    std::thread([this]() { processFrame(); }).detach();

    return true;
}

bool CameraManager::stop()
{
    if (!running)
        return false;

    running = false;

    if (cap.isOpened())
        cap.release();

    m_frame = QImage();
    emit frameCleared();

    return true;
}

void CameraManager::setMode(const QString& m)
{
    currentMode = m;
}

void CameraManager::processFrame()
{
    try
    {
        cv::Mat mat;

        while(true)
        {
            if(!running) break;

            this->cap >> mat;
            if (mat.empty())
                continue;

            // start detecting
            if(currentMode == "Face Detection")
                faceDetector.detect(mat);
            else if (currentMode == "Object Detection") {
                objectDetector.detect(mat);
            }
            else if (currentMode == "Motion Detection") {
                motionDetector.detect(mat);
            }

            cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);

            this->m_frame = QImage(
                                mat.data,
                                mat.cols,
                                mat.rows,
                                static_cast<int>(mat.step),
                                QImage::Format_RGB888
                                ).copy();


            if (!running)
                break;

            emit frameChanged();

            QThread::msleep(15);
        }
    }
    catch(QString msg)
    {
        qDebug() << msg;
    }
}

QImage CameraManager::frame() const
{
    return m_frame;
}
