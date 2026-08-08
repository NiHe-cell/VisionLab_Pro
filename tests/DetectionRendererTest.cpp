#include <QtTest/QtTest>

#include <opencv2/imgproc.hpp>

#include "core/VisionTypes.h"
#include "rendering/DetectionRenderer.h"

using visionlab::Detection;
using visionlab::DetectionRenderer;

namespace {

Detection makeDetection(int classId, const std::string& label, const cv::Rect& box)
{
    Detection d;
    d.classId = classId;
    d.label = label;
    d.confidence = 0.8F;
    d.box = box;
    return d;
}

} // namespace

class DetectionRendererTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyListIsNoOp();
    void emptyFrameIsNoOp();
    void drawsBoxPixels();
    void motionUsesYellow();
    void clipsOutOfBoundsBox();
    void keepsFrameSize();
};

void DetectionRendererTest::emptyListIsNoOp()
{
    cv::Mat frame(20, 20, CV_8UC3, cv::Scalar(10, 10, 10));
    const cv::Mat before = frame.clone();

    DetectionRenderer().render(frame, {});

    QCOMPARE(cv::countNonZero(frame.reshape(1) != before.reshape(1)), 0);
}

void DetectionRendererTest::emptyFrameIsNoOp()
{
    cv::Mat frame;
    DetectionRenderer().render(frame, {makeDetection(1, "x", {0, 0, 5, 5})});
    QVERIFY(frame.empty());
}

void DetectionRendererTest::drawsBoxPixels()
{
    cv::Mat frame(50, 50, CV_8UC3, cv::Scalar(0, 0, 0));

    DetectionRenderer().render(frame, {makeDetection(0, "person", {10, 10, 20, 20})});

    // 框的左上角应被绘制成绿色（BGR）。
    QCOMPARE(frame.at<cv::Vec3b>(10, 10), cv::Vec3b(0, 255, 0));
}

void DetectionRendererTest::motionUsesYellow()
{
    cv::Mat frame(50, 50, CV_8UC3, cv::Scalar(0, 0, 0));

    Detection motion;
    motion.classId = visionlab::kMotionClassId;
    motion.label = "In Motion";
    motion.confidence = 1.0F;
    motion.box = cv::Rect(10, 10, 20, 20);

    DetectionRenderer().render(frame, {motion});

    // 运动区域沿用黄色（BGR: 0,255,255）。
    QCOMPARE(frame.at<cv::Vec3b>(10, 10), cv::Vec3b(0, 255, 255));
}

void DetectionRendererTest::clipsOutOfBoundsBox()
{
    cv::Mat frame(50, 50, CV_8UC3, cv::Scalar(0, 0, 0));

    // 部分越界的框 + 顶部标签：不得崩溃，图像尺寸不变。
    DetectionRenderer().render(frame, {makeDetection(1, "edge", {-5, 0, 20, 20})});

    QCOMPARE(frame.cols, 50);
    QCOMPARE(frame.rows, 50);
}

void DetectionRendererTest::keepsFrameSize()
{
    cv::Mat frame(60, 80, CV_8UC3, cv::Scalar(0, 0, 0));

    DetectionRenderer().render(frame,
                               {makeDetection(1, "a", {0, 0, 10, 10}),
                                makeDetection(2, "b", {30, 30, 10, 10})});

    QCOMPARE(frame.cols, 80);
    QCOMPARE(frame.rows, 60);
}

QTEST_APPLESS_MAIN(DetectionRendererTest)

#include "DetectionRendererTest.moc"
