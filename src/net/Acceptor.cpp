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
#include "mq/net/TCPV4Endpoint.h"
#include "mq/net/TCPV6Endpoint.h"
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

void Acceptor::setReuseAddr(bool reuseAddr) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    reuseAddr_ = reuseAddr;
}

void Acceptor::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvBufferMaxCapacity_ = recvBufferMaxCapacity;
}

void Acceptor::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendBufferMaxCapacity_ = sendBufferMaxCapacity;
}

void Acceptor::setRecvChunkSize(size_t recvChunkSize) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvChunkSize_ = recvChunkSize;
}

void Acceptor::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvTimeout_ = recvTimeout;
}

void Acceptor::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendTimeout_ = sendTimeout;
}

void Acceptor::setRcvBuf(int rcvBuf) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    rcvBuf_ = rcvBuf;
}

void Acceptor::setSndBuf(int sndBuf) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sndBuf_ = sndBuf;
}

void Acceptor::setNoDelay(bool noDelay) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    noDelay_ = noDelay;
}

void Acceptor::setKeepAlive(KeepAlive keepAlive) {
    LOG(debug, "");

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
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.emplace_back(std::move(acceptCallback));
}

void Acceptor::clearAcceptCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.clear();
}

void Acceptor::dispatchAccept(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "");

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
            remoteEndpoint = std::make_unique<TCPV4Endpoint>(addr);
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
            remoteEndpoint = std::make_unique<TCPV6Endpoint>(addr);
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
    }

    LOG(debug, "accept: connFd={}, remoteEndpoint={}", connFd, *remoteEndpoint);

    LOG(info, "Accepted connection from {}", *remoteEndpoint);

    std::unique_ptr<Socket> socket = std::make_unique<Socket>(loop_);

    socket->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
    socket->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
    socket->setRecvChunkSize(recvChunkSize_);
    socket->setRecvTimeout(recvTimeout_);
    socket->setSendTimeout(sendTimeout_);
    socket->setRcvBuf(rcvBuf_);
    socket->setSndBuf(sndBuf_);
    socket->setNoDelay(noDelay_);
    socket->setKeepAlive(keepAlive_);

    socket->open(connFd, *remoteEndpoint);

    dispatchAccept(std::move(socket), *remoteEndpoint);

    return true;
}
