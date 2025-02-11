// SPDX-License-Identifier: MIT

#include "mq/net/IpAddr.h"

#include <string>

#include <arpa/inet.h>

#include "mq/utils/Check.h"

#define TAG "IpAddr"

using namespace mq;

IpAddr::IpAddr(const char *addr) {
    uint_type dst;
    CHECK(inet_pton(AF_INET, addr, &dst) == 1);
    addr_ = ntohl(dst);
}

std::string IpAddr::string() const {
    uint_type src = htonl(addr_);
    char dst[INET_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET, &src, dst, sizeof(dst)) != nullptr);
    return dst;
}
