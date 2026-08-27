#include "CameraManager.h"

#include <filesystem>
#include <map>
#include <string>
#include <utility>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QMetaObject>
#include <QMutexLocker>
#include <QStringList>

#include "inference/InferenceEngineFactory.h"
#include "inference/InferenceSelection.h"
#include "plugin/DetectorCreateRequest.h"
#include "tracking/ByteTrackTracker.h"
#include "video/CameraSource.h"

namespace {

std::string modelDirPath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("VisionLab/models"))
        .toStdString();
}

std::filesystem::path pluginDirPath()
{
    const QByteArray env = qgetenv("VISIONLAB_PLUGIN_DIR");
    if (!env.isEmpty())
        return QString::fromUtf8(env).toStdString();
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("plugins"))
        .toStdString();
}

void warnInvalidInferenceEnv()
{
    const QByteArray backend = qgetenv("VISIONLAB_INFERENCE_BACKEND");
    if (!backend.isEmpty()
        && !visionlab::parseInferenceBackend(backend.toStdString()).has_value())
    {
        qWarning() << "CameraManager: unknown VISIONLAB_INFERENCE_BACKEND"
                   << backend << "- using onnx-cpu";
    }

    const QByteArray precision = qgetenv("VISIONLAB_INFERENCE_PRECISION");
    if (!precision.isEmpty()
        && !visionlab::parseInferencePrecision(precision.toStdString()).has_value())
    {
        qWarning() << "CameraManager: unknown VISIONLAB_INFERENCE_PRECISION"
                   << precision << "- using fp32";
    }

    const QByteArray device = qgetenv("VISIONLAB_INFERENCE_DEVICE");
    if (!device.isEmpty())
    {
        bool ok = false;
        const int parsed = device.toInt(&ok);
        if (!ok || parsed < 0)
        {
            qWarning() << "CameraManager: invalid VISIONLAB_INFERENCE_DEVICE"
                       << device << "- using 0";
        }
    }
}

QString modeLabel(visionlab::DetectionMode mode)
{
    const auto label = visionlab::labelForDetectionMode(mode);
    return QString::fromUtf8(label.data(), static_cast<int>(label.size()));
}

} // namespace

CameraManager::CameraManager()
    : CameraManager(std::make_unique<visionlab::CameraSource>(0))
{
}

CameraManager::CameraManager(std::unique_ptr<visionlab::IVideoSource> source)
{
    assembleFromPlugins(std::move(source));
    bindPresentedCallback();
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

void CameraManager::assembleFromPlugins(std::unique_ptr<visionlab::IVideoSource> source)
{
    warnInvalidInferenceEnv();
    const visionlab::InferenceSelection selection = visionlab::inferenceSelectionFromEnv();

    m_plugins.scan(pluginDirPath());

    QStringList ids;
    for (const auto& meta : m_plugins.metadata())
        ids << QString::fromStdString(meta.id);
    qInfo() << "CameraManager: loaded plugins" << ids;

    visionlab::DetectorCreateRequest request;
    request.modelDir = modelDirPath();
    request.backend = selection.backend;
    request.precision = selection.precision;
    request.deviceId = selection.deviceId;

    std::map<visionlab::DetectionMode, std::unique_ptr<visionlab::IDetector>> detectors;
    const auto plugins = m_plugins.metadata();
    for (const visionlab::DetectionMode mode :
         {visionlab::DetectionMode::Face,
          visionlab::DetectionMode::Object,
          visionlab::DetectionMode::Motion})
    {
        const visionlab::PluginMetadata* match = nullptr;
        for (const auto& meta : plugins)
        {
            if (meta.mode.has_value() && *meta.mode == mode)
            {
                match = &meta;
                break;
            }
        }
        if (!match)
        {
            qWarning() << "CameraManager: no plugin for" << modeLabel(mode);
            continue;
        }

        auto detector = m_plugins.createDetector(match->id, request);
        if (!detector)
        {
            qWarning() << "CameraManager: createDetector failed for"
                       << QString::fromStdString(match->id);
            continue;
        }
        detectors.emplace(mode, std::move(detector));
    }

    m_pipeline = std::make_unique<visionlab::VisionPipeline>(
        std::move(source), std::move(detectors),
        visionlab::VisionPipeline::kDefaultQueueCapacity,
        std::make_unique<visionlab::ByteTrackTracker>());
    m_pipeline->setMode(visionlab::DetectionMode::Face);
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
