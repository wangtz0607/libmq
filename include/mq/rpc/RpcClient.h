// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingRequester.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"
#include "mq/rpc/RpcError.h"
#include "mq/utils/Expected.h"
#include "mq/utils/MaybeOwnedString.h"

namespace mq {

class RpcClient {
public:
    enum class State {
        kClosed = static_cast<int>(MultiplexingRequester::State::kClosed),
        kOpened = static_cast<int>(MultiplexingRequester::State::kOpened),
    };

    RpcClient(const RpcClient &) = delete;
    RpcClient(RpcClient &&) = delete;

    RpcClient &operator=(const RpcClient &) = delete;
    RpcClient &operator=(RpcClient &&) = delete;

    RpcClient(EventLoop *loop, const Endpoint &remoteEndpoint);
    ~RpcClient();

    EventLoop *loop() const {
        return requester_.loop();
    }

    std::unique_ptr<Endpoint> remoteEndpoint() const {
        return requester_.remoteEndpoint();
    }

    void setMaxPendingRequests(size_t maxPendingRequests) {
        requester_.setMaxPendingRequests(maxPendingRequests);
    }

    void setRequestTimeout(std::chrono::nanoseconds requestTimeout) {
        requester_.setRequestTimeout(requestTimeout);
    }

    void setReconnectInterval(std::chrono::nanoseconds reconnectInterval) {
        requester_.setReconnectInterval(reconnectInterval);
    }

    void setMaxMessageLength(size_t maxMessageLength) {
        requester_.setMaxMessageLength(maxMessageLength);
    }

    void setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
        requester_.setRecvBufferMaxCapacity(recvBufferMaxCapacity);
    }

    void setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
        requester_.setSendBufferMaxCapacity(sendBufferMaxCapacity);
    }

    void setRecvChunkSize(size_t recvChunkSize) {
        requester_.setRecvChunkSize(recvChunkSize);
    }

    void setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
        requester_.setRecvTimeout(recvTimeout);
    }

    void setSendTimeout(std::chrono::nanoseconds sendTimeout) {
        requester_.setSendTimeout(sendTimeout);
    }

    void setNoDelay(bool noDelay) {
        requester_.setNoDelay(noDelay);
    }

    void setKeepAlive(KeepAlive keepAlive) {
        requester_.setKeepAlive(keepAlive);
    }

    State state() const {
        return static_cast<State>(requester_.state());
    }

    void open() {
        requester_.open();
    }

    int waitForConnected(std::chrono::nanoseconds timeout = {}) {
        return requester_.waitForConnected(timeout);
    }

    std::future<Expected<std::string, RpcError>> call(
        MaybeOwnedString methodName, MaybeOwnedString payload);

    std::future<Expected<std::string, RpcError>> call(
        MaybeOwnedString methodName, std::vector<MaybeOwnedString> pieces);

    size_t numPendingRequests() const {
        return requester_.numPendingRequests();
    }

    void close() {
        requester_.close();
    }

private:
    MultiplexingRequester requester_;
};

} // namespace mq
