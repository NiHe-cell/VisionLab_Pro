#ifndef SSDDECODING_H
#define SSDDECODING_H

#include <vector>

#include <opencv2/core.hpp>

#include "core/Detection.h"

namespace visionlab::ssd {

// 解码 Res10-SSD (Caffe) 人脸检测的 forward 输出。
// 输出张量形状为 [1, 1, N, 7]，每条记录：
// [保留, 保留, confidence, x1, y1, x2, y2]，坐标为归一化值。
// 返回像素坐标下的 Detection（label="Face"，classId=0），
// 低于 confidenceThreshold 的记录被丢弃，包围框与图像范围求交。
std::vector<Detection> decodeDetections(const cv::Mat& output,
                                        const cv::Size& frameSize,
                                        float confidenceThreshold);

} // namespace visionlab::ssd

#endif // SSDDECODING_H
