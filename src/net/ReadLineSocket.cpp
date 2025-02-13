// SPDX-License-Identifier: MIT

#include "mq/net/ReadLineSocket.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "ReadLineSocket"

using namespace mq;

ReadLineSocket::ReadLineSocket(EventLoop *loop) : loop_(loop) {
    LOG(debug, "");
}

ReadLineSocket::~ReadLineSocket() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(loop_->state() == EventLoop::State::kTask);
    CHECK(state_ == State::kClosed);
}

void ReadLineSocket::setDelimiter(std::string delimiter) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    delimiter_ = std::move(delimiter);
}

void ReadLineSocket::setMaxLineLength(size_t maxLineLength) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    maxLineLength_ = maxLineLength;
}

void ReadLineSocket::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvBufferMaxCapacity_ = recvBufferMaxCapacity;
}

void ReadLineSocket::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendBufferMaxCapacity_ = sendBufferMaxCapacity;
}

void ReadLineSocket::setRecvChunkSize(size_t recvChunkSize) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvChunkSize_ = recvChunkSize;
}

void ReadLineSocket::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvTimeout_ = recvTimeout;
}

void ReadLineSocket::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendTimeout_ = sendTimeout;
}

void ReadLineSocket::setNoDelay(bool noDelay) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    noDelay_ = noDelay;
}

void ReadLineSocket::setKeepAlive(KeepAlive keepAlive) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    keepAlive_ = keepAlive;
}

ReadLineSocket::State ReadLineSocket::state() const {
    CHECK(loop_->isInLoopThread());

    return state_;
}

Socket &ReadLineSocket::socket() {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ != State::kClosed);

    return *socket_;
}

const Socket &ReadLineSocket::socket() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ != State::kClosed);

    return *socket_;
}

std::unique_ptr<Endpoint> ReadLineSocket::localEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kConnected);

    return localEndpoint_->clone();
}

std::unique_ptr<Endpoint> ReadLineSocket::remoteEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kConnected);

    return remoteEndpoint_->clone();
}

bool ReadLineSocket::hasConnectCallback() const {
    CHECK(loop_->isInLoopThread());

    return !connectCallbacks_.empty();
}

bool ReadLineSocket::hasRecvCallback() const {
    CHECK(loop_->isInLoopThread());

    return !recvCallbacks_.empty();
}

bool ReadLineSocket::hasSendCompleteCallback() const {
    CHECK(loop_->isInLoopThread());

    return !sendCompleteCallbacks_.empty();
}

bool ReadLineSocket::hasCloseCallback() const {
    CHECK(loop_->isInLoopThread());

    return !closeCallbacks_.empty();
}

void ReadLineSocket::addConnectCallback(ConnectCallback connectCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    connectCallbacks_.emplace_back(std::move(connectCallback));
}

void ReadLineSocket::addRecvCallback(RecvCallback recvCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    recvCallbacks_.emplace_back(std::move(recvCallback));
}

void ReadLineSocket::addSendCompleteCallback(SendCompleteCallback sendCompleteCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    sendCompleteCallbacks_.emplace_back(std::move(sendCompleteCallback));
}

void ReadLineSocket::addCloseCallback(CloseCallback closeCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    closeCallbacks_.emplace_back(std::move(closeCallback));
}

void ReadLineSocket::clearConnectCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    connectCallbacks_.clear();
}

void ReadLineSocket::clearRecvCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    recvCallbacks_.clear();
}

void ReadLineSocket::clearSendCompleteCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    sendCompleteCallbacks_.clear();
}

void ReadLineSocket::clearCloseCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    closeCallbacks_.clear();
}

void ReadLineSocket::dispatchConnect(int error) {
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

void ReadLineSocket::dispatchRecv(std::string_view line) {
    LOG(debug, "line: size={}", line.size());

    CHECK(loop_->isInLoopThread());

    std::vector<RecvCallback> recvCallbacks(std::move(recvCallbacks_));
    recvCallbacks_.clear();

    for (RecvCallback &recvCallback : recvCallbacks) {
        if (recvCallback(line)) {
            recvCallbacks_.emplace_back(std::move(recvCallback));
        }
    }
}

void ReadLineSocket::dispatchSendComplete() {
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

void ReadLineSocket::dispatchClose(int error) {
    LOG(debug, "error={}", strerrorname_np(error));

    CHECK(loop_->isInLoopThread());

    std::vector<CloseCallback> closeCallbacks(std::move(closeCallbacks_));
    closeCallbacks_.clear();

    for (CloseCallback &closeCallback : closeCallbacks) {
        if (closeCallback(error)) {
            closeCallbacks_.emplace_back(std::move(closeCallback));
        }
    }
}

void ReadLineSocket::open(const Endpoint &remoteEndpoint) {
    LOG(debug, "remoteEndpoint={}", remoteEndpoint);

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    socket_ = std::make_unique<Socket>(loop_);

    socket_->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
    socket_->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
    socket_->setRecvChunkSize(recvChunkSize_);
    socket_->setRecvTimeout(recvTimeout_);
    socket_->setSendTimeout(sendTimeout_);
    socket_->setNoDelay(noDelay_);
    socket_->setKeepAlive(keepAlive_);

    socket_->addConnectCallback([this, remoteEndpoint = remoteEndpoint.clone()](int error) {
        if (error == 0) {
            localEndpoint_ = socket_->localEndpoint();
            remoteEndpoint_ = remoteEndpoint->clone();
            pos_ = 0;

            State oldState = state_;
            state_ = State::kConnected;
            LOG(debug, "{} -> {}", oldState, state_);

            dispatchConnect(0);

            socket_->addRecvCallback([this](const char *data, size_t size, size_t &newSize) {
                return onSocketRecv(data, size, newSize);
            });
            socket_->addSendCompleteCallback([this] {
                return onSocketSendComplete();
            });
            socket_->addCloseCallback([this](int error, const char *, size_t) {
                return onSocketClose(error);
            });
        } else {
            State oldState = state_;
            state_ = State::kClosed;
            LOG(debug, "{} -> {}", oldState, state_);

            loop_->post([socket = std::move(socket_)] {});

            socket_ = nullptr;

            dispatchConnect(error);
        }

        return false;
    });

    State oldState = state_;
    state_ = State::kConnecting;
    LOG(debug, "{} -> {}", oldState, state_);

    socket_->open(remoteEndpoint);
}

void ReadLineSocket::open(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(socket->state() == Socket::State::kConnected);
    CHECK(state_ == State::kClosed);

    socket_ = std::move(socket);

    localEndpoint_ = socket_->localEndpoint();
    remoteEndpoint_ = remoteEndpoint.clone();
    pos_ = 0;

    State oldState = state_;
    state_ = State::kConnected;
    LOG(debug, "{} -> {}", oldState, state_);

    dispatchConnect(0);

    socket_->addRecvCallback([this](const char *data, size_t size, size_t &newSize) {
        return onSocketRecv(data, size, newSize);
    });
    socket_->addSendCompleteCallback([this] {
        return onSocketSendComplete();
    });
    socket_->addCloseCallback([this](int error, const char *, size_t) {
        return onSocketClose(error);
    });
}

int ReadLineSocket::send(const char *data, size_t size) {
    LOG(debug, "size={}", size);

    CHECK(loop_->isInLoopThread());

    if (state_ != State::kConnected) return ENOTCONN;

    return socket_->send(data, size);
}

int ReadLineSocket::send(const std::vector<std::pair<const char *, size_t>> &buffers) {
    LOG(debug, "buffers: size={}", buffers.size());

    CHECK(loop_->isInLoopThread());

    if (state_ != State::kConnected) return ENOTCONN;

    return socket_->send(buffers);
}

void ReadLineSocket::close(int error) {
    LOG(debug, "error={}", strerrorname_np(error));

    CHECK(loop_->isInLoopThread());

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(debug, "{} -> {}", oldState, state_);

    socket_->reset();

    loop_->post([socket = std::move(socket_)] {});

    socket_ = nullptr;

    localEndpoint_ = nullptr;
    remoteEndpoint_ = nullptr;

    dispatchClose(error);
}

void ReadLineSocket::reset() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    clearConnectCallbacks();
    clearRecvCallbacks();
    clearSendCompleteCallbacks();
    clearCloseCallbacks();

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(debug, "{} -> {}", oldState, state_);

    socket_->reset();

    loop_->post([socket = std::move(socket_)] {});

    socket_ = nullptr;

    localEndpoint_ = nullptr;
    remoteEndpoint_ = nullptr;
}

bool ReadLineSocket::onSocketRecv(const char *data, size_t size, size_t &newSize) {
    LOG(debug, "size={}", size);

    for (;;) {
        size_t length = std::string_view(data, size).find(delimiter_, pos_);

        if (length == std::string_view::npos) {
            pos_ = size;
            break;
        }

        if (length > maxLineLength_) {
            LOG(warning, "Line too long");

            close(EMSGSIZE);

            return false;
        }

        dispatchRecv(std::string_view(data, length));

        data += length + delimiter_.size();
        size -= length + delimiter_.size();
        pos_ = 0;
    }

    newSize = size;

    if (newSize > maxLineLength_) {
        LOG(warning, "Line too long");

        close(EMSGSIZE);

        return false;
    }

    return true;
}

bool ReadLineSocket::onSocketSendComplete() {
    LOG(debug, "");

    dispatchSendComplete();

    return true;
}

bool ReadLineSocket::onSocketClose(int error) {
    LOG(debug, "");

    close(error);

    return true;
}
