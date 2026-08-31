#include <QtTest/QtTest>

#include <memory>

#include "core/Track.h"
#include "fakes/FakeRule.h"
#include "analytics/IRule.h"

using visionlab::EventType;
using visionlab::IRule;
using visionlab::RuleContext;
using visionlab::Track;

namespace {

Track makeTrack(std::uint64_t trackId,
                int classId,
                const std::string& label,
                float confidence,
                const cv::Rect& box)
{
    Track track;
    track.trackId = trackId;
    track.classId = classId;
    track.label = label;
    track.confidence = confidence;
    track.box = box;
    return track;
}

} // namespace

class IRuleTest : public QObject
{
    Q_OBJECT

private slots:
    void twoTracksMapToEvents();
    void emptyTracksReturnEmpty();
    void resetIncrementsCount();
};

void IRuleTest::twoTracksMapToEvents()
{
    std::unique_ptr<IRule> rule = std::make_unique<FakeRule>();
    QCOMPARE(rule->id(), std::string("fake"));
    QCOMPARE(rule->name(), std::string("fake"));
    QCOMPARE(rule->eventType(), EventType::RoiIntrusion);

    RuleContext context;
    context.frameId = 7;
    context.sourceId = "cam:0";

    const std::vector<Track> tracks{
        makeTrack(1, 0, "person", 0.9F, cv::Rect(1, 2, 3, 4)),
        makeTrack(2, 2, "car", 0.8F, cv::Rect(10, 20, 30, 40)),
    };

    const auto events = rule->evaluate(tracks, context);
    QCOMPARE(events.size(), std::size_t{2});
    QCOMPARE(events[0].ruleId, std::string("fake"));
    QCOMPARE(events[0].sourceId, std::string("cam:0"));
    QCOMPARE(events[0].trackId, std::uint64_t{1});
    QCOMPARE(events[0].classId, 0);
    QCOMPARE(events[0].label, std::string("person"));
    QCOMPARE(events[0].confidence, 0.9F);
    QCOMPARE(events[0].box, cv::Rect(1, 2, 3, 4));
    QCOMPARE(events[0].frameId, std::int64_t{7});
    QCOMPARE(events[0].message, std::string("fake"));
    QCOMPARE(events[1].trackId, std::uint64_t{2});
    QCOMPARE(events[1].label, std::string("car"));
    QCOMPARE(events[1].box, cv::Rect(10, 20, 30, 40));
}

void IRuleTest::emptyTracksReturnEmpty()
{
    FakeRule rule;
    QVERIFY(rule.evaluate({}, RuleContext{}).empty());
}

void IRuleTest::resetIncrementsCount()
{
    FakeRule rule;
    QCOMPARE(rule.resetCount(), 0);
    rule.reset();
    QCOMPARE(rule.resetCount(), 1);
}

QTEST_APPLESS_MAIN(IRuleTest)

#include "IRuleTest.moc"
