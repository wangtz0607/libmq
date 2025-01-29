#include "mq/net/IPV4Host.h"

#include <arpa/inet.h>

#include "mq/utils/Check.h"

#define TAG "IPV4Host"

using namespace mq;

IPV4Host::IPV4Host(const char *host) {
    CHECK(inet_pton(AF_INET, host, &host_) == 1);
    host_ = ntohl(host_);
}

std::string IPV4Host::string() const {
    uint32_t host = htonl(host_);
    char buf[INET_ADDRSTRLEN];
    CHECK(inet_ntop(AF_INET, &host, buf, sizeof(buf)) != nullptr);
    return buf;
}
