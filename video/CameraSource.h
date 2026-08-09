#ifndef CAMERASOURCE_H
#define CAMERASOURCE_H

#include <string>

#include <opencv2/videoio.hpp>

#include "IVideoSource.h"

namespace visionlab {

// 本地摄像头视频源：封装 cv::VideoCapture，保持现有的
// 默认后端采集行为。设备索引由构造注入。
class CameraSource : public IVideoSource
{
public:
    explicit CameraSource(int deviceIndex = 0);

    bool open() override;
    bool read(cv::Mat& frame) override;
    void close() override;

    bool isOpen() const override;
    std::string sourceId() const override;
    std::string lastError() const override;

private:
    int m_deviceIndex;
    cv::VideoCapture m_capture;
    std::string m_lastError;
};

} // namespace visionlab

#endif // CAMERASOURCE_H
