// SPDX-License-Identifier: MIT

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

    explicit UnixEndpoint(const sockaddr_un &addr, socklen_t addrLen)
        : addr_(addr), addrLen_(addrLen) {}

    std::filesystem::path path() const;

    sa_family_t domain() const override {
        return AF_UNIX;
    }

    const sockaddr *data() const override {
        return reinterpret_cast<const sockaddr *>(&addr_);
    }

    socklen_t size() const override {
        return addrLen_;
    }

    std::string format() const override;
    std::unique_ptr<Endpoint> clone() const override;

protected:
    bool equals(const Endpoint &other) const override;
    size_t hashCode() const noexcept override;

private:
    sockaddr_un addr_;
    socklen_t addrLen_;
};

} // namespace mq
