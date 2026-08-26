#include <QtTest/QtTest>

#include <QFileInfo>
#include <QPluginLoader>

#include "plugin/IVisionPlugin.h"
#include "plugin/PluginManager.h"

using visionlab::DetectionMode;
using visionlab::DetectorCreateRequest;
using visionlab::IVisionPlugin;
using visionlab::PluginManager;

namespace {

visionlab::FramePacket makePacket()
{
    visionlab::FramePacket packet;
    packet.image = cv::Mat(8, 8, CV_8UC3, cv::Scalar(0, 0, 0));
    return packet;
}

} // namespace

class MotionPluginTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsWithMotionMode();
    void pluginManagerCreatesMotionDetector();
};

void MotionPluginTest::loadsWithMotionMode()
{
    QPluginLoader loader(QString::fromUtf8(VISIONLAB_MOTION_PLUGIN));
    QVERIFY2(loader.load(), qPrintable(loader.errorString()));

    auto* plugin = qobject_cast<IVisionPlugin*>(loader.instance());
    QVERIFY(plugin);

    const auto meta = plugin->metadata();
    QCOMPARE(meta.id, std::string("vision.motion"));
    QCOMPARE(meta.interfaceVersion, 1);
    QVERIFY(meta.mode.has_value());
    QVERIFY(*meta.mode == DetectionMode::Motion);

    const auto detector = plugin->createDetector(DetectorCreateRequest{});
    QVERIFY(detector);
    QVERIFY(detector->mode() == DetectionMode::Motion);
    QVERIFY(detector->isReady());
    detector->detect(makePacket());
}

void MotionPluginTest::pluginManagerCreatesMotionDetector()
{
    PluginManager manager;
    manager.scan(QFileInfo(QString::fromUtf8(VISIONLAB_MOTION_PLUGIN))
                     .absolutePath()
                     .toStdString());

    const auto detector = manager.createDetector("vision.motion", DetectorCreateRequest{});
    QVERIFY(detector);
    QVERIFY(detector->mode() == DetectionMode::Motion);
    QVERIFY(detector->isReady());
}

QTEST_GUILESS_MAIN(MotionPluginTest)

#include "MotionPluginTest.moc"
