#pragma once

#include <cstdint>

#define MINIFFI_MAX_ARGS 16l

#if defined(__linux__) || defined(__APPLE__)
    typedef unsigned long mffi_value;
    // Be sure to add more args to the below line if MINIFFI_MAX_ARGS is bumped
    // in the future.
    typedef mffi_value (*MINIFFI_FUNC)(mffi_value, mffi_value,
                                       mffi_value, mffi_value,
                                       mffi_value, mffi_value,
                                       mffi_value, mffi_value,
                                       mffi_value, mffi_value,
                                       mffi_value, mffi_value,
                                       mffi_value, mffi_value,
                                       mffi_value, mffi_value);
#else // Windows
    #ifdef __MINGW64__
        typedef uint64_t mffi_value;
    #else
        typedef uint32_t mffi_value;
    #endif
    typedef mffi_value (*MINIFFI_FUNC)(...);
#endif

typedef struct {
  mffi_value params[MINIFFI_MAX_ARGS];
} MiniFFIFuncArgs;

mffi_value miniffi_call_intern(MINIFFI_FUNC target, MiniFFIFuncArgs *p, int nparams);
