#ifndef FRAMEPACKET_H
#define FRAMEPACKET_H

#include <chrono>
#include <cstdint>
#include <string>

#include <opencv2/core.hpp>

namespace visionlab {

// 在管线中流转的一帧图像。
//
// cv::Mat 所有权语义：
//   image 按值持有。cv::Mat 是引用计数句柄，拷贝 FramePacket 开销很小，
//   拷贝体与原件共享同一块像素缓冲。因此管线各阶段之间只以 const 引用
//   或移动（按值）方式传递 FramePacket。
//
//   由于拷贝体会别名同一缓冲，消费方必须将 image 视为只读；
//   任何需要在像素上标注的阶段（如叠加渲染）必须先 clone() 再写。
struct FramePacket
{
    // 由采集阶段分配的单调递增帧 id。
    std::int64_t frameId = 0;

    // 从视频源读出该帧后立即记录的时间戳。
    std::chrono::steady_clock::time_point captureTimestamp{};

    // 来源视频源标识（如 "camera:0"）。
    std::string sourceId;

    // 像素数据；引用计数共享，见上方所有权说明。
    cv::Mat image;
};

} // namespace visionlab

#endif // FRAMEPACKET_H
