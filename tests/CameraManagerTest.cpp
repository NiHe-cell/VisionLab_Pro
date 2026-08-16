#include <QtTest/QtTest>

#include <map>
#include <memory>

#include "core/VisionTypes.h"
#include "fakes/FakeDetector.h"
#include "fakes/FakeVideoSource.h"
#include "pipeline/VisionPipeline.h"
#include "utilities/CameraManager.h"

using visionlab::DetectionMode;
using visionlab::IDetector;
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
    return std::make_unique<VisionPipeline>(std::move(source), makeFaceDetector());
}

} // namespace

class CameraManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void startFailsWhenSourceOpenFails();
    void startPublishesFrameThenStopClears();
};

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

    QVERIFY(manager.stop());
    QCOMPARE(cleared.count(), 1);
    QVERIFY(manager.frame().isNull());
}

QTEST_GUILESS_MAIN(CameraManagerTest)

#include "CameraManagerTest.moc"
