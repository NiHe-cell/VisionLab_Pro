#include "core/LatencyWindow.h"
#include "core/PipelineStats.h"
#include "core/VisionTypes.h"
#include "detectors/IDetector.h"
#include "fakes/FakeDetector.h"
#include "fakes/FakeVideoSource.h"
#include "fakes/SlowDetector.h"
#include "pipeline/VisionPipeline.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>

namespace {

struct Options
{
    int seconds = 8;
    int queue = 2;
    int detectMs = 50;
    int width = 64;
    int height = 64;
};

void printUsage(std::ostream& out)
{
    out << "Usage: bench_pipeline [--seconds N] [--queue N] [--detect-ms M]\n"
           "                      [--width W] [--height H]\n"
           "Fake source + SlowDetector (or FakeDetector when --detect-ms 0).\n"
           "Does not open a camera or load YOLO. Prints field names only;\n"
           "do not treat the numbers as a pass/fail gate.\n"
           "Soak locally: --seconds 3600 --detect-ms 0 (not for CI).\n";
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
        if (arg == "--seconds")
        {
            const char* value = requireValue("--seconds");
            if (value == nullptr)
                return false;
            if (!parseInt(value, options.seconds) || options.seconds < 1)
            {
                error = "invalid --seconds";
                return false;
            }
            continue;
        }
        if (arg == "--queue")
        {
            const char* value = requireValue("--queue");
            if (value == nullptr)
                return false;
            if (!parseInt(value, options.queue) || options.queue < 1)
            {
                error = "invalid --queue";
                return false;
            }
            continue;
        }
        if (arg == "--detect-ms")
        {
            const char* value = requireValue("--detect-ms");
            if (value == nullptr)
                return false;
            if (!parseInt(value, options.detectMs) || options.detectMs < 0)
            {
                error = "invalid --detect-ms";
                return false;
            }
            continue;
        }
        if (arg == "--width")
        {
            const char* value = requireValue("--width");
            if (value == nullptr)
                return false;
            if (!parseInt(value, options.width) || options.width < 1)
            {
                error = "invalid --width";
                return false;
            }
            continue;
        }
        if (arg == "--height")
        {
            const char* value = requireValue("--height");
            if (value == nullptr)
                return false;
            if (!parseInt(value, options.height) || options.height < 1)
            {
                error = "invalid --height";
                return false;
            }
            continue;
        }
        error = "unknown argument: " + arg;
        return false;
    }
    return true;
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

    auto source = std::make_unique<FakeVideoSource>(
        32, "fake:pipeline-bench", true, true, options.width, options.height);

    std::map<visionlab::DetectionMode, std::unique_ptr<visionlab::IDetector>> detectors;
    if (options.detectMs == 0)
    {
        detectors.emplace(visionlab::DetectionMode::Object,
                          std::make_unique<FakeDetector>("bench"));
    }
    else
    {
        detectors.emplace(
            visionlab::DetectionMode::Object,
            std::make_unique<SlowDetector>(std::chrono::milliseconds(options.detectMs)));
    }

    visionlab::VisionPipeline pipeline(
        std::move(source), std::move(detectors),
        static_cast<std::size_t>(options.queue));
    pipeline.setMode(visionlab::DetectionMode::Object);
    if (!pipeline.start())
    {
        std::cerr << "pipeline start failed\n";
        return 1;
    }

    visionlab::LatencyWindow e2e(256);
    std::size_t peakQueueDepth = 0;
    const auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::seconds(options.seconds);
    while (std::chrono::steady_clock::now() < deadline)
    {
        const visionlab::PipelineStats snap = pipeline.stats();
        peakQueueDepth = std::max(peakQueueDepth, snap.captureQueueDepth);
        e2e.record(snap.endToEndLatencyMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    const int stillRunningBeforeStop = pipeline.isRunning() ? 1 : 0;
    const visionlab::PipelineStats stats = pipeline.stats();
    pipeline.stop();

    std::cout << "backend: fake-pipeline\n";
    std::cout << "seconds: " << options.seconds << '\n';
    std::cout << "queue_capacity: " << options.queue << '\n';
    std::cout << "detect_ms: " << options.detectMs << '\n';
    std::cout << "captured: " << stats.capturedFrames << '\n';
    std::cout << "processed: " << stats.processedFrames << '\n';
    std::cout << "dropped: " << stats.droppedFrames << '\n';
    std::cout << "peak_queue_depth: " << peakQueueDepth << '\n';
    std::cout << "e2e_p50_ms: " << e2e.percentile(50.0) << '\n';
    std::cout << "e2e_p95_ms: " << e2e.percentile(95.0) << '\n';
    std::cout << "still_running_before_stop: " << stillRunningBeforeStop << '\n';
    return 0;
}
