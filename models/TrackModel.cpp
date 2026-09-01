#include "models/TrackModel.h"

TrackModel::TrackModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void TrackModel::setTracks(const std::vector<visionlab::Track>& tracks)
{
    beginResetModel();
    m_rows.clear();
    m_rows.reserve(static_cast<int>(tracks.size()));
    for (const visionlab::Track& track : tracks)
    {
        Row row;
        row.classId = track.classId;
        row.label = QString::fromStdString(track.label);
        row.confidence = track.confidence;
        row.x = track.box.x;
        row.y = track.box.y;
        row.width = track.box.width;
        row.height = track.box.height;
        row.trackId = track.trackId;
        row.state = static_cast<int>(track.state);
        row.age = track.age;
        m_rows.push_back(std::move(row));
    }
    endResetModel();
}

int TrackModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant TrackModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row& row = m_rows.at(index.row());
    switch (role)
    {
    case ClassIdRole:
        return row.classId;
    case LabelRole:
        return row.label;
    case ConfidenceRole:
        return row.confidence;
    case XRole:
        return row.x;
    case YRole:
        return row.y;
    case WidthRole:
        return row.width;
    case HeightRole:
        return row.height;
    case TrackIdRole:
        return QVariant::fromValue(row.trackId);
    case StateRole:
        return row.state;
    case AgeRole:
        return row.age;
    default:
        return {};
    }
}

QHash<int, QByteArray> TrackModel::roleNames() const
{
    return {
        {ClassIdRole, "classId"},
        {LabelRole, "label"},
        {ConfidenceRole, "confidence"},
        {XRole, "x"},
        {YRole, "y"},
        {WidthRole, "width"},
        {HeightRole, "height"},
        {TrackIdRole, "trackId"},
        {StateRole, "state"},
        {AgeRole, "age"},
    };
}
