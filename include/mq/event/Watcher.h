#pragma once

#include <functional>
#include <vector>

namespace mq {

class EventLoop;

class Watcher {
public:
    using ReadCallback = std::move_only_function<bool ()>;
    using WriteCallback = std::move_only_function<bool ()>;

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

    bool hasReadCallback() const;
    bool hasWriteCallback() const;

    void addReadCallback(ReadCallback readCallback);
    void addWriteCallback(WriteCallback writeCallback);

    void clearReadCallbacks();
    void clearWriteCallbacks();

    void dispatchRead();
    void dispatchWrite();

    void registerSelf();
    void unregisterSelf();

private:
    EventLoop *loop_;
    int fd_;
    std::vector<ReadCallback> readCallbacks_;
    std::vector<WriteCallback> writeCallbacks_;

    friend class EventLoop;
};

} // namespace mq
