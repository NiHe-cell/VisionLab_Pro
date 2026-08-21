#include <QtTest/QtTest>

#include <filesystem>
#include <string>

#include "inference/InferenceEngineFactory.h"
#include "inference/InferenceTypes.h"
#include "inference/ModelConfig.h"

using visionlab::createInferenceEngine;
using visionlab::InferenceBackend;
using visionlab::InferencePrecision;
using visionlab::ModelConfig;
using visionlab::parseInferenceBackend;
using visionlab::parseInferencePrecision;

namespace {

std::filesystem::path identityModelPath()
{
    return std::filesystem::path(VISIONLAB_TEST_FIXTURES) / "identity_f32_1x3x2x2.onnx";
}

} // namespace

class InferenceEngineFactoryTest : public QObject
{
    Q_OBJECT

private slots:
    void cpuIdentityInitializes();
    void cudaBackendIsUnavailableObject();
    void tensorRtBackendIsUnavailableObject();
    void neverReturnsNull();
    void parseBackendAliases();
    void parsePrecisionAliases();
};

void InferenceEngineFactoryTest::cpuIdentityInitializes()
{
    const auto path = identityModelPath();
    if (!std::filesystem::exists(path))
        QSKIP("identity ONNX fixture is not present");

    ModelConfig config;
    config.modelPath = path;
    config.backend = InferenceBackend::OnnxRuntimeCpu;

    const auto engine = createInferenceEngine(config);
    QVERIFY(engine);
    QCOMPARE(engine->backendId(), std::string("onnxruntime-cpu"));
    QVERIFY2(engine->initialize(config), engine->lastError().c_str());
    QVERIFY(engine->isReady());
}

void InferenceEngineFactoryTest::cudaBackendIsUnavailableObject()
{
    ModelConfig config;
    config.modelPath = identityModelPath();
    config.backend = InferenceBackend::OnnxRuntimeCuda;

    const auto engine = createInferenceEngine(config);
    QVERIFY(engine);
    QVERIFY(!engine->initialize(config));
    QVERIFY(!engine->isReady());
    const QString error = QString::fromStdString(engine->lastError());
    QVERIFY(!error.contains(QStringLiteral("not implemented")));
    QVERIFY(error.contains(QStringLiteral("OnnxRuntimeCuda"), Qt::CaseInsensitive)
            || error.contains(QStringLiteral("onnxruntime-cuda"), Qt::CaseInsensitive));
    QVERIFY(error.contains(QStringLiteral("not available"), Qt::CaseInsensitive));
}

void InferenceEngineFactoryTest::tensorRtBackendIsUnavailableObject()
{
    ModelConfig config;
    config.modelPath = identityModelPath();
    config.backend = InferenceBackend::TensorRT;

    const auto engine = createInferenceEngine(config);
    QVERIFY(engine);
    QVERIFY(!engine->initialize(config));
    QVERIFY(!engine->isReady());
    const QString error = QString::fromStdString(engine->lastError());
    QVERIFY(error.contains(QStringLiteral("TensorRT"), Qt::CaseInsensitive)
            || error.contains(QStringLiteral("tensorrt"), Qt::CaseInsensitive));
    QVERIFY(error.contains(QStringLiteral("not available"), Qt::CaseInsensitive)
            || error.contains(QStringLiteral("unavailable"), Qt::CaseInsensitive));
}

void InferenceEngineFactoryTest::neverReturnsNull()
{
    ModelConfig config;
    config.backend = InferenceBackend::TensorRT;
    QVERIFY(createInferenceEngine(config));
}

void InferenceEngineFactoryTest::parseBackendAliases()
{
    QVERIFY(parseInferenceBackend("onnx-cpu") == InferenceBackend::OnnxRuntimeCpu);
    QVERIFY(parseInferenceBackend("onnxruntime-cpu") == InferenceBackend::OnnxRuntimeCpu);
    QVERIFY(parseInferenceBackend("onnx-cuda") == InferenceBackend::OnnxRuntimeCuda);
    QVERIFY(parseInferenceBackend("onnxruntime-cuda") == InferenceBackend::OnnxRuntimeCuda);
    QVERIFY(parseInferenceBackend("tensorrt") == InferenceBackend::TensorRT);
    QVERIFY(parseInferenceBackend("trt") == InferenceBackend::TensorRT);
    QVERIFY(!parseInferenceBackend("bogus").has_value());
    QVERIFY(!parseInferenceBackend("").has_value());
}

void InferenceEngineFactoryTest::parsePrecisionAliases()
{
    QVERIFY(parseInferencePrecision("fp32") == InferencePrecision::Fp32);
    QVERIFY(parseInferencePrecision("fp16") == InferencePrecision::Fp16);
    QVERIFY(!parseInferencePrecision("int8").has_value());
    QVERIFY(!parseInferencePrecision("bogus").has_value());
}

QTEST_APPLESS_MAIN(InferenceEngineFactoryTest)

#include "InferenceEngineFactoryTest.moc"
