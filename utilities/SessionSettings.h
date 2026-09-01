#ifndef SESSIONSETTINGS_H
#define SESSIONSETTINGS_H

#include "inference/InferenceSelection.h"

namespace visionlab {

// 本会话有效的推理与跟踪设置。不写回环境变量，不写 QSettings。
struct SessionSettings
{
    InferenceSelection inference;
    float confidenceThreshold = 0.25F;
    float nmsThreshold = 0.45F;
    bool trackingEnabled = true;
};

} // namespace visionlab

#endif // SESSIONSETTINGS_H
