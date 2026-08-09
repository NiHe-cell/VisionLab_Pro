#include <QtTest/QtTest>

#include "video/CameraSource.h"

using visionlab::CameraSource;

// 本组用例不依赖真实摄像头：只验证错误路径与标识契约。
class CameraSourceTest : public QObject
{
    Q_OBJECT

private slots:
    void invalidDeviceFailsWithError();
    void readBeforeOpenFails();
    void sourceIdContainsIndex();
    void closeIsIdempotent();
};

void CameraSourceTest::invalidDeviceFailsWithError()
{
    CameraSource source(9999); // 不可能存在的设备索引

    QVERIFY(!source.open());
    QVERIFY(!source.lastError().empty());
    QVERIFY(!source.isOpen());
}

void CameraSourceTest::readBeforeOpenFails()
{
    CameraSource source(0);

    cv::Mat frame;
    QVERIFY(!source.read(frame));
    QVERIFY(!source.lastError().empty());
}

void CameraSourceTest::sourceIdContainsIndex()
{
    QCOMPARE(CameraSource(0).sourceId(), std::string("camera:0"));
    QCOMPARE(CameraSource(2).sourceId(), std::string("camera:2"));
}

void CameraSourceTest::closeIsIdempotent()
{
    CameraSource source(0);
    source.close(); // 未打开时关闭必须安全
    source.close();
    QVERIFY(!source.isOpen());
}

QTEST_APPLESS_MAIN(CameraSourceTest)

#include "CameraSourceTest.moc"
