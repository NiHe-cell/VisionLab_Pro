#include <QtTest/QtTest>

#include "detectors/YoloDecoding.h"

namespace {

// 构造 YOLO 风格输出张量（默认 80 类），每行：
// [cx, cy, w, h, objectness, 类别分数...]，坐标为归一化值。
cv::Mat makeOutput(int rowCount)
{
    return cv::Mat(rowCount, 85, CV_32F, cv::Scalar(0));
}

void setCandidate(cv::Mat& out, int row, float cx, float cy, float w, float h,
                  float objectness, int bestClass, float classScore)
{
    auto* data = out.ptr<float>(row);
    data[0] = cx;
    data[1] = cy;
    data[2] = w;
    data[3] = h;
    data[4] = objectness;
    data[5 + bestClass] = classScore;
}

} // namespace

class YoloDecodingTest : public QObject
{
    Q_OBJECT

private slots:
    void decodesSingleDetection();
    void filtersLowConfidence();
    void picksBestClass();
    void nmsMergesOverlaps();
    void fallsBackWhenNoClassNames();
    void emptyOutputsYieldNothing();
};

void YoloDecodingTest::decodesSingleDetection()
{
    cv::Mat out = makeOutput(1);
    setCandidate(out, 0, 0.5F, 0.5F, 0.2F, 0.2F, 0.9F, 1, 0.9F);

    const auto detections = visionlab::yolo::decodeDetections(
        {out}, {100, 100}, {"a", "b"}, 0.25F, 0.45F);

    QCOMPARE(detections.size(), size_t{1});
    const auto& d = detections.front();
    QCOMPARE(d.classId, 1);
    QCOMPARE(d.label, std::string("b"));
    QVERIFY(std::abs(d.confidence - 0.81F) < 1e-4F);
    // cx=50, cy=50, w=20, h=20 → 左上角 (40, 40)
    QCOMPARE(d.box, cv::Rect(40, 40, 20, 20));
}

void YoloDecodingTest::filtersLowConfidence()
{
    cv::Mat out = makeOutput(1);
    // objectness 低于阈值，应被丢弃。
    setCandidate(out, 0, 0.5F, 0.5F, 0.2F, 0.2F, 0.10F, 3, 0.9F);

    const auto detections = visionlab::yolo::decodeDetections(
        {out}, {100, 100}, {"a", "b", "c", "d"}, 0.25F, 0.45F);

    QVERIFY(detections.empty());
}

void YoloDecodingTest::picksBestClass()
{
    cv::Mat out = makeOutput(1);
    setCandidate(out, 0, 0.5F, 0.5F, 0.1F, 0.1F, 0.9F, 7, 0.5F);
    // 同帧再放一个更优类别分数，argmax 应选择 class 9。
    out.ptr<float>(0)[5 + 9] = 0.95F;

    const auto detections = visionlab::yolo::decodeDetections(
        {out}, {100, 100}, {}, 0.25F, 0.45F);

    QCOMPARE(detections.size(), size_t{1});
    QCOMPARE(detections.front().classId, 9);
}

void YoloDecodingTest::nmsMergesOverlaps()
{
    // 两个几乎完全重叠的高置信候选应被 NMS 合并为一个。
    cv::Mat out = makeOutput(2);
    setCandidate(out, 0, 0.5F, 0.5F, 0.2F, 0.2F, 0.90F, 1, 0.9F);
    setCandidate(out, 1, 0.5F, 0.5F, 0.2F, 0.2F, 0.85F, 1, 0.9F);

    const auto detections = visionlab::yolo::decodeDetections(
        {out}, {100, 100}, {"a", "b"}, 0.25F, 0.45F);

    QCOMPARE(detections.size(), size_t{1});
}

void YoloDecodingTest::fallsBackWhenNoClassNames()
{
    cv::Mat out = makeOutput(1);
    setCandidate(out, 0, 0.5F, 0.5F, 0.2F, 0.2F, 0.9F, 42, 0.9F);

    // 类别表为空时不得越界（旧实现会崩溃），应回退为 "class_42"。
    const auto detections = visionlab::yolo::decodeDetections(
        {out}, {100, 100}, {}, 0.25F, 0.45F);

    QCOMPARE(detections.size(), size_t{1});
    QCOMPARE(detections.front().label, std::string("class_42"));
}

void YoloDecodingTest::emptyOutputsYieldNothing()
{
    const auto detections = visionlab::yolo::decodeDetections(
        {}, {100, 100}, {"a"}, 0.25F, 0.45F);
    QVERIFY(detections.empty());
}

QTEST_APPLESS_MAIN(YoloDecodingTest)

#include "YoloDecodingTest.moc"
