#pragma once

#include <bit>
#include <concepts>

static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);

namespace mq {

template <typename T>
    requires std::integral<T>
constexpr T toLittleEndian(T value) {
    if constexpr (std::endian::native == std::endian::little) {
        return value;
    } else {
        return std::byteswap(value);
    }
}

template <typename T>
    requires std::integral<T>
constexpr T fromLittleEndian(T value) {
    return toLittleEndian(value);
}

template <typename T>
    requires std::integral<T>
constexpr T toBigEndian(T value) {
    if constexpr (std::endian::native == std::endian::big) {
        return value;
    } else {
        return std::byteswap(value);
    }
}

template <typename T>
    requires std::integral<T>
constexpr T fromBigEndian(T value) {
    return toBigEndian(value);
}

} // namespace mq
