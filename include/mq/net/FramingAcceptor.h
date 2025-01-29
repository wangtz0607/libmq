#pragma once

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
    struct Params {
        bool reuseAddr;
        FramingSocket::Params framingSocket;

        Params() = default;

        Params(bool reuseAddr, FramingSocket::Params framingSocket)
            : reuseAddr(reuseAddr), framingSocket(framingSocket) {}

        bool operator==(const Params &) const = default;

        Params withReuseAddr(bool reuseAddr) const {
            return {reuseAddr, framingSocket};
        }

        Params withMaxMessageLength(size_t maxMessageLength) const {
            return {reuseAddr, framingSocket.withMaxMessageLength(maxMessageLength)};
        }

        Params withRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) const {
            return {reuseAddr, framingSocket.withRecvBufferMaxCapacity(recvBufferMaxCapacity)};
        }

        Params withSendBufferMaxCapacity(size_t sendBufferMaxCapacity) const {
            return {reuseAddr, framingSocket.withSendBufferMaxCapacity(sendBufferMaxCapacity)};
        }

        Params withRecvChunkSize(size_t recvChunkSize) const {
            return {reuseAddr, framingSocket.withRecvChunkSize(recvChunkSize)};
        }

        Params withSendTimeout(std::chrono::nanoseconds sendTimeout) const {
            return {reuseAddr, framingSocket.withSendTimeout(sendTimeout)};
        }

        Params withNoDelay(bool noDelay) const {
            return {reuseAddr, framingSocket.withNoDelay(noDelay)};
        }

        Params withKeepAliveOff() const {
            return {reuseAddr, framingSocket.withKeepAliveOff()};
        }

        Params withKeepAliveIdle(std::chrono::seconds idle) const {
            return {reuseAddr, framingSocket.withKeepAliveIdle(idle)};
        }

        Params withKeepAliveInterval(std::chrono::seconds interval) const {
            return {reuseAddr, framingSocket.withKeepAliveInterval(interval)};
        }

        Params withKeepAliveCount(int count) const {
            return {reuseAddr, framingSocket.withKeepAliveCount(count)};
        }
    };

    enum class State {
        kClosed,
        kListening,
    };

    using AcceptCallback = std::function<bool (std::unique_ptr<FramingSocket> socket, const Endpoint &remoteEndpoint)>;

    static Params defaultParams() {
        return Params()
            .withReuseAddr(true)
            .withMaxMessageLength(8 * 1024 * 1024)
            .withRecvBufferMaxCapacity(16 * 1024 * 1024)
            .withSendBufferMaxCapacity(16 * 1024 * 1024)
            .withRecvChunkSize(4096)
            .withSendTimeout({})
            .withNoDelay(false)
            .withKeepAliveOff();
    }

    explicit FramingAcceptor(EventLoop *loop, Params params = defaultParams());
    ~FramingAcceptor();

    FramingAcceptor(const FramingAcceptor &) = delete;
    FramingAcceptor(FramingAcceptor &&) = delete;

    FramingAcceptor &operator=(const FramingAcceptor &) = delete;
    FramingAcceptor &operator=(FramingAcceptor &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    const Params &params() const {
        return params_;
    }

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
    Params params_;
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
            case kClosed: return "CLOSED";
            case kListening: return "LISTENING";
            default: return nullptr;
        }
    }
};
