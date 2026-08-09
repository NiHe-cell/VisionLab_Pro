#include "CameraManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QThread>

#include <opencv2/imgproc.hpp>

#include "detectors/DetectorFactory.h"
#include "video/CameraSource.h"

namespace {

// 模型文件随 QML 模块资源一同被复制到 <应用目录>/VisionLab/models。
std::string modelDirPath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("VisionLab/models"))
        .toStdString();
}

} // namespace

CameraManager::CameraManager()
    : m_source(std::make_unique<visionlab::CameraSource>(0))
{
    const std::string modelDir = modelDirPath();
    for (const visionlab::DetectionMode mode :
         {visionlab::DetectionMode::Face,
          visionlab::DetectionMode::Object,
          visionlab::DetectionMode::Motion})
    {
        m_detectors.emplace(mode, visionlab::createDetector(mode, modelDir));
    }
}

bool CameraManager::start()
{
    if (m_running)
        return true;

    if (!m_source->open())
    {
        qWarning() << "CameraManager: 打开视频源失败:"
                   << QString::fromStdString(m_source->lastError());
        return false;
    }

    m_frameId = 0;
    m_running = true;

    // 捕获循环仍沿用原有后台线程（Phase 2 替换为 jthread + 有界队列）。
    std::thread([this]() { processFrame(); }).detach();

    return true;
}

bool CameraManager::stop()
{
    if (!m_running)
        return false;

    m_running = false;
    m_source->close();

    m_frame = QImage();
    emit frameCleared();

    return true;
}

void CameraManager::setMode(visionlab::DetectionMode mode)
{
    m_mode = mode;
}

void CameraManager::processFrame()
{
    try
    {
        while (m_running)
        {
            cv::Mat mat;
            if (!m_source->read(mat))
                continue;

            ++m_frameId;

            visionlab::FramePacket packet;
            packet.frameId = m_frameId;
            packet.captureTimestamp = std::chrono::steady_clock::now();
            packet.sourceId = m_source->sourceId();
            packet.image = mat;

            const visionlab::DetectionMode mode = m_mode.load();
            const auto it = m_detectors.find(mode);
            visionlab::IDetector* detector =
                it != m_detectors.end() ? it->second.get() : nullptr;

            // 保持旧行为：Object 模式每 3 帧才真正推理一次。
            const bool skipFrame =
                mode == visionlab::DetectionMode::Object && m_frameId % 3 != 0;

            if (detector && detector->isReady() && !skipFrame)
            {
                // 在克隆帧上绘制，遵守 FramePacket::image 只读契约。
                cv::Mat annotated = mat.clone();
                m_renderer.render(annotated, detector->detect(packet));
                mat = annotated;
            }

            cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);

            m_frame = QImage(mat.data,
                             mat.cols,
                             mat.rows,
                             static_cast<int>(mat.step),
                             QImage::Format_RGB888)
                          .copy();

            if (!m_running)
                break;

            emit frameChanged();

            QThread::msleep(15);
        }
    }
    catch (const cv::Exception& e)
    {
        qWarning() << "CameraManager: 捕获循环异常退出:" << e.what();
        m_running = false;
    }
}

QImage CameraManager::frame() const
{
    return m_frame;
}
