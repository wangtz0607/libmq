// SPDX-License-Identifier: MIT

#include "mq/net/Ip6Addr.h"

#include <cstdint>
#include <cstring>
#include <string>

#include <arpa/inet.h>

#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"
#include "mq/utils/ZStringView.h"

#define TAG "Ip6Addr"

using namespace mq;

Ip6Addr::Ip6Addr(const uint8_t *addr) : addr_{} {
    memcpy(addr_.data(), addr, 16);
}

Ip6Addr::Ip6Addr(ZStringView addr) : addr_{} {
    bytes_type dst;
    CHECK(inet_pton(AF_INET6, addr.c_str(), dst.data()) == 1);
    fromBigEndian(dst.data(), 16);
    addr_ = dst;
}

std::string Ip6Addr::string() const {
    bytes_type src = addr_;
    toBigEndian(src.data(), 16);
    char dst[INET6_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET6, src.data(), dst, sizeof(dst)) != nullptr);
    return dst;
}
