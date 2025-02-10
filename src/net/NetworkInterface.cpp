// SPDX-License-Identifier: MIT

#include "mq/net/NetworkInterface.h"

#include <string>

#include <net/if.h>

#include "mq/utils/Check.h"

#define TAG "NetworkInterface"

using namespace mq;

NetworkInterface::NetworkInterface(const char *name) {
    CHECK((index_ = if_nametoindex(name)) != 0);
}

std::string NetworkInterface::name() const {
    char name[IF_NAMESIZE];
    CHECK(if_indextoname(index_, name) != nullptr);
    return name;
}
