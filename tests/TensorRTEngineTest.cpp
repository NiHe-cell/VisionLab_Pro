#include <QtTest/QtTest>

#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include <cuda_runtime_api.h>

#include "inference/InferenceEngineFactory.h"
#include "inference/TensorRTEngine.h"

using visionlab::createInferenceEngine;
using visionlab::InferenceBackend;
using visionlab::InferencePrecision;
using visionlab::InferResult;
using visionlab::ModelConfig;
using visionlab::TensorRTEngine;
using visionlab::TensorView;

namespace {

std::filesystem::path identityModelPath()
{
    return std::filesystem::path(VISIONLAB_TEST_FIXTURES) / "identity_f32_1x3x2x2.onnx";
}

std::filesystem::path dynamicModelPath()
{
    return std::filesystem::path(VISIONLAB_TEST_FIXTURES) / "dynamic_f32_1x3xHxW.onnx";
}

std::filesystem::path yoloModelPath()
{
    return std::filesystem::path(VISIONLAB_MODELS_DIR) / "yolov4-tiny.onnx";
}

bool cudaDeviceAvailable()
{
    int count = 0;
    return cudaGetDeviceCount(&count) == cudaSuccess && count >= 1;
}

ModelConfig identityConfig()
{
    ModelConfig config;
    config.modelPath = identityModelPath();
    config.backend = InferenceBackend::TensorRT;
    config.precision = InferencePrecision::Fp32;
    config.inputWidth = 2;
    config.inputHeight = 2;
    config.deviceId = 0;
    return config;
}

} // namespace

class TensorRTEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void backendIdAndDeviceAreGpu();
    void missingModelReportsPath();
    void rejectsNonTensorRtBackend();
    void rejectsFp16UntilT07();
    void rejectsDynamicShape();
    void factoryReturnsTensorRtEngine();
    void identityModelRoundTripIfGpuAvailable();
    void optionalYoloInitializeDoesNotCrash();
};

void TensorRTEngineTest::backendIdAndDeviceAreGpu()
{
    TensorRTEngine engine;
    QCOMPARE(engine.backendId(), std::string("tensorrt"));
    QVERIFY(engine.device().gpu);
}

void TensorRTEngineTest::missingModelReportsPath()
{
    ModelConfig config;
    config.backend = InferenceBackend::TensorRT;
    config.modelPath = std::filesystem::path("Z:/visionlab-missing-trt-model.onnx");

    TensorRTEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("visionlab-missing-trt-model.onnx")));
}

void TensorRTEngineTest::rejectsNonTensorRtBackend()
{
    ModelConfig config = identityConfig();
    config.backend = InferenceBackend::OnnxRuntimeCpu;

    TensorRTEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("TensorRT"))
            || error.contains(QStringLiteral("tensorrt")));
}

void TensorRTEngineTest::rejectsFp16UntilT07()
{
    ModelConfig config = identityConfig();
    config.precision = InferencePrecision::Fp16;

    TensorRTEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("FP16"), Qt::CaseInsensitive)
            || error.contains(QStringLiteral("Fp16")));
    QVERIFY(error.contains(QStringLiteral("T07"))
            || error.contains(QStringLiteral("not enabled"), Qt::CaseInsensitive));
}

void TensorRTEngineTest::rejectsDynamicShape()
{
    const auto path = dynamicModelPath();
    if (!std::filesystem::exists(path))
        QSKIP("dynamic ONNX fixture is not present");

    ModelConfig config;
    config.modelPath = path;
    config.backend = InferenceBackend::TensorRT;
    config.precision = InferencePrecision::Fp32;
    config.inputWidth = 2;
    config.inputHeight = 2;

    TensorRTEngine engine;
    if (!cudaDeviceAvailable())
        QSKIP("no CUDA device");

    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("dynamic"), Qt::CaseInsensitive));
}

void TensorRTEngineTest::factoryReturnsTensorRtEngine()
{
    ModelConfig config;
    config.backend = InferenceBackend::TensorRT;
    const auto engine = createInferenceEngine(config);
    QVERIFY(engine);
    QCOMPARE(engine->backendId(), std::string("tensorrt"));
    QVERIFY(engine->lastError().empty());
}

void TensorRTEngineTest::identityModelRoundTripIfGpuAvailable()
{
    const auto path = identityModelPath();
    if (!std::filesystem::exists(path))
        QSKIP("identity ONNX fixture is not present");
    if (!cudaDeviceAvailable())
        QSKIP("no CUDA device");

    ModelConfig config = identityConfig();
    TensorRTEngine engine;
    QVERIFY2(engine.initialize(config), engine.lastError().c_str());
    QVERIFY(engine.isReady());

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
    for (std::size_t i = 0; i < result.outputs.front().data.size(); ++i)
    {
        QVERIFY2(std::abs(result.outputs.front().data[i] - 0.25F) < 1e-5F,
                 qPrintable(QStringLiteral("index %1 value %2")
                                .arg(i)
                                .arg(double(result.outputs.front().data[i]))));
    }
    QVERIFY(result.latencyMs >= 0.0);
}

void TensorRTEngineTest::optionalYoloInitializeDoesNotCrash()
{
    const auto path = yoloModelPath();
    if (!std::filesystem::exists(path))
        QSKIP("yolov4-tiny.onnx is not in models/");
    if (!cudaDeviceAvailable())
        QSKIP("no CUDA device");

    ModelConfig config;
    config.modelPath = path;
    config.backend = InferenceBackend::TensorRT;
    config.precision = InferencePrecision::Fp32;
    config.inputWidth = 320;
    config.inputHeight = 320;

    TensorRTEngine engine;
    const bool ok = engine.initialize(config);
    if (!ok)
    {
        QVERIFY(!engine.isReady());
        QVERIFY(!engine.lastError().empty());
        return;
    }

    QVERIFY(engine.isReady());
}

QTEST_APPLESS_MAIN(TensorRTEngineTest)

#include "TensorRTEngineTest.moc"
