#include <QtTest/QtTest>

class SmokeTest : public QObject
{
    Q_OBJECT

private slots:
    void sanity();
};

void SmokeTest::sanity()
{
    QVERIFY(true);
    QCOMPARE(1 + 1, 2);
}

QTEST_APPLESS_MAIN(SmokeTest)

#include "SmokeTest.moc"
