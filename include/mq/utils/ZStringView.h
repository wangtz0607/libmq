// SPDX-License-Identifier: MIT

#pragma once

#include <string>

namespace mq {

class ZStringView {
public:
    constexpr ZStringView(const char *str) : str_(str) {}

    constexpr ZStringView(const std::string &str) : str_(str.c_str()) {}

    constexpr const char *c_str() const {
        return str_;
    }

    constexpr operator const char *() const {
        return str_;
    }

    constexpr operator std::string_view() const {
        return str_;
    }

    constexpr operator std::string() const {
        return str_;
    }

private:
    const char *str_;
};

} // namespace mq
