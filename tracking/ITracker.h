#ifndef ITRACKER_H
#define ITRACKER_H

#include <string>
#include <vector>

#include "core/Detection.h"
#include "core/Track.h"

namespace visionlab {

// 多目标跟踪扩展点：一帧 Detection 列表进，仍存活的 Track 列表出。
//
// 约定：
// - 实现不得读取或写入像素；只使用 Detection 框与 TrackUpdateContext。
// - update 不得修改入参 detections。
// - 返回 Confirmed 与仍存活的 Lost；不返回 Tentative、不返回 Removed。
// - 同一实例的 update / reset 不承诺可并发调用；只应由推理线程驱动。
class ITracker
{
public:
    virtual ~ITracker() = default;

    virtual std::string name() const = 0;

    virtual std::vector<Track> update(
        const std::vector<Detection>& detections,
        const TrackUpdateContext& context) = 0;

    virtual void reset() = 0;

    virtual TrackerStats stats() const = 0;

    virtual TrackerConfig config() const = 0;
};

} // namespace visionlab

#endif // ITRACKER_H
