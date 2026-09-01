#include <QtTest/QtTest>

#include <filesystem>

#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include "core/VisionEvent.h"
#include "storage/SqliteEventRepository.h"

using visionlab::CrossingDirection;
using visionlab::EventQuery;
using visionlab::EventType;
using visionlab::SqliteEventRepository;
using visionlab::StoredEvent;
using visionlab::VisionEvent;

namespace {

VisionEvent makeEvent(std::uint64_t eventId, EventType type, const std::string& ruleId)
{
    VisionEvent event;
    event.eventId = eventId;
    event.type = type;
    event.ruleId = ruleId;
    event.sourceId = "cam:0";
    event.trackId = eventId;
    event.classId = 0;
    event.label = "person";
    event.confidence = 0.8F;
    event.box = cv::Rect(1, 2, 3, 4);
    event.frameId = static_cast<std::int64_t>(eventId);
    event.message = "test";
    event.direction = CrossingDirection::None;
    return event;
}

std::filesystem::path dbPath(const QTemporaryDir& dir)
{
    return std::filesystem::path(dir.path().toStdWString()) / L"events.sqlite";
}

} // namespace

class EventRepositoryTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void insertThreeAndQueryAscending();
    void filterByType();
    void paginateAfterRowId();
    void pruneDropsOldest();
    void emptyPathAndFileParentFailOpen();

private:
    void requireSqlite();
};

void EventRepositoryTest::requireSqlite()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")))
        QSKIP("QSQLITE driver is not available");
}

void EventRepositoryTest::init()
{
    requireSqlite();
}

void EventRepositoryTest::insertThreeAndQueryAscending()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SqliteEventRepository repo;
    QVERIFY(repo.open(dbPath(dir)));
    QVERIFY(repo.insert(makeEvent(1, EventType::RoiIntrusion, "roi"), 1000));
    QVERIFY(repo.insert(makeEvent(2, EventType::LineCrossing, "line"), 2000));
    QVERIFY(repo.insert(makeEvent(3, EventType::Loitering, "loiter"), 3000));
    QCOMPARE(repo.count(), std::size_t{3});

    const std::vector<StoredEvent> rows = repo.query(EventQuery{});
    QCOMPARE(rows.size(), std::size_t{3});
    QCOMPARE(rows[0].rowId, std::int64_t{1});
    QCOMPARE(rows[0].wallUtcMs, std::int64_t{1000});
    QCOMPARE(rows[0].event.eventId, std::uint64_t{1});
    QCOMPARE(rows[1].event.eventId, std::uint64_t{2});
    QCOMPARE(rows[2].event.eventId, std::uint64_t{3});
    QCOMPARE(rows[2].event.ruleId, std::string("loiter"));
    repo.close();
}

void EventRepositoryTest::filterByType()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SqliteEventRepository repo;
    QVERIFY(repo.open(dbPath(dir)));
    QVERIFY(repo.insert(makeEvent(1, EventType::RoiIntrusion, "roi"), 1));
    QVERIFY(repo.insert(makeEvent(2, EventType::LineCrossing, "line"), 2));
    QVERIFY(repo.insert(makeEvent(3, EventType::Loitering, "loiter"), 3));

    EventQuery query;
    query.type = EventType::LineCrossing;
    const auto rows = repo.query(query);
    QCOMPARE(rows.size(), std::size_t{1});
    QCOMPARE(rows[0].event.type, EventType::LineCrossing);
    QCOMPARE(rows[0].event.ruleId, std::string("line"));
}

void EventRepositoryTest::paginateAfterRowId()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SqliteEventRepository repo;
    QVERIFY(repo.open(dbPath(dir)));
    for (std::uint64_t i = 1; i <= 5; ++i)
        QVERIFY(repo.insert(makeEvent(i, EventType::RoiIntrusion, "roi"), static_cast<std::int64_t>(i)));

    EventQuery query;
    query.afterRowId = 2;
    query.limit = 2;
    const auto rows = repo.query(query);
    QCOMPARE(rows.size(), std::size_t{2});
    QCOMPARE(rows[0].rowId, std::int64_t{3});
    QCOMPARE(rows[1].rowId, std::int64_t{4});
}

void EventRepositoryTest::pruneDropsOldest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SqliteEventRepository repo;
    QVERIFY(repo.open(dbPath(dir)));
    for (std::uint64_t i = 1; i <= 10005; ++i)
        QVERIFY(repo.insert(makeEvent(i, EventType::RoiIntrusion, "roi"), 1));

    QCOMPARE(repo.count(), std::size_t{10005});
    repo.prune(10000);
    QCOMPARE(repo.count(), std::size_t{10000});

    EventQuery all;
    all.limit = 10000;
    const auto rows = repo.query(all);
    QCOMPARE(rows.front().event.eventId, std::uint64_t{6});
    QCOMPARE(rows.back().event.eventId, std::uint64_t{10005});
}

void EventRepositoryTest::emptyPathAndFileParentFailOpen()
{
    SqliteEventRepository repo;
    QVERIFY(!repo.open({}));
    QCOMPARE(repo.count(), std::size_t{0});
    QVERIFY(repo.query(EventQuery{}).empty());

    QTemporaryFile file;
    QVERIFY(file.open());
    const QString nested = file.fileName() + QStringLiteral("/events.sqlite");
    QVERIFY(!repo.open(nested.toStdWString()));
}

QTEST_MAIN(EventRepositoryTest)

#include "EventRepositoryTest.moc"
