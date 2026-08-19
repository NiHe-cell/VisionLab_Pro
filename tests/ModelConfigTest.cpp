#include <QtTest/QtTest>

#include "inference/InferenceTypes.h"
#include "inference/ModelConfig.h"

using visionlab::InferResult;
using visionlab::InferenceBackend;
using visionlab::InferencePrecision;
using visionlab::ModelConfig;
using visionlab::TensorView;

class ModelConfigTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsMatchYoloV4TinyCpu();
    void tensorViewCopyOwnsIndependentBuffer();
    void inferResultDefaultsToNotOk();
};

void ModelConfigTest::defaultsMatchYoloV4TinyCpu()
{
    const ModelConfig config;

    QCOMPARE(config.inputWidth, 320);
    QCOMPARE(config.inputHeight, 320);
    QCOMPARE(config.confidenceThreshold, 0.25F);
    QCOMPARE(config.nmsThreshold, 0.45F);
    QVERIFY(config.backend == InferenceBackend::OnnxRuntimeCpu);
    QCOMPARE(config.deviceId, 0);
    QVERIFY(config.precision == InferencePrecision::Fp32);
    QVERIFY(config.modelPath.empty());
    QVERIFY(config.classNamesPath.empty());
}

void ModelConfigTest::tensorViewCopyOwnsIndependentBuffer()
{
    TensorView original;
    original.shape = {1, 3, 2, 2};
    original.data = {1.0F, 2.0F, 3.0F, 4.0F};

    TensorView copy = original;
    copy.data[0] = 99.0F;

    QCOMPARE(original.data.front(), 1.0F);
    QCOMPARE(copy.data.front(), 99.0F);
    QCOMPARE(copy.shape, original.shape);
}

void ModelConfigTest::inferResultDefaultsToNotOk()
{
    const InferResult result;
    QVERIFY(!result.ok);
    QVERIFY(result.error.empty());
    QVERIFY(result.outputs.empty());
    QCOMPARE(result.latencyMs, 0.0);
}

QTEST_APPLESS_MAIN(ModelConfigTest)

#include "ModelConfigTest.moc"
