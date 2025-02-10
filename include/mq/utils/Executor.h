// SPDX-License-Identifier: MIT

#pragma once

#include <functional>

namespace mq {

class Executor {
public:
    using Task = std::move_only_function<void ()>;

    virtual ~Executor() = default;

    virtual void post(Task task) = 0;

    void postAndWait(Task task);
};

} // namespace mq
