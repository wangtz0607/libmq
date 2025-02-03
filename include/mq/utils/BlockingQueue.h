#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <type_traits>
#include <utility>

namespace mq {

template <typename T>
    requires std::is_nothrow_move_constructible_v<T>
class BlockingQueue {
public:
    BlockingQueue() = default;

    BlockingQueue(const BlockingQueue &) = delete;
    BlockingQueue(BlockingQueue &&) = delete;

    BlockingQueue& operator=(const BlockingQueue &) = delete;
    BlockingQueue& operator=(BlockingQueue &&) = delete;

    void push(T value) {
        {
            std::unique_lock lock(mutex_);
            queue_.push(std::move(value));
        }
        condition_.notify_one();
    }

    T pop() {
        std::unique_lock lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty(); });
        T value(std::move(queue_.front()));
        queue_.pop();
        return value;
    }

    bool empty() const {
        std::unique_lock lock(mutex_);
        return queue_.empty();
    }

    size_t size() const {
        std::unique_lock lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
};

} // namespace mq
