#pragma once

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "mq/utils/Logging.h"

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            LOG(error, "CHECK: {}, errno={}", #condition, strerrorname_np(errno)); \
            abort(); \
        } \
    } while (false);
