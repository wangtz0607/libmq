// SPDX-License-Identifier: MIT

#include "mq/utils/Logging.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <format>
#include <string>
#include <string_view>

#include <unistd.h>

using namespace mq;

using enum Level;

namespace mq::detail {

std::atomic<FILE *> sink_ = stderr;
std::atomic<Level> level_ = kInfo;

} // namespace mq::detail

void mq::detail::log(FILE *sink,
                     Level level,
                     std::string_view tag,
                     std::string_view function,
                     std::string_view file,
                     int line,
                     std::string_view message) {
    const thread_local pid_t kThreadId = gettid();

    const char *levelName;

    switch (level) {
        case kDebug: levelName = "debug"; break;
        case kInfo: levelName = "info"; break;
        case kWarning: levelName = "warning"; break;
        case kError: levelName = "error"; break;
    }

    const char *setStyle,
               *resetStyle;

    if (isatty(fileno(sink))) {
        switch (level) {
            case kDebug: setStyle = "\033[1;39m"; break;
            case kInfo: setStyle = "\033[1;36m"; break;
            case kWarning: setStyle = "\033[1;33m"; break;
            case kError: setStyle = "\033[1;31m"; break;
        }

        resetStyle = "\033[0m";
    } else {
        setStyle = "";
        resetStyle = "";
    }

    std::string logMessage =
        std::format("{}{:%FT%TZ}: {}: {}: {}: {} ({}, {}:{}){}\n",
                    setStyle,
                    std::chrono::system_clock::now(),
                    kThreadId,
                    levelName,
                    tag,
                    message,
                    function,
                    file,
                    line,
                    resetStyle);

    fwrite(logMessage.c_str(), 1, logMessage.size(), sink);
}
