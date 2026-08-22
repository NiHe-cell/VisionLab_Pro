#include "CudaStream.h"

#include <utility>

namespace visionlab {

CudaStream::CudaStream()
{
    const cudaError_t err = cudaStreamCreate(&m_stream);
    if (err != cudaSuccess)
    {
        m_stream = nullptr;
        m_error = cudaGetErrorString(err);
    }
}

CudaStream::~CudaStream()
{
    destroy();
}

CudaStream::CudaStream(CudaStream&& other) noexcept
    : m_stream(other.m_stream)
    , m_error(std::move(other.m_error))
{
    other.m_stream = nullptr;
}

CudaStream& CudaStream::operator=(CudaStream&& other) noexcept
{
    if (this == &other)
        return *this;
    destroy();
    m_stream = other.m_stream;
    m_error = std::move(other.m_error);
    other.m_stream = nullptr;
    return *this;
}

void CudaStream::destroy() noexcept
{
    if (m_stream == nullptr)
        return;
    cudaStreamDestroy(m_stream);
    m_stream = nullptr;
}

} // namespace visionlab
