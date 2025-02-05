#include "mq/rpc/RPCClient.h"

#include <cstdint>
#include <cstring>
#include <format>
#include <future>
#include <string>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingRequester.h"
#include "mq/net/Endpoint.h"
#include "mq/rpc/RPCError.h"
#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Expected.h"
#include "mq/utils/Logging.h"

#define TAG "RPCClient"

using namespace mq;

RPCClient::RPCClient(EventLoop *loop, const Endpoint &remoteEndpoint) : requester_(loop, remoteEndpoint) {
    LOG(debug, "");
}

RPCClient::~RPCClient() {
    LOG(debug, "");
}

std::future<Expected<std::string, RPCError>> RPCClient::call(std::string_view methodName, std::string_view payload) {
    LOG(debug, "methodName={}", methodName);

    CHECK(methodName.size() < 256);

    std::string message;
    message.resize_and_overwrite(1 + methodName.size() + payload.size(),
        [methodName, payload](char *data, size_t size) {
            uint8_t methodNameLength = static_cast<uint8_t>(methodName.size());
            methodNameLength = toLittleEndian(methodNameLength);

            memcpy(data, &methodNameLength, 1);
            data += 1;
            memcpy(data, methodName.data(), methodName.size());
            data += methodName.size();
            memcpy(data, payload.data(), payload.size());

            return size;
        });

    std::promise<Expected<std::string, RPCError>> promise;
    std::future<Expected<std::string, RPCError>> future = promise.get_future();

    MultiplexingRequester::RecvCallback recvCallback =
        [promise = std::move(promise)](std::string_view message) mutable {
            if (message.size() < 1) {
                promise.set_value(RPCError::kBadReply);
                return;
            }

            uint8_t statusCode;
            memcpy(&statusCode, message.data(), 1);
            statusCode = fromLittleEndian(statusCode);

            RPCError status = static_cast<RPCError>(statusCode);

            if (status != RPCError::kOk) {
                promise.set_value(status);
                return;
            }

            promise.set_value(std::string(message.substr(1)));
        };

    requester_.send(std::move(message), std::move(recvCallback));

    return future;
}
