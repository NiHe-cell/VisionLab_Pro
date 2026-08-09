#include <QtTest/QtTest>

#include "detectors/SsdDecoding.h"

namespace {

// 构造 [1, 1, N, 7] 的 SSD 输出张量。
cv::Mat makeOutput(int count)
{
    const int sizes[4] = {1, 1, count, 7};
    return cv::Mat(4, sizes, CV_32F, cv::Scalar(0));
}

void setRecord(cv::Mat& out, int i, float confidence,
               float x1, float y1, float x2, float y2)
{
    float* data = out.ptr<float>(0, 0, i);
    data[2] = confidence;
    data[3] = x1;
    data[4] = y1;
    data[5] = x2;
    data[6] = y2;
}

} // namespace

class SsdDecodingTest : public QObject
{
    Q_OBJECT

private slots:
    void decodesAndMapsCoordinates();
    void filtersLowConfidence();
    void clampsToFrame();
    void rejectsMalformedOutput();
    void emptyOutputYieldsNothing();
};

void SsdDecodingTest::decodesAndMapsCoordinates()
{
    cv::Mat out = makeOutput(1);
    setRecord(out, 0, 0.9F, 0.1F, 0.2F, 0.5F, 0.6F);

    const auto detections = visionlab::ssd::decodeDetections(out, {200, 100}, 0.6F);

    QCOMPARE(detections.size(), size_t{1});
    const auto& d = detections.front();
    QCOMPARE(d.label, std::string("Face"));
    QCOMPARE(d.classId, 0);
    QCOMPARE(d.confidence, 0.9F);
    // 归一化 (0.1,0.2)-(0.5,0.6) × 200x100 → (20,20)-(100,60)
    QCOMPARE(d.box, cv::Rect(20, 20, 80, 40));
}

void SsdDecodingTest::filtersLowConfidence()
{
    cv::Mat out = makeOutput(2);
    setRecord(out, 0, 0.3F, 0.1F, 0.1F, 0.2F, 0.2F); // 低于阈值
    setRecord(out, 1, 0.8F, 0.1F, 0.1F, 0.2F, 0.2F);

    const auto detections = visionlab::ssd::decodeDetections(out, {100, 100}, 0.6F);

    QCOMPARE(detections.size(), size_t{1});
    QCOMPARE(detections.front().confidence, 0.8F);
}

void SsdDecodingTest::clampsToFrame()
{
    cv::Mat out = makeOutput(1);
    // 越界框：左上 (-0.1,-0.1)，右下 (1.2,1.2) → 应被裁剪到图像范围。
    setRecord(out, 0, 0.9F, -0.1F, -0.1F, 1.2F, 1.2F);

    const auto detections = visionlab::ssd::decodeDetections(out, {100, 100}, 0.6F);

    QCOMPARE(detections.size(), size_t{1});
    QCOMPARE(detections.front().box, cv::Rect(0, 0, 100, 100));
}

void SsdDecodingTest::rejectsMalformedOutput()
{
    // 形状不是 [1,1,N,7] 时安全返回空。
    const cv::Mat wrong(3, 3, CV_32F, cv::Scalar(0));
    QVERIFY(visionlab::ssd::decodeDetections(wrong, {100, 100}, 0.6F).empty());
}

void SsdDecodingTest::emptyOutputYieldsNothing()
{
    QVERIFY(visionlab::ssd::decodeDetections(cv::Mat(), {100, 100}, 0.6F).empty());
    QVERIFY(visionlab::ssd::decodeDetections(makeOutput(0), {100, 100}, 0.6F).empty());
}

QTEST_APPLESS_MAIN(SsdDecodingTest)

#include "SsdDecodingTest.moc"
