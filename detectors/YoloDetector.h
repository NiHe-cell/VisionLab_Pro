#ifndef YOLODETECTOR_H
#define YOLODETECTOR_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "IDetector.h"
#include "inference/IInferenceEngine.h"
#include "inference/ModelConfig.h"

namespace visionlab {

// YOLOv4-tiny 目标检测：预处理与解码留在检测器，运行时经 IInferenceEngine。
// 不持有 cv::dnn::Net，不在帧上绘制。
class YoloDetector : public IDetector
{
public:
    YoloDetector(std::unique_ptr<IInferenceEngine> engine, ModelConfig config);

    std::string name() const override { return "YOLOv4-tiny"; }
    DetectionMode mode() const override { return DetectionMode::Object; }
    bool isReady() const override;

    std::vector<Detection> detect(const FramePacket& frame) override;

private:
    void loadClassNames(const std::filesystem::path& path);

    std::unique_ptr<IInferenceEngine> m_engine;
    ModelConfig m_config;
    std::vector<std::string> m_classNames;
};

} // namespace visionlab

#endif // YOLODETECTOR_H
