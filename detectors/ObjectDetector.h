#ifndef OBJECTDETECTOR_H
#define OBJECTDETECTOR_H

#include <string>
#include <vector>

#include <opencv2/dnn.hpp>

#include "IDetector.h"

namespace visionlab {

// 基于 OpenCV DNN 的 YOLOv4-tiny 目标检测器（COCO, 80 类）。
// 纯 C++ 实现：不依赖 Qt，不在帧上绘制；模型路径由构造注入。
class ObjectDetector : public IDetector
{
public:
    // 三个路径分别指向 .cfg / .weights / 类别名文件；
    // 任一缺失或加载失败时 isReady() 为 false，detect 返回空结果。
    ObjectDetector(const std::string& configPath,
                   const std::string& weightsPath,
                   const std::string& classNamesPath);

    std::string name() const override { return "YOLOv4-tiny"; }
    DetectionMode mode() const override { return DetectionMode::Object; }
    bool isReady() const override { return m_ready; }

    std::vector<Detection> detect(const FramePacket& frame) override;

private:
    void loadClassNames(const std::string& path);

    cv::dnn::Net m_net;
    bool m_ready = false;
    float m_confidenceThreshold = 0.25F;
    float m_nmsThreshold = 0.45F;
    cv::Size m_inputSize{320, 320};
    std::vector<std::string> m_classNames;
};

} // namespace visionlab

#endif // OBJECTDETECTOR_H
