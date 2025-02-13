// SPDX-License-Identifier: MIT

#include "mq/net/Acceptor.h"

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <utility>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"
#include "mq/net/Tcp6Endpoint.h"
#include "mq/net/TcpEndpoint.h"
#include "mq/net/UnixEndpoint.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "Acceptor"

using namespace mq;

Acceptor::Acceptor(EventLoop *loop) : loop_(loop) {
    LOG(debug, "");
}

Acceptor::~Acceptor() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(loop_->state() == EventLoop::State::kTask);
    CHECK(state_ == State::kClosed);
}

void Acceptor::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvBufferMaxCapacity_ = recvBufferMaxCapacity;
}

void Acceptor::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendBufferMaxCapacity_ = sendBufferMaxCapacity;
}

void Acceptor::setRecvChunkSize(size_t recvChunkSize) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvChunkSize_ = recvChunkSize;
}

void Acceptor::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvTimeout_ = recvTimeout;
}

void Acceptor::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendTimeout_ = sendTimeout;
}

void Acceptor::setReuseAddr(bool reuseAddr) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    reuseAddr_ = reuseAddr;
}

void Acceptor::setReusePort(bool reusePort) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    reusePort_ = reusePort;
}

void Acceptor::setNoDelay(bool noDelay) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    noDelay_ = noDelay;
}

void Acceptor::setKeepAlive(KeepAlive keepAlive) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    keepAlive_ = keepAlive;
}

Acceptor::State Acceptor::state() const {
    CHECK(loop_->isInLoopThread());

    return state_;
}

int Acceptor::fd() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return fd_;
}

Watcher &Acceptor::watcher() {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return *watcher_;
}

const Watcher &Acceptor::watcher() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return *watcher_;
}

std::unique_ptr<Endpoint> Acceptor::localEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return localEndpoint_->clone();
}

bool Acceptor::hasAcceptCallback() const {
    CHECK(loop_->isInLoopThread());

    return !acceptCallbacks_.empty();
}

void Acceptor::addAcceptCallback(AcceptCallback acceptCallback) {
    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.emplace_back(std::move(acceptCallback));
}

void Acceptor::clearAcceptCallbacks() {
    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.clear();
}

void Acceptor::dispatchAccept(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "remoteEndpoint={}", remoteEndpoint);

    CHECK(loop_->isInLoopThread());

    CHECK(acceptCallbacks_.size() == 1);

    AcceptCallback acceptCallback(std::move(acceptCallbacks_.front()));
    acceptCallbacks_.clear();

    if (acceptCallback(std::move(socket), remoteEndpoint)) {
        acceptCallbacks_.emplace_back(std::move(acceptCallback));
    }
}

int Acceptor::open(const Endpoint &localEndpoint) {
    LOG(debug, "localEndpoint={}", localEndpoint);

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    CHECK((fd_ = socket(localEndpoint.domain(), SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) >= 0);
    LOG(debug, "fd={}", fd_);

    if (reuseAddr_) {
        int optVal = 1;
        CHECK(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) == 0);
    }
    if (reusePort_) {
        int optVal = 1;
        CHECK(setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal)) == 0);
    }

    if (bind(fd_, localEndpoint.data(), localEndpoint.size()) < 0) {
        return errno;
    }

    if (listen(fd_, SOMAXCONN) < 0) {
        return errno;
    }

    watcher_ = std::make_unique<Watcher>(loop_, fd_);
    watcher_->registerSelf();
    watcher_->addReadReadyCallback([this] { return onWatcherReadReady(); });

    localEndpoint_ = localEndpoint.clone();

    State oldState = state_;
    state_ = State::kListening;
    LOG(debug, "{} -> {}", oldState, state_);

    LOG(info, "Listening on {}", *localEndpoint_);

    return 0;
}

void Acceptor::close() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(debug, "{} -> {}", oldState, state_);

    watcher_->clearReadReadyCallbacks();
    watcher_->clearWriteReadyCallbacks();

    loop_->post([watcher = std::move(watcher_), fd = fd_] {
        watcher->unregisterSelf();

        CHECK(::close(fd) == 0);
    });

    watcher_ = nullptr;

    localEndpoint_ = nullptr;
}

void Acceptor::reset() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    clearAcceptCallbacks();

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(debug, "{} -> {}", oldState, state_);

    watcher_->clearReadReadyCallbacks();
    watcher_->clearWriteReadyCallbacks();

    loop_->post([watcher = std::move(watcher_), fd = fd_] {
        watcher->unregisterSelf();

        CHECK(::close(fd) == 0);
    });

    watcher_ = nullptr;

    localEndpoint_ = nullptr;
}

bool Acceptor::onWatcherReadReady() {
    LOG(debug, "");

    int connFd;
    std::unique_ptr<Endpoint> remoteEndpoint;

    switch (localEndpoint()->domain()) {
        case AF_INET: {
            sockaddr_in addr;
            socklen_t addrLen = sizeof(addr);
            while ((connFd = accept4(fd_,
                                     reinterpret_cast<sockaddr *>(&addr),
                                     &addrLen,
                                     SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0 && errno == EINTR);
            CHECK(connFd >= 0);
            remoteEndpoint = std::make_unique<TcpEndpoint>(addr);
            break;
        }
        case AF_INET6: {
            sockaddr_in6 addr;
            socklen_t addrLen = sizeof(addr);
            while ((connFd = accept4(fd_,
                                     reinterpret_cast<sockaddr *>(&addr),
                                     &addrLen,
                                     SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0 && errno == EINTR);
            CHECK(connFd >= 0);
            remoteEndpoint = std::make_unique<Tcp6Endpoint>(addr);
            break;
        }
        case AF_UNIX: {
            sockaddr_un addr;
            socklen_t addrLen = sizeof(addr);
            while ((connFd = accept4(fd_,
                                     reinterpret_cast<sockaddr *>(&addr),
                                     &addrLen,
                                     SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0 && errno == EINTR);
            CHECK(connFd >= 0);
            remoteEndpoint = std::make_unique<UnixEndpoint>(addr, addrLen);
            break;
        }
        default:
            std::unreachable();
    }

    LOG(debug, "accept: connFd={}, remoteEndpoint={}", connFd, *remoteEndpoint);

    LOG(info, "Accepted connection from {}", *remoteEndpoint);

    std::unique_ptr<Socket> socket = std::make_unique<Socket>(loop_);

    socket->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
    socket->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
    socket->setRecvChunkSize(recvChunkSize_);
    socket->setRecvTimeout(recvTimeout_);
    socket->setSendTimeout(sendTimeout_);
    socket->setNoDelay(noDelay_);
    socket->setKeepAlive(keepAlive_);

    socket->open(connFd, *remoteEndpoint);

    dispatchAccept(std::move(socket), *remoteEndpoint);

    return true;
}
