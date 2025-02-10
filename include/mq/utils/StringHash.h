// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace mq {

struct StringHash {
    using is_transparent = void;

    size_t operator()(std::string_view str) const noexcept {
        return std::hash<std::string_view>{}(str);
    }

    size_t operator()(const std::string &str) const noexcept {
        return std::hash<std::string_view>{}(str);
    }

    size_t operator()(const char *str) const noexcept {
        return std::hash<std::string_view>{}(str);
    }
};

} // namespace mq
