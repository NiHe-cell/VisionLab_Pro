#include <QtTest/QtTest>

#include "rendering/Letterbox.h"

using visionlab::Letterbox;
using visionlab::computeLetterbox;
using visionlab::frameToItem;
using visionlab::itemToFrame;

class LetterboxTest : public QObject
{
    Q_OBJECT

private slots:
    void squareFrameInWideItemIsCentered();
    void itemCornersMapToFrameCorners();
    void blackBarPointIsOutside();
    void squareItemAroundWideFrameHasSymmetricBars();
    void invalidSizesAreZeroAndRejectMapping();
    void roundTripInsideFrame();
};

void LetterboxTest::squareFrameInWideItemIsCentered()
{
    const Letterbox box = computeLetterbox(200.0F, 100.0F, 100, 100);
    QCOMPARE(box.offsetX, 50.0F);
    QCOMPARE(box.offsetY, 0.0F);
    QCOMPARE(box.scale, 1.0F);
    QCOMPARE(box.contentW, 100.0F);
    QCOMPARE(box.contentH, 100.0F);
}

void LetterboxTest::itemCornersMapToFrameCorners()
{
    const Letterbox box = computeLetterbox(200.0F, 100.0F, 100, 100);
    cv::Point2f origin;
    cv::Point2f far;
    QVERIFY(itemToFrame(box, 50.0F, 0.0F, 100, 100, origin));
    QCOMPARE(origin.x, 0.0F);
    QCOMPARE(origin.y, 0.0F);
    QVERIFY(itemToFrame(box, 150.0F, 100.0F, 100, 100, far));
    QCOMPARE(far.x, 100.0F);
    QCOMPARE(far.y, 100.0F);
}

void LetterboxTest::blackBarPointIsOutside()
{
    const Letterbox box = computeLetterbox(200.0F, 100.0F, 100, 100);
    cv::Point2f out{99.0F, 99.0F};
    QVERIFY(!itemToFrame(box, 0.0F, 0.0F, 100, 100, out));
    QCOMPARE(out.x, 99.0F);
    QCOMPARE(out.y, 99.0F);
}

void LetterboxTest::squareItemAroundWideFrameHasSymmetricBars()
{
    const Letterbox box = computeLetterbox(160.0F, 160.0F, 1920, 1080);
    QCOMPARE(box.offsetX, 0.0F);
    QCOMPARE(box.contentW, 160.0F);
    QCOMPARE(box.contentH, 90.0F);
    QCOMPARE(box.offsetY, 35.0F);
    QCOMPARE(box.offsetY, 160.0F - box.offsetY - box.contentH);
}

void LetterboxTest::invalidSizesAreZeroAndRejectMapping()
{
    const Letterbox box = computeLetterbox(0.0F, 100.0F, 100, 100);
    QCOMPARE(box.offsetX, 0.0F);
    QCOMPARE(box.offsetY, 0.0F);
    QCOMPARE(box.contentW, 0.0F);
    QCOMPARE(box.contentH, 0.0F);
    QCOMPARE(box.scale, 0.0F);

    cv::Point2f out;
    QVERIFY(!itemToFrame(box, 10.0F, 10.0F, 100, 100, out));
    QVERIFY(!frameToItem(box, 10.0F, 10.0F, out));
    QVERIFY(computeLetterbox(100.0F, 100.0F, 0, 100).scale == 0.0F);
}

void LetterboxTest::roundTripInsideFrame()
{
    const Letterbox box = computeLetterbox(200.0F, 100.0F, 100, 100);
    const cv::Point2f samples[] = {{0.0F, 0.0F}, {50.0F, 40.0F}, {100.0F, 100.0F}};
    for (const cv::Point2f& sample : samples)
    {
        cv::Point2f item;
        cv::Point2f back;
        QVERIFY(frameToItem(box, sample.x, sample.y, item));
        QVERIFY(itemToFrame(box, item.x, item.y, 100, 100, back));
        QVERIFY(qAbs(back.x - sample.x) < 1e-3F);
        QVERIFY(qAbs(back.y - sample.y) < 1e-3F);
    }
}

QTEST_APPLESS_MAIN(LetterboxTest)

#include "LetterboxTest.moc"
