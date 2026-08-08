#ifndef IDETECTOR_H
#define IDETECTOR_H

#include <string>
#include <vector>

#include "core/Detection.h"
#include "core/FramePacket.h"
#include "core/VisionTypes.h"

namespace visionlab {

// 检测算法扩展点：一帧进、结构化 Detection 列表出。
//
// 约定：
// - 实现不得在帧上绘制；渲染由独立组件负责。
// - detect 接收 const FramePacket&，从类型上禁止检测器修改帧。
// - detect 允许为非 const：有状态实现（如背景建模）在内部维护状态。
// - 实现必须自身保证：模型缺失等情况通过 isReady()==false 表达，
//   此时 detect 必须返回空结果而不是崩溃。
//
// 线程模型：同一实例的 detect 不承诺可并发调用；
// 同一时刻只应由一个管线工作线程驱动某个实例。
class IDetector
{
public:
    virtual ~IDetector() = default;

    // 稳定的实现名称，用于日志与界面显示（如 "YOLOv4-tiny"）。
    virtual std::string name() const = 0;

    // 该检测器对应的管线模式，供上层按枚举分发。
    virtual DetectionMode mode() const = 0;

    // 模型/资源是否就绪；false 时 detect 必须安全地返回空结果。
    virtual bool isReady() const = 0;

    // 对一帧执行检测，返回原始帧像素坐标下的结构化结果。
    virtual std::vector<Detection> detect(const FramePacket& frame) = 0;
};

} // namespace visionlab

#endif // IDETECTOR_H
