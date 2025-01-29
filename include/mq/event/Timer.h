#pragma once

#include <chrono>
#include <format>
#include <functional>
#include <memory>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/event/Watcher.h"

namespace mq {

class Timer {
public:
    using ExpireCallback = std::move_only_function<bool ()>;

    enum class State {
        kClosed,
        kOpened,
    };

    explicit Timer(EventLoop *loop);
    ~Timer();

    Timer(const Timer &) = delete;
    Timer(Timer &&) = delete;

    Timer &operator=(const Timer &) = delete;
    Timer &operator=(Timer &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    State state() const;
    int fd() const;
    Watcher &watcher();
    const Watcher &watcher() const;

    bool hasExpireCallback() const;
    void addExpireCallback(ExpireCallback expireCallback);
    void clearExpireCallbacks();
    void dispatchExpire();

    void open();
    void setTime(std::chrono::nanoseconds delay);
    void setTime(std::chrono::nanoseconds delay, std::chrono::nanoseconds interval);
    void cancel();
    void close();
    void reset();

private:
    EventLoop *loop_;
    State state_ = State::kClosed;
    int fd_;
    std::unique_ptr<Watcher> watcher_;
    std::vector<ExpireCallback> expireCallbacks_;

    bool onWatcherRead();
};

} // namespace mq

template <>
struct std::formatter<mq::Timer::State> {
public:
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::Timer::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::Timer::State state) {
        using enum mq::Timer::State;
        switch (state) {
            case kClosed: return "CLOSED";
            case kOpened: return "OPENED";
            default: return nullptr;
        }
    }
};
