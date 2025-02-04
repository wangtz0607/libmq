#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <print>
#include <thread>

#include "mq/event/EventLoop.h"
#include "mq/message/Publisher.h"
#include "mq/net/TCPV4Endpoint.h"
#include "mq/utils/Check.h"
#include "mq/utils/Logging.h"

#define TAG "main"

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kWarning);

    mq::EventLoop *loop = mq::EventLoop::background();

    mq::Publisher publisher(loop, mq::TCPV4Endpoint("0.0.0.0", 9999));

    CHECK(publisher.open() == 0);

    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        publisher.send(std::format("time: {}", std::chrono::system_clock::now()));
    }
}
