// SPDX-License-Identifier: MIT

#include "mq/net/FramingAcceptor.h"

#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/net/Acceptor.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "FramingAcceptor"

using namespace mq;

FramingAcceptor::FramingAcceptor(EventLoop *loop): loop_(loop) {
    LOG(debug, "");
}

FramingAcceptor::~FramingAcceptor() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(loop_->state() == EventLoop::State::kTask);
    CHECK(state_ == State::kClosed);
}

void FramingAcceptor::setMaxMessageLength(size_t maxMessageLength) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    maxMessageLength_ = maxMessageLength;
}

void FramingAcceptor::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvBufferMaxCapacity_ = recvBufferMaxCapacity;
}

void FramingAcceptor::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendBufferMaxCapacity_ = sendBufferMaxCapacity;
}

void FramingAcceptor::setRecvChunkSize(size_t recvChunkSize) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvChunkSize_ = recvChunkSize;
}

void FramingAcceptor::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvTimeout_ = recvTimeout;
}

void FramingAcceptor::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendTimeout_ = sendTimeout;
}

void FramingAcceptor::setReuseAddr(bool reuseAddr) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    reuseAddr_ = reuseAddr;
}

void FramingAcceptor::setReusePort(bool reusePort) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    reusePort_ = reusePort;
}

void FramingAcceptor::setNoDelay(bool noDelay) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    noDelay_ = noDelay;
}

void FramingAcceptor::setKeepAlive(KeepAlive keepAlive) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    keepAlive_ = keepAlive;
}

FramingAcceptor::State FramingAcceptor::state() {
    CHECK(loop_->isInLoopThread());

    return state_;
}

Acceptor &FramingAcceptor::acceptor() {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return *acceptor_;
}

const Acceptor &FramingAcceptor::acceptor() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return *acceptor_;
}

std::unique_ptr<Endpoint> FramingAcceptor::localEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return localEndpoint_->clone();
}

bool FramingAcceptor::hasAcceptCallback() const {
    CHECK(loop_->isInLoopThread());

    return !acceptCallbacks_.empty();
}

void FramingAcceptor::addAcceptCallback(AcceptCallback acceptCallback) {
    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.emplace_back(std::move(acceptCallback));
}

void FramingAcceptor::clearAcceptCallbacks() {
    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.clear();
}

void FramingAcceptor::dispatchAccept(std::unique_ptr<FramingSocket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "remoteEndpoint={}", remoteEndpoint);

    CHECK(loop_->isInLoopThread());

    CHECK(acceptCallbacks_.size() == 1);

    AcceptCallback acceptCallback(std::move(acceptCallbacks_.front()));
    acceptCallbacks_.clear();

    if (acceptCallback(std::move(socket), remoteEndpoint)) {
        acceptCallbacks_.emplace_back(std::move(acceptCallback));
    }
}

int FramingAcceptor::open(const Endpoint &localEndpoint) {
    LOG(debug, "localEndpoint={}", localEndpoint);

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    acceptor_ = std::make_unique<Acceptor>(loop_);

    acceptor_->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
    acceptor_->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
    acceptor_->setRecvChunkSize(recvChunkSize_);
    acceptor_->setRecvTimeout(recvTimeout_);
    acceptor_->setSendTimeout(sendTimeout_);
    acceptor_->setReuseAddr(reuseAddr_);
    acceptor_->setReusePort(reusePort_);
    acceptor_->setNoDelay(noDelay_);
    acceptor_->setKeepAlive(keepAlive_);

    acceptor_->addAcceptCallback([this](std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint) {
        return onAcceptorAccept(std::move(socket), remoteEndpoint);
    });

    if (int error = acceptor_->open(localEndpoint)) {
        return error;
    }

    localEndpoint_ = acceptor_->localEndpoint();

    State oldState = state_;
    state_ = State::kListening;
    LOG(debug, "{} -> {}", oldState, state_);

    return 0;
}

void FramingAcceptor::close() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(debug, "{} -> {}", oldState, state_);

    acceptor_->reset();

    loop_->post([acceptor = std::move(acceptor_)] {});

    acceptor_ = nullptr;

    localEndpoint_ = nullptr;
}

void FramingAcceptor::reset() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    clearAcceptCallbacks();

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(debug, "{} -> {}", oldState, state_);

    acceptor_->reset();

    loop_->post([acceptor = std::move(acceptor_)] {});

    acceptor_ = nullptr;

    localEndpoint_ = nullptr;
}

bool FramingAcceptor::onAcceptorAccept(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "");

    std::unique_ptr<FramingSocket> framingSocket = std::make_unique<FramingSocket>(loop_);

    framingSocket->setMaxMessageLength(maxMessageLength_);
    framingSocket->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
    framingSocket->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
    framingSocket->setRecvChunkSize(recvChunkSize_);
    framingSocket->setRecvTimeout(recvTimeout_);
    framingSocket->setSendTimeout(sendTimeout_);
    framingSocket->setNoDelay(noDelay_);
    framingSocket->setKeepAlive(keepAlive_);

    framingSocket->open(std::move(socket), remoteEndpoint);

    dispatchAccept(std::move(framingSocket), remoteEndpoint);

    return true;
}
