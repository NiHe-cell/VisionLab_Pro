#include "SsdDecoding.h"

namespace visionlab::ssd {

namespace {

// 人脸检测没有类别语义，统一使用 0 并配上 "Face" 标签。
constexpr int kFaceClassId = 0;

} // namespace

std::vector<Detection> decodeDetections(const cv::Mat& output,
                                        const cv::Size& frameSize,
                                        float confidenceThreshold)
{
    std::vector<Detection> detections;

    // 期望 [1, 1, N, 7]；形状不符时安全返回空结果。
    if (output.dims != 4 || output.size[2] <= 0 || output.size[3] < 7)
        return detections;

    const cv::Rect frameRect(0, 0, frameSize.width, frameSize.height);
    const int count = output.size[2];

    for (int i = 0; i < count; ++i)
    {
        const float* data = output.ptr<float>(0, 0, i);
        const float confidence = data[2];
        if (confidence < confidenceThreshold)
            continue;

        const int x1 = static_cast<int>(data[3] * frameSize.width);
        const int y1 = static_cast<int>(data[4] * frameSize.height);
        const int x2 = static_cast<int>(data[5] * frameSize.width);
        const int y2 = static_cast<int>(data[6] * frameSize.height);

        const cv::Rect box = cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)) & frameRect;
        if (box.empty())
            continue;

        Detection detection;
        detection.classId = kFaceClassId;
        detection.label = "Face";
        detection.confidence = confidence;
        detection.box = box;
        detections.push_back(detection);
    }

    return detections;
}

} // namespace visionlab::ssd
