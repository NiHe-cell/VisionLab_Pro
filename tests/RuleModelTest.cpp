#include <QtTest/QtTest>

#include "analytics/RuleSpec.h"
#include "models/RuleModel.h"

using visionlab::RuleKind;
using visionlab::RuleSpec;

namespace {

RuleSpec validRoi(const std::string& id = "roi-a")
{
    RuleSpec spec;
    spec.ruleId = id;
    spec.kind = RuleKind::RoiIntrusion;
    spec.polygon = {{0.0F, 0.0F}, {10.0F, 0.0F}, {10.0F, 10.0F}, {0.0F, 10.0F}};
    return spec;
}

RuleSpec validLine(const std::string& id = "line-a")
{
    RuleSpec spec;
    spec.ruleId = id;
    spec.kind = RuleKind::LineCrossing;
    spec.a = {0.0F, 0.0F};
    spec.b = {10.0F, 0.0F};
    return spec;
}

RuleSpec validLoiter(const std::string& id = "loiter-a")
{
    RuleSpec spec;
    spec.ruleId = id;
    spec.kind = RuleKind::Loitering;
    spec.polygon = {{0.0F, 0.0F}, {8.0F, 0.0F}, {8.0F, 8.0F}};
    spec.loiterSeconds = 2.5;
    return spec;
}

} // namespace

class RuleModelTest : public QObject
{
    Q_OBJECT

private slots:
    void addValidRoi();
    void duplicateIdRejected();
    void invalidPolygonRejected();
    void emptyIdAllocatesPrefix();
    void setLoiterSecondsOnlyOnLoitering();
};

void RuleModelTest::addValidRoi()
{
    RuleModel model;
    QVERIFY(model.addSpec(validRoi()));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), RuleModel::RuleIdRole).toString(),
             QStringLiteral("roi-a"));
    QCOMPARE(model.data(model.index(0, 0), RuleModel::KindRole).toInt(),
             static_cast<int>(RuleKind::RoiIntrusion));
    QCOMPARE(model.data(model.index(0, 0), RuleModel::VertexCountRole).toInt(), 4);
    QCOMPARE(model.specs().size(), std::size_t{1});
}

void RuleModelTest::duplicateIdRejected()
{
    RuleModel model;
    QVERIFY(model.addSpec(validRoi("roi-a")));
    QVERIFY(!model.addSpec(validRoi("roi-a")));
    QCOMPARE(model.rowCount(), 1);
}

void RuleModelTest::invalidPolygonRejected()
{
    RuleModel model;
    RuleSpec spec;
    spec.ruleId = "roi-bad";
    spec.kind = RuleKind::RoiIntrusion;
    spec.polygon = {{0.0F, 0.0F}, {1.0F, 1.0F}};
    QVERIFY(!model.addSpec(spec));
    QCOMPARE(model.rowCount(), 0);
}

void RuleModelTest::emptyIdAllocatesPrefix()
{
    RuleModel model;
    RuleSpec roi = validRoi("");
    QVERIFY(model.addSpec(roi));
    QCOMPARE(model.data(model.index(0, 0), RuleModel::RuleIdRole).toString(),
             QStringLiteral("roi-1"));

    RuleSpec line = validLine("");
    QVERIFY(model.addSpec(line));
    QCOMPARE(model.data(model.index(1, 0), RuleModel::RuleIdRole).toString(),
             QStringLiteral("line-1"));
}

void RuleModelTest::setLoiterSecondsOnlyOnLoitering()
{
    RuleModel model;
    QVERIFY(model.addSpec(validRoi()));
    QVERIFY(model.addSpec(validLoiter()));
    QVERIFY(!model.setLoiterSeconds(0, 3.0));
    QVERIFY(model.setLoiterSeconds(1, 4.0));
    QCOMPARE(model.data(model.index(1, 0), RuleModel::LoiterSecondsRole).toDouble(), 4.0);
    QVERIFY(model.setEnabled(0, false));
    QVERIFY(!model.data(model.index(0, 0), RuleModel::EnabledRole).toBool());
}

QTEST_GUILESS_MAIN(RuleModelTest)

#include "RuleModelTest.moc"
