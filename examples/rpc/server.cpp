#include <charconv>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string_view>

#include "mq/event/EventLoop.h"
#include "mq/net/TCPV4Endpoint.h"
#include "mq/rpc/RPCServer.h"
#include "mq/utils/Logging.h"
#include "mq/utils/ThreadPool.h"

int main() {
    mq::setLogSink(stderr);
    mq::setLogLevel(mq::Level::kWarning);

    mq::EventLoop loop;
    mq::ThreadPool pool; // Optional

    mq::RPCServer server(&loop, mq::TCPV4Endpoint("0.0.0.0", 9999));

    server.registerMethod("increment", [](std::string_view message) {
        int value;
        auto [ptr, ec] = std::from_chars(message.data(), message.data() + message.size(), value);

        if (ptr != message.data() + message.size() || ec != std::errc()) {
            return std::string("invalid");
        } else if (value == std::numeric_limits<int>::max()) {
            return std::string("overflow");
        } else {
            return std::to_string(value + 1);
        }
    }, &pool /* Optional */);

    server.open();

    loop.run();
}
