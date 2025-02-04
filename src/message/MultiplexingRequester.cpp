#include "mq/message/MultiplexingRequester.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Executor.h"
#include "mq/utils/Logging.h"

#define TAG "MultiplexingRequester"

using namespace mq;

uint64_t MultiplexingRequester::nextRequestId_(0);

MultiplexingRequester::MultiplexingRequester(EventLoop *loop, const Endpoint &remoteEndpoint)
    : requester_(loop, remoteEndpoint) {
    LOG(debug, "");

    requester_.setRecvCallback([this](std::string_view message) {
        onRequesterRecv(message);
    });
}

void MultiplexingRequester::send(std::string message, RecvCallback recvCallback, Executor *recvCallbackExecutor) {
    LOG(debug, "");

    if (loop()->isInLoopThread()) {
        CHECK(state() == State::kOpened);

        uint64_t requestId = nextRequestId_++;
        requestId = toLittleEndian(requestId);

        requests_.emplace(requestId, std::pair(std::move(recvCallback), recvCallbackExecutor));

        message.insert(0, reinterpret_cast<const char *>(&requestId), 8);
        requester_.send(message);
    } else {
        loop()->post([this,
                      message = std::move(message),
                      recvCallback = std::move(recvCallback),
                      recvCallbackExecutor] mutable {
            send(std::move(message), std::move(recvCallback), recvCallbackExecutor);
        });
    }
}

void MultiplexingRequester::onRequesterRecv(std::string_view message) {
    LOG(debug, "");

    if (message.size() < 8) {
        LOG(warning, "Bad reply");

        return;
    }

    uint64_t requestId;
    memcpy(&requestId, message.data(), 8);
    requestId = fromLittleEndian(requestId);

    if (auto i = requests_.find(requestId); i != requests_.end()) {
        RecvCallback recvCallback = std::move(i->second.first);
        Executor *recvCallbackExecutor = i->second.second;

        requests_.erase(i);

        if (!recvCallbackExecutor) {
            recvCallback(message.substr(8));
        } else {
            recvCallbackExecutor->post([recvCallback = std::move(recvCallback), message = std::string(message)] mutable {
                recvCallback(message.substr(8));
            });
        }
    } else {
        LOG(warning, "Unknown request");

        return;
    }
}
