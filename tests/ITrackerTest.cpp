#include <QtTest/QtTest>

#include <memory>

#include "core/Detection.h"
#include "fakes/FakeTracker.h"
#include "tracking/ITracker.h"

using visionlab::Detection;
using visionlab::ITracker;
using visionlab::TrackState;
using visionlab::TrackUpdateContext;

namespace {

Detection makeDetection(int classId, const std::string& label, float confidence, const cv::Rect& box)
{
    Detection detection;
    detection.classId = classId;
    detection.label = label;
    detection.confidence = confidence;
    detection.box = box;
    return detection;
}

} // namespace

class ITrackerTest : public QObject
{
    Q_OBJECT

private slots:
    void twoDetectionsMapToConfirmedTracks();
    void emptyDetectionsReturnEmpty();
    void resetRestartsIdsFromOne();
};

void ITrackerTest::twoDetectionsMapToConfirmedTracks()
{
    std::unique_ptr<ITracker> tracker = std::make_unique<FakeTracker>();
    QCOMPARE(tracker->name(), std::string("fake"));

    const std::vector<Detection> detections{
        makeDetection(0, "person", 0.9F, cv::Rect(1, 2, 3, 4)),
        makeDetection(2, "car", 0.8F, cv::Rect(10, 20, 30, 40)),
    };

    const auto tracks = tracker->update(detections, TrackUpdateContext{});
    QCOMPARE(tracks.size(), std::size_t{2});
    QCOMPARE(tracks[0].trackId, std::uint64_t{1});
    QCOMPARE(tracks[0].classId, 0);
    QCOMPARE(tracks[0].label, std::string("person"));
    QCOMPARE(tracks[0].confidence, 0.9F);
    QCOMPARE(tracks[0].box, cv::Rect(1, 2, 3, 4));
    QCOMPARE(tracks[0].state, TrackState::Confirmed);
    QCOMPARE(tracks[1].trackId, std::uint64_t{2});
    QCOMPARE(tracks[1].label, std::string("car"));
    QCOMPARE(tracker->stats().createdTracks, std::uint64_t{2});
}

void ITrackerTest::emptyDetectionsReturnEmpty()
{
    FakeTracker tracker;
    QVERIFY(tracker.update({}, TrackUpdateContext{}).empty());
    QCOMPARE(tracker.stats().activeTracks, std::size_t{0});
}

void ITrackerTest::resetRestartsIdsFromOne()
{
    FakeTracker tracker;
    const auto first = makeDetection(0, "a", 1.0F, cv::Rect(0, 0, 2, 2));
    tracker.update({first, first}, TrackUpdateContext{});
    tracker.reset();
    QCOMPARE(tracker.stats().createdTracks, std::uint64_t{0});

    const auto tracks = tracker.update({first}, TrackUpdateContext{});
    QCOMPARE(tracks.size(), std::size_t{1});
    QCOMPARE(tracks.front().trackId, std::uint64_t{1});
}

QTEST_APPLESS_MAIN(ITrackerTest)

#include "ITrackerTest.moc"
