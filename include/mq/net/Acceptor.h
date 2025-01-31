#pragma once

#include <format>
#include <functional>
#include <memory>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/event/Watcher.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"

namespace mq {

class Acceptor {
public:
    struct Params {
        bool reuseAddr;
        Socket::Params socket;

        Params() = default;

        Params(bool reuseAddr, Socket::Params socket)
            : reuseAddr(reuseAddr), socket(socket) {}

        bool operator==(const Params &) const = default;

        Params withReuseAddr(bool reuseAddr) const {
            return {reuseAddr, socket};
        }

        Params withRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) const {
            return {reuseAddr, socket.withRecvBufferMaxCapacity(recvBufferMaxCapacity)};
        }

        Params withSendBufferMaxCapacity(size_t sendBufferMaxCapacity) const {
            return {reuseAddr, socket.withSendBufferMaxCapacity(sendBufferMaxCapacity)};
        }

        Params withRecvChunkSize(size_t recvChunkSize) const {
            return {reuseAddr, socket.withRecvChunkSize(recvChunkSize)};
        }

        Params withRecvTimeout(std::chrono::nanoseconds recvTimeout) const {
            return {reuseAddr, socket.withRecvTimeout(recvTimeout)};
        }

        Params withSendTimeout(std::chrono::nanoseconds sendTimeout) const {
            return {reuseAddr, socket.withSendTimeout(sendTimeout)};
        }

        Params withNoDelay(bool noDelay) const {
            return {reuseAddr, socket.withNoDelay(noDelay)};
        }

        Params withKeepAliveOff() const {
            return {reuseAddr, socket.withKeepAliveOff()};
        }

        Params withKeepAliveIdle(std::chrono::seconds idle) const {
            return {reuseAddr, socket.withKeepAliveIdle(idle)};
        }

        Params withKeepAliveInterval(std::chrono::seconds interval) const {
            return {reuseAddr, socket.withKeepAliveInterval(interval)};
        }

        Params withKeepAliveCount(int count) const {
            return {reuseAddr, socket.withKeepAliveCount(count)};
        }
    };

    enum class State {
        kClosed,
        kListening,
    };

    using AcceptCallback = std::function<bool (std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint)>;

    static Params defaultParams() {
        return Params()
            .withReuseAddr(true)
            .withRecvBufferMaxCapacity(16 * 1024 * 1024)
            .withSendBufferMaxCapacity(16 * 1024 * 1024)
            .withRecvChunkSize(4096)
            .withRecvTimeout({})
            .withSendTimeout({})
            .withNoDelay(false)
            .withKeepAliveOff();
    }

    explicit Acceptor(EventLoop *loop, Params params = defaultParams());
    ~Acceptor();

    Acceptor(const Acceptor &) = delete;
    Acceptor(Acceptor &&) = delete;

    Acceptor &operator=(const Acceptor &) = delete;
    Acceptor &operator=(Acceptor &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    const Params &params() const {
        return params_;
    }

    State state() const;
    int fd() const;
    Watcher &watcher();
    const Watcher &watcher() const;
    std::unique_ptr<Endpoint> localEndpoint() const;

    bool hasAcceptCallback() const;
    void addAcceptCallback(AcceptCallback acceptCallback);
    void clearAcceptCallbacks();
    void dispatchAccept(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint);

    int open(const Endpoint &localEndpoint);
    void close();
    void reset();

private:
    EventLoop *loop_;
    Params params_;
    State state_ = State::kClosed;
    int fd_;
    std::unique_ptr<Watcher> watcher_;
    std::unique_ptr<Endpoint> localEndpoint_;
    std::vector<AcceptCallback> acceptCallbacks_;

    bool onWatcherRead();
};

} // namespace mq

template <>
struct std::formatter<mq::Acceptor::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::Acceptor::State state, FormatContext &ctx) const {
        return format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::Acceptor::State state) {
        using enum mq::Acceptor::State;
        switch (state) {
            case kClosed: return "CLOSED";
            case kListening: return "LISTENING";
            default: return nullptr;
        }
    }
};
