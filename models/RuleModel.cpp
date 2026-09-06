#include "models/RuleModel.h"

#include "analytics/RuleFactory.h"

namespace {

const char* idPrefix(visionlab::RuleKind kind)
{
    switch (kind)
    {
    case visionlab::RuleKind::RoiIntrusion:
        return "roi-";
    case visionlab::RuleKind::LineCrossing:
        return "line-";
    case visionlab::RuleKind::Loitering:
        return "loiter-";
    case visionlab::RuleKind::Counting:
        return "count-";
    }
    return "rule-";
}

} // namespace

RuleModel::RuleModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

bool RuleModel::hasId(const std::string& id) const
{
    for (const visionlab::RuleSpec& spec : m_specs)
    {
        if (spec.ruleId == id)
            return true;
    }
    return false;
}

std::string RuleModel::allocateId(visionlab::RuleKind kind) const
{
    int count = 0;
    for (const visionlab::RuleSpec& spec : m_specs)
    {
        if (spec.kind == kind)
            ++count;
    }
    return std::string(idPrefix(kind)) + std::to_string(count + 1);
}

bool RuleModel::addSpec(visionlab::RuleSpec spec)
{
    if (spec.ruleId.empty())
        spec.ruleId = allocateId(spec.kind);
    else if (hasId(spec.ruleId))
        return false;
    if (!visionlab::makeRule(spec))
        return false;

    const int row = m_specs.size();
    beginInsertRows(QModelIndex(), row, row);
    m_specs.push_back(std::move(spec));
    endInsertRows();
    return true;
}

bool RuleModel::removeAt(int row)
{
    if (row < 0 || row >= m_specs.size())
        return false;
    beginRemoveRows(QModelIndex(), row, row);
    m_specs.removeAt(row);
    endRemoveRows();
    return true;
}

bool RuleModel::setEnabled(int row, bool enabled)
{
    if (row < 0 || row >= m_specs.size())
        return false;
    if (m_specs[row].enabled == enabled)
        return true;
    m_specs[row].enabled = enabled;
    const QModelIndex index = this->index(row, 0);
    emit dataChanged(index, index, {EnabledRole});
    return true;
}

bool RuleModel::setLoiterSeconds(int row, double seconds)
{
    if (row < 0 || row >= m_specs.size())
        return false;
    if (m_specs[row].kind != visionlab::RuleKind::Loitering)
        return false;
    if (seconds <= 0.0)
        return false;
    m_specs[row].loiterSeconds = seconds;
    const QModelIndex index = this->index(row, 0);
    emit dataChanged(index, index, {LoiterSecondsRole});
    return true;
}

std::vector<visionlab::RuleSpec> RuleModel::specs() const
{
    return std::vector<visionlab::RuleSpec>(m_specs.begin(), m_specs.end());
}

void RuleModel::replaceAll(std::vector<visionlab::RuleSpec> specs)
{
    beginResetModel();
    m_specs.clear();
    m_specs.reserve(static_cast<int>(specs.size()));
    for (visionlab::RuleSpec& spec : specs)
        m_specs.push_back(std::move(spec));
    endResetModel();
}

int RuleModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_specs.size();
}

QVariant RuleModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_specs.size())
        return {};
    const visionlab::RuleSpec& spec = m_specs.at(index.row());
    switch (role)
    {
    case RuleIdRole:
        return QString::fromStdString(spec.ruleId);
    case KindRole:
        return static_cast<int>(spec.kind);
    case EnabledRole:
        return spec.enabled;
    case VertexCountRole:
        return static_cast<int>(spec.polygon.size());
    case LoiterSecondsRole:
        return spec.loiterSeconds;
    case AxRole:
        return spec.a.x;
    case AyRole:
        return spec.a.y;
    case BxRole:
        return spec.b.x;
    case ByRole:
        return spec.b.y;
    default:
        return {};
    }
}

QHash<int, QByteArray> RuleModel::roleNames() const
{
    return {
        {RuleIdRole, "ruleId"},
        {KindRole, "kind"},
        {EnabledRole, "enabled"},
        {VertexCountRole, "vertexCount"},
        {LoiterSecondsRole, "loiterSeconds"},
        {AxRole, "ax"},
        {AyRole, "ay"},
        {BxRole, "bx"},
        {ByRole, "by"},
    };
}
