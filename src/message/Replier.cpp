#include "mq/message/Replier.h"

#include <cstring>
#include <memory>
#include <optional>
#include <utility>

#include "mq/net/Endpoint.h"
#include "mq/net/FramingAcceptor.h"
#include "mq/net/FramingSocket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"

#define TAG "Replier"

using namespace mq;

Replier::Replier(EventLoop *loop, const Endpoint &localEndpoint)
    : loop_(loop),
      localEndpoint_(localEndpoint.clone()) {
    LOG(debug, "");
}

Replier::~Replier() {
    LOG(debug, "");

    CHECK(state_ == State::kClosed);
}

void Replier::setMaxConnections(size_t maxConnections) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        maxConnections_ = maxConnections;
    } else {
        loop_->postAndWait([this, maxConnections] {
            CHECK(state_ == State::kClosed);

            maxConnections_ = maxConnections;
        });
    }
}

void Replier::setReuseAddr(bool reuseAddr) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        reuseAddr_ = reuseAddr;
    } else {
        loop_->postAndWait([this, reuseAddr] {
            CHECK(state_ == State::kClosed);

            reuseAddr_ = reuseAddr;
        });
    }
}

void Replier::setMaxMessageLength(size_t maxMessageLength) {
    LOG(debug, "");

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

void Replier::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
    LOG(debug, "");

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

void Replier::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
    LOG(debug, "");

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

void Replier::setRecvChunkSize(size_t recvChunkSize) {
    LOG(debug, "");

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

void Replier::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
    LOG(debug, "");

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

void Replier::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
    LOG(debug, "");

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

void Replier::setNoDelay(bool noDelay) {
    LOG(debug, "");

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

void Replier::setKeepAlive(KeepAlive keepAlive) {
    LOG(debug, "");

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

void Replier::setRecvCallback(RecvCallback recvCallback) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        recvCallback_ = std::move(recvCallback);
    } else {
        loop_->postAndWait([this, &recvCallback] {
            recvCallback_ = std::move(recvCallback);
        });
    }
}

void Replier::setRecvCallbackExecutor(Executor *recvCallbackExecutor) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        recvCallbackExecutor_ = recvCallbackExecutor;
    } else {
        loop_->postAndWait([this, recvCallbackExecutor] {
            recvCallbackExecutor_ = recvCallbackExecutor;
        });
    }
}

std::optional<std::string> Replier::dispatchRecv(const Endpoint &remoteEndpoint, std::string_view message) {
    LOG(debug, "");

    if (recvCallback_) {
        return recvCallback_(remoteEndpoint, message);
    }

    return std::nullopt;
}

Replier::State Replier::state() const {
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

int Replier::open() {
    LOG(debug, "");

    CHECK(state_ == State::kClosed);

    int error;

    if (loop_->isInLoopThread()) {
        acceptor_ = std::make_unique<FramingAcceptor>(loop_);

        acceptor_->setReuseAddr(reuseAddr_);
        acceptor_->setMaxMessageLength(maxMessageLength_);
        acceptor_->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
        acceptor_->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
        acceptor_->setRecvChunkSize(recvChunkSize_);
        acceptor_->setRecvTimeout(recvTimeout_);
        acceptor_->setSendTimeout(sendTimeout_);
        acceptor_->setNoDelay(noDelay_);
        acceptor_->setKeepAlive(keepAlive_);

        error = acceptor_->open(*localEndpoint_);
        if (error) {
            loop_->post([acceptor = std::move(acceptor_)] {});

            acceptor_ = nullptr;
        } else {
            State oldState = state_;
            state_ = State::kOpened;
            LOG(info, "{} -> {}", oldState, state_);

            acceptor_->addAcceptCallback([this](std::unique_ptr<FramingSocket> socket, const Endpoint &) {
                return onFramingAcceptorAccept(std::move(socket));
            });
        }
    } else {
        loop_->postAndWait([this, &error] {
            error = open();
        });
    }

    return error;
}

bool Replier::onFramingAcceptorAccept(std::unique_ptr<FramingSocket> socket) {
    LOG(debug, "");

    if (sockets_.size() >= maxConnections_) {
        LOG(warning, "Too many connections");

        socket->reset();

        loop_->post([socket = std::move(socket)] {});

        return true;
    }

    socket->addRecvCallback([this, socket = socket.get()](std::string_view message) {
        return onFramingSocketRecv(socket, message);
    });

    socket->addCloseCallback([this, socket = socket.get()](int error) {
        return onFramingSocketClose(socket);
    });

    sockets_.insert(std::shared_ptr(std::move(socket)));

    return true;
}

bool Replier::onFramingSocketRecv(FramingSocket *socket, std::string_view message) {
    LOG(debug, "");

    std::unique_ptr<Endpoint> remoteEndpoint = socket->remoteEndpoint();

    if (!recvCallbackExecutor_) {
        std::optional<std::string> replyMessage = dispatchRecv(*remoteEndpoint, message);

        if (replyMessage) {
            if (int error = socket->send(*replyMessage)) {
                LOG(warning, "send: error={}", strerrorname_np(error));

                loop_->post([this, socket = socket->shared_from_this()] {
                    socket->reset();

                    if (auto i = sockets_.find(socket.get()); i != sockets_.end()) {
                        sockets_.erase(i);
                    }
                });
            }
        }
    } else {
        recvCallbackExecutor_->post([this, socket = socket->shared_from_this(), remoteEndpoint = std::move(remoteEndpoint), message = std::string(message)] {
            std::optional<std::string> replyMessage = dispatchRecv(*remoteEndpoint, message);

            if (replyMessage) {
                loop_->post([this, socket, replyMessage = std::move(replyMessage)] {
                    if (int error = socket->send(*replyMessage)) {
                        LOG(warning, "send: error={}", strerrorname_np(error));

                        socket->reset();

                        loop_->post([this, socket] {
                            if (auto i = sockets_.find(socket.get()); i != sockets_.end()) {
                                sockets_.erase(i);
                            }
                        });
                    }
                });
            }
        });
    }

    return true;
}

bool Replier::onFramingSocketClose(FramingSocket *socket) {
    LOG(debug, "");

    socket->reset();

    loop_->post([socket = socket->shared_from_this()] {});

    sockets_.erase(sockets_.find(socket));

    return true;
}
