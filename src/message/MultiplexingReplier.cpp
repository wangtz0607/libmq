#include "mq/message/MultiplexingReplier.h"

#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

#include "mq/message/Replier.h"
#include "mq/net/Endpoint.h"
#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"

#define TAG "MultiplexingReplier"

using namespace mq;

MultiplexingReplier::MultiplexingReplier(EventLoop *loop, const Endpoint &localEndpoint)
    : replier_(loop, localEndpoint) {
    LOG(debug, "");

    replier_.setRecvCallback([this](const Endpoint &remoteEndpoint, std::string_view message) {
        return onReplierRecv(remoteEndpoint, message);
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

std::optional<std::string> MultiplexingReplier::onReplierRecv(const Endpoint &remoteEndpoint, std::string_view message) {
    LOG(debug, "");

    if (message.size() < 8) {
        LOG(warning, "Bad request");

        return "";
    }

    uint64_t requestId;
    memcpy(&requestId, message.data(), 8);
    requestId = fromLittleEndian(requestId);

    std::optional<std::string> replyMessage = recvCallback_(remoteEndpoint, message.substr(8));

    if (replyMessage) {
        replyMessage->insert(0, reinterpret_cast<const char *>(&requestId), 8);
    }

    return replyMessage;
}
