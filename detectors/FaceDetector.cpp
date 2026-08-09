#include "FaceDetector.h"

#include <filesystem>
#include <iostream>

#include "SsdDecoding.h"

namespace visionlab {

FaceDetector::FaceDetector(const std::string& protoPath, const std::string& modelPath)
{
    namespace fs = std::filesystem;

    if (!fs::exists(protoPath) || !fs::exists(modelPath))
    {
        std::cerr << "[FaceDetector] 模型文件缺失: " << protoPath
                  << " / " << modelPath << '\n';
        return;
    }

    try
    {
        m_net = cv::dnn::readNetFromCaffe(protoPath, modelPath);
    }
    catch (const cv::Exception& e)
    {
        std::cerr << "[FaceDetector] 加载 Caffe 人脸模型失败: " << e.what() << '\n';
        return;
    }

    if (m_net.empty())
    {
        std::cerr << "[FaceDetector] 加载 Caffe 人脸模型失败: 网络为空\n";
        return;
    }

    m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    m_ready = true;
}

std::vector<Detection> FaceDetector::detect(const FramePacket& frame)
{
    if (!m_ready || frame.image.empty())
        return {};

    const cv::Mat blob = cv::dnn::blobFromImage(
        frame.image,
        1.0,
        m_inputSize,
        cv::Scalar(104, 177, 123),
        /*swapRB=*/false,
        /*crop=*/false);

    m_net.setInput(blob);
    const cv::Mat output = m_net.forward();

    return ssd::decodeDetections(output, frame.image.size(), m_confidenceThreshold);
}

} // namespace visionlab
