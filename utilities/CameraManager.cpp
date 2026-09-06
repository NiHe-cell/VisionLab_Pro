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
#include <QStandardPaths>
#include <QStringList>

#include "inference/InferenceEngineFactory.h"
#include "inference/InferenceSelection.h"
#include "plugin/DetectorCreateRequest.h"
#include "analytics/IRule.h"
#include "analytics/RuleEngine.h"
#include "analytics/RuleFactory.h"
#include "storage/SqliteEventRepository.h"
#include "tracking/ByteTrackTracker.h"
#include "tracking/ITracker.h"
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

std::filesystem::path defaultEventDbPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return std::filesystem::path(QDir(dir).filePath(QStringLiteral("events.sqlite")).toStdWString());
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
    if (m_writer && !m_writer->start(defaultEventDbPath()))
        qWarning() << "CameraManager: event database open failed";
}

CameraManager::CameraManager(std::unique_ptr<visionlab::IVideoSource> source)
{
    warnInvalidInferenceEnv();
    m_session.inference = visionlab::inferenceSelectionFromEnv();
    assembleFromPlugins(std::move(source));
    bindPresentedCallback();
    m_writer = std::make_unique<visionlab::EventWriter>(
        std::make_unique<visionlab::SqliteEventRepository>());
}

CameraManager::CameraManager(std::unique_ptr<visionlab::VisionPipeline> pipeline)
    : m_pipeline(std::move(pipeline))
{
    warnInvalidInferenceEnv();
    m_session.inference = visionlab::inferenceSelectionFromEnv();
    if (m_pipeline)
        m_pipeline->setMode(visionlab::DetectionMode::Face);
    bindPresentedCallback();
    m_writer = std::make_unique<visionlab::EventWriter>(
        std::make_unique<visionlab::SqliteEventRepository>());
}

CameraManager::~CameraManager()
{
    if (m_pipeline)
        m_pipeline->stop();
    if (m_writer)
        m_writer->stop();
}

void CameraManager::assembleFromPlugins(std::unique_ptr<visionlab::IVideoSource> source)
{
    m_plugins.scan(pluginDirPath());

    QStringList ids;
    for (const auto& meta : m_plugins.metadata())
        ids << QString::fromStdString(meta.id);
    qInfo() << "CameraManager: loaded plugins" << ids;

    visionlab::DetectorCreateRequest request;
    request.modelDir = modelDirPath();
    request.backend = m_session.inference.backend;
    request.precision = m_session.inference.precision;
    request.deviceId = m_session.inference.deviceId;
    request.confidenceThreshold = m_session.confidenceThreshold;
    request.nmsThreshold = m_session.nmsThreshold;

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

    // 生产注入空 RuleEngine，不注册默认 ROI / 越线。
    std::unique_ptr<visionlab::ITracker> tracker;
    if (m_session.trackingEnabled)
        tracker = std::make_unique<visionlab::ByteTrackTracker>();
    m_pipeline = std::make_unique<visionlab::VisionPipeline>(
        std::move(source), std::move(detectors),
        visionlab::VisionPipeline::kDefaultQueueCapacity,
        std::move(tracker),
        std::make_unique<visionlab::RuleEngine>());
    m_pipeline->setMode(visionlab::DetectionMode::Face);
    injectRules();
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

    injectRules();
    if (!m_pipeline->start())
    {
        qWarning() << "CameraManager: 打开视频源失败";
        return false;
    }
    m_persistedMaxEventId = 0;
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
        m_detections.clear();
        m_tracks.clear();
    }
    emit frameCleared();
    return true;
}

void CameraManager::setMode(visionlab::DetectionMode mode)
{
    if (m_pipeline)
        m_pipeline->setMode(mode);
}

visionlab::DetectionMode CameraManager::mode() const
{
    if (!m_pipeline)
        return visionlab::DetectionMode::None;
    return m_pipeline->mode();
}

visionlab::SessionSettings CameraManager::sessionSettings() const
{
    return m_session;
}

bool CameraManager::applySessionSettings(const visionlab::SessionSettings& settings)
{
    if (m_pipeline && m_pipeline->isRunning())
        return false;
    if (!m_pipeline)
        return false;

    auto source = m_pipeline->releaseSource();
    if (!source)
        return false;

    m_session = settings;
    assembleFromPlugins(std::move(source));
    bindPresentedCallback();
    return true;
}

std::vector<visionlab::PluginMetadata> CameraManager::pluginMetadata() const
{
    return m_plugins.metadata();
}

std::vector<std::string> CameraManager::pluginLoadErrors() const
{
    return m_plugins.errors();
}

std::vector<visionlab::RuleSpec> CameraManager::ruleSpecs() const
{
    return m_ruleSpecs;
}

bool CameraManager::applyRuleSpecs(std::vector<visionlab::RuleSpec> specs)
{
    if (m_pipeline && m_pipeline->isRunning())
        return false;

    std::vector<std::unique_ptr<visionlab::IRule>> built;
    built.reserve(specs.size());
    for (const visionlab::RuleSpec& spec : specs)
    {
        auto rule = visionlab::makeRule(spec);
        if (!rule)
            return false;
        built.push_back(std::move(rule));
    }

    m_ruleSpecs = std::move(specs);
    if (visionlab::RuleEngine* engine = m_pipeline ? m_pipeline->ruleEngine() : nullptr)
    {
        engine->clear();
        for (std::size_t i = 0; i < built.size(); ++i)
        {
            const bool enabled = m_ruleSpecs[i].enabled;
            const std::string id = m_ruleSpecs[i].ruleId;
            engine->addRule(std::move(built[i]));
            engine->setEnabled(id, enabled);
        }
    }
    return true;
}

void CameraManager::setEventDatabasePath(const std::filesystem::path& path)
{
    if (!m_writer)
    {
        m_writer = std::make_unique<visionlab::EventWriter>(
            std::make_unique<visionlab::SqliteEventRepository>());
    }
    if (!m_writer->start(path))
        qWarning() << "CameraManager: event database open failed";
}

void CameraManager::queryEvents(const visionlab::EventQuery& query,
                               QObject* receiver,
                               std::function<void(std::vector<visionlab::StoredEvent>)> onResult)
{
    if (m_writer)
        m_writer->requestQuery(query, receiver, std::move(onResult));
}

std::vector<visionlab::VisionEvent> CameraManager::recentEvents() const
{
    if (!m_pipeline)
        return {};
    return m_pipeline->recentEvents();
}

std::vector<visionlab::Detection> CameraManager::latestDetections() const
{
    QMutexLocker lock(&m_frameMutex);
    return m_detections;
}

std::vector<visionlab::Track> CameraManager::latestTracks() const
{
    QMutexLocker lock(&m_frameMutex);
    return m_tracks;
}

void CameraManager::injectRules()
{
    visionlab::RuleEngine* engine = m_pipeline ? m_pipeline->ruleEngine() : nullptr;
    if (!engine)
        return;
    engine->clear();
    for (const visionlab::RuleSpec& spec : m_ruleSpecs)
    {
        auto rule = visionlab::makeRule(spec);
        if (!rule)
            continue;
        const bool enabled = spec.enabled;
        const std::string id = spec.ruleId;
        engine->addRule(std::move(rule));
        engine->setEnabled(id, enabled);
    }
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
        m_detections = presented->detections;
        m_tracks = presented->tracks;
    }

    if (m_writer)
    {
        for (const visionlab::VisionEvent& event : m_pipeline->recentEvents())
        {
            if (event.eventId <= m_persistedMaxEventId)
                continue;
            m_writer->enqueue(event);
            m_persistedMaxEventId = event.eventId;
        }
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
