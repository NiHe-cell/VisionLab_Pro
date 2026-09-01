#include "models/DetectionModel.h"

DetectionModel::DetectionModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void DetectionModel::setDetections(const std::vector<visionlab::Detection>& detections)
{
    beginResetModel();
    m_rows.clear();
    m_rows.reserve(static_cast<int>(detections.size()));
    for (const visionlab::Detection& detection : detections)
    {
        Row row;
        row.classId = detection.classId;
        row.label = QString::fromStdString(detection.label);
        row.confidence = detection.confidence;
        row.x = detection.box.x;
        row.y = detection.box.y;
        row.width = detection.box.width;
        row.height = detection.box.height;
        m_rows.push_back(std::move(row));
    }
    endResetModel();
}

int DetectionModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant DetectionModel::data(const QModelIndex& index, int role) const
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
    default:
        return {};
    }
}

QHash<int, QByteArray> DetectionModel::roleNames() const
{
    return {
        {ClassIdRole, "classId"},
        {LabelRole, "label"},
        {ConfidenceRole, "confidence"},
        {XRole, "x"},
        {YRole, "y"},
        {WidthRole, "width"},
        {HeightRole, "height"},
    };
}
