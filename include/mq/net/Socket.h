#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/event/Timer.h"
#include "mq/event/Watcher.h"
#include "mq/net/Endpoint.h"
#include "mq/utils/Buffer.h"

namespace mq {

struct KeepAlive {
    std::chrono::seconds idle;
    std::chrono::seconds interval;
    int count;

    KeepAlive()
        : idle(), interval(), count(0) {}

    KeepAlive(std::chrono::seconds idle, std::chrono::seconds interval, int count)
        : idle(idle), interval(interval), count(count) {}

    bool operator==(const KeepAlive &) const = default;

    operator bool() const {
        return idle.count() != 0 && interval.count() != 0 && count != 0;
    }
};

class Socket : public std::enable_shared_from_this<Socket> {
public:
    enum class State {
        kClosed,
        kConnecting,
        kConnected,
    };

    using ConnectCallback = std::move_only_function<bool (int error)>;
    using RecvCallback = std::move_only_function<bool (const char *data, size_t size, size_t &newSize)>;
    using SendCompleteCallback = std::move_only_function<bool ()>;
    using CloseCallback = std::move_only_function<bool (int error, const char *data, size_t size)>;

    explicit Socket(EventLoop *loop);
    ~Socket();

    Socket(const Socket &) = delete;
    Socket(Socket &&) = delete;

    Socket &operator=(const Socket &) = delete;
    Socket &operator=(Socket &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

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
    int fd() const;
    Watcher &watcher();
    const Watcher &watcher() const;
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
    void dispatchRecv(const char *data, size_t size, size_t &newSize);
    void dispatchSendComplete();
    void dispatchClose(int error, const char *data, size_t size);

    void open(const Endpoint &remoteEndpoint);
    void open(int fd, const Endpoint &remoteEndpoint);
    int send(const char *data, size_t size);
    int send(const std::vector<std::pair<const char *, size_t>> &buffers);
    void close(int error = 0);
    void reset();

private:
    EventLoop *loop_;
    size_t recvChunkSize_ = 4096;
    std::chrono::nanoseconds recvTimeout_{};
    std::chrono::nanoseconds sendTimeout_{};
    int rcvBuf_ = 212992;
    int sndBuf_ = 212992;
    bool noDelay_ = false;
    KeepAlive keepAlive_{};
    State state_ = State::kClosed;
    int fd_;
    std::unique_ptr<Watcher> watcher_;
    std::unique_ptr<Endpoint> localEndpoint_;
    std::unique_ptr<Endpoint> remoteEndpoint_;
    Buffer recvBuffer_;
    Buffer sendBuffer_;
    std::unique_ptr<Timer> recvTimer_;
    std::unique_ptr<Timer> sendTimer_;
    bool recvActive_ = false;
    bool sendActive_ = false;
    std::vector<ConnectCallback> connectCallbacks_;
    std::vector<RecvCallback> recvCallbacks_;
    std::vector<SendCompleteCallback> sendCompleteCallbacks_;
    std::vector<CloseCallback> closeCallbacks_;

    bool onWatcherReadReady();
    bool onWatcherWriteReady();
    bool onRecvTimerExpire();
    bool onSendTimerExpire();
};

void enableAutoReconnectAndOpen(Socket &socket,
                                const Endpoint &remoteEndpoint,
                                std::chrono::nanoseconds interval);

} // namespace mq

template <>
struct std::formatter<mq::Socket::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::Socket::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::Socket::State state) {
        using enum mq::Socket::State;
        switch (state) {
            case kClosed: return "CLOSED";
            case kConnecting: return "CONNECTING";
            case kConnected: return "CONNECTED";
            default: return nullptr;
        }
    }
};
