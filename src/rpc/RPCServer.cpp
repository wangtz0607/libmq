// SPDX-License-Identifier: MIT

#include "mq/rpc/RPCServer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingReplier.h"
#include "mq/net/Endpoint.h"
#include "mq/rpc/RPCError.h"
#include "mq/utils/Check.h"
#include "mq/utils/Empty.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"
#include "mq/utils/MaybeOwnedString.h"

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

int RPCServer::open() {
    LOG(debug, "");

    int error;

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kClosed);

        error = replier_.open();

        if (!error) {
            token_ = std::make_shared<Empty>();
        }
    } else {
        loop()->postAndWait([this, &error] {
            error = open();
        });
    }

    return error;
}

void RPCServer::close() {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        if (state() == State::kClosed) return;

        token_ = nullptr;

        replier_.close();
    } else {
        loop()->postAndWait([this] {
            close();
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

            std::vector<MaybeOwnedString> resultPieces;
            resultPieces.reserve(2);
            resultPieces.emplace_back(reinterpret_cast<const char *>(&statusCode), 1);
            resultPieces.emplace_back(std::move(resultPayload));

            promise(std::move(resultPieces));
        } else {
            methodExecutor->post([&method,
                                  remoteEndpoint = remoteEndpoint.clone(),
                                  payload = std::string(payload),
                                  promise = std::move(promise),
                                  token = std::weak_ptr(token_)] mutable {
                if (token.expired()) return;

                RPCError status = RPCError::kOk;
                uint8_t statusCode = static_cast<uint8_t>(status);

                std::string resultPayload = method(*remoteEndpoint, payload);

                std::vector<MaybeOwnedString> resultPieces;
                resultPieces.reserve(2);
                resultPieces.emplace_back(reinterpret_cast<const char *>(&statusCode), 1);
                resultPieces.emplace_back(std::move(resultPayload));

                promise(std::move(resultPieces));
            });
        }
    } else {
        LOG(warning, "Method not found: {}", methodName);

        RPCError status = RPCError::kMethodNotFound;
        uint8_t statusCode = static_cast<uint8_t>(status);
        promise(std::string(reinterpret_cast<const char *>(&statusCode), 1));
    }
}
