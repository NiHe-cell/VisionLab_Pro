#include "KalmanBoxFilter.h"

#include <algorithm>

namespace visionlab {
namespace {

constexpr float kMinSize = 1.0F;
constexpr float kMinAspect = 1.0e-3F;
constexpr float kDefaultDt = 1.0F / 30.0F;

cv::Mat xyahFromRect(const cv::Rect& box)
{
    const float width = std::max(kMinSize, static_cast<float>(box.width));
    const float height = std::max(kMinSize, static_cast<float>(box.height));
    cv::Mat measurement(4, 1, CV_32F);
    measurement.at<float>(0) = static_cast<float>(box.x) + width * 0.5F;
    measurement.at<float>(1) = static_cast<float>(box.y) + height * 0.5F;
    measurement.at<float>(2) = width / height;
    measurement.at<float>(3) = height;
    return measurement;
}

cv::Rect rectFromState(const cv::Mat& state)
{
    const float cx = state.at<float>(0);
    const float cy = state.at<float>(1);
    const float aspect = std::max(kMinAspect, state.at<float>(2));
    const float height = std::max(kMinSize, state.at<float>(3));
    const float width = aspect * height;
    return {
        cvRound(cx - width * 0.5F),
        cvRound(cy - height * 0.5F),
        std::max(1, cvRound(width)),
        std::max(1, cvRound(height)),
    };
}

} // namespace

KalmanBoxFilter::KalmanBoxFilter(const cv::Rect& initialBox)
    : m_filter(8, 4, 0, CV_32F)
    , m_box(initialBox)
{
    m_filter.transitionMatrix = cv::Mat::eye(8, 8, CV_32F);
    setDt(kDefaultDt);

    m_filter.measurementMatrix = cv::Mat::zeros(4, 8, CV_32F);
    for (int i = 0; i < 4; ++i)
        m_filter.measurementMatrix.at<float>(i, i) = 1.0F;

    cv::setIdentity(m_filter.processNoiseCov, cv::Scalar::all(1.0e-2));
    cv::setIdentity(m_filter.measurementNoiseCov, cv::Scalar::all(1.0e-1));
    cv::setIdentity(m_filter.errorCovPost, cv::Scalar::all(1.0));

    const cv::Mat xyah = xyahFromRect(initialBox);
    m_filter.statePost = cv::Mat::zeros(8, 1, CV_32F);
    for (int i = 0; i < 4; ++i)
        m_filter.statePost.at<float>(i) = xyah.at<float>(i);
    m_box = rectFromState(m_filter.statePost);
}

void KalmanBoxFilter::setDt(float dt)
{
    m_filter.transitionMatrix.at<float>(0, 4) = dt;
    m_filter.transitionMatrix.at<float>(1, 5) = dt;
    m_filter.transitionMatrix.at<float>(2, 6) = dt;
    m_filter.transitionMatrix.at<float>(3, 7) = dt;
}

cv::Rect KalmanBoxFilter::predict(double dtSeconds)
{
    const float dt = dtSeconds > 0.0 ? static_cast<float>(dtSeconds) : kDefaultDt;
    setDt(dt);
    const cv::Mat predicted = m_filter.predict();
    predicted.copyTo(m_filter.statePost);
    m_box = rectFromState(m_filter.statePost);
    return m_box;
}

cv::Rect KalmanBoxFilter::update(const cv::Rect& measurement)
{
    const cv::Mat corrected = m_filter.correct(xyahFromRect(measurement));
    m_box = rectFromState(corrected);
    return m_box;
}

cv::Rect KalmanBoxFilter::box() const
{
    return m_box;
}

} // namespace visionlab
