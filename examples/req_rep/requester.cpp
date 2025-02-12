// SPDX-License-Identifier: MIT

#include <chrono>
#include <cstdio>
#include <print>
#include <string_view>

#include "mq/event/EventLoop.h"
#include "mq/message/Requester.h"
#include "mq/net/TcpEndpoint.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"
#include "mq/utils/ThreadPool.h"

#define TAG "main"

using namespace std::chrono_literals;

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kInfo);

    mq::EventLoop *loop = mq::EventLoop::background();
    mq::ThreadPool pool;

    mq::Requester requester(loop, mq::TcpEndpoint("127.0.0.1", 9999));

    requester.setRecvCallback([](std::string_view message) {
        std::println("{}", message);
    });

    requester.setRecvCallbackExecutor(&pool);

    requester.open();
    CHECK(requester.waitForConnected(30s) == 0);

    for (;;) {
        requester.send("World");
        std::this_thread::sleep_for(1s);
    }
}
