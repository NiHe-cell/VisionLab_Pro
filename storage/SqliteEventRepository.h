#ifndef SQLITEEVENTREPOSITORY_H
#define SQLITEEVENTREPOSITORY_H

#include <QSqlDatabase>
#include <QString>

#include "storage/IEventRepository.h"

namespace visionlab {

// 每个实例只用在一个线程上。连接名唯一，不占用默认连接。
class SqliteEventRepository final : public IEventRepository
{
public:
    SqliteEventRepository();
    ~SqliteEventRepository() override;

    SqliteEventRepository(const SqliteEventRepository&) = delete;
    SqliteEventRepository& operator=(const SqliteEventRepository&) = delete;

    bool open(const std::filesystem::path& dbPath) override;
    bool insert(const VisionEvent& event, std::int64_t wallUtcMs) override;
    std::vector<StoredEvent> query(const EventQuery& query) const override;
    void prune(std::size_t maxRows) override;
    std::size_t count() const override;
    void close() override;

private:
    bool execSchema();

    QString m_connectionName;
    QSqlDatabase m_db;
    bool m_open = false;
};

} // namespace visionlab

#endif // SQLITEEVENTREPOSITORY_H
