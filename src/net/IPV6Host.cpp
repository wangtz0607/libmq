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
    Bytes dst;
    CHECK(inet_pton(AF_INET6, host, dst.data()) == 1);
    std::ranges::reverse(dst);
    host_ = dst;
}

std::string IPV6Host::string() const {
    Bytes src = host_;
    std::ranges::reverse(src);
    char dst[INET6_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET6, src.data(), dst, sizeof(dst)) != nullptr);
    return dst;
}
