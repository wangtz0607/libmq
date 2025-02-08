#pragma once

#include <chrono>

#include "mq/utils/Executor.h"

namespace mq {

class TimedExecutor : public Executor {
public:
    using TimedTask = std::move_only_function<void ()>;

    virtual void postTimed(TimedTask task, std::chrono::nanoseconds delay) = 0;
};

} // namespace mq
