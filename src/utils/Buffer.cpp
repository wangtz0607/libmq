#include "mq/utils/Buffer.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

using namespace mq;

Buffer::Buffer(size_t maxCapacity)
    : maxCapacity_(maxCapacity),
      capacity_(0),
      begin_(0),
      end_(0),
      buffer_(nullptr) {}

Buffer::Buffer(const Buffer &other)
    : maxCapacity_(other.maxCapacity_),
      capacity_(other.end_ - other.begin_),
      begin_(0),
      end_(other.end_ - other.begin_) {
    buffer_ = static_cast<char *>(malloc(other.end_ - other.begin_));

    if (!buffer_) {
        perror("malloc");
        abort();
    }

    memcpy(buffer_, other.buffer_ + other.begin_, other.end_ - other.begin_);
}

Buffer::Buffer(Buffer &&other) noexcept : Buffer() {
    swap(other);
}

Buffer::~Buffer() {
    free(buffer_);
}

Buffer &Buffer::operator=(Buffer other) noexcept {
    swap(other);
    return *this;
}

void Buffer::setMaxCapacity(size_t maxCapacity) {
    assert(maxCapacity >= capacity_);

    maxCapacity_ = maxCapacity;
}

void Buffer::extendBack(size_t size) {
    assert(end_ - begin_ + size <= maxCapacity_);

    if (end_ + size > capacity_) {
        reallocate(std::max((end_ - begin_) + size, capacity_ + capacity_ / 2));
    }

    end_ += size;
}

void Buffer::retractFront(size_t size) {
    assert(end_ - begin_ >= size);

#ifndef NDEBUG
    if (buffer_) {
        memset(buffer_ + begin_, 0xcc, size);
    }
#endif

    begin_ += size;

    if (begin_ == end_) {
        begin_ = 0;
        end_ = 0;
    }

    if (begin_ > capacity_ / 2) {
        move();
    }
}

void Buffer::retractBack(size_t size) {
    assert(end_ - begin_ >= size);

#ifndef NDEBUG
    if (buffer_) {
        memset(buffer_ + end_ - size, 0xcc, size);
    }
#endif

    end_ -= size;

    if (begin_ == end_) {
        begin_ = 0;
        end_ = 0;
    }
}

void Buffer::clear() {
#ifndef NDEBUG
    if (buffer_) {
        memset(buffer_ + begin_, 0xcc, end_ - begin_);
    }
#endif

    begin_ = 0;
    end_ = 0;
}

void Buffer::shrinkToFit() {
    if (capacity_ > end_ - begin_) {
        reallocate(end_ - begin_);
    }
}

void Buffer::swap(Buffer &other) noexcept {
    using std::swap;
    swap(maxCapacity_, other.maxCapacity_);
    swap(capacity_, other.capacity_);
    swap(begin_, other.begin_);
    swap(end_, other.end_);
    swap(buffer_, other.buffer_);
}

void Buffer::reallocate(size_t newCapacity) {
    size_t size = end_ - begin_;

    char *newBuffer = static_cast<char *>(realloc(buffer_, newCapacity));

    if (!newBuffer) {
        perror("realloc");
        abort();
    }

#ifndef NDEBUG
    memset(newBuffer + size, 0xcc, newCapacity - size);
#endif

    capacity_ = newCapacity;
    begin_ = 0;
    end_ = size;
    buffer_ = newBuffer;
}

void Buffer::move() {
    size_t size = end_ - begin_;
    memmove(buffer_, buffer_ + begin_, size);

#ifndef NDEBUG
    memset(buffer_ + size, 0xcc, capacity_ - size);
#endif

    begin_ = 0;
    end_ = size;
}
