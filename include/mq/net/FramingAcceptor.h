// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <functional>
#include <memory>
#include <vector>

#include "mq/net/Acceptor.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"

namespace mq {

class FramingAcceptor {
public:
    enum class State {
        kClosed,
        kListening,
    };

    using AcceptCallback = std::function<bool (std::unique_ptr<FramingSocket> socket, const Endpoint &remoteEndpoint)>;

    explicit FramingAcceptor(EventLoop *loop);
    ~FramingAcceptor();

    FramingAcceptor(const FramingAcceptor &) = delete;
    FramingAcceptor(FramingAcceptor &&) = delete;

    FramingAcceptor &operator=(const FramingAcceptor &) = delete;
    FramingAcceptor &operator=(FramingAcceptor &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    void setMaxMessageLength(size_t maxMessageLength);
    void setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity);
    void setSendBufferMaxCapacity(size_t sendBufferMaxCapacity);
    void setRecvChunkSize(size_t recvChunkSize);
    void setRecvTimeout(std::chrono::nanoseconds recvTimeout);
    void setSendTimeout(std::chrono::nanoseconds sendTimeout);
    void setReuseAddr(bool reuseAddr);
    void setReusePort(bool reusePort);
    void setRcvBuf(int rcvBuf);
    void setSndBuf(int sndBuf);
    void setNoDelay(bool noDelay);
    void setKeepAlive(KeepAlive keepAlive);

    State state();
    Acceptor &acceptor();
    const Acceptor &acceptor() const;
    std::unique_ptr<Endpoint> localEndpoint() const;

    bool hasAcceptCallback() const;
    void addAcceptCallback(AcceptCallback acceptCallback);
    void clearAcceptCallbacks();
    void dispatchAccept(std::unique_ptr<FramingSocket> socket, const Endpoint &remoteEndpoint);

    int open(const Endpoint &localEndpoint);
    void close();
    void reset();

private:
    EventLoop *loop_;
    size_t maxMessageLength_ = 8 * 1024 * 1024;
    size_t recvBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t sendBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t recvChunkSize_ = 4096;
    std::chrono::nanoseconds recvTimeout_{};
    std::chrono::nanoseconds sendTimeout_{};
    bool reuseAddr_ = true;
    bool reusePort_ = true;
    int rcvBuf_ = -1;
    int sndBuf_ = -1;
    bool noDelay_ = false;
    KeepAlive keepAlive_{};
    State state_ = State::kClosed;
    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<Endpoint> localEndpoint_;
    std::vector<AcceptCallback> acceptCallbacks_;

    bool onAcceptorAccept(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint);
};

} // namespace mq

template <>
struct std::formatter<mq::FramingAcceptor::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::FramingAcceptor::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::FramingAcceptor::State state) {
        using enum mq::FramingAcceptor::State;

        switch (state) {
            case kClosed: return "Closed";
            case kListening: return "Listening";
            default: return nullptr;
        }
    }
};
