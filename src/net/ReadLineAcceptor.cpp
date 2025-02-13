// SPDX-License-Identifier: MIT

#include "mq/net/ReadLineAcceptor.h"

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include "mq/event/EventLoop.h"
#include "mq/net/Acceptor.h"
#include "mq/net/Endpoint.h"
#include "mq/net/ReadLineSocket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "ReadLineAcceptor"

using namespace mq;

ReadLineAcceptor::ReadLineAcceptor(EventLoop *loop): loop_(loop) {
    LOG(debug, "");
}

ReadLineAcceptor::~ReadLineAcceptor() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(loop_->state() == EventLoop::State::kTask);
    CHECK(state_ == State::kClosed);
}

void ReadLineAcceptor::setDelimiter(std::string delimiter) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    delimiter_ = std::move(delimiter);
}

void ReadLineAcceptor::setMaxLineLength(size_t maxLineLength) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    maxLineLength_ = maxLineLength;
}

void ReadLineAcceptor::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvBufferMaxCapacity_ = recvBufferMaxCapacity;
}

void ReadLineAcceptor::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendBufferMaxCapacity_ = sendBufferMaxCapacity;
}

void ReadLineAcceptor::setRecvChunkSize(size_t recvChunkSize) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvChunkSize_ = recvChunkSize;
}

void ReadLineAcceptor::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvTimeout_ = recvTimeout;
}

void ReadLineAcceptor::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendTimeout_ = sendTimeout;
}

void ReadLineAcceptor::setReuseAddr(bool reuseAddr) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    reuseAddr_ = reuseAddr;
}

void ReadLineAcceptor::setReusePort(bool reusePort) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    reusePort_ = reusePort;
}

void ReadLineAcceptor::setNoDelay(bool noDelay) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    noDelay_ = noDelay;
}

void ReadLineAcceptor::setKeepAlive(KeepAlive keepAlive) {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    keepAlive_ = keepAlive;
}

ReadLineAcceptor::State ReadLineAcceptor::state() {
    CHECK(loop_->isInLoopThread());

    return state_;
}

Acceptor &ReadLineAcceptor::acceptor() {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return *acceptor_;
}

const Acceptor &ReadLineAcceptor::acceptor() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return *acceptor_;
}

std::unique_ptr<Endpoint> ReadLineAcceptor::localEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return acceptor_->localEndpoint();
}

bool ReadLineAcceptor::hasAcceptCallback() const {
    CHECK(loop_->isInLoopThread());

    return !acceptCallbacks_.empty();
}

void ReadLineAcceptor::addAcceptCallback(AcceptCallback acceptCallback) {
    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.emplace_back(std::move(acceptCallback));
}

void ReadLineAcceptor::clearAcceptCallbacks() {
    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.clear();
}

void ReadLineAcceptor::dispatchAccept(std::unique_ptr<ReadLineSocket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "remoteEndpoint={}", remoteEndpoint);

    CHECK(loop_->isInLoopThread());

    CHECK(acceptCallbacks_.size() == 1);

    AcceptCallback acceptCallback(std::move(acceptCallbacks_.front()));
    acceptCallbacks_.clear();

    if (acceptCallback(std::move(socket), remoteEndpoint)) {
        acceptCallbacks_.emplace_back(std::move(acceptCallback));
    }
}

int ReadLineAcceptor::open(const Endpoint &localEndpoint) {
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

void ReadLineAcceptor::close() {
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

void ReadLineAcceptor::reset() {
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

bool ReadLineAcceptor::onAcceptorAccept(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "");

    std::unique_ptr<ReadLineSocket> readLineSocket = std::make_unique<ReadLineSocket>(loop_);

    readLineSocket->setDelimiter(delimiter_);
    readLineSocket->setMaxLineLength(maxLineLength_);
    readLineSocket->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
    readLineSocket->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
    readLineSocket->setRecvChunkSize(recvChunkSize_);
    readLineSocket->setRecvTimeout(recvTimeout_);
    readLineSocket->setSendTimeout(sendTimeout_);
    readLineSocket->setNoDelay(noDelay_);
    readLineSocket->setKeepAlive(keepAlive_);

    readLineSocket->open(std::move(socket), remoteEndpoint);

    dispatchAccept(std::move(readLineSocket), remoteEndpoint);

    return true;
}
