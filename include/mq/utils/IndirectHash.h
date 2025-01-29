#pragma once

#include <cstddef>
#include <functional>
#include <memory>

namespace mq {

template <typename Ptr>
class IndirectHash {
    using Element = std::pointer_traits<Ptr>::element_type;

public:
    using is_transparent = void;

    constexpr size_t operator()(const Ptr &ptr) const noexcept {
        return std::hash<Element>{}(*ptr);
    }

    constexpr size_t operator()(const Element &element) const noexcept {
        return std::hash<Element>{}(element);
    }
};

} // namespace mq
