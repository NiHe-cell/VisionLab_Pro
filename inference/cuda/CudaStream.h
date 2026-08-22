#ifndef CUDASTREAM_H
#define CUDASTREAM_H

#include <string>

#include <cuda_runtime_api.h>

namespace visionlab {

// 单条 CUDA stream 的 RAII 包装。move-only，禁止拷贝。
class CudaStream
{
public:
    CudaStream();
    ~CudaStream();

    CudaStream(CudaStream&& other) noexcept;
    CudaStream& operator=(CudaStream&& other) noexcept;

    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;

    cudaStream_t get() const { return m_stream; }
    bool valid() const { return m_stream != nullptr; }
    const std::string& lastError() const { return m_error; }

private:
    void destroy() noexcept;

    cudaStream_t m_stream = nullptr;
    std::string m_error;
};

} // namespace visionlab

#endif // CUDASTREAM_H
