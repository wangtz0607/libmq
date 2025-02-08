#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <string>

namespace mq {

class NetworkInterface {
public:
    NetworkInterface()
        : index_(0) {}

    explicit NetworkInterface(uint32_t index)
        : index_(index) {}

    explicit NetworkInterface(const char *name);

    explicit NetworkInterface(const std::string &name)
        : NetworkInterface(name.c_str()) {}

    uint32_t index() const {
        return index_;
    }

    std::string name() const;

private:
    uint32_t index_;
};

inline bool operator==(NetworkInterface lhs, NetworkInterface rhs) {
    return lhs.index() == rhs.index();
}

} // namespace mq

template <>
struct std::hash<mq::NetworkInterface> {
    size_t operator()(mq::NetworkInterface interface) const noexcept {
        return std::hash<uint32_t>{}(interface.index());
    }
};

template <>
struct std::formatter<mq::NetworkInterface> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::NetworkInterface interface, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", interface.name());
    }
};
