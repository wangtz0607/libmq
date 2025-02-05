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

using namespace std::chrono_literals;

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kWarning);

    mq::EventLoop *loop = mq::EventLoop::background();

    mq::Publisher publisher(loop, mq::TCPV4Endpoint("0.0.0.0", 9999));

    CHECK(publisher.open() == 0);

    for (;;) {
        publisher.send(std::format("time: {}", std::chrono::system_clock::now()));
        std::this_thread::sleep_for(1s);
    }
}
