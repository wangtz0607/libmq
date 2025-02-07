#include "mq/net/UnixEndpoint.h"

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <string>

#include <sys/socket.h>
#include <sys/un.h>

#include "mq/utils/Check.h"

#define TAG "UnixEndpoint"

using namespace mq;

UnixEndpoint::UnixEndpoint(const std::filesystem::path &path) : addr_{} {
    CHECK(strlen(path.c_str()) < sizeof(addr_.sun_path));

    addr_.sun_family = AF_UNIX;

    if (path.c_str()[0] == '@') {
        addr_.sun_path[0] = '\0';
        strcpy(addr_.sun_path + 1, path.c_str() + 1);

        addrLen_ = offsetof(sockaddr_un, sun_path) + strlen(path.c_str());
    } else {
        strcpy(addr_.sun_path, path.c_str());

        addrLen_ = offsetof(sockaddr_un, sun_path) + strlen(path.c_str()) + 1;
    }
}

std::filesystem::path UnixEndpoint::path() const {
    size_t len = addrLen_ - offsetof(sockaddr_un, sun_path);

    if (len == 0) {
        return "";
    } else if (addr_.sun_path[0] == '\0') {
        return std::filesystem::path("@").concat(addr_.sun_path + 1, addr_.sun_path + len);
    } else {
        return std::filesystem::path(addr_.sun_path, addr_.sun_path + len - 1);
    }
}

std::string UnixEndpoint::format() const {
    return std::format("unix://{}", path().string());
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
    return std::hash<std::string>{}(path().string());
}
