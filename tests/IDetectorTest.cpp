#include <QtTest/QtTest>

#include <memory>

#include "detectors/IDetector.h"
#include "fakes/FakeDetector.h"

using visionlab::DetectionMode;
using visionlab::FramePacket;
using visionlab::IDetector;

namespace {

FramePacket makePacket()
{
    FramePacket packet;
    packet.frameId = 1;
    packet.sourceId = "fake:0";
    packet.captureTimestamp = std::chrono::steady_clock::now();
    packet.image = cv::Mat(8, 8, CV_8UC3, cv::Scalar(0, 0, 0));
    return packet;
}

} // namespace

class IDetectorTest : public QObject
{
    Q_OBJECT

private slots:
    void polymorphicDispatch();
    void notReadyReturnsEmpty();
    void emptyFrameReturnsEmpty();
};

void IDetectorTest::polymorphicDispatch()
{
    std::unique_ptr<IDetector> detector = std::make_unique<FakeDetector>();

    QCOMPARE(detector->name(), std::string("fake"));
    QVERIFY(detector->mode() == DetectionMode::Object);
    QVERIFY(detector->isReady());

    const auto results = detector->detect(makePacket());
    QCOMPARE(results.size(), size_t{1});
    QCOMPARE(results.front().label, std::string("fake-object"));
    QCOMPARE(results.front().box, cv::Rect(1, 2, 3, 4));
}

void IDetectorTest::notReadyReturnsEmpty()
{
    FakeDetector detector;
    detector.setReady(false);

    QVERIFY(detector.detect(makePacket()).empty());
    QCOMPARE(detector.callCount(), 1); // 调用被记录，但结果为安全空集
}

void IDetectorTest::emptyFrameReturnsEmpty()
{
    FakeDetector detector;
    QVERIFY(detector.detect(FramePacket{}).empty());
}

QTEST_APPLESS_MAIN(IDetectorTest)

#include "IDetectorTest.moc"
