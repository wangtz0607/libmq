#include "mq/rpc/RPCClient.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <future>
#include <string>
#include <string_view>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingRequester.h"
#include "mq/net/Endpoint.h"
#include "mq/rpc/RPCError.h"
#include "mq/utils/Check.h"
#include "mq/utils/Expected.h"
#include "mq/utils/Logging.h"

#define TAG "RPCClient"

using namespace mq;

namespace {

class RecvCallbackImpl {
public:
    explicit RecvCallbackImpl(std::promise<Expected<std::string, RPCError>> promise)
        : promise_(std::move(promise)), valid_(true) {}

    ~RecvCallbackImpl() {
        if (valid_) {
            promise_.set_value(RPCError::kTimedOut);
        }
    }

    RecvCallbackImpl(RecvCallbackImpl &&other) noexcept
        : promise_(std::move(other.promise_)), valid_(other.valid_) {
        other.valid_ = false;
    }

    void operator()(std::string_view message) {
        if (message.size() < 1) {
            LOG(warning, "Bad reply");

            promise_.set_value(RPCError::kBadReply);
            valid_ = false;
            return;
        }

        uint8_t statusCode;
        memcpy(&statusCode, message.data(), 1);

        RPCError status = static_cast<RPCError>(statusCode);

        if (status != RPCError::kOk) {
            promise_.set_value(status);
            valid_ = false;
            return;
        }

        promise_.set_value(std::string(message.substr(1)));
        valid_ = false;
    }

private:
    std::promise<Expected<std::string, RPCError>> promise_;
    bool valid_;
};

} // namespace

RPCClient::RPCClient(EventLoop *loop, const Endpoint &remoteEndpoint) : requester_(loop, remoteEndpoint) {
    LOG(debug, "");
}

RPCClient::~RPCClient() {
    LOG(debug, "");
}

std::future<Expected<std::string, RPCError>> RPCClient::call(std::string_view methodName, std::string_view payload) {
    LOG(debug, "methodName={}", methodName);

    CHECK(methodName.size() < 256);

    size_t size = 1 + methodName.size() + payload.size();

    auto op = [methodName, payload](char *data, size_t size) {
        uint8_t methodNameLength = static_cast<uint8_t>(methodName.size());

        memcpy(data, &methodNameLength, 1);
        data += 1;
        memcpy(data, methodName.data(), methodName.size());
        data += methodName.size();
        memcpy(data, payload.data(), payload.size());

        return size;
    };

    std::string message;
    message.resize_and_overwrite(size, std::move(op));

    std::promise<Expected<std::string, RPCError>> promise;
    std::future<Expected<std::string, RPCError>> future = promise.get_future();

    RecvCallbackImpl recvCallback(std::move(promise));

    requester_.send(message, std::move(recvCallback));

    return future;
}
