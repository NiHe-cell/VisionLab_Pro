#ifndef LATESTRESULT_H
#define LATESTRESULT_H

#include <mutex>
#include <optional>
#include <utility>

namespace visionlab {

// 最新结果邮箱：容量为 1，publish 覆盖写入，snapshot 拷贝取样、不消费。
//
// 用于推理结果交给 UI：读者只关心最新一帧，不会在结果侧堆积。
// 需要阻塞等待的阶段应使用 BoundedQueue，而不是本类型。
//
// T 必须可拷贝（snapshot 返回独立副本）。cv::Mat 拷贝只增加引用计数，
// 像素所有权仍按 FramePacket 契约：读者视为只读，写入前 clone。
//
// 无 Qt 依赖。
template<typename T>
class LatestResult
{
public:
    LatestResult() = default;
    LatestResult(const LatestResult&) = delete;
    LatestResult& operator=(const LatestResult&) = delete;

    void publish(T value)
    {
        std::lock_guard lock(m_mutex);
        m_value = std::move(value);
    }

    std::optional<T> snapshot() const
    {
        std::lock_guard lock(m_mutex);
        return m_value;
    }

    void clear()
    {
        std::lock_guard lock(m_mutex);
        m_value.reset();
    }

private:
    mutable std::mutex m_mutex;
    std::optional<T> m_value;
};

} // namespace visionlab

#endif // LATESTRESULT_H
