#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "plugin/PluginManager.h"

using visionlab::DetectorCreateRequest;
using visionlab::PluginManager;

namespace {

bool copyPlugin(const QString& from, const QString& to)
{
    QFile::remove(to);
    return QFile::copy(from, to);
}

bool hasErrorContaining(const PluginManager& manager, const char* needle)
{
    const QString n = QString::fromUtf8(needle).toLower();
    for (const auto& error : manager.errors())
    {
        if (QString::fromStdString(error).toLower().contains(n))
            return true;
    }
    return false;
}

QString dummyPluginPath()
{
    return QString::fromUtf8(VISIONLAB_DUMMY_PLUGIN);
}

} // namespace

class PluginManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void missingDirectoryIsEmptySuccess();
    void emptyDirectoryIsEmptySuccess();
    void loadsDummyFromDirectory();
    void skipsInvalidLibraryAndKeepsDummy();
    void rejectsWrongIid();
    void rejectsDuplicateId();
    void createFailureReturnsNull();
    void unknownIdReturnsNull();
    void detectorsWorkUntilDestroyed();
};

void PluginManagerTest::missingDirectoryIsEmptySuccess()
{
    PluginManager manager;
    manager.scan(QDir::temp().filePath(QStringLiteral("visionlab-no-plugin-dir-9f3c")).toStdString());
    QVERIFY(manager.metadata().empty());
    QVERIFY(manager.errors().empty());
}

void PluginManagerTest::emptyDirectoryIsEmptySuccess()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    PluginManager manager;
    manager.scan(dir.path().toStdString());
    QVERIFY(manager.metadata().empty());
    QVERIFY(manager.errors().empty());
}

void PluginManagerTest::loadsDummyFromDirectory()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString dest = dir.filePath(QFileInfo(dummyPluginPath()).fileName());
    QVERIFY(copyPlugin(dummyPluginPath(), dest));

    PluginManager manager;
    manager.scan(dir.path().toStdString());
    QCOMPARE(manager.metadata().size(), size_t{1});
    QCOMPARE(manager.metadata().front().id, std::string("vision.dummy"));

    const auto detector = manager.createDetector("vision.dummy", DetectorCreateRequest{});
    QVERIFY(detector);
    QCOMPARE(detector->name(), std::string("Dummy"));
}

void PluginManagerTest::skipsInvalidLibraryAndKeepsDummy()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(copyPlugin(dummyPluginPath(),
                       dir.filePath(QFileInfo(dummyPluginPath()).fileName())));
    QFile garbage(dir.filePath(QStringLiteral("vision_garbage.dll")));
    QVERIFY(garbage.open(QIODevice::WriteOnly | QIODevice::Truncate));
    garbage.write("not a plugin");
    garbage.close();

    PluginManager manager;
    manager.scan(dir.path().toStdString());
    QVERIFY(!manager.errors().empty());
    QCOMPARE(manager.metadata().size(), size_t{1});
    QCOMPARE(manager.metadata().front().id, std::string("vision.dummy"));
}

void PluginManagerTest::rejectsWrongIid()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString src = QString::fromUtf8(VISIONLAB_WRONG_IID_PLUGIN);
    QVERIFY(copyPlugin(src, dir.filePath(QFileInfo(src).fileName())));

    PluginManager manager;
    manager.scan(dir.path().toStdString());
    QVERIFY(manager.metadata().empty());
    QVERIFY(hasErrorContaining(manager, "iid")
            || hasErrorContaining(manager, "interface"));
}

void PluginManagerTest::rejectsDuplicateId()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString name = QFileInfo(dummyPluginPath()).fileName();
    QVERIFY(copyPlugin(dummyPluginPath(), dir.filePath(name)));
    QVERIFY(copyPlugin(dummyPluginPath(), dir.filePath(QStringLiteral("vision_dummy_copy.dll"))));

    PluginManager manager;
    manager.scan(dir.path().toStdString());
    QCOMPARE(manager.metadata().size(), size_t{1});
    QVERIFY(hasErrorContaining(manager, "duplicate"));
}

void PluginManagerTest::createFailureReturnsNull()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString src = QString::fromUtf8(VISIONLAB_FAIL_CREATE_PLUGIN);
    QVERIFY(copyPlugin(src, dir.filePath(QFileInfo(src).fileName())));

    PluginManager manager;
    manager.scan(dir.path().toStdString());
    QCOMPARE(manager.metadata().size(), size_t{1});
    QCOMPARE(manager.metadata().front().id, std::string("vision.fail-create"));

    QVERIFY(!manager.createDetector("vision.fail-create", DetectorCreateRequest{}));
    QVERIFY(!manager.errors().empty());
}

void PluginManagerTest::unknownIdReturnsNull()
{
    PluginManager manager;
    QVERIFY(!manager.createDetector("vision.missing", DetectorCreateRequest{}));
    QVERIFY(hasErrorContaining(manager, "unknown"));
}

void PluginManagerTest::detectorsWorkUntilDestroyed()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(copyPlugin(dummyPluginPath(),
                       dir.filePath(QFileInfo(dummyPluginPath()).fileName())));

    PluginManager manager;
    manager.scan(dir.path().toStdString());
    auto first = manager.createDetector("vision.dummy", DetectorCreateRequest{});
    auto second = manager.createDetector("vision.dummy", DetectorCreateRequest{});
    QVERIFY(first);
    QVERIFY(second);
    QVERIFY(first->isReady());
    QVERIFY(second->isReady());
    first.reset();
    QVERIFY(second->isReady());
    second.reset();
}

QTEST_GUILESS_MAIN(PluginManagerTest)

#include "PluginManagerTest.moc"
