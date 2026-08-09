#include <QtTest/QtTest>

#include <opencv2/imgproc.hpp>

#include "core/VisionTypes.h"
#include "detectors/MotionDetector.h"

using visionlab::FramePacket;
using visionlab::MotionDetector;

namespace {

FramePacket makePacket(const cv::Mat& image)
{
    FramePacket packet;
    packet.image = image;
    packet.captureTimestamp = std::chrono::steady_clock::now();
    packet.sourceId = "test:0";
    return packet;
}

// 纯黑背景帧。
cv::Mat backgroundFrame()
{
    return cv::Mat(240, 320, CV_8UC3, cv::Scalar(0, 0, 0));
}

} // namespace

class MotionDetectorTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyFrameReturnsEmpty();
    void staticSceneSettlesToNoMotion();
    void movingBlockProducesDetection();
};

void MotionDetectorTest::emptyFrameReturnsEmpty()
{
    MotionDetector detector;
    QVERIFY(detector.detect(FramePacket{}).empty());
}

void MotionDetectorTest::staticSceneSettlesToNoMotion()
{
    MotionDetector detector;

    // 连续喂入静止背景，背景模型收敛后不应再有运动输出。
    int lastCount = -1;
    for (int i = 0; i < 40; ++i)
        lastCount = static_cast<int>(detector.detect(makePacket(backgroundFrame())).size());

    QCOMPARE(lastCount, 0);
}

void MotionDetectorTest::movingBlockProducesDetection()
{
    MotionDetector detector;

    // 先让背景模型学习静止场景。
    for (int i = 0; i < 20; ++i)
        detector.detect(makePacket(backgroundFrame()));

    // 大块白色区域进入画面 → 应产生运动 Detection。
    cv::Mat frame = backgroundFrame();
    const cv::Rect block(100, 80, 60, 60);
    cv::rectangle(frame, block, cv::Scalar(255, 255, 255), cv::FILLED);

    const auto detections = detector.detect(makePacket(frame));

    QVERIFY(!detections.empty());

    bool found = false;
    for (const auto& d : detections)
    {
        QCOMPARE(d.classId, visionlab::kMotionClassId);
        QCOMPARE(d.label, std::string("In Motion"));
        // 包围盒应与运动块明显重叠（形态学操作允许少量膨胀/收缩）。
        const double overlap = static_cast<double>((d.box & block).area());
        if (overlap > 0.5 * block.area())
            found = true;
    }
    QVERIFY(found);
}

QTEST_APPLESS_MAIN(MotionDetectorTest)

#include "MotionDetectorTest.moc"
