#ifndef MOTIONDETECTOR_H
#define MOTIONDETECTOR_H

#include <QObject>
#include <opencv2/opencv.hpp>

class MotionDetector : public QObject
{
    Q_OBJECT
public:
    explicit MotionDetector(QObject *parent = nullptr);
    void detect(cv::Mat& frame);

private:
    cv::Ptr<cv::BackgroundSubtractor> bgSubtractor;
    cv::Mat fgMask;

    int minArea;
};

#endif // MOTIONDETECTOR_H

