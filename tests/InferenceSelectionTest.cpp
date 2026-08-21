#include <QtTest/QtTest>

#include "inference/InferenceSelection.h"
#include "inference/InferenceTypes.h"

using visionlab::InferenceBackend;
using visionlab::InferencePrecision;
using visionlab::inferenceSelectionFromEnv;

namespace {

void clearInferenceEnv()
{
    qunsetenv("VISIONLAB_INFERENCE_BACKEND");
    qunsetenv("VISIONLAB_INFERENCE_PRECISION");
    qunsetenv("VISIONLAB_INFERENCE_DEVICE");
}

} // namespace

class InferenceSelectionTest : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void unsetDefaultsToCpuFp32Device0();
    void parsesCudaFp16Device1();
    void bogusBackendKeepsCpu();
    void bogusPrecisionKeepsFp32();
    void bogusDeviceKeepsZero();
};

void InferenceSelectionTest::cleanup()
{
    clearInferenceEnv();
}

void InferenceSelectionTest::unsetDefaultsToCpuFp32Device0()
{
    clearInferenceEnv();
    const auto selection = inferenceSelectionFromEnv();
    QVERIFY(selection.backend == InferenceBackend::OnnxRuntimeCpu);
    QVERIFY(selection.precision == InferencePrecision::Fp32);
    QCOMPARE(selection.deviceId, 0);
}

void InferenceSelectionTest::parsesCudaFp16Device1()
{
    QVERIFY(qputenv("VISIONLAB_INFERENCE_BACKEND", QByteArrayLiteral("onnx-cuda")));
    QVERIFY(qputenv("VISIONLAB_INFERENCE_PRECISION", QByteArrayLiteral("fp16")));
    QVERIFY(qputenv("VISIONLAB_INFERENCE_DEVICE", QByteArrayLiteral("1")));

    const auto selection = inferenceSelectionFromEnv();
    QVERIFY(selection.backend == InferenceBackend::OnnxRuntimeCuda);
    QVERIFY(selection.precision == InferencePrecision::Fp16);
    QCOMPARE(selection.deviceId, 1);
}

void InferenceSelectionTest::bogusBackendKeepsCpu()
{
    QVERIFY(qputenv("VISIONLAB_INFERENCE_BACKEND", QByteArrayLiteral("bogus")));
    const auto selection = inferenceSelectionFromEnv();
    QVERIFY(selection.backend == InferenceBackend::OnnxRuntimeCpu);
}

void InferenceSelectionTest::bogusPrecisionKeepsFp32()
{
    QVERIFY(qputenv("VISIONLAB_INFERENCE_PRECISION", QByteArrayLiteral("int8")));
    const auto selection = inferenceSelectionFromEnv();
    QVERIFY(selection.precision == InferencePrecision::Fp32);
}

void InferenceSelectionTest::bogusDeviceKeepsZero()
{
    QVERIFY(qputenv("VISIONLAB_INFERENCE_DEVICE", QByteArrayLiteral("not-a-number")));
    const auto selection = inferenceSelectionFromEnv();
    QCOMPARE(selection.deviceId, 0);
}

QTEST_APPLESS_MAIN(InferenceSelectionTest)

#include "InferenceSelectionTest.moc"
