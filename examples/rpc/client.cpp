// SPDX-License-Identifier: MIT

#include <chrono>
#include <cstdio>
#include <future>
#include <print>
#include <string>

#include "mq/event/EventLoop.h"
#include "mq/net/TcpEndpoint.h"
#include "mq/rpc/RpcClient.h"
#include "mq/rpc/RpcError.h"
#include "mq/utils/Check.h"
#include "mq/utils/Expected.h"
#include "mq/utils/Logging.h"

#define TAG "main"

using namespace std::chrono_literals;

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kInfo);

    mq::EventLoop *loop = mq::EventLoop::background();

    mq::RpcClient client(loop, mq::TcpEndpoint("127.0.0.1", 9999));

    client.open();
    CHECK(client.waitForConnected(30s) == 0);

    std::future<mq::Expected<std::string, mq::RpcError>> future = client.call("increment", "42");

    mq::Expected<std::string, mq::RpcError> result = future.get();

    if (result) {
        std::println("{}", result.value());
    } else {
        std::println("error: {}", result.error());
    }

    client.close();
    return 0;
}
