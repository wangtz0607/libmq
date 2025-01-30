#include "mq/message/Requester.h"

#include <cerrno>
#include <memory>

#include "mq/net/FramingSocket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "Requester"

using namespace mq;

Requester::Requester(EventLoop *loop, const Endpoint &remoteEndpoint, Params params)
    : loop_(loop),
      remoteEndpoint_(remoteEndpoint.clone()),
      params_(params) {
    LOG(debug, "");
}

Requester::~Requester() {
    LOG(debug, "");

    CHECK(state_ == State::kClosed);
}

void Requester::setConnectCallback(ConnectCallback connectCallback) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        connectCallback_ = std::move(connectCallback);
    } else {
        loop_->postAndWait([this, &connectCallback] {
            connectCallback_ = std::move(connectCallback);
        });
    }
}

void Requester::setRecvCallback(RecvCallback recvCallback) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        recvCallback_ = std::move(recvCallback);
    } else {
        loop_->postAndWait([this, &recvCallback] {
            recvCallback_ = std::move(recvCallback);
        });
    }
}

void Requester::setConnectCallbackExecutor(Executor *connectCallbackExecutor) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        connectCallbackExecutor_ = connectCallbackExecutor;
    } else {
        loop_->postAndWait([this, connectCallbackExecutor] {
            connectCallbackExecutor_ = connectCallbackExecutor;
        });
    }
}

void Requester::setRecvCallbackExecutor(Executor *recvCallbackExecutor) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        recvCallbackExecutor_ = recvCallbackExecutor;
    } else {
        loop_->postAndWait([this, recvCallbackExecutor] {
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

    CHECK(state_ == State::kClosed);

    socket_ = std::make_unique<FramingSocket>(loop_, params_.framingSocket);

    if (loop_->isInLoopThread()) {
        if (params_.reconnectInterval.count() > 0) {
            enableAutoReconnectAndOpen(*socket_, *remoteEndpoint_, params_.reconnectInterval);
        } else {
            socket_->open(*remoteEndpoint_);
        }

        socket_->addConnectCallback([this](int error) {
            return onFramingSocketConnect(error);
        });
        socket_->addRecvCallback([this](std::string_view message) {
            return onFramingSocketRecv(message);
        });
    } else {
        loop_->postAndWait([this] {
            open();
        });
    }

    State oldState = state_;
    state_ = State::kOpened;
    LOG(info, "{} -> {}", oldState, state_);
}

void Requester::send(std::string_view message) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(state_ == State::kOpened);

        if (int error = socket_->send(message)) {
            LOG(warning, "send: error={}", strerrorname_np(error));
        }
    } else {
        loop_->post([this, message = std::string(message)] {
            send(message);
        });
    }
}

bool Requester::onFramingSocketConnect(int error) {
    LOG(debug, "");

    if (!error) {
        if (!connectCallbackExecutor_) {
            dispatchConnect();
        } else {
            connectCallbackExecutor_->post([this] {
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
        recvCallbackExecutor_->post([this, message = std::string(message)] {
            dispatchRecv(message);
        });
    }

    return true;
}
