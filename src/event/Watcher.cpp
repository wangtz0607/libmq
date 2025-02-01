#include "mq/event/Watcher.h"

#include <utility>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "Watcher"

using namespace mq;

Watcher::Watcher(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd) {
    LOG(debug, "fd={}", fd_);
}

Watcher::~Watcher() {
    LOG(debug, "fd={}", fd_);

    CHECK(!loop_->hasWatcher(fd_));
}

bool Watcher::hasReadReadyCallback() const {
    CHECK(loop_->isInLoopThread());

    return !readReadyCallbacks_.empty();
}

bool Watcher::hasWriteReadyCallback() const {
    CHECK(loop_->isInLoopThread());

    return !writeReadyCallbacks_.empty();
}

void Watcher::addReadReadyCallback(ReadReadyCallback readReadyCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    readReadyCallbacks_.emplace_back(std::move(readReadyCallback));

    if (readReadyCallbacks_.size() == 1) {
        loop_->updateWatcherIfRegistered(this);
    }
}

void Watcher::addWriteReadyCallback(WriteReadyCallback writeReadyCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    writeReadyCallbacks_.emplace_back(std::move(writeReadyCallback));

    if (writeReadyCallbacks_.size() == 1) {
        loop_->updateWatcherIfRegistered(this);
    }
}

void Watcher::clearReadReadyCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (!readReadyCallbacks_.empty()) {
        readReadyCallbacks_.clear();

        loop_->updateWatcherIfRegistered(this);
    }
}

void Watcher::clearWriteReadyCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (!writeReadyCallbacks_.empty()) {
        writeReadyCallbacks_.clear();

        loop_->updateWatcherIfRegistered(this);
    }
}

void Watcher::dispatchReadReady() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    std::vector<ReadReadyCallback> readReadyCallbacks(std::move(readReadyCallbacks_));
    readReadyCallbacks_.clear();

    for (ReadReadyCallback &readReadyCallback : readReadyCallbacks) {
        if (readReadyCallback()) {
            readReadyCallbacks_.emplace_back(std::move(readReadyCallback));
        }
    }
}

void Watcher::dispatchWriteReady() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    std::vector<WriteReadyCallback> writeReadyCallbacks(std::move(writeReadyCallbacks_));
    writeReadyCallbacks_.clear();

    for (WriteReadyCallback &writeReadyCallback : writeReadyCallbacks) {
        if (writeReadyCallback()) {
            writeReadyCallbacks_.emplace_back(std::move(writeReadyCallback));
        }
    }
}

void Watcher::registerSelf() {
    LOG(debug, "");

    loop_->addWatcher(this);
}

void Watcher::unregisterSelf() {
    LOG(debug, "");

    CHECK(loop_->state_ == EventLoop::State::kTask);

    loop_->removeWatcher(this);
}
