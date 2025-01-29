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

bool Watcher::hasReadCallback() const {
    CHECK(loop_->isInLoopThread());

    return !readCallbacks_.empty();
}

bool Watcher::hasWriteCallback() const {
    CHECK(loop_->isInLoopThread());

    return !writeCallbacks_.empty();
}

void Watcher::addReadCallback(ReadCallback readCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    readCallbacks_.emplace_back(std::move(readCallback));

    if (readCallbacks_.size() == 1) {
        loop_->updateWatcherIfRegistered(this);
    }
}

void Watcher::addWriteCallback(WriteCallback writeCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    writeCallbacks_.emplace_back(std::move(writeCallback));

    if (writeCallbacks_.size() == 1) {
        loop_->updateWatcherIfRegistered(this);
    }
}

void Watcher::clearReadCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (!readCallbacks_.empty()) {
        readCallbacks_.clear();

        loop_->updateWatcherIfRegistered(this);
    }
}

void Watcher::clearWriteCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (!writeCallbacks_.empty()) {
        writeCallbacks_.clear();

        loop_->updateWatcherIfRegistered(this);
    }
}

void Watcher::dispatchRead() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    std::vector<ReadCallback> readCallbacks(std::move(readCallbacks_));
    readCallbacks_.clear();

    for (ReadCallback &readCallback : readCallbacks) {
        if (readCallback()) {
            readCallbacks_.emplace_back(std::move(readCallback));
        }
    }
}

void Watcher::dispatchWrite() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    std::vector<WriteCallback> writeCallbacks(std::move(writeCallbacks_));
    writeCallbacks_.clear();

    for (WriteCallback &writeCallback : writeCallbacks) {
        if (writeCallback()) {
            writeCallbacks_.emplace_back(std::move(writeCallback));
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
