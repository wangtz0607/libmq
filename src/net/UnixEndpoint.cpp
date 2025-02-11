// SPDX-License-Identifier: MIT

#include "mq/net/UnixEndpoint.h"

#include <cstddef>
#include <cstring>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <sys/socket.h>
#include <sys/un.h>

#include "mq/utils/Check.h"

#define TAG "UnixEndpoint"

using namespace mq;

UnixEndpoint::UnixEndpoint(std::string_view path) : addr_{} {
    CHECK(path.size() < sizeof(addr_.sun_path));

    addr_.sun_family = AF_UNIX;

    if (path[0] == '@') {
        addr_.sun_path[0] = '\0';
        memcpy(addr_.sun_path + 1, path.data() + 1, path.size() - 1);

        addrLen_ = offsetof(sockaddr_un, sun_path) + path.size();
    } else {
        memcpy(addr_.sun_path, path.data(), path.size());
        addr_.sun_path[path.size()] = '\0';

        addrLen_ = offsetof(sockaddr_un, sun_path) + path.size() + 1;
    }
}

std::string UnixEndpoint::path() const {
    size_t len = addrLen_ - offsetof(sockaddr_un, sun_path);

    if (len == 0) {
        return "";
    } else if (addr_.sun_path[0] == '\0') {
        return "@" + std::string(addr_.sun_path + 1, addr_.sun_path + len);
    } else {
        return std::string(addr_.sun_path, addr_.sun_path + len - 1);
    }
}

std::string UnixEndpoint::format() const {
    return std::format("unix://{}", path());
}

std::unique_ptr<Endpoint> UnixEndpoint::clone() const {
    return std::make_unique<UnixEndpoint>(addr_, addrLen_);
}

bool UnixEndpoint::equals(const Endpoint &other) const {
    if (domain() != other.domain()) return false;
    const UnixEndpoint &castOther = static_cast<const UnixEndpoint &>(other);
    return path() == castOther.path();
}

size_t UnixEndpoint::hashCode() const noexcept {
    return std::hash<std::string>{}(path());
}
