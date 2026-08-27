#include <QtTest/QtTest>

#include "core/Track.h"

using visionlab::Track;
using visionlab::TrackState;
using visionlab::TrackUpdateContext;
using visionlab::TrackerConfig;
using visionlab::TrackerStats;

class TrackTest : public QObject
{
    Q_OBJECT

private slots:
    void trackDefaults();
    void trackerConfigDefaults();
    void trackUpdateContextDefaults();
    void trackerStatsDefaults();
};

void TrackTest::trackDefaults()
{
    const Track track;
    QCOMPARE(track.trackId, std::uint64_t{0});
    QCOMPARE(track.classId, -1);
    QVERIFY(track.label.empty());
    QCOMPARE(track.confidence, 0.0F);
    QVERIFY(track.box.empty());
    QCOMPARE(track.state, TrackState::Tentative);
    QCOMPARE(track.age, 0);
    QCOMPARE(track.hits, 0);
    QCOMPARE(track.timeSinceUpdate, 0);
    QVERIFY(track.trajectory.empty());
}

void TrackTest::trackerConfigDefaults()
{
    const TrackerConfig config;
    QCOMPARE(config.highThresh, 0.6F);
    QCOMPARE(config.lowThresh, 0.1F);
    QCOMPARE(config.matchIou, 0.5F);
    QCOMPARE(config.maxLostFrames, 30);
    QCOMPARE(config.minHits, 3);
    QCOMPARE(config.maxTrajectoryPoints, std::size_t{30});
}

void TrackTest::trackUpdateContextDefaults()
{
    const TrackUpdateContext context;
    QCOMPARE(context.frameId, std::int64_t{0});
}

void TrackTest::trackerStatsDefaults()
{
    const TrackerStats stats;
    QCOMPARE(stats.activeTracks, std::size_t{0});
    QCOMPARE(stats.createdTracks, std::uint64_t{0});
    QCOMPARE(stats.lostTracks, std::uint64_t{0});
    QCOMPARE(stats.removedTracks, std::uint64_t{0});
    QCOMPARE(stats.lastUpdateLatencyMs, 0.0);
}

QTEST_APPLESS_MAIN(TrackTest)

#include "TrackTest.moc"
