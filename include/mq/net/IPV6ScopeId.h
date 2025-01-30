#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <string>

namespace mq {

class IPV6ScopeId {
public:
    IPV6ScopeId()
        : scopeId_(0) {}

    explicit IPV6ScopeId(uint32_t scopeId)
        : scopeId_(scopeId) {}

    explicit IPV6ScopeId(const char *ifName);

    explicit IPV6ScopeId(const std::string &ifName)
        : IPV6ScopeId(ifName.c_str()) {}

    explicit operator bool() const {
        return scopeId_ != 0;
    }

    uint32_t uint() const {
        return scopeId_;
    }

    std::string string() const;

private:
    uint32_t scopeId_;
};

inline bool operator==(IPV6ScopeId lhs, IPV6ScopeId rhs) {
    return lhs.uint() == rhs.uint();
}

} // namespace mq

template <>
struct std::hash<mq::IPV6ScopeId> {
    size_t operator()(mq::IPV6ScopeId scopeId) const noexcept {
        return std::hash<uint32_t>{}(scopeId.uint());
    }
};

template <>
struct std::formatter<mq::IPV6ScopeId> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::IPV6ScopeId scopeId, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", scopeId.string());
    }
};
