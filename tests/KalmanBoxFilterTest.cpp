#include <QtTest/QtTest>

#include <cmath>

#include "tracking/KalmanBoxFilter.h"

using visionlab::KalmanBoxFilter;

class KalmanBoxFilterTest : public QObject
{
    Q_OBJECT

private slots:
    void stationaryStaysNearInitialCenter();
    void rightwardMotionPredictsNonDecreasingCx();
};

namespace {

cv::Point2f center(const cv::Rect& box)
{
    return {
        static_cast<float>(box.x) + static_cast<float>(box.width) * 0.5F,
        static_cast<float>(box.y) + static_cast<float>(box.height) * 0.5F,
    };
}

} // namespace

void KalmanBoxFilterTest::stationaryStaysNearInitialCenter()
{
    const cv::Rect box{10, 10, 20, 20};
    KalmanBoxFilter filter(box);
    const cv::Point2f origin = center(box);

    for (int i = 0; i < 8; ++i)
    {
        filter.predict(1.0 / 30.0);
        filter.update(box);
    }

    const cv::Point2f now = center(filter.box());
    QVERIFY(std::abs(now.x - origin.x) < 5.0F);
    QVERIFY(std::abs(now.y - origin.y) < 5.0F);
}

void KalmanBoxFilterTest::rightwardMotionPredictsNonDecreasingCx()
{
    KalmanBoxFilter filter({10, 10, 20, 20});
    float lastCx = center(filter.box()).x;

    for (int x : {20, 30, 40, 50})
    {
        filter.predict(1.0 / 30.0);
        filter.update({x, 10, 20, 20});
        const float cx = center(filter.box()).x;
        QVERIFY(cx + 0.5F >= lastCx);
        lastCx = cx;
    }

    filter.predict(1.0 / 30.0);
    QVERIFY(center(filter.box()).x + 0.5F >= lastCx);
}

QTEST_APPLESS_MAIN(KalmanBoxFilterTest)

#include "KalmanBoxFilterTest.moc"
