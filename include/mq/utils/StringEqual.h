#pragma once

#include <cstring>
#include <string>
#include <string_view>

namespace mq {

struct StringEqual {
    using is_transparent = void;

    bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
        return lhs == rhs;
    }

    bool operator()(std::string_view lhs, const std::string &rhs) const noexcept {
        return lhs == rhs;
    }

    bool operator()(std::string_view lhs, const char *rhs) const noexcept {
        return lhs == rhs;
    }

    bool operator()(const std::string &lhs, std::string_view rhs) const noexcept {
        return lhs == rhs;
    }

    bool operator()(const std::string &lhs, const std::string &rhs) const noexcept {
        return lhs == rhs;
    }

    bool operator()(const std::string &lhs, const char *rhs) const noexcept {
        return lhs == rhs;
    }

    bool operator()(const char *lhs, std::string_view rhs) const noexcept {
        return rhs == lhs;
    }

    bool operator()(const char *lhs, const std::string &rhs) const noexcept {
        return rhs == lhs;
    }

    bool operator()(const char *lhs, const char *rhs) const noexcept {
        return strcmp(lhs, rhs) == 0;
    }
};

} // namespace mq
