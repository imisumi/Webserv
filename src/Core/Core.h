#pragma once

#include "Log.h"


#define WEB_DEBUG

#ifdef WEB_DEBUG
    #define WEB_ASSERT(x, ...) { \
        if (!(x)) { \
            LOG_ERROR("Assertion Failed: {0} (File: {1}, Line: {2})", __VA_ARGS__, __FILE__, __LINE__); \
            __builtin_trap(); \
        } \
    }
#else
    #define WEB_ASSERT(x, ...)
#endif
