#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/message/Requester.h"
#include "mq/net/Endpoint.h"
#include "mq/utils/Executor.h"

namespace mq {

class MultiplexingRequester {
public:
    enum class State {
        kClosed = static_cast<int>(Requester::State::kClosed),
        kOpened = static_cast<int>(Requester::State::kOpened),
    };

    using ConnectCallback = Requester::ConnectCallback;
    using RecvCallback = Requester::RecvCallback;

    MultiplexingRequester(EventLoop *loop, const Endpoint &remoteEndpoint);

    MultiplexingRequester(const MultiplexingRequester &) = delete;
    MultiplexingRequester(MultiplexingRequester &&) = delete;

    MultiplexingRequester &operator=(const MultiplexingRequester &) = delete;
    MultiplexingRequester &operator=(MultiplexingRequester &&) = delete;

    EventLoop *loop() const {
        return requester_.loop();
    }

    std::unique_ptr<Endpoint> remoteEndpoint() const {
        return requester_.remoteEndpoint();
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

    void setConnectCallback(ConnectCallback connectCallback) {
        requester_.setConnectCallback(std::move(connectCallback));
    }

    State state() const {
        return static_cast<State>(requester_.state());
    }

    void open() {
        requester_.open();
    }

    void send(std::string message, RecvCallback recvCallback, Executor *recvCallbackExecutor = nullptr);

private:
    Requester requester_;
    std::unordered_map<uint64_t, std::pair<RecvCallback, Executor *>> requests_;

    void onRequesterRecv(std::string_view message);

    static std::atomic<uint64_t> nextRequestId_;
};

} // namespace mq
