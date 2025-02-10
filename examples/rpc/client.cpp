// SPDX-License-Identifier: MIT

#include <chrono>
#include <cstdio>
#include <future>
#include <print>
#include <string>

#include "mq/event/EventLoop.h"
#include "mq/net/TCPV4Endpoint.h"
#include "mq/rpc/RPCClient.h"
#include "mq/rpc/RPCError.h"
#include "mq/utils/Check.h"
#include "mq/utils/Expected.h"
#include "mq/utils/Logging.h"

#define TAG "main"

using namespace std::chrono_literals;

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kInfo);

    mq::EventLoop *loop = mq::EventLoop::background();

    mq::RPCClient client(loop, mq::TCPV4Endpoint("127.0.0.1", 9999));

    client.open();
    CHECK(client.waitForConnected(30s) == 0);

    std::future<mq::Expected<std::string, mq::RPCError>> future = client.call("increment", "42");

    mq::Expected<std::string, mq::RPCError> result = future.get();

    if (result) {
        std::println("{}", result.value());
    } else {
        std::println("error: {}", result.error());
    }

    client.close();
    return 0;
}
