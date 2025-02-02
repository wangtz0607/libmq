#pragma once

#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <string>

#include "mq/utils/Hash.h"

namespace mq {

class IPV6Host {
public:
    using Bytes = std::array<uint8_t, 16>;

    explicit IPV6Host(const uint8_t *host);

    explicit IPV6Host(const Bytes &host)
        : IPV6Host(host.data()) {}

    explicit IPV6Host(const char *host);

    explicit IPV6Host(const std::string &host)
        : IPV6Host(host.c_str()) {}

    Bytes bytes() const {
        return host_;
    }

    std::string string() const;

private:
    Bytes host_;
};

inline bool operator==(IPV6Host lhs, IPV6Host rhs) {
    return lhs.bytes() == rhs.bytes();
}

} // namespace mq

template <>
struct std::hash<mq::IPV6Host> {
    size_t operator()(mq::IPV6Host host) const noexcept {
        size_t seed = 0;
        for (uint8_t byte : host.bytes()) {
            mq::hash_combine(seed, byte);
        }
        return seed;
    }
};

template <>
struct std::formatter<mq::IPV6Host> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::IPV6Host host, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", host.string());
    }
};
