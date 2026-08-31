#ifndef RULEGEOMETRY_H
#define RULEGEOMETRY_H

#include <vector>

#include <opencv2/core.hpp>

#include "core/VisionEvent.h"

namespace visionlab {

// 包围框底边中点。空框返回 (0, 0)。
cv::Point2f footPoint(const cv::Rect& box);

// polygon.size()<3 返回 false。cv::pointPolygonTest >= 0 视为 inside（含边）。
bool pointInPolygon(const cv::Point2f& point,
                    const std::vector<cv::Point2f>& polygon);

// 四顶点：TL, TR, BR, BL。空矩形仍返回 4 点。
std::vector<cv::Point2f> polygonFromRect(const cv::Rect& rect);

// 含端点相触。零长度线段返回 false。
bool segmentsIntersect(const cv::Point2f& p1, const cv::Point2f& p2,
                       const cv::Point2f& q1, const cv::Point2f& q2);

// 叉积符号：+1 左，-1 右，0 共线或 AB 零长度。
int lineSide(const cv::Point2f& point,
             const cv::Point2f& a, const cv::Point2f& b);

// 上一侧与本侧异号且都不为 0，且 prev-curr 与 A-B 相交 → Forward / Reverse。
CrossingDirection classifyCrossing(const cv::Point2f& previous,
                                   const cv::Point2f& current,
                                   const cv::Point2f& a,
                                   const cv::Point2f& b);

} // namespace visionlab

#endif // RULEGEOMETRY_H
