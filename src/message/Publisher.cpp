#include "mq/message/Publisher.h"

#include <cstring>
#include <memory>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingAcceptor.h"
#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

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

void Publisher::setReuseAddr(bool reuseAddr) {
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

void Publisher::setMaxMessageLength(size_t maxMessageLength) {
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

void Publisher::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
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

void Publisher::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
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

void Publisher::setRecvChunkSize(size_t recvChunkSize) {
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

void Publisher::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
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

void Publisher::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
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

void Publisher::setRcvBuf(int rcvBuf) {
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

void Publisher::setSndBuf(int sndBuf) {
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

void Publisher::setNoDelay(bool noDelay) {
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

void Publisher::setKeepAlive(KeepAlive keepAlive) {
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

int Publisher::open() {
    LOG(debug, "");

    int error;

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        acceptor_ = std::make_unique<FramingAcceptor>(loop_);

        acceptor_->setReuseAddr(reuseAddr_);
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
            State oldState = state_;
            state_ = State::kOpened;
            LOG(info, "{} -> {}", oldState, state_);
        }
    } else {
        loop_->postAndWait([this, &error] {
            error = open();
        });
    }

    return error;
}

void Publisher::send(std::string_view message) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        for (const std::shared_ptr<FramingSocket> &socket : sockets_) {
            if (int error = socket->send(message)) {
                LOG(warning, "send: error={}", strerrorname_np(error));
            }
        }
    } else {
        loop_->post([this, message = std::string(message)] {
            send(message);
        });
    }
}

bool Publisher::onFramingAcceptorAccept(std::unique_ptr<FramingSocket> socket) {
    LOG(debug, "");

    if (sockets_.size() >= maxConnections_) {
        LOG(warning, "Too many connections");

        socket->reset();

        loop_->post([socket = std::move(socket)] {});

        return true;
    }

    socket->addCloseCallback([this, socket = socket.get()](int error) {
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
