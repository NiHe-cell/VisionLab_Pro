#include "CameraManager.h"

#include <map>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QMetaObject>
#include <QMutexLocker>

#include "detectors/DetectorFactory.h"
#include "video/CameraSource.h"

namespace {

std::string modelDirPath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("VisionLab/models"))
        .toStdString();
}

std::unique_ptr<visionlab::VisionPipeline> makeProductionPipeline()
{
    std::map<visionlab::DetectionMode, std::unique_ptr<visionlab::IDetector>> detectors;
    const std::string modelDir = modelDirPath();
    for (const visionlab::DetectionMode mode :
         {visionlab::DetectionMode::Face,
          visionlab::DetectionMode::Object,
          visionlab::DetectionMode::Motion})
    {
        detectors.emplace(mode, visionlab::createDetector(mode, modelDir));
    }
    return std::make_unique<visionlab::VisionPipeline>(
        std::make_unique<visionlab::CameraSource>(0), std::move(detectors));
}

} // namespace

CameraManager::CameraManager()
    : CameraManager(makeProductionPipeline())
{
}

CameraManager::CameraManager(std::unique_ptr<visionlab::VisionPipeline> pipeline)
    : m_pipeline(std::move(pipeline))
{
    if (m_pipeline)
        m_pipeline->setMode(visionlab::DetectionMode::Face);
    bindPresentedCallback();
}

CameraManager::~CameraManager()
{
    if (m_pipeline)
        m_pipeline->stop();
}

void CameraManager::bindPresentedCallback()
{
    if (!m_pipeline)
        return;
    m_pipeline->setPresentedCallback([this] {
        QMetaObject::invokeMethod(this, &CameraManager::notifyFrame, Qt::QueuedConnection);
    });
}

bool CameraManager::start()
{
    if (!m_pipeline)
        return false;
    if (m_pipeline->isRunning())
        return true;

    if (!m_pipeline->start())
    {
        qWarning() << "CameraManager: 打开视频源失败";
        return false;
    }
    return true;
}

bool CameraManager::stop()
{
    if (!m_pipeline || !m_pipeline->isRunning())
        return false;

    m_pipeline->stop();
    {
        QMutexLocker lock(&m_frameMutex);
        m_frame = QImage();
    }
    emit frameCleared();
    return true;
}

void CameraManager::setMode(visionlab::DetectionMode mode)
{
    if (m_pipeline)
        m_pipeline->setMode(mode);
}

void CameraManager::notifyFrame()
{
    if (!m_pipeline || !m_pipeline->isRunning())
        return;

    const auto presented = m_pipeline->latest();
    if (!presented || presented->rgb.empty())
        return;

    const cv::Mat& rgb = presented->rgb;
    QImage next(rgb.data,
                rgb.cols,
                rgb.rows,
                static_cast<int>(rgb.step),
                QImage::Format_RGB888);
    next = next.copy();

    {
        QMutexLocker lock(&m_frameMutex);
        m_frame = std::move(next);
    }
    emit frameChanged();
}

QImage CameraManager::frame() const
{
    QMutexLocker lock(&m_frameMutex);
    return m_frame;
}

visionlab::PipelineStats CameraManager::statsSnapshot() const
{
    if (!m_pipeline)
        return {};
    return m_pipeline->stats();
}
