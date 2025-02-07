#include "mq/utils/Logging.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <print>
#include <string_view>

#include <unistd.h>

using namespace mq;

namespace mq::detail {

FILE *sink_ = stderr;
Level level_ = Level::kInfo;

} // namespace mq::detail

namespace {

constexpr const char *name(Level level) {
    using enum Level;
    switch (level) {
        case kDebug: return "debug";
        case kInfo: return "info";
        case kWarning: return "warning";
        case kError: return "error";
        default: return nullptr;
    }
}

constexpr const char *style(Level level) {
    using enum Level;
    switch (level) {
        case kDebug: return "\033[1;39m";
        case kInfo: return "\033[1;36m";
        case kWarning: return "\033[1;33m";
        case kError: return "\033[1;31m";
        default: return nullptr;
    }
}

constexpr const char *kResetStyle = "\033[0m";

} // namespace

void mq::detail::log(Level level,
                     std::string_view tag,
                     std::string_view function,
                     std::string_view file,
                     int line,
                     std::string_view message) {
    const char *begin = "", *end = "";

    if (isatty(fileno(sink_))) {
        begin = style(level);
        end = kResetStyle;
    }

    std::println(sink_,
                 "{}{:%FT%TZ}: {}: {}: {} ({}, {}:{}){}",
                 begin,
                 std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()),
                 name(level),
                 tag,
                 message,
                 function,
                 std::filesystem::path(file).filename().string(),
                 line,
                 end);
}
