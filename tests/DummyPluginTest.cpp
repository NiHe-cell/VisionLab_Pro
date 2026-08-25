#include <QtTest/QtTest>

#include <algorithm>

#include <QPluginLoader>

#include "plugin/IVisionPlugin.h"

using visionlab::DetectionMode;
using visionlab::DetectorCreateRequest;
using visionlab::IVisionPlugin;

namespace {

visionlab::FramePacket makePacket()
{
    visionlab::FramePacket packet;
    packet.image = cv::Mat(8, 8, CV_8UC3, cv::Scalar(0, 0, 0));
    return packet;
}

} // namespace

class DummyPluginTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsAndCreatesDummyDetector();
};

void DummyPluginTest::loadsAndCreatesDummyDetector()
{
    QPluginLoader loader(QString::fromUtf8(VISIONLAB_DUMMY_PLUGIN));
    QVERIFY2(loader.load(), qPrintable(loader.errorString()));

    auto* plugin = qobject_cast<IVisionPlugin*>(loader.instance());
    QVERIFY(plugin);

    const auto meta = plugin->metadata();
    QCOMPARE(meta.id, std::string("vision.dummy"));
    QCOMPARE(meta.name, std::string("Dummy Detector"));
    QCOMPARE(meta.version, std::string("1.0.0"));
    QCOMPARE(meta.interfaceVersion, 1);
    QVERIFY(!meta.mode.has_value());
    QVERIFY(std::find(meta.capabilities.begin(), meta.capabilities.end(), "detect")
            != meta.capabilities.end());

    const auto detector = plugin->createDetector(DetectorCreateRequest{});
    QVERIFY(detector);
    QCOMPARE(detector->name(), std::string("Dummy"));
    QVERIFY(detector->mode() == DetectionMode::None);
    QVERIFY(detector->isReady());

    const auto detections = detector->detect(makePacket());
    QCOMPARE(detections.size(), size_t{1});
    QCOMPARE(detections.front().label, std::string("dummy"));
    QCOMPARE(detections.front().box, cv::Rect(10, 10, 20, 20));
}

QTEST_GUILESS_MAIN(DummyPluginTest)

#include "DummyPluginTest.moc"
