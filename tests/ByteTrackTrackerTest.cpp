#include <QtTest/QtTest>

#include <chrono>

#include "core/Detection.h"
#include "tracking/ByteTrackTracker.h"

using visionlab::ByteTrackTracker;
using visionlab::Detection;
using visionlab::Track;
using visionlab::TrackUpdateContext;
using visionlab::TrackerConfig;

namespace {

Detection makeDet(int classId, const std::string& label, float confidence, const cv::Rect& box)
{
    Detection detection;
    detection.classId = classId;
    detection.label = label;
    detection.confidence = confidence;
    detection.box = box;
    return detection;
}

TrackUpdateContext makeContext(std::int64_t frameId)
{
    TrackUpdateContext context;
    context.frameId = frameId;
    context.timestamp = std::chrono::steady_clock::time_point{}
                        + std::chrono::milliseconds(33 * frameId);
    return context;
}

TrackerConfig shortConfig()
{
    TrackerConfig config;
    config.minHits = 1;
    config.maxLostFrames = 3;
    config.maxTrajectoryPoints = 5;
    return config;
}

std::uint64_t singleId(ByteTrackTracker& tracker, std::int64_t frameId, const Detection& detection)
{
    const auto tracks = tracker.update({detection}, makeContext(frameId));
    return tracks.empty() ? 0 : tracks.front().trackId;
}

} // namespace

class ByteTrackTrackerTest : public QObject
{
    Q_OBJECT

private slots:
    void nameIsByteTrack();
    void sameBoxKeepsId();
    void briefMissRecoversSameId();
    void expiredTrackGetsNewId();
    void twoSeparatedBoxesHaveTwoIds();
    void differentClassIdsDoNotMerge();
    void lowScoreRecoversLostTrack();
    void emptyDetectionsEventuallyClear();
    void resetRestartsIdFromOne();
    void trajectoryIsBounded();
};

void ByteTrackTrackerTest::nameIsByteTrack()
{
    QCOMPARE(ByteTrackTracker{}.name(), std::string("ByteTrack"));
}

void ByteTrackTrackerTest::sameBoxKeepsId()
{
    ByteTrackTracker tracker(shortConfig());
    const Detection det = makeDet(0, "person", 0.9F, {10, 10, 20, 20});
    const std::uint64_t id = singleId(tracker, 1, det);
    QVERIFY(id != 0);
    for (std::int64_t frame = 2; frame <= 10; ++frame)
        QCOMPARE(singleId(tracker, frame, det), id);
}

void ByteTrackTrackerTest::briefMissRecoversSameId()
{
    TrackerConfig config = shortConfig();
    config.maxLostFrames = 5;
    ByteTrackTracker tracker(config);
    const Detection det = makeDet(0, "person", 0.9F, {10, 10, 20, 20});
    const std::uint64_t id = singleId(tracker, 1, det);
    QVERIFY(tracker.update({}, makeContext(2)).size() == 1);
    QVERIFY(tracker.update({}, makeContext(3)).size() == 1);
    const Detection nearby = makeDet(0, "person", 0.9F, {12, 10, 20, 20});
    QCOMPARE(singleId(tracker, 4, nearby), id);
}

void ByteTrackTrackerTest::expiredTrackGetsNewId()
{
    ByteTrackTracker tracker(shortConfig());
    const Detection det = makeDet(0, "person", 0.9F, {10, 10, 20, 20});
    const std::uint64_t first = singleId(tracker, 1, det);
    for (std::int64_t frame = 2; frame <= 5; ++frame)
        tracker.update({}, makeContext(frame));
    QVERIFY(tracker.stats().removedTracks >= 1);
    const std::uint64_t next = singleId(tracker, 6, det);
    QVERIFY(next != 0);
    QVERIFY(next != first);
}

void ByteTrackTrackerTest::twoSeparatedBoxesHaveTwoIds()
{
    ByteTrackTracker tracker(shortConfig());
    const Detection left = makeDet(0, "person", 0.9F, {0, 0, 10, 10});
    const Detection right = makeDet(0, "person", 0.9F, {80, 0, 10, 10});
    const auto first = tracker.update({left, right}, makeContext(1));
    QCOMPARE(first.size(), std::size_t{2});
    QVERIFY(first[0].trackId != first[1].trackId);

    const Detection leftMoved = makeDet(0, "person", 0.9F, {2, 0, 10, 10});
    const Detection rightMoved = makeDet(0, "person", 0.9F, {82, 0, 10, 10});
    const auto second = tracker.update({leftMoved, rightMoved}, makeContext(2));
    QCOMPARE(second.size(), std::size_t{2});
    QCOMPARE(second[0].trackId, first[0].trackId);
    QCOMPARE(second[1].trackId, first[1].trackId);
}

void ByteTrackTrackerTest::differentClassIdsDoNotMerge()
{
    ByteTrackTracker tracker(shortConfig());
    const Detection person = makeDet(0, "person", 0.9F, {10, 10, 20, 20});
    const Detection car = makeDet(2, "car", 0.9F, {10, 10, 20, 20});
    const auto tracks = tracker.update({person, car}, makeContext(1));
    QCOMPARE(tracks.size(), std::size_t{2});
    QVERIFY(tracks[0].trackId != tracks[1].trackId);
    QVERIFY(tracks[0].classId != tracks[1].classId);
}

void ByteTrackTrackerTest::lowScoreRecoversLostTrack()
{
    ByteTrackTracker tracker(shortConfig());
    const Detection high = makeDet(0, "person", 0.9F, {10, 10, 20, 20});
    const std::uint64_t id = singleId(tracker, 1, high);
    tracker.update({}, makeContext(2));
    const Detection low = makeDet(0, "person", 0.3F, {11, 10, 20, 20});
    QCOMPARE(singleId(tracker, 3, low), id);
}

void ByteTrackTrackerTest::emptyDetectionsEventuallyClear()
{
    ByteTrackTracker tracker(shortConfig());
    singleId(tracker, 1, makeDet(0, "person", 0.9F, {10, 10, 20, 20}));
    std::vector<Track> last;
    for (std::int64_t frame = 2; frame <= 8; ++frame)
        last = tracker.update({}, makeContext(frame));
    QVERIFY(last.empty());
}

void ByteTrackTrackerTest::resetRestartsIdFromOne()
{
    ByteTrackTracker tracker(shortConfig());
    singleId(tracker, 1, makeDet(0, "person", 0.9F, {10, 10, 20, 20}));
    tracker.reset();
    QCOMPARE(tracker.stats().createdTracks, std::uint64_t{0});
    QCOMPARE(singleId(tracker, 1, makeDet(0, "person", 0.9F, {40, 40, 20, 20})),
             std::uint64_t{1});
}

void ByteTrackTrackerTest::trajectoryIsBounded()
{
    TrackerConfig config = shortConfig();
    config.maxTrajectoryPoints = 5;
    ByteTrackTracker tracker(config);
    const Detection det = makeDet(0, "person", 0.9F, {10, 10, 20, 20});
    std::vector<Track> tracks;
    for (std::int64_t frame = 1; frame <= 40; ++frame)
        tracks = tracker.update({det}, makeContext(frame));
    QCOMPARE(tracks.size(), std::size_t{1});
    QCOMPARE(tracks.front().trajectory.size(), std::size_t{5});
}

QTEST_APPLESS_MAIN(ByteTrackTrackerTest)

#include "ByteTrackTrackerTest.moc"
