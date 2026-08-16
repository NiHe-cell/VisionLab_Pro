#include "VisionPipeline.h"

namespace visionlab {

VisionPipeline::VisionPipeline(std::unique_ptr<IVideoSource> source,
                               std::map<DetectionMode, std::unique_ptr<IDetector>> detectors,
                               std::size_t queueCapacity)
    : m_source(std::move(source))
    , m_detectors(std::move(detectors))
    , m_queueCapacity(queueCapacity)
{
}

VisionPipeline::~VisionPipeline()
{
    stop();
}

bool VisionPipeline::start()
{
    std::lock_guard lock(m_lifecycle);
    if (m_running)
        return true;
    if (!m_source || !m_source->open())
        return false;

    m_latest.clear();
    m_stats.reset();
    m_queue = std::make_unique<BoundedQueue<FramePacket>>(
        m_queueCapacity, OverflowPolicy::DropOldest);
    m_captureWorker = std::make_unique<CaptureWorker>(*m_source, *m_queue, m_stats);
    m_inferenceWorker = std::make_unique<InferenceWorker>(
        *m_queue, m_latest, [this] { return currentDetector(); }, m_stats);

    m_captureThread = std::jthread([this](std::stop_token stop) {
        m_captureWorker->run(stop);
    });
    m_inferenceThread = std::jthread([this](std::stop_token stop) {
        m_inferenceWorker->run(stop);
    });
    m_running = true;
    return true;
}

void VisionPipeline::stop()
{
    std::lock_guard lock(m_lifecycle);
    joinWorkers();
}

bool VisionPipeline::isRunning() const
{
    return m_running.load();
}

void VisionPipeline::setMode(DetectionMode mode)
{
    m_mode.store(mode);
}

DetectionMode VisionPipeline::mode() const
{
    return m_mode.load();
}

std::optional<PresentedFrame> VisionPipeline::latest() const
{
    return m_latest.snapshot();
}

PipelineStats VisionPipeline::stats() const
{
    return m_stats.snapshot();
}

IDetector* VisionPipeline::currentDetector()
{
    const auto it = m_detectors.find(m_mode.load());
    if (it == m_detectors.end())
        return nullptr;
    return it->second.get();
}

void VisionPipeline::joinWorkers()
{
    m_running = false;
    if (m_captureThread.joinable())
        m_captureThread.request_stop();
    if (m_inferenceThread.joinable())
        m_inferenceThread.request_stop();
    if (m_source)
        m_source->close();
    if (m_queue)
        m_queue->close();
    if (m_captureThread.joinable())
        m_captureThread.join();
    if (m_inferenceThread.joinable())
        m_inferenceThread.join();
    m_captureWorker.reset();
    m_inferenceWorker.reset();
    m_queue.reset();
}

} // namespace visionlab
