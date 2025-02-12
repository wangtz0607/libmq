// SPDX-License-Identifier: MIT

#include "mq/net/Socket.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "mq/event/EventLoop.h"
#include "mq/event/Timer.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Tcp6Endpoint.h"
#include "mq/net/TcpEndpoint.h"
#include "mq/net/UnixEndpoint.h"
#include "mq/utils/Buffer.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "Socket"

using namespace mq;

namespace {

std::unique_ptr<Endpoint> getSockName(int fd) {
    int optVal;
    socklen_t optLen = sizeof(optVal);
    CHECK(getsockopt(fd, SOL_SOCKET, SO_DOMAIN, &optVal, &optLen) == 0);

    switch (optVal) {
        case AF_INET: {
            sockaddr_in addr;
            socklen_t addrLen = sizeof(addr);
            CHECK(getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &addrLen) == 0);
            return std::make_unique<TcpEndpoint>(addr);
        }
        case AF_INET6: {
            sockaddr_in6 addr;
            socklen_t addrLen = sizeof(addr);
            CHECK(getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &addrLen) == 0);
            return std::make_unique<Tcp6Endpoint>(addr);
        }
        case AF_UNIX: {
            sockaddr_un addr;
            socklen_t addrLen = sizeof(addr);
            CHECK(getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &addrLen) == 0);
            return std::make_unique<UnixEndpoint>(addr, addrLen);
        }
        default:
            return nullptr;
    }
}

void setRcvBufSockOpt(int fd, int rcvBuf) {
    CHECK(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvBuf, sizeof(rcvBuf)) == 0);
}

void setSndBufSockOpt(int fd, int sndBuf) {
    CHECK(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndBuf, sizeof(sndBuf)) == 0);
}

void setNoDelaySockOpt(int fd, bool noDelay) {
    int optVal = noDelay;
    CHECK(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optVal, sizeof(optVal)) == 0);
}

void setKeepAliveSockOpt(int fd, const KeepAlive &keepAlive) {
    int optVal;

    if (keepAlive) {
        optVal = 1;
        CHECK(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optVal, sizeof(optVal)) == 0);

        optVal = keepAlive.idle.count();
        CHECK(setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &optVal, sizeof(optVal)) == 0);

        optVal = keepAlive.interval.count();
        CHECK(setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &optVal, sizeof(optVal)) == 0);

        optVal = keepAlive.count;
        CHECK(setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &optVal, sizeof(optVal)) == 0);
    } else {
        optVal = 0;
        CHECK(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optVal, sizeof(optVal)) == 0);
    }
}

} // namespace

Socket::Socket(EventLoop *loop)
    : loop_(loop),
      recvBuffer_(16 * 1024 * 1024),
      sendBuffer_(16 * 1024 * 1024) {
    LOG(debug, "");
}

Socket::~Socket() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(loop_->state() == EventLoop::State::kTask);
    CHECK(state_ == State::kClosed);
}

void Socket::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvBuffer_.setMaxCapacity(recvBufferMaxCapacity);
}

void Socket::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendBuffer_.setMaxCapacity(sendBufferMaxCapacity);
}

void Socket::setRecvChunkSize(size_t recvChunkSize) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvChunkSize_ = recvChunkSize;
}

void Socket::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvTimeout_ = recvTimeout;
}

void Socket::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendTimeout_ = sendTimeout;
}

void Socket::setRcvBuf(int rcvBuf) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    rcvBuf_ = rcvBuf;
}

void Socket::setSndBuf(int sndBuf) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sndBuf_ = sndBuf;
}

void Socket::setNoDelay(bool noDelay) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    noDelay_ = noDelay;
}

void Socket::setKeepAlive(KeepAlive keepAlive) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    keepAlive_ = keepAlive;
}

Socket::State Socket::state() const {
    CHECK(loop_->isInLoopThread());

    return state_;
}

int Socket::fd() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ != State::kClosed);

    return fd_;
}

Watcher &Socket::watcher() {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ != State::kClosed);

    return *watcher_;
}

const Watcher &Socket::watcher() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ != State::kClosed);

    return *watcher_;
}

std::unique_ptr<Endpoint> Socket::localEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kConnected);

    return localEndpoint_->clone();
}

std::unique_ptr<Endpoint> Socket::remoteEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kConnected);

    return remoteEndpoint_->clone();
}

bool Socket::hasConnectCallback() const {
    CHECK(loop_->isInLoopThread());

    return !connectCallbacks_.empty();
}

bool Socket::hasRecvCallback() const {
    CHECK(loop_->isInLoopThread());

    return !recvCallbacks_.empty();
}

bool Socket::hasSendCompleteCallback() const {
    CHECK(loop_->isInLoopThread());

    return !sendCompleteCallbacks_.empty();
}

bool Socket::hasCloseCallback() const {
    CHECK(loop_->isInLoopThread());

    return !closeCallbacks_.empty();
}

void Socket::addConnectCallback(ConnectCallback connectCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    connectCallbacks_.emplace_back(std::move(connectCallback));
}

void Socket::addRecvCallback(RecvCallback recvCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    recvCallbacks_.emplace_back(std::move(recvCallback));
}

void Socket::addSendCompleteCallback(SendCompleteCallback sendCompleteCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    sendCompleteCallbacks_.emplace_back(std::move(sendCompleteCallback));
}

void Socket::addCloseCallback(CloseCallback closeCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    closeCallbacks_.emplace_back(std::move(closeCallback));
}

void Socket::clearConnectCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    connectCallbacks_.clear();
}

void Socket::clearRecvCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    recvCallbacks_.clear();
}

void Socket::clearSendCompleteCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    sendCompleteCallbacks_.clear();
}

void Socket::clearCloseCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    closeCallbacks_.clear();
}

void Socket::dispatchConnect(int error) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    std::vector<ConnectCallback> connectCallbacks(std::move(connectCallbacks_));
    connectCallbacks_.clear();

    for (ConnectCallback &connectCallback : connectCallbacks) {
        if (connectCallback(error)) {
            connectCallbacks_.emplace_back(std::move(connectCallback));
        }
    }
}

void Socket::dispatchRecv(const char *data, size_t size, size_t &newSize) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    std::vector<RecvCallback> recvCallbacks(std::move(recvCallbacks_));
    recvCallbacks_.clear();

    for (RecvCallback &recvCallback : recvCallbacks) {
        if (recvCallback(data, size, newSize)) {
            recvCallbacks_.emplace_back(std::move(recvCallback));
        }
    }
}

void Socket::dispatchSendComplete() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    std::vector<SendCompleteCallback> sendCompleteCallbacks(std::move(sendCompleteCallbacks_));
    sendCompleteCallbacks_.clear();

    for (SendCompleteCallback &sendCompleteCallback : sendCompleteCallbacks) {
        if (sendCompleteCallback()) {
            sendCompleteCallbacks_.emplace_back(std::move(sendCompleteCallback));
        }
    }
}

void Socket::dispatchClose(int error, const char *data, size_t size) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    std::vector<CloseCallback> closeCallbacks(std::move(closeCallbacks_));
    closeCallbacks_.clear();

    for (CloseCallback &closeCallback : closeCallbacks) {
        if (closeCallback(error, data, size)) {
            closeCallbacks_.emplace_back(std::move(closeCallback));
        }
    }
}

void Socket::open(const Endpoint &remoteEndpoint) {
    LOG(debug, "remoteEndpoint={}", remoteEndpoint);

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    CHECK((fd_ = socket(remoteEndpoint.domain(), SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) >= 0);
    LOG(debug, "fd={}", fd_);

    if (rcvBuf_ >= 0) {
        setRcvBufSockOpt(fd_, rcvBuf_);
    }
    if (sndBuf_ >= 0) {
        setSndBufSockOpt(fd_, sndBuf_);
    }

    if (remoteEndpoint.domain() == AF_INET || remoteEndpoint.domain() == AF_INET6) {
        if (noDelay_) {
            setNoDelaySockOpt(fd_, noDelay_);
        }
        if (keepAlive_) {
            setKeepAliveSockOpt(fd_, keepAlive_);
        }
    }

    watcher_ = std::make_unique<Watcher>(loop_, fd_);
    watcher_->registerSelf();

    if (connect(fd_, remoteEndpoint.data(), remoteEndpoint.size()) == 0) {
        localEndpoint_ = getSockName(fd_);
        remoteEndpoint_ = remoteEndpoint.clone();

        State oldState = state_;
        state_ = State::kConnected;
        LOG(debug, "{} -> {}", oldState, state_);

        LOG(info, "Connected to {}", *remoteEndpoint_);

        dispatchConnect(0);

        watcher_->addReadReadyCallback([this] { return onWatcherReadReady(); });

        if (recvTimeout_.count() > 0) {
            recvTimer_ = std::make_unique<Timer>(loop_);
            recvTimer_->addExpireCallback([this] { return onRecvTimerExpire(); });
            recvTimer_->open();
            recvTimer_->setTime(recvTimeout_, recvTimeout_);
        }

        if (sendTimeout_.count() > 0) {
            sendTimer_ = std::make_unique<Timer>(loop_);
            sendTimer_->addExpireCallback([this] { return onSendTimerExpire(); });
            sendTimer_->open();
            sendTimer_->setTime(sendTimeout_, sendTimeout_);
        }
    } else {
        if (errno != EINPROGRESS) {
            LOG(warning, "connect: errno={}", strerrorname_np(errno));

            CHECK(errno != EINVAL);

            loop_->post([watcher = std::move(watcher_), fd = fd_]{
                watcher->unregisterSelf();

                CHECK(::close(fd) == 0);
            });

            watcher_ = nullptr;

            dispatchConnect(errno);
        } else {
            LOG(debug, "connect: errno={}", strerrorname_np(errno));

            State oldState = state_;
            state_ = State::kConnecting;
            LOG(debug, "{} -> {}", oldState, state_);

            watcher_->addWriteReadyCallback([this, remoteEndpoint = remoteEndpoint.clone()] {
                int optVal;
                socklen_t optLen = sizeof(optVal);
                CHECK(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optVal, &optLen) == 0);

                if (optVal == 0) {
                    localEndpoint_ = getSockName(fd_);
                    remoteEndpoint_ = remoteEndpoint->clone();

                    State oldState = state_;
                    state_ = State::kConnected;
                    LOG(debug, "{} -> {}", oldState, state_);

                    LOG(info, "Connected to {}", *remoteEndpoint_);

                    dispatchConnect(0);

                    watcher_->addReadReadyCallback([this] { return onWatcherReadReady(); });

                    if (recvTimeout_.count() > 0) {
                        recvTimer_ = std::make_unique<Timer>(loop_);
                        recvTimer_->addExpireCallback([this] { return onRecvTimerExpire(); });
                        recvTimer_->open();
                        recvTimer_->setTime(recvTimeout_, recvTimeout_);
                    }

                    if (sendTimeout_.count() > 0) {
                        sendTimer_ = std::make_unique<Timer>(loop_);
                        sendTimer_->addExpireCallback([this] { return onSendTimerExpire(); });
                        sendTimer_->open();
                        sendTimer_->setTime(sendTimeout_, sendTimeout_);
                    }
                } else {
                    LOG(warning, "connect: optVal={}", strerrorname_np(optVal));

                    State oldState = state_;
                    state_ = State::kClosed;
                    LOG(debug, "{} -> {}", oldState, state_);

                    loop_->post([watcher = std::move(watcher_), fd = fd_] {
                        watcher->unregisterSelf();

                        CHECK(::close(fd) == 0);
                    });

                    watcher_ = nullptr;

                    dispatchConnect(optVal);
                }

                return false;
            });
        }
    }
}

void Socket::open(int fd, const Endpoint &remoteEndpoint) {
    LOG(debug, "fd={}", fd);

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    int optVal;
    socklen_t optLen = sizeof(optVal);
    CHECK(getsockopt(fd, SOL_SOCKET, SO_TYPE, &optVal, &optLen) == 0);
    CHECK(optVal == SOCK_STREAM);

    if (remoteEndpoint.domain() == AF_INET || remoteEndpoint.domain() == AF_INET6) {
        if (noDelay_) {
            setNoDelaySockOpt(fd, noDelay_);
        }
        if (keepAlive_) {
            setKeepAliveSockOpt(fd, keepAlive_);
        }
    }

    fd_ = fd;

    watcher_ = std::make_unique<Watcher>(loop_, fd_);
    watcher_->registerSelf();

    localEndpoint_ = getSockName(fd_);
    remoteEndpoint_ = remoteEndpoint.clone();

    State oldState = state_;
    state_ = State::kConnected;
    LOG(debug, "{} -> {}", oldState, state_);

    dispatchConnect(0);

    watcher_->addReadReadyCallback([this] { return onWatcherReadReady(); });

    if (recvTimeout_.count() > 0) {
        recvTimer_ = std::make_unique<Timer>(loop_);
        recvTimer_->addExpireCallback([this] { return onRecvTimerExpire(); });
        recvTimer_->open();
        recvTimer_->setTime(recvTimeout_, recvTimeout_);
    }

    if (sendTimeout_.count() > 0) {
        sendTimer_ = std::make_unique<Timer>(loop_);
        sendTimer_->addExpireCallback([this] { return onSendTimerExpire(); });
        sendTimer_->open();
        sendTimer_->setTime(sendTimeout_, sendTimeout_);
    }
}

int Socket::send(const char *data, size_t size) {
    LOG(debug, "size={}", size);

    CHECK(loop_->isInLoopThread());

    if (state_ != State::kConnected) return ENOTCONN;

    if (size > 0) {
        if (sendBuffer_.maxCapacity() - sendBuffer_.size() < size) {
            return ENOBUFS;
        }

        if (sendBuffer_.empty()) {
            while (size > 0) {
                ssize_t n = ::send(fd_, data, size, MSG_NOSIGNAL);

                if (n >= 0) {
                    data += n;
                    size -= n;
                } else {
                    LOG(debug, "send: errno={}", strerrorname_np(errno));

                    if (errno == EINTR) continue;
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;

                    close(errno);

                    return 0;
                }
            }
        }

        if (size > 0) {
            sendBuffer_.extend(size);

            memcpy(sendBuffer_.data() + sendBuffer_.size() - size, data, size);

            if (sendBuffer_.size() == size) {
                watcher_->addWriteReadyCallback([this] { return onWatcherWriteReady(); });
            }
        }
    } else {
        dispatchSendComplete();
    }

    return 0;
}

int Socket::send(const std::vector<std::pair<const char *, size_t>> &buffers) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (state_ != State::kConnected) return ENOTCONN;

    size_t totalSize = 0;
    for (auto [data, size] : buffers) {
        totalSize += size;
    }

    if (totalSize > 0) {
        if (sendBuffer_.maxCapacity() - sendBuffer_.size() < totalSize) {
            return ENOBUFS;
        }

        sendBuffer_.reserve(totalSize);

        for (auto [data, size] : buffers) {
            sendBuffer_.extend(size);
            memcpy(sendBuffer_.data() + sendBuffer_.size() - size, data, size);
        }

        if (sendBuffer_.size() == totalSize) {
            watcher_->addWriteReadyCallback([this] { return onWatcherWriteReady(); });
        }
    } else {
        dispatchSendComplete();
    }

    return 0;
}

void Socket::close(int error) {
    LOG(debug, "error={}", strerrorname_np(error));

    CHECK(loop_->isInLoopThread());

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(debug, "{} -> {}", oldState, state_);

    LOG(info, "Closing connection to {}", *remoteEndpoint_);

    watcher_->clearReadReadyCallbacks();
    watcher_->clearWriteReadyCallbacks();

    if (recvTimeout_.count() > 0) {
        recvTimer_->reset();
    }

    if (sendTimeout_.count() > 0) {
        sendTimer_->reset();
    }

    loop_->post([recvTimer = std::move(recvTimer_),
                 sendTimer = std::move(sendTimer_),
                 watcher = std::move(watcher_),
                 fd = fd_] {
        watcher->unregisterSelf();

        CHECK(::close(fd) == 0);
    });

    watcher_ = nullptr;
    recvTimer_ = nullptr;
    sendTimer_ = nullptr;

    localEndpoint_ = nullptr;
    remoteEndpoint_ = nullptr;

    dispatchClose(error, sendBuffer_.data(), sendBuffer_.size());

    recvBuffer_.clear();
    sendBuffer_.clear();
}

void Socket::reset() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    recvBuffer_.clear();
    sendBuffer_.clear();

    clearConnectCallbacks();
    clearRecvCallbacks();
    clearSendCompleteCallbacks();
    clearCloseCallbacks();

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(debug, "{} -> {}", oldState, state_);

    watcher_->clearReadReadyCallbacks();
    watcher_->clearWriteReadyCallbacks();

    if (recvTimeout_.count() > 0) {
        recvTimer_->reset();
    }

    if (sendTimeout_.count() > 0) {
        sendTimer_->reset();
    }

    loop_->post([recvTimer = std::move(recvTimer_),
                 sendTimer = std::move(sendTimer_),
                 watcher = std::move(watcher_),
                 fd = fd_] {
        watcher->unregisterSelf();

        CHECK(::close(fd) == 0);
    });

    watcher_ = nullptr;
    recvTimer_ = nullptr;
    sendTimer_ = nullptr;

    localEndpoint_ = nullptr;
    remoteEndpoint_ = nullptr;
}

bool Socket::onWatcherReadReady() {
    LOG(debug, "");

    if (recvBuffer_.size() == recvBuffer_.maxCapacity()) {
        LOG(warning, "Recv buffer full");

        close(ENOBUFS);

        return false;
    }

    size_t chunkSize = std::min(recvChunkSize_, recvBuffer_.maxCapacity() - recvBuffer_.size());
    recvBuffer_.extend(chunkSize);

    ssize_t n = recv(fd_, recvBuffer_.data() + recvBuffer_.size() - chunkSize, chunkSize, 0);
    LOG(debug, "recv: n={}", n);

    if (n > 0) {
        recvBuffer_.retractBack(chunkSize - n);

        const char *data = recvBuffer_.data();
        size_t size = recvBuffer_.size();
        size_t newSize;

        dispatchRecv(data, size, newSize);

        if (newSize < size) {
            recvBuffer_.retractFront(size - newSize);
            recvActive_ = true;
        }
    } else {
        recvBuffer_.retractBack(chunkSize);

        if (n == 0) {
            close();

            return false;
        } else {
            LOG(debug, "recv: errno={}", strerrorname_np(errno));

            if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                close(errno);

                return false;
            }
        }
    }

    return true;
}

bool Socket::onWatcherWriteReady() {
    LOG(debug, "");

    if (!sendBuffer_.empty()) {
        ssize_t n = ::send(fd_, sendBuffer_.data(), sendBuffer_.size(), MSG_NOSIGNAL);
        LOG(debug, "send: n={}", n);

        if (n >= 0) {
            sendBuffer_.retractFront(n);
        } else {
            LOG(debug, "send: errno={}", strerrorname_np(errno));

            if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                close(errno);

                return false;
            }
        }
    }

    if (sendBuffer_.empty()) {
        dispatchSendComplete();
    }

    sendActive_ = true;

    return !sendBuffer_.empty();
}

bool Socket::onRecvTimerExpire() {
    LOG(debug, "");

    if (!recvBuffer_.empty() && !recvActive_) {
        LOG(warning, "Recv timed out");

        close(ETIMEDOUT);

        return false;
    }

    recvActive_ = false;

    return true;
}

bool Socket::onSendTimerExpire() {
    LOG(debug, "");

    if (!sendBuffer_.empty() && !sendActive_) {
        LOG(warning, "Send timed out");

        close(ETIMEDOUT);

        return false;
    }

    sendActive_ = false;

    return true;
}
