#include <QtTest/QtTest>

#include <memory>

#include "detectors/IDetector.h"

using visionlab::Detection;
using visionlab::DetectionMode;
using visionlab::FramePacket;
using visionlab::IDetector;

namespace {

// 确定性的假检测器：证明接口可被实现、可多态替换、可有内部状态。
class FakeDetector : public IDetector
{
public:
    std::string name() const override { return "fake"; }
    DetectionMode mode() const override { return DetectionMode::Object; }
    bool isReady() const override { return m_ready; }

    std::vector<Detection> detect(const FramePacket& frame) override
    {
        ++m_callCount; // 有状态：证明 detect 允许修改实现内部状态
        if (!m_ready || frame.image.empty())
            return {};

        Detection d;
        d.classId = 1;
        d.label = "fake-object";
        d.confidence = 0.9F;
        d.box = cv::Rect(1, 2, 3, 4);
        return {d};
    }

    void setReady(bool ready) { m_ready = ready; }
    int callCount() const { return m_callCount; }

private:
    bool m_ready = true;
    int m_callCount = 0;
};

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
