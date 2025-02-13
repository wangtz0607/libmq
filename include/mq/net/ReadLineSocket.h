// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"

namespace mq {

class ReadLineSocket {
public:
    enum class State {
        kClosed,
        kConnecting,
        kConnected,
    };

    using ConnectCallback = std::move_only_function<bool (int error)>;
    using RecvCallback = std::move_only_function<bool (std::string_view line)>;
    using SendCompleteCallback = std::move_only_function<bool ()>;
    using CloseCallback = std::move_only_function<bool (int error)>;

    explicit ReadLineSocket(EventLoop *loop);
    ~ReadLineSocket();

    ReadLineSocket(const ReadLineSocket &) = delete;
    ReadLineSocket(ReadLineSocket &&) = delete;

    ReadLineSocket &operator=(const ReadLineSocket &) = delete;
    ReadLineSocket &operator=(ReadLineSocket &&) = delete;

    EventLoop *loop() const {
        return loop_;
    }

    void setDelimiter(std::string delimiter);
    void setMaxLineLength(size_t maxLineLength);
    void setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity);
    void setSendBufferMaxCapacity(size_t sendBufferMaxCapacity);
    void setRecvChunkSize(size_t recvChunkSize);
    void setRecvTimeout(std::chrono::nanoseconds recvTimeout);
    void setSendTimeout(std::chrono::nanoseconds sendTimeout);
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
    void dispatchRecv(std::string_view line);
    void dispatchSendComplete();
    void dispatchClose(int error);

    void open(const Endpoint &remoteEndpoint);
    void open(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint);
    int send(const char *data, size_t size);
    int send(const std::vector<std::pair<const char *, size_t>> &buffers);
    void close(int error = 0);
    void reset();

private:
    EventLoop *loop_;
    std::string delimiter_ = "\n";
    size_t maxLineLength_ = 8 * 1024 * 1024;
    size_t recvBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t sendBufferMaxCapacity_ = 16 * 1024 * 1024;
    size_t recvChunkSize_ = 4096;
    std::chrono::nanoseconds recvTimeout_{};
    std::chrono::nanoseconds sendTimeout_{};
    bool noDelay_ = false;
    KeepAlive keepAlive_{};
    State state_ = State::kClosed;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Endpoint> localEndpoint_;
    std::unique_ptr<Endpoint> remoteEndpoint_;
    size_t pos_;
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
struct std::formatter<mq::ReadLineSocket::State> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::ReadLineSocket::State state, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(state));
    }

private:
    static constexpr const char *name(mq::ReadLineSocket::State state) {
        using enum mq::ReadLineSocket::State;

        switch (state) {
            case kClosed: return "Closed";
            case kConnecting: return "Connecting";
            case kConnected: return "Connected";
            default: return nullptr;
        }
    }
};
