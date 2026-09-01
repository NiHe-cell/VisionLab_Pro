#ifndef TRACKMODEL_H
#define TRACKMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <vector>

#include "core/Track.h"

// 只读轨迹列表。仅在 GUI 线程调用 setTracks。不暴露 trajectory，不持有像素。
class TrackModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role
    {
        ClassIdRole = Qt::UserRole + 1,
        LabelRole,
        ConfidenceRole,
        XRole,
        YRole,
        WidthRole,
        HeightRole,
        TrackIdRole,
        StateRole,
        AgeRole,
    };

    explicit TrackModel(QObject* parent = nullptr);

    void setTracks(const std::vector<visionlab::Track>& tracks);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct Row
    {
        int classId = -1;
        QString label;
        float confidence = 0.0F;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        quint64 trackId = 0;
        int state = 0;
        int age = 0;
    };

    QVector<Row> m_rows;
};

#endif // TRACKMODEL_H
