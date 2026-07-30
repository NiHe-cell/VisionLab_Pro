/*
 * Author - Muhammed Suwaneh
*/

#include "MotionDetector.h"
#include <QDebug>

MotionDetector::MotionDetector(QObject *parent)
    : QObject{parent}
{
    this->bgSubtractor = cv::createBackgroundSubtractorMOG2(500, 16, true);
    this->minArea = 500;
}

void MotionDetector::detect(cv::Mat &frame)
{
    qDebug() << "detecting motion ...";

    if(frame.empty()) return;

    this->bgSubtractor->apply(frame, this->fgMask); // apply background subtraction - bg->black | moving area->white

    cv::threshold(this->fgMask, this->fgMask, 200, 255, cv::THRESH_BINARY); // remove shadows

    // reduce noise
    cv::erode(this->fgMask, this->fgMask, cv::Mat(), cv::Point(-1, -1), 1);
    cv::dilate(this->fgMask, this->fgMask, cv::Mat(), cv::Point(-1, -1), 2);

    // find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(this->fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for(const auto& contour: contours)
    {
        if(cv::contourArea(contour) < this->minArea)
            continue;

        // draw rect on original frame
        cv::Rect box = cv::boundingRect(contour);
        cv::rectangle(frame, box, cv::Scalar(0, 255, 255), 2); // yellow box

        // Label motion
        cv::putText(frame, "In Motion", cv::Point(box.x, box.y-5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 2);
    }
}
