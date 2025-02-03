#include "mq/net/IPV6Addr.h"

#include <cstring>

#include <arpa/inet.h>

#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"

#define TAG "IPV6Addr"

using namespace mq;

IPV6Addr::IPV6Addr(const uint8_t *addr) {
    memcpy(addr_.data(), addr, 16);
}

IPV6Addr::IPV6Addr(const char *addr) {
    Bytes dst;
    CHECK(inet_pton(AF_INET6, addr, dst.data()) == 1);
    fromBigEndian(dst.data(), 16);
    addr_ = dst;
}

std::string IPV6Addr::string() const {
    Bytes src = addr_;
    toBigEndian(src.data(), 16);
    char dst[INET6_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET6, src.data(), dst, sizeof(dst)) != nullptr);
    return dst;
}
