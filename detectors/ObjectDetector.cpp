#include "ObjectDetector.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

ObjectDetector::ObjectDetector(QObject *parent) : QObject(parent)
{
    QDir appDir(QCoreApplication::applicationDirPath());

    QString cfgPath     = appDir.filePath("VisionLab/models/yolov4-tiny.cfg");
    QString weightsPath = appDir.filePath("VisionLab/models/yolov4-tiny.weights");
    QString namesPath   = appDir.filePath("VisionLab/models/coco.names");

    if (!QFile::exists(cfgPath) || !QFile::exists(weightsPath))
    {
        qDebug() << "YOLOv4-tiny files NOT found";
        return;
    }

    net = cv::dnn::readNetFromDarknet(
        cfgPath.toStdString(),
        weightsPath.toStdString()
        );

    if (net.empty())
    {
        qDebug() << "Failed to load YOLOv4-tiny";
        return;
    }

    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    loadClassNames(namesPath);

    qDebug() << "YOLOv4-tiny COCO loaded successfully";
}


void ObjectDetector::detect(cv::Mat& frame)
{
    qDebug() << "detecting objects ...";

    if (frame.empty())
        return;

    static int skip = 0;
    if (++skip % 3 != 0) // skip frames for speed
        return;

    cv::Mat blob = cv::dnn::blobFromImage(
        frame,
        1.0 / 255.0,
        cv::Size(320, 320),
        cv::Scalar(),
        true,
        false
        );

    net.setInput(blob);

    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    for (const cv::Mat& out : outputs)
    {
        const float* data = (float*)out.data;

        for (int i = 0; i < out.rows; ++i, data += out.cols)
        {
            float objectness = data[4];
            if (objectness < 0.25f)
                continue;

            cv::Mat scores(1, out.cols - 5, CV_32F, (void*)(data + 5));
            cv::Point classId;
            double classScore;
            cv::minMaxLoc(scores, 0, &classScore, 0, &classId);

            float conf = objectness * (float)classScore;
            if (conf < 0.25f)
                continue;

            int cx = int(data[0] * frame.cols);
            int cy = int(data[1] * frame.rows);
            int w  = int(data[2] * frame.cols);
            int h  = int(data[3] * frame.rows);

            boxes.emplace_back(cx - w/2, cy - h/2, w, h);
            confidences.push_back(conf);
            classIds.push_back(classId.x);
        }
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, 0.25f, 0.45f, indices);

    for (int idx : indices)
    {
        cv::rectangle(frame, boxes[idx], {0,255,0}, 2);
        cv::putText(
            frame,
            classNames[classIds[idx]].toStdString(),
            {boxes[idx].x, boxes[idx].y - 5},
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            {0,255,0},
            2
            );
    }
}


void ObjectDetector::loadClassNames(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open coco.names";
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd())
        classNames.push_back(in.readLine());
}
