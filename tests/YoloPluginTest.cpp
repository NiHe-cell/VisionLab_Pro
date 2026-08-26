#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QTemporaryDir>

#include "plugin/IVisionPlugin.h"
#include "plugin/PluginManager.h"

using visionlab::DetectionMode;
using visionlab::DetectorCreateRequest;
using visionlab::InferenceBackend;
using visionlab::IVisionPlugin;
using visionlab::PluginManager;

namespace {

visionlab::FramePacket makePacket()
{
    visionlab::FramePacket packet;
    packet.image = cv::Mat(8, 8, CV_8UC3, cv::Scalar(0, 0, 0));
    return packet;
}

bool copyFile(const QString& from, const QString& to)
{
    QFile::remove(to);
    return QFile::copy(from, to);
}

} // namespace

class YoloPluginTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void metadataIsObject();
    void missingOnnxIsNotReady();
    void identityFixtureIsReadyWithoutDarknetFiles();
    void cudaBackendIsNotReadyWithoutGpuSession();
};

void YoloPluginTest::initTestCase()
{
    qunsetenv("VISIONLAB_INFERENCE_BACKEND");
    qunsetenv("VISIONLAB_INFERENCE_PRECISION");
    qunsetenv("VISIONLAB_INFERENCE_DEVICE");
}

void YoloPluginTest::metadataIsObject()
{
    QPluginLoader loader(QString::fromUtf8(VISIONLAB_YOLO_PLUGIN));
    QVERIFY2(loader.load(), qPrintable(loader.errorString()));
    auto* plugin = qobject_cast<IVisionPlugin*>(loader.instance());
    QVERIFY(plugin);
    const auto meta = plugin->metadata();
    QCOMPARE(meta.id, std::string("vision.yolo"));
    QVERIFY(meta.mode.has_value());
    QVERIFY(*meta.mode == DetectionMode::Object);

    DetectorCreateRequest request;
    const auto detector = plugin->createDetector(request);
    QVERIFY(detector);
    QCOMPARE(detector->name(), std::string("YOLOv4-tiny"));
    QVERIFY(detector->mode() == DetectionMode::Object);
}

void YoloPluginTest::missingOnnxIsNotReady()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    PluginManager manager;
    manager.scan(QFileInfo(QString::fromUtf8(VISIONLAB_YOLO_PLUGIN))
                     .absolutePath()
                     .toStdString());

    DetectorCreateRequest request;
    request.modelDir = dir.path().toStdString();
    const auto detector = manager.createDetector("vision.yolo", request);
    QVERIFY(detector);
    QVERIFY(!detector->isReady());
    QVERIFY(detector->detect(makePacket()).empty());
}

void YoloPluginTest::identityFixtureIsReadyWithoutDarknetFiles()
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

    PluginManager manager;
    manager.scan(QFileInfo(QString::fromUtf8(VISIONLAB_YOLO_PLUGIN))
                     .absolutePath()
                     .toStdString());

    DetectorCreateRequest request;
    request.modelDir = dir.path().toStdString();
    const auto detector = manager.createDetector("vision.yolo", request);
    QVERIFY(detector);
    QCOMPARE(detector->name(), std::string("YOLOv4-tiny"));
    QVERIFY2(detector->isReady(),
             "Yolo plugin must load yolov4-tiny.onnx; Darknet cfg/weights must not be required");
}

void YoloPluginTest::cudaBackendIsNotReadyWithoutGpuSession()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    PluginManager manager;
    manager.scan(QFileInfo(QString::fromUtf8(VISIONLAB_YOLO_PLUGIN))
                     .absolutePath()
                     .toStdString());

    DetectorCreateRequest request;
    request.modelDir = dir.path().toStdString();
    request.backend = InferenceBackend::OnnxRuntimeCuda;
    const auto detector = manager.createDetector("vision.yolo", request);
    QVERIFY(detector);
    if (detector->isReady())
        QSKIP("CUDA EP is available on this machine; cannot assert the failure path");
    QVERIFY(detector->detect(makePacket()).empty());
}

QTEST_GUILESS_MAIN(YoloPluginTest)

#include "YoloPluginTest.moc"
