// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>

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

inline constexpr void toLittleEndian(void *data, size_t size) {
    if constexpr (std::endian::native != std::endian::little) {
        std::reverse(static_cast<uint8_t *>(data), static_cast<uint8_t *>(data) + size);
    }
}

template <typename T>
    requires std::integral<T>
constexpr T fromLittleEndian(T value) {
    return toLittleEndian(value);
}

inline constexpr void fromLittleEndian(void *data, size_t size) {
    toLittleEndian(data, size);
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

inline constexpr void toBigEndian(void *data, size_t size) {
    if constexpr (std::endian::native != std::endian::big) {
        std::reverse(static_cast<uint8_t *>(data), static_cast<uint8_t *>(data) + size);
    }
}

template <typename T>
    requires std::integral<T>
constexpr T fromBigEndian(T value) {
    return toBigEndian(value);
}

inline constexpr void fromBigEndian(void *data, size_t size) {
    toBigEndian(data, size);
}

} // namespace mq
