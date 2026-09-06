#ifndef EVENTWRITER_H
#define EVENTWRITER_H

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <thread>
#include <variant>
#include <vector>

#include <QObject>
#include <QPointer>

#include "core/BoundedQueue.h"
#include "core/VisionEvent.h"
#include "storage/EventQuery.h"
#include "storage/IEventRepository.h"

namespace visionlab {

// 后台 jthread 写 SQLite。enqueue 在 GUI 线程，不阻塞 SQL。
class EventWriter
{
public:
    explicit EventWriter(std::unique_ptr<IEventRepository> repo,
                         std::size_t queueCapacity = 1024);
    ~EventWriter();

    EventWriter(const EventWriter&) = delete;
    EventWriter& operator=(const EventWriter&) = delete;

    bool start(const std::filesystem::path& dbPath);
    void enqueue(VisionEvent event);
    void requestQuery(EventQuery query,
                      QObject* receiver,
                      std::function<void(std::vector<StoredEvent>)> onResult);
    void stop();
    bool isRunning() const;

private:
    struct QueryJob
    {
        EventQuery query;
        QPointer<QObject> receiver;
        std::function<void(std::vector<StoredEvent>)> onResult;
    };

    using Command = std::variant<VisionEvent, QueryJob>;

    void runLoop();
    void handle(Command& command);

    std::unique_ptr<IEventRepository> m_repo;
    const std::size_t m_capacity;
    std::unique_ptr<BoundedQueue<Command>> m_queue;
    std::jthread m_thread;
    bool m_running = false;
};

} // namespace visionlab

#endif // EVENTWRITER_H
