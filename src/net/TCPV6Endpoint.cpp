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
#include "mq/net/IPV6Host.h"
#include "mq/net/IPV6ScopeId.h"
#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"
#include "mq/utils/Hash.h"

#define TAG "TCPV6Endpoint"

using namespace mq;

TCPV6Endpoint::TCPV6Endpoint(IPV6Host host, IPV6ScopeId scopeId, uint16_t port) : addr_{} {
    addr_.sin6_family = AF_INET6;
    addr_.sin6_port = htons(port);
    IPV6Host::Bytes src = host.bytes();
    toBigEndian(src.data(), 16);
    memcpy(&addr_.sin6_addr, src.data(), 16);
    addr_.sin6_scope_id = scopeId.uint();
}

TCPV6Endpoint::TCPV6Endpoint(const std::string &hostAndScopeId, uint16_t port) : addr_{} {
    addr_.sin6_family = AF_INET6;
    addr_.sin6_port = htons(port);

    if (size_t i = hostAndScopeId.find('%'); i != std::string::npos) {
        std::string host = hostAndScopeId.substr(0, i);
        std::string scopeId = hostAndScopeId.substr(i + 1);

        CHECK(inet_pton(AF_INET6, host.c_str(), &addr_.sin6_addr) == 1);

        if (!scopeId.empty() && std::ranges::all_of(scopeId, isdigit)) {
            addr_.sin6_scope_id = std::stoi(scopeId);
        } else {
            CHECK((addr_.sin6_scope_id = if_nametoindex(scopeId.c_str())) != 0);
        }
    } else {
        CHECK(inet_pton(AF_INET6, hostAndScopeId.c_str(), &addr_.sin6_addr) == 1);
    }
}

IPV6Host TCPV6Endpoint::host() const {
    IPV6Host::Bytes dst;
    memcpy(dst.data(), &addr_.sin6_addr, 16);
    fromBigEndian(dst.data(), 16);
    return IPV6Host(dst);
}

IPV6ScopeId TCPV6Endpoint::scopeId() const {
    return IPV6ScopeId(addr_.sin6_scope_id);
}

uint16_t TCPV6Endpoint::port() const {
    return ntohs(addr_.sin6_port);
}

std::string TCPV6Endpoint::format() const {
    if (scopeId()) {
        return std::format("tcp://[{}%{}]:{}", host(), scopeId(), port());
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
           scopeId() == castOther.scopeId() &&
           port() == castOther.port();
}

size_t TCPV6Endpoint::hashCode() const noexcept {
    size_t seed = 0;
    hash_combine(seed, host());
    hash_combine(seed, scopeId());
    hash_combine(seed, port());
    return seed;
}
