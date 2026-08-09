#include <QtTest/QtTest>

#include "core/VisionTypes.h"

using visionlab::DetectionMode;

class ModeMappingTest : public QObject
{
    Q_OBJECT

private slots:
    void labelRoundTrip();
    void unknownLabelFallsToNone();
    void noneLabelIsStable();
};

void ModeMappingTest::labelRoundTrip()
{
    for (const DetectionMode mode :
         {DetectionMode::Face, DetectionMode::Object, DetectionMode::Motion})
    {
        QCOMPARE(visionlab::detectionModeFromLabel(visionlab::labelForDetectionMode(mode)),
                 mode);
    }
}

void ModeMappingTest::unknownLabelFallsToNone()
{
    // 未识别的 UI 字符串必须安全落到 None，不得误触发任何检测。
    QCOMPARE(visionlab::detectionModeFromLabel("garbage"), DetectionMode::None);
    QCOMPARE(visionlab::detectionModeFromLabel(""), DetectionMode::None);
}

void ModeMappingTest::noneLabelIsStable()
{
    QCOMPARE(visionlab::labelForDetectionMode(DetectionMode::None),
             std::string_view("None"));
    // 旧 QML 字符串必须与枚举保持一一对应（防 UI 文案漂移）。
    QCOMPARE(visionlab::kFaceModeLabel, std::string_view("Face Detection"));
    QCOMPARE(visionlab::kObjectModeLabel, std::string_view("Object Detection"));
    QCOMPARE(visionlab::kMotionModeLabel, std::string_view("Motion Detection"));
}

QTEST_APPLESS_MAIN(ModeMappingTest)

#include "ModeMappingTest.moc"
