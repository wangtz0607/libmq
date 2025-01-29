#include "mq/event/EventLoop.h"

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <future>
#include <iterator>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "mq/event/EventLoop.h"
#include "mq/event/Watcher.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "EventLoop"

using namespace mq;

thread_local EventLoop *EventLoop::loop_ = nullptr;

EventLoop::EventLoop() {
    LOG(debug, "");

    CHECK(loop_ == nullptr);

    CHECK((epollFd_ = epoll_create1(0)) >= 0);
    CHECK((eventFd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) >= 0);

    LOG(debug, "epollFd={}, eventFd={}", epollFd_, eventFd_);

    struct epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = eventFd_;
    CHECK(epoll_ctl(epollFd_, EPOLL_CTL_ADD, eventFd_, &event) == 0);

    loop_ = this;
}

EventLoop::~EventLoop() {
    LOG(debug, "");

    CHECK(isInLoopThread());

    CHECK(close(eventFd_) == 0);
    CHECK(close(epollFd_) == 0);

    loop_ = nullptr;
}

EventLoop::State EventLoop::state() const {
    CHECK(isInLoopThread());

    return state_;
}

void EventLoop::post(Task task) {
    LOG(debug, "");

    {
        std::unique_lock lock(tasksMutex_);
        tasks_.emplace_back(std::move(task));
    }

    wakeUp();
}

void EventLoop::postAndWait(Task task) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    post([task = std::move(task), promise = std::move(promise)] mutable {
        task();
        promise.set_value();
    });

    future.get();
}

void EventLoop::postTimed(TimedTask task, std::chrono::nanoseconds delay) {
    LOG(debug, "delay={}", delay);

    CHECK(delay.count() > 0);

    int timerFd;
    CHECK((timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)) >= 0);

    LOG(debug, "timerFd={}", timerFd);

    struct itimerspec newValue{};
    newValue.it_value.tv_sec = delay.count() / 1'000'000'000;
    newValue.it_value.tv_nsec = delay.count() % 1'000'000'000;

    CHECK(timerfd_settime(timerFd, 0, &newValue, nullptr) == 0);

    {
        std::unique_lock lock(timedTasksMutex_);
        timedTasks_.emplace(timerFd, std::move(task));
    }

    struct epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = timerFd;
    CHECK(epoll_ctl(epollFd_, EPOLL_CTL_ADD, timerFd, &event) == 0);

    wakeUp();
}

void EventLoop::run() {
    LOG(debug, "");

    CHECK(isInLoopThread());

    constexpr size_t kMaxEvents = 256;
    struct epoll_event events[kMaxEvents];

    for (;;) {
        int n = epoll_wait(epollFd_, events, kMaxEvents, -1);
        LOG(debug, "epoll_wait: n={}", n);

        if (n < 0) {
            LOG(debug, "epoll_wait: errno={}", strerrorname_np(errno));
            CHECK(errno == EINTR);
            continue;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t eventsMask = events[i].events;

            LOG(debug, "fd={}, eventsMask={}", fd, eventsMask);

            if (fd == eventFd_) {
                LOG(debug, "eventFd");

                uint64_t value;
                CHECK(read(eventFd_, &value, sizeof(value)) == sizeof(value));
            } else {
                TimedTask timedTask;
                {
                    std::unique_lock lock(timedTasksMutex_);
                    if (auto j = timedTasks_.find(fd); j != timedTasks_.end()) {
                        timedTask = std::move(j->second);
                        timedTasks_.erase(j);
                    }
                }

                if (timedTask) {
                    LOG(debug, "fd={}, timedTask", fd);

                    uint64_t value;
                    CHECK(read(fd, &value, sizeof(value)) == sizeof(value));

                    state_ = State::kTimedTask;
                    std::chrono::nanoseconds delay = timedTask();
                    state_ = State::kIdle;

                    if (delay.count() == 0) {
                        CHECK(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == 0);
                        CHECK(close(fd) == 0);
                    } else {
                        struct itimerspec newValue{};
                        newValue.it_value.tv_sec = delay.count() / 1'000'000'000;
                        newValue.it_value.tv_nsec = delay.count() % 1'000'000'000;

                        CHECK(timerfd_settime(fd, 0, &newValue, nullptr) == 0);

                        {
                            std::unique_lock lock(timedTasksMutex_);
                            timedTasks_.emplace(fd, std::move(timedTask));
                        }
                    }
                } else {
                    Watcher *watcher;
                    {
                        std::unique_lock lock(watchersMutex_);
                        watcher = watchers_.find(fd)->second;
                    }

                    if (eventsMask & EPOLLIN) {
                        LOG(debug, "fd={}, EPOLLIN", fd);

                        state_ = State::kCallback;
                        watcher->dispatchRead();
                        state_ = State::kIdle;

                        if (!watcher->hasReadCallback()) {
                            updateWatcher(watcher);
                        }
                    }

                    if (eventsMask & EPOLLOUT) {
                        LOG(debug, "fd={}, EPOLLOUT", fd);

                        state_ = State::kCallback;
                        watcher->dispatchWrite();
                        state_ = State::kIdle;

                        if (!watcher->hasWriteCallback()) {
                            updateWatcher(watcher);
                        }
                    }
                }
            }
        }

        state_ = State::kTask;

        {
            std::vector<Task> tasks;
            {
                std::unique_lock lock(tasksMutex_);

                constexpr size_t kMaxTasks = 256;

                if (tasks_.size() <= kMaxTasks) {
                    tasks.swap(tasks_);
                } else {
                    tasks.insert(tasks.end(),
                                 std::make_move_iterator(tasks_.begin()),
                                 std::make_move_iterator(tasks_.begin() + kMaxTasks));

                    tasks_.erase(tasks_.begin(), tasks_.begin() + kMaxTasks);

                    wakeUp();
                }
            }
            for (Task &task : tasks) {
                LOG(debug, "task");

                task();
            }
        }

        state_ = State::kIdle;
    }
}

bool EventLoop::hasWatcher(int fd) {
    std::unique_lock lock(watchersMutex_);
    return watchers_.find(fd) != watchers_.end();
}

void EventLoop::addWatcher(Watcher *watcher) {
    LOG(debug, "fd={}", watcher->fd_);

    std::unique_lock lock(watchersMutex_);

    CHECK(watchers_.emplace(watcher->fd_, watcher).second);

    struct epoll_event event{};
    if (watcher->hasReadCallback()) {
        event.events |= EPOLLIN;
    }
    if (watcher->hasWriteCallback()) {
        event.events |= EPOLLOUT;
    }
    event.data.fd = watcher->fd_;

    CHECK(epoll_ctl(epollFd_, EPOLL_CTL_ADD, watcher->fd_, &event) == 0);

    lock.unlock();

    wakeUp();
}

void EventLoop::updateWatcher(Watcher *watcher) {
    LOG(debug, "fd={}", watcher->fd_);

    std::unique_lock lock(watchersMutex_);

    struct epoll_event event{};
    if (watcher->hasReadCallback()) {
        event.events |= EPOLLIN;
    }
    if (watcher->hasWriteCallback()) {
        event.events |= EPOLLOUT;
    }
    event.data.fd = watcher->fd_;

    CHECK(epoll_ctl(epollFd_, EPOLL_CTL_MOD, watcher->fd_, &event) == 0);

    lock.unlock();

    wakeUp();
}

void EventLoop::updateWatcherIfRegistered(Watcher *watcher) {
    LOG(debug, "fd={}", watcher->fd_);

    std::unique_lock lock(watchersMutex_);

    if (watchers_.find(watcher->fd_) == watchers_.end()) return;

    struct epoll_event event{};
    if (watcher->hasReadCallback()) {
        event.events |= EPOLLIN;
    }
    if (watcher->hasWriteCallback()) {
        event.events |= EPOLLOUT;
    }
    event.data.fd = watcher->fd_;

    CHECK(epoll_ctl(epollFd_, EPOLL_CTL_MOD, watcher->fd_, &event) == 0);

    lock.unlock();

    wakeUp();
}

void EventLoop::removeWatcher(Watcher *watcher) {
    LOG(debug, "fd={}", watcher->fd_);

    std::unique_lock lock(watchersMutex_);

    CHECK(epoll_ctl(epollFd_, EPOLL_CTL_DEL, watcher->fd_, nullptr) == 0);

    CHECK(watchers_.erase(watcher->fd_) == 1);

    lock.unlock();

    wakeUp();
}

void EventLoop::wakeUp() {
    LOG(debug, "");

    uint64_t value = 1;
    CHECK(write(eventFd_, &value, sizeof(value)) == sizeof(value));
}

EventLoop *mq::EventLoop::background() {
    std::promise<EventLoop *> promise;
    std::future<EventLoop *> future = promise.get_future();

    std::thread([promise = std::move(promise)] mutable {
        EventLoop loop;
        promise.set_value(&loop);
        loop.run();
    }).detach();

    return future.get();
}
