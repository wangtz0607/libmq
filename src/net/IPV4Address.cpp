// SPDX-License-Identifier: MIT

#include "mq/net/IPV4Address.h"

#include <cstdint>
#include <string>

#include <arpa/inet.h>

#include "mq/utils/Check.h"

#define TAG "IPV4Address"

using namespace mq;

IPV4Address::IPV4Address(const char *addr) {
    uint32_t dst;
    CHECK(inet_pton(AF_INET, addr, &dst) == 1);
    addr_ = ntohl(dst);
}

std::string IPV4Address::string() const {
    uint32_t src = htonl(addr_);
    char dst[INET_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET, &src, dst, sizeof(dst)) != nullptr);
    return dst;
}
