#include <QtTest/QtTest>

#include <utility>

#include <cuda_runtime_api.h>

#include <array>
#include <cstring>

#include "inference/cuda/CudaDeviceBuffer.h"
#include "inference/cuda/CudaStream.h"

using visionlab::CudaDeviceBuffer;
using visionlab::CudaStream;

#define SKIP_IF_NO_GPU()                                                       \
    do                                                                         \
    {                                                                          \
        int count = 0;                                                         \
        if (cudaGetDeviceCount(&count) != cudaSuccess || count < 1)            \
            QSKIP("no CUDA device");                                           \
    } while (false)

class CudaRaiiTest : public QObject
{
    Q_OBJECT

private slots:
    void streamIsValidWhenGpuPresent();
    void bufferHostDeviceRoundTrip();
    void movedFromStreamIsEmpty();
};

void CudaRaiiTest::streamIsValidWhenGpuPresent()
{
    SKIP_IF_NO_GPU();

    CudaStream stream;
    QVERIFY2(stream.valid(), stream.lastError().c_str());
    QVERIFY(stream.get() != nullptr);
}

void CudaRaiiTest::bufferHostDeviceRoundTrip()
{
    SKIP_IF_NO_GPU();

    CudaStream stream;
    QVERIFY2(stream.valid(), stream.lastError().c_str());

    CudaDeviceBuffer buffer(16);
    QVERIFY2(buffer.valid(), buffer.lastError().c_str());
    QCOMPARE(buffer.bytes(), std::size_t{16});

    std::array<unsigned char, 16> hostIn{};
    std::array<unsigned char, 16> hostOut{};
    for (unsigned i = 0; i < hostIn.size(); ++i)
        hostIn[i] = static_cast<unsigned char>(i + 1);

    QVERIFY2(buffer.copyFromHost(hostIn.data(), hostIn.size(), stream.get()),
             buffer.lastError().c_str());
    QVERIFY2(buffer.copyToHost(hostOut.data(), hostOut.size(), stream.get()),
             buffer.lastError().c_str());
    QCOMPARE(cudaStreamSynchronize(stream.get()), cudaSuccess);
    QVERIFY(std::memcmp(hostIn.data(), hostOut.data(), hostIn.size()) == 0);

    CudaDeviceBuffer again(16);
    QVERIFY2(again.valid(), again.lastError().c_str());
}

void CudaRaiiTest::movedFromStreamIsEmpty()
{
    SKIP_IF_NO_GPU();

    CudaStream stream;
    QVERIFY(stream.valid());
    CudaStream moved = std::move(stream);
    QVERIFY(moved.valid());
    QVERIFY(!stream.valid());
}

QTEST_APPLESS_MAIN(CudaRaiiTest)

#include "CudaRaiiTest.moc"
