#pragma once

#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <string>

#include "mq/utils/Hash.h"

namespace mq {

class IPV6Addr {
public:
    using Bytes = std::array<uint8_t, 16>;

    IPV6Addr()
        : addr_() {}

    explicit IPV6Addr(const uint8_t *addr);

    explicit IPV6Addr(const Bytes &addr)
        : IPV6Addr(addr.data()) {}

    explicit IPV6Addr(const char *addr);

    explicit IPV6Addr(const std::string &addr)
        : IPV6Addr(addr.c_str()) {}

    Bytes bytes() const {
        return addr_;
    }

    std::string string() const;

private:
    Bytes addr_;
};

inline bool operator==(IPV6Addr lhs, IPV6Addr rhs) {
    return lhs.bytes() == rhs.bytes();
}

} // namespace mq

template <>
struct std::hash<mq::IPV6Addr> {
    size_t operator()(mq::IPV6Addr addr) const noexcept {
        size_t seed = 0;
        for (uint8_t byte : addr.bytes()) {
            mq::hash_combine(seed, byte);
        }
        return seed;
    }
};

template <>
struct std::formatter<mq::IPV6Addr> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::IPV6Addr addr, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", addr.string());
    }
};
