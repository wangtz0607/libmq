// SPDX-License-Identifier: MIT

#include "mq/message/Requester.h"

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <future>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Empty.h"
#include "mq/utils/Logging.h"
#include "mq/utils/MaybeOwnedString.h"

#define TAG "Requester"

using namespace mq;

Requester::Requester(EventLoop *loop, const Endpoint &remoteEndpoint)
    : loop_(loop),
      remoteEndpoint_(remoteEndpoint.clone()) {
    LOG(debug, "");
}

Requester::~Requester() {
    LOG(debug, "");

    CHECK(state_ == State::kClosed);
}

void Requester::setReconnectInterval(std::chrono::nanoseconds reconnectInterval) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        reconnectInterval_ = reconnectInterval;
    } else {
        loop_->postAndWait([this, reconnectInterval] {
            CHECK(state_ == State::kClosed);

            reconnectInterval_ = reconnectInterval;
        });
    }
}

void Requester::setMaxMessageLength(size_t maxMessageLength) {
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

void Requester::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
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

void Requester::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
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

void Requester::setRecvChunkSize(size_t recvChunkSize) {
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

void Requester::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
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

void Requester::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
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

void Requester::setNoDelay(bool noDelay) {
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

void Requester::setKeepAlive(KeepAlive keepAlive) {
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

void Requester::setConnectCallback(ConnectCallback connectCallback) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        connectCallback_ = std::move(connectCallback);
    } else {
        loop_->postAndWait([this, &connectCallback] {
            CHECK(state_ == State::kClosed);

            connectCallback_ = std::move(connectCallback);
        });
    }
}

void Requester::setRecvCallback(RecvCallback recvCallback) {
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

void Requester::setConnectCallbackExecutor(Executor *connectCallbackExecutor) {
    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        connectCallbackExecutor_ = connectCallbackExecutor;
    } else {
        loop_->postAndWait([this, connectCallbackExecutor] {
            CHECK(state_ == State::kClosed);

            connectCallbackExecutor_ = connectCallbackExecutor;
        });
    }
}

void Requester::setRecvCallbackExecutor(Executor *recvCallbackExecutor) {
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

void Requester::dispatchConnect() {
    LOG(debug, "");

    if (connectCallback_) {
        connectCallback_();
    }
}

void Requester::dispatchRecv(std::string_view message) {
    LOG(debug, "");

    if (recvCallback_) {
        recvCallback_(message);
    }
}

Requester::State Requester::state() const {
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

void Requester::open() {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kClosed);

        State oldState = state_;
        state_ = State::kOpened;
        LOG(debug, "{} -> {}", oldState, state_);

        token_ = std::make_shared<Empty>();

        socket_ = std::make_unique<FramingSocket>(loop_);

        socket_->setMaxMessageLength(maxMessageLength_);
        socket_->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
        socket_->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
        socket_->setRecvChunkSize(recvChunkSize_);
        socket_->setRecvTimeout(recvTimeout_);
        socket_->setSendTimeout(sendTimeout_);
        socket_->setNoDelay(noDelay_);
        socket_->setKeepAlive(keepAlive_);

        socket_->addConnectCallback([this](int error) {
            return onFramingSocketConnect(error);
        });
        socket_->addRecvCallback([this](std::string_view message) {
            return onFramingSocketRecv(message);
        });

        if (reconnectInterval_.count() > 0) {
            socket_->addConnectCallback([this](int error) {
                if (error != 0) {
                    loop_->postTimed([this, token = std::weak_ptr(token_)] {
                        if (!token.expired() && socket_->state() == FramingSocket::State::kClosed) {
                            socket_->open(*remoteEndpoint_);
                        }
                    }, reconnectInterval_);
                }

                return true;
            });

            socket_->addCloseCallback([this](int) {
                loop_->postTimed([this, token = std::weak_ptr(token_)] {
                    if (!token.expired() && socket_->state() == FramingSocket::State::kClosed) {
                        socket_->open(*remoteEndpoint_);
                    }
                }, reconnectInterval_);

                return true;
            });
        }

        socket_->open(*remoteEndpoint_);
    } else {
        loop_->postAndWait([this] {
            open();
        });
    }
}

int Requester::waitForConnected(std::chrono::nanoseconds timeout) {
    CHECK(!loop_->isInLoopThread());

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    loop_->postAndWait([this, promise = std::move(promise)] mutable {
        if (socket_->state() == FramingSocket::State::kConnected) {
            promise.set_value();
        } else {
            socket_->addConnectCallback([promise = std::move(promise)](int error) mutable {
                if (error == 0) {
                    promise.set_value();
                }

                return error != 0;
            });
        }
    });

    if (timeout.count() == 0) {
        future.wait();
    } else {
        if (future.wait_for(timeout) != std::future_status::ready) {
            return ETIMEDOUT;
        }
    }

    return 0;
}

void Requester::send(MaybeOwnedString message) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kOpened);

        if (int error = socket_->send(message)) {
            LOG(warning, "send: error={}", strerrorname_np(error));
        }
    } else {
        loop_->post([this, message = std::string(std::move(message)), token = std::weak_ptr(token_)] mutable {
            if (token.expired()) return;

            send(std::move(message));
        });
    }
}

void Requester::send(std::vector<MaybeOwnedString> pieces) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kOpened);

        std::vector<std::string_view> newPieces(std::make_move_iterator(pieces.begin()),
                                                std::make_move_iterator(pieces.end()));

        if (int error = socket_->send(newPieces)) {
            LOG(warning, "send: error={}", strerrorname_np(error));
        }
    } else {
        size_t size = 0;
        for (const MaybeOwnedString &piece : pieces) {
            size += piece.size();
        }

        auto op = [pieces = std::move(pieces)](char *data, size_t size) {
            for (const MaybeOwnedString &piece : pieces) {
                memcpy(data, piece.data(), piece.size());
                data += piece.size();
            }

            return size;
        };

        std::string message;
        message.resize_and_overwrite(size, std::move(op));

        loop_->post([this, message = std::move(message), token = std::weak_ptr(token_)] mutable {
            if (token.expired()) return;

            send(std::move(message));
        });
    }
}

void Requester::close() {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        if (state_ == State::kClosed) return;

        socket_->reset();

        loop_->post([socket = std::move(socket_)] {});

        socket_ = nullptr;

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

bool Requester::onFramingSocketConnect(int error) {
    LOG(debug, "error={}", error);

    if (!error) {
        if (!connectCallbackExecutor_) {
            dispatchConnect();
        } else {
            connectCallbackExecutor_->post([this, token = std::weak_ptr(token_)] {
                if (token.expired()) return;

                dispatchConnect();
            });
        }
    }

    return true;
}

bool Requester::onFramingSocketRecv(std::string_view message) {
    LOG(debug, "");

    if (!recvCallbackExecutor_) {
        dispatchRecv(message);
    } else {
        recvCallbackExecutor_->post([this, message = std::string(message), token = std::weak_ptr(token_)] {
            if (token.expired()) return;

            dispatchRecv(message);
        });
    }

    return true;
}
