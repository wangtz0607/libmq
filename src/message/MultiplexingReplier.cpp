#include "mq/message/MultiplexingReplier.h"

#include <cstdint>
#include <cstring>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/message/Replier.h"
#include "mq/net/Endpoint.h"
#include "mq/utils/Check.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"
#include "mq/utils/MaybeOwnedString.h"

#define TAG "MultiplexingReplier"

using namespace mq;

void MultiplexingReplier::Promise::operator()(MaybeOwnedString replyMessage) {
    std::vector<MaybeOwnedString> replyPieces;
    replyPieces.reserve(2);
    replyPieces.emplace_back(reinterpret_cast<const char *>(&requestIdLE_), 8);
    replyPieces.push_back(std::move(replyMessage));

    promise_(std::move(replyPieces));
}

void MultiplexingReplier::Promise::operator()(std::vector<MaybeOwnedString> replyPieces) {
    std::vector<MaybeOwnedString> newReplyPieces;
    newReplyPieces.reserve(1 + replyPieces.size());
    newReplyPieces.emplace_back(reinterpret_cast<const char *>(&requestIdLE_), 8);
    newReplyPieces.insert(newReplyPieces.end(),
                          std::make_move_iterator(replyPieces.begin()),
                          std::make_move_iterator(replyPieces.end()));

    promise_(std::move(newReplyPieces));
}

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

    Promise newPromise(requestIdLE, std::move(promise));

    recvCallback_(remoteEndpoint, message.substr(8), std::move(newPromise));
}
