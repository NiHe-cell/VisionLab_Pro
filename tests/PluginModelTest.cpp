#include <QtTest/QtTest>

#include "core/VisionTypes.h"
#include "models/PluginModel.h"
#include "plugin/PluginMetadata.h"

using visionlab::DetectionMode;
using visionlab::PluginMetadata;

class PluginModelTest : public QObject
{
    Q_OBJECT

private slots:
    void twoPluginsExposeIdNameAndEmptyDummyMode();
};

void PluginModelTest::twoPluginsExposeIdNameAndEmptyDummyMode()
{
    PluginMetadata dummy;
    dummy.id = "vision.dummy";
    dummy.name = "Dummy Detector";
    dummy.version = "1.0.0";
    dummy.description = "test dummy";
    dummy.capabilities = {"detect"};

    PluginMetadata face;
    face.id = "vision.face";
    face.name = "Face Detector";
    face.version = "1.0.0";
    face.description = "test face";
    face.capabilities = {"detect"};
    face.mode = DetectionMode::Face;

    PluginModel model;
    model.setPlugins({dummy, face}, {"load failed"});

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 0), PluginModel::PluginIdRole).toString(),
             QStringLiteral("vision.dummy"));
    QCOMPARE(model.data(model.index(0, 0), PluginModel::NameRole).toString(),
             QStringLiteral("Dummy Detector"));
    QCOMPARE(model.data(model.index(0, 0), PluginModel::ModeLabelRole).toString(), QString());
    QCOMPARE(model.data(model.index(1, 0), PluginModel::PluginIdRole).toString(),
             QStringLiteral("vision.face"));
    QCOMPARE(model.data(model.index(1, 0), PluginModel::NameRole).toString(),
             QStringLiteral("Face Detector"));
    QCOMPARE(model.data(model.index(1, 0), PluginModel::ModeLabelRole).toString(),
             QStringLiteral("Face Detection"));
    QCOMPARE(model.data(model.index(1, 0), PluginModel::CapabilitiesRole).toStringList(),
             QStringList{QStringLiteral("detect")});
    QCOMPARE(model.loadErrors(), QStringList{QStringLiteral("load failed")});

    const auto names = model.roleNames();
    QCOMPARE(names.value(PluginModel::PluginIdRole), QByteArray("pluginId"));
    QCOMPARE(names.value(PluginModel::NameRole), QByteArray("name"));
    QCOMPARE(names.value(PluginModel::VersionRole), QByteArray("version"));
    QCOMPARE(names.value(PluginModel::DescriptionRole), QByteArray("description"));
    QCOMPARE(names.value(PluginModel::ModeLabelRole), QByteArray("modeLabel"));
    QCOMPARE(names.value(PluginModel::CapabilitiesRole), QByteArray("capabilities"));
}

QTEST_GUILESS_MAIN(PluginModelTest)

#include "PluginModelTest.moc"
