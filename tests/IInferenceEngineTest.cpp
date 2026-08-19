#include <QtTest/QtTest>

#include "inference/IInferenceEngine.h"
#include "fakes/FakeInferenceEngine.h"

using visionlab::IInferenceEngine;
using visionlab::InferResult;
using visionlab::ModelConfig;
using visionlab::TensorView;

class IInferenceEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void fakeInitializeSuccessIsReady();
    void fakeInitializeFailureReportsError();
    void fakeInferReturnsPresetOutputs();
    void warmupIncrementsInferCount();
};

void IInferenceEngineTest::fakeInitializeSuccessIsReady()
{
    FakeInferenceEngine engine;
    QVERIFY(engine.initialize(ModelConfig{}));
    QVERIFY(engine.isReady());
    QCOMPARE(engine.backendId(), std::string("fake"));
    QCOMPARE(engine.device().name, std::string("CPU"));
    QVERIFY(!engine.device().gpu);
}

void IInferenceEngineTest::fakeInitializeFailureReportsError()
{
    FakeInferenceEngine engine;
    engine.setInitializeOk(false);
    engine.setErrorMessage("synthetic failure");

    QVERIFY(!engine.initialize(ModelConfig{}));
    QVERIFY(!engine.isReady());
    QVERIFY(!engine.lastError().empty());

    const InferResult result = engine.infer(TensorView{});
    QVERIFY(!result.ok);
}

void IInferenceEngineTest::fakeInferReturnsPresetOutputs()
{
    FakeInferenceEngine engine;
    QVERIFY(engine.initialize(ModelConfig{}));

    TensorView preset;
    preset.shape = {1, 85};
    preset.data.assign(85, 0.5F);
    engine.setOutputs({preset});

    const InferResult result = engine.infer(TensorView{});
    QVERIFY(result.ok);
    QCOMPARE(result.outputs.size(), size_t{1});
    QCOMPARE(result.outputs.front().shape, preset.shape);
    QCOMPARE(result.outputs.front().data.size(), preset.data.size());
    QCOMPARE(result.outputs.front().data[0], 0.5F);
}

void IInferenceEngineTest::warmupIncrementsInferCount()
{
    FakeInferenceEngine engine;
    QVERIFY(engine.initialize(ModelConfig{}));
    QCOMPARE(engine.inferCount(), 0);

    engine.warmup(3);
    QCOMPARE(engine.inferCount(), 3);
}

QTEST_APPLESS_MAIN(IInferenceEngineTest)

#include "IInferenceEngineTest.moc"
