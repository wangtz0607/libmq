#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <print>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingReplier.h"
#include "mq/net/Endpoint.h"
#include "mq/net/TCPV4Endpoint.h"
#include "mq/utils/Logging.h"
#include "mq/utils/ThreadPool.h"

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kWarning);

    mq::EventLoop loop;
    mq::ThreadPool pool;

    mq::MultiplexingReplier replier(&loop, mq::TCPV4Endpoint("0.0.0.0", 9999));

    replier.setRecvCallback(
        [](const mq::Endpoint &remoteEndpoint, std::string_view message, mq::MultiplexingReplier::Promise promise) {
            std::println("{}: {}", remoteEndpoint, message);

            promise(std::format("Hello, {}!", message));
        });

    replier.setRecvCallbackExecutor(&pool);

    if (int error = replier.open()) {
        std::println(stderr, "error: {}", strerror(error));
        exit(1);
    }

    loop.run();
}
