#pragma once

#include <chrono>
#include <format>
#include <functional>
#include <memory>
#include <string_view>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Executor.h"

namespace mq {

class Requester {
public:
    enum class State {
        kClosed,
        kOpened,
    };

    using ConnectCallback = std::move_only_function<void ()>;
    using RecvCallback = std::move_only_function<void (std::string_view message)>;

    Requester(EventLoop *loop, const Endpoint &remoteEndpoint);
    ~Requester();

    Requester(const Requester &) = delete;
    Requester(Requester &&) = delete;

    Requester &operator=(const Requester &) = delete;
    Requester &operator=(Requester &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    std::unique_ptr<Endpoint> remoteEndpoint() const {
        return remoteEndpoint_->clone();
    }

    void setReconnectInterval(std::chrono::nanoseconds reconnectInterval);
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

    void setConnectCallback(ConnectCallback connectCallback);
    void setRecvCallback(RecvCallback recvCallback);

    void setConnectCallbackExecutor(Executor *connectCallbackExecutor);
    void setRecvCallbackExecutor(Executor *recvCallbackExecutor);

    void dispatchConnect();
    void dispatchRecv(std::string_view message);

    State state() const;
    void open();
    int waitForConnected(std::chrono::nanoseconds timeout = {});
    void send(std::string_view message);

private:
    EventLoop *loop_;
    std::unique_ptr<Endpoint> remoteEndpoint_;
    std::chrono::nanoseconds reconnectInterval_ = std::chrono::milliseconds(100);
    size_t maxMessageLength_ = 8 * 1024 * 1024;
    size_t recvBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t sendBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t recvChunkSize_ = 4096;
    std::chrono::nanoseconds recvTimeout_{};
    std::chrono::nanoseconds sendTimeout_{};
    int rcvBuf_ = 212992;
    int sndBuf_ = 212992;
    bool noDelay_ = true;
    KeepAlive keepAlive_{};
    ConnectCallback connectCallback_;
    RecvCallback recvCallback_;
    Executor *connectCallbackExecutor_ = nullptr;
    Executor *recvCallbackExecutor_ = nullptr;
    State state_ = State::kClosed;
    std::unique_ptr<FramingSocket> socket_;

    bool onFramingSocketConnect(int error);
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
