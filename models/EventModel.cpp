#include "models/EventModel.h"

#include <QDateTime>

EventModel::EventModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void EventModel::beginSession()
{
    m_maxSessionEventId = 0;
}

EventModel::Row EventModel::fromEvent(const visionlab::VisionEvent& event,
                                      qint64 wallUtcMs,
                                      qint64 rowId)
{
    Row row;
    row.sessionEventId = event.eventId;
    row.rowId = rowId;
    row.wallUtcMs = wallUtcMs;
    row.type = static_cast<int>(event.type);
    row.ruleId = QString::fromStdString(event.ruleId);
    row.trackId = event.trackId;
    row.label = QString::fromStdString(event.label);
    row.confidence = event.confidence;
    row.message = QString::fromStdString(event.message);
    row.direction = static_cast<int>(event.direction);
    row.countIn = event.countIn;
    row.countOut = event.countOut;
    row.occupancy = event.occupancy;
    row.frameId = event.frameId;
    return row;
}

void EventModel::appendRows(const QVector<Row>& rows)
{
    if (rows.isEmpty())
        return;
    const int first = m_rows.size();
    beginInsertRows(QModelIndex(), first, first + rows.size() - 1);
    m_rows += rows;
    endInsertRows();
}

void EventModel::ingest(const std::vector<visionlab::VisionEvent>& snapshot,
                        const std::vector<visionlab::StoredEvent>& history)
{
    if (!m_historyLoaded && !history.empty())
    {
        QVector<Row> histRows;
        histRows.reserve(static_cast<int>(history.size()));
        for (const visionlab::StoredEvent& stored : history)
            histRows.push_back(fromEvent(stored.event, stored.wallUtcMs, stored.rowId));
        appendRows(histRows);
        m_historyLoaded = true;
    }

    const qint64 wall = QDateTime::currentMSecsSinceEpoch();
    QVector<Row> live;
    quint64 newMax = m_maxSessionEventId;
    for (const visionlab::VisionEvent& event : snapshot)
    {
        if (event.eventId <= m_maxSessionEventId)
            continue;
        live.push_back(fromEvent(event, wall, 0));
        if (event.eventId > newMax)
            newMax = event.eventId;
    }
    appendRows(live);
    m_maxSessionEventId = newMax;
}

int EventModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant EventModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row& row = m_rows.at(index.row());
    switch (role)
    {
    case SessionEventIdRole:
        return QVariant::fromValue(row.sessionEventId);
    case RowIdRole:
        return QVariant::fromValue(row.rowId);
    case WallUtcMsRole:
        return QVariant::fromValue(row.wallUtcMs);
    case TypeRole:
        return row.type;
    case RuleIdRole:
        return row.ruleId;
    case TrackIdRole:
        return QVariant::fromValue(row.trackId);
    case LabelRole:
        return row.label;
    case ConfidenceRole:
        return row.confidence;
    case MessageRole:
        return row.message;
    case DirectionRole:
        return row.direction;
    case CountInRole:
        return QVariant::fromValue(row.countIn);
    case CountOutRole:
        return QVariant::fromValue(row.countOut);
    case OccupancyRole:
        return QVariant::fromValue(row.occupancy);
    case FrameIdRole:
        return QVariant::fromValue(row.frameId);
    default:
        return {};
    }
}

QHash<int, QByteArray> EventModel::roleNames() const
{
    return {
        {SessionEventIdRole, "sessionEventId"},
        {RowIdRole, "rowId"},
        {WallUtcMsRole, "wallUtcMs"},
        {TypeRole, "type"},
        {RuleIdRole, "ruleId"},
        {TrackIdRole, "trackId"},
        {LabelRole, "label"},
        {ConfidenceRole, "confidence"},
        {MessageRole, "message"},
        {DirectionRole, "direction"},
        {CountInRole, "countIn"},
        {CountOutRole, "countOut"},
        {OccupancyRole, "occupancy"},
        {FrameIdRole, "frameId"},
    };
}
