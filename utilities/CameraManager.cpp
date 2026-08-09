/*
 * Author - Muhammed Suwaneh
*/

#include "CameraManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QThread>
#include <QDebug>

namespace {

// 模型文件随 QML 模块资源一同被复制到 <应用目录>/VisionLab/models。
std::string modelPath(const QString& fileName)
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("VisionLab/models/") + fileName)
        .toStdString();
}

} // namespace

CameraManager::CameraManager()
    : currentMode("Face Detection"),
      m_faceDetector(modelPath(QStringLiteral("deploy.prototxt")),
                     modelPath(QStringLiteral("res10_300x300_ssd_iter_140000.caffemodel"))),
      m_objectDetector(modelPath(QStringLiteral("yolov4-tiny.cfg")),
                       modelPath(QStringLiteral("yolov4-tiny.weights")),
                       modelPath(QStringLiteral("coco.names")))
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

            ++m_frameId;

            // 组装本帧的域数据包；image 按只读契约共享，绘制前需克隆。
            visionlab::FramePacket packet;
            packet.frameId = m_frameId;
            packet.captureTimestamp = std::chrono::steady_clock::now();
            packet.sourceId = "camera:0";
            packet.image = mat;

            // start detecting
            if(currentMode == "Face Detection")
            {
                cv::Mat annotated = mat.clone();
                m_renderer.render(annotated, m_faceDetector.detect(packet));
                mat = annotated;
            }
            else if (currentMode == "Object Detection") {
                // 保持旧行为：每 3 帧才真正推理一次，其余帧原样显示。
                if (m_frameId % 3 == 0)
                {
                    cv::Mat annotated = mat.clone();
                    m_renderer.render(annotated, m_objectDetector.detect(packet));
                    mat = annotated;
                }
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
