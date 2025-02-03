#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <string>

namespace mq {

class IPV4Addr {
public:
    explicit IPV4Addr()
        : addr_(0) {}

    explicit IPV4Addr(uint32_t addr)
        : addr_(addr) {}

    explicit IPV4Addr(const char* addr);

    explicit IPV4Addr(const std::string &addr)
        : IPV4Addr(addr.c_str()) {}

    uint32_t uint() const {
        return addr_;
    }

    std::string string() const;

private:
    uint32_t addr_;
};

inline bool operator==(IPV4Addr lhs, IPV4Addr rhs) {
    return lhs.uint() == rhs.uint();
}

} // namespace mq

template <>
struct std::hash<mq::IPV4Addr> {
    size_t operator()(mq::IPV4Addr addr) const noexcept {
        return std::hash<uint32_t>{}(addr.uint());
    }
};

template <>
struct std::formatter<mq::IPV4Addr> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::IPV4Addr addr, FormatContext &ctx) const {
        return format_to(ctx.out(), "{}", addr.string());
    }
};
