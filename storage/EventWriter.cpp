#include "storage/EventWriter.h"

#include <QDateTime>
#include <QMetaObject>

namespace visionlab {

EventWriter::EventWriter(std::unique_ptr<IEventRepository> repo, std::size_t queueCapacity)
    : m_repo(std::move(repo))
    , m_capacity(queueCapacity == 0 ? 1024 : queueCapacity)
{
}

EventWriter::~EventWriter()
{
    stop();
}

bool EventWriter::start(const std::filesystem::path& dbPath)
{
    stop();
    if (!m_repo || !m_repo->open(dbPath))
        return false;
    m_queue = std::make_unique<BoundedQueue<Command>>(m_capacity, OverflowPolicy::DropOldest);
    m_thread = std::jthread([this](std::stop_token) { runLoop(); });
    m_running = true;
    return true;
}

void EventWriter::enqueue(VisionEvent event)
{
    if (!m_running || !m_queue)
        return;
    m_queue->push(Command{std::move(event)});
}

void EventWriter::requestQuery(EventQuery query,
                              QObject* receiver,
                              std::function<void(std::vector<StoredEvent>)> onResult)
{
    if (!m_running || !m_queue)
        return;
    QueryJob job;
    job.query = query;
    job.receiver = receiver;
    job.onResult = std::move(onResult);
    m_queue->push(Command{std::move(job)});
}

void EventWriter::stop()
{
    if (m_queue)
        m_queue->close();
    if (m_thread.joinable())
        m_thread.join();
    m_thread = {};
    m_queue.reset();
    if (m_repo)
        m_repo->close();
    m_running = false;
}

bool EventWriter::isRunning() const
{
    return m_running;
}

void EventWriter::runLoop()
{
    Command command;
    while (m_queue && m_queue->pop(command))
        handle(command);
}

void EventWriter::handle(Command& command)
{
    if (!m_repo)
        return;
    if (auto* event = std::get_if<VisionEvent>(&command))
    {
        m_repo->insert(*event, QDateTime::currentMSecsSinceEpoch());
        m_repo->prune(10000);
        return;
    }
    auto* job = std::get_if<QueryJob>(&command);
    if (!job || !job->onResult)
        return;
    std::vector<StoredEvent> rows = m_repo->query(job->query);
    if (!job->receiver)
        return;
    QMetaObject::invokeMethod(
        job->receiver,
        [cb = std::move(job->onResult), rows = std::move(rows)]() { cb(rows); },
        Qt::QueuedConnection);
}

} // namespace visionlab
