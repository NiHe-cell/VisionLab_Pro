#ifndef PLUGINMODEL_H
#define PLUGINMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QVector>
#include <string>
#include <vector>

#include "plugin/PluginMetadata.h"

// 已扫描插件的只读列表。仅 GUI 线程调用 setPlugins。
class PluginModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role
    {
        PluginIdRole = Qt::UserRole + 1,
        NameRole,
        VersionRole,
        DescriptionRole,
        ModeLabelRole,
        CapabilitiesRole,
    };

    explicit PluginModel(QObject* parent = nullptr);

    void setPlugins(const std::vector<visionlab::PluginMetadata>& plugins,
                    const std::vector<std::string>& errors);

    Q_INVOKABLE QStringList loadErrors() const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct Row
    {
        QString pluginId;
        QString name;
        QString version;
        QString description;
        QString modeLabel;
        QStringList capabilities;
    };

    QVector<Row> m_rows;
    QStringList m_errors;
};

#endif // PLUGINMODEL_H
