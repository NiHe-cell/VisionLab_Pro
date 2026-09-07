#include <QtTest/QtTest>

#include <QProcess>
#include <QString>

#include "TrackerBenchmarkPaths.h"

class TrackerBenchmarkTest : public QObject
{
    Q_OBJECT

private slots:
    void unknownFlagExitsWithoutMeanKey();
    void shortRunPrintsMetricFields();
};

void TrackerBenchmarkTest::unknownFlagExitsWithoutMeanKey()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_TRACKER), {QStringLiteral("--not-a-flag")});
    QVERIFY(process.waitForFinished(10000));
    QVERIFY(process.exitCode() != 0);

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput())
                           + QString::fromLocal8Bit(process.readAllStandardError());
    QVERIFY(!output.contains(QStringLiteral("mean_ms:")));
}

void TrackerBenchmarkTest::shortRunPrintsMetricFields()
{
    QProcess process;
    process.start(QString::fromUtf8(BENCH_TRACKER),
                  {QStringLiteral("--frames"),
                   QStringLiteral("5"),
                   QStringLiteral("--dets"),
                   QStringLiteral("2"),
                   QStringLiteral("--warmup"),
                   QStringLiteral("1")});
    QVERIFY(process.waitForFinished(15000));
    QCOMPARE(process.exitCode(), 0);

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput())
                           + QString::fromLocal8Bit(process.readAllStandardError());
    QVERIFY(output.contains(QStringLiteral("backend: bytetrack")));
    QVERIFY(output.contains(QStringLiteral("frames:")));
    QVERIFY(output.contains(QStringLiteral("dets:")));
    QVERIFY(output.contains(QStringLiteral("warmup:")));
    QVERIFY(output.contains(QStringLiteral("mean_ms:")));
    QVERIFY(output.contains(QStringLiteral("p50_ms:")));
    QVERIFY(output.contains(QStringLiteral("p95_ms:")));
    QVERIFY(output.contains(QStringLiteral("active_tracks:")));
}

QTEST_APPLESS_MAIN(TrackerBenchmarkTest)

#include "TrackerBenchmarkTest.moc"
