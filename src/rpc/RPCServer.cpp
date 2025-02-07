#include "mq/rpc/RPCServer.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingReplier.h"
#include "mq/net/Endpoint.h"
#include "mq/rpc/RPCError.h"
#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"

#define TAG "RPCServer"

using namespace mq;

RPCServer::RPCServer(EventLoop *loop, const Endpoint &localEndpoint) : replier_(loop, localEndpoint) {
    LOG(debug, "");

    replier_.setRecvCallback([this](const Endpoint &remoteEndpoint,
                                    std::string_view message,
                                    MultiplexingReplier::Promise promise) {
        onMultiplexingReplierRecv(remoteEndpoint, message, std::move(promise));
    });
}

RPCServer::~RPCServer() {
    LOG(debug, "");
}

bool RPCServer::hasMethod(std::string_view methodName) const {
    LOG(debug, "methodName={}", methodName);

    bool result;

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        result = methods_.find(methodName) != methods_.end();
    } else {
        loop()->postAndWait([this, methodName, &result] {
            CHECK(state() == State::kClosed);

            result = methods_.find(methodName) != methods_.end();
        });
    }

    return result;
}

void RPCServer::registerMethod(std::string methodName, Method method, Executor *methodExecutor) {
    LOG(debug, "methodName={}", methodName);

    CHECK(methodName.size() < 256);

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        methods_.emplace(std::move(methodName), std::pair(std::move(method), methodExecutor));
    } else {
        loop()->postAndWait([this,
                             methodName = std::move(methodName),
                             method = std::move(method),
                             methodExecutor] mutable {
            CHECK(state() == State::kClosed);

            methods_.emplace(std::move(methodName), std::pair(std::move(method), methodExecutor));
        });
    }
}

void RPCServer::unregisterMethod(std::string_view methodName) {
    LOG(debug, "methodName={}", methodName);

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        methods_.erase(methods_.find(methodName));
    } else {
        loop()->postAndWait([this, methodName] {
            CHECK(state() == State::kClosed);

            methods_.erase(methods_.find(methodName));
        });
    }
}

void RPCServer::unregisterAllMethods() {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        methods_.clear();
    } else {
        loop()->postAndWait([this] {
            CHECK(state() == State::kClosed);

            methods_.clear();
        });
    }
}

void RPCServer::onMultiplexingReplierRecv(const Endpoint &remoteEndpoint,
                                          std::string_view message,
                                          MultiplexingReplier::Promise promise) {
    LOG(debug, "");

    if (message.size() < 1) {
        LOG(warning, "Bad request");

        RPCError status = RPCError::kBadRequest;
        uint8_t statusCode = static_cast<uint8_t>(status);
        promise(std::string(reinterpret_cast<const char *>(&statusCode), 1));
        return;
    }

    uint8_t methodNameLengthLE;
    memcpy(&methodNameLengthLE, message.data(), 1);

    uint8_t methodNameLength = fromLittleEndian(methodNameLengthLE);

    if (message.size() < static_cast<size_t>(1 + methodNameLength)) {
        LOG(warning, "Bad request");

        RPCError status = RPCError::kBadRequest;
        uint8_t statusCode = static_cast<uint8_t>(status);
        promise(std::string(reinterpret_cast<const char *>(&statusCode), 1));
        return;
    }

    std::string_view methodName = message.substr(1, methodNameLength);
    std::string_view payload = message.substr(1 + methodNameLength);

    if (auto i = methods_.find(methodName); i != methods_.end()) {
        Method &method = i->second.first;
        Executor *methodExecutor = i->second.second;

        if (!methodExecutor) {
            RPCError status = RPCError::kOk;
            uint8_t statusCode = static_cast<uint8_t>(status);

            std::string resultPayload = method(remoteEndpoint, payload);

            size_t size = 1 + resultPayload.size();

            auto op = [statusCode, &resultPayload](char *data, size_t size) {
                memcpy(data, reinterpret_cast<const char *>(&statusCode), 1);
                data += 1;
                memcpy(data, resultPayload.data(), resultPayload.size());

                return size;
            };

            std::string resultMessage;
            resultMessage.resize_and_overwrite(size, std::move(op));

            promise(resultMessage);
        } else {
            methodExecutor->post([&method,
                                  remoteEndpoint = remoteEndpoint.clone(),
                                  payload = std::string(payload),
                                  promise = std::move(promise)] mutable {
                RPCError status = RPCError::kOk;
                uint8_t statusCode = static_cast<uint8_t>(status);

                std::string resultPayload = method(*remoteEndpoint, payload);

                size_t size = 1 + resultPayload.size();

                auto op = [statusCode, &resultPayload](char *data, size_t size) {
                    memcpy(data, reinterpret_cast<const char *>(&statusCode), 1);
                    data += 1;
                    memcpy(data, resultPayload.data(), resultPayload.size());

                    return size;
                };

                std::string resultMessage;
                resultMessage.resize_and_overwrite(size, std::move(op));

                promise(resultMessage);
            });
        }
    } else {
        LOG(warning, "Method not found: {}", methodName);

        RPCError status = RPCError::kMethodNotFound;
        uint8_t statusCode = static_cast<uint8_t>(status);
        promise(std::string(reinterpret_cast<const char *>(&statusCode), 1));
    }
}
