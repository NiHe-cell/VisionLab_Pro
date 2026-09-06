#ifndef RULEMODEL_H
#define RULEMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <vector>

#include "analytics/RuleSpec.h"

// 会话规则规格列表。仅 GUI 线程调用。不持有 IRule。
class RuleModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role
    {
        RuleIdRole = Qt::UserRole + 1,
        KindRole,
        EnabledRole,
        VertexCountRole,
        LoiterSecondsRole,
        AxRole,
        AyRole,
        BxRole,
        ByRole,
        PointsRole,
    };

    explicit RuleModel(QObject* parent = nullptr);

    bool addSpec(visionlab::RuleSpec spec);
    bool removeAt(int row);
    bool setEnabled(int row, bool enabled);
    bool setLoiterSeconds(int row, double seconds);
    std::vector<visionlab::RuleSpec> specs() const;
    void replaceAll(std::vector<visionlab::RuleSpec> specs);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    bool hasId(const std::string& id) const;
    std::string allocateId(visionlab::RuleKind kind) const;

    QVector<visionlab::RuleSpec> m_specs;
};

#endif // RULEMODEL_H
