#ifndef DUMMYDETECTOR_H
#define DUMMYDETECTOR_H

#include <string>
#include <vector>

#include "IDetector.h"

namespace visionlab {

// 确定性假检测器：不依赖模型，始终就绪。
// mode() 为 None，不占用 Face / Object / Motion，也不进入生产管线 map。
// 实现不得在帧上绘制。
class DummyDetector : public IDetector
{
public:
    std::string name() const override { return "Dummy"; }
    DetectionMode mode() const override { return DetectionMode::None; }
    bool isReady() const override { return true; }

    std::vector<Detection> detect(const FramePacket& frame) override;
};

} // namespace visionlab

#endif // DUMMYDETECTOR_H
