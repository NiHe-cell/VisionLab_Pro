#ifndef YOLODECODING_H
#define YOLODECODING_H

#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "core/Detection.h"

namespace visionlab::yolo {

// 解码 YOLOv4-tiny 风格的输出张量（每行：[cx, cy, w, h, objectness, 类别分数...]，
// 坐标为相对输入尺寸的归一化值），输出原始帧像素坐标下的 Detection。
//
// 置信度 = objectness × 最优类别分数；低于 confidenceThreshold 的候选被丢弃，
// 其余经 NMS 合并。类别 id 超出 classNames 范围时回退为 "class_<id>"，
// 避免类别表缺失时越界访问（旧实现存在该崩溃隐患）。
std::vector<Detection> decodeDetections(
    const std::vector<cv::Mat>& outputs,
    const cv::Size& frameSize,
    const std::vector<std::string>& classNames,
    float confidenceThreshold,
    float nmsThreshold);

} // namespace visionlab::yolo

#endif // YOLODECODING_H
