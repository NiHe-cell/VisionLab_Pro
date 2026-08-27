#include "TrackRenderer.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <opencv2/imgproc.hpp>

#include "core/VisionTypes.h"

namespace visionlab {

namespace {

constexpr int kBoxThickness = 2;
constexpr double kFontScale = 0.6;
constexpr int kTextThickness = 2;
constexpr int kLabelOffset = 5;

const cv::Scalar kObjectColor{0, 255, 0};
const cv::Scalar kMotionColor{0, 255, 255};

cv::Point roundToPixel(const cv::Point2f& centroid)
{
    return {static_cast<int>(std::lround(centroid.x)),
            static_cast<int>(std::lround(centroid.y))};
}

bool inFrame(const cv::Mat& frame, const cv::Point& pixel)
{
    return pixel.x >= 0 && pixel.y >= 0 && pixel.x < frame.cols && pixel.y < frame.rows;
}

} // namespace

cv::Scalar TrackRenderer::colorFor(const Track& track)
{
    return track.classId == kMotionClassId ? kMotionColor : kObjectColor;
}

void TrackRenderer::render(cv::Mat& frame, const std::vector<Track>& tracks) const
{
    if (frame.empty())
        return;

    for (const Track& track : tracks)
    {
        const cv::Scalar color = colorFor(track);

        cv::rectangle(frame, track.box, color, kBoxThickness);

        const int labelY = track.box.y - kLabelOffset >= kLabelOffset
                               ? track.box.y - kLabelOffset
                               : track.box.y + kLabelOffset + 10;
        const cv::Point origin{std::max(track.box.x, 0), labelY};
        const std::string caption = track.label + " #" + std::to_string(track.trackId);
        cv::putText(frame, caption, origin, cv::FONT_HERSHEY_SIMPLEX, kFontScale, color,
                    kTextThickness);

        if (track.trajectory.size() < 2)
            continue;

        auto it = track.trajectory.begin();
        cv::Point previous = roundToPixel(it->centroid);
        ++it;
        for (; it != track.trajectory.end(); ++it)
        {
            const cv::Point current = roundToPixel(it->centroid);
            if (inFrame(frame, previous) && inFrame(frame, current))
                cv::line(frame, previous, current, color, kBoxThickness);
            previous = current;
        }
    }
}

} // namespace visionlab
