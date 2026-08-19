#include <QtTest/QtTest>

#include <QDir>
#include <QFile>

#include <memory>

#include "detectors/YoloDetector.h"
#include "fakes/FakeInferenceEngine.h"

using visionlab::DetectionMode;
using visionlab::FramePacket;
using visionlab::ModelConfig;
using visionlab::TensorView;
using visionlab::YoloDetector;

namespace {

FramePacket makeFrame(int width, int height)
{
    FramePacket packet;
    packet.frameId = 1;
    packet.image = cv::Mat(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
    return packet;
}

TensorView makeYoloRow(float cx, float cy, float w, float h,
                       float objectness, int bestClass, float classScore)
{
    TensorView view;
    view.shape = {1, 1, 85};
    view.data.assign(85, 0.0F);
    view.data[0] = cx;
    view.data[1] = cy;
    view.data[2] = w;
    view.data[3] = h;
    view.data[4] = objectness;
    view.data[5 + bestClass] = classScore;
    return view;
}

std::filesystem::path writeClassNames(const QString& content)
{
    const QString path = QDir::temp().filePath(
        QStringLiteral("visionlab-classes-%1.txt").arg(QTest::currentTestFunction()));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return {};
    file.write(content.toUtf8());
    file.close();
    return std::filesystem::path(path.toStdString());
}

} // namespace

class YoloDetectorTest : public QObject
{
    Q_OBJECT

private slots:
    void fakeOutputMatchesDecodingCoordinates();
    void initializeFailureIsNotReady();
    void inferFailureReturnsEmpty();
    void emptyFrameReturnsEmpty();
};

void YoloDetectorTest::fakeOutputMatchesDecodingCoordinates()
{
    auto engine = std::make_unique<FakeInferenceEngine>();
    FakeInferenceEngine* fake = engine.get();
    fake->setOutputs({makeYoloRow(0.5F, 0.5F, 0.2F, 0.2F, 0.9F, 1, 0.9F)});

    ModelConfig config;
    config.classNamesPath = writeClassNames(QStringLiteral("a\nb\n"));

    YoloDetector detector(std::move(engine), config);
    QVERIFY(detector.isReady());
    QCOMPARE(detector.name(), std::string("YOLOv4-tiny"));
    QVERIFY(detector.mode() == DetectionMode::Object);

    const auto detections = detector.detect(makeFrame(100, 100));
    QCOMPARE(detections.size(), size_t{1});
    QCOMPARE(detections.front().classId, 1);
    QCOMPARE(detections.front().label, std::string("b"));
    QVERIFY(std::abs(detections.front().confidence - 0.81F) < 1e-4F);
    QCOMPARE(detections.front().box, cv::Rect(40, 40, 20, 20));
}

void YoloDetectorTest::initializeFailureIsNotReady()
{
    auto engine = std::make_unique<FakeInferenceEngine>();
    engine->setInitializeOk(false);
    engine->setErrorMessage("no model");

    YoloDetector detector(std::move(engine), ModelConfig{});
    QVERIFY(!detector.isReady());
    QVERIFY(detector.detect(makeFrame(8, 8)).empty());
}

void YoloDetectorTest::inferFailureReturnsEmpty()
{
    auto engine = std::make_unique<FakeInferenceEngine>();
    engine->setInferOk(false);

    YoloDetector detector(std::move(engine), ModelConfig{});
    QVERIFY(detector.isReady());
    QVERIFY(detector.detect(makeFrame(8, 8)).empty());
}

void YoloDetectorTest::emptyFrameReturnsEmpty()
{
    auto engine = std::make_unique<FakeInferenceEngine>();
    engine->setOutputs({makeYoloRow(0.5F, 0.5F, 0.2F, 0.2F, 0.9F, 0, 0.9F)});

    YoloDetector detector(std::move(engine), ModelConfig{});
    QVERIFY(detector.detect(FramePacket{}).empty());
}

QTEST_APPLESS_MAIN(YoloDetectorTest)

#include "YoloDetectorTest.moc"
