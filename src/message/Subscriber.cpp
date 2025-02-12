// SPDX-License-Identifier: MIT

#include "mq/message/Subscriber.h"

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/net/Endpoint.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Empty.h"
#include "mq/utils/Logging.h"

#define TAG "Subscriber"

using namespace mq;

Subscriber::Subscriber(EventLoop *loop) : loop_(loop) {
    LOG(debug, "");
}

Subscriber::~Subscriber() {
    LOG(debug, "");

    CHECK(state_ == State::kClosed);
}

void Subscriber::setMaxMessageLength(size_t maxMessageLength) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        maxMessageLength_ = maxMessageLength;
    } else {
        loop_->postAndWait([this, maxMessageLength] {
            CHECK(state_ == State::kClosed);

            maxMessageLength_ = maxMessageLength;
        });
    }
}

void Subscriber::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        recvBufferMaxCapacity_ = recvBufferMaxCapacity;
    } else {
        loop_->postAndWait([this, recvBufferMaxCapacity] {
            CHECK(state_ == State::kClosed);

            recvBufferMaxCapacity_ = recvBufferMaxCapacity;
        });
    }
}

void Subscriber::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        sendBufferMaxCapacity_ = sendBufferMaxCapacity;
    } else {
        loop_->postAndWait([this, sendBufferMaxCapacity] {
            CHECK(state_ == State::kClosed);

            sendBufferMaxCapacity_ = sendBufferMaxCapacity;
        });
    }
}

void Subscriber::setRecvChunkSize(size_t recvChunkSize) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        recvChunkSize_ = recvChunkSize;
    } else {
        loop_->postAndWait([this, recvChunkSize] {
            CHECK(state_ == State::kClosed);

            recvChunkSize_ = recvChunkSize;
        });
    }
}

void Subscriber::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        recvTimeout_ = recvTimeout;
    } else {
        loop_->postAndWait([this, recvTimeout] {
            CHECK(state_ == State::kClosed);

            recvTimeout_ = recvTimeout;
        });
    }
}

void Subscriber::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        sendTimeout_ = sendTimeout;
    } else {
        loop_->postAndWait([this, sendTimeout] {
            CHECK(state_ == State::kClosed);

            sendTimeout_ = sendTimeout;
        });
    }
}

void Subscriber::setNoDelay(bool noDelay) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        noDelay_ = noDelay;
    } else {
        loop_->postAndWait([this, noDelay] {
            CHECK(state_ == State::kClosed);

            noDelay_ = noDelay;
        });
    }
}

void Subscriber::setKeepAlive(KeepAlive keepAlive) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        keepAlive_ = keepAlive;
    } else {
        loop_->postAndWait([this, keepAlive] {
            CHECK(state_ == State::kClosed);

            keepAlive_ = keepAlive;
        });
    }
}

void Subscriber::setRecvCallback(RecvCallback recvCallback) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        recvCallback_ = std::move(recvCallback);
    } else {
        loop_->postAndWait([this, &recvCallback] {
            CHECK(state_ == State::kClosed);

            recvCallback_ = std::move(recvCallback);
        });
    }
}

void Subscriber::setRecvCallbackExecutor(Executor *recvCallbackExecutor) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        recvCallbackExecutor_ = recvCallbackExecutor;
    } else {
        loop_->postAndWait([this, recvCallbackExecutor] {
            CHECK(state_ == State::kClosed);

            recvCallbackExecutor_ = recvCallbackExecutor;
        });
    }
}

void Subscriber::dispatchRecv(const Endpoint &remoteEndpoint, std::string_view message) {
    LOG(debug, "remoteEndpoint={}", remoteEndpoint);

    if (recvCallback_) {
        recvCallback_(remoteEndpoint, message);
    }
}

Subscriber::State Subscriber::state() const {
    State state;

    if (loop_->isInLoopThread()) {
        state = state_;
    } else {
        loop_->postAndWait([this, &state] {
            state = state_;
        });
    }

    return state;
}

void Subscriber::subscribe(const Endpoint &remoteEndpoint, std::vector<std::string> topics) {
    LOG(debug, "remoteEndpoint={}", remoteEndpoint);

    if (loop_->isInLoopThread()) {
        CHECK(endpointToSocket_.find(remoteEndpoint) == endpointToSocket_.end());

        if (sockets_.empty()) {
            State oldState = state_;
            state_ = State::kOpened;
            LOG(debug, "{} -> {}", oldState, state_);

            token_ = std::make_shared<Empty>();
        }

        std::unique_ptr<FramingSocket> socket = std::make_unique<FramingSocket>(loop_);

        socket->setMaxMessageLength(maxMessageLength_);
        socket->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
        socket->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
        socket->setRecvChunkSize(recvChunkSize_);
        socket->setRecvTimeout(recvTimeout_);
        socket->setSendTimeout(sendTimeout_);
        socket->setNoDelay(noDelay_);
        socket->setKeepAlive(keepAlive_);

        socket->addRecvCallback([this, socket = socket.get()](std::string_view message) {
            return onFramingSocketRecv(socket, message);
        });

        if (reconnectInterval_.count() > 0) {
            socket->addConnectCallback([this,
                                        socket = socket.get(),
                                        remoteEndpoint = remoteEndpoint.clone(),
                                        reconnectInterval = reconnectInterval_,
                                        token = std::weak_ptr(token_)](int error) {
                if (error != 0) {
                    socket->loop()->postTimed([this,
                                               socket = socket->shared_from_this(),
                                               remoteEndpoint = remoteEndpoint->clone(),
                                               token = std::weak_ptr(token_)] mutable {
                        if (token.expired() || sockets_.find(socket.get()) == sockets_.end()) {
                            loop_->post([socket = std::move(socket)] {});
                        }

                        if (socket->state() == FramingSocket::State::kClosed) {
                            socket->open(*remoteEndpoint);
                        }
                    }, reconnectInterval);
                }

                return true;
            });

            socket->addCloseCallback([this,
                                      socket = socket.get(),
                                      remoteEndpoint = remoteEndpoint.clone(),
                                      reconnectInterval = reconnectInterval_](int) {
                socket->loop()->postTimed([this,
                                           socket = socket->shared_from_this(),
                                           remoteEndpoint = remoteEndpoint->clone(),
                                           token = std::weak_ptr(token_)] mutable {
                    if (token.expired() || sockets_.find(socket.get()) == sockets_.end()) {
                        loop_->post([socket = std::move(socket)] {});
                    }

                    if (socket->state() == FramingSocket::State::kClosed) {
                        socket->open(*remoteEndpoint);
                    }
                }, reconnectInterval);

                return true;
            });
        }

        socket->open(remoteEndpoint);

        endpointToSocket_.emplace(remoteEndpoint.clone(), socket.get());
        socketToTopics_.emplace(socket.get(), std::move(topics));
        sockets_.insert(std::shared_ptr(std::move(socket)));
    } else {
        loop_->postAndWait([this, &remoteEndpoint, &topics] {
            subscribe(remoteEndpoint, std::move(topics));
        });
    }
}

void Subscriber::unsubscribe(const Endpoint &remoteEndpoint) {
    LOG(debug, "remoteEndpoint={}", remoteEndpoint);

    if (loop_->isInLoopThread()) {
        auto i = endpointToSocket_.find(remoteEndpoint);
        auto j = socketToTopics_.find(i->second);
        auto k = sockets_.find(i->second);

        endpointToSocket_.erase(i);
        socketToTopics_.erase(j);

        std::shared_ptr<FramingSocket> socket = std::move(sockets_.extract(k).value());

        socket->reset();

        loop_->post([socket = std::move(socket)] {});

        if (sockets_.empty()) {
            token_ = nullptr;

            State oldState = state_;
            state_ = State::kClosed;
            LOG(debug, "{} -> {}", oldState, state_);
        }
    } else {
        loop_->postAndWait([this, &remoteEndpoint] {
            unsubscribe(remoteEndpoint);
        });
    }
}

bool Subscriber::onFramingSocketRecv(FramingSocket *socket, std::string_view message) {
    LOG(debug, "");

    if (!recvCallbackExecutor_) {
        for (const std::string &topic : socketToTopics_.find(socket)->second) {
            if (message.starts_with(topic)) {
                dispatchRecv(*socket->remoteEndpoint(), message);
                break;
            }
        }
    } else {
        for (const std::string &topic : socketToTopics_.find(socket)->second) {
            if (message.starts_with(topic)) {
                recvCallbackExecutor_->post([this,
                                             socket,
                                             remoteEndpoint = socket->remoteEndpoint(),
                                             message = std::string(message),
                                             token = std::weak_ptr(token_)] {
                    if (token.expired() || sockets_.find(socket) == sockets_.end()) return;

                    dispatchRecv(*remoteEndpoint, message);
                });

                break;
            }
        }
    }

    return true;
}
