#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <string>

#include "mq/utils/Hash.h"

namespace mq {

class IPV6Address {
public:
    using Bytes = std::array<uint8_t, 16>;

    IPV6Address()
        : addr_() {}

    explicit IPV6Address(const uint8_t *addr);

    explicit IPV6Address(const Bytes &addr)
        : IPV6Address(addr.data()) {}

    explicit IPV6Address(const char *addr);

    explicit IPV6Address(const std::string &addr)
        : IPV6Address(addr.c_str()) {}

    Bytes bytes() const {
        return addr_;
    }

    std::string string() const;

private:
    Bytes addr_;
};

inline bool operator==(IPV6Address lhs, IPV6Address rhs) {
    return lhs.bytes() == rhs.bytes();
}

} // namespace mq

template <>
struct std::hash<mq::IPV6Address> {
    size_t operator()(mq::IPV6Address addr) const noexcept {
        size_t seed = 0;
        for (uint8_t byte : addr.bytes()) {
            mq::hash_combine(seed, byte);
        }
        return seed;
    }
};

template <>
struct std::formatter<mq::IPV6Address> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::IPV6Address addr, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", addr.string());
    }
};
