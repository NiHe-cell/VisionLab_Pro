#include "RuleGeometry.h"

#include <algorithm>
#include <cmath>

#include <opencv2/imgproc.hpp>

namespace visionlab {
namespace {

constexpr float kEps = 1e-6F;

bool nearlyZero(float value)
{
    return std::fabs(value) < kEps;
}

bool samePoint(const cv::Point2f& a, const cv::Point2f& b)
{
    return nearlyZero(a.x - b.x) && nearlyZero(a.y - b.y);
}

float cross2(const cv::Point2f& u, const cv::Point2f& v)
{
    return u.x * v.y - u.y * v.x;
}

int orientation(const cv::Point2f& p, const cv::Point2f& q, const cv::Point2f& r)
{
    const float value = cross2(q - p, r - p);
    if (nearlyZero(value))
        return 0;
    return value > 0.0F ? 1 : -1;
}

bool onSegment(const cv::Point2f& p, const cv::Point2f& q, const cv::Point2f& r)
{
    return q.x <= std::max(p.x, r.x) + kEps
        && q.x >= std::min(p.x, r.x) - kEps
        && q.y <= std::max(p.y, r.y) + kEps
        && q.y >= std::min(p.y, r.y) - kEps;
}

} // namespace

cv::Point2f footPoint(const cv::Rect& box)
{
    if (box.empty())
        return {};
    return {static_cast<float>(box.x) + static_cast<float>(box.width) * 0.5F,
            static_cast<float>(box.y + box.height)};
}

bool pointInPolygon(const cv::Point2f& point,
                    const std::vector<cv::Point2f>& polygon)
{
    if (polygon.size() < 3)
        return false;
    return cv::pointPolygonTest(polygon, point, false) >= 0.0;
}

std::vector<cv::Point2f> polygonFromRect(const cv::Rect& rect)
{
    const float x = static_cast<float>(rect.x);
    const float y = static_cast<float>(rect.y);
    const float right = static_cast<float>(rect.x + rect.width);
    const float bottom = static_cast<float>(rect.y + rect.height);
    return {{x, y}, {right, y}, {right, bottom}, {x, bottom}};
}

bool segmentsIntersect(const cv::Point2f& p1, const cv::Point2f& p2,
                       const cv::Point2f& q1, const cv::Point2f& q2)
{
    if (samePoint(p1, p2) || samePoint(q1, q2))
        return false;

    const int o1 = orientation(p1, p2, q1);
    const int o2 = orientation(p1, p2, q2);
    const int o3 = orientation(q1, q2, p1);
    const int o4 = orientation(q1, q2, p2);

    if (o1 != o2 && o3 != o4)
        return true;
    if (o1 == 0 && onSegment(p1, q1, p2))
        return true;
    if (o2 == 0 && onSegment(p1, q2, p2))
        return true;
    if (o3 == 0 && onSegment(q1, p1, q2))
        return true;
    if (o4 == 0 && onSegment(q1, p2, q2))
        return true;
    return false;
}

int lineSide(const cv::Point2f& point,
             const cv::Point2f& a, const cv::Point2f& b)
{
    if (samePoint(a, b))
        return 0;
    const float value = cross2(b - a, point - a);
    if (nearlyZero(value))
        return 0;
    return value > 0.0F ? 1 : -1;
}

CrossingDirection classifyCrossing(const cv::Point2f& previous,
                                   const cv::Point2f& current,
                                   const cv::Point2f& a,
                                   const cv::Point2f& b)
{
    const int previousSide = lineSide(previous, a, b);
    const int currentSide = lineSide(current, a, b);
    if (previousSide == 0 || currentSide == 0 || previousSide == currentSide)
        return CrossingDirection::None;
    if (!segmentsIntersect(previous, current, a, b))
        return CrossingDirection::None;
    return previousSide > 0 ? CrossingDirection::Forward
                            : CrossingDirection::Reverse;
}

} // namespace visionlab
