#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <variant>

namespace mq {

class StringOrView {
public:
    constexpr StringOrView() : value_(std::string_view()) {}

    constexpr StringOrView(std::string &&value) : value_(std::move(value)) {}

    constexpr StringOrView(const std::string &value) : value_(std::string_view(value)) {}

    constexpr StringOrView(std::string_view value) : value_(value) {}

    constexpr StringOrView(const char *value) : value_(std::string_view(value)) {}

    constexpr StringOrView(const char *value, size_t size) : value_(std::string_view(value, size)) {}

    constexpr operator std::string() const & {
        return std::visit([](const auto &value) { return std::string(value); }, value_);
    }

    constexpr operator std::string() && {
        return std::visit([](auto &value) { return std::string(std::move(value)); }, value_);
    }

    constexpr operator std::string_view() const {
        return std::visit([](const auto &value) { return std::string_view(value); }, value_);
    }

    constexpr const char *data() const {
        return std::visit([](const auto &value) { return value.data(); }, value_);
    }

    constexpr size_t size() const {
        return std::visit([](const auto &value) { return value.size(); }, value_);
    }

private:
    std::variant<std::string, std::string_view> value_;
};

} // namespace mq

template <>
struct std::hash<mq::StringOrView> {
    constexpr size_t operator()(const mq::StringOrView &value) const noexcept {
        return std::hash<std::string_view>{}(std::string_view(value));
    }
};

template <>
struct std::formatter<mq::StringOrView> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const mq::StringOrView &value, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", std::string_view(value));
    }
};
