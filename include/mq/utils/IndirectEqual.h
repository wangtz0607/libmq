#pragma once

#include <memory>

namespace mq {

template <typename Ptr>
class IndirectEqual {
    using Element = std::pointer_traits<Ptr>::element_type;

public:
    using is_transparent = void;

    constexpr bool operator()(const Ptr &lhs, const Ptr &rhs) const noexcept {
        return *lhs == *rhs;
    }

    constexpr bool operator()(const Ptr &lhs, const Element &rhs) const noexcept {
        return *lhs == rhs;
    }

    constexpr bool operator()(const Element &lhs, const Ptr &rhs) const noexcept {
        return lhs == *rhs;
    }
};

} // namespace mq
