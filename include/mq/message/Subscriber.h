#pragma once

#include <chrono>
#include <format>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Executor.h"
#include "mq/utils/IndirectEqual.h"
#include "mq/utils/IndirectHash.h"
#include "mq/utils/PtrEqual.h"
#include "mq/utils/PtrHash.h"

namespace mq {

class Subscriber {
    using SocketSet = std::unordered_set<std::shared_ptr<FramingSocket>,
                                         PtrHash<std::shared_ptr<FramingSocket>>,
                                         PtrEqual<std::shared_ptr<FramingSocket>>>;

    using EndpointToSocketMap = std::unordered_map<std::unique_ptr<Endpoint>,
                                                   FramingSocket *,
                                                   IndirectHash<std::unique_ptr<Endpoint>>,
                                                   IndirectEqual<std::unique_ptr<Endpoint>>>;

    using SocketToTopicsMap = std::unordered_map<FramingSocket *, std::vector<std::string>>;

public:
    enum class State {
        kClosed,
        kOpened,
    };

    using RecvCallback = std::move_only_function<void (const Endpoint &remoteEndpoint, std::string_view message)>;

    explicit Subscriber(EventLoop *loop);
    ~Subscriber();

    Subscriber(const Subscriber &) = delete;
    Subscriber(Subscriber &&) = delete;

    Subscriber &operator=(const Subscriber &) = delete;
    Subscriber &operator=(Subscriber &&) = delete;

    EventLoop *loop() const {
        return loop_;
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

    void setRecvCallback(RecvCallback recvCallback);
    void setRecvCallbackExecutor(Executor *recvCallbackExecutor);
    void dispatchRecv(const Endpoint &remoteEndpoint, std::string_view message);

    State state() const;
    void subscribe(const Endpoint &remoteEndpoint, std::vector<std::string> topics);
    void unsubscribe(const Endpoint &remoteEndpoint);

private:
    EventLoop *loop_;
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
    RecvCallback recvCallback_;
    Executor *recvCallbackExecutor_ = nullptr;
    State state_ = State::kClosed;
    SocketSet sockets_;
    EndpointToSocketMap endpointToSocket_;
    SocketToTopicsMap socketToTopics_;
    std::shared_ptr<char> flag_;

    bool onFramingSocketRecv(FramingSocket *socket, std::string_view message);
};

} // namespace mq

template <>
struct std::formatter<mq::Subscriber::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::Subscriber::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::Subscriber::State state) {
        using enum mq::Subscriber::State;
        switch (state) {
            case kClosed: return "CLOSED";
            case kOpened: return "OPENED";
            default: return nullptr;
        }
    }
};
