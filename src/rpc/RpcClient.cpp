// SPDX-License-Identifier: MIT

#include "mq/rpc/RpcClient.h"

#include <cstdint>
#include <cstring>
#include <format>
#include <future>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingRequester.h"
#include "mq/net/Endpoint.h"
#include "mq/rpc/RpcError.h"
#include "mq/utils/Check.h"
#include "mq/utils/Expected.h"
#include "mq/utils/Logging.h"
#include "mq/utils/MaybeOwnedString.h"

#define TAG "RpcClient"

using namespace mq;

namespace {

class RecvCallbackImpl {
public:
    explicit RecvCallbackImpl(std::promise<Expected<std::string, RpcError>> promise)
        : promise_(std::move(promise)), valid_(true) {}

    ~RecvCallbackImpl() {
        if (valid_) {
            promise_.set_value(RpcError::kCancelled);
        }
    }

    RecvCallbackImpl(RecvCallbackImpl &&other) noexcept
        : promise_(std::move(other.promise_)), valid_(other.valid_) {
        other.valid_ = false;
    }

    void operator()(std::string_view message) {
        if (message.size() < 1) {
            LOG(warning, "Bad reply");

            promise_.set_value(RpcError::kBadReply);
            valid_ = false;
            return;
        }

        uint8_t statusCode;
        memcpy(&statusCode, message.data(), 1);

        RpcError status = static_cast<RpcError>(statusCode);

        if (status != RpcError::kOk) {
            promise_.set_value(status);
            valid_ = false;
            return;
        }

        promise_.set_value(std::string(message.substr(1)));
        valid_ = false;
    }

private:
    std::promise<Expected<std::string, RpcError>> promise_;
    bool valid_;
};

} // namespace

RpcClient::RpcClient(EventLoop *loop, const Endpoint &remoteEndpoint) : requester_(loop, remoteEndpoint) {
    LOG(debug, "");
}

RpcClient::~RpcClient() {
    LOG(debug, "");
}

std::future<Expected<std::string, RpcError>> RpcClient::call(MaybeOwnedString methodName, MaybeOwnedString payload) {
    LOG(debug, "methodName={}", methodName);

    CHECK(methodName.size() <= std::numeric_limits<uint8_t>::max());

    std::promise<Expected<std::string, RpcError>> promise;
    std::future<Expected<std::string, RpcError>> future = promise.get_future();

    uint8_t methodNameLength = static_cast<uint8_t>(methodName.size());

    std::vector<MaybeOwnedString> pieces;
    pieces.reserve(3);
    pieces.emplace_back(reinterpret_cast<const char *>(&methodNameLength), 1);
    pieces.emplace_back(std::move(methodName));
    pieces.emplace_back(std::move(payload));

    RecvCallbackImpl recvCallback(std::move(promise));

    requester_.send(std::move(pieces), std::move(recvCallback));

    return future;
}

std::future<Expected<std::string, RpcError>> RpcClient::call(MaybeOwnedString methodName,
                                                             std::vector<MaybeOwnedString> pieces) {
    LOG(debug, "methodName={}", methodName);

    CHECK(methodName.size() <= std::numeric_limits<uint8_t>::max());

    std::promise<Expected<std::string, RpcError>> promise;
    std::future<Expected<std::string, RpcError>> future = promise.get_future();

    uint8_t methodNameLength = static_cast<uint8_t>(methodName.size());

    std::vector<MaybeOwnedString> newPieces;
    newPieces.reserve(2 + pieces.size());
    newPieces.emplace_back(reinterpret_cast<const char *>(&methodNameLength), 1);
    newPieces.emplace_back(std::move(methodName));
    newPieces.insert(newPieces.end(),
                     std::make_move_iterator(pieces.begin()),
                     std::make_move_iterator(pieces.end()));

    RecvCallbackImpl recvCallback(std::move(promise));

    requester_.send(std::move(newPieces), std::move(recvCallback));

    return future;
}
