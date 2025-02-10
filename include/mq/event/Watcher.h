// SPDX-License-Identifier: MIT

#pragma once

#include <functional>
#include <vector>

namespace mq {

class EventLoop;

class Watcher {
public:
    using ReadReadyCallback = std::move_only_function<bool ()>;
    using WriteReadyCallback = std::move_only_function<bool ()>;

    explicit Watcher(EventLoop *loop, int fd);
    ~Watcher();

    Watcher(const Watcher &) = delete;
    Watcher(Watcher &&) = delete;

    Watcher &operator=(const Watcher &) = delete;
    Watcher &operator=(Watcher &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    int fd() const {
        return fd_;
    }

    bool hasReadReadyCallback() const;
    bool hasWriteReadyCallback() const;

    void addReadReadyCallback(ReadReadyCallback readReadyCallback);
    void addWriteReadyCallback(WriteReadyCallback writeReadyCallback);

    void clearReadReadyCallbacks();
    void clearWriteReadyCallbacks();

    void dispatchReadReady();
    void dispatchWriteReady();

    void registerSelf();
    void unregisterSelf();

private:
    EventLoop *loop_;
    int fd_;
    std::vector<ReadReadyCallback> readReadyCallbacks_;
    std::vector<WriteReadyCallback> writeReadyCallbacks_;

    friend class EventLoop;
};

} // namespace mq
