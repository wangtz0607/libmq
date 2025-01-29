#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <string>

namespace mq {

class IPV4Host {
public:
    explicit IPV4Host(uint32_t host)
        : host_(host) {}

    explicit IPV4Host(const char* host);

    explicit IPV4Host(const std::string &host)
        : IPV4Host(host.c_str()) {}

    uint32_t binary() const {
        return host_;
    }

    std::string string() const;

private:
    uint32_t host_;
};

inline bool operator==(IPV4Host lhs, IPV4Host rhs) {
    return lhs.binary() == rhs.binary();
}

} // namespace mq

template <>
struct std::hash<mq::IPV4Host> {
    size_t operator()(mq::IPV4Host host) const noexcept {
        return std::hash<uint32_t>{}(host.binary());
    }
};

template <>
struct std::formatter<mq::IPV4Host> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::IPV4Host host, FormatContext &ctx) const {
        return format_to(ctx.out(), "{}", host.string());
    }
};
