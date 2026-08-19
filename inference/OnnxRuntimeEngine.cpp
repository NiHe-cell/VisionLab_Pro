#include "OnnxRuntimeEngine.h"

namespace visionlab {

struct OnnxRuntimeEngine::Impl
{
    std::string error{"ONNX Runtime session is not implemented"};
};

OnnxRuntimeEngine::OnnxRuntimeEngine()
    : m_impl(std::make_unique<Impl>())
{
}

OnnxRuntimeEngine::~OnnxRuntimeEngine() = default;

bool OnnxRuntimeEngine::initialize(const ModelConfig&)
{
    m_impl->error = "ONNX Runtime session is not implemented";
    return false;
}

bool OnnxRuntimeEngine::isReady() const
{
    return false;
}

std::string OnnxRuntimeEngine::lastError() const
{
    return m_impl->error;
}

std::string OnnxRuntimeEngine::backendId() const
{
    return "onnxruntime-cpu";
}

DeviceInfo OnnxRuntimeEngine::device() const
{
    DeviceInfo info;
    info.name = "CPU";
    info.deviceId = 0;
    info.gpu = false;
    return info;
}

TensorMetadata OnnxRuntimeEngine::inputMetadata() const
{
    return {};
}

std::vector<TensorMetadata> OnnxRuntimeEngine::outputMetadata() const
{
    return {};
}

InferResult OnnxRuntimeEngine::infer(const TensorView&)
{
    InferResult result;
    result.ok = false;
    result.error = m_impl->error;
    return result;
}

void OnnxRuntimeEngine::warmup(int)
{
}

} // namespace visionlab
