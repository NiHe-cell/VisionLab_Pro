#include "LocaleController.h"

#include <QCoreApplication>
#include <QLocale>
#include <QSettings>

namespace {

constexpr auto kKey = "ui/language";

QSettings makeSettings()
{
    const QString org = QCoreApplication::organizationName().isEmpty()
                            ? QStringLiteral("VisionLab")
                            : QCoreApplication::organizationName();
    const QString app = QCoreApplication::applicationName().isEmpty()
                            ? QStringLiteral("VisionLab")
                            : QCoreApplication::applicationName();
    return QSettings(org, app);
}

} // namespace

LocaleController::LocaleController(QObject* parent)
    : QObject(parent)
{
    const QString stored = makeSettings().value(kKey).toString();
    if (!stored.isEmpty())
    {
        m_language = normalize(stored);
        return;
    }

    m_language = QLocale::system().language() == QLocale::Chinese
                     ? QStringLiteral("zh_CN")
                     : QStringLiteral("en");
}

QString LocaleController::language() const
{
    return m_language;
}

bool LocaleController::isChinese() const
{
    return m_language == QLatin1String("zh_CN");
}

void LocaleController::toggle()
{
    setLanguage(isChinese() ? QStringLiteral("en") : QStringLiteral("zh_CN"));
}

void LocaleController::setLanguage(const QString& language)
{
    const QString next = normalize(language);
    if (next == m_language)
        return;
    m_language = next;
    persist();
    emit languageChanged();
}

QString LocaleController::normalize(const QString& language) const
{
    if (language.startsWith(QLatin1String("zh"), Qt::CaseInsensitive))
        return QStringLiteral("zh_CN");
    return QStringLiteral("en");
}

void LocaleController::persist() const
{
    makeSettings().setValue(kKey, m_language);
}

