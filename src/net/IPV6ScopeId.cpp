#include "mq/net/IPV6ScopeId.h"

#include <string>

#include <net/if.h>

#include "mq/utils/Check.h"

#define TAG "IPV6ScopeId"

using namespace mq;

IPV6ScopeId::IPV6ScopeId(const char *ifName) {
    CHECK((scopeId_ = if_nametoindex(ifName)) != 0);
}

std::string IPV6ScopeId::string() const {
    char ifName[IF_NAMESIZE];
    CHECK(if_indextoname(scopeId_, ifName) != nullptr);
    return ifName;
}
