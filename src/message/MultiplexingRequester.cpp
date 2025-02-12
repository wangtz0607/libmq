// SPDX-License-Identifier: MIT

#include "mq/message/MultiplexingRequester.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/utils/Check.h"
#include "mq/utils/Empty.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"
#include "mq/utils/MaybeOwnedString.h"

#define TAG "MultiplexingRequester"

using namespace mq;

uint64_t MultiplexingRequester::nextRequestId_ = 0;

MultiplexingRequester::MultiplexingRequester(EventLoop *loop, const Endpoint &remoteEndpoint)
    : requester_(loop, remoteEndpoint) {
    LOG(debug, "");

    requester_.setRecvCallback([this](std::string_view message) {
        onRequesterRecv(message);
    });
}

MultiplexingRequester::~MultiplexingRequester() {
    LOG(debug, "");
}

void MultiplexingRequester::setMaxPendingRequests(size_t maxPendingRequests) {
    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        maxPendingRequests_ = maxPendingRequests;
    } else {
        loop()->postAndWait([this, maxPendingRequests] {
            CHECK(state() == State::kClosed);

            maxPendingRequests_ = maxPendingRequests;
        });
    }
}

void MultiplexingRequester::setRequestTimeout(std::chrono::nanoseconds requestTimeout) {
    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        requestTimeout_ = requestTimeout;
    } else {
        loop()->postAndWait([this, requestTimeout] {
            CHECK(state() == State::kClosed);

            requestTimeout_ = requestTimeout;
        });
    }
}

void MultiplexingRequester::open() {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        requester_.open();

        token_ = std::make_shared<Empty>();

        if (requestTimeout_.count() > 0) {
            timer_ = std::make_unique<Timer>(loop());

            timer_->addExpireCallback([this] {
                return onTimerExpire();
            });

            timer_->open();

            timer_->setTime(requestTimeout_, requestTimeout_);
        }
    } else {
        loop()->postAndWait([this] {
            open();
        });
    }
}

void MultiplexingRequester::send(MaybeOwnedString message,
                                 RecvCallback recvCallback,
                                 Executor *recvCallbackExecutor) {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kOpened);

        if (maxPendingRequests_ > 0 && requests_.size() == maxPendingRequests_) {
            LOG(warning, "Too many pending requests");

            requests_.erase(requests_.begin());
        }

        uint64_t requestId = nextRequestId_++;
        uint64_t requestIdLE = toLittleEndian(requestId);

        requests_.emplace(requestId, std::pair(std::move(recvCallback), recvCallbackExecutor));

        std::vector<MaybeOwnedString> pieces;
        pieces.reserve(2);
        pieces.emplace_back(reinterpret_cast<const char *>(&requestIdLE), 8);
        pieces.emplace_back(std::move(message));

        requester_.send(std::move(pieces));
    } else {
        loop()->post([this,
                      message = std::string(std::move(message)),
                      recvCallback = std::move(recvCallback),
                      recvCallbackExecutor,
                      token = std::weak_ptr(token_)] mutable {
            if (token.expired()) return;

            send(std::move(message), std::move(recvCallback), recvCallbackExecutor);
        });
    }
}

void MultiplexingRequester::send(std::vector<MaybeOwnedString> pieces,
                                 RecvCallback recvCallback,
                                 Executor *recvCallbackExecutor) {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kOpened);

        if (maxPendingRequests_ > 0 && requests_.size() == maxPendingRequests_) {
            LOG(warning, "Too many pending requests");

            requests_.erase(requests_.begin());
        }

        uint64_t requestId = nextRequestId_++;
        uint64_t requestIdLE = toLittleEndian(requestId);

        requests_.emplace(requestId, std::pair(std::move(recvCallback), recvCallbackExecutor));

        std::vector<MaybeOwnedString> newPieces;
        newPieces.reserve(1 + pieces.size());
        newPieces.emplace_back(reinterpret_cast<const char *>(&requestIdLE), 8);
        newPieces.insert(newPieces.end(),
                         std::make_move_iterator(pieces.begin()),
                         std::make_move_iterator(pieces.end()));

        requester_.send(std::move(newPieces));
    } else {
        size_t size = 0;
        for (const MaybeOwnedString &piece : pieces) {
            size += piece.size();
        }

        auto op = [pieces = std::move(pieces)](char *data, size_t size) {
            for (const MaybeOwnedString &piece : pieces) {
                memcpy(data, piece.data(), piece.size());
                data += piece.size();
            }

            return size;
        };

        std::string message;
        message.resize_and_overwrite(size, std::move(op));

        loop()->post([this,
                      message = std::move(message),
                      recvCallback = std::move(recvCallback),
                      recvCallbackExecutor,
                      token = std::weak_ptr(token_)] mutable {
            if (token.expired()) return;

            send(std::move(message), std::move(recvCallback), recvCallbackExecutor);
        });
    }
}

size_t MultiplexingRequester::numPendingRequests() const {
    size_t result;

    if (loop()->isInLoopThread()) {
        result = requests_.size();
    } else {
        loop()->postAndWait([this, &result] {
            result = requests_.size();
        });
    }

    return result;
}

void MultiplexingRequester::close() {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        if (state() == State::kClosed) return;

        if (requestTimeout_.count() > 0) {
            timer_->reset();

            loop()->post([timer = std::move(timer_)] {});
        }

        token_ = nullptr;

        requester_.close();
    } else {
        loop()->postAndWait([this] {
            close();
        });
    }
}

void MultiplexingRequester::onRequesterRecv(std::string_view message) {
    LOG(debug, "");

    if (message.size() < 8) {
        LOG(warning, "Bad reply");

        return;
    }

    uint64_t requestIdLE;
    memcpy(&requestIdLE, message.data(), 8);

    uint64_t requestId = fromLittleEndian(requestIdLE);

    if (auto i = requests_.find(requestId); i != requests_.end()) {
        RecvCallback recvCallback = std::move(i->second.first);
        Executor *recvCallbackExecutor = i->second.second;

        requests_.erase(i);

        if (!recvCallbackExecutor) {
            recvCallback(message.substr(8));
        } else {
            recvCallbackExecutor->post([recvCallback = std::move(recvCallback),
                                        message = std::string(message)] mutable {
                recvCallback(message.substr(8));
            });
        }
    } else {
        LOG(warning, "Unknown request: {}", requestId);

        return;
    }
}

bool MultiplexingRequester::onTimerExpire() {
    LOG(debug, "");

    for (uint64_t requestId : expiringRequests_) {
        if (auto i = requests_.find(requestId); i != requests_.end()) {
            LOG(warning, "Request timed out: {}", requestId);

            requests_.erase(i);
        }
    }

    expiringRequests_.clear();

    for (const auto &request : requests_) {
        expiringRequests_.push_back(request.first);
    }

    return true;
}
