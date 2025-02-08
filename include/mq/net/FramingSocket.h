#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"

namespace mq {

class FramingSocket : public std::enable_shared_from_this<FramingSocket> {
public:
    enum class State {
        kClosed,
        kConnecting,
        kConnected,
    };

    using ConnectCallback = std::move_only_function<bool (int error)>;
    using RecvCallback = std::move_only_function<bool (std::string_view message)>;
    using SendCompleteCallback = std::move_only_function<bool ()>;
    using CloseCallback = std::move_only_function<bool (int error)>;

    explicit FramingSocket(EventLoop *loop);
    ~FramingSocket();

    FramingSocket(const FramingSocket &) = delete;
    FramingSocket(FramingSocket &&) = delete;

    FramingSocket &operator=(const FramingSocket &) = delete;
    FramingSocket &operator=(FramingSocket &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    void setMaxMessageLength(size_t maxMessageLength);
    void setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity);
    void setSendBufferMaxCapacity(size_t sendBufferMaxCapacity);
    void setRecvChunkSize(size_t recvChunkSize);
    void setRecvTimeout(std::chrono::nanoseconds recvTimeout);
    void setSendTimeout(std::chrono::nanoseconds sendTimeout);
    void setRcvBuf(int rcvBuf);
    void setSndBuf(int sndBuf);
    void setNoDelay(bool noDelay);
    void setKeepAlive(KeepAlive keepAlive);

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
    int send(const std::vector<std::string_view> &pieces);
    void close(int error = 0);
    void reset();

private:
    EventLoop *loop_;
    size_t maxMessageLength_ = 8 * 1024 * 1024;
    size_t recvBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t sendBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t recvChunkSize_ = 4096;
    std::chrono::nanoseconds recvTimeout_{};
    std::chrono::nanoseconds sendTimeout_{};
    int rcvBuf_ = 212992;
    int sndBuf_ = 212992;
    bool noDelay_ = false;
    KeepAlive keepAlive_{};
    State state_ = State::kClosed;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Endpoint> localEndpoint_;
    std::unique_ptr<Endpoint> remoteEndpoint_;
    std::vector<ConnectCallback> connectCallbacks_;
    std::vector<RecvCallback> recvCallbacks_;
    std::vector<SendCompleteCallback> sendCompleteCallbacks_;
    std::vector<CloseCallback> closeCallbacks_;

    bool onSocketRecv(const char *data, size_t size, size_t &newSize);
    bool onSocketSendComplete();
    bool onSocketClose(int error);
};

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
