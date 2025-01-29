#include <chrono>
#include <cstdio>
#include <print>
#include <string_view>
#include <thread>

#include "mq/event/EventLoop.h"
#include "mq/message/Requester.h"
#include "mq/net/TCPV4Endpoint.h"
#include "mq/utils/Logging.h"
#include "mq/utils/ThreadPool.h"

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kWarning);

    mq::EventLoop *loop = mq::EventLoop::background();
    mq::ThreadPool pool;

    mq::Requester requester(loop, &pool, mq::TCPV4Endpoint("127.0.0.1", 9999));

    requester.setRecvCallback([](std::string_view message) {
        std::println("{}", message);
    });

    requester.open();

    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        requester.send("World");
    }
}
