#ifndef EVENTTYPEFILTERMODEL_H
#define EVENTTYPEFILTERMODEL_H

#include <QSortFilterProxyModel>

// 按 EventType 整值过滤 EventModel。typeFilter == -1 表示全部。
class EventTypeFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int typeFilter READ typeFilter WRITE setTypeFilter NOTIFY typeFilterChanged)
public:
    explicit EventTypeFilterModel(QObject* parent = nullptr);

    int typeFilter() const;
    void setTypeFilter(int filter);

signals:
    void typeFilterChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int m_typeFilter = -1;
};

#endif // EVENTTYPEFILTERMODEL_H
