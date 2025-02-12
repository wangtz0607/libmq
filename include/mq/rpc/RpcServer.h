// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingReplier.h"
#include "mq/net/Endpoint.h"
#include "mq/utils/Executor.h"
#include "mq/utils/StringEqual.h"
#include "mq/utils/StringHash.h"

namespace mq {

class RpcServer {
public:
    enum class State {
        kClosed = static_cast<int>(MultiplexingReplier::State::kClosed),
        kOpened = static_cast<int>(MultiplexingReplier::State::kOpened),
    };

    using Method = std::move_only_function<std::string (const Endpoint &remoteEndpoint, std::string_view payload)>;

    RpcServer(EventLoop *loop, const Endpoint &localEndpoint);
    ~RpcServer();

    RpcServer(const RpcServer &) = delete;
    RpcServer(RpcServer &&) = delete;

    RpcServer &operator=(const RpcServer &) = delete;
    RpcServer &operator=(RpcServer &&) = delete;

    EventLoop *loop() const {
        return replier_.loop();
    }

    std::unique_ptr<Endpoint> localEndpoint() const {
        return replier_.localEndpoint();
    }

    void setMaxConnections(size_t maxConnections) {
        replier_.setMaxConnections(maxConnections);
    }

    void setMaxMessageLength(size_t maxMessageLength) {
        replier_.setMaxMessageLength(maxMessageLength);
    }

    void setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
        replier_.setRecvBufferMaxCapacity(recvBufferMaxCapacity);
    }

    void setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
        replier_.setSendBufferMaxCapacity(sendBufferMaxCapacity);
    }

    void setRecvChunkSize(size_t recvChunkSize) {
        replier_.setRecvChunkSize(recvChunkSize);
    }

    void setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
        replier_.setRecvTimeout(recvTimeout);
    }

    void setSendTimeout(std::chrono::nanoseconds sendTimeout) {
        replier_.setSendTimeout(sendTimeout);
    }

    void setReuseAddr(bool reuseAddr) {
        replier_.setReuseAddr(reuseAddr);
    }

    void setReusePort(bool reusePort) {
        replier_.setReusePort(reusePort);
    }

    void setNoDelay(bool noDelay) {
        replier_.setNoDelay(noDelay);
    }

    void setKeepAlive(KeepAlive keepAlive) {
        replier_.setKeepAlive(keepAlive);
    }

    bool hasMethod(std::string_view methodName) const;
    void registerMethod(std::string methodName, Method method, Executor *methodExecutor = nullptr);
    void unregisterMethod(std::string_view methodName);
    void unregisterAllMethods();

    State state() const {
        return static_cast<State>(replier_.state());
    }

    int open();
    void close();

private:
    using MethodMap = std::unordered_map<std::string, std::pair<Method, Executor *>, StringHash, StringEqual>;

    MultiplexingReplier replier_;
    MethodMap methods_;
    std::shared_ptr<void> token_;

    void onMultiplexingReplierRecv(const Endpoint &remoteEndpoint,
                                   std::string_view message,
                                   MultiplexingReplier::Promise promise);
};

} // namespace mq
