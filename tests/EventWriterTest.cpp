#include <QtTest/QtTest>

#include <filesystem>

#include <QTemporaryDir>

#include "core/VisionEvent.h"
#include "storage/EventWriter.h"
#include "storage/SqliteEventRepository.h"

using visionlab::EventQuery;
using visionlab::EventType;
using visionlab::EventWriter;
using visionlab::SqliteEventRepository;
using visionlab::StoredEvent;
using visionlab::VisionEvent;

namespace {

VisionEvent makeEvent(std::uint64_t id)
{
    VisionEvent event;
    event.eventId = id;
    event.type = EventType::RoiIntrusion;
    event.ruleId = "roi-1";
    event.sourceId = "fake:0";
    event.trackId = id;
    event.label = "person";
    event.message = "intrusion";
    event.frameId = static_cast<std::int64_t>(id);
    return event;
}

std::filesystem::path dbPath(const QTemporaryDir& dir)
{
    return std::filesystem::path(dir.path().toStdWString()) / L"events.sqlite";
}

} // namespace

class EventWriterTest : public QObject
{
    Q_OBJECT

private slots:
    void enqueueThreeThenQuery();
    void enqueueManyDoesNotCrash();
    void startFailsOnEmptyPath();
};

void EventWriterTest::enqueueThreeThenQuery()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    EventWriter writer(std::make_unique<SqliteEventRepository>());
    QVERIFY(writer.start(dbPath(dir)));

    writer.enqueue(makeEvent(1));
    writer.enqueue(makeEvent(2));
    writer.enqueue(makeEvent(3));

    std::vector<StoredEvent> rows;
    writer.requestQuery(EventQuery{}, this, [&](std::vector<StoredEvent> result) {
        rows = std::move(result);
    });
    QTRY_COMPARE(rows.size(), std::size_t{3});
    QVERIFY(rows.front().wallUtcMs > 0);
    QCOMPARE(rows[0].event.eventId, quint64{1});
    QCOMPARE(rows[2].event.eventId, quint64{3});
    writer.stop();
}

void EventWriterTest::enqueueManyDoesNotCrash()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    EventWriter writer(std::make_unique<SqliteEventRepository>());
    QVERIFY(writer.start(dbPath(dir)));
    for (std::uint64_t i = 1; i <= 2000; ++i)
        writer.enqueue(makeEvent(i));

    std::vector<StoredEvent> rows;
    writer.requestQuery(EventQuery{}, this, [&](std::vector<StoredEvent> result) {
        rows = std::move(result);
    });
    QTRY_VERIFY(rows.size() > 0);
    QVERIFY(rows.size() <= 2000);
    writer.stop();
}

void EventWriterTest::startFailsOnEmptyPath()
{
    EventWriter writer(std::make_unique<SqliteEventRepository>());
    QVERIFY(!writer.start({}));
    writer.enqueue(makeEvent(1));
    QVERIFY(!writer.isRunning());
}

QTEST_MAIN(EventWriterTest)

#include "EventWriterTest.moc"
