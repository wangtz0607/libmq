#include "mq/net/TCPV4Endpoint.h"

#include <cstdint>
#include <format>
#include <memory>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "mq/net/Endpoint.h"
#include "mq/net/IPV4Address.h"
#include "mq/utils/Hash.h"

#define TAG "TCPV4Endpoint"

using namespace mq;

TCPV4Endpoint::TCPV4Endpoint(IPV4Address hostAddr, uint16_t port) : addr_{} {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = htonl(hostAddr.uint());
}

IPV4Address TCPV4Endpoint::hostAddr() const {
    return IPV4Address(ntohl(addr_.sin_addr.s_addr));
}

uint16_t TCPV4Endpoint::port() const {
    return ntohs(addr_.sin_port);
}

std::string TCPV4Endpoint::format() const {
    return std::format("tcp://{}:{}", hostAddr(), port());
}

std::unique_ptr<Endpoint> TCPV4Endpoint::clone() const {
    return std::make_unique<TCPV4Endpoint>(addr_);
}

bool TCPV4Endpoint::equals(const Endpoint &other) const {
    if (domain() != other.domain()) return false;
    const TCPV4Endpoint &castOther = static_cast<const TCPV4Endpoint &>(other);
    return hostAddr() == castOther.hostAddr() && port() == castOther.port();
}

size_t TCPV4Endpoint::hashCode() const noexcept {
    size_t seed = 0;
    hash_combine(seed, hostAddr());
    hash_combine(seed, port());
    return seed;
}
