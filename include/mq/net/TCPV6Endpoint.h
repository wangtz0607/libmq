#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "mq/net/Endpoint.h"
#include "mq/net/IPV6Addr.h"
#include "mq/net/NetworkInterface.h"

namespace mq {

class TCPV6Endpoint final : public Endpoint {
public:
    TCPV6Endpoint(IPV6Addr hostAddr, uint16_t port);

    TCPV6Endpoint(IPV6Addr hostAddr, NetworkInterface interface, uint16_t port);

    TCPV6Endpoint(const std::string &hostAddrAndInterface, uint16_t port);

    explicit TCPV6Endpoint(const sockaddr_in6 &addr)
        : addr_(addr) {}

    IPV6Addr hostAddr() const;
    NetworkInterface interface() const;
    uint16_t port() const;

    int domain() const override {
        return AF_INET6;
    }

    const sockaddr *addr() const override {
        return reinterpret_cast<const sockaddr *>(&addr_);
    }

    socklen_t addrLen() const override {
        return sizeof(addr_);
    }

    std::string format() const override;
    std::unique_ptr<Endpoint> clone() const override;

protected:
    bool equals(const Endpoint &other) const override;
    size_t hashCode() const noexcept override;

private:
    sockaddr_in6 addr_;
};

} // namespace mq
