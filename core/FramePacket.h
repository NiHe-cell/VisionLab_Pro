#ifndef FRAMEPACKET_H
#define FRAMEPACKET_H

#include <chrono>
#include <cstdint>
#include <string>

#include <opencv2/core.hpp>

namespace visionlab {

// A single captured frame travelling through the pipeline.
//
// cv::Mat ownership semantics:
//   `image` is held BY VALUE. cv::Mat is a reference-counted handle, so
//   copying a FramePacket is cheap and shares the pixel buffer with the
//   original. FramePackets are therefore passed between pipeline stages
//   either as const references or moved by value.
//
//   Because copies alias the same buffer, consumers must treat `image` as
//   read-only. Any stage that needs to annotate pixels (e.g. overlay
//   rendering) must clone() before writing.
struct FramePacket
{
    // Monotonically increasing id assigned by the capture stage.
    std::int64_t frameId = 0;

    // Timestamp taken immediately after the frame was read from the source.
    std::chrono::steady_clock::time_point captureTimestamp{};

    // Identity of the originating video source (e.g. "camera:0").
    std::string sourceId;

    // Pixel data; reference-counted, see ownership note above.
    cv::Mat image;
};

} // namespace visionlab

#endif // FRAMEPACKET_H
