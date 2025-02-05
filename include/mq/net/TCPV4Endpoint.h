#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "mq/net/Endpoint.h"
#include "mq/net/IPV4Address.h"

namespace mq {

class TCPV4Endpoint final : public Endpoint {
public:
    TCPV4Endpoint(IPV4Address hostAddr, uint16_t port);

    TCPV4Endpoint(const char *hostAddr, uint16_t port)
        : TCPV4Endpoint(IPV4Address(hostAddr), port) {}

    TCPV4Endpoint(const std::string &hostAddr, uint16_t port)
        : TCPV4Endpoint(IPV4Address(hostAddr), port) {}

    explicit TCPV4Endpoint(const sockaddr_in &addr)
        : addr_(addr) {}

    IPV4Address hostAddr() const;
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
