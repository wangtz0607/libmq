#include "mq/net/UnixEndpoint.h"

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
}

std::filesystem::path UnixEndpoint::path() const {
    return addr_.sun_path;
}

std::string UnixEndpoint::format() const {
    return std::format("unix://{}", path().string());
}

std::unique_ptr<Endpoint> UnixEndpoint::clone() const {
    return std::make_unique<UnixEndpoint>(addr_);
}

bool UnixEndpoint::equals(const Endpoint &other) const {
    if (typeid(*this) != typeid(other)) return false;
    const UnixEndpoint &castOther = static_cast<const UnixEndpoint &>(other);
    return path() == castOther.path();
}

size_t UnixEndpoint::hashCode() const noexcept {
    return std::hash<std::string>{}(path().string());
}
