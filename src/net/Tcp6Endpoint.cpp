// SPDX-License-Identifier: MIT

#include "mq/net/Tcp6Endpoint.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <memory>
#include <string>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

#include "mq/net/Endpoint.h"
#include "mq/net/Ip6Addr.h"
#include "mq/net/NetworkInterface.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Hash.h"

#define TAG "Tcp6Endpoint"

using namespace mq;

namespace {

Ip6Addr parseHostAddr(const std::string &host) {
    if (size_t i = host.find('%'); i != std::string::npos) {
        return Ip6Addr(host.substr(0, i));
    } else {
        return Ip6Addr(host);
    }
}

NetworkInterface parseInterface(const std::string &host) {
    if (size_t i = host.find('%'); i != std::string::npos) {
        std::string interface = host.substr(i + 1);

        uint32_t index;
        auto [ptr, ec] = std::from_chars(interface.data(), interface.data() + interface.size(), index);

        if (ptr == interface.data() + interface.size() && ec == std::errc()) {
            return NetworkInterface(index);
        } else {
            return NetworkInterface(interface);
        }
    } else {
        return NetworkInterface();
    }
}

} // namespace

Tcp6Endpoint::Tcp6Endpoint(Ip6Addr hostAddr, NetworkInterface interface, uint16_t port) : addr_{} {
    addr_.sin6_family = AF_INET6;
    addr_.sin6_port = htons(port);
    Ip6Addr::bytes_type src = hostAddr.bytes();
    toBigEndian(src.data(), 16);
    memcpy(&addr_.sin6_addr, src.data(), 16);
    addr_.sin6_scope_id = interface.index();
}

Tcp6Endpoint::Tcp6Endpoint(const std::string &host, uint16_t port)
    : Tcp6Endpoint(parseHostAddr(host), parseInterface(host), port) {}

Ip6Addr Tcp6Endpoint::hostAddr() const {
    Ip6Addr::bytes_type dst;
    memcpy(dst.data(), &addr_.sin6_addr, 16);
    fromBigEndian(dst.data(), 16);
    return Ip6Addr(dst);
}

NetworkInterface Tcp6Endpoint::interface() const {
    return NetworkInterface(addr_.sin6_scope_id);
}

uint16_t Tcp6Endpoint::port() const {
    return ntohs(addr_.sin6_port);
}

std::string Tcp6Endpoint::format() const {
    if (interface().index() != 0) {
        return std::format("tcp://[{}%{}]:{}", hostAddr(), interface(), port());
    } else {
        return std::format("tcp://[{}]:{}", hostAddr(), port());
    }
}

std::unique_ptr<Endpoint> Tcp6Endpoint::clone() const {
    return std::make_unique<Tcp6Endpoint>(addr_);
}

bool Tcp6Endpoint::equals(const Endpoint &other) const {
    if (domain() != other.domain()) return false;
    const Tcp6Endpoint &castOther = static_cast<const Tcp6Endpoint &>(other);
    return hostAddr() == castOther.hostAddr() &&
           interface() == castOther.interface() &&
           port() == castOther.port();
}

size_t Tcp6Endpoint::hashCode() const noexcept {
    size_t seed = 0;
    hash_combine(seed, hostAddr());
    hash_combine(seed, interface());
    hash_combine(seed, port());
    return seed;
}
