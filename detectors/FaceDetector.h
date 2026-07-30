#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

class FaceDetector
{
public:
    explicit FaceDetector();
    void detect(cv::Mat& frame);
private:
    cv::dnn::Net net;
    float confidenceThreshold = 0.6f;
};

#endif // FACEDETECTOR_H
