#include "mq/message/MultiplexingRequester.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"

#define TAG "MultiplexingRequester"

using namespace mq;

uint64_t MultiplexingRequester::nextRequestId_ = 0;

MultiplexingRequester::MultiplexingRequester(EventLoop *loop, const Endpoint &remoteEndpoint)
    : requester_(loop, remoteEndpoint), timer_(loop) {
    LOG(debug, "");

    requester_.setRecvCallback([this](std::string_view message) {
        onRequesterRecv(message);
    });
}

MultiplexingRequester::~MultiplexingRequester() {
    LOG(debug, "");
}

void MultiplexingRequester::setRequestTimeout(std::chrono::nanoseconds requestTimeout) {
    LOG(debug, "");

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

        if (requestTimeout_.count() > 0) {
            timer_.addExpireCallback([this] {
                return onTimerExpire();
            });

            timer_.open();

            timer_.setTime(requestTimeout_, requestTimeout_);
        }
    } else {
        loop()->postAndWait([this] {
            open();
        });
    }
}

void MultiplexingRequester::send(std::string message, RecvCallback recvCallback, Executor *recvCallbackExecutor) {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kOpened);

        uint64_t requestId = nextRequestId_++;
        requestId = toLittleEndian(requestId);

        requests_.emplace(requestId, std::pair(std::move(recvCallback), recvCallbackExecutor));

        message.insert(0, reinterpret_cast<const char *>(&requestId), 8);
        requester_.send(message);
    } else {
        loop()->post([this,
                      message = std::move(message),
                      recvCallback = std::move(recvCallback),
                      recvCallbackExecutor] mutable {
            send(std::move(message), std::move(recvCallback), recvCallbackExecutor);
        });
    }
}

size_t MultiplexingRequester::numOutstandingRequests() const {
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

void MultiplexingRequester::onRequesterRecv(std::string_view message) {
    LOG(debug, "");

    if (message.size() < 8) {
        LOG(warning, "Bad reply");

        return;
    }

    uint64_t requestId;
    memcpy(&requestId, message.data(), 8);
    requestId = fromLittleEndian(requestId);

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
        LOG(warning, "Unknown request");

        return;
    }
}

bool MultiplexingRequester::onTimerExpire() {
    LOG(debug, "");

    for (uint64_t requestId : requestsToExpire_) {
        if (auto i = requests_.find(requestId); i != requests_.end()) {
            LOG(warning, "Request timed out: {}", requestId);

            requests_.erase(i);
        }
    }

    requestsToExpire_.clear();

    for (const auto &request : requests_) {
        requestsToExpire_.push_back(request.first);
    }

    return true;
}
