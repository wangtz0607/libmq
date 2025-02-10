#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <functional>
#include <memory>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingAcceptor.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Executor.h"
#include "mq/utils/MaybeOwnedString.h"
#include "mq/utils/PtrEqual.h"
#include "mq/utils/PtrHash.h"

namespace mq {

class Replier {
    using SocketSet = std::unordered_set<std::shared_ptr<FramingSocket>,
                                         PtrHash<std::shared_ptr<FramingSocket>>,
                                         PtrEqual<std::shared_ptr<FramingSocket>>>;

public:
    enum class State {
        kClosed,
        kOpened,
    };

    class Promise {
    public:
        void operator()(MaybeOwnedString replyMessage);
        void operator()(std::vector<MaybeOwnedString> replyPieces);

    private:
        Replier *replier_;
        std::shared_ptr<FramingSocket> socket_;
        std::weak_ptr<void> token_;

        Promise(Replier *replier, std::shared_ptr<FramingSocket> socket, std::weak_ptr<void> token)
            : replier_(replier), socket_(std::move(socket)), token_(std::move(token)) {}

        friend class Replier;
    };

    using RecvCallback =
        std::move_only_function<void (const Endpoint &remoteEndpoint, std::string_view message, Promise promise)>;

    Replier(EventLoop *loop, const Endpoint &localEndpoint);
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

    void setMaxConnections(size_t maxConnections);
    void setReuseAddr(bool reuseAddr);
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
    void dispatchRecv(const Endpoint &remoteEndpoint, std::string_view message, Promise promise);

    State state() const;
    int open();
    void close();

private:
    EventLoop *loop_;
    std::unique_ptr<Endpoint> localEndpoint_;
    size_t maxConnections_ = 512;
    bool reuseAddr_ = true;
    size_t maxMessageLength_ = 8 * 1024 * 1024;
    size_t recvBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t sendBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t recvChunkSize_ = 4096;
    std::chrono::nanoseconds recvTimeout_ = std::chrono::seconds(5);
    std::chrono::nanoseconds sendTimeout_ = std::chrono::seconds(5);
    int rcvBuf_ = 212992;
    int sndBuf_ = 212992;
    bool noDelay_ = true;
    KeepAlive keepAlive_{std::chrono::seconds(120), std::chrono::seconds(20), 30};
    RecvCallback recvCallback_;
    Executor *recvCallbackExecutor_ = nullptr;
    State state_ = State::kClosed;
    std::unique_ptr<FramingAcceptor> acceptor_;
    SocketSet sockets_;
    std::shared_ptr<void> token_;

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
