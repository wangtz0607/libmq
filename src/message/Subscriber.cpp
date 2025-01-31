#include "mq/message/Subscriber.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "mq/net/Endpoint.h"
#include "mq/net/FramingSocket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "Subscriber"

using namespace mq;

Subscriber::Subscriber(EventLoop *loop, Params params)
    : loop_(loop),
      params_(params) {
    LOG(debug, "");
}

Subscriber::~Subscriber() {
    LOG(debug, "");

    CHECK(state_ == State::kClosed);
}

void Subscriber::setConnectCallback(ConnectCallback connectCallback) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        connectCallback_ = std::move(connectCallback);
    } else {
        loop_->postAndWait([this, &connectCallback] {
            connectCallback_ = std::move(connectCallback);
        });
    }
}

void Subscriber::setRecvCallback(RecvCallback recvCallback) {
    LOG(debug, "");

    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        recvCallback_ = std::move(recvCallback);
    } else {
        loop_->postAndWait([this, &recvCallback] {
            recvCallback_ = std::move(recvCallback);
        });
    }
}

void Subscriber::setConnectCallbackExecutor(Executor *connectCallbackExecutor) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        connectCallbackExecutor_ = connectCallbackExecutor;
    } else {
        loop_->postAndWait([this, connectCallbackExecutor] {
            connectCallbackExecutor_ = connectCallbackExecutor;
        });
    }
}

void Subscriber::setRecvCallbackExecutor(Executor *recvCallbackExecutor) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        recvCallbackExecutor_ = recvCallbackExecutor;
    } else {
        loop_->postAndWait([this, recvCallbackExecutor] {
            recvCallbackExecutor_ = recvCallbackExecutor;
        });
    }
}

void Subscriber::dispatchConnect(const Endpoint &remoteEndpoint) {
    LOG(debug, "");

    if (connectCallback_) {
        connectCallback_(remoteEndpoint);
    }
}

void Subscriber::dispatchRecv(const Endpoint &remoteEndpoint, std::string_view message) {
    LOG(debug, "");

    if (recvCallback_) {
        recvCallback_(remoteEndpoint, message);
    }
}

Subscriber::State Subscriber::state() const {
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

void Subscriber::subscribe(const Endpoint &remoteEndpoint, std::vector<std::string> topics) {
    LOG(debug, "");

    if (loop_->isInLoopThread()) {
        CHECK(sockets_.find(remoteEndpoint) == sockets_.end());

        if (sockets_.empty()) {
            State oldState = state_;
            state_ = State::kOpened;
            LOG(info, "{} -> {}", oldState, state_);
        }

        std::unique_ptr<FramingSocket> socket = std::make_unique<FramingSocket>(loop_, params_.framingSocket);

        socket->addConnectCallback([this, socket = socket.get()](int error) {
            return onFramingSocketConnect(socket, error);
        });
        socket->addRecvCallback([this, socket = socket.get()](std::string_view message) {
            return onFramingSocketRecv(socket, message);
        });

        enableAutoReconnectAndOpen(*socket, remoteEndpoint, params_.reconnectInterval);

        topics_.emplace(socket.get(), std::move(topics));
        sockets_.emplace(remoteEndpoint.clone(), std::move(socket));
    } else {
        loop_->postAndWait([this, &remoteEndpoint, &topics] {
            subscribe(remoteEndpoint, std::move(topics));
        });
    }
}

void Subscriber::unsubscribe(const Endpoint &remoteEndpoint) {
    LOG(debug, "");

    auto i = sockets_.find(remoteEndpoint);
    CHECK(i != sockets_.end());

    std::unique_ptr<FramingSocket> socket = std::move(i->second);
    topics_.erase(i->second.get());
    sockets_.erase(i);

    loop_->post([this, socket = std::move(socket)] {
        socket->reset();
    });

    if (sockets_.empty()) {
        State oldState = state_;
        state_ = State::kClosed;
        LOG(info, "{} -> {}", oldState, state_);
    }
}

bool Subscriber::onFramingSocketConnect(FramingSocket *socket, int error) {
    LOG(debug, "");

    if (!error) {
        if (!connectCallbackExecutor_) {
            dispatchConnect(*socket->remoteEndpoint());
        } else {
            connectCallbackExecutor_->post([this, remoteEndpoint = socket->remoteEndpoint()] {
                dispatchConnect(*remoteEndpoint);
            });
        }
    }

    return true;
}

bool Subscriber::onFramingSocketRecv(FramingSocket *socket, std::string_view message) {
    LOG(debug, "");

    if (!recvCallbackExecutor_) {
        if (auto i = topics_.find(socket); i != topics_.end()) {
            for (const std::string &topic : i->second) {
                if (message.starts_with(topic)) {
                    dispatchRecv(*socket->remoteEndpoint(), message);
                    break;
                }
            }
        }
    } else {
        recvCallbackExecutor_->post([this, socket, remoteEndpoint = socket->remoteEndpoint(), message = std::string(message)] {
            if (auto i = topics_.find(socket); i != topics_.end()) {
                for (const std::string &topic : i->second) {
                    if (message.starts_with(topic)) {
                        dispatchRecv(*remoteEndpoint, message);
                        break;
                    }
                }
            }
        });
    }

    return true;
}
