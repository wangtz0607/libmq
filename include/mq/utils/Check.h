#pragma once

#include <cstdlib>

#include "mq/utils/Logging.h"

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            LOG(error, "CHECK: {}", #condition); \
            abort(); \
        } \
    } while (false)
