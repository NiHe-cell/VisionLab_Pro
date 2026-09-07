#include <QtTest/QtTest>

#include <QProcess>
#include <QString>

#include "PipelineBenchmarkPaths.h"

class PipelineBenchmarkTest : public QObject
{
    Q_OBJECT

private slots:
    void helpExitsZero();
    void negativeSecondsExitsWithoutDroppedKey();
    void zeroQueueExitsWithoutDroppedKey();
    void shortRunPrintsMetricFields();
    void zeroDetectMsSoakPrintsDroppedAndRunningFlag();
};

void PipelineBenchmarkTest::helpExitsZero()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_PIPELINE), {QStringLiteral("--help")});
    QVERIFY(process.waitForFinished(10000));
    QCOMPARE(process.exitCode(), 0);
}

void PipelineBenchmarkTest::negativeSecondsExitsWithoutDroppedKey()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_PIPELINE),
                  {QStringLiteral("--seconds"), QStringLiteral("-1")});
    QVERIFY(process.waitForFinished(10000));
    QVERIFY(process.exitCode() != 0);

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput())
                           + QString::fromLocal8Bit(process.readAllStandardError());
    QVERIFY(!output.contains(QStringLiteral("dropped:")));
}

void PipelineBenchmarkTest::zeroQueueExitsWithoutDroppedKey()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_PIPELINE),
                  {QStringLiteral("--queue"), QStringLiteral("0")});
    QVERIFY(process.waitForFinished(10000));
    QVERIFY(process.exitCode() != 0);

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput())
                           + QString::fromLocal8Bit(process.readAllStandardError());
    QVERIFY(!output.contains(QStringLiteral("dropped:")));
}

void PipelineBenchmarkTest::shortRunPrintsMetricFields()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_PIPELINE),
                  {QStringLiteral("--seconds"),
                   QStringLiteral("2"),
                   QStringLiteral("--detect-ms"),
                   QStringLiteral("50"),
                   QStringLiteral("--queue"),
                   QStringLiteral("2")});
    QVERIFY(process.waitForFinished(30000));
    QCOMPARE(process.exitCode(), 0);

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput())
                           + QString::fromLocal8Bit(process.readAllStandardError());
    QVERIFY(output.contains(QStringLiteral("backend: fake-pipeline")));
    QVERIFY(output.contains(QStringLiteral("seconds:")));
    QVERIFY(output.contains(QStringLiteral("queue_capacity:")));
    QVERIFY(output.contains(QStringLiteral("detect_ms:")));
    QVERIFY(output.contains(QStringLiteral("captured:")));
    QVERIFY(output.contains(QStringLiteral("processed:")));
    QVERIFY(output.contains(QStringLiteral("dropped:")));
    QVERIFY(output.contains(QStringLiteral("peak_queue_depth:")));
    QVERIFY(output.contains(QStringLiteral("e2e_p50_ms:")));
    QVERIFY(output.contains(QStringLiteral("e2e_p95_ms:")));
}

void PipelineBenchmarkTest::zeroDetectMsSoakPrintsDroppedAndRunningFlag()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_PIPELINE),
                  {QStringLiteral("--detect-ms"),
                   QStringLiteral("0"),
                   QStringLiteral("--seconds"),
                   QStringLiteral("2")});
    QVERIFY(process.waitForFinished(30000));
    QCOMPARE(process.exitCode(), 0);

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput())
                           + QString::fromLocal8Bit(process.readAllStandardError());
    QVERIFY(output.contains(QStringLiteral("dropped:")));
    QVERIFY(output.contains(QStringLiteral("still_running_before_stop:")));
}

QTEST_APPLESS_MAIN(PipelineBenchmarkTest)

#include "PipelineBenchmarkTest.moc"
