// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "mq/net/Endpoint.h"
#include "mq/net/IpAddr.h"

namespace mq {

class TcpEndpoint final : public Endpoint {
public:
    TcpEndpoint(IpAddr hostAddr, uint16_t port);

    TcpEndpoint(const char *hostAddr, uint16_t port)
        : TcpEndpoint(IpAddr(hostAddr), port) {}

    TcpEndpoint(const std::string &hostAddr, uint16_t port)
        : TcpEndpoint(IpAddr(hostAddr), port) {}

    explicit TcpEndpoint(const sockaddr_in &addr)
        : addr_(addr) {}

    IpAddr hostAddr() const;
    uint16_t port() const;

    int domain() const override {
        return AF_INET;
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
    sockaddr_in addr_;
};

} // namespace mq
