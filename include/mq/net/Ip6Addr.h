// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <string>

#include "mq/utils/Hash.h"
#include "mq/utils/ZStringView.h"

namespace mq {

class Ip6Addr {
public:
    using bytes_type = std::array<uint8_t, 16>;

    Ip6Addr()
        : addr_() {}

    explicit Ip6Addr(const uint8_t *addr);

    explicit Ip6Addr(const bytes_type &addr)
        : Ip6Addr(addr.data()) {}

    explicit Ip6Addr(ZStringView addr);

    bytes_type bytes() const {
        return addr_;
    }

    std::string string() const;

private:
    bytes_type addr_;
};

inline bool operator==(Ip6Addr lhs, Ip6Addr rhs) {
    return lhs.bytes() == rhs.bytes();
}

} // namespace mq

template <>
struct std::hash<mq::Ip6Addr> {
    size_t operator()(mq::Ip6Addr addr) const noexcept {
        size_t seed = 0;
        for (uint8_t byte : addr.bytes()) {
            mq::hash_combine(seed, byte);
        }
        return seed;
    }
};

template <>
struct std::formatter<mq::Ip6Addr> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::Ip6Addr addr, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", addr.string());
    }
};
