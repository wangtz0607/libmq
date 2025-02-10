#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <variant>

namespace mq {

template <typename Char, typename Traits = std::char_traits<Char>>
class MaybeOwnedBasicString {
public:
    constexpr MaybeOwnedBasicString()
        : value_(std::basic_string_view<Char, Traits>()) {}

    constexpr MaybeOwnedBasicString(std::basic_string<Char, Traits> &&value)
        : value_(std::move(value)) {}

    constexpr MaybeOwnedBasicString(const std::basic_string<Char, Traits> &value)
        : value_(std::basic_string_view<Char, Traits>(value)) {}

    constexpr MaybeOwnedBasicString(std::basic_string_view<Char, Traits> value)
        : value_(value) {}

    constexpr MaybeOwnedBasicString(const char *value)
        : value_(std::basic_string_view<Char, Traits>(value)) {}

    constexpr MaybeOwnedBasicString(const char *value, size_t size)
        : value_(std::basic_string_view<Char, Traits>(value, size)) {}

    constexpr operator std::basic_string<Char, Traits>() const & {
        return std::visit([](const auto &value) { return std::basic_string<Char, Traits>(value); }, value_);
    }

    constexpr operator std::basic_string<Char, Traits>() && {
        return std::visit([](auto &value) { return std::basic_string<Char, Traits>(std::move(value)); }, value_);
    }

    constexpr operator std::basic_string_view<Char, Traits>() const {
        return std::visit([](const auto &value) { return std::basic_string_view<Char, Traits>(value); }, value_);
    }

    constexpr const char *data() const {
        return std::visit([](const auto &value) { return value.data(); }, value_);
    }

    constexpr size_t size() const {
        return std::visit([](const auto &value) { return value.size(); }, value_);
    }

private:
    std::variant<std::basic_string<Char, Traits>, std::basic_string_view<Char, Traits>> value_;
};

using MaybeOwnedString = MaybeOwnedBasicString<char>;

} // namespace mq

template <typename Char, typename Traits>
struct std::hash<mq::MaybeOwnedBasicString<Char, Traits>> {
    constexpr size_t operator()(const mq::MaybeOwnedBasicString<Char, Traits> &value) const noexcept {
        return std::hash<std::basic_string_view<Char, Traits>>{}(std::basic_string_view<Char, Traits>(value));
    }
};

template <typename Char, typename Traits>
struct std::formatter<mq::MaybeOwnedBasicString<Char, Traits>> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const mq::MaybeOwnedBasicString<Char, Traits> &value, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", std::basic_string_view<Char, Traits>(value));
    }
};
