#include "mq/net/FramingAcceptor.h"

#include "mq/event/EventLoop.h"
#include "mq/net/Acceptor.h"
#include "mq/net/Endpoint.h"
#include "mq/net/FramingSocket.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "FramingAcceptor"

using namespace mq;

FramingAcceptor::FramingAcceptor(EventLoop *loop, Params params)
    : loop_(loop), params_(params) {
    LOG(debug, "");
}

FramingAcceptor::~FramingAcceptor() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());
    CHECK(loop_->state() == EventLoop::State::kTask);
    CHECK(state_ == State::kClosed);
}

FramingAcceptor::State FramingAcceptor::state() {
    CHECK(loop_->isInLoopThread());

    return state_;
}

Acceptor &FramingAcceptor::acceptor() {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return *acceptor_;
}

const Acceptor &FramingAcceptor::acceptor() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return *acceptor_;
}

std::unique_ptr<Endpoint> FramingAcceptor::localEndpoint() const {
    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kListening);

    return localEndpoint_->clone();
}

bool FramingAcceptor::hasAcceptCallback() const {
    CHECK(loop_->isInLoopThread());

    return !acceptCallbacks_.empty();
}

void FramingAcceptor::addAcceptCallback(AcceptCallback acceptCallback) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.emplace_back(std::move(acceptCallback));
}

void FramingAcceptor::clearAcceptCallbacks() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    acceptCallbacks_.clear();
}

void FramingAcceptor::dispatchAccept(std::unique_ptr<FramingSocket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    CHECK(acceptCallbacks_.size() == 1);

    AcceptCallback acceptCallback(std::move(acceptCallbacks_.front()));
    acceptCallbacks_.clear();

    if (acceptCallback(std::move(socket), remoteEndpoint)) {
        acceptCallbacks_.emplace_back(std::move(acceptCallback));
    }
}

int FramingAcceptor::open(const Endpoint &localEndpoint) {
    LOG(debug, "localEndpoint={}", localEndpoint);

    CHECK(loop_->isInLoopThread());
    CHECK(state_ == State::kClosed);

    Acceptor::Params params{params_.reuseAddr, params_.framingSocket.socket};
    acceptor_ = std::make_unique<Acceptor>(loop_, params);
    acceptor_->addAcceptCallback([this](std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint) {
        return onAcceptorAccept(std::move(socket), remoteEndpoint);
    });
    if (int error = acceptor_->open(localEndpoint)) {
        return error;
    }

    localEndpoint_ = acceptor_->localEndpoint();

    State oldState = state_;
    state_ = State::kListening;
    LOG(info, "{} -> {}", oldState, state_);

    return 0;
}

void FramingAcceptor::close() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(info, "{} -> {}", oldState, state_);

    acceptor_->reset();

    loop_->post([acceptor = std::move(acceptor_)] {});

    acceptor_ = nullptr;

    localEndpoint_ = nullptr;
}

void FramingAcceptor::reset() {
    LOG(debug, "");

    CHECK(loop_->isInLoopThread());

    clearAcceptCallbacks();

    if (state_ == State::kClosed) return;

    State oldState = state_;
    state_ = State::kClosed;
    LOG(info, "{} -> {}", oldState, state_);

    acceptor_->reset();

    loop_->post([acceptor = std::move(acceptor_)] {});

    acceptor_ = nullptr;

    localEndpoint_ = nullptr;
}

bool FramingAcceptor::onAcceptorAccept(std::unique_ptr<Socket> socket, const Endpoint &remoteEndpoint) {
    LOG(debug, "");

    std::unique_ptr<FramingSocket> framingSocket = std::make_unique<FramingSocket>(loop_, params_.framingSocket);
    framingSocket->open(std::move(socket), remoteEndpoint);

    dispatchAccept(std::move(framingSocket), remoteEndpoint);

    return true;
}
