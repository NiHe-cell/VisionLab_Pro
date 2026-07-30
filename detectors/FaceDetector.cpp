/*
 * Author - Muhammed Suwaneh
*/

#include "FaceDetector.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QMessageBox>

FaceDetector::FaceDetector()
{
    try
    {
        QDir appDir(QCoreApplication::applicationDirPath());

        // get face detection models
        QString protoPath = appDir.filePath("VisionLab/models/deploy.prototxt");
        QString modelPath = appDir.filePath("VisionLab/models/res10_300x300_ssd_iter_140000.caffemodel");

        if (!QFileInfo::exists(protoPath) || !QFileInfo::exists(modelPath)) {
            throw("Face detection files NOT FOUND");
        }

        net = cv::dnn::readNetFromCaffe(
            protoPath.toStdString(),
            modelPath.toStdString()
            );

        if (net.empty()) {
            throw("Failed to load Caffe face detector");
        }

        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    }
    catch(QString msg)
    {
        // log errors
        qDebug() << msg;
    }
}


void FaceDetector::detect(cv::Mat &frame)
{
    qDebug() << "detecting faces";

    if(frame.empty()) return;

    // convert frame into blob
    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0, cv::Size(300, 300), cv::Scalar(104, 177, 123), false, false);

    net.setInput(blob);

    cv::Mat detections = net.forward();

    // get current frame height and width
    const int h = frame.rows;
    const int w = frame.cols;

    for(int iter = 0; iter < detections.size[2]; iter++)
    {
        const float* data = detections.ptr<float>(0, 0, iter);

        float confidence = data[2];

        if(confidence > confidenceThreshold)
        {
            int x1 = static_cast<int>(data[3] * w);
            int y1 = static_cast<int>(data[4] * h);
            int x2 = static_cast<int>(data[5] * w);
            int y2 = static_cast<int>(data[6] * h);

            cv::rectangle(frame, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 0), 2); // draw green boxes around detections

            // draw label on face
            cv::putText(frame, "Face", cv::Point(x1, y1 - 10), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        }
    }
}
