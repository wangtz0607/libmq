#include "mq/message/MultiplexingReplier.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "mq/message/Replier.h"
#include "mq/net/Endpoint.h"
#include "mq/utils/Check.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"

#define TAG "MultiplexingReplier"

using namespace mq;

MultiplexingReplier::MultiplexingReplier(EventLoop *loop, const Endpoint &localEndpoint)
    : replier_(loop, localEndpoint) {
    LOG(debug, "");

    replier_.setRecvCallback(
        [this](const Endpoint &remoteEndpoint, std::string_view message, Replier::Promise promise) mutable {
            return onReplierRecv(remoteEndpoint, message, std::move(promise));
        });
}

MultiplexingReplier::~MultiplexingReplier() {
    LOG(debug, "");
}

void MultiplexingReplier::setRecvCallback(RecvCallback recvCallback) {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        recvCallback_ = std::move(recvCallback);
    } else {
        loop()->postAndWait([this, &recvCallback] {
            CHECK(state() == State::kClosed);

            recvCallback_ = std::move(recvCallback);
        });
    }
}

void MultiplexingReplier::setRecvCallbackExecutor(Executor *recvCallbackExecutor) {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        replier_.setRecvCallbackExecutor(recvCallbackExecutor);
    } else {
        loop()->postAndWait([this, &recvCallbackExecutor] {
            CHECK(state() == State::kClosed);

            replier_.setRecvCallbackExecutor(recvCallbackExecutor);
        });
    }
}

void MultiplexingReplier::onReplierRecv(const Endpoint &remoteEndpoint,
                                        std::string_view message,
                                        Replier::Promise promise) {
    LOG(debug, "");

    if (message.size() < 8) {
        LOG(warning, "Bad request");

        return;
    }

    uint64_t requestIdLE;
    memcpy(&requestIdLE, message.data(), 8);

    Promise multiplexingPromise =
        [requestIdLE, promise = std::move(promise)](std::string_view replyMessage) mutable {
            size_t size = 8 + replyMessage.size();

            auto op = [requestIdLE, replyMessage](char *data, size_t size) {
                memcpy(data, reinterpret_cast<const char *>(&requestIdLE), 8);
                data += 8;
                memcpy(data, replyMessage.data(), replyMessage.size());

                return size;
            };

            std::string multiplexingReplyMessage;
            multiplexingReplyMessage.resize_and_overwrite(size, std::move(op));

            promise(multiplexingReplyMessage);
        };

    recvCallback_(remoteEndpoint, message.substr(8), std::move(multiplexingPromise));
}
