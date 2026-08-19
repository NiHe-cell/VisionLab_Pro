#include "YoloDetector.h"

#include <fstream>

#include "YoloDecoding.h"
#include "YoloPreprocess.h"

namespace visionlab {

namespace {

cv::Mat asYoloRows(const TensorView& tensor)
{
    std::int64_t expected = 1;
    for (const std::int64_t dim : tensor.shape)
        expected *= dim;
    if (expected <= 0 || tensor.data.size() != static_cast<size_t>(expected))
        return {};

    int rows = 0;
    int cols = 0;
    if (tensor.shape.size() == 2)
    {
        rows = static_cast<int>(tensor.shape[0]);
        cols = static_cast<int>(tensor.shape[1]);
    }
    else if (tensor.shape.size() == 3 && tensor.shape[0] == 1)
    {
        rows = static_cast<int>(tensor.shape[1]);
        cols = static_cast<int>(tensor.shape[2]);
    }
    else
    {
        return {};
    }

    if (rows <= 0 || cols <= 0)
        return {};

    return cv::Mat(rows, cols, CV_32F, const_cast<float*>(tensor.data.data()));
}

} // namespace

YoloDetector::YoloDetector(std::unique_ptr<IInferenceEngine> engine, ModelConfig config)
    : m_engine(std::move(engine))
    , m_config(std::move(config))
{
    loadClassNames(m_config.classNamesPath);
    if (m_engine)
        m_engine->initialize(m_config);
}

bool YoloDetector::isReady() const
{
    return m_engine && m_engine->isReady();
}

std::vector<Detection> YoloDetector::detect(const FramePacket& frame)
{
    if (!isReady() || frame.image.empty())
        return {};

    const auto preprocessed = yolo::preprocessYoloV4Tiny(
        frame.image, m_config.inputWidth, m_config.inputHeight);
    if (preprocessed.input.data.empty())
        return {};

    const InferResult inferred = m_engine->infer(preprocessed.input);
    if (!inferred.ok)
        return {};

    std::vector<cv::Mat> outputs;
    outputs.reserve(inferred.outputs.size());
    for (const TensorView& tensor : inferred.outputs)
    {
        const cv::Mat rows = asYoloRows(tensor);
        if (rows.empty())
            return {};
        outputs.push_back(rows);
    }

    return yolo::decodeDetections(
        outputs,
        frame.image.size(),
        m_classNames,
        m_config.confidenceThreshold,
        m_config.nmsThreshold);
}

void YoloDetector::loadClassNames(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file)
        return;

    std::string line;
    while (std::getline(file, line))
        m_classNames.push_back(line);
}

} // namespace visionlab
