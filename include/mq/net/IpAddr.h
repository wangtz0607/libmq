// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <string>

namespace mq {

class IpAddr {
public:
    using uint_type = uint32_t;

    explicit IpAddr()
        : addr_(0) {}

    explicit IpAddr(uint_type addr)
        : addr_(addr) {}

    explicit IpAddr(const char* addr);

    explicit IpAddr(const std::string &addr)
        : IpAddr(addr.c_str()) {}

    uint_type uint() const {
        return addr_;
    }

    std::string string() const;

private:
    uint_type addr_;
};

inline bool operator==(IpAddr lhs, IpAddr rhs) {
    return lhs.uint() == rhs.uint();
}

} // namespace mq

template <>
struct std::hash<mq::IpAddr> {
    size_t operator()(mq::IpAddr addr) const noexcept {
        return std::hash<mq::IpAddr::uint_type>{}(addr.uint());
    }
};

template <>
struct std::formatter<mq::IpAddr> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::IpAddr addr, FormatContext &ctx) const {
        return format_to(ctx.out(), "{}", addr.string());
    }
};
