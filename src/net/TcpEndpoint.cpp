// SPDX-License-Identifier: MIT

#include "mq/net/TcpEndpoint.h"

#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "mq/net/Endpoint.h"
#include "mq/net/IpAddr.h"
#include "mq/utils/Hash.h"

#define TAG "TcpEndpoint"

using namespace mq;

TcpEndpoint::TcpEndpoint(IpAddr hostAddr, uint16_t port) : addr_{} {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = htonl(hostAddr.uint());
}

IpAddr TcpEndpoint::hostAddr() const {
    return IpAddr(ntohl(addr_.sin_addr.s_addr));
}

uint16_t TcpEndpoint::port() const {
    return ntohs(addr_.sin_port);
}

std::string TcpEndpoint::format() const {
    return std::format("tcp://{}:{}", hostAddr(), port());
}

std::unique_ptr<Endpoint> TcpEndpoint::clone() const {
    return std::make_unique<TcpEndpoint>(addr_);
}

bool TcpEndpoint::equals(const Endpoint &other) const {
    if (domain() != other.domain()) return false;
    const TcpEndpoint &castOther = static_cast<const TcpEndpoint &>(other);
    return hostAddr() == castOther.hostAddr() && port() == castOther.port();
}

size_t TcpEndpoint::hashCode() const noexcept {
    size_t seed = 0;
    hash_combine(seed, hostAddr());
    hash_combine(seed, port());
    return seed;
}
