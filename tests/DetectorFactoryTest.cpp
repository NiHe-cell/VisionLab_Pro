#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "detectors/DetectorFactory.h"

using visionlab::DetectionMode;
using visionlab::FramePacket;
using visionlab::IDetector;

namespace {

FramePacket makeFrame()
{
    FramePacket packet;
    packet.frameId = 1;
    packet.image = cv::Mat(8, 8, CV_8UC3, cv::Scalar(0, 0, 0));
    return packet;
}

bool copyFile(const QString& from, const QString& to)
{
    QFile::remove(to);
    return QFile::copy(from, to);
}

} // namespace

class DetectorFactoryTest : public QObject
{
    Q_OBJECT

private slots:
    void objectModeNameIsYoloV4Tiny();
    void missingOnnxIsNotReady();
    void objectModeLoadsOnnxWithoutDarknetFiles();
    void motionDoesNotNeedModels();
};

void DetectorFactoryTest::objectModeNameIsYoloV4Tiny()
{
    const auto detector = visionlab::createDetector(DetectionMode::Object, ".");
    QVERIFY(detector);
    QCOMPARE(detector->name(), std::string("YOLOv4-tiny"));
    QVERIFY(detector->mode() == DetectionMode::Object);
}

void DetectorFactoryTest::missingOnnxIsNotReady()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const auto detector =
        visionlab::createDetector(DetectionMode::Object, dir.path().toStdString());
    QVERIFY(detector);
    QVERIFY(!detector->isReady());
    QVERIFY(detector->detect(makeFrame()).empty());
}

void DetectorFactoryTest::objectModeLoadsOnnxWithoutDarknetFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString onnxSrc = QDir(QString::fromUtf8(VISIONLAB_TEST_FIXTURES))
                                .filePath(QStringLiteral("identity_f32_1x3x2x2.onnx"));
    QVERIFY(QFileInfo::exists(onnxSrc));
    QVERIFY(copyFile(onnxSrc, dir.filePath(QStringLiteral("yolov4-tiny.onnx"))));

    QFile names(dir.filePath(QStringLiteral("coco.names")));
    QVERIFY(names.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    names.write("person\nbicycle\n");
    names.close();

    QVERIFY(!QFileInfo::exists(dir.filePath(QStringLiteral("yolov4-tiny.cfg"))));
    QVERIFY(!QFileInfo::exists(dir.filePath(QStringLiteral("yolov4-tiny.weights"))));

    const auto detector =
        visionlab::createDetector(DetectionMode::Object, dir.path().toStdString());
    QVERIFY(detector);
    QCOMPARE(detector->name(), std::string("YOLOv4-tiny"));
    QVERIFY2(detector->isReady(),
             "Object mode must load yolov4-tiny.onnx; Darknet cfg/weights must not be required");
}

void DetectorFactoryTest::motionDoesNotNeedModels()
{
    const auto detector = visionlab::createDetector(DetectionMode::Motion, "");
    QVERIFY(detector);
    QVERIFY(detector->isReady());
}

QTEST_APPLESS_MAIN(DetectorFactoryTest)

#include "DetectorFactoryTest.moc"
