#ifndef VISIONTYPES_H
#define VISIONTYPES_H

#include <cstdint>

namespace visionlab {

// Detection algorithm selection for the processing pipeline.
// Replaces the legacy string-based mode switching ("Face Detection", ...).
// Values are stable; map to/from UI strings only at the application boundary.
enum class DetectionMode : std::uint8_t
{
    None,
    Face,
    Object,
    Motion,
};

// Reserved class id for MotionDetector regions, which are not classified
// objects. Kept outside the COCO class range on purpose.
inline constexpr int kMotionClassId = -1;

} // namespace visionlab

#endif // VISIONTYPES_H
