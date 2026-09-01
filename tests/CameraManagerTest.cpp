#include <QtTest/QtTest>

#include <map>
#include <memory>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "core/VisionTypes.h"
#include "fakes/FakeDetector.h"
#include "fakes/FakeVideoSource.h"
#include "pipeline/VisionPipeline.h"
#include "tracking/ByteTrackTracker.h"
#include "utilities/CameraManager.h"
#include "utilities/SessionSettings.h"

using visionlab::ByteTrackTracker;
using visionlab::DetectionMode;
using visionlab::IDetector;
using visionlab::SessionSettings;
using visionlab::VisionPipeline;

namespace {

std::map<DetectionMode, std::unique_ptr<IDetector>> makeFaceDetector()
{
    std::map<DetectionMode, std::unique_ptr<IDetector>> detectors;
    detectors.emplace(DetectionMode::Face, std::make_unique<FakeDetector>("face"));
    return detectors;
}

std::unique_ptr<VisionPipeline> makePipeline(std::unique_ptr<FakeVideoSource> source)
{
    return std::make_unique<VisionPipeline>(
        std::move(source), makeFaceDetector(), VisionPipeline::kDefaultQueueCapacity,
        std::make_unique<ByteTrackTracker>());
}

bool copyNativePlugin(const QDir& src, const QDir& dst, const QString& stem)
{
    const QStringList names =
        src.entryList({stem + QStringLiteral(".*"), QStringLiteral("lib") + stem + QStringLiteral(".*")},
                      QDir::Files);
    bool copied = false;
    for (const QString& name : names)
    {
        if (name.endsWith(QStringLiteral(".pdb"), Qt::CaseInsensitive)
            || name.endsWith(QStringLiteral(".lib"), Qt::CaseInsensitive)
            || name.endsWith(QStringLiteral(".exp"), Qt::CaseInsensitive)
            || name.endsWith(QStringLiteral(".ilk"), Qt::CaseInsensitive))
        {
            continue;
        }
        const QString to = dst.filePath(name);
        QFile::remove(to);
        if (!QFile::copy(src.filePath(name), to))
            return false;
        copied = true;
    }
    return copied;
}

} // namespace

class CameraManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void startFailsWhenSourceOpenFails();
    void startPublishesFrameThenStopClears();
    void emptyPluginDirStartStopDoesNotAbort();
    void missingYoloObjectModeStaysAlive();
    void productionPluginsSwitchModesWithoutCrash();
    void productionPluginsRepeatStartStop();
    void applyRejectedWhileRunningKeepsMode();
    void trackingDisabledRebuildPublishesFramesWithoutTracks();
};

void CameraManagerTest::cleanup()
{
    qunsetenv("VISIONLAB_PLUGIN_DIR");
}

void CameraManagerTest::startFailsWhenSourceOpenFails()
{
    auto source = std::make_unique<FakeVideoSource>(10, "fake:0", false);
    CameraManager manager(makePipeline(std::move(source)));

    QVERIFY(!manager.start());
    QVERIFY(manager.frame().isNull());
}

void CameraManagerTest::startPublishesFrameThenStopClears()
{
    auto source = std::make_unique<FakeVideoSource>(32, "fake:cam", true, true);
    CameraManager manager(makePipeline(std::move(source)));
    manager.setMode(DetectionMode::Face);

    QSignalSpy changed(&manager, &CameraManager::frameChanged);
    QSignalSpy cleared(&manager, &CameraManager::frameCleared);

    QVERIFY(manager.start());
    QVERIFY(changed.wait(2000));
    QVERIFY(!manager.frame().isNull());
    QCOMPARE(manager.frame().format(), QImage::Format_RGB888);
    QVERIFY(manager.statsSnapshot().capturedFrames > 0);
    QTRY_VERIFY_WITH_TIMEOUT(manager.statsSnapshot().activeTracks > 0, 2000);

    QVERIFY(manager.stop());
    QCOMPARE(cleared.count(), 1);
    QVERIFY(manager.frame().isNull());
}

void CameraManagerTest::emptyPluginDirStartStopDoesNotAbort()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    qputenv("VISIONLAB_PLUGIN_DIR", dir.path().toUtf8());

    auto source = std::make_unique<FakeVideoSource>(16, "fake:empty-plugins", true, true);
    CameraManager manager(std::move(source));

    QVERIFY(manager.start());
    QVERIFY(manager.stop());
}

void CameraManagerTest::missingYoloObjectModeStaysAlive()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QDir src(QString::fromUtf8(VISIONLAB_BUILD_PLUGINS_DIR));
    const QDir dst(dir.path());
    QVERIFY2(copyNativePlugin(src, dst, QStringLiteral("vision_face")),
             "need vision_face in the build plugins directory");
    QVERIFY2(copyNativePlugin(src, dst, QStringLiteral("vision_motion")),
             "need vision_motion in the build plugins directory");
    QVERIFY(dst.entryList({QStringLiteral("vision_yolo.*"), QStringLiteral("libvision_yolo.*")},
                          QDir::Files)
                .isEmpty());

    qputenv("VISIONLAB_PLUGIN_DIR", dir.path().toUtf8());

    auto source = std::make_unique<FakeVideoSource>(32, "fake:no-yolo", true, true);
    CameraManager manager(std::move(source));
    manager.setMode(DetectionMode::Object);

    QSignalSpy changed(&manager, &CameraManager::frameChanged);
    QVERIFY(manager.start());
    QVERIFY(changed.wait(2000));
    QVERIFY(!manager.frame().isNull());
    QVERIFY(manager.stop());
}

void CameraManagerTest::productionPluginsSwitchModesWithoutCrash()
{
    qputenv("VISIONLAB_PLUGIN_DIR", VISIONLAB_BUILD_PLUGINS_DIR);

    auto source = std::make_unique<FakeVideoSource>(64, "fake:all-plugins", true, true);
    CameraManager manager(std::move(source));

    QSignalSpy changed(&manager, &CameraManager::frameChanged);
    QVERIFY(manager.start());
    QVERIFY(changed.wait(2000));

    manager.setMode(DetectionMode::Face);
    manager.setMode(DetectionMode::Object);
    manager.setMode(DetectionMode::Motion);
    QVERIFY(!manager.frame().isNull());
    QVERIFY(manager.stop());
}

void CameraManagerTest::productionPluginsRepeatStartStop()
{
    qputenv("VISIONLAB_PLUGIN_DIR", VISIONLAB_BUILD_PLUGINS_DIR);

    auto source = std::make_unique<FakeVideoSource>(64, "fake:restart-plugins", true, true);
    CameraManager manager(std::move(source));

    QVERIFY(manager.start());
    QVERIFY(manager.stop());
    QVERIFY(manager.start());
    QVERIFY(manager.stop());
}

void CameraManagerTest::applyRejectedWhileRunningKeepsMode()
{
    auto source = std::make_unique<FakeVideoSource>(32, "fake:apply-running", true, true);
    CameraManager manager(makePipeline(std::move(source)));
    QVERIFY(manager.start());
    QVERIFY(manager.mode() == DetectionMode::Face);

    SessionSettings settings = manager.sessionSettings();
    settings.trackingEnabled = false;
    QVERIFY(!manager.applySessionSettings(settings));
    QVERIFY(manager.mode() == DetectionMode::Face);
    QVERIFY(manager.sessionSettings().trackingEnabled);

    QVERIFY(manager.stop());
}

void CameraManagerTest::trackingDisabledRebuildPublishesFramesWithoutTracks()
{
    qputenv("VISIONLAB_PLUGIN_DIR", VISIONLAB_BUILD_PLUGINS_DIR);

    auto source = std::make_unique<FakeVideoSource>(64, "fake:no-track", true, true);
    CameraManager manager(std::move(source));

    SessionSettings settings = manager.sessionSettings();
    settings.trackingEnabled = false;
    QVERIFY(manager.applySessionSettings(settings));
    QVERIFY(!manager.sessionSettings().trackingEnabled);

    QSignalSpy changed(&manager, &CameraManager::frameChanged);
    QVERIFY(manager.start());
    QVERIFY(changed.wait(2000));
    QVERIFY(!manager.frame().isNull());
    QTRY_VERIFY_WITH_TIMEOUT(manager.statsSnapshot().processedFrames > 0, 2000);
    QCOMPARE(manager.statsSnapshot().activeTracks, std::size_t{0});
    QVERIFY(manager.stop());
}

QTEST_GUILESS_MAIN(CameraManagerTest)

#include "CameraManagerTest.moc"
