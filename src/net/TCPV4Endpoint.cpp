#include "mq/net/TCPV4Endpoint.h"

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <typeinfo>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "mq/net/Endpoint.h"
#include "mq/net/IPV4Host.h"
#include "mq/utils/Hash.h"

#define TAG "TCPV4Endpoint"

using namespace mq;

TCPV4Endpoint::TCPV4Endpoint(IPV4Host host, uint16_t port) : addr_{} {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = ntohl(host.binary());
}

IPV4Host TCPV4Endpoint::host() const {
    return IPV4Host(ntohl(addr_.sin_addr.s_addr));
}

uint16_t TCPV4Endpoint::port() const {
    return ntohs(addr_.sin_port);
}

std::string TCPV4Endpoint::format() const {
    return std::format("tcp://{}:{}", host(), port());
}

std::unique_ptr<Endpoint> TCPV4Endpoint::clone() const {
    return std::make_unique<TCPV4Endpoint>(addr_);
}

bool TCPV4Endpoint::equals(const Endpoint &other) const {
    if (typeid(*this) != typeid(other)) return false;
    const TCPV4Endpoint &castOther = dynamic_cast<const TCPV4Endpoint &>(other);
    return host() == castOther.host() && port() == castOther.port();
}

size_t TCPV4Endpoint::hashCode() const noexcept {
    size_t seed = 0;
    hash_combine(seed, host());
    hash_combine(seed, port());
    return seed;
}
