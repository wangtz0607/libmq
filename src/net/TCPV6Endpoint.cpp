#include "mq/net/TCPV6Endpoint.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <memory>
#include <string>
#include <typeinfo>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

#include "mq/net/Endpoint.h"
#include "mq/net/IPV6Addr.h"
#include "mq/net/NetworkInterface.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Hash.h"

#define TAG "TCPV6Endpoint"

using namespace mq;

namespace {

IPV6Addr parseHost(const std::string &hostAndInterface) {
    if (size_t i = hostAndInterface.find('%'); i != std::string::npos) {
        return IPV6Addr(hostAndInterface.substr(0, i));
    } else {
        return IPV6Addr(hostAndInterface);
    }
}

NetworkInterface parseInterface(const std::string &hostAndInterface) {
    if (size_t i = hostAndInterface.find('%'); i != std::string::npos) {
        std::string interface = hostAndInterface.substr(i + 1);
        if (!interface.empty() && std::ranges::all_of(interface, isdigit)) {
            return NetworkInterface(std::stoi(interface));
        } else {
            return NetworkInterface(interface);
        }
    } else {
        return NetworkInterface();
    }
}

} // namespace

TCPV6Endpoint::TCPV6Endpoint(IPV6Addr host, uint16_t port) : addr_{} {
    addr_.sin6_family = AF_INET6;
    addr_.sin6_port = htons(port);
    IPV6Addr::Bytes src = host.bytes();
    toBigEndian(src.data(), 16);
    memcpy(&addr_.sin6_addr, src.data(), 16);
}

TCPV6Endpoint::TCPV6Endpoint(IPV6Addr host, NetworkInterface interface, uint16_t port)
    : TCPV6Endpoint(host, port) {
    addr_.sin6_scope_id = interface.index();
}

TCPV6Endpoint::TCPV6Endpoint(const std::string &hostAndInterface, uint16_t port)
    : TCPV6Endpoint(parseHost(hostAndInterface), parseInterface(hostAndInterface), port) {}

IPV6Addr TCPV6Endpoint::host() const {
    IPV6Addr::Bytes dst;
    memcpy(dst.data(), &addr_.sin6_addr, 16);
    fromBigEndian(dst.data(), 16);
    return IPV6Addr(dst);
}

NetworkInterface TCPV6Endpoint::interface() const {
    return NetworkInterface(addr_.sin6_scope_id);
}

uint16_t TCPV6Endpoint::port() const {
    return ntohs(addr_.sin6_port);
}

std::string TCPV6Endpoint::format() const {
    if (interface().index() != 0) {
        return std::format("tcp://[{}%{}]:{}", host(), interface(), port());
    } else {
        return std::format("tcp://[{}]:{}", host(), port());
    }
}

std::unique_ptr<Endpoint> TCPV6Endpoint::clone() const {
    return std::make_unique<TCPV6Endpoint>(addr_);
}

bool TCPV6Endpoint::equals(const Endpoint &other) const {
    if (typeid(*this) != typeid(other)) return false;
    const TCPV6Endpoint &castOther = static_cast<const TCPV6Endpoint &>(other);
    return host() == castOther.host() &&
           interface() == castOther.interface() &&
           port() == castOther.port();
}

size_t TCPV6Endpoint::hashCode() const noexcept {
    size_t seed = 0;
    hash_combine(seed, host());
    hash_combine(seed, interface());
    hash_combine(seed, port());
    return seed;
}
