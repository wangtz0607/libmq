// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <cstddef>
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
    enum class State {
        kClosed,
        kListening,
    };

    using AcceptCallback =
        std::move_only_function<bool (std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint)>;

    explicit Acceptor(EventLoop *loop);
    ~Acceptor();

    Acceptor(const Acceptor &) = delete;
    Acceptor(Acceptor &&) = delete;

    Acceptor &operator=(const Acceptor &) = delete;
    Acceptor &operator=(Acceptor &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

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
    size_t recvBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t sendBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t recvChunkSize_ = 4096;
    std::chrono::nanoseconds recvTimeout_{};
    std::chrono::nanoseconds sendTimeout_{};
    bool reuseAddr_ = true;
    bool reusePort_ = true;
    bool noDelay_ = false;
    KeepAlive keepAlive_{};
    State state_ = State::kClosed;
    int fd_;
    std::unique_ptr<Watcher> watcher_;
    std::unique_ptr<Endpoint> localEndpoint_;
    std::vector<AcceptCallback> acceptCallbacks_;

    bool onWatcherReadReady();
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
            case kClosed: return "Closed";
            case kListening: return "Listening";
            default: return nullptr;
        }
    }
};
