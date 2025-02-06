#include "mq/message/Requester.h"

#include <cerrno>
#include <chrono>
#include <future>
#include <memory>
#include <utility>

#include "mq/net/FramingSocket.h"
#include "mq/net/Socket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

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
    LOG(debug, "");

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

void Requester::setRecvBufferMaxCapacity(size_t recvBufferMaxCapacity) {
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

void Requester::setSendBufferMaxCapacity(size_t sendBufferMaxCapacity) {
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

void Requester::setRecvChunkSize(size_t recvChunkSize) {
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

void Requester::setRecvTimeout(std::chrono::nanoseconds recvTimeout) {
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

void Requester::setSendTimeout(std::chrono::nanoseconds sendTimeout) {
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

void Requester::setRcvBuf(int rcvBuf) {
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

void Requester::setSndBuf(int sndBuf) {
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

void Requester::setNoDelay(bool noDelay) {
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

void Requester::setKeepAlive(KeepAlive keepAlive) {
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

void Requester::setConnectCallback(ConnectCallback connectCallback) {
    LOG(debug, "");

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
    LOG(debug, "");

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
    LOG(debug, "");

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
    LOG(debug, "");

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
    LOG(debug, "");

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

        socket_ = std::make_unique<FramingSocket>(loop_);

        socket_->setMaxMessageLength(maxMessageLength_);
        socket_->setRecvBufferMaxCapacity(recvBufferMaxCapacity_);
        socket_->setSendBufferMaxCapacity(sendBufferMaxCapacity_);
        socket_->setRecvChunkSize(recvChunkSize_);
        socket_->setRecvTimeout(recvTimeout_);
        socket_->setSendTimeout(sendTimeout_);
        socket_->setRcvBuf(rcvBuf_);
        socket_->setSndBuf(sndBuf_);
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
                    loop_->postTimed([this] {
                        if (socket_->state() == FramingSocket::State::kClosed) {
                            socket_->open(*remoteEndpoint_);
                        }

                        return std::chrono::nanoseconds{};
                    }, reconnectInterval_);
                }

                return true;
            });

            socket_->addCloseCallback([this](int) {
                loop_->postTimed([this] {
                    if (socket_->state() == FramingSocket::State::kClosed) {
                        socket_->open(*remoteEndpoint_);
                    }

                    return std::chrono::nanoseconds{};
                }, reconnectInterval_);

                return true;
            });
        }

        socket_->open(*remoteEndpoint_);

        flag_ = std::make_shared<char>();

        State oldState = state_;
        state_ = State::kOpened;
        LOG(info, "{} -> {}", oldState, state_);
    } else {
        loop_->postAndWait([this] {
            open();
        });
    }
}

int Requester::waitForConnected(std::chrono::nanoseconds timeout) {
    LOG(debug, "");

    CHECK(!loop_->isInLoopThread());

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    loop_->postAndWait([this, promise = std::move(promise)] mutable {
        if (socket_->state() == FramingSocket::State::kConnected) {
            promise.set_value();
        } else {
            socket_->addConnectCallback([this, promise = std::move(promise)](int error) mutable {
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

void Requester::send(std::string_view message) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kOpened);

        if (int error = socket_->send(message)) {
            LOG(warning, "send: error={}", strerrorname_np(error));
        }
    } else {
        loop_->post([this, message = std::string(message), flag = std::weak_ptr(flag_)] {
            if (flag.expired()) return;

            send(message);
        });
    }
}

void Requester::close() {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        if (state_ == State::kClosed) return;

        State oldState = state_;
        state_ = State::kClosed;
        LOG(info, "{} -> {}", oldState, state_);

        flag_ = nullptr;

        socket_->reset();

        loop_->post([socket = std::move(socket_)] {});

        socket_ = nullptr;
    } else {
        loop_->postAndWait([this] {
            close();
        });
    }
}

bool Requester::onFramingSocketConnect(int error) {
    LOG(debug, "");

    if (!error) {
        if (!connectCallbackExecutor_) {
            dispatchConnect();
        } else {
            connectCallbackExecutor_->post([this, flag = std::weak_ptr(flag_)] {
                if (flag.expired()) return;

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
        recvCallbackExecutor_->post([this, message = std::string(message), flag = std::weak_ptr(flag_)] {
            if (flag.expired()) return;

            dispatchRecv(message);
        });
    }

    return true;
}
