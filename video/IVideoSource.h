#ifndef IVIDEOSOURCE_H
#define IVIDEOSOURCE_H

#include <string>

#include <opencv2/core.hpp>

namespace visionlab {

// 视频采集扩展点：屏蔽 cv::VideoCapture 等具体采集实现。
//
// 职责边界：本接口只交付原始帧数据（cv::Mat）。帧号分配与时间戳记录
// 由编排层负责（Phase 2 的 CaptureWorker 在采集后立即打戳）。
//
// 错误模型：open/read 以 bool 报告成败，细节经 lastError() 获取；
// close 总是安全可调（幂等）。实现不引入额外线程。
//
// 线程模型：同一实例不承诺并发调用；由单管线线程驱动。
class IVideoSource
{
public:
    virtual ~IVideoSource() = default;

    // 打开源；失败返回 false 并设置 lastError。重复调用应安全。
    virtual bool open() = 0;

    // 读取一帧；失败（含未打开、读帧错误、流结束）返回 false，
    // frame 内容保持未定义，调用方不得使用。
    virtual bool read(cv::Mat& frame) = 0;

    // 关闭并释放资源；幂等。
    virtual void close() = 0;

    virtual bool isOpen() const = 0;

    // 稳定标识（如 "camera:0"、"file:<路径>"），用于 FramePacket::sourceId。
    virtual std::string sourceId() const = 0;

    // 最近一次失败的可读描述；无错误时为空串。
    virtual std::string lastError() const = 0;
};

} // namespace visionlab

#endif // IVIDEOSOURCE_H
