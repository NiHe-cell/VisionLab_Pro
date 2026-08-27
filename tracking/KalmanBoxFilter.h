#ifndef KALMANBOXFILTER_H
#define KALMANBOXFILTER_H

#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp>

namespace visionlab {

// SORT/BYTE 风格的恒速框滤波。状态 [cx, cy, a, h, vx, vy, va, vh]，
// a = width / height。不读取像素。
class KalmanBoxFilter
{
public:
    explicit KalmanBoxFilter(const cv::Rect& initialBox);

    cv::Rect predict(double dtSeconds);
    cv::Rect update(const cv::Rect& measurement);
    cv::Rect box() const;

private:
    void setDt(float dt);
    cv::KalmanFilter m_filter;
    cv::Rect m_box;
};

} // namespace visionlab

#endif // KALMANBOXFILTER_H
