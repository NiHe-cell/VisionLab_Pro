#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

#include <string>
#include <vector>

#include <opencv2/dnn.hpp>

#include "IDetector.h"

namespace visionlab {

// 基于 OpenCV DNN 的 Res10-SSD (Caffe) 人脸检测器。
// 纯 C++ 实现：不依赖 Qt，不在帧上绘制；模型路径由构造注入。
class FaceDetector : public IDetector
{
public:
    // 两个路径分别指向 deploy.prototxt / .caffemodel；
    // 任一缺失或加载失败时 isReady() 为 false，detect 返回空结果。
    FaceDetector(const std::string& protoPath, const std::string& modelPath);

    std::string name() const override { return "Res10-SSD Face"; }
    DetectionMode mode() const override { return DetectionMode::Face; }
    bool isReady() const override { return m_ready; }

    std::vector<Detection> detect(const FramePacket& frame) override;

private:
    cv::dnn::Net m_net;
    bool m_ready = false;
    float m_confidenceThreshold = 0.6F;
    cv::Size m_inputSize{300, 300};
};

} // namespace visionlab

#endif // FACEDETECTOR_H
