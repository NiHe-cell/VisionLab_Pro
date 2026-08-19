#ifndef YOLOPREPROCESS_H
#define YOLOPREPROCESS_H

#include <opencv2/core.hpp>

#include "inference/InferenceTypes.h"

namespace visionlab::yolo {

// 与现有 YOLOv4-tiny OpenCV DNN 路径对齐：stretch（非 letterbox）到
// inputWidth×inputHeight、BGR→RGB、/255、HWC→NCHW。
// 解码侧仍用「归一化坐标 × 原图尺寸」，见 YoloDecoding。
struct YoloPreprocessResult
{
    TensorView input;
    int originalWidth = 0;
    int originalHeight = 0;
    int inputWidth = 0;
    int inputHeight = 0;
};

YoloPreprocessResult preprocessYoloV4Tiny(const cv::Mat& bgr, int inW, int inH);

} // namespace visionlab::yolo

#endif // YOLOPREPROCESS_H
