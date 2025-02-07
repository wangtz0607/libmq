#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <string_view>

#include "mq/event/EventLoop.h"
#include "mq/message/Replier.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"
#include "mq/utils/Executor.h"

namespace mq {

class MultiplexingReplier {
public:
    enum class State {
        kClosed = static_cast<int>(Replier::State::kClosed),
        kOpened = static_cast<int>(Replier::State::kOpened),
    };

    using Promise = Replier::Promise;
    using RecvCallback = Replier::RecvCallback;

    MultiplexingReplier(EventLoop *loop, const Endpoint &localEndpoint);
    ~MultiplexingReplier();

    MultiplexingReplier(const MultiplexingReplier &) = delete;
    MultiplexingReplier(MultiplexingReplier &&) = delete;

    MultiplexingReplier &operator=(const MultiplexingReplier &) = delete;
    MultiplexingReplier &operator=(MultiplexingReplier &&) = delete;

    EventLoop *loop() const {
        return replier_.loop();
    }

    std::unique_ptr<Endpoint> localEndpoint() const {
        return replier_.localEndpoint();
    }

    void setMaxConnections(size_t maxConnections) {
        replier_.setMaxConnections(maxConnections);
    }

    void setReuseAddr(bool reuseAddr) {
        replier_.setReuseAddr(reuseAddr);
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

    void setRcvBuf(int rcvBuf) {
        replier_.setRcvBuf(rcvBuf);
    }

    void setSndBuf(int sndBuf) {
        replier_.setSndBuf(sndBuf);
    }

    void setNoDelay(bool noDelay) {
        replier_.setNoDelay(noDelay);
    }

    void setKeepAlive(KeepAlive keepAlive) {
        replier_.setKeepAlive(keepAlive);
    }

    void setRecvCallback(RecvCallback recvCallback);
    void setRecvCallbackExecutor(Executor *recvCallbackExecutor);

    State state() const {
        return static_cast<State>(replier_.state());
    }

    int open() {
        return replier_.open();
    }

    void close() {
        replier_.close();
    }

private:
    Replier replier_;
    RecvCallback recvCallback_;

    void onReplierRecv(const Endpoint &remoteEndpoint, std::string_view message, Replier::Promise promise);
};

} // namespace mq
