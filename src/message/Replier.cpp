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

Replier::Replier(EventLoop *loop, const Endpoint &localEndpoint, Params params)
    : loop_(loop),
      localEndpoint_(localEndpoint.clone()),
      params_(params) {
    LOG(debug, "");
}

Replier::~Replier() {
    LOG(debug, "");

    CHECK(state_ == State::kClosed);
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
        acceptor_ = std::make_unique<FramingAcceptor>(loop_, params_.framingAcceptor);
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

    if (sockets_.size() >= params_.maxConnections) {
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

                loop_->post([this, socket = socket->weak_from_this()] {
                    if (socket.expired()) return;

                    socket.lock()->reset();

                    if (auto i = sockets_.find(socket.lock().get()); i != sockets_.end()) {
                        sockets_.erase(i);
                    }
                });
            }
        }
    } else {
        recvCallbackExecutor_->post([this, socket = socket->weak_from_this(), remoteEndpoint = std::move(remoteEndpoint), message = std::string(message)] {
            if (socket.expired()) return;

            std::optional<std::string> replyMessage = dispatchRecv(*remoteEndpoint, message);

            if (replyMessage) {
                loop_->post([this, socket, replyMessage = std::move(replyMessage)] {
                    if (socket.expired()) return;

                    if (int error = socket.lock()->send(*replyMessage)) {
                        LOG(warning, "send: error={}", strerrorname_np(error));

                        socket.lock()->reset();

                        loop_->post([this, socket] {
                            if (socket.expired()) return;

                            if (auto i = sockets_.find(socket.lock().get()); i != sockets_.end()) {
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
