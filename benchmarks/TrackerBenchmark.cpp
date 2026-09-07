#include "core/Detection.h"
#include "core/LatencyWindow.h"
#include "core/Track.h"
#include "tracking/ByteTrackTracker.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Options
{
    int frames = 200;
    int dets = 10;
    int warmup = 10;
};

void printUsage(std::ostream& out)
{
    out << "Usage: bench_tracker [--frames N] [--dets K] [--warmup N]\n"
           "Synthetic detections through ByteTrackTracker::update.\n"
           "Does not read images or run inference. Numbers are not a ctest gate.\n";
}

bool parseInt(const char* text, int& value)
{
    if (text == nullptr || *text == '\0')
        return false;
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (end == text || *end != '\0')
        return false;
    if (parsed < -1'000'000 || parsed > 1'000'000)
        return false;
    value = static_cast<int>(parsed);
    return true;
}

bool parseArgs(int argc, char** argv, Options& options, std::string& error)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        const auto requireValue = [&](const char* name) -> const char* {
            if (i + 1 >= argc)
            {
                error = std::string(name) + " requires a value";
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h")
        {
            printUsage(std::cout);
            std::exit(0);
        }
        if (arg == "--frames")
        {
            const char* value = requireValue("--frames");
            if (value == nullptr)
                return false;
            if (!parseInt(value, options.frames) || options.frames < 1)
            {
                error = "invalid --frames";
                return false;
            }
            continue;
        }
        if (arg == "--dets")
        {
            const char* value = requireValue("--dets");
            if (value == nullptr)
                return false;
            if (!parseInt(value, options.dets) || options.dets < 1)
            {
                error = "invalid --dets";
                return false;
            }
            continue;
        }
        if (arg == "--warmup")
        {
            const char* value = requireValue("--warmup");
            if (value == nullptr)
                return false;
            if (!parseInt(value, options.warmup) || options.warmup < 0)
            {
                error = "invalid --warmup";
                return false;
            }
            continue;
        }
        error = "unknown argument: " + arg;
        return false;
    }
    return true;
}

visionlab::Detection makeDet(int index, int frame, int total)
{
    visionlab::Detection detection;
    detection.classId = 0;
    detection.label = "person";
    detection.confidence = 0.9F;
    const int stride = 640 / (total + 1);
    const int x = stride * (index + 1) + (frame % 20);
    const int y = 80 + (index % 3) * 40;
    detection.box = cv::Rect(x, y, 40, 80);
    return detection;
}

std::vector<visionlab::Detection> makeFrameDets(int frame, int count)
{
    std::vector<visionlab::Detection> detections;
    detections.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
        detections.push_back(makeDet(i, frame, count));
    return detections;
}

visionlab::TrackUpdateContext makeContext(int frame)
{
    visionlab::TrackUpdateContext context;
    context.frameId = frame;
    context.timestamp = std::chrono::steady_clock::time_point{}
                        + std::chrono::milliseconds(33 * frame);
    return context;
}

} // namespace

int main(int argc, char** argv)
{
    Options options;
    std::string error;
    if (!parseArgs(argc, argv, options, error))
    {
        std::cerr << error << '\n';
        printUsage(std::cerr);
        return 2;
    }

    visionlab::ByteTrackTracker tracker;
    int frame = 1;
    for (int i = 0; i < options.warmup; ++i)
    {
        tracker.update(makeFrameDets(frame, options.dets), makeContext(frame));
        ++frame;
    }

    visionlab::LatencyWindow window(
        static_cast<std::size_t>(std::max(options.frames, 1)));
    for (int i = 0; i < options.frames; ++i)
    {
        const auto start = std::chrono::steady_clock::now();
        tracker.update(makeFrameDets(frame, options.dets), makeContext(frame));
        const auto elapsed = std::chrono::steady_clock::now() - start;
        window.record(std::chrono::duration<double, std::milli>(elapsed).count());
        ++frame;
    }

    std::cout << "backend: bytetrack\n";
    std::cout << "frames: " << options.frames << '\n';
    std::cout << "dets: " << options.dets << '\n';
    std::cout << "warmup: " << options.warmup << '\n';
    std::cout << "mean_ms: " << window.mean() << '\n';
    std::cout << "p50_ms: " << window.percentile(50.0) << '\n';
    std::cout << "p95_ms: " << window.percentile(95.0) << '\n';
    std::cout << "active_tracks: " << tracker.stats().activeTracks << '\n';
    return 0;
}
