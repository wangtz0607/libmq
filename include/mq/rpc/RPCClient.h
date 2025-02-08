#pragma once

#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingRequester.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"
#include "mq/rpc/RPCError.h"
#include "mq/utils/Expected.h"

namespace mq {

class RPCClient {
public:
    enum class State {
        kClosed = static_cast<int>(MultiplexingRequester::State::kClosed),
        kOpened = static_cast<int>(MultiplexingRequester::State::kOpened),
    };

    RPCClient(const RPCClient &) = delete;
    RPCClient(RPCClient &&) = delete;

    RPCClient &operator=(const RPCClient &) = delete;
    RPCClient &operator=(RPCClient &&) = delete;

    RPCClient(EventLoop *loop, const Endpoint &remoteEndpoint);
    ~RPCClient();

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

    void setRcvBuf(int rcvBuf) {
        requester_.setRcvBuf(rcvBuf);
    }

    void setSndBuf(int sndBuf) {
        requester_.setSndBuf(sndBuf);
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

    std::future<Expected<std::string, RPCError>> call(
        std::string_view methodName, std::string_view payload);

    std::future<Expected<std::string, RPCError>> call(
        std::string_view methodName, const std::vector<std::string_view> &pieces);

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
