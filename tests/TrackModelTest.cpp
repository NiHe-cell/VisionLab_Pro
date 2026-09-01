#include <QtTest/QtTest>

#include "core/Track.h"
#include "models/TrackModel.h"

using visionlab::Track;
using visionlab::TrackState;

class TrackModelTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultIsEmpty();
    void exposesTrackIdStateAndAge();
    void emptyClears();
};

void TrackModelTest::defaultIsEmpty()
{
    TrackModel model;
    QCOMPARE(model.rowCount(), 0);
}

void TrackModelTest::exposesTrackIdStateAndAge()
{
    TrackModel model;
    Track confirmed;
    confirmed.trackId = 17;
    confirmed.classId = 0;
    confirmed.label = "person";
    confirmed.confidence = 0.8F;
    confirmed.box = cv::Rect(5, 6, 7, 8);
    confirmed.state = TrackState::Confirmed;
    confirmed.age = 4;
    Track lost;
    lost.trackId = 18;
    lost.state = TrackState::Lost;
    lost.age = 12;
    Track tentative;
    tentative.trackId = 19;
    tentative.state = TrackState::Tentative;
    model.setTracks({confirmed, lost, tentative});

    QCOMPARE(model.rowCount(), 3);
    const QModelIndex first = model.index(0, 0);
    QCOMPARE(model.data(first, TrackModel::TrackIdRole).toULongLong(), quint64{17});
    QCOMPARE(model.data(first, TrackModel::StateRole).toInt(),
             static_cast<int>(TrackState::Confirmed));
    QCOMPARE(model.data(first, TrackModel::AgeRole).toInt(), 4);
    QCOMPARE(model.data(first, TrackModel::LabelRole).toString(), QStringLiteral("person"));
    QCOMPARE(model.data(first, TrackModel::XRole).toInt(), 5);

    QCOMPARE(model.data(model.index(1, 0), TrackModel::StateRole).toInt(),
             static_cast<int>(TrackState::Lost));
    QCOMPARE(model.data(model.index(2, 0), TrackModel::StateRole).toInt(),
             static_cast<int>(TrackState::Tentative));

    const auto names = model.roleNames();
    QCOMPARE(names.value(TrackModel::TrackIdRole), QByteArray("trackId"));
    QCOMPARE(names.value(TrackModel::StateRole), QByteArray("state"));
    QCOMPARE(names.value(TrackModel::AgeRole), QByteArray("age"));
    QCOMPARE(names.value(TrackModel::ClassIdRole), QByteArray("classId"));
}

void TrackModelTest::emptyClears()
{
    TrackModel model;
    Track track;
    track.trackId = 1;
    model.setTracks({track});
    model.setTracks({});
    QCOMPARE(model.rowCount(), 0);
}

QTEST_GUILESS_MAIN(TrackModelTest)

#include "TrackModelTest.moc"
