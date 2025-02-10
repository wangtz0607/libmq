// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <functional>
#include <memory>

namespace mq {

template <typename Ptr, typename Hash = std::hash<typename std::pointer_traits<Ptr>::element_type>>
class IndirectHash {
    using Element = std::pointer_traits<Ptr>::element_type;

public:
    using is_transparent = void;

    constexpr size_t operator()(const Ptr &ptr) const noexcept {
        return hash_(*ptr);
    }

    constexpr size_t operator()(const Element &element) const noexcept {
        return hash_(element);
    }

private:
    [[no_unique_address]] Hash hash_;
};

} // namespace mq
