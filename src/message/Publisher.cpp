// SPDX-License-Identifier: MIT

#include "mq/message/Publisher.h"

#include <chrono>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingAcceptor.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Empty.h"
#include "mq/utils/Logging.h"
#include "mq/utils/MaybeOwnedString.h"

#define TAG "Publisher"

using namespace mq;

Publisher::Publisher(EventLoop *loop, const Endpoint &localEndpoint)
    : loop_(loop), localEndpoint_(localEndpoint.clone()) {
    LOG(debug, "");
}

Publisher::~Publisher() {
    LOG(debug, "");

    CHECK(state_ == State::kClosed);
}

void Publisher::setMaxConnections(size_t maxConnections) {
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


void Publisher::setMaxMessageLength(size_t maxMessageLength) {
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

void Publisher::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
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

void Publisher::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
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

void Publisher::setRecvChunkSize(size_t recvChunkSize) {
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

void Publisher::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
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

void Publisher::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
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

void Publisher::setNoDelay(bool noDelay) {
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

void Publisher::setKeepAlive(KeepAlive keepAlive) {
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

int Publisher::open() {
    LOG(debug, "");

    int error;

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        acceptor_ = std::make_unique<FramingAcceptor>(loop_);

        acceptor_->setMaxMessageLength(maxMessageLength_);
        acceptor_->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
        acceptor_->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
        acceptor_->setRecvChunkSize(recvChunkSize_);
        acceptor_->setRecvTimeout(recvTimeout_);
        acceptor_->setSendTimeout(sendTimeout_);
        acceptor_->setReuseAddr(reuseAddr_);
        acceptor_->setReusePort(reusePort_);
        acceptor_->setNoDelay(noDelay_);
        acceptor_->setKeepAlive(keepAlive_);

        acceptor_->addAcceptCallback([this](std::unique_ptr<FramingSocket> socket, const Endpoint &) {
            return onFramingAcceptorAccept(std::move(socket));
        });

        error = acceptor_->open(*localEndpoint_);
        if (error) {
            loop_->post([acceptor = std::move(acceptor_)] {});

            acceptor_ = nullptr;
        } else {
            token_ = std::make_shared<Empty>();

            State oldState = state_;
            state_ = State::kOpened;
            LOG(debug, "{} -> {}", oldState, state_);
        }
    } else {
        loop_->postAndWait([this, &error] {
            error = open();
        });
    }

    return error;
}

void Publisher::send(MaybeOwnedString message) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        for (const std::shared_ptr<FramingSocket> &socket : sockets_) {
            if (int error = socket->send(message)) {
                LOG(warning, "send: error={}", strerrorname_np(error));
            }
        }
    } else {
        loop_->post([this, message = std::string(std::move(message)), token = std::weak_ptr(token_)] mutable {
            if (token.expired()) return;

            send(std::move(message));
        });
    }
}

void Publisher::send(std::vector<MaybeOwnedString> pieces) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        std::vector<std::string_view> newPieces(std::make_move_iterator(pieces.begin()),
                                                std::make_move_iterator(pieces.end()));

        for (const std::shared_ptr<FramingSocket> &socket : sockets_) {
            if (int error = socket->send(newPieces)) {
                LOG(warning, "send: error={}", strerrorname_np(error));
            }
        }
    } else {
        if (!sockets_.empty()) {
            std::vector<MaybeOwnedString> newPieces;
            newPieces.reserve(pieces.size());
            for (MaybeOwnedString &piece : pieces) {
                newPieces.emplace_back(std::string(std::move(piece)));
            }

            loop_->post([this, newPieces = std::move(newPieces), token = std::weak_ptr(token_)] mutable {
                if (token.expired()) return;

                send(std::move(newPieces));
            });
        }
    }
}

void Publisher::close() {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        if (state_ == State::kClosed) return;

        acceptor_->reset();

        for (const std::shared_ptr<FramingSocket> &socket : sockets_) {
            socket->reset();
        }

        loop_->post([acceptor = std::move(acceptor_), sockets = std::move(sockets_)] {});

        acceptor_ = nullptr;
        sockets_.clear();

        token_ = nullptr;

        State oldState = state_;
        state_ = State::kClosed;
        LOG(debug, "{} -> {}", oldState, state_);
    } else {
        loop_->postAndWait([this] {
            close();
        });
    }
}

bool Publisher::onFramingAcceptorAccept(std::unique_ptr<FramingSocket> socket) {
    LOG(debug, "");

    if (maxConnections_ > 0 && sockets_.size() == maxConnections_) {
        LOG(warning, "Too many connections");

        socket->reset();

        loop_->post([socket = std::move(socket)] {});

        return true;
    }

    socket->addCloseCallback([this, socket = socket.get()](int) {
        return onFramingSocketClose(socket);
    });

    sockets_.insert(std::shared_ptr(std::move(socket)));

    return true;
}

bool Publisher::onFramingSocketClose(FramingSocket *socket) {
    LOG(debug, "");

    socket->reset();

    loop_->post([socket = socket->shared_from_this()] {});

    sockets_.erase(sockets_.find(socket));

    return true;
}
