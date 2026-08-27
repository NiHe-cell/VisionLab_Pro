#include <QtTest/QtTest>

#include "tracking/IouMatching.h"

using visionlab::Association;
using visionlab::greedyIouAssociate;
using visionlab::intersectionOverUnion;

class IouMatchingTest : public QObject
{
    Q_OBJECT

private slots:
    void identicalBoxesHaveUnitIou();
    void disjointBoxesHaveZeroIou();
    void halfShiftHasKnownIou();
    void emptyBoxHasZeroIou();
    void greedyKeepsTwoHighPairsAndDropsLow();
    void differentClassIdsDoNotAssociate();
};

void IouMatchingTest::identicalBoxesHaveUnitIou()
{
    QCOMPARE(intersectionOverUnion({0, 0, 10, 10}, {0, 0, 10, 10}), 1.0F);
}

void IouMatchingTest::disjointBoxesHaveZeroIou()
{
    QCOMPARE(intersectionOverUnion({0, 0, 10, 10}, {20, 20, 10, 10}), 0.0F);
}

void IouMatchingTest::halfShiftHasKnownIou()
{
    // 相交 5x10=50，并集 100+100-50=150，IoU=1/3。
    QCOMPARE(intersectionOverUnion({0, 0, 10, 10}, {5, 0, 10, 10}), 1.0F / 3.0F);
}

void IouMatchingTest::emptyBoxHasZeroIou()
{
    QCOMPARE(intersectionOverUnion({}, {0, 0, 10, 10}), 0.0F);
    QCOMPARE(intersectionOverUnion({0, 0, 10, 10}, {}), 0.0F);
}

void IouMatchingTest::greedyKeepsTwoHighPairsAndDropsLow()
{
    const std::vector<cv::Rect> detections{{0, 0, 10, 10}, {50, 0, 10, 10}, {5, 0, 10, 10}};
    const std::vector<int> detectionClass{0, 0, 0};
    const std::vector<cv::Rect> tracks{{0, 0, 10, 10}, {50, 0, 10, 10}};
    const std::vector<int> trackClass{0, 0};

    const auto matches = greedyIouAssociate(detections, detectionClass, tracks, trackClass, 0.5F);

    QCOMPARE(matches.size(), std::size_t{2});
    QVERIFY(matches[0].detectionIndex != matches[1].detectionIndex);
    QVERIFY(matches[0].trackIndex != matches[1].trackIndex);
    for (const Association& match : matches)
    {
        QVERIFY(match.iou >= 0.5F);
        QVERIFY(match.detectionIndex == 0 || match.detectionIndex == 1);
    }
}

void IouMatchingTest::differentClassIdsDoNotAssociate()
{
    const std::vector<cv::Rect> detections{{0, 0, 10, 10}};
    const std::vector<int> detectionClass{0};
    const std::vector<cv::Rect> tracks{{0, 0, 10, 10}};
    const std::vector<int> trackClass{1};

    QVERIFY(greedyIouAssociate(detections, detectionClass, tracks, trackClass, 0.1F).empty());
}

QTEST_APPLESS_MAIN(IouMatchingTest)

#include "IouMatchingTest.moc"
