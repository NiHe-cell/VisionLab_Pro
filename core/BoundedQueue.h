#ifndef BOUNDEDQUEUE_H
#define BOUNDEDQUEUE_H

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace visionlab {

// 队列满时的溢出策略。实时帧管线默认 DropOldest。
enum class OverflowPolicy
{
    // 生产者阻塞直到有空位或 close。
    BlockProducer,
    // 拒绝本次入队（丢掉最新），不阻塞。
    DropNewest,
    // 丢掉队头（最旧）再入队，不阻塞。
    DropOldest,
};

// 线程安全有界队列。
//
// 契约：
// - push 按值接收以便移动；返回 true 表示本元素已入队。
// - DropOldest 满时丢掉最旧元素后仍入队（除非已 close）。
// - DropNewest 满时拒绝本元素并计入 droppedCount。
// - BlockProducer 满时等待；close 期间等待会被唤醒且 push 返回 false。
// - pop 阻塞直到有元素或已 close；close 后先排空再返回 false。
// - close 幂等，唤醒所有等待者。析构会 close。
// - 销毁前应先 close 并汇合所有生产者/消费者线程，避免析构与 wait 并发。
//
// 无 Qt 依赖。
template<typename T>
class BoundedQueue
{
public:
    explicit BoundedQueue(std::size_t capacity,
                          OverflowPolicy policy = OverflowPolicy::DropOldest)
        : m_capacity(capacity)
        , m_policy(policy)
    {
        if (m_capacity == 0)
            throw std::invalid_argument("BoundedQueue capacity must be >= 1");
    }

    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;

    ~BoundedQueue() { close(); }

    bool push(T item)
    {
        std::unique_lock lock(m_mutex);
        if (m_policy == OverflowPolicy::BlockProducer)
        {
            m_notFull.wait(lock, [&] { return m_closed || m_queue.size() < m_capacity; });
        }
        if (m_closed)
            return false;

        if (m_queue.size() >= m_capacity)
        {
            if (m_policy == OverflowPolicy::DropNewest)
            {
                ++m_dropped;
                return false;
            }
            m_queue.pop_front();
            ++m_dropped;
        }

        m_queue.push_back(std::move(item));
        lock.unlock();
        m_notEmpty.notify_one();
        return true;
    }

    bool pop(T& out)
    {
        std::unique_lock lock(m_mutex);
        m_notEmpty.wait(lock, [&] { return m_closed || !m_queue.empty(); });
        if (m_queue.empty())
            return false;

        out = std::move(m_queue.front());
        m_queue.pop_front();
        lock.unlock();
        m_notFull.notify_one();
        return true;
    }

    void close()
    {
        std::lock_guard lock(m_mutex);
        if (m_closed)
            return;
        m_closed = true;
        m_notEmpty.notify_all();
        m_notFull.notify_all();
    }

    bool closed() const
    {
        std::lock_guard lock(m_mutex);
        return m_closed;
    }

    std::size_t size() const
    {
        std::lock_guard lock(m_mutex);
        return m_queue.size();
    }

    std::size_t capacity() const { return m_capacity; }

    std::size_t droppedCount() const
    {
        std::lock_guard lock(m_mutex);
        return m_dropped;
    }

private:
    const std::size_t m_capacity;
    const OverflowPolicy m_policy;
    mutable std::mutex m_mutex;
    std::condition_variable m_notEmpty;
    std::condition_variable m_notFull;
    std::deque<T> m_queue;
    bool m_closed = false;
    std::size_t m_dropped = 0;
};

} // namespace visionlab

#endif // BOUNDEDQUEUE_H
