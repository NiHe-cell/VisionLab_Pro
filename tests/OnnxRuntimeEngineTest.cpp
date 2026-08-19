#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <filesystem>
#include <fstream>

#include "inference/OnnxRuntimeEngine.h"

using visionlab::InferenceBackend;
using visionlab::InferencePrecision;
using visionlab::InferResult;
using visionlab::ModelConfig;
using visionlab::OnnxRuntimeEngine;
using visionlab::TensorView;

namespace {

std::filesystem::path writeTemp(const QByteArray& bytes, const QString& suffix)
{
    const QString path = QDir::temp().filePath(
        QStringLiteral("visionlab-ort-%1-%2").arg(QTest::currentTestFunction(), suffix));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return {};
    file.write(bytes);
    file.close();
    return std::filesystem::path(file.fileName().toStdString());
}

std::filesystem::path identityModelPath()
{
    return std::filesystem::path(VISIONLAB_TEST_FIXTURES) / "identity_f32_1x3x2x2.onnx";
}

std::filesystem::path yoloModelPath()
{
    return std::filesystem::path(VISIONLAB_MODELS_DIR) / "yolov4-tiny.onnx";
}

} // namespace

class OnnxRuntimeEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void backendIdAndDeviceAreCpu();
    void missingModelReportsPath();
    void garbageModelFailsInitialize();
    void rejectsNonCpuBackend();
    void rejectsNonFp32Precision();
    void inferBeforeInitializeFails();
    void identityModelRoundTrip();
    void optionalYoloModelLoadsIfPresent();
};

void OnnxRuntimeEngineTest::backendIdAndDeviceAreCpu()
{
    OnnxRuntimeEngine engine;
    QCOMPARE(engine.backendId(), std::string("onnxruntime-cpu"));
    QCOMPARE(engine.device().name, std::string("CPU"));
    QVERIFY(!engine.device().gpu);
    QCOMPARE(engine.device().deviceId, 0);
}

void OnnxRuntimeEngineTest::missingModelReportsPath()
{
    ModelConfig config;
    config.modelPath = std::filesystem::path("Z:/visionlab-missing-model.onnx");

    OnnxRuntimeEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("visionlab-missing-model.onnx")));
}

void OnnxRuntimeEngineTest::garbageModelFailsInitialize()
{
    const auto path = writeTemp(QByteArray("this is not onnx"), QStringLiteral("garbage.onnx"));
    QVERIFY(!path.empty());

    ModelConfig config;
    config.modelPath = path;

    OnnxRuntimeEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    QVERIFY(!engine.lastError().empty());
    QVERIFY(engine.lastError().find("not implemented") == std::string::npos);
}

void OnnxRuntimeEngineTest::rejectsNonCpuBackend()
{
    ModelConfig config;
    config.modelPath = identityModelPath();
    config.backend = InferenceBackend::TensorRT;

    OnnxRuntimeEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("OnnxRuntimeCpu"))
            || error.contains(QStringLiteral("onnxruntime-cpu")));
}

void OnnxRuntimeEngineTest::rejectsNonFp32Precision()
{
    ModelConfig config;
    config.modelPath = identityModelPath();
    config.precision = InferencePrecision::Fp16;

    OnnxRuntimeEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("Fp32"))
            || error.contains(QStringLiteral("precision")));
}

void OnnxRuntimeEngineTest::inferBeforeInitializeFails()
{
    OnnxRuntimeEngine engine;
    const InferResult result = engine.infer(TensorView{});
    QVERIFY(!result.ok);
    QVERIFY(!result.error.empty());
    QVERIFY(!engine.isReady());
}

void OnnxRuntimeEngineTest::identityModelRoundTrip()
{
    const auto path = identityModelPath();
    if (!std::filesystem::exists(path))
        QSKIP("identity ONNX fixture is not present");

    ModelConfig config;
    config.modelPath = path;

    OnnxRuntimeEngine engine;
    QVERIFY2(engine.initialize(config), engine.lastError().c_str());
    QVERIFY(engine.isReady());

    const auto inputMeta = engine.inputMetadata();
    QCOMPARE(inputMeta.dtype, std::string("float32"));
    const std::vector<std::int64_t> expectedShape{1, 3, 2, 2};
    QCOMPARE(inputMeta.shape, expectedShape);

    TensorView input;
    input.shape = expectedShape;
    input.data.assign(12, 0.25F);

    engine.warmup(2);

    const InferResult result = engine.infer(input);
    QVERIFY2(result.ok, result.error.c_str());
    QCOMPARE(result.outputs.size(), size_t{1});
    QCOMPARE(result.outputs.front().shape, expectedShape);
    QCOMPARE(result.outputs.front().data.size(), size_t{12});
    QCOMPARE(result.outputs.front().data.front(), 0.25F);
    QVERIFY(result.latencyMs >= 0.0);
}

void OnnxRuntimeEngineTest::optionalYoloModelLoadsIfPresent()
{
    const auto path = yoloModelPath();
    if (!std::filesystem::exists(path))
        QSKIP("yolov4-tiny.onnx is not in models/");

    ModelConfig config;
    config.modelPath = path;

    OnnxRuntimeEngine engine;
    QVERIFY2(engine.initialize(config), engine.lastError().c_str());
    QVERIFY(engine.isReady());

    TensorView input;
    input.shape = engine.inputMetadata().shape;
    std::size_t count = 1;
    for (const std::int64_t dim : input.shape)
        count *= static_cast<std::size_t>(dim);
    input.data.assign(count, 0.0F);

    engine.warmup(1);
    const InferResult result = engine.infer(input);
    QVERIFY2(result.ok, result.error.c_str());
    QVERIFY(!result.outputs.empty());
}

QTEST_APPLESS_MAIN(OnnxRuntimeEngineTest)

#include "OnnxRuntimeEngineTest.moc"
