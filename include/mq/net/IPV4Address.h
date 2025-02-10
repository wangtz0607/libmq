// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <string>

namespace mq {

class IPV4Address {
public:
    using uint_type = uint32_t;

    explicit IPV4Address()
        : addr_(0) {}

    explicit IPV4Address(uint_type addr)
        : addr_(addr) {}

    explicit IPV4Address(const char* addr);

    explicit IPV4Address(const std::string &addr)
        : IPV4Address(addr.c_str()) {}

    uint_type uint() const {
        return addr_;
    }

    std::string string() const;

private:
    uint_type addr_;
};

inline bool operator==(IPV4Address lhs, IPV4Address rhs) {
    return lhs.uint() == rhs.uint();
}

} // namespace mq

template <>
struct std::hash<mq::IPV4Address> {
    size_t operator()(mq::IPV4Address addr) const noexcept {
        return std::hash<mq::IPV4Address::uint_type>{}(addr.uint());
    }
};

template <>
struct std::formatter<mq::IPV4Address> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::IPV4Address addr, FormatContext &ctx) const {
        return format_to(ctx.out(), "{}", addr.string());
    }
};
