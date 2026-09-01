#include <QtTest/QtTest>

#include "core/Detection.h"
#include "models/DetectionModel.h"

using visionlab::Detection;

class DetectionModelTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultIsEmpty();
    void twoDetectionsExposeRoles();
    void replaceShrinksAndEmptyClears();
};

void DetectionModelTest::defaultIsEmpty()
{
    DetectionModel model;
    QCOMPARE(model.rowCount(), 0);
}

void DetectionModelTest::twoDetectionsExposeRoles()
{
    DetectionModel model;
    Detection a;
    a.classId = 0;
    a.label = "person";
    a.confidence = 0.9F;
    a.box = cv::Rect(1, 2, 3, 4);
    Detection b;
    b.classId = 2;
    b.label = "car";
    b.confidence = 0.5F;
    b.box = cv::Rect(10, 20, 30, 40);
    model.setDetections({a, b});

    QCOMPARE(model.rowCount(), 2);
    const QModelIndex first = model.index(0, 0);
    QCOMPARE(model.data(first, DetectionModel::ClassIdRole).toInt(), 0);
    QCOMPARE(model.data(first, DetectionModel::LabelRole).toString(), QStringLiteral("person"));
    QCOMPARE(model.data(first, DetectionModel::ConfidenceRole).toFloat(), 0.9F);
    QCOMPARE(model.data(first, DetectionModel::XRole).toInt(), 1);
    QCOMPARE(model.data(first, DetectionModel::YRole).toInt(), 2);
    QCOMPARE(model.data(first, DetectionModel::WidthRole).toInt(), 3);
    QCOMPARE(model.data(first, DetectionModel::HeightRole).toInt(), 4);

    const auto names = model.roleNames();
    QCOMPARE(names.value(DetectionModel::ClassIdRole), QByteArray("classId"));
    QCOMPARE(names.value(DetectionModel::LabelRole), QByteArray("label"));
    QCOMPARE(names.value(DetectionModel::ConfidenceRole), QByteArray("confidence"));
    QCOMPARE(names.value(DetectionModel::XRole), QByteArray("x"));
    QCOMPARE(names.value(DetectionModel::YRole), QByteArray("y"));
    QCOMPARE(names.value(DetectionModel::WidthRole), QByteArray("width"));
    QCOMPARE(names.value(DetectionModel::HeightRole), QByteArray("height"));
}

void DetectionModelTest::replaceShrinksAndEmptyClears()
{
    DetectionModel model;
    Detection a;
    a.label = "a";
    Detection b;
    b.label = "b";
    model.setDetections({a, b});
    Detection c;
    c.label = "c";
    model.setDetections({c});
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), DetectionModel::LabelRole).toString(),
             QStringLiteral("c"));

    model.setDetections({});
    QCOMPARE(model.rowCount(), 0);
}

QTEST_GUILESS_MAIN(DetectionModelTest)

#include "DetectionModelTest.moc"
