#ifndef FAKEINFERENCEENGINE_H
#define FAKEINFERENCEENGINE_H

#include <utility>
#include <vector>

#include "inference/IInferenceEngine.h"

// 确定性假引擎：供检测器单测注入，不链接 ONNX Runtime。
class FakeInferenceEngine : public visionlab::IInferenceEngine
{
public:
    bool initialize(const visionlab::ModelConfig&) override
    {
        m_ready = m_initializeOk;
        if (!m_ready && m_error.empty())
            m_error = "fake initialize failed";
        if (m_ready)
            m_error.clear();
        return m_ready;
    }

    bool isReady() const override { return m_ready; }
    std::string lastError() const override { return m_error; }
    std::string backendId() const override { return "fake"; }

    visionlab::DeviceInfo device() const override
    {
        visionlab::DeviceInfo info;
        info.name = "CPU";
        info.deviceId = 0;
        info.gpu = false;
        return info;
    }

    visionlab::TensorMetadata inputMetadata() const override { return m_inputMeta; }
    std::vector<visionlab::TensorMetadata> outputMetadata() const override
    {
        return m_outputMeta;
    }

    visionlab::InferResult infer(const visionlab::TensorView&) override
    {
        ++m_inferCount;
        visionlab::InferResult result;
        if (!m_ready)
        {
            result.ok = false;
            result.error = m_error.empty() ? "engine not ready" : m_error;
            return result;
        }
        result.ok = true;
        result.outputs = m_outputs;
        return result;
    }

    void warmup(int iterations) override
    {
        visionlab::TensorView dummy;
        for (int i = 0; i < iterations; ++i)
            infer(dummy);
    }

    void setInitializeOk(bool ok) { m_initializeOk = ok; }
    void setErrorMessage(std::string message) { m_error = std::move(message); }
    void setOutputs(std::vector<visionlab::TensorView> outputs)
    {
        m_outputs = std::move(outputs);
    }
    int inferCount() const { return m_inferCount; }

private:
    bool m_initializeOk = true;
    bool m_ready = false;
    std::string m_error;
    std::vector<visionlab::TensorView> m_outputs;
    visionlab::TensorMetadata m_inputMeta;
    std::vector<visionlab::TensorMetadata> m_outputMeta;
    int m_inferCount = 0;
};

#endif // FAKEINFERENCEENGINE_H
