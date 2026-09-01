#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <vector>

#include "core/VisionEvent.h"
#include "storage/EventQuery.h"

// 事件列表：按 session 增量追加。仅 GUI 线程调用 ingest / beginSession。
class EventModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role
    {
        SessionEventIdRole = Qt::UserRole + 1,
        RowIdRole,
        WallUtcMsRole,
        TypeRole,
        RuleIdRole,
        TrackIdRole,
        LabelRole,
        ConfidenceRole,
        MessageRole,
        DirectionRole,
        CountInRole,
        CountOutRole,
        OccupancyRole,
        FrameIdRole,
    };

    explicit EventModel(QObject* parent = nullptr);

    void beginSession();
    void ingest(const std::vector<visionlab::VisionEvent>& snapshot,
                const std::vector<visionlab::StoredEvent>& history = {});

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct Row
    {
        quint64 sessionEventId = 0;
        qint64 rowId = 0;
        qint64 wallUtcMs = 0;
        int type = 0;
        QString ruleId;
        quint64 trackId = 0;
        QString label;
        float confidence = 0.0F;
        QString message;
        int direction = 0;
        quint64 countIn = 0;
        quint64 countOut = 0;
        quint64 occupancy = 0;
        qint64 frameId = 0;
    };

    static Row fromEvent(const visionlab::VisionEvent& event, qint64 wallUtcMs, qint64 rowId);
    void appendRows(const QVector<Row>& rows);

    QVector<Row> m_rows;
    quint64 m_maxSessionEventId = 0;
    bool m_historyLoaded = false;
};

#endif // EVENTMODEL_H
