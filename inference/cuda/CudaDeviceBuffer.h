#ifndef CUDADEVICEBUFFER_H
#define CUDADEVICEBUFFER_H

#include <cstddef>
#include <string>

#include <cuda_runtime_api.h>

namespace visionlab {

// 设备内存的 RAII 包装。TensorRT 引擎不得在别处调用 cudaMalloc/cudaFree。
class CudaDeviceBuffer
{
public:
    explicit CudaDeviceBuffer(std::size_t bytes);
    ~CudaDeviceBuffer();

    CudaDeviceBuffer(CudaDeviceBuffer&& other) noexcept;
    CudaDeviceBuffer& operator=(CudaDeviceBuffer&& other) noexcept;

    CudaDeviceBuffer(const CudaDeviceBuffer&) = delete;
    CudaDeviceBuffer& operator=(const CudaDeviceBuffer&) = delete;

    void* data() const { return m_device; }
    std::size_t bytes() const { return m_bytes; }
    bool valid() const { return m_device != nullptr; }
    const std::string& lastError() const { return m_error; }

    bool copyFromHost(const void* src, std::size_t bytes, cudaStream_t stream);
    bool copyToHost(void* dst, std::size_t bytes, cudaStream_t stream);

private:
    void destroy() noexcept;
    bool checkSize(std::size_t bytes);

    void* m_device = nullptr;
    std::size_t m_bytes = 0;
    std::string m_error;
};

} // namespace visionlab

#endif // CUDADEVICEBUFFER_H
