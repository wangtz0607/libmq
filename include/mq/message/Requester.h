#pragma once

#include <chrono>
#include <format>
#include <memory>
#include <string_view>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingSocket.h"
#include "mq/utils/ThreadPool.h"

namespace mq {

class Requester {
public:
    struct Params {
        std::chrono::nanoseconds reconnectInterval;
        FramingSocket::Params framingSocket;

        Params() = default;

        Params(std::chrono::nanoseconds reconnectInterval, FramingSocket::Params framingSocket)
            : reconnectInterval(reconnectInterval),
              framingSocket(framingSocket) {}

        bool operator==(const Params &) const = default;

        Params withReconnectInterval(std::chrono::nanoseconds reconnectInterval) const {
            return {reconnectInterval, framingSocket};
        }

        Params withMaxMessageLength(size_t maxMessageLength) const {
            return {reconnectInterval, framingSocket.withMaxMessageLength(maxMessageLength)};
        }

        Params withRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) const {
            return {reconnectInterval, framingSocket.withRecvBufferMaxCapacity(recvBufferMaxCapacity)};
        }

        Params withSendBufferMaxCapacity(size_t sendBufferMaxCapacity) const {
            return {reconnectInterval, framingSocket.withSendBufferMaxCapacity(sendBufferMaxCapacity)};
        }

        Params withRecvChunkSize(size_t recvChunkSize) const {
            return {reconnectInterval, framingSocket.withRecvChunkSize(recvChunkSize)};
        }

        Params withSendTimeout(std::chrono::nanoseconds sendTimeout) const {
            return {reconnectInterval, framingSocket.withSendTimeout(sendTimeout)};
        }

        Params withNoDelay(bool noDelay) const {
            return {reconnectInterval, framingSocket.withNoDelay(noDelay)};
        }

        Params withKeepAliveOff() const {
            return {reconnectInterval, framingSocket.withKeepAliveOff()};
        }

        Params withKeepAliveIdle(std::chrono::seconds idle) const {
            return {reconnectInterval, framingSocket.withKeepAliveIdle(idle)};
        }

        Params withKeepAliveInterval(std::chrono::seconds interval) const {
            return {reconnectInterval, framingSocket.withKeepAliveInterval(interval)};
        }

        Params withKeepAliveCount(int count) const {
            return {reconnectInterval, framingSocket.withKeepAliveCount(count)};
        }
    };

    enum class State {
        kClosed,
        kOpened,
    };

    using RecvCallback = std::move_only_function<void (std::string_view message)>;

    static Params defaultParams() {
        return Params()
            .withReconnectInterval(std::chrono::milliseconds(100))
            .withMaxMessageLength(8 * 1024 * 1024)
            .withRecvBufferMaxCapacity(16 * 1024 * 1024)
            .withSendBufferMaxCapacity(16 * 1024 * 1024)
            .withRecvChunkSize(4096)
            .withSendTimeout({})
            .withNoDelay(true)
            .withKeepAliveOff();
    }

    Requester(EventLoop *loop, ThreadPool *pool, const Endpoint &remoteEndpoint, Params params = defaultParams());
    ~Requester();

    Requester(const Requester &) = delete;
    Requester(Requester &&) = delete;

    Requester &operator=(const Requester &) = delete;
    Requester &operator=(Requester &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    ThreadPool *pool() const {
        return pool_;
    }

    std::unique_ptr<Endpoint> remoteEndpoint() const {
        return remoteEndpoint_->clone();
    }

    const Params &params() const {
        return params_;
    }

    void setRecvCallback(RecvCallback recvCallback);
    void dispatchRecv(std::string_view message);

    State state() const;
    void open();
    void send(std::string_view message);

private:
    EventLoop *loop_;
    ThreadPool *pool_;
    std::unique_ptr<Endpoint> remoteEndpoint_;
    Params params_;
    RecvCallback recvCallback_;
    State state_ = State::kClosed;
    std::unique_ptr<FramingSocket> socket_;

    bool onFramingSocketRecv(std::string_view message);
};

} // namespace mq

template <>
struct std::formatter<mq::Requester::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::Requester::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::Requester::State state) {
        using enum mq::Requester::State;
        switch (state) {
            case kClosed: return "CLOSED";
            case kOpened: return "OPENED";
            default: return nullptr;
        }
    }
};
