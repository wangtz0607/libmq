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

IPV6Addr parseHostAddr(const std::string &host) {
    if (size_t i = host.find('%'); i != std::string::npos) {
        return IPV6Addr(host.substr(0, i));
    } else {
        return IPV6Addr(host);
    }
}

NetworkInterface parseInterface(const std::string &host) {
    if (size_t i = host.find('%'); i != std::string::npos) {
        std::string interface = host.substr(i + 1);
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

TCPV6Endpoint::TCPV6Endpoint(IPV6Addr hostAddr, uint16_t port) : addr_{} {
    addr_.sin6_family = AF_INET6;
    addr_.sin6_port = htons(port);
    IPV6Addr::Bytes src = hostAddr.bytes();
    toBigEndian(src.data(), 16);
    memcpy(&addr_.sin6_addr, src.data(), 16);
}

TCPV6Endpoint::TCPV6Endpoint(IPV6Addr hostAddr, NetworkInterface interface, uint16_t port)
    : TCPV6Endpoint(hostAddr, port) {
    addr_.sin6_scope_id = interface.index();
}

TCPV6Endpoint::TCPV6Endpoint(const std::string &host, uint16_t port)
    : TCPV6Endpoint(parseHostAddr(host), parseInterface(host), port) {}

IPV6Addr TCPV6Endpoint::hostAddr() const {
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
        return std::format("tcp://[{}%{}]:{}", hostAddr(), interface(), port());
    } else {
        return std::format("tcp://[{}]:{}", hostAddr(), port());
    }
}

std::unique_ptr<Endpoint> TCPV6Endpoint::clone() const {
    return std::make_unique<TCPV6Endpoint>(addr_);
}

bool TCPV6Endpoint::equals(const Endpoint &other) const {
    if (typeid(*this) != typeid(other)) return false;
    const TCPV6Endpoint &castOther = static_cast<const TCPV6Endpoint &>(other);
    return hostAddr() == castOther.hostAddr() &&
           interface() == castOther.interface() &&
           port() == castOther.port();
}

size_t TCPV6Endpoint::hashCode() const noexcept {
    size_t seed = 0;
    hash_combine(seed, hostAddr());
    hash_combine(seed, interface());
    hash_combine(seed, port());
    return seed;
}
