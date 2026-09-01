#include "models/EventTypeFilterModel.h"

#include "models/EventModel.h"

EventTypeFilterModel::EventTypeFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

int EventTypeFilterModel::typeFilter() const
{
    return m_typeFilter;
}

void EventTypeFilterModel::setTypeFilter(int filter)
{
    if (m_typeFilter == filter)
        return;
    m_typeFilter = filter;
    invalidateFilter();
    emit typeFilterChanged();
}

bool EventTypeFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_typeFilter < 0)
        return true;
    const QAbstractItemModel* src = sourceModel();
    if (!src)
        return false;
    const QModelIndex index = src->index(sourceRow, 0, sourceParent);
    return src->data(index, EventModel::TypeRole).toInt() == m_typeFilter;
}
