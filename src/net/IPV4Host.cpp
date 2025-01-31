#include "mq/net/IPV4Host.h"

#include <cstdint>

#include <arpa/inet.h>

#include "mq/utils/Check.h"

#define TAG "IPV4Host"

using namespace mq;

IPV4Host::IPV4Host(const char *host) {
    uint32_t dst;
    CHECK(inet_pton(AF_INET, host, &dst) == 1);
    host_ = ntohl(dst);
}

std::string IPV4Host::string() const {
    uint32_t src = htonl(host_);
    char dst[INET_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET, &src, dst, sizeof(dst)) != nullptr);
    return dst;
}
