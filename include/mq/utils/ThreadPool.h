#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "mq/utils/Executor.h"

namespace mq {

class ThreadPool final : public Executor {
public:
    using Task = Executor::Task;

    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool() override;

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void post(Task task) override;

private:
    std::vector<std::thread> threads_;
    std::queue<Task> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stop_ = false;
};

} // namespace mq
