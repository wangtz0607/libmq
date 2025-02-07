#include <cstdio>
#include <print>

#include "mq/event/EventLoop.h"
#include "mq/message/Subscriber.h"
#include "mq/net/Endpoint.h"
#include "mq/net/TCPV4Endpoint.h"
#include "mq/utils/Logging.h"
#include "mq/utils/ThreadPool.h"

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kInfo);

    mq::EventLoop loop;
    mq::ThreadPool pool;

    mq::Subscriber subscriber(&loop);

    subscriber.setRecvCallback([](const mq::Endpoint &remoteEndpoint, std::string_view message) {
        std::println("{}: {}", remoteEndpoint, message);
    });

    subscriber.setRecvCallbackExecutor(&pool);

    subscriber.subscribe(mq::TCPV4Endpoint("127.0.0.1", 9999), {"time"});

    loop.run();
}
