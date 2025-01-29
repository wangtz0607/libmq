#pragma once

#include <chrono>
#include <format>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"
#include "mq/utils/Buffer.h"

namespace mq {

class FramingSocket : public std::enable_shared_from_this<FramingSocket> {
public:
    struct Params {
        size_t maxMessageLength;
        Socket::Params socket;

        Params() = default;

        Params(size_t maxMessageLength, Socket::Params socket)
            : maxMessageLength(maxMessageLength),
              socket(socket) {}

        bool operator==(const Params &) const = default;

        Params withMaxMessageLength(size_t maxMessageLength) const {
            return {maxMessageLength, socket};
        }

        Params withRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) const {
            return {maxMessageLength, socket.withRecvBufferMaxCapacity(recvBufferMaxCapacity)};
        }

        Params withSendBufferMaxCapacity(size_t sendBufferMaxCapacity) const {
            return {maxMessageLength, socket.withSendBufferMaxCapacity(sendBufferMaxCapacity)};
        }

        Params withRecvChunkSize(size_t recvChunkSize) const {
            return {maxMessageLength, socket.withRecvChunkSize(recvChunkSize)};
        }

        Params withSendTimeout(std::chrono::nanoseconds sendTimeout) const {
            return {maxMessageLength, socket.withSendTimeout(sendTimeout)};
        }

        Params withNoDelay(bool noDelay) const {
            return {maxMessageLength, socket.withNoDelay(noDelay)};
        }

        Params withKeepAliveOff() const {
            return {maxMessageLength, socket.withKeepAliveOff()};
        }

        Params withKeepAliveIdle(std::chrono::seconds idle) const {
            return {maxMessageLength, socket.withKeepAliveIdle(idle)};
        }

        Params withKeepAliveInterval(std::chrono::seconds interval) const {
            return {maxMessageLength, socket.withKeepAliveInterval(interval)};
        }

        Params withKeepAliveCount(int count) const {
            return {maxMessageLength, socket.withKeepAliveCount(count)};
        }
    };

    enum class State {
        kClosed,
        kConnecting,
        kConnected,
    };

    using ConnectCallback = std::move_only_function<bool (int error)>;
    using RecvCallback = std::move_only_function<bool (std::string_view message)>;
    using SendCompleteCallback = std::move_only_function<bool ()>;
    using CloseCallback = std::move_only_function<bool (int error)>;

    static Params defaultParams() {
        return Params()
            .withMaxMessageLength(8 * 1024 * 1024)
            .withRecvBufferMaxCapacity(16 * 1024 * 1024)
            .withSendBufferMaxCapacity(16 * 1024 * 1024)
            .withRecvChunkSize(4096)
            .withSendTimeout({})
            .withNoDelay(false)
            .withKeepAliveOff();
    }

    explicit FramingSocket(EventLoop *loop, Params params = defaultParams());
    ~FramingSocket();

    FramingSocket(const FramingSocket &) = delete;
    FramingSocket(FramingSocket &&) = delete;

    FramingSocket &operator=(const FramingSocket &) = delete;
    FramingSocket &operator=(FramingSocket &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    const Params &params() const {
        return params_;
    }

    State state() const;
    Socket &socket();
    const Socket &socket() const;
    std::unique_ptr<Endpoint> localEndpoint() const;
    std::unique_ptr<Endpoint> remoteEndpoint() const;

    bool hasConnectCallback() const;
    bool hasRecvCallback() const;
    bool hasSendCompleteCallback() const;
    bool hasCloseCallback() const;

    void addConnectCallback(ConnectCallback connectCallback);
    void addRecvCallback(RecvCallback recvCallback);
    void addSendCompleteCallback(SendCompleteCallback sendCompleteCallback);
    void addCloseCallback(CloseCallback closeCallback);

    void clearConnectCallbacks();
    void clearRecvCallbacks();
    void clearSendCompleteCallbacks();
    void clearCloseCallbacks();

    void dispatchConnect(int error);
    void dispatchRecv(std::string_view message);
    void dispatchSendComplete();
    void dispatchClose(int error);

    void open(const Endpoint &remoteEndpoint);
    void open(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint);
    int send(std::string_view message);
    int send(const std::vector<std::string_view> &messages);
    void close(int error = 0);
    void reset();

private:
    EventLoop *loop_;
    Params params_;
    State state_ = State::kClosed;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Endpoint> localEndpoint_;
    std::unique_ptr<Endpoint> remoteEndpoint_;
    std::vector<ConnectCallback> connectCallbacks_;
    std::vector<RecvCallback> recvCallbacks_;
    std::vector<SendCompleteCallback> sendCompleteCallbacks_;
    std::vector<CloseCallback> closeCallbacks_;

    bool onSocketRecv(Buffer &recvBuffer);
    bool onSocketSendComplete();
    bool onSocketClose(int error);
};

void enableAutoReconnectAndOpen(FramingSocket &socket,
                                const Endpoint &remoteEndpoint,
                                std::chrono::nanoseconds interval);

} // namespace mq

template <>
struct std::formatter<mq::FramingSocket::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::FramingSocket::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::FramingSocket::State state) {
        using enum mq::FramingSocket::State;
        switch (state) {
            case kClosed: return "CLOSED";
            case kConnecting: return "CONNECTING";
            case kConnected: return "CONNECTED";
            default: return nullptr;
        }
    }
};
