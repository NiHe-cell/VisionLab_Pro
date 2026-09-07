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

class FacePluginTest : public QObject
{
    Q_OBJECT

private slots:
    void metadataIsFace();
    void missingModelsIsNotReady();
    void repoModelsAreReady();
    void rescanEmptyDirectoryKeepsLiveDetector();
};

void FacePluginTest::metadataIsFace()
{
    QPluginLoader loader(QString::fromUtf8(VISIONLAB_FACE_PLUGIN));
    QVERIFY2(loader.load(), qPrintable(loader.errorString()));
    auto* plugin = qobject_cast<IVisionPlugin*>(loader.instance());
    QVERIFY(plugin);
    const auto meta = plugin->metadata();
    QCOMPARE(meta.id, std::string("vision.face"));
    QVERIFY(meta.mode.has_value());
    QVERIFY(*meta.mode == DetectionMode::Face);
}

void FacePluginTest::missingModelsIsNotReady()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    PluginManager manager;
    manager.scan(QFileInfo(QString::fromUtf8(VISIONLAB_FACE_PLUGIN))
                     .absolutePath()
                     .toStdString());

    DetectorCreateRequest request;
    request.modelDir = dir.path().toStdString();
    const auto detector = manager.createDetector("vision.face", request);
    QVERIFY(detector);
    QVERIFY(detector->mode() == DetectionMode::Face);
    QVERIFY(!detector->isReady());
    QVERIFY(detector->detect(makePacket()).empty());
}

void FacePluginTest::repoModelsAreReady()
{
    PluginManager manager;
    manager.scan(QFileInfo(QString::fromUtf8(VISIONLAB_FACE_PLUGIN))
                     .absolutePath()
                     .toStdString());

    DetectorCreateRequest request;
    request.modelDir = QString::fromUtf8(VISIONLAB_MODELS_DIR).toStdString();
    const auto detector = manager.createDetector("vision.face", request);
    QVERIFY(detector);
    QVERIFY2(detector->isReady(), "Face plugin must load deploy.prototxt + caffemodel from models/");
}

void FacePluginTest::rescanEmptyDirectoryKeepsLiveDetector()
{
    QTemporaryDir plugDir;
    QVERIFY(plugDir.isValid());
    const QString src = QString::fromUtf8(VISIONLAB_FACE_PLUGIN);
    QVERIFY(QFile::copy(src, plugDir.filePath(QFileInfo(src).fileName())));

    PluginManager manager;
    manager.scan(plugDir.path().toStdString());

    DetectorCreateRequest request;
    request.modelDir = QString::fromUtf8(VISIONLAB_MODELS_DIR).toStdString();
    const auto detector = manager.createDetector("vision.face", request);
    QVERIFY(detector);
    QVERIFY(detector->isReady());

    QTemporaryDir empty;
    QVERIFY(empty.isValid());
    manager.scan(empty.path().toStdString());
    QVERIFY(manager.metadata().empty());
    QVERIFY(detector->isReady());
    (void)detector->detect(makePacket());
}

QTEST_GUILESS_MAIN(FacePluginTest)

#include "FacePluginTest.moc"
