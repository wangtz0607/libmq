// SPDX-License-Identifier: MIT

#include "mq/utils/Logging.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include <unistd.h>

using namespace mq;

namespace mq::detail {

FILE *sink_ = stderr;
Level level_ = Level::kInfo;

} // namespace mq::detail

void mq::detail::log(FILE *sink,
                     Level level,
                     std::string_view tag,
                     std::string_view function,
                     std::string_view file,
                     int line,
                     std::string_view message) {
    using enum Level;

    const char *levelName;

    switch (level) {
        case kDebug: levelName = "debug"; break;
        case kInfo: levelName = "info"; break;
        case kWarning: levelName = "warning"; break;
        case kError: levelName = "error"; break;
    }

    const char *setStyle, *resetStyle;

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
        std::format("{}{:%FT%TZ}: {}: {}: {} ({}, {}:{}){}\n",
                    setStyle,
                    std::chrono::system_clock::now(),
                    levelName,
                    tag,
                    message,
                    function,
                    std::filesystem::path(file).filename().string(),
                    line,
                    resetStyle);

    fwrite(logMessage.c_str(), 1, logMessage.size(), sink);
}
