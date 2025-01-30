#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

#include <sys/socket.h>
#include <sys/un.h>

#include "mq/net/Endpoint.h"

namespace mq {

class UnixEndpoint final : public Endpoint {
public:
    explicit UnixEndpoint(const std::filesystem::path &path);

    explicit UnixEndpoint(const struct sockaddr_un &addr)
        : addr_(addr) {}

    std::filesystem::path path() const;

    int domain() const override {
        return AF_UNIX;
    }

    const struct sockaddr *addr() const override {
        return reinterpret_cast<const struct sockaddr *>(&addr_);
    }

    socklen_t addrLen() const override {
        return sizeof(addr_);
    }

    std::string format() const override;
    std::unique_ptr<Endpoint> clone() const override;

protected:
    bool equals(const Endpoint &other) const override;
    size_t hashCode() const noexcept override;

private:
    struct sockaddr_un addr_;
};

} // namespace mq
