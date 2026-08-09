#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>

#include <QImage>
#include <QObject>

#include "core/VisionTypes.h"
#include "detectors/IDetector.h"
#include "rendering/DetectionRenderer.h"
#include "video/IVideoSource.h"

// 相机编排器：驱动 视频源 → 当前检测器 → 渲染器 → 帧发布。
//
// 不再直接拥有任何具体检测器：实例由工厂按 DetectionMode 创建注入，
// 模式切换为枚举分发，不存在字符串 if/else 链。
//
// 已知技术债（Phase 2 统一处理）：捕获循环仍是 detached std::thread；
// m_frame 的跨线程读写尚无同步（沿用旧行为，未恶化）。
class CameraManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QImage frame READ frame NOTIFY frameChanged)
public:
    explicit CameraManager();

    bool start();
    bool stop();
    void setMode(visionlab::DetectionMode mode);

    QImage frame() const;

signals:
    void frameChanged();
    void frameCleared();

private:
    void processFrame();

    std::unique_ptr<visionlab::IVideoSource> m_source;
    std::map<visionlab::DetectionMode, std::unique_ptr<visionlab::IDetector>> m_detectors;
    visionlab::DetectionRenderer m_renderer;

    // GUI 线程写 / 捕获线程读：原子化（较旧的裸跨线程变量严格改善）。
    std::atomic<visionlab::DetectionMode> m_mode{visionlab::DetectionMode::Face};
    std::atomic<bool> m_running{false};

    // 仅捕获线程访问：帧序号，用于 FramePacket 与 Object 模式跳帧。
    std::int64_t m_frameId = 0;

    QImage m_frame;
};

#endif // CAMERAMANAGER_H
