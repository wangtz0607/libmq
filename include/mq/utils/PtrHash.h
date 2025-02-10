// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <functional>
#include <memory>

namespace mq {

template <typename Ptr>
class PtrHash {
    using Element = std::pointer_traits<Ptr>::element_type;

public:
    using is_transparent = void;

    constexpr size_t operator()(const Ptr &ptr) const noexcept {
        return std::hash<Element *>{}(ptr.get());
    }

    constexpr size_t operator()(Element *ptr) const noexcept {
        return std::hash<Element *>{}(ptr);
    }
};

} // namespace mq
