#include "mq/message/MultiplexingReplier.h"

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

void MultiplexingReplier::onReplierRecv(const Endpoint &remoteEndpoint, std::string_view message, Replier::Promise promise) {
    LOG(debug, "");

    if (message.size() < 8) {
        LOG(warning, "Bad request");

        return;
    }

    Promise multiplexingPromise =
        [message, promise = std::move(promise)](const std::string_view replyMessage) mutable {
            std::string multiplexingReplyMessage;
            multiplexingReplyMessage.resize_and_overwrite(8 + replyMessage.size(),
                [message, replyMessage](char *data, size_t size) {
                    memcpy(data, message.data(), 8);
                    data += 8;
                    memcpy(data, replyMessage.data(), replyMessage.size());

                    return size;
                });

            promise(multiplexingReplyMessage);
        };

    recvCallback_(remoteEndpoint, message.substr(8), std::move(multiplexingPromise));
}
