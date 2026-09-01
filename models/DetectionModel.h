#ifndef DETECTIONMODEL_H
#define DETECTIONMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <vector>

#include "core/Detection.h"

// 只读检测列表。仅在 GUI 线程调用 setDetections。不持有 cv::Mat。
class DetectionModel : public QAbstractListModel
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
    };

    explicit DetectionModel(QObject* parent = nullptr);

    void setDetections(const std::vector<visionlab::Detection>& detections);

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
    };

    QVector<Row> m_rows;
};

#endif // DETECTIONMODEL_H
