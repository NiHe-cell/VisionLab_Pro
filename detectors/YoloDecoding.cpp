#include "YoloDecoding.h"

#include <opencv2/dnn.hpp>

namespace visionlab::yolo {

std::vector<Detection> decodeDetections(
    const std::vector<cv::Mat>& outputs,
    const cv::Size& frameSize,
    const std::vector<std::string>& classNames,
    float confidenceThreshold,
    float nmsThreshold)
{
    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    for (const cv::Mat& out : outputs)
    {
        const auto* data = reinterpret_cast<const float*>(out.data);

        for (int i = 0; i < out.rows; ++i, data += out.cols)
        {
            const float objectness = data[4];
            if (objectness < confidenceThreshold)
                continue;

            // 在类别分数段中找最优类别。
            const cv::Mat scores(1, out.cols - 5, CV_32F, const_cast<float*>(data + 5));
            cv::Point classId;
            double classScore = 0.0;
            cv::minMaxLoc(scores, nullptr, &classScore, nullptr, &classId);

            const float confidence = objectness * static_cast<float>(classScore);
            if (confidence < confidenceThreshold)
                continue;

            const int cx = static_cast<int>(data[0] * frameSize.width);
            const int cy = static_cast<int>(data[1] * frameSize.height);
            const int w  = static_cast<int>(data[2] * frameSize.width);
            const int h  = static_cast<int>(data[3] * frameSize.height);

            // 与图像范围求交，保证 Detection 是良好形成的矩形。
            const cv::Rect box = cv::Rect(cx - w / 2, cy - h / 2, w, h)
                                 & cv::Rect(0, 0, frameSize.width, frameSize.height);
            if (box.empty())
                continue;

            boxes.push_back(box);
            confidences.push_back(confidence);
            classIds.push_back(classId.x);
        }
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, confidenceThreshold, nmsThreshold, indices);

    std::vector<Detection> detections;
    detections.reserve(indices.size());
    for (const int idx : indices)
    {
        Detection detection;
        detection.classId = classIds[idx];
        detection.confidence = confidences[idx];
        detection.box = boxes[idx];
        const size_t id = static_cast<size_t>(detection.classId);
        detection.label = id < classNames.size()
                              ? classNames[id]
                              : "class_" + std::to_string(detection.classId);
        detections.push_back(std::move(detection));
    }
    return detections;
}

} // namespace visionlab::yolo
