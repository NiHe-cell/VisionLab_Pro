#include <QtTest/QtTest>

#include <QProcess>
#include <QString>

#include "OnnxCpuBenchmarkPaths.h"

class OnnxCpuBenchmarkTest : public QObject
{
    Q_OBJECT

private slots:
    void missingModelExitsNonZeroWithoutMetrics();
    void identityFixturePrintsMetricFields();
    void unknownBackendExitsNonZeroWithoutMetrics();
};

void OnnxCpuBenchmarkTest::missingModelExitsNonZeroWithoutMetrics()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_ONNX_CPU),
                  {QStringLiteral("--model"),
                   QStringLiteral("Z:/visionlab-missing-model.onnx"),
                   QStringLiteral("--warmup"),
                   QStringLiteral("0"),
                   QStringLiteral("--iters"),
                   QStringLiteral("1")});
    QVERIFY(process.waitForFinished(10000));
    QVERIFY(process.exitCode() != 0);

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput())
                           + QString::fromLocal8Bit(process.readAllStandardError());
    QVERIFY(output.contains(QStringLiteral("visionlab-missing-model.onnx")));
    QVERIFY(!output.contains(QStringLiteral("mean_ms:")));
    QVERIFY(!output.contains(QStringLiteral("throughput_fps:")));
}

void OnnxCpuBenchmarkTest::identityFixturePrintsMetricFields()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_ONNX_CPU),
                  {QStringLiteral("--model"),
                   QString::fromUtf8(VISIONLAB_IDENTITY_ONNX),
                   QStringLiteral("--warmup"),
                   QStringLiteral("1"),
                   QStringLiteral("--iters"),
                   QStringLiteral("3")});
    QVERIFY(process.waitForFinished(30000));
    QCOMPARE(process.exitCode(), 0);

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput())
                           + QString::fromLocal8Bit(process.readAllStandardError());
    QVERIFY(output.contains(QStringLiteral("backend: onnxruntime-cpu")));
    QVERIFY(output.contains(QStringLiteral("warmup: 1")));
    QVERIFY(output.contains(QStringLiteral("iters: 3")));
    QVERIFY(output.contains(QStringLiteral("mean_ms:")));
    QVERIFY(output.contains(QStringLiteral("p50_ms:")));
    QVERIFY(output.contains(QStringLiteral("p95_ms:")));
    QVERIFY(output.contains(QStringLiteral("p99_ms:")));
    QVERIFY(output.contains(QStringLiteral("throughput_fps:")));
}

void OnnxCpuBenchmarkTest::unknownBackendExitsNonZeroWithoutMetrics()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_ONNX_CPU),
                  {QStringLiteral("--backend"),
                   QStringLiteral("not-a-backend"),
                   QStringLiteral("--model"),
                   QString::fromUtf8(VISIONLAB_IDENTITY_ONNX),
                   QStringLiteral("--warmup"),
                   QStringLiteral("0"),
                   QStringLiteral("--iters"),
                   QStringLiteral("1")});
    QVERIFY(process.waitForFinished(10000));
    QVERIFY(process.exitCode() != 0);

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput())
                           + QString::fromLocal8Bit(process.readAllStandardError());
    QVERIFY(!output.contains(QStringLiteral("mean_ms:")));
    QVERIFY(!output.contains(QStringLiteral("throughput_fps:")));
}

QTEST_APPLESS_MAIN(OnnxCpuBenchmarkTest)

#include "OnnxCpuBenchmarkTest.moc"
