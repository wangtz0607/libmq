#include "mq/net/FramingSocket.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/Socket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Logging.h"

#define TAG "FramingSocket"

using namespace mq;

FramingSocket::FramingSocket(EventLoop *loop) : loop_(loop) {
    LOG(debug, "");
}

FramingSocket::~FramingSocket() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(loop_->state() == EventLoop::State::kTask);
    CHECK(state_ == State::kClosed);
}

void FramingSocket::setMaxMessageLength(size_t maxMessageLength) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    maxMessageLength_ = maxMessageLength;
}

void FramingSocket::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvBufferMaxCapacity_ = recvBufferMaxCapacity;
}

void FramingSocket::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendBufferMaxCapacity_ = sendBufferMaxCapacity;
}

void FramingSocket::setRecvChunkSize(size_t recvChunkSize) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvChunkSize_ = recvChunkSize;
}

void FramingSocket::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    recvTimeout_ = recvTimeout;
}

void FramingSocket::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    sendTimeout_ = sendTimeout;
}

void FramingSocket::setNoDelay(bool noDelay) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    noDelay_ = noDelay;
}

void FramingSocket::setKeepAlive(KeepAlive keepAlive) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    keepAlive_ = keepAlive;
}

FramingSocket::State FramingSocket::state() const {
    CHECK(loop_->isInLoopThread());

    return state_;
}

Socket &FramingSocket::socket() {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ != State::kClosed);

    return *socket_;
}

const Socket &FramingSocket::socket() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ != State::kClosed);

    return *socket_;
}

std::unique_ptr<Endpoint> FramingSocket::localEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kConnected);

    return localEndpoint_->clone();
}

std::unique_ptr<Endpoint> FramingSocket::remoteEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kConnected);

    return remoteEndpoint_->clone();
}

bool FramingSocket::hasConnectCallback() const {
    CHECK(loop_->isInLoopThread());

    return !connectCallbacks_.empty();
}

bool FramingSocket::hasRecvCallback() const {
    CHECK(loop_->isInLoopThread());

    return !recvCallbacks_.empty();
}

bool FramingSocket::hasSendCompleteCallback() const {
    CHECK(loop_->isInLoopThread());

    return !sendCompleteCallbacks_.empty();
}

bool FramingSocket::hasCloseCallback() const {
    CHECK(loop_->isInLoopThread());

    return !closeCallbacks_.empty();
}

void FramingSocket::addConnectCallback(ConnectCallback connectCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    connectCallbacks_.emplace_back(std::move(connectCallback));
}

void FramingSocket::addRecvCallback(RecvCallback recvCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    recvCallbacks_.emplace_back(std::move(recvCallback));
}

void FramingSocket::addSendCompleteCallback(SendCompleteCallback sendCompleteCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    sendCompleteCallbacks_.emplace_back(std::move(sendCompleteCallback));
}

void FramingSocket::addCloseCallback(CloseCallback closeCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    closeCallbacks_.emplace_back(std::move(closeCallback));
}

void FramingSocket::clearConnectCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    connectCallbacks_.clear();
}

void FramingSocket::clearRecvCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    recvCallbacks_.clear();
}

void FramingSocket::clearSendCompleteCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    sendCompleteCallbacks_.clear();
}

void FramingSocket::clearCloseCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    closeCallbacks_.clear();
}

void FramingSocket::dispatchConnect(int error) {
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

void FramingSocket::dispatchRecv(std::string_view message) {
    LOG(debug, "message: size={}", message.size());

    CHECK(loop_->isInLoopThread());

    std::vector<RecvCallback> recvCallbacks(std::move(recvCallbacks_));
    recvCallbacks_.clear();

    for (RecvCallback &recvCallback : recvCallbacks) {
        if (recvCallback(message)) {
            recvCallbacks_.emplace_back(std::move(recvCallback));
        }
    }
}

void FramingSocket::dispatchSendComplete() {
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

void FramingSocket::dispatchClose(int error) {
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

void FramingSocket::open(const Endpoint &remoteEndpoint) {
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

            State oldState = state_;
            state_ = State::kConnected;
            LOG(info, "{} -> {}", oldState, state_);

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
            LOG(info, "{} -> {}", oldState, state_);

            loop_->post([socket = std::move(socket_)] {});

            socket_ = nullptr;

            dispatchConnect(error);
        }

        return false;
    });

    State oldState = state_;
    state_ = State::kConnecting;
    LOG(info, "{} -> {}", oldState, state_);

    socket_->open(remoteEndpoint);
}

void FramingSocket::open(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(socket->state() == Socket::State::kConnected);
    CHECK(state_ == State::kClosed);

    socket_ = std::move(socket);

    localEndpoint_ = socket_->localEndpoint();
    remoteEndpoint_ = remoteEndpoint.clone();

    State oldState = state_;
    state_ = State::kConnected;
    LOG(info, "{} -> {}", oldState, state_);

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

int FramingSocket::send(std::string_view message) {
    LOG(debug, "message: size={}", message.size());

    CHECK(loop_->isInLoopThread());
    CHECK(message.size() <= maxMessageLength_);

    if (state_ != State::kConnected) return ENOTCONN;

    uint32_t length = static_cast<uint32_t>(message.size());
    length = toLittleEndian(length);

    return socket_->send({{reinterpret_cast<const char *>(&length), 4}, {message.data(), message.size()}});
}

int FramingSocket::send(const std::vector<std::string_view> &messages) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    for (std::string_view message : messages) {
        CHECK(message.size() <= maxMessageLength_);
    }

    if (state_ != State::kConnected) return ENOTCONN;

    std::vector<std::pair<const char *, size_t>> buffers;

    for (std::string_view message: messages) {
        uint32_t length = static_cast<uint32_t>(message.size());
        length = toLittleEndian(length);

        buffers.emplace_back(reinterpret_cast<const char *>(&length), 4);
        buffers.emplace_back(message.data(), message.size());
    }

    return socket_->send(buffers);
}

void FramingSocket::close(int error) {
    LOG(debug, "error={}", strerrorname_np(error));

    CHECK(loop_->isInLoopThread());

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(info, "{} -> {}", oldState, state_);

    socket_->reset();

    loop_->post([socket = std::move(socket_)] {});

    socket_ = nullptr;

    localEndpoint_ = nullptr;
    remoteEndpoint_ = nullptr;

    dispatchClose(error);
}

void FramingSocket::reset() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    clearConnectCallbacks();
    clearRecvCallbacks();
    clearSendCompleteCallbacks();
    clearCloseCallbacks();

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(info, "{} -> {}", oldState, state_);

    socket_->reset();

    loop_->post([socket = std::move(socket_)] {});

    socket_ = nullptr;

    localEndpoint_ = nullptr;
    remoteEndpoint_ = nullptr;
}

bool FramingSocket::onSocketRecv(const char *data, size_t size, size_t &newSize) {
    LOG(debug, "");

    for (;;) {
        if (size < 4) break;

        uint32_t length;
        memcpy(&length, data, 4);
        length = fromLittleEndian(length);

        if (length > maxMessageLength_) {
            LOG(warning, "Message too long ({})", length);

            close(EMSGSIZE);

            return false;
        }

        if (size < 4 + length) break;

        dispatchRecv(std::string_view(data + 4, length));

        data += 4 + length;
        size -= 4 + length;
    }

    newSize = size;

    return true;
}

bool FramingSocket::onSocketSendComplete() {
    LOG(debug, "");

    dispatchSendComplete();

    return true;
}

bool FramingSocket::onSocketClose(int error) {
    LOG(debug, "");

    close(error);

    return true;
}

void mq::enableAutoReconnectAndOpen(FramingSocket &socket,
                                    const Endpoint &remoteEndpoint,
                                    std::chrono::nanoseconds interval) {
    LOG(debug, "");

    CHECK(socket.loop()->isInLoopThread());

    socket.addConnectCallback([&socket, remoteEndpoint = remoteEndpoint.clone(), interval](int error) {
        if (error) {
            socket.loop()->postTimed([&socket, remoteEndpoint = remoteEndpoint->clone()] {
                if (socket.state() == FramingSocket::State::kClosed) {
                    socket.open(*remoteEndpoint);
                }

                return std::chrono::nanoseconds{};
            }, interval);
        }

        return true;
    });

    socket.addCloseCallback([&socket, remoteEndpoint = remoteEndpoint.clone(), interval](int) {
        socket.loop()->postTimed([&socket, remoteEndpoint = remoteEndpoint->clone()] {
            if (socket.state() == FramingSocket::State::kClosed) {
                socket.open(*remoteEndpoint);
            }

            return std::chrono::nanoseconds{};
        }, interval);

        return true;
    });

    socket.open(remoteEndpoint);
}
