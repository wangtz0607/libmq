#include "mq/net/Acceptor.h"

#include <cerrno>
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

Acceptor::Acceptor(EventLoop *loop, Params params)
    : loop_(loop), params_(params) {
    LOG(debug, "");
}

Acceptor::~Acceptor() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(loop_->state() == EventLoop::State::kTask);
    CHECK(state_ == State::kClosed);
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

    if (params_.reuseAddr) {
        int optVal = 1;
        CHECK(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) == 0);
    }

    if (bind(fd_, localEndpoint.addr(), localEndpoint.addrLen()) < 0) {
        return errno;
    }

    if (listen(fd_, SOMAXCONN) < 0) {
        return errno;
    }

    watcher_ = std::make_unique<Watcher>(loop_, fd_);
    watcher_->registerSelf();
    watcher_->addReadCallback([this] { return onWatcherRead(); });

    localEndpoint_ = localEndpoint.clone();

    State oldState = state_;
    state_ = State::kListening;
    LOG(info, "{} -> {}", oldState, state_);

    return 0;
}

void Acceptor::close() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(info, "{} -> {}", oldState, state_);

    watcher_->clearReadCallbacks();
    watcher_->clearWriteCallbacks();

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
    LOG(info, "{} -> {}", oldState, state_);

    watcher_->clearReadCallbacks();
    watcher_->clearWriteCallbacks();

    loop_->post([watcher = std::move(watcher_), fd = fd_] {
        watcher->unregisterSelf();

        CHECK(::close(fd) == 0);
    });

    watcher_ = nullptr;

    localEndpoint_ = nullptr;
}

bool Acceptor::onWatcherRead() {
    LOG(debug, "");

    int connFd;
    std::unique_ptr<Endpoint> remoteEndpoint;

    switch (localEndpoint()->domain()) {
        case AF_INET: {
            struct sockaddr_in addr;
            socklen_t addrLen = sizeof(addr);
            while ((connFd = accept4(fd_,
                                     reinterpret_cast<struct sockaddr *>(&addr),
                                     &addrLen,
                                     SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0 && errno == EINTR);
            CHECK(connFd >= 0);
            remoteEndpoint = std::make_unique<TCPV4Endpoint>(addr);
            break;
        }
        case AF_INET6: {
            struct sockaddr_in6 addr;
            socklen_t addrLen = sizeof(addr);
            while ((connFd = accept4(fd_,
                                     reinterpret_cast<struct sockaddr *>(&addr),
                                     &addrLen,
                                     SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0 && errno == EINTR);
            CHECK(connFd >= 0);
            remoteEndpoint = std::make_unique<TCPV6Endpoint>(addr);
            break;
        }
        case AF_UNIX: {
            while ((connFd = accept4(fd_,
                                     nullptr,
                                     nullptr,
                                     SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0 && errno == EINTR);
            CHECK(connFd >= 0);
            remoteEndpoint = std::make_unique<UnixEndpoint>("");
            break;
        }
    }

    LOG(debug, "accept: connFd={}, remoteEndpoint={}", connFd, *remoteEndpoint);

    std::unique_ptr<Socket> socket = std::make_unique<Socket>(loop_, params_.socket);
    socket->open(connFd, *remoteEndpoint);

    dispatchAccept(std::move(socket), *remoteEndpoint);

    return true;
}
