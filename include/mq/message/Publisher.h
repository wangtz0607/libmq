// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <memory>
#include <unordered_set>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingAcceptor.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/MaybeOwnedString.h"
#include "mq/utils/PtrEqual.h"
#include "mq/utils/PtrHash.h"

namespace mq {

class Publisher {
public:
    enum class State {
        kClosed,
        kOpened,
    };

    Publisher(EventLoop *loop, const Endpoint &localEndpoint);
    ~Publisher();

    Publisher(const Publisher &) = delete;
    Publisher(Publisher &&) = delete;

    Publisher &operator=(const Publisher &) = delete;
    Publisher &operator=(Publisher &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    std::unique_ptr<Endpoint> localEndpoint() const {
        return localEndpoint_->clone();
    }

    void setMaxConnections(size_t maxConnections);
    void setMaxMessageLength(size_t maxMessageLength);
    void setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity);
    void setSendBufferMaxCapacity(size_t sendBufferMaxCapacity);
    void setRecvChunkSize(size_t recvChunkSize);
    void setRecvTimeout(std::chrono::nanoseconds recvTimeout);
    void setSendTimeout(std::chrono::nanoseconds sendTimeout);
    void setReuseAddr(bool reuseAddr);
    void setReusePort(bool reusePort);
    void setNoDelay(bool noDelay);
    void setKeepAlive(KeepAlive keepAlive);

    State state() const;
    int open();
    void send(MaybeOwnedString message);
    void send(std::vector<MaybeOwnedString> pieces);
    void close();

private:
    using SocketSet = std::unordered_set<std::shared_ptr<FramingSocket>,
                                         PtrHash<std::shared_ptr<FramingSocket>>,
                                         PtrEqual<std::shared_ptr<FramingSocket>>>;

    EventLoop *loop_;
    std::unique_ptr<Endpoint> localEndpoint_;
    size_t maxConnections_ = 512;
    size_t maxMessageLength_ = 8 * 1024 * 1024;
    size_t recvBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t sendBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t recvChunkSize_ = 4096;
    std::chrono::nanoseconds recvTimeout_ = std::chrono::seconds(30);
    std::chrono::nanoseconds sendTimeout_ = std::chrono::seconds(30);
    bool reuseAddr_ = true;
    bool reusePort_ = true;
    bool noDelay_ = true;
    KeepAlive keepAlive_{std::chrono::seconds(120), std::chrono::seconds(20), 3};
    State state_ = State::kClosed;
    std::unique_ptr<FramingAcceptor> acceptor_;
    SocketSet sockets_;
    std::shared_ptr<void> token_;

    bool onFramingAcceptorAccept(std::unique_ptr<FramingSocket> socket);
    bool onFramingSocketClose(FramingSocket *socket);
};

} // namespace mq

template <>
struct std::formatter<mq::Publisher::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::Publisher::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::Publisher::State state) {
        using enum mq::Publisher::State;

        switch (state) {
            case kClosed: return "Closed";
            case kOpened: return "Opened";
            default: return nullptr;
        }
    }
};
