#include <QtTest/QtTest>

#include "plugin/DetectorCreateRequest.h"
#include "plugin/IVisionPlugin.h"
#include "plugin/PluginMetadata.h"

using visionlab::DetectionMode;
using visionlab::DetectorCreateRequest;
using visionlab::InferenceBackend;
using visionlab::InferencePrecision;
using visionlab::PluginMetadata;

class PluginMetadataTest : public QObject
{
    Q_OBJECT

private slots:
    void metadataDefaultsToInterfaceVersionOne();
    void dummyMetadataHasNoDetectionMode();
    void objectMetadataCarriesObjectMode();
    void createRequestDefaultsToCpuFp32();
    void pluginIidIsStable();
};

void PluginMetadataTest::metadataDefaultsToInterfaceVersionOne()
{
    const PluginMetadata meta;
    QCOMPARE(meta.interfaceVersion, 1);
    QVERIFY(meta.id.empty());
    QVERIFY(meta.capabilities.empty());
}

void PluginMetadataTest::dummyMetadataHasNoDetectionMode()
{
    PluginMetadata dummy;
    dummy.id = "vision.dummy";
    dummy.name = "Dummy Detector";
    dummy.version = "1.0.0";
    dummy.capabilities = {"detect"};
    dummy.interfaceVersion = 1;

    QVERIFY(!dummy.mode.has_value());
}

void PluginMetadataTest::objectMetadataCarriesObjectMode()
{
    PluginMetadata object;
    object.id = "vision.yolo";
    object.mode = DetectionMode::Object;

    QVERIFY(object.mode.has_value());
    QVERIFY(*object.mode == DetectionMode::Object);
}

void PluginMetadataTest::createRequestDefaultsToCpuFp32()
{
    const DetectorCreateRequest request;
    QVERIFY(request.backend == InferenceBackend::OnnxRuntimeCpu);
    QVERIFY(request.precision == InferencePrecision::Fp32);
    QCOMPARE(request.deviceId, 0);
    QCOMPARE(request.confidenceThreshold, 0.25F);
    QCOMPARE(request.nmsThreshold, 0.45F);
    QVERIFY(request.modelDir.empty());
}

void PluginMetadataTest::pluginIidIsStable()
{
    QCOMPARE(QLatin1String(VisionLab_IVisionPlugin_iid),
             QLatin1String("com.visionlab.IVisionPlugin/1.0"));
}

QTEST_APPLESS_MAIN(PluginMetadataTest)

#include "PluginMetadataTest.moc"
