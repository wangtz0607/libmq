#pragma once

#include <memory>

namespace mq {

template <typename Ptr>
class PtrEqual {
    using Element = std::pointer_traits<Ptr>::element_type;

public:
    using is_transparent = void;

    constexpr bool operator()(const Ptr &lhs, const Ptr &rhs) const noexcept {
        return lhs.get() == rhs.get();
    }

    constexpr bool operator()(const Ptr &lhs, Element *rhs) const noexcept {
        return lhs.get() == rhs;
    }

    constexpr bool operator()(Element *lhs, const Ptr &rhs) const noexcept {
        return lhs == rhs.get();
    }
};

} // namespace mq
