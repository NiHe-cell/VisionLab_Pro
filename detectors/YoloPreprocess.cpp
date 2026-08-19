#include "YoloPreprocess.h"

#include <opencv2/dnn.hpp>

namespace visionlab::yolo {

YoloPreprocessResult preprocessYoloV4Tiny(const cv::Mat& bgr, int inW, int inH)
{
    YoloPreprocessResult result;
    if (bgr.empty() || inW <= 0 || inH <= 0)
        return result;

    result.originalWidth = bgr.cols;
    result.originalHeight = bgr.rows;
    result.inputWidth = inW;
    result.inputHeight = inH;

    const cv::Mat blob = cv::dnn::blobFromImage(
        bgr,
        1.0 / 255.0,
        cv::Size(inW, inH),
        cv::Scalar(),
        /*swapRB=*/true,
        /*crop=*/false);

    result.input.shape = {1, 3, inH, inW};
    const auto* begin = blob.ptr<float>();
    result.input.data.assign(begin, begin + blob.total());
    return result;
}

} // namespace visionlab::yolo
