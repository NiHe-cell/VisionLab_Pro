#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QImage>
#include <QMutex>
#include <QObject>

#include "analytics/RuleSpec.h"
#include "core/PipelineStats.h"
#include "core/VisionEvent.h"
#include "core/VisionTypes.h"
#include "pipeline/VisionPipeline.h"
#include "plugin/PluginManager.h"
#include "plugin/PluginMetadata.h"
#include "storage/EventQuery.h"
#include "storage/EventWriter.h"
#include "utilities/SessionSettings.h"
#include "video/IVideoSource.h"

// GUI 边界上的相机编排器：拥有 PluginManager 与 VisionPipeline，
// 把 PresentedFrame 深拷贝为 QImage。
// VisionPipeline 拥有 ITracker（生产路径为 ByteTrackTracker）
// 与空的 RuleEngine（无默认 ROI / 越线）。
// QObject 只活在 GUI 线程；采集与推理在 pipeline 的 jthread 上。
// frame() 加锁，供场景图线程上的 ImageProvider 读取。
//
// 成员顺序：先 PluginManager 后 pipeline，析构时先销毁检测器再释放 QPluginLoader。
class CameraManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QImage frame READ frame NOTIFY frameChanged)
public:
    explicit CameraManager();
    explicit CameraManager(std::unique_ptr<visionlab::IVideoSource> source);
    explicit CameraManager(std::unique_ptr<visionlab::VisionPipeline> pipeline);

    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    ~CameraManager() override;

    bool start();
    bool stop();
    void setMode(visionlab::DetectionMode mode);
    visionlab::DetectionMode mode() const;

    visionlab::SessionSettings sessionSettings() const;
    // running 时返回 false 且不改动。停机时用新设置重建管线。
    bool applySessionSettings(const visionlab::SessionSettings& settings);

    std::vector<visionlab::PluginMetadata> pluginMetadata() const;
    std::vector<std::string> pluginLoadErrors() const;

    std::vector<visionlab::RuleSpec> ruleSpecs() const;
    // running 时返回 false 且不改引擎。整批 makeRule 成功才 clear + 注入。
    bool applyRuleSpecs(std::vector<visionlab::RuleSpec> specs);

    void setEventDatabasePath(const std::filesystem::path& path);
    void queryEvents(const visionlab::EventQuery& query,
                     QObject* receiver,
                     std::function<void(std::vector<visionlab::StoredEvent>)> onResult);
    std::vector<visionlab::VisionEvent> recentEvents() const;

    QImage frame() const;
    visionlab::PipelineStats statsSnapshot() const;

signals:
    void frameChanged();
    void frameCleared();

private slots:
    void notifyFrame();

private:
    void assembleFromPlugins(std::unique_ptr<visionlab::IVideoSource> source);
    void bindPresentedCallback();
    void injectRules();

    visionlab::PluginManager m_plugins;
    visionlab::SessionSettings m_session;
    std::vector<visionlab::RuleSpec> m_ruleSpecs;
    std::unique_ptr<visionlab::VisionPipeline> m_pipeline;
    std::unique_ptr<visionlab::EventWriter> m_writer;
    std::uint64_t m_persistedMaxEventId = 0;
    mutable QMutex m_frameMutex;
    QImage m_frame;
};

#endif // CAMERAMANAGER_H
