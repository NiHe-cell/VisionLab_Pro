#ifndef LATENCYWINDOW_H
#define LATENCYWINDOW_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace visionlab {

// 固定容量环形缓冲，用于推理延迟的 mean / 百分位。
// 非线程安全：由 StatsProbe 的互斥保护。record 在容量填满后不再分配。
class LatencyWindow
{
public:
    explicit LatencyWindow(std::size_t capacity = 256)
        : m_capacity(capacity)
        , m_samples()
        , m_next(0)
    {
        if (m_capacity == 0)
            throw std::invalid_argument("LatencyWindow capacity must be >= 1");
        m_samples.reserve(m_capacity);
    }

    void record(double milliseconds)
    {
        if (m_samples.size() < m_capacity)
        {
            m_samples.push_back(milliseconds);
            return;
        }
        m_samples[m_next] = milliseconds;
        m_next = (m_next + 1) % m_capacity;
    }

    void reset()
    {
        m_samples.clear();
        m_next = 0;
    }

    double mean() const
    {
        if (m_samples.empty())
            return 0.0;
        double sum = 0.0;
        for (double sample : m_samples)
            sum += sample;
        return sum / static_cast<double>(m_samples.size());
    }

    // percent 取值 [0, 100]。对已排序样本做线性插值；空窗口返回 0。
    double percentile(double percent) const
    {
        if (m_samples.empty())
            return 0.0;

        std::vector<double> sorted = m_samples;
        std::sort(sorted.begin(), sorted.end());

        const double clamped = std::clamp(percent, 0.0, 100.0);
        const double rank = (clamped / 100.0) * static_cast<double>(sorted.size() - 1);
        const std::size_t lo = static_cast<std::size_t>(std::floor(rank));
        const std::size_t hi = static_cast<std::size_t>(std::ceil(rank));
        if (lo == hi)
            return sorted[lo];
        const double frac = rank - static_cast<double>(lo);
        return sorted[lo] + frac * (sorted[hi] - sorted[lo]);
    }

    std::size_t size() const { return m_samples.size(); }

private:
    std::size_t m_capacity;
    std::vector<double> m_samples;
    std::size_t m_next;
};

} // namespace visionlab

#endif // LATENCYWINDOW_H
