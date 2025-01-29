#include "mq/message/Publisher.h"

#include <cstring>
#include <memory>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingAcceptor.h"
#include "mq/net/FramingSocket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "Publisher"

using namespace mq;

Publisher::Publisher(EventLoop *loop, const Endpoint &localEndpoint, Params params)
    : loop_(loop), localEndpoint_(localEndpoint.clone()), params_(params) {
    LOG(debug, "");
}

Publisher::~Publisher() {
    LOG(debug, "");

    CHECK(state_ == State::kClosed);
}

int Publisher::open() {
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

    if (sockets_.size() >= params_.maxConnections) {
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
