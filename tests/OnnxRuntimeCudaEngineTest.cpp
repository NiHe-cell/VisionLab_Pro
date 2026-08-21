#include <QtTest/QtTest>

#include <QString>

#include <filesystem>
#include <vector>

#include "inference/InferenceEngineFactory.h"
#include "inference/OnnxRuntimeCudaEngine.h"

using visionlab::createInferenceEngine;
using visionlab::InferenceBackend;
using visionlab::InferencePrecision;
using visionlab::InferResult;
using visionlab::ModelConfig;
using visionlab::OnnxRuntimeCudaEngine;
using visionlab::TensorView;

namespace {

std::filesystem::path identityModelPath()
{
    return std::filesystem::path(VISIONLAB_TEST_FIXTURES) / "identity_f32_1x3x2x2.onnx";
}

bool lastErrorLooksLikeCudaFailure(const std::string& error)
{
    const QString text = QString::fromStdString(error);
    return text.contains(QStringLiteral("CUDA"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("cuda"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("device"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("GPU"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("provider"), Qt::CaseInsensitive);
}

} // namespace

class OnnxRuntimeCudaEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void backendIdAndDeviceAreCuda();
    void missingModelReportsPath();
    void rejectsNonCudaBackend();
    void rejectsNonFp32Precision();
    void factoryReturnsCudaEngine();
    void initializeFailureIsNotCpuSession();
    void identityModelRoundTripIfCudaAvailable();
};

void OnnxRuntimeCudaEngineTest::backendIdAndDeviceAreCuda()
{
    OnnxRuntimeCudaEngine engine;
    QCOMPARE(engine.backendId(), std::string("onnxruntime-cuda"));
    QVERIFY(engine.device().gpu);
}

void OnnxRuntimeCudaEngineTest::missingModelReportsPath()
{
    ModelConfig config;
    config.backend = InferenceBackend::OnnxRuntimeCuda;
    config.modelPath = std::filesystem::path("Z:/visionlab-missing-cuda-model.onnx");

    OnnxRuntimeCudaEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("visionlab-missing-cuda-model.onnx")));
}

void OnnxRuntimeCudaEngineTest::rejectsNonCudaBackend()
{
    ModelConfig config;
    config.modelPath = identityModelPath();
    config.backend = InferenceBackend::OnnxRuntimeCpu;

    OnnxRuntimeCudaEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("OnnxRuntimeCuda"))
            || error.contains(QStringLiteral("onnxruntime-cuda")));
}

void OnnxRuntimeCudaEngineTest::rejectsNonFp32Precision()
{
    ModelConfig config;
    config.modelPath = identityModelPath();
    config.backend = InferenceBackend::OnnxRuntimeCuda;
    config.precision = InferencePrecision::Fp16;

    OnnxRuntimeCudaEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("Fp32"))
            || error.contains(QStringLiteral("precision")));
}

void OnnxRuntimeCudaEngineTest::factoryReturnsCudaEngine()
{
    ModelConfig config;
    config.backend = InferenceBackend::OnnxRuntimeCuda;
    const auto engine = createInferenceEngine(config);
    QVERIFY(engine);
    QCOMPARE(engine->backendId(), std::string("onnxruntime-cuda"));
}

void OnnxRuntimeCudaEngineTest::initializeFailureIsNotCpuSession()
{
    const auto path = identityModelPath();
    if (!std::filesystem::exists(path))
        QSKIP("identity ONNX fixture is not present");

    ModelConfig config;
    config.modelPath = path;
    config.backend = InferenceBackend::OnnxRuntimeCuda;
    config.deviceId = 0;

    OnnxRuntimeCudaEngine engine;
    const bool ok = engine.initialize(config);
    QCOMPARE(engine.backendId(), std::string("onnxruntime-cuda"));
    QVERIFY(engine.device().gpu);
    if (ok)
        return;

    QVERIFY(!engine.isReady());
    QVERIFY(lastErrorLooksLikeCudaFailure(engine.lastError()));
    QVERIFY(engine.backendId() != std::string("onnxruntime-cpu"));
}

void OnnxRuntimeCudaEngineTest::identityModelRoundTripIfCudaAvailable()
{
    const auto path = identityModelPath();
    if (!std::filesystem::exists(path))
        QSKIP("identity ONNX fixture is not present");

    ModelConfig config;
    config.modelPath = path;
    config.backend = InferenceBackend::OnnxRuntimeCuda;

    OnnxRuntimeCudaEngine engine;
    if (!engine.initialize(config) || !engine.isReady())
        QSKIP(engine.lastError().c_str());

    const auto inputMeta = engine.inputMetadata();
    QCOMPARE(inputMeta.dtype, std::string("float32"));
    const std::vector<std::int64_t> expectedShape{1, 3, 2, 2};
    QCOMPARE(inputMeta.shape, expectedShape);

    TensorView input;
    input.shape = expectedShape;
    input.data.assign(12, 0.25F);

    engine.warmup(1);

    const InferResult result = engine.infer(input);
    QVERIFY2(result.ok, result.error.c_str());
    QCOMPARE(result.outputs.size(), size_t{1});
    QCOMPARE(result.outputs.front().shape, expectedShape);
    QCOMPARE(result.outputs.front().data.size(), size_t{12});
    QCOMPARE(result.outputs.front().data.front(), 0.25F);
    QVERIFY(result.latencyMs >= 0.0);
}

QTEST_APPLESS_MAIN(OnnxRuntimeCudaEngineTest)

#include "OnnxRuntimeCudaEngineTest.moc"
