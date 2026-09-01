#include "storage/SqliteEventRepository.h"

#include <atomic>

#include <QSqlQuery>
#include <QVariant>

namespace visionlab {
namespace {

std::atomic<int> g_connectionSeq{0};

QString connectionName()
{
    return QStringLiteral("visionlab_events_%1").arg(++g_connectionSeq);
}

VisionEvent eventFromQuery(const QSqlQuery& query)
{
    VisionEvent event;
    event.eventId = query.value(2).toULongLong();
    event.type = static_cast<EventType>(query.value(3).toInt());
    event.ruleId = query.value(4).toString().toStdString();
    event.sourceId = query.value(5).toString().toStdString();
    event.trackId = query.value(6).toULongLong();
    event.classId = query.value(7).toInt();
    event.label = query.value(8).toString().toStdString();
    event.confidence = query.value(9).toFloat();
    event.box = cv::Rect(query.value(10).toInt(),
                         query.value(11).toInt(),
                         query.value(12).toInt(),
                         query.value(13).toInt());
    event.frameId = query.value(14).toLongLong();
    event.message = query.value(15).toString().toStdString();
    event.snapshotRef = query.value(16).toString().toStdString();
    event.direction = static_cast<CrossingDirection>(query.value(17).toInt());
    event.countIn = query.value(18).toULongLong();
    event.countOut = query.value(19).toULongLong();
    event.occupancy = static_cast<std::size_t>(query.value(20).toULongLong());
    return event;
}

} // namespace

SqliteEventRepository::SqliteEventRepository()
    : m_connectionName(connectionName())
{
}

SqliteEventRepository::~SqliteEventRepository()
{
    close();
}

bool SqliteEventRepository::open(const std::filesystem::path& dbPath)
{
    close();
    if (dbPath.empty())
        return false;

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(QString::fromStdWString(dbPath.wstring()));
    if (!m_db.open())
    {
        close();
        return false;
    }
    if (!execSchema())
    {
        close();
        return false;
    }
    m_open = true;
    return true;
}

bool SqliteEventRepository::execSchema()
{
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS events ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  wall_utc_ms INTEGER NOT NULL,"
            "  pipeline_event_id INTEGER NOT NULL,"
            "  type INTEGER NOT NULL,"
            "  rule_id TEXT,"
            "  source_id TEXT,"
            "  track_id INTEGER,"
            "  class_id INTEGER,"
            "  label TEXT,"
            "  confidence REAL,"
            "  box_x INTEGER, box_y INTEGER, box_w INTEGER, box_h INTEGER,"
            "  frame_id INTEGER,"
            "  message TEXT,"
            "  snapshot_ref TEXT,"
            "  direction INTEGER,"
            "  count_in INTEGER,"
            "  count_out INTEGER,"
            "  occupancy INTEGER"
            ")")))
    {
        return false;
    }
    if (!query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_events_type ON events(type)")))
        return false;
    query.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
    query.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));
    return true;
}

bool SqliteEventRepository::insert(const VisionEvent& event, std::int64_t wallUtcMs)
{
    if (!m_open)
        return false;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO events ("
        "wall_utc_ms, pipeline_event_id, type, rule_id, source_id, track_id, class_id, "
        "label, confidence, box_x, box_y, box_w, box_h, frame_id, message, snapshot_ref, "
        "direction, count_in, count_out, occupancy"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(static_cast<qlonglong>(wallUtcMs));
    query.addBindValue(static_cast<qlonglong>(event.eventId));
    query.addBindValue(static_cast<int>(event.type));
    query.addBindValue(QString::fromStdString(event.ruleId));
    query.addBindValue(QString::fromStdString(event.sourceId));
    query.addBindValue(static_cast<qlonglong>(event.trackId));
    query.addBindValue(event.classId);
    query.addBindValue(QString::fromStdString(event.label));
    query.addBindValue(event.confidence);
    query.addBindValue(event.box.x);
    query.addBindValue(event.box.y);
    query.addBindValue(event.box.width);
    query.addBindValue(event.box.height);
    query.addBindValue(static_cast<qlonglong>(event.frameId));
    query.addBindValue(QString::fromStdString(event.message));
    query.addBindValue(QString::fromStdString(event.snapshotRef));
    query.addBindValue(static_cast<int>(event.direction));
    query.addBindValue(static_cast<qlonglong>(event.countIn));
    query.addBindValue(static_cast<qlonglong>(event.countOut));
    query.addBindValue(static_cast<qlonglong>(event.occupancy));
    return query.exec();
}

std::vector<StoredEvent> SqliteEventRepository::query(const EventQuery& query) const
{
    if (!m_open)
        return {};

    QString sql = QStringLiteral(
        "SELECT id, wall_utc_ms, pipeline_event_id, type, rule_id, source_id, track_id, "
        "class_id, label, confidence, box_x, box_y, box_w, box_h, frame_id, message, "
        "snapshot_ref, direction, count_in, count_out, occupancy "
        "FROM events WHERE id > :after");
    if (query.type.has_value())
        sql += QStringLiteral(" AND type = :type");
    sql += QStringLiteral(" ORDER BY id ASC LIMIT :limit");

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(sql);
    sqlQuery.bindValue(QStringLiteral(":after"), static_cast<qlonglong>(query.afterRowId));
    if (query.type.has_value())
        sqlQuery.bindValue(QStringLiteral(":type"), static_cast<int>(*query.type));
    sqlQuery.bindValue(QStringLiteral(":limit"), static_cast<int>(query.limit));
    if (!sqlQuery.exec())
        return {};

    std::vector<StoredEvent> rows;
    while (sqlQuery.next())
    {
        StoredEvent stored;
        stored.rowId = sqlQuery.value(0).toLongLong();
        stored.wallUtcMs = sqlQuery.value(1).toLongLong();
        stored.event = eventFromQuery(sqlQuery);
        rows.push_back(std::move(stored));
    }
    return rows;
}

void SqliteEventRepository::prune(std::size_t maxRows)
{
    if (!m_open)
        return;
    const std::size_t n = count();
    if (n <= maxRows)
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "DELETE FROM events WHERE id IN ("
        "SELECT id FROM events ORDER BY id ASC LIMIT :extra)"));
    query.bindValue(QStringLiteral(":extra"), static_cast<int>(n - maxRows));
    query.exec();
}

std::size_t SqliteEventRepository::count() const
{
    if (!m_open)
        return 0;
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM events")) || !query.next())
        return 0;
    return static_cast<std::size_t>(query.value(0).toULongLong());
}

void SqliteEventRepository::close()
{
    m_open = false;
    if (!m_db.isValid())
        return;
    const QString name = m_db.connectionName();
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

} // namespace visionlab
