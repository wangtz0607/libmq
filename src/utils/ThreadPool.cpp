#include "mq/utils/ThreadPool.h"

#include <future>
#include <mutex>
#include <utility>

using namespace mq;

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        threads_.emplace_back([this] {
            for (;;) {
                Task task;

                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

                    if (stop_ && tasks_.empty()) return;

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
    }

    condition_.notify_all();

    for (std::thread &thread : threads_) {
        thread.join();
    }
}

void ThreadPool::post(Task task) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.emplace(std::move(task));
    }

    condition_.notify_one();
}
