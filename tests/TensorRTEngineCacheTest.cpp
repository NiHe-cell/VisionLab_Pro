#include <QtTest/QtTest>

#include <QTemporaryDir>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <cuda_runtime_api.h>

#include "inference/TensorRTEngine.h"

using visionlab::InferenceBackend;
using visionlab::InferencePrecision;
using visionlab::InferResult;
using visionlab::ModelConfig;
using visionlab::TensorRTEngine;
using visionlab::TensorView;

namespace {

std::filesystem::path identityFixturePath()
{
    return std::filesystem::path(VISIONLAB_TEST_FIXTURES) / "identity_f32_1x3x2x2.onnx";
}

bool cudaDeviceAvailable()
{
    int count = 0;
    return cudaGetDeviceCount(&count) == cudaSuccess && count >= 1;
}

std::filesystem::path cacheDirFor(const std::filesystem::path& onnx)
{
    return onnx.parent_path() / ".trt-cache";
}

std::vector<std::filesystem::path> engineFiles(const std::filesystem::path& cacheDir)
{
    std::vector<std::filesystem::path> files;
    std::error_code error;
    if (!std::filesystem::exists(cacheDir, error))
        return files;
    for (const auto& entry : std::filesystem::directory_iterator(cacheDir, error))
    {
        if (entry.path().extension() == ".engine")
            files.push_back(entry.path());
    }
    return files;
}

ModelConfig makeConfig(const std::filesystem::path& onnx, visionlab::InferencePrecision precision)
{
    ModelConfig config;
    config.modelPath = onnx;
    config.backend = InferenceBackend::TensorRT;
    config.precision = precision;
    config.inputWidth = 2;
    config.inputHeight = 2;
    config.deviceId = 0;
    return config;
}

std::filesystem::path copyIdentityTo(const QTemporaryDir& dir)
{
    const auto dest = std::filesystem::path(dir.path().toStdString()) / "identity_f32_1x3x2x2.onnx";
    std::filesystem::copy_file(identityFixturePath(), dest);
    return dest;
}

} // namespace

class TensorRTEngineCacheTest : public QObject
{
    Q_OBJECT

private slots:
    void initializeWritesFp32CacheFile();
    void dirtyCacheIsDeletedAndRebuilt();
    void wrongCacheNameIsIgnored();
    void cacheHitDoesNotNeedOnnxBytes();
    void fp16UsesHardwareOrFailsWithoutSubstitute();
    void rejectsInt8Precision();
};

void TensorRTEngineCacheTest::initializeWritesFp32CacheFile()
{
    if (!std::filesystem::exists(identityFixturePath()))
        QSKIP("identity ONNX fixture is not present");
    if (!cudaDeviceAvailable())
        QSKIP("no CUDA device");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto onnx = copyIdentityTo(dir);

    TensorRTEngine engine;
    QVERIFY2(engine.initialize(makeConfig(onnx, InferencePrecision::Fp32)),
             engine.lastError().c_str());
    QVERIFY(engine.isReady());

    const auto files = engineFiles(cacheDirFor(onnx));
    QCOMPARE(files.size(), size_t{1});
    const QString name = QString::fromStdString(files.front().filename().string());
    QVERIFY(name.contains(QStringLiteral("identity_f32_1x3x2x2")));
    QVERIFY(name.contains(QStringLiteral("-pfp32-")));
    QVERIFY(name.contains(QStringLiteral("-1x3x2x2.engine")));
    QVERIFY(name.contains(QStringLiteral("-v10.")));
    QVERIFY(std::filesystem::file_size(files.front()) > 0);
}

void TensorRTEngineCacheTest::dirtyCacheIsDeletedAndRebuilt()
{
    if (!std::filesystem::exists(identityFixturePath()))
        QSKIP("identity ONNX fixture is not present");
    if (!cudaDeviceAvailable())
        QSKIP("no CUDA device");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto onnx = copyIdentityTo(dir);

    TensorRTEngine first;
    QVERIFY2(first.initialize(makeConfig(onnx, InferencePrecision::Fp32)),
             first.lastError().c_str());
    const auto files = engineFiles(cacheDirFor(onnx));
    QCOMPARE(files.size(), size_t{1});
    const auto cachePath = files.front();
    const auto originalSize = std::filesystem::file_size(cachePath);
    QVERIFY(originalSize > 8);

    {
        std::ofstream out(cachePath, std::ios::binary | std::ios::trunc);
        out.write("junk", 4);
    }
    QCOMPARE(std::filesystem::file_size(cachePath), std::uintmax_t{4});

    TensorRTEngine second;
    QVERIFY2(second.initialize(makeConfig(onnx, InferencePrecision::Fp32)),
             second.lastError().c_str());
    QVERIFY(second.isReady());
    const auto rebuilt = engineFiles(cacheDirFor(onnx));
    QCOMPARE(rebuilt.size(), size_t{1});
    QVERIFY(std::filesystem::file_size(rebuilt.front()) > 8);
}

void TensorRTEngineCacheTest::wrongCacheNameIsIgnored()
{
    if (!std::filesystem::exists(identityFixturePath()))
        QSKIP("identity ONNX fixture is not present");
    if (!cudaDeviceAvailable())
        QSKIP("no CUDA device");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto onnx = copyIdentityTo(dir);
    const auto cacheDir = cacheDirFor(onnx);
    std::filesystem::create_directories(cacheDir);
    const auto bogus = cacheDir / "wrong-key.engine";
    {
        std::ofstream out(bogus, std::ios::binary);
        out.write("not-an-engine", 13);
    }

    TensorRTEngine engine;
    QVERIFY2(engine.initialize(makeConfig(onnx, InferencePrecision::Fp32)),
             engine.lastError().c_str());
    QVERIFY(engine.isReady());
    QVERIFY(std::filesystem::exists(bogus));
    QCOMPARE(std::filesystem::file_size(bogus), std::uintmax_t{13});

    const auto files = engineFiles(cacheDir);
    QCOMPARE(files.size(), size_t{2});
}

void TensorRTEngineCacheTest::cacheHitDoesNotNeedOnnxBytes()
{
    if (!std::filesystem::exists(identityFixturePath()))
        QSKIP("identity ONNX fixture is not present");
    if (!cudaDeviceAvailable())
        QSKIP("no CUDA device");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto onnx = copyIdentityTo(dir);

    TensorRTEngine first;
    QVERIFY2(first.initialize(makeConfig(onnx, InferencePrecision::Fp32)),
             first.lastError().c_str());
    QCOMPARE(engineFiles(cacheDirFor(onnx)).size(), size_t{1});

    const auto size = std::filesystem::file_size(onnx);
    const auto mtime = std::filesystem::last_write_time(onnx);
    {
        std::vector<char> junk(static_cast<std::size_t>(size), '\x7F');
        std::ofstream out(onnx, std::ios::binary | std::ios::trunc);
        out.write(junk.data(), static_cast<std::streamsize>(junk.size()));
    }
    std::filesystem::last_write_time(onnx, mtime);
    QCOMPARE(std::filesystem::file_size(onnx), size);

    TensorRTEngine second;
    QVERIFY2(second.initialize(makeConfig(onnx, InferencePrecision::Fp32)),
             second.lastError().c_str());
    QVERIFY(second.isReady());

    TensorView input;
    input.shape = {1, 3, 2, 2};
    input.data.assign(12, 0.25F);
    const InferResult result = second.infer(input);
    QVERIFY2(result.ok, result.error.c_str());
    QCOMPARE(result.outputs.front().data.size(), size_t{12});
}

void TensorRTEngineCacheTest::fp16UsesHardwareOrFailsWithoutSubstitute()
{
    if (!std::filesystem::exists(identityFixturePath()))
        QSKIP("identity ONNX fixture is not present");
    if (!cudaDeviceAvailable())
        QSKIP("no CUDA device");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto onnx = copyIdentityTo(dir);

    TensorRTEngine engine;
    const bool ok = engine.initialize(makeConfig(onnx, InferencePrecision::Fp16));
    if (!ok)
    {
        QVERIFY(!engine.isReady());
        const QString error = QString::fromStdString(engine.lastError());
        QVERIFY(error.contains(QStringLiteral("Fp16"), Qt::CaseInsensitive)
                || error.contains(QStringLiteral("FP16")));
        QVERIFY(error.contains(QStringLiteral("not supported"), Qt::CaseInsensitive));
        QVERIFY(!error.contains(QStringLiteral("T07")));
        return;
    }

    QVERIFY(engine.isReady());
    const auto files = engineFiles(cacheDirFor(onnx));
    QCOMPARE(files.size(), size_t{1});
    QVERIFY(QString::fromStdString(files.front().filename().string())
                .contains(QStringLiteral("-pfp16-")));

    TensorView input;
    input.shape = {1, 3, 2, 2};
    input.data.assign(12, 0.25F);
    const InferResult result = engine.infer(input);
    QVERIFY2(result.ok, result.error.c_str());
    QCOMPARE(result.outputs.front().data.size(), size_t{12});
    for (float value : result.outputs.front().data)
        QVERIFY(std::abs(value - 0.25F) < 1e-2F);
}

void TensorRTEngineCacheTest::rejectsInt8Precision()
{
    ModelConfig config = makeConfig(identityFixturePath(), InferencePrecision::Int8);
    config.backend = InferenceBackend::TensorRT;

    TensorRTEngine engine;
    QVERIFY(!engine.initialize(config));
    QVERIFY(!engine.isReady());
    const QString error = QString::fromStdString(engine.lastError());
    QVERIFY(error.contains(QStringLiteral("Int8"), Qt::CaseInsensitive)
            || error.contains(QStringLiteral("Fp32"))
            || error.contains(QStringLiteral("precision")));
}

QTEST_APPLESS_MAIN(TensorRTEngineCacheTest)

#include "TensorRTEngineCacheTest.moc"
