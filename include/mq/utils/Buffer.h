#pragma once

#include <cstddef>
#include <limits>

namespace mq {

class Buffer {
public:
    explicit Buffer(size_t maxCapacity = std::numeric_limits<size_t>::max());

    Buffer(const Buffer &other);
    Buffer(Buffer &&other) noexcept;

    ~Buffer();

    Buffer &operator=(Buffer other) noexcept;

    size_t maxCapacity() const {
        return maxCapacity_;
    }

    size_t capacity() const {
        return capacity_;
    }

    size_t size() const {
        return end_ - begin_;
    }

    bool empty() const {
        return begin_ == end_;
    }

    char *data() {
        return buffer_ + begin_;
    }

    const char *data() const {
        return buffer_ + begin_;
    }

    char &operator[](size_t i) {
        return buffer_[i];
    }

    const char &operator[](size_t i) const {
        return buffer_[i];
    }

    void extendBack(size_t size);
    void retractFront(size_t size);
    void retractBack(size_t size);
    void clear();
    void swap(Buffer &other) noexcept;

private:
    size_t maxCapacity_;
    size_t capacity_;
    size_t begin_, end_;
    char *buffer_;

    void reallocate(size_t newCapacity);
    void move();
};

inline void swap(Buffer &lhs, Buffer &rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace mq
