#include <QtTest/QtTest>

#include <map>
#include <memory>

#include "analytics/RuleEngine.h"
#include "analytics/RuleSpec.h"
#include "core/VisionTypes.h"
#include "fakes/FakeDetector.h"
#include "fakes/FakeVideoSource.h"
#include "controllers/VisionController.h"
#include "pipeline/VisionPipeline.h"
#include "tracking/ByteTrackTracker.h"
#include "utilities/CameraManager.h"

using visionlab::ByteTrackTracker;
using visionlab::DetectionMode;
using visionlab::IDetector;
using visionlab::RuleEngine;
using visionlab::RuleKind;
using visionlab::VisionPipeline;

namespace {

std::map<DetectionMode, std::unique_ptr<IDetector>> makeFaceDetector()
{
    std::map<DetectionMode, std::unique_ptr<IDetector>> detectors;
    detectors.emplace(DetectionMode::Face, std::make_unique<FakeDetector>("face"));
    return detectors;
}

std::unique_ptr<VisionPipeline> makePipeline()
{
    auto source = std::make_unique<FakeVideoSource>(32, "fake:vc", true, true);
    return std::make_unique<VisionPipeline>(
        std::move(source), makeFaceDetector(), VisionPipeline::kDefaultQueueCapacity,
        std::make_unique<ByteTrackTracker>(),
        std::make_unique<RuleEngine>());
}

} // namespace

class VisionControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void startFillsDetectionModel();
    void mapsItemToFrameWithLetterbox();
    void rejectsLetterboxMargin();
    void finishDrawRoiThenStartEnablesRules();
    void beginDrawIgnoredWhileRunning();
};

void VisionControllerTest::startFillsDetectionModel()
{
    CameraManager camera(makePipeline());
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

void VisionControllerTest::mapsItemToFrameWithLetterbox()
{
    CameraManager camera(makePipeline());
    VisionController controller(&camera);
    controller.setFrameSize(100, 100);

    const QPointF frame = controller.itemToFrame(50, 0, 200, 100);
    QCOMPARE(frame, QPointF(0, 0));
    QVERIFY(controller.itemPointInVideo(50, 0, 200, 100));
}

void VisionControllerTest::rejectsLetterboxMargin()
{
    CameraManager camera(makePipeline());
    VisionController controller(&camera);
    controller.setFrameSize(100, 100);

    QVERIFY(!controller.itemPointInVideo(0, 0, 200, 100));
    QCOMPARE(controller.itemToFrame(0, 0, 200, 100), QPointF());
}

void VisionControllerTest::finishDrawRoiThenStartEnablesRules()
{
    CameraManager camera(makePipeline());
    VisionController controller(&camera);
    controller.setFrameSize(100, 100);

    controller.beginDraw(VisionController::Roi);
    controller.addDrawPoint(50, 0, 200, 100);
    controller.addDrawPoint(149, 0, 200, 100);
    controller.addDrawPoint(149, 99, 200, 100);
    controller.addDrawPoint(50, 99, 200, 100);
    controller.finishDraw();

    QCOMPARE(controller.ruleModel()->rowCount(), 1);
    QCOMPARE(controller.ruleModel()->data(controller.ruleModel()->index(0, 0),
                                          RuleModel::KindRole).toInt(),
             static_cast<int>(RuleKind::RoiIntrusion));
    QCOMPARE(controller.ruleModel()->data(controller.ruleModel()->index(0, 0),
                                          RuleModel::VertexCountRole).toInt(),
             4);

    controller.startCamera();
    QVERIFY(controller.running());
    QTRY_VERIFY_WITH_TIMEOUT(camera.statsSnapshot().enabledRules >= 1, 2000);
    controller.stopCamera();
}

void VisionControllerTest::beginDrawIgnoredWhileRunning()
{
    CameraManager camera(makePipeline());
    VisionController controller(&camera);
    controller.setFrameSize(100, 100);
    controller.startCamera();
    QVERIFY(controller.running());

    controller.beginDraw(VisionController::Roi);
    controller.addDrawPoint(50, 0, 200, 100);
    controller.finishDraw();
    QCOMPARE(controller.ruleModel()->rowCount(), 0);
    QCOMPARE(controller.drawTool(), static_cast<int>(VisionController::None));

    controller.stopCamera();
}

QTEST_GUILESS_MAIN(VisionControllerTest)

#include "VisionControllerTest.moc"
