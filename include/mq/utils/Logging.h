// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <cstdio>
#include <format>
#include <string_view>

namespace mq {

enum class Level {
    kDebug,
    kInfo,
    kWarning,
    kError,
};

namespace detail {

extern std::atomic<FILE *> sink_;
extern std::atomic<Level> level_;

void log(FILE *sink,
         Level level,
         std::string_view tag,
         std::string_view function,
         std::string_view file,
         int line,
         std::string_view message);

template <typename... Args>
void log(FILE *sink,
         Level level,
         std::string_view tag,
         std::string_view function,
         std::string_view file,
         int line,
         std::format_string<Args...> fmt,
         Args &&...args) {
    log(sink, level, tag, function, file, line, std::format(fmt, std::forward<Args>(args)...));
}

} // namespace detail

inline FILE *getLogSink() {
    return detail::sink_.load(std::memory_order_relaxed);
}

inline Level getLogLevel() {
    return detail::level_.load(std::memory_order_relaxed);
}

inline void setLogSink(FILE *sink) {
    detail::sink_.store(sink, std::memory_order_relaxed);
}

inline void setLogLevel(Level level) {
    detail::level_.store(level, std::memory_order_relaxed);
}

} // namespace mq

template <>
struct std::formatter<mq::Level> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::Level level, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(level));
    }

private:
    static constexpr const char *name(mq::Level level) {
        using enum mq::Level;

        switch (level) {
            case kDebug: return "Debug";
            case kInfo: return "Info";
            case kWarning: return "Warning";
            case kError: return "Error";
            default: return nullptr;
        }
    }
};

#define LOG(level, ...) LOG_##level(__VA_ARGS__)

#define LOG_debug(...) LOG_LEVEL(mq::Level::kDebug, __VA_ARGS__)

#define LOG_info(...) LOG_LEVEL(mq::Level::kInfo, __VA_ARGS__)

#define LOG_warning(...) LOG_LEVEL(mq::Level::kWarning, __VA_ARGS__)

#define LOG_error(...) LOG_LEVEL(mq::Level::kError, __VA_ARGS__)

#define LOG_LEVEL(level, ...) \
    do { \
        FILE *sink_ = mq::detail::sink_.load(std::memory_order_relaxed); \
        mq::Level level_ = mq::detail::level_.load(std::memory_order_relaxed); \
        if (sink_ && level_ <= level) { \
            mq::detail::log( \
                sink_, \
                level, \
                TAG, \
                __FUNCTION__, \
                (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__), \
                __LINE__, \
                __VA_ARGS__); \
        } \
    } while (false)
