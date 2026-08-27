#include <QtTest/QtTest>

#include <opencv2/imgproc.hpp>

#include "core/Track.h"
#include "core/VisionTypes.h"
#include "rendering/TrackRenderer.h"

using visionlab::Track;
using visionlab::TrackPoint;
using visionlab::TrackRenderer;
using visionlab::TrackState;

namespace {

Track makeTrack(std::uint64_t id, int classId, const std::string& label, const cv::Rect& box)
{
    Track track;
    track.trackId = id;
    track.classId = classId;
    track.label = label;
    track.confidence = 0.9F;
    track.box = box;
    track.state = TrackState::Confirmed;
    return track;
}

} // namespace

class TrackRendererTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyListIsNoOp();
    void emptyFrameIsNoOp();
    void drawsGreenBoxForPerson();
    void motionUsesYellow();
    void trajectoryAddsPixels();
    void outOfBoundsDoesNotChangeSize();
};

void TrackRendererTest::emptyListIsNoOp()
{
    cv::Mat frame(20, 20, CV_8UC3, cv::Scalar(10, 10, 10));
    const cv::Mat before = frame.clone();

    TrackRenderer().render(frame, {});

    QCOMPARE(cv::countNonZero(frame.reshape(1) != before.reshape(1)), 0);
}

void TrackRendererTest::emptyFrameIsNoOp()
{
    cv::Mat frame;
    TrackRenderer().render(frame, {makeTrack(1, 0, "person", {0, 0, 5, 5})});
    QVERIFY(frame.empty());
}

void TrackRendererTest::drawsGreenBoxForPerson()
{
    cv::Mat frame(50, 50, CV_8UC3, cv::Scalar(0, 0, 0));
    const int zerosBefore = cv::countNonZero(frame.reshape(1));

    TrackRenderer().render(frame, {makeTrack(17, 0, "person", {10, 10, 20, 20})});

    QCOMPARE(frame.at<cv::Vec3b>(10, 10), cv::Vec3b(0, 255, 0));
    QVERIFY(cv::countNonZero(frame.reshape(1)) > zerosBefore);
}

void TrackRendererTest::motionUsesYellow()
{
    cv::Mat frame(50, 50, CV_8UC3, cv::Scalar(0, 0, 0));
    Track motion = makeTrack(3, visionlab::kMotionClassId, "In Motion", {10, 10, 20, 20});

    TrackRenderer().render(frame, {motion});

    QCOMPARE(frame.at<cv::Vec3b>(10, 10), cv::Vec3b(0, 255, 255));
}

void TrackRendererTest::trajectoryAddsPixels()
{
    cv::Mat boxed(60, 80, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::Mat withPath = boxed.clone();
    Track track = makeTrack(1, 0, "person", {40, 20, 10, 10});
    TrackRenderer().render(boxed, {track});

    TrackPoint a;
    a.centroid = {5.0F, 30.0F};
    a.box = {0, 25, 10, 10};
    TrackPoint b;
    b.centroid = {35.0F, 30.0F};
    b.box = {30, 25, 10, 10};
    track.trajectory = {a, b};
    TrackRenderer().render(withPath, {track});

    QCOMPARE(withPath.cols, 80);
    QCOMPARE(withPath.rows, 60);
    QVERIFY(cv::countNonZero(withPath.reshape(1)) >= cv::countNonZero(boxed.reshape(1)));
}

void TrackRendererTest::outOfBoundsDoesNotChangeSize()
{
    cv::Mat frame(50, 50, CV_8UC3, cv::Scalar(0, 0, 0));
    Track track = makeTrack(1, 0, "edge", {-5, 0, 20, 20});
    TrackPoint outside;
    outside.centroid = {-10.0F, -10.0F};
    TrackPoint inside;
    inside.centroid = {20.0F, 20.0F};
    track.trajectory = {outside, inside};

    TrackRenderer().render(frame, {track});

    QCOMPARE(frame.cols, 50);
    QCOMPARE(frame.rows, 50);
}

QTEST_APPLESS_MAIN(TrackRendererTest)

#include "TrackRendererTest.moc"
