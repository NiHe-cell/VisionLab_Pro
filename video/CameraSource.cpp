#include "CameraSource.h"

namespace visionlab {

CameraSource::CameraSource(int deviceIndex)
    : m_deviceIndex(deviceIndex)
{
}

bool CameraSource::open()
{
    if (m_capture.isOpened())
        return true; // 重复打开是安全无操作

    if (!m_capture.open(m_deviceIndex))
    {
        m_lastError = "无法打开摄像头设备 " + std::to_string(m_deviceIndex);
        return false;
    }

    m_lastError.clear();
    return true;
}

bool CameraSource::read(cv::Mat& frame)
{
    if (!m_capture.isOpened())
    {
        m_lastError = "read 调用前源未打开";
        return false;
    }

    m_capture >> frame;
    if (frame.empty())
    {
        m_lastError = "读帧失败或视频流结束";
        return false;
    }

    return true;
}

void CameraSource::close()
{
    if (m_capture.isOpened())
        m_capture.release();
}

bool CameraSource::isOpen() const
{
    return m_capture.isOpened();
}

std::string CameraSource::sourceId() const
{
    return "camera:" + std::to_string(m_deviceIndex);
}

std::string CameraSource::lastError() const
{
    return m_lastError;
}

} // namespace visionlab
