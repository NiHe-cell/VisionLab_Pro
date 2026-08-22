#include "CudaDeviceBuffer.h"

#include <utility>

namespace visionlab {

CudaDeviceBuffer::CudaDeviceBuffer(std::size_t bytes)
    : m_bytes(bytes)
{
    if (bytes == 0)
    {
        m_error = "CudaDeviceBuffer size must be > 0";
        return;
    }

    const cudaError_t err = cudaMalloc(&m_device, bytes);
    if (err != cudaSuccess)
    {
        m_device = nullptr;
        m_bytes = 0;
        m_error = cudaGetErrorString(err);
    }
}

CudaDeviceBuffer::~CudaDeviceBuffer()
{
    destroy();
}

CudaDeviceBuffer::CudaDeviceBuffer(CudaDeviceBuffer&& other) noexcept
    : m_device(other.m_device)
    , m_bytes(other.m_bytes)
    , m_error(std::move(other.m_error))
{
    other.m_device = nullptr;
    other.m_bytes = 0;
}

CudaDeviceBuffer& CudaDeviceBuffer::operator=(CudaDeviceBuffer&& other) noexcept
{
    if (this == &other)
        return *this;
    destroy();
    m_device = other.m_device;
    m_bytes = other.m_bytes;
    m_error = std::move(other.m_error);
    other.m_device = nullptr;
    other.m_bytes = 0;
    return *this;
}

bool CudaDeviceBuffer::copyFromHost(const void* src, std::size_t bytes, cudaStream_t stream)
{
    if (!checkSize(bytes) || src == nullptr)
    {
        if (src == nullptr)
            m_error = "copyFromHost src is null";
        return false;
    }

    const cudaError_t err =
        cudaMemcpyAsync(m_device, src, bytes, cudaMemcpyHostToDevice, stream);
    if (err != cudaSuccess)
    {
        m_error = cudaGetErrorString(err);
        return false;
    }
    m_error.clear();
    return true;
}

bool CudaDeviceBuffer::copyToHost(void* dst, std::size_t bytes, cudaStream_t stream)
{
    if (!checkSize(bytes) || dst == nullptr)
    {
        if (dst == nullptr)
            m_error = "copyToHost dst is null";
        return false;
    }

    const cudaError_t err =
        cudaMemcpyAsync(dst, m_device, bytes, cudaMemcpyDeviceToHost, stream);
    if (err != cudaSuccess)
    {
        m_error = cudaGetErrorString(err);
        return false;
    }
    m_error.clear();
    return true;
}

void CudaDeviceBuffer::destroy() noexcept
{
    if (m_device == nullptr)
        return;
    cudaFree(m_device);
    m_device = nullptr;
    m_bytes = 0;
}

bool CudaDeviceBuffer::checkSize(std::size_t bytes)
{
    if (!valid())
    {
        if (m_error.empty())
            m_error = "CudaDeviceBuffer is not valid";
        return false;
    }
    if (bytes != m_bytes)
    {
        m_error = "copy size does not match buffer";
        return false;
    }
    return true;
}

} // namespace visionlab
