#include "models/PluginModel.h"

#include "core/VisionTypes.h"

PluginModel::PluginModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void PluginModel::setPlugins(const std::vector<visionlab::PluginMetadata>& plugins,
                             const std::vector<std::string>& errors)
{
    beginResetModel();
    m_rows.clear();
    m_rows.reserve(static_cast<int>(plugins.size()));
    for (const visionlab::PluginMetadata& meta : plugins)
    {
        Row row;
        row.pluginId = QString::fromStdString(meta.id);
        row.name = QString::fromStdString(meta.name);
        row.version = QString::fromStdString(meta.version);
        row.description = QString::fromStdString(meta.description);
        if (meta.mode.has_value())
        {
            const auto label = visionlab::labelForDetectionMode(*meta.mode);
            row.modeLabel = QString::fromUtf8(label.data(), static_cast<int>(label.size()));
        }
        for (const std::string& cap : meta.capabilities)
            row.capabilities.push_back(QString::fromStdString(cap));
        m_rows.push_back(std::move(row));
    }
    m_errors.clear();
    for (const std::string& error : errors)
        m_errors.push_back(QString::fromStdString(error));
    endResetModel();
}

QStringList PluginModel::loadErrors() const
{
    return m_errors;
}

int PluginModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant PluginModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row& row = m_rows.at(index.row());
    switch (role)
    {
    case PluginIdRole:
        return row.pluginId;
    case NameRole:
        return row.name;
    case VersionRole:
        return row.version;
    case DescriptionRole:
        return row.description;
    case ModeLabelRole:
        return row.modeLabel;
    case CapabilitiesRole:
        return row.capabilities;
    default:
        return {};
    }
}

QHash<int, QByteArray> PluginModel::roleNames() const
{
    return {
        {PluginIdRole, "pluginId"},
        {NameRole, "name"},
        {VersionRole, "version"},
        {DescriptionRole, "description"},
        {ModeLabelRole, "modeLabel"},
        {CapabilitiesRole, "capabilities"},
    };
}
