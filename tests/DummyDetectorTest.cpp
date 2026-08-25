#include <QtTest/QtTest>

#include <cstring>

#include "detectors/DummyDetector.h"

using visionlab::DetectionMode;
using visionlab::DummyDetector;
using visionlab::FramePacket;

namespace {

FramePacket makePacket(const cv::Mat& image)
{
    FramePacket packet;
    packet.frameId = 1;
    packet.sourceId = "test:0";
    packet.captureTimestamp = std::chrono::steady_clock::now();
    packet.image = image;
    return packet;
}

} // namespace

class DummyDetectorTest : public QObject
{
    Q_OBJECT

private slots:
    void identityIsDummyAndAlwaysReady();
    void emptyFrameReturnsEmpty();
    void nonEmptyFrameReturnsFixedBox();
    void detectIsDeterministic();
    void detectDoesNotMutatePixels();
};

void DummyDetectorTest::identityIsDummyAndAlwaysReady()
{
    DummyDetector detector;
    QCOMPARE(detector.name(), std::string("Dummy"));
    QVERIFY(detector.mode() == DetectionMode::None);
    QVERIFY(detector.isReady());
}

void DummyDetectorTest::emptyFrameReturnsEmpty()
{
    DummyDetector detector;
    QVERIFY(detector.detect(FramePacket{}).empty());
    QVERIFY(detector.isReady());
}

void DummyDetectorTest::nonEmptyFrameReturnsFixedBox()
{
    DummyDetector detector;
    const auto detections = detector.detect(makePacket(cv::Mat(8, 8, CV_8UC3, cv::Scalar(0, 0, 0))));
    QCOMPARE(detections.size(), size_t{1});
    QCOMPARE(detections.front().classId, 0);
    QCOMPARE(detections.front().label, std::string("dummy"));
    QCOMPARE(detections.front().confidence, 1.0F);
    QCOMPARE(detections.front().box, cv::Rect(10, 10, 20, 20));
}

void DummyDetectorTest::detectIsDeterministic()
{
    DummyDetector detector;
    const auto frame = makePacket(cv::Mat(8, 8, CV_8UC3, cv::Scalar(7, 8, 9)));
    const auto first = detector.detect(frame);
    const auto second = detector.detect(frame);
    QCOMPARE(first.size(), second.size());
    QCOMPARE(first.front().classId, second.front().classId);
    QCOMPARE(first.front().label, second.front().label);
    QCOMPARE(first.front().confidence, second.front().confidence);
    QCOMPARE(first.front().box, second.front().box);
}

void DummyDetectorTest::detectDoesNotMutatePixels()
{
    DummyDetector detector;
    FramePacket packet = makePacket(cv::Mat(8, 8, CV_8UC3, cv::Scalar(3, 4, 5)));
    const cv::Mat before = packet.image.clone();
    const uchar* dataBefore = packet.image.data;
    detector.detect(packet);
    QCOMPARE(packet.image.data, dataBefore);
    QCOMPARE(std::memcmp(packet.image.data, before.data, before.total() * before.elemSize()), 0);
}

QTEST_APPLESS_MAIN(DummyDetectorTest)

#include "DummyDetectorTest.moc"
