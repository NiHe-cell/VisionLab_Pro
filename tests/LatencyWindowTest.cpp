#include <QtTest/QtTest>

#include <stdexcept>

#include "core/LatencyWindow.h"

using visionlab::LatencyWindow;

class LatencyWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void capacityMustBeAtLeastOne();
    void emptyMeanAndPercentileAreZero();
    void knownSequenceMeanAndPercentiles();
    void ringOverwritesOldest();
    void resetClearsSamples();
};

void LatencyWindowTest::capacityMustBeAtLeastOne()
{
    QVERIFY_EXCEPTION_THROWN(LatencyWindow(0), std::invalid_argument);
}

void LatencyWindowTest::emptyMeanAndPercentileAreZero()
{
    const LatencyWindow window(8);
    QCOMPARE(window.size(), std::size_t(0));
    QCOMPARE(window.mean(), 0.0);
    QCOMPARE(window.percentile(50.0), 0.0);
    QCOMPARE(window.percentile(95.0), 0.0);
}

void LatencyWindowTest::knownSequenceMeanAndPercentiles()
{
    LatencyWindow window(8);
    for (double ms : {10.0, 20.0, 30.0, 40.0, 50.0})
        window.record(ms);

    QCOMPARE(window.size(), std::size_t(5));
    QCOMPARE(window.mean(), 30.0);
    QCOMPARE(window.percentile(50.0), 30.0);
    // rank = 0.95 * 4 = 3.8 → 40 + 0.8 * (50-40) = 48
    QCOMPARE(window.percentile(95.0), 48.0);
}

void LatencyWindowTest::ringOverwritesOldest()
{
    LatencyWindow window(3);
    window.record(1.0);
    window.record(2.0);
    window.record(3.0);
    window.record(4.0); // 丢掉 1，留下 2,3,4

    QCOMPARE(window.size(), std::size_t(3));
    QCOMPARE(window.mean(), 3.0);
}

void LatencyWindowTest::resetClearsSamples()
{
    LatencyWindow window(4);
    window.record(9.0);
    window.reset();
    QCOMPARE(window.size(), std::size_t(0));
    QCOMPARE(window.mean(), 0.0);
}

QTEST_APPLESS_MAIN(LatencyWindowTest)

#include "LatencyWindowTest.moc"
