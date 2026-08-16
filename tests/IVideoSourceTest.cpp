#include <QtTest/QtTest>

#include <memory>

#include "fakes/FakeVideoSource.h"
#include "video/IVideoSource.h"

using visionlab::IVideoSource;

class IVideoSourceTest : public QObject
{
    Q_OBJECT

private slots:
    void lifecycleContract();
    void readBeforeOpenFails();
    void endOfStreamIsReported();
    void polymorphicUse();
};

void IVideoSourceTest::lifecycleContract()
{
    FakeVideoSource source(2);

    QVERIFY(!source.isOpen());
    QVERIFY(source.open());
    QVERIFY(source.isOpen());
    QCOMPARE(source.sourceId(), std::string("fake:0"));

    source.close();
    QVERIFY(!source.isOpen());
    source.close(); // 幂等：重复关闭安全
    QVERIFY(!source.isOpen());
}

void IVideoSourceTest::readBeforeOpenFails()
{
    FakeVideoSource source(1);

    cv::Mat frame;
    QVERIFY(!source.read(frame));
    QVERIFY(!source.lastError().empty());
}

void IVideoSourceTest::endOfStreamIsReported()
{
    FakeVideoSource source(2);
    QVERIFY(source.open());

    cv::Mat frame;
    QVERIFY(source.read(frame));
    QVERIFY(!frame.empty());
    QVERIFY(source.read(frame));

    // 流耗尽：read 失败且 lastError 有描述。
    QVERIFY(!source.read(frame));
    QCOMPARE(source.lastError(), std::string("end of stream"));
}

void IVideoSourceTest::polymorphicUse()
{
    std::unique_ptr<IVideoSource> source = std::make_unique<FakeVideoSource>(1);
    QVERIFY(source->open());

    cv::Mat frame;
    QVERIFY(source->read(frame));
    QCOMPARE(frame.cols, 4);
    QCOMPARE(frame.rows, 4);

    source->close();
    QVERIFY(!source->isOpen());
}

QTEST_APPLESS_MAIN(IVideoSourceTest)

#include "IVideoSourceTest.moc"
