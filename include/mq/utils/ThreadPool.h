#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace mq {

class ThreadPool {
public:
    using Task = std::move_only_function<void ()>;

    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void post(Task task);
    void postAndWait(Task task);

private:
    std::vector<std::thread> threads_;
    std::queue<Task> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stop_ = false;
};

} // namespace mq
