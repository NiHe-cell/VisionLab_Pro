#ifndef FAKEVIDEOSOURCE_H
#define FAKEVIDEOSOURCE_H

#include <mutex>
#include <string>
#include <utility>

#include "video/IVideoSource.h"

// 内存假视频源：按脚本产出固定数量的合成帧。
// 状态受互斥保护，允许测试线程 close、工作线程 read 并发。
class FakeVideoSource : public visionlab::IVideoSource
{
public:
    explicit FakeVideoSource(int frameCount,
                             std::string id = "fake:0",
                             bool openSucceeds = true,
                             bool loop = false)
        : m_frameCount(frameCount)
        , m_id(std::move(id))
        , m_openSucceeds(openSucceeds)
        , m_loop(loop)
    {
    }

    bool open() override
    {
        std::lock_guard lock(m_mutex);
        if (!m_openSucceeds)
        {
            m_lastError = "forced open failure";
            return false;
        }
        m_open = true;
        m_readCount = 0;
        m_lastError.clear();
        return true;
    }

    bool read(cv::Mat& frame) override
    {
        std::lock_guard lock(m_mutex);
        if (!m_open)
        {
            m_lastError = "read before open";
            return false;
        }
        if (m_readCount >= m_frameCount)
        {
            if (!m_loop || m_frameCount <= 0)
            {
                m_lastError = "end of stream";
                return false;
            }
            m_readCount = 0;
        }
        frame = cv::Mat(4, 4, CV_8UC1, cv::Scalar(m_readCount % 255));
        ++m_readCount;
        return true;
    }

    void close() override
    {
        std::lock_guard lock(m_mutex);
        m_open = false;
    }

    bool isOpen() const override
    {
        std::lock_guard lock(m_mutex);
        return m_open;
    }

    std::string sourceId() const override
    {
        std::lock_guard lock(m_mutex);
        return m_id;
    }

    std::string lastError() const override
    {
        std::lock_guard lock(m_mutex);
        return m_lastError;
    }

private:
    const int m_frameCount;
    const std::string m_id;
    const bool m_openSucceeds;
    const bool m_loop;
    mutable std::mutex m_mutex;
    bool m_open = false;
    int m_readCount = 0;
    std::string m_lastError;
};

#endif // FAKEVIDEOSOURCE_H
