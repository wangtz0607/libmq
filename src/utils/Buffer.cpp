#include "mq/utils/Buffer.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <utility>

#include "mq/utils/Check.h"

#define TAG "Buffer"

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
      end_(other.end_ - other.begin_),
      buffer_(new char[other.end_ - other.begin_]) {
    memcpy(buffer_, other.buffer_ + other.begin_, other.end_ - other.begin_);
}

Buffer::Buffer(Buffer &&other) noexcept {
    swap(other);
}

Buffer::~Buffer() {
    delete[] buffer_;
}

Buffer &Buffer::operator=(Buffer other) noexcept {
    swap(other);
    return *this;
}

void Buffer::extendBack(size_t size) {
    CHECK(end_ - begin_ + size <= maxCapacity_);

    if (end_ + size > capacity_) {
        reallocate(std::max((end_ - begin_) + size, capacity_ + capacity_ / 2));
    }

    end_ += size;
}

void Buffer::retractFront(size_t size) {
    CHECK(end_ - begin_ >= size);

    begin_ += size;

    if (begin_ > capacity_ / 2) {
        move();
    }
}

void Buffer::retractBack(size_t size) {
    CHECK(end_ - begin_ >= size);

    end_ -= size;
}

void Buffer::clear() {
    begin_ = 0;
    end_ = 0;
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
    char *newBuffer = new char[newCapacity];
    if (buffer_) {
        memcpy(newBuffer, buffer_ + begin_, size);
        delete[] buffer_;
    }
    capacity_ = newCapacity;
    begin_ = 0;
    end_ = size;
    buffer_ = newBuffer;
}

void Buffer::move() {
    size_t size = end_ - begin_;
    memmove(buffer_, buffer_ + begin_, size);
    begin_ = 0;
    end_ = size;
}
