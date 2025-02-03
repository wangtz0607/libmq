#include <cstdio>
#include <print>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include "mq/event/EventLoop.h"
#include "mq/message/MultiplexingRequester.h"
#include "mq/net/TCPV4Endpoint.h"
#include "mq/utils/Logging.h"
#include "mq/utils/ThreadPool.h"

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kWarning);

    mq::EventLoop *loop = mq::EventLoop::background();
    mq::ThreadPool pool;

    mq::MultiplexingRequester requester(loop, mq::TCPV4Endpoint("127.0.0.1", 9999));

    requester.open();

    for (int i = 0;; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        mq::MultiplexingRequester::RecvCallback recvCallback = [i](std::string_view message) {
            std::println("{}: {}", i, message);
        };

        requester.send(std::to_string(i), std::move(recvCallback), &pool);
    }
}
