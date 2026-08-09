#include "ObjectDetector.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "YoloDecoding.h"

namespace visionlab {

ObjectDetector::ObjectDetector(const std::string& configPath,
                               const std::string& weightsPath,
                               const std::string& classNamesPath)
{
    namespace fs = std::filesystem;

    if (!fs::exists(configPath) || !fs::exists(weightsPath))
    {
        std::cerr << "[ObjectDetector] 模型文件缺失: " << configPath
                  << " / " << weightsPath << '\n';
        return;
    }

    try
    {
        m_net = cv::dnn::readNetFromDarknet(configPath, weightsPath);
    }
    catch (const cv::Exception& e)
    {
        std::cerr << "[ObjectDetector] 加载 YOLOv4-tiny 失败: " << e.what() << '\n';
        return;
    }

    if (m_net.empty())
    {
        std::cerr << "[ObjectDetector] 加载 YOLOv4-tiny 失败: 网络为空\n";
        return;
    }

    m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    loadClassNames(classNamesPath);
    m_ready = true;
}

std::vector<Detection> ObjectDetector::detect(const FramePacket& frame)
{
    if (!m_ready || frame.image.empty())
        return {};

    const cv::Mat blob = cv::dnn::blobFromImage(
        frame.image,
        1.0 / 255.0,
        m_inputSize,
        cv::Scalar(),
        /*swapRB=*/true,
        /*crop=*/false);

    m_net.setInput(blob);

    std::vector<cv::Mat> outputs;
    m_net.forward(outputs, m_net.getUnconnectedOutLayersNames());

    return yolo::decodeDetections(
        outputs,
        frame.image.size(),
        m_classNames,
        m_confidenceThreshold,
        m_nmsThreshold);
}

void ObjectDetector::loadClassNames(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cerr << "[ObjectDetector] 无法打开类别名文件: " << path << '\n';
        return;
    }

    std::string line;
    while (std::getline(file, line))
        m_classNames.push_back(line);
}

} // namespace visionlab
