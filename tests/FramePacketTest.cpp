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

    // Value semantics for metadata...
    QCOMPARE(copy.frameId, 42);
    QCOMPARE(copy.sourceId, std::string("camera:0"));
    QVERIFY(copy.captureTimestamp == original.captureTimestamp);

    // ...but the pixel buffer is intentionally shared (documented contract):
    // same data pointer, and a write through one packet is visible via the other.
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
    // Motion regions must not collide with real class indices.
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
