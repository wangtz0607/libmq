// SPDX-License-Identifier: MIT

#include "mq/message/Replier.h"

#include <chrono>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/net/Endpoint.h"
#include "mq/net/FramingAcceptor.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Empty.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"
#include "mq/utils/MaybeOwnedString.h"

#define TAG "Replier"

using namespace mq;

void Replier::Promise::operator()(MaybeOwnedString replyMessage) {
    if (token_.expired() || replier_->sockets_.find(socket_.get()) == replier_->sockets_.end()) {
        replier_->loop_->post([socket = std::move(socket_)] {});

        return;
    }

    if (replier_->loop_->isInLoopThread()) {
        if (int error = socket_->send(replyMessage)) {
            LOG(warning, "send: error={}", strerrorname_np(error));

            replier_->loop_->post([replier = replier_,
                                   socket = std::move(socket_),
                                   token = std::weak_ptr(token_)] mutable {
                if (token.expired() || replier->sockets_.find(socket.get()) == replier->sockets_.end()) return;

                socket->reset();

                replier->sockets_.erase(replier->sockets_.find(socket.get()));
            });
        }
    } else {
        replier_->loop_->post([replier = replier_,
                               socket = std::move(socket_),
                               token = std::weak_ptr(token_),
                               replyMessage = std::string(std::move(replyMessage))] {
            if (token.expired() || replier->sockets_.find(socket.get()) == replier->sockets_.end()) return;

            if (int error = socket->send(replyMessage)) {
                LOG(warning, "send: error={}", strerrorname_np(error));

                socket->reset();

                replier->sockets_.erase(replier->sockets_.find(socket));
            }
        });
    }
}

void Replier::Promise::operator()(std::vector<MaybeOwnedString> replyPieces) {
    if (token_.expired() || replier_->sockets_.find(socket_.get()) == replier_->sockets_.end()) {
        replier_->loop_->post([socket = std::move(socket_)] {});

        return;
    }

    if (replier_->loop_->isInLoopThread()) {
        std::vector<std::string_view> pieces;
        pieces.reserve(replyPieces.size());
        for (const MaybeOwnedString &replyPiece : replyPieces) {
            pieces.push_back(std::string_view(replyPiece));
        }

        if (int error = socket_->send(pieces)) {
            LOG(warning, "send: error={}", strerrorname_np(error));

            replier_->loop_->post([replier = replier_,
                                   socket = std::move(socket_),
                                   token = std::weak_ptr(token_)] mutable {
                if (token.expired() || replier->sockets_.find(socket.get()) == replier->sockets_.end()) return;

                socket->reset();

                replier->sockets_.erase(replier->sockets_.find(socket.get()));
            });
        }
    } else {
        size_t size = 0;
        for (const MaybeOwnedString &piece : replyPieces) {
            size += piece.size();
        }

        auto op = [replyPieces = std::move(replyPieces)](char *data, size_t size) {
            for (const MaybeOwnedString &piece : replyPieces) {
                memcpy(data, piece.data(), piece.size());
                data += piece.size();
            }

            return size;
        };

        std::string replyMessage;
        replyMessage.resize_and_overwrite(size, std::move(op));

        replier_->loop_->post([replier = replier_,
                               socket = std::move(socket_),
                               token = std::weak_ptr(token_),
                               replyMessage = std::move(replyMessage)] {
            if (token.expired() || replier->sockets_.find(socket.get()) == replier->sockets_.end()) return;

            if (int error = socket->send(replyMessage)) {
                LOG(warning, "send: error={}", strerrorname_np(error));

                socket->reset();

                replier->sockets_.erase(replier->sockets_.find(socket));
            }
        });
    }
}

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

void Replier::setReusePort(bool reusePort) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        reusePort_ = reusePort;
    } else {
        loop_->postAndWait([this, reusePort] {
            CHECK(state_ == State::kClosed);

            reusePort_ = reusePort;
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

void Replier::setRcvBuf(int rcvBuf) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        rcvBuf_ = rcvBuf;
    } else {
        loop_->postAndWait([this, rcvBuf] {
            CHECK(state_ == State::kClosed);

            rcvBuf_ = rcvBuf;
        });
    }
}

void Replier::setSndBuf(int sndBuf) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        sndBuf_ = sndBuf;
    } else {
        loop_->postAndWait([this, sndBuf] {
            CHECK(state_ == State::kClosed);

            sndBuf_ = sndBuf;
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

void Replier::dispatchRecv(const Endpoint &remoteEndpoint, std::string_view message, Promise promise) {
    LOG(debug, "");

    if (recvCallback_) {
        return recvCallback_(remoteEndpoint, message, std::move(promise));
    }
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

    int error;

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        acceptor_ = std::make_unique<FramingAcceptor>(loop_);

        acceptor_->setReuseAddr(reuseAddr_);
        acceptor_->setReusePort(reusePort_);
        acceptor_->setMaxMessageLength(maxMessageLength_);
        acceptor_->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
        acceptor_->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
        acceptor_->setRecvChunkSize(recvChunkSize_);
        acceptor_->setRecvTimeout(recvTimeout_);
        acceptor_->setSendTimeout(sendTimeout_);
        acceptor_->setRcvBuf(rcvBuf_);
        acceptor_->setSndBuf(sndBuf_);
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

void Replier::close() {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        if (state_ == State::kClosed) return;

        acceptor_->reset();

        loop_->post([acceptor = std::move(acceptor_)] {});

        acceptor_ = nullptr;

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

bool Replier::onFramingAcceptorAccept(std::unique_ptr<FramingSocket> socket) {
    LOG(debug, "");

    if (maxConnections_ > 0 && sockets_.size() == maxConnections_) {
        LOG(warning, "Too many connections");

        socket->reset();

        loop_->post([socket = std::move(socket)] {});

        return true;
    }

    socket->addRecvCallback([this, socket = socket.get()](std::string_view message) {
        return onFramingSocketRecv(socket, message);
    });

    socket->addCloseCallback([this, socket = socket.get()](int) {
        return onFramingSocketClose(socket);
    });

    sockets_.insert(std::shared_ptr(std::move(socket)));

    return true;
}

bool Replier::onFramingSocketRecv(FramingSocket *socket, std::string_view message) {
    LOG(debug, "");

    std::unique_ptr<Endpoint> remoteEndpoint = socket->remoteEndpoint();

    Promise promise(this, socket->shared_from_this(), std::weak_ptr(token_));

    if (!recvCallbackExecutor_) {
        dispatchRecv(*remoteEndpoint, message, std::move(promise));
    } else {
        recvCallbackExecutor_->post([this,
                                     remoteEndpoint = std::move(remoteEndpoint),
                                     message = std::string(message),
                                     promise = std::move(promise),
                                     token = std::weak_ptr(token_)] mutable {
            if (token.expired()) return;

            dispatchRecv(*remoteEndpoint, message, std::move(promise));
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
