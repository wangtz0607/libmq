#include "mq/net/IPV6Host.h"

#include <algorithm>
#include <cstring>

#include <arpa/inet.h>

#include "mq/utils/Check.h"

#define TAG "IPV6Host"

using namespace mq;

IPV6Host::IPV6Host(const uint8_t *host) {
    memcpy(host_.data(), host, 16);
}

IPV6Host::IPV6Host(const char *host) {
    CHECK(inet_pton(AF_INET6, host, &host_) == 1);
    std::ranges::reverse(host_);
}

std::string IPV6Host::string() const {
    ByteArray host = host_;
    std::ranges::reverse(host);
    char buf[INET6_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET6, host.data(), buf, sizeof(buf)) != nullptr);
    return buf;
}
