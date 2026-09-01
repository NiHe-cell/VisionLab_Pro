#include <QtTest/QtTest>

#include "core/VisionEvent.h"
#include "models/EventModel.h"
#include "models/EventTypeFilterModel.h"
#include "storage/EventQuery.h"

using visionlab::CrossingDirection;
using visionlab::EventType;
using visionlab::StoredEvent;
using visionlab::VisionEvent;

namespace {

VisionEvent makeEvent(std::uint64_t id, EventType type)
{
    VisionEvent event;
    event.eventId = id;
    event.type = type;
    event.ruleId = "rule-a";
    event.trackId = 9;
    event.label = "person";
    event.confidence = 0.7F;
    event.message = "hit";
    event.direction = CrossingDirection::Forward;
    event.countIn = 1;
    event.countOut = 2;
    event.occupancy = 3;
    event.frameId = 40;
    return event;
}

} // namespace

class EventModelTest : public QObject
{
    Q_OBJECT

private slots:
    void ingestAppendsOnlyNewIds();
    void beginSessionAllowsSameIdAgain();
    void emptySnapshotDoesNotChangeCount();
    void filterShowsOnlyLoitering();
    void historyPrefillsRowIdAndWallClock();
};

void EventModelTest::ingestAppendsOnlyNewIds()
{
    EventModel model;
    model.ingest({makeEvent(1, EventType::RoiIntrusion),
                  makeEvent(2, EventType::LineCrossing)});
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 0), EventModel::SessionEventIdRole).toULongLong(),
             quint64{1});
    QCOMPARE(model.data(model.index(0, 0), EventModel::TypeRole).toInt(),
             static_cast<int>(EventType::RoiIntrusion));
    QCOMPARE(model.data(model.index(0, 0), EventModel::RuleIdRole).toString(),
             QStringLiteral("rule-a"));
    QCOMPARE(model.data(model.index(0, 0), EventModel::TrackIdRole).toULongLong(),
             quint64{9});
    QCOMPARE(model.data(model.index(0, 0), EventModel::LabelRole).toString(),
             QStringLiteral("person"));
    QCOMPARE(model.data(model.index(0, 0), EventModel::ConfidenceRole).toFloat(), 0.7F);
    QCOMPARE(model.data(model.index(0, 0), EventModel::MessageRole).toString(),
             QStringLiteral("hit"));
    QCOMPARE(model.data(model.index(0, 0), EventModel::DirectionRole).toInt(),
             static_cast<int>(CrossingDirection::Forward));
    QCOMPARE(model.data(model.index(0, 0), EventModel::CountInRole).toULongLong(), quint64{1});
    QCOMPARE(model.data(model.index(0, 0), EventModel::CountOutRole).toULongLong(), quint64{2});
    QCOMPARE(model.data(model.index(0, 0), EventModel::OccupancyRole).toULongLong(), quint64{3});
    QCOMPARE(model.data(model.index(0, 0), EventModel::FrameIdRole).toLongLong(), qint64{40});
    QVERIFY(model.data(model.index(0, 0), EventModel::WallUtcMsRole).toLongLong() > 0);

    model.ingest({makeEvent(1, EventType::RoiIntrusion),
                  makeEvent(2, EventType::LineCrossing),
                  makeEvent(3, EventType::Loitering)});
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.data(model.index(2, 0), EventModel::SessionEventIdRole).toULongLong(),
             quint64{3});

    const auto names = model.roleNames();
    QCOMPARE(names.value(EventModel::SessionEventIdRole), QByteArray("sessionEventId"));
    QCOMPARE(names.value(EventModel::RowIdRole), QByteArray("rowId"));
    QCOMPARE(names.value(EventModel::WallUtcMsRole), QByteArray("wallUtcMs"));
    QCOMPARE(names.value(EventModel::TypeRole), QByteArray("type"));
    QCOMPARE(names.value(EventModel::RuleIdRole), QByteArray("ruleId"));
    QCOMPARE(names.value(EventModel::TrackIdRole), QByteArray("trackId"));
    QCOMPARE(names.value(EventModel::LabelRole), QByteArray("label"));
    QCOMPARE(names.value(EventModel::ConfidenceRole), QByteArray("confidence"));
    QCOMPARE(names.value(EventModel::MessageRole), QByteArray("message"));
    QCOMPARE(names.value(EventModel::DirectionRole), QByteArray("direction"));
    QCOMPARE(names.value(EventModel::CountInRole), QByteArray("countIn"));
    QCOMPARE(names.value(EventModel::CountOutRole), QByteArray("countOut"));
    QCOMPARE(names.value(EventModel::OccupancyRole), QByteArray("occupancy"));
    QCOMPARE(names.value(EventModel::FrameIdRole), QByteArray("frameId"));
}

void EventModelTest::beginSessionAllowsSameIdAgain()
{
    EventModel model;
    model.ingest({makeEvent(1, EventType::RoiIntrusion)});
    model.beginSession();
    model.ingest({makeEvent(1, EventType::Counting)});
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(1, 0), EventModel::TypeRole).toInt(),
             static_cast<int>(EventType::Counting));
}

void EventModelTest::emptySnapshotDoesNotChangeCount()
{
    EventModel model;
    model.ingest({makeEvent(1, EventType::RoiIntrusion)});
    model.ingest({});
    QCOMPARE(model.rowCount(), 1);
}

void EventModelTest::filterShowsOnlyLoitering()
{
    EventModel model;
    model.ingest({makeEvent(1, EventType::RoiIntrusion),
                  makeEvent(2, EventType::LineCrossing),
                  makeEvent(3, EventType::Loitering)});

    EventTypeFilterModel filter;
    filter.setSourceModel(&model);
    QCOMPARE(filter.rowCount(), 3);

    filter.setTypeFilter(static_cast<int>(EventType::Loitering));
    QCOMPARE(filter.rowCount(), 1);
    QCOMPARE(filter.data(filter.index(0, 0), EventModel::SessionEventIdRole).toULongLong(),
             quint64{3});

    filter.setTypeFilter(-1);
    QCOMPARE(filter.rowCount(), 3);
}

void EventModelTest::historyPrefillsRowIdAndWallClock()
{
    EventModel model;
    StoredEvent stored;
    stored.rowId = 77;
    stored.wallUtcMs = 1'700'000'000'000;
    stored.event = makeEvent(11, EventType::Counting);
    model.ingest({}, {stored});
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), EventModel::RowIdRole).toLongLong(), qint64{77});
    QCOMPARE(model.data(model.index(0, 0), EventModel::WallUtcMsRole).toLongLong(),
             qint64{1'700'000'000'000});
    QCOMPARE(model.data(model.index(0, 0), EventModel::SessionEventIdRole).toULongLong(),
             quint64{11});

    StoredEvent again;
    again.rowId = 78;
    again.event = makeEvent(12, EventType::Counting);
    model.ingest({makeEvent(1, EventType::RoiIntrusion)}, {again});
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(1, 0), EventModel::SessionEventIdRole).toULongLong(),
             quint64{1});
    QCOMPARE(model.data(model.index(1, 0), EventModel::RowIdRole).toLongLong(), qint64{0});
}

QTEST_GUILESS_MAIN(EventModelTest)

#include "EventModelTest.moc"
