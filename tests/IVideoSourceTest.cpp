#include <QtTest/QtTest>

#include <memory>

#include "video/IVideoSource.h"

using visionlab::IVideoSource;

namespace {

// 内存假视频源：按脚本产出固定数量的合成帧，用于锁定接口契约。
class FakeVideoSource : public IVideoSource
{
public:
    explicit FakeVideoSource(int frameCount, std::string id = "fake:0")
        : m_frameCount(frameCount), m_id(std::move(id))
    {
    }

    bool open() override
    {
        m_open = true;
        m_readCount = 0;
        m_lastError.clear();
        return true;
    }

    bool read(cv::Mat& frame) override
    {
        if (!m_open)
        {
            m_lastError = "read before open";
            return false;
        }
        if (m_readCount >= m_frameCount)
        {
            m_lastError = "end of stream";
            return false;
        }
        // 每帧填充不同的灰度值，便于区分帧内容。
        frame = cv::Mat(4, 4, CV_8UC1, cv::Scalar(m_readCount % 255));
        ++m_readCount;
        return true;
    }

    void close() override { m_open = false; }

    bool isOpen() const override { return m_open; }
    std::string sourceId() const override { return m_id; }
    std::string lastError() const override { return m_lastError; }

private:
    int m_frameCount;
    std::string m_id;
    bool m_open = false;
    int m_readCount = 0;
    std::string m_lastError;
};

} // namespace

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
