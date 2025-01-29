#include "mq/utils/Logging.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <print>
#include <string_view>

namespace mq::detail {

FILE *sink_ = stderr;
Level level_ = Level::kWarning;

} // namespace mq::detail

void mq::detail::log(Level level,
                     std::string_view tag,
                     std::string_view function,
                     std::string_view file,
                     int line,
                     std::string_view message) {
    std::println(sink_,
                 "{:%FT%TZ}: {}: {}: {} ({}, {}:{})",
                 std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()),
                 level,
                 tag,
                 message,
                 function,
                 std::filesystem::path(file).filename().string(),
                 line);
}
