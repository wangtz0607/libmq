// SPDX-License-Identifier: MIT

#pragma once

#include <functional>
#include <memory>

namespace mq {

template <typename Ptr, typename Equal = std::equal_to<typename std::pointer_traits<Ptr>::element_type>>
class IndirectEqual {
    using Element = std::pointer_traits<Ptr>::element_type;

public:
    using is_transparent = void;

    constexpr bool operator()(const Ptr &lhs, const Ptr &rhs) const noexcept {
        return equal_(*lhs, *rhs);
    }

    constexpr bool operator()(const Ptr &lhs, const Element &rhs) const noexcept {
        return equal_(*lhs, rhs);
    }

    constexpr bool operator()(const Element &lhs, const Ptr &rhs) const noexcept {
        return equal_(lhs, *rhs);
    }

private:
    [[no_unique_address]] Equal equal_;
};

} // namespace mq
