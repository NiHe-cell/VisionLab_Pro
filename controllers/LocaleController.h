#ifndef LOCALECONTROLLER_H
#define LOCALECONTROLLER_H

#include <QObject>
#include <QString>

// GUI 语言：en / zh_CN。检测模式仍用英文领域标签传给 VisionController。
class LocaleController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(bool chinese READ isChinese NOTIFY languageChanged)
public:
    explicit LocaleController(QObject* parent = nullptr);

    QString language() const;
    bool isChinese() const;

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void setLanguage(const QString& language);

signals:
    void languageChanged();

private:
    QString normalize(const QString& language) const;
    void persist() const;

    QString m_language;
};

#endif // LOCALECONTROLLER_H
