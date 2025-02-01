#include "mq/net/IPV6Host.h"

#include <cstring>

#include <arpa/inet.h>

#include "mq/utils/Check.h"
#include "mq/utils/Endian.h"

#define TAG "IPV6Host"

using namespace mq;

IPV6Host::IPV6Host(const uint8_t *host) {
    memcpy(host_.data(), host, 16);
}

IPV6Host::IPV6Host(const char *host) {
    Bytes dst;
    CHECK(inet_pton(AF_INET6, host, dst.data()) == 1);
    fromBigEndian(dst.data(), 16);
    host_ = dst;
}

IPV6Host::Bits IPV6Host::bits() const {
    Bytes bytes = host_;
    toLittleEndian(bytes.data(), 16);
    Bits bits;
    for (size_t i = 0; i < 16; ++i) {
        for (size_t j = 0; j < 8; ++j) {
            bits.set(i * 8 + j, (bytes[i] >> j) & 1);
        }
    }
    return bits;
}

std::string IPV6Host::string() const {
    Bytes src = host_;
    toBigEndian(src.data(), 16);
    char dst[INET6_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET6, src.data(), dst, sizeof(dst)) != nullptr);
    return dst;
}
