#ifndef OBJECTDETECTOR_H
#define OBJECTDETECTOR_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <QString>

class ObjectDetector : public QObject
{
    Q_OBJECT
public:
    explicit ObjectDetector(QObject *parent = nullptr);
    void detect(cv::Mat& frame);
private:

    cv::dnn::Net net;
    float confidenceThreshold = 0.25f;
    float nmsThreshold = 0.45f;
    int inputSize = 640;

    std::vector<QString> classNames;
    void loadClassNames(const QString& path);
};

#endif // OBJECTDETECTOR_H
