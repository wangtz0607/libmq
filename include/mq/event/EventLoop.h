#pragma once

#include <chrono>
#include <format>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mq {

class Watcher;

class EventLoop {
public:
    enum class State {
        kIdle,
        kCallback,
        kTask,
        kTimedTask,
    };

    using Task = std::move_only_function<void ()>;
    using TimedTask = std::move_only_function<std::chrono::nanoseconds ()>;

    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop &) = delete;
    EventLoop(EventLoop &&) = delete;

    EventLoop &operator=(const EventLoop &) = delete;
    EventLoop &operator=(EventLoop &&) = delete;

    bool isInLoopThread() const {
        return loop_ == this;
    }

    State state() const;
    void post(Task task);
    void postAndWait(Task task);
    void postTimed(TimedTask task, std::chrono::nanoseconds delay);
    [[noreturn]] void run();

    static EventLoop *background();

private:
    static thread_local EventLoop *loop_;

    State state_ = State::kIdle;
    int epollFd_;
    int eventFd_;
    std::unordered_map<int, Watcher *> watchers_;
    std::mutex watchersMutex_;
    std::vector<Task> tasks_;
    std::mutex tasksMutex_;
    std::unordered_map<int, TimedTask> timedTasks_;
    std::mutex timedTasksMutex_;

    bool hasWatcher(int fd);
    void addWatcher(Watcher *watcher);
    void updateWatcher(Watcher *watcher);
    void updateWatcherIfRegistered(Watcher *watcher);
    void removeWatcher(Watcher *watcher);
    void wakeUp();

    friend class Watcher;
};

} // namespace mq

template <>
struct std::formatter<mq::EventLoop::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::EventLoop::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::EventLoop::State state) {
        using enum mq::EventLoop::State;
        switch (state) {
            case kIdle: return "IDLE";
            case kCallback: return "CALLBACK";
            case kTask: return "TASK";
            case kTimedTask: return "TIMED_TASK";
            default: return nullptr;
        }
    }
};
