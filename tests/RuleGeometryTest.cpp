#include <QtTest/QtTest>

#include "analytics/RuleGeometry.h"
#include "core/VisionEvent.h"

using visionlab::CrossingDirection;
using visionlab::classifyCrossing;
using visionlab::footPoint;
using visionlab::lineSide;
using visionlab::pointInPolygon;
using visionlab::polygonFromRect;
using visionlab::segmentsIntersect;

class RuleGeometryTest : public QObject
{
    Q_OBJECT

private slots:
    void footPointOfKnownBox();
    void footPointOfEmptyBoxIsOrigin();
    void pointInsideOnAndOutsideSquare();
    void fewerThanThreeVerticesIsOutside();
    void polygonFromRectContainsCenter();
    void segmentsCrossParallelTouchAndZeroLength();
    void crossingForwardReverseParallelAndOnLine();
};

void RuleGeometryTest::footPointOfKnownBox()
{
    const cv::Point2f foot = footPoint(cv::Rect(10, 10, 20, 30));
    QCOMPARE(foot.x, 20.0F);
    QCOMPARE(foot.y, 40.0F);
}

void RuleGeometryTest::footPointOfEmptyBoxIsOrigin()
{
    const cv::Point2f foot = footPoint(cv::Rect{});
    QCOMPARE(foot.x, 0.0F);
    QCOMPARE(foot.y, 0.0F);
}

void RuleGeometryTest::pointInsideOnAndOutsideSquare()
{
    const std::vector<cv::Point2f> square{
        {0.0F, 0.0F}, {10.0F, 0.0F}, {10.0F, 10.0F}, {0.0F, 10.0F}};
    QVERIFY(pointInPolygon({5.0F, 5.0F}, square));
    QVERIFY(pointInPolygon({5.0F, 0.0F}, square));
    QVERIFY(!pointInPolygon({20.0F, 5.0F}, square));
}

void RuleGeometryTest::fewerThanThreeVerticesIsOutside()
{
    QVERIFY(!pointInPolygon({1.0F, 1.0F}, {{0.0F, 0.0F}, {2.0F, 0.0F}}));
    QVERIFY(!pointInPolygon({1.0F, 1.0F}, {}));
}

void RuleGeometryTest::polygonFromRectContainsCenter()
{
    const cv::Rect rect(10, 20, 30, 40);
    const auto polygon = polygonFromRect(rect);
    QCOMPARE(polygon.size(), std::size_t{4});
    QVERIFY(pointInPolygon({25.0F, 40.0F}, polygon));
}

void RuleGeometryTest::segmentsCrossParallelTouchAndZeroLength()
{
    QVERIFY(segmentsIntersect({0.0F, 0.0F}, {10.0F, 10.0F},
                              {0.0F, 10.0F}, {10.0F, 0.0F}));
    QVERIFY(!segmentsIntersect({0.0F, 0.0F}, {10.0F, 0.0F},
                               {0.0F, 10.0F}, {10.0F, 10.0F}));
    QVERIFY(segmentsIntersect({0.0F, 0.0F}, {10.0F, 0.0F},
                              {10.0F, 0.0F}, {10.0F, 10.0F}));
    QVERIFY(!segmentsIntersect({0.0F, 0.0F}, {0.0F, 0.0F},
                               {0.0F, 0.0F}, {10.0F, 0.0F}));
}

void RuleGeometryTest::crossingForwardReverseParallelAndOnLine()
{
    const cv::Point2f a{0.0F, 0.0F};
    const cv::Point2f b{10.0F, 0.0F};
    QCOMPARE(lineSide({5.0F, 5.0F}, a, b), 1);
    QCOMPARE(lineSide({5.0F, -5.0F}, a, b), -1);
    QCOMPARE(classifyCrossing({5.0F, 5.0F}, {5.0F, -5.0F}, a, b),
             CrossingDirection::Forward);
    QCOMPARE(classifyCrossing({5.0F, -5.0F}, {5.0F, 5.0F}, a, b),
             CrossingDirection::Reverse);
    QCOMPARE(classifyCrossing({5.0F, 5.0F}, {15.0F, 5.0F}, a, b),
             CrossingDirection::None);
    QCOMPARE(classifyCrossing({5.0F, 5.0F}, {5.0F, 0.0F}, a, b),
             CrossingDirection::None);
}

QTEST_APPLESS_MAIN(RuleGeometryTest)

#include "RuleGeometryTest.moc"
