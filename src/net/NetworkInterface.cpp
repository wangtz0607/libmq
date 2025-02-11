// SPDX-License-Identifier: MIT

#include "mq/net/NetworkInterface.h"

#include <string>

#include <net/if.h>

#include "mq/utils/Check.h"
#include "mq/utils/ZStringView.h"

#define TAG "NetworkInterface"

using namespace mq;

NetworkInterface::NetworkInterface(ZStringView name) {
    CHECK((index_ = if_nametoindex(name.c_str())) != 0);
}

std::string NetworkInterface::name() const {
    char name[IF_NAMESIZE];
    CHECK(if_indextoname(index_, name) != nullptr);
    return name;
}
