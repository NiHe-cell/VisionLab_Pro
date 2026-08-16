#include <QtTest/QtTest>

#include <QCoreApplication>

#include "controllers/LocaleController.h"

class LocaleControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void toggleSwitchesBetweenChineseAndEnglish();
    void setLanguageNormalizesChinesePrefix();
};

void LocaleControllerTest::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("VisionLab.Test"));
    QCoreApplication::setApplicationName(QStringLiteral("LocaleControllerTest"));
}

void LocaleControllerTest::toggleSwitchesBetweenChineseAndEnglish()
{
    LocaleController locale;
    const bool startedChinese = locale.isChinese();
    locale.toggle();
    QCOMPARE(locale.isChinese(), !startedChinese);
    locale.toggle();
    QCOMPARE(locale.isChinese(), startedChinese);
}

void LocaleControllerTest::setLanguageNormalizesChinesePrefix()
{
    LocaleController locale;
    locale.setLanguage("zh");
    QVERIFY(locale.isChinese());
    QCOMPARE(locale.language(), QString("zh_CN"));
    locale.setLanguage("en_US");
    QVERIFY(!locale.isChinese());
    QCOMPARE(locale.language(), QString("en"));
}

QTEST_GUILESS_MAIN(LocaleControllerTest)

#include "LocaleControllerTest.moc"
