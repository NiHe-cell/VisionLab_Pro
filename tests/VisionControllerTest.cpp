#include <QtTest/QtTest>

#include <map>
#include <memory>

#include "core/VisionTypes.h"
#include "fakes/FakeDetector.h"
#include "fakes/FakeVideoSource.h"
#include "analytics/RuleEngine.h"
#include "controllers/VisionController.h"
#include "pipeline/VisionPipeline.h"
#include "tracking/ByteTrackTracker.h"
#include "utilities/CameraManager.h"

using visionlab::ByteTrackTracker;
using visionlab::DetectionMode;
using visionlab::IDetector;
using visionlab::RuleEngine;
using visionlab::VisionPipeline;

namespace {

std::map<DetectionMode, std::unique_ptr<IDetector>> makeFaceDetector()
{
    std::map<DetectionMode, std::unique_ptr<IDetector>> detectors;
    detectors.emplace(DetectionMode::Face, std::make_unique<FakeDetector>("face"));
    return detectors;
}

} // namespace

class VisionControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void startFillsDetectionModel();
};

void VisionControllerTest::startFillsDetectionModel()
{
    auto source = std::make_unique<FakeVideoSource>(32, "fake:vc", true, true);
    auto pipeline = std::make_unique<VisionPipeline>(
        std::move(source), makeFaceDetector(), VisionPipeline::kDefaultQueueCapacity,
        std::make_unique<ByteTrackTracker>(),
        std::make_unique<RuleEngine>());
    CameraManager camera(std::move(pipeline));
    VisionController controller(&camera);

    QVERIFY(controller.detectionModel());
    QVERIFY(controller.trackModel());
    QVERIFY(controller.eventModel());
    QVERIFY(controller.eventFilterModel());
    QVERIFY(controller.performanceModel());
    QVERIFY(controller.pluginModel());
    QVERIFY(controller.ruleModel());
    QCOMPARE(controller.currentPage(), 0);
    QCOMPARE(controller.detectionModel()->rowCount(), 0);

    controller.startCamera();
    QVERIFY(controller.running());
    QTRY_VERIFY_WITH_TIMEOUT(controller.detectionModel()->rowCount() >= 1, 2000);
    controller.eventModel()->ingest(camera.recentEvents());
    controller.stopCamera();
    QVERIFY(!controller.running());
}

QTEST_GUILESS_MAIN(VisionControllerTest)

#include "VisionControllerTest.moc"
