#include "mq/net/UnixEndpoint.h"

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <string>
#include <typeinfo>

#include <sys/socket.h>
#include <sys/un.h>

#include "mq/utils/Check.h"

#define TAG "UnixEndpoint"

using namespace mq;

UnixEndpoint::UnixEndpoint(const std::filesystem::path &path) : addr_{} {
    CHECK(path.string().size() < sizeof(addr_.sun_path));

    addr_.sun_family = AF_UNIX;
    strcpy(addr_.sun_path, path.c_str());
    addrLen_ = offsetof(struct sockaddr_un, sun_path) + path.string().size() + 1;
}

std::filesystem::path UnixEndpoint::path() const {
    size_t len = addrLen_ - offsetof(struct sockaddr_un, sun_path);
    return std::filesystem::path(addr_.sun_path, addr_.sun_path + len);
}

std::string UnixEndpoint::format() const {
    return std::format("unix://{}", path().string());
}

std::unique_ptr<Endpoint> UnixEndpoint::clone() const {
    return std::make_unique<UnixEndpoint>(addr_, addrLen_);
}

bool UnixEndpoint::equals(const Endpoint &other) const {
    if (typeid(*this) != typeid(other)) return false;
    const UnixEndpoint &castOther = static_cast<const UnixEndpoint &>(other);
    return path() == castOther.path();
}

size_t UnixEndpoint::hashCode() const noexcept {
    return std::hash<std::string>{}(path().string());
}
