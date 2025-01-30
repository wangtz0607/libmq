#pragma once

#include <chrono>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingAcceptor.h"
#include "mq/net/FramingSocket.h"
#include "mq/utils/Executor.h"
#include "mq/utils/PtrEqual.h"
#include "mq/utils/PtrHash.h"

namespace mq {

class Replier {
    using SocketSet = std::unordered_set<std::shared_ptr<FramingSocket>,
                                         PtrHash<std::shared_ptr<FramingSocket>>,
                                         PtrEqual<std::shared_ptr<FramingSocket>>>;

public:
    struct Params {
        size_t maxConnections;
        FramingAcceptor::Params framingAcceptor;

        Params() = default;

        Params(size_t maxConnections, FramingAcceptor::Params framingAcceptor)
            : maxConnections(maxConnections),
              framingAcceptor(framingAcceptor) {}

        bool operator==(const Params &) const = default;

        Params withMaxConnections(size_t maxConnections) const {
            return {maxConnections, framingAcceptor};
        }

        Params withReuseAddr(bool reuseAddr) const {
            return {maxConnections, framingAcceptor.withReuseAddr(reuseAddr)};
        }

        Params withMaxMessageLength(size_t maxMessageLength) const {
            return {maxConnections, framingAcceptor.withMaxMessageLength(maxMessageLength)};
        }

        Params withRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) const {
            return {maxConnections, framingAcceptor.withRecvBufferMaxCapacity(recvBufferMaxCapacity)};
        }

        Params withSendBufferMaxCapacity(size_t sendBufferMaxCapacity) const {
            return {maxConnections, framingAcceptor.withSendBufferMaxCapacity(sendBufferMaxCapacity)};
        }

        Params withRecvChunkSize(size_t recvChunkSize) const {
            return {maxConnections, framingAcceptor.withRecvChunkSize(recvChunkSize)};
        }

        Params withSendTimeout(std::chrono::nanoseconds sendTimeout) const {
            return {maxConnections, framingAcceptor.withSendTimeout(sendTimeout)};
        }

        Params withNoDelay(bool noDelay) const {
            return {maxConnections, framingAcceptor.withNoDelay(noDelay)};
        }

        Params withKeepAliveOff() const {
            return {maxConnections, framingAcceptor.withKeepAliveOff()};
        }

        Params withKeepAliveIdle(std::chrono::seconds idle) const {
            return {maxConnections, framingAcceptor.withKeepAliveIdle(idle)};
        }

        Params withKeepAliveInterval(std::chrono::seconds interval) const {
            return {maxConnections, framingAcceptor.withKeepAliveInterval(interval)};
        }

        Params withKeepAliveCount(int count) const {
            return {maxConnections, framingAcceptor.withKeepAliveCount(count)};
        }
    };

    enum class State {
        kClosed,
        kOpened,
    };

    using RecvCallback = std::move_only_function<std::string (const Endpoint &remoteEndpoint, std::string_view message)>;

    static Params defaultParams() {
        return Params()
            .withMaxConnections(512)
            .withReuseAddr(true)
            .withMaxMessageLength(8 * 1024 * 1024)
            .withRecvBufferMaxCapacity(16 * 1024 * 1024)
            .withSendBufferMaxCapacity(16 * 1024 * 1024)
            .withRecvChunkSize(4096)
            .withSendTimeout(std::chrono::seconds(5))
            .withNoDelay(true)
            .withKeepAliveIdle(std::chrono::seconds(120))
            .withKeepAliveInterval(std::chrono::seconds(20))
            .withKeepAliveCount(3);
    }

    Replier(EventLoop *loop, const Endpoint &localEndpoint, Params params = defaultParams());
    ~Replier();

    Replier(const Replier &) = delete;
    Replier(Replier &&) = delete;

    Replier &operator=(const Replier &) = delete;
    Replier &operator=(Replier &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    std::unique_ptr<Endpoint> localEndpoint() const {
        return localEndpoint_->clone();
    }

    const Params &params() {
        return params_;
    }

    void setRecvCallback(RecvCallback recvCallback);
    void setRecvCallbackExecutor(Executor *recvCallbackExecutor);
    std::optional<std::string> dispatchRecv(const Endpoint &remoteEndpoint, std::string_view message);

    State state() const;
    int open();

private:
    EventLoop *loop_;
    std::unique_ptr<Endpoint> localEndpoint_;
    Params params_;
    RecvCallback recvCallback_;
    Executor *recvCallbackExecutor_ = nullptr;
    State state_ = State::kClosed;
    std::unique_ptr<FramingAcceptor> acceptor_;
    SocketSet sockets_;

    bool onFramingAcceptorAccept(std::unique_ptr<FramingSocket> socket);
    bool onFramingSocketRecv(FramingSocket *socket, std::string_view message);
    bool onFramingSocketClose(FramingSocket *socket);
};

} // namespace mq

template <>
struct std::formatter<mq::Replier::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::Replier::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::Replier::State state) {
        using enum mq::Replier::State;
        switch (state) {
            case kClosed: return "CLOSED";
            case kOpened: return "OPENED";
            default: return nullptr;
        }
    }
};
