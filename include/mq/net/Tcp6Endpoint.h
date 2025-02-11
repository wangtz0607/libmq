// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "mq/net/Endpoint.h"
#include "mq/net/Ip6Addr.h"
#include "mq/net/NetworkInterface.h"

namespace mq {

class Tcp6Endpoint final : public Endpoint {
public:
    Tcp6Endpoint(Ip6Addr hostAddr, NetworkInterface interface, uint16_t port);

    Tcp6Endpoint(Ip6Addr hostAddr, uint16_t port)
        : Tcp6Endpoint(hostAddr, NetworkInterface(), port) {}

    Tcp6Endpoint(std::string_view host, uint16_t port);

    explicit Tcp6Endpoint(const sockaddr_in6 &addr)
        : addr_(addr) {}

    Ip6Addr hostAddr() const;
    NetworkInterface interface() const;
    uint16_t port() const;

    sa_family_t domain() const override {
        return AF_INET6;
    }

    const sockaddr *data() const override {
        return reinterpret_cast<const sockaddr *>(&addr_);
    }

    socklen_t size() const override {
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
