#include "mq/net/IPV6Address.h"

#include <cstring>
#include <string>

#include <arpa/inet.h>

#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"

#define TAG "IPV6Address"

using namespace mq;

IPV6Address::IPV6Address(const uint8_t *addr) : addr_{} {
    memcpy(addr_.data(), addr, 16);
}

IPV6Address::IPV6Address(const char *addr) : addr_{} {
    Bytes dst;
    CHECK(inet_pton(AF_INET6, addr, dst.data()) == 1);
    fromBigEndian(dst.data(), 16);
    addr_ = dst;
}

std::string IPV6Address::string() const {
    Bytes src = addr_;
    toBigEndian(src.data(), 16);
    char dst[INET6_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET6, src.data(), dst, sizeof(dst)) != nullptr);
    return dst;
}
