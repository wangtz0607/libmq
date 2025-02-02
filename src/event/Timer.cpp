#include "mq/event/Timer.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <sys/timerfd.h>

#include "mq/event/EventLoop.h"
#include "mq/event/Watcher.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "Timer"

using namespace mq;

Timer::Timer(EventLoop *loop)
    : loop_(loop) {
    LOG(debug, "");
}

Timer::~Timer() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(loop_->state() == EventLoop::State::kTask);
    CHECK(state_ == State::kClosed);
}

bool Timer::hasExpireCallback() const {
    CHECK(loop_->isInLoopThread());

    return !expireCallbacks_.empty();
}

void Timer::addExpireCallback(ExpireCallback expireCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    expireCallbacks_.emplace_back(std::move(expireCallback));
}

void Timer::clearExpireCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    expireCallbacks_.clear();
}

void Timer::dispatchExpire() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    std::vector<ExpireCallback> expireCallbacks = std::move(expireCallbacks_);
    expireCallbacks_.clear();

    for (ExpireCallback &expireCallback : expireCallbacks) {
        if (expireCallback()) {
            expireCallbacks_.emplace_back(std::move(expireCallback));
        }
    }
}

Timer::State Timer::state() const {
    CHECK(loop_->isInLoopThread());

    return state_;
}

int Timer::fd() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kOpened);

    return fd_;
}

Watcher &Timer::watcher() {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kOpened);

    return *watcher_;
}

const Watcher &Timer::watcher() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kOpened);

    return *watcher_;
}

void Timer::open() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    CHECK((fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)) >= 0);

    watcher_ = std::make_unique<Watcher>(loop_, fd_);
    watcher_->registerSelf();
    watcher_->addReadReadyCallback([this] { return onWatcherReadReady(); });

    State oldState = state_;
    state_ = State::kOpened;
    LOG(info, "{} -> {}", oldState, state_);
}

void Timer::setTime(std::chrono::nanoseconds delay) {
    LOG(debug, "delay={}", delay);

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kOpened);
    CHECK(delay.count() > 0);

    itimerspec newValue{};
    newValue.it_value.tv_sec = delay.count() / 1'000'000'000;
    newValue.it_value.tv_nsec = delay.count() % 1'000'000'000;

    CHECK(timerfd_settime(fd_, 0, &newValue, nullptr) == 0);
}

void Timer::setTime(std::chrono::nanoseconds delay, std::chrono::nanoseconds interval) {
    LOG(debug, "delay={}, interval={}", delay, interval);

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kOpened);
    CHECK(delay.count() > 0);
    CHECK(interval.count() > 0);

    itimerspec newValue{};
    newValue.it_value.tv_sec = delay.count() / 1'000'000'000;
    newValue.it_value.tv_nsec = delay.count() % 1'000'000'000;
    newValue.it_interval.tv_sec = interval.count() / 1'000'000'000;
    newValue.it_interval.tv_nsec = interval.count() % 1'000'000'000;

    CHECK(timerfd_settime(fd_, 0, &newValue, nullptr) == 0);
}

void Timer::cancel() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kOpened);

    itimerspec newValue{};
    CHECK(timerfd_settime(fd_, 0, &newValue, nullptr) == 0);
}

void Timer::close() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(info, "{} -> {}", oldState, state_);

    watcher_->clearReadReadyCallbacks();

    loop_->post([watcher = std::move(watcher_), fd = fd_] {
        watcher->unregisterSelf();

        CHECK(::close(fd) == 0);
    });

    watcher_ = nullptr;
}

void Timer::reset() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    clearExpireCallbacks();

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(info, "{} -> {}", oldState, state_);

    watcher_->clearReadReadyCallbacks();

    loop_->post([watcher = std::move(watcher_), fd = fd_] {
        watcher->unregisterSelf();

        CHECK(::close(fd) == 0);
    });

    watcher_ = nullptr;
}

bool Timer::onWatcherReadReady() {
    LOG(debug, "");

    uint64_t value;
    CHECK(read(fd_, &value, sizeof(value)) == sizeof(value));

    dispatchExpire();

    return true;
}
