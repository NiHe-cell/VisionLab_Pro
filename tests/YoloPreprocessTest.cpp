#include <QtTest/QtTest>

#include "detectors/YoloPreprocess.h"

using visionlab::yolo::YoloPreprocessResult;
using visionlab::yolo::preprocessYoloV4Tiny;

class YoloPreprocessTest : public QObject
{
    Q_OBJECT

private slots:
    void stretchProducesNchwShapeAndUnitRange();
    void bgrRedLandsInRedChannel();
    void emptyImageIsSafe();
    void recordsOriginalSize();
};

void YoloPreprocessTest::stretchProducesNchwShapeAndUnitRange()
{
    const cv::Mat bgr(2, 2, CV_8UC3, cv::Scalar(10, 20, 30));
    const YoloPreprocessResult result = preprocessYoloV4Tiny(bgr, 4, 4);

    const std::vector<std::int64_t> expected{1, 3, 4, 4};
    QCOMPARE(result.input.shape, expected);
    QCOMPARE(result.input.data.size(), size_t{1 * 3 * 4 * 4});
    for (float value : result.input.data)
    {
        QVERIFY(value >= 0.0F);
        QVERIFY(value <= 1.0F);
    }
}

void YoloPreprocessTest::bgrRedLandsInRedChannel()
{
    // OpenCV BGR：纯红是 (0, 0, 255)。swapRB 之后应出现在 NCHW 的 R 平面。
    const cv::Mat bgr(1, 1, CV_8UC3, cv::Scalar(0, 0, 255));
    const YoloPreprocessResult result = preprocessYoloV4Tiny(bgr, 2, 2);

    QCOMPARE(result.input.data.size(), size_t{12});
    const float* data = result.input.data.data();
    const int plane = 2 * 2;
    for (int i = 0; i < plane; ++i)
        QCOMPARE(data[i], 1.0F); // R
    for (int i = plane; i < 3 * plane; ++i)
        QCOMPARE(data[i], 0.0F); // G then B
}

void YoloPreprocessTest::emptyImageIsSafe()
{
    const YoloPreprocessResult result = preprocessYoloV4Tiny(cv::Mat{}, 320, 320);
    QVERIFY(result.input.data.empty());
    QVERIFY(result.input.shape.empty());
    QCOMPARE(result.originalWidth, 0);
    QCOMPARE(result.originalHeight, 0);
}

void YoloPreprocessTest::recordsOriginalSize()
{
    const cv::Mat bgr(40, 80, CV_8UC3, cv::Scalar(1, 2, 3));
    const YoloPreprocessResult result = preprocessYoloV4Tiny(bgr, 320, 320);
    QCOMPARE(result.originalWidth, 80);
    QCOMPARE(result.originalHeight, 40);
    QCOMPARE(result.inputWidth, 320);
    QCOMPARE(result.inputHeight, 320);
}

QTEST_APPLESS_MAIN(YoloPreprocessTest)

#include "YoloPreprocessTest.moc"
