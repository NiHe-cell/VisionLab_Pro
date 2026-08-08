#include <QtTest/QtTest>

#include <thread>
#include <utility>

#include "core/Detection.h"
#include "core/FramePacket.h"
#include "core/VisionTypes.h"

using visionlab::Detection;
using visionlab::DetectionMode;
using visionlab::FramePacket;

class FramePacketTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreEmpty();
    void copySharesPixelBuffer();
    void moveTransfersPixels();
    void motionClassIdIsReserved();
    void detectionDefaults();
};

void FramePacketTest::defaultsAreEmpty()
{
    const FramePacket packet;
    QCOMPARE(packet.frameId, 0);
    QVERIFY(packet.sourceId.empty());
    QVERIFY(packet.image.empty());
}

void FramePacketTest::copySharesPixelBuffer()
{
    FramePacket original;
    original.frameId = 42;
    original.sourceId = "camera:0";
    original.captureTimestamp = std::chrono::steady_clock::now();
    original.image = cv::Mat(4, 4, CV_8UC3, cv::Scalar(1, 2, 3));

    const FramePacket copy = original;

    // 元数据遵循值语义……
    QCOMPARE(copy.frameId, 42);
    QCOMPARE(copy.sourceId, std::string("camera:0"));
    QVERIFY(copy.captureTimestamp == original.captureTimestamp);

    // ……但像素缓冲按契约共享：数据指针相同，
    // 且通过一个副本写入能被另一个副本观察到。
    QCOMPARE(copy.image.data, original.image.data);
    original.image.at<cv::Vec3b>(0, 0) = cv::Vec3b(7, 8, 9);
    QCOMPARE(copy.image.at<cv::Vec3b>(0, 0), cv::Vec3b(7, 8, 9));
}

void FramePacketTest::moveTransfersPixels()
{
    FramePacket source;
    source.frameId = 7;
    source.image = cv::Mat(2, 2, CV_8UC1, cv::Scalar(9));
    const uchar* originalData = source.image.data;

    const FramePacket moved = std::move(source);

    QCOMPARE(moved.frameId, 7);
    QCOMPARE(moved.image.data, originalData);
}

void FramePacketTest::motionClassIdIsReserved()
{
    // 运动区域的类别 id 不得与真实类别索引冲突。
    QVERIFY(visionlab::kMotionClassId < 0);
    QVERIFY(DetectionMode::Motion != DetectionMode::Object);
}

void FramePacketTest::detectionDefaults()
{
    const Detection detection;
    QCOMPARE(detection.classId, -1);
    QVERIFY(detection.label.empty());
    QCOMPARE(detection.confidence, 0.0F);
    QVERIFY(detection.box.empty());
}

QTEST_APPLESS_MAIN(FramePacketTest)

#include "FramePacketTest.moc"
