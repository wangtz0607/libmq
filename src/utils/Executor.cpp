// SPDX-License-Identifier: MIT

#include "mq/utils/Executor.h"

#include <future>
#include <utility>

using namespace mq;

void Executor::postAndWait(Task task) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    post([task = std::move(task), promise = std::move(promise)] mutable {
        task();
        promise.set_value();
    });

    future.wait();
}
