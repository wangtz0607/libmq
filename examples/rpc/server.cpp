// SPDX-License-Identifier: MIT

#include <charconv>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <print>
#include <string>
#include <string_view>

#include "mq/event/EventLoop.h"
#include "mq/net/Endpoint.h"
#include "mq/net/TcpEndpoint.h"
#include "mq/rpc/RpcServer.h"
#include "mq/utils/Logging.h"
#include "mq/utils/ThreadPool.h"

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kInfo);

    mq::EventLoop loop;
    mq::ThreadPool pool;

    mq::RpcServer server(&loop, mq::TcpEndpoint("0.0.0.0", 9999));

    server.registerMethod("increment", [](const mq::Endpoint &remoteEndpoint, std::string_view message) {
        std::println("{}: {}", remoteEndpoint, message);

        int value;
        auto [ptr, ec] = std::from_chars(message.data(), message.data() + message.size(), value);

        if (ptr != message.data() + message.size() || ec != std::errc()) {
            return std::string("invalid");
        }

        if (value == std::numeric_limits<int>::max()) {
            return std::string("overflow");
        }

        return std::to_string(value + 1);
    }, &pool);

    server.open();

    loop.run();
}
