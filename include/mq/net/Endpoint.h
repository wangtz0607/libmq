#pragma once

#include <concepts>
#include <format>
#include <functional>
#include <memory>
#include <string>

#include <sys/socket.h>

namespace mq {

class Endpoint {
public:
    virtual ~Endpoint() = default;

    Endpoint() = default;

    Endpoint(const Endpoint &) = delete;
    Endpoint(Endpoint &&) = delete;

    Endpoint &operator=(const Endpoint &) = delete;
    Endpoint &operator=(Endpoint &&) = delete;

    virtual int domain() const = 0;
    virtual const struct sockaddr *addr() const = 0;
    virtual socklen_t addrLen() const = 0;
    virtual std::string format() const = 0;
    virtual std::unique_ptr<Endpoint> clone() const = 0;

protected:
    virtual bool equals(const Endpoint &other) const = 0;
    virtual size_t hashCode() const noexcept = 0;

    friend bool operator==(const Endpoint &lhs, const Endpoint &rhs);
    friend struct std::hash<Endpoint>;
};

inline bool operator==(const Endpoint &lhs, const Endpoint &rhs) {
    return lhs.equals(rhs);
}

} // namespace mq

template <typename EndpointT>
    requires std::derived_from<EndpointT, mq::Endpoint>
struct std::hash<EndpointT> {
    size_t operator()(const EndpointT &endpoint) const noexcept {
        return endpoint.hashCode();
    }
};

template <typename EndpointT>
    requires std::derived_from<EndpointT, mq::Endpoint>
struct std::formatter<EndpointT> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const EndpointT &endpoint, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", endpoint.format());
    }
};
