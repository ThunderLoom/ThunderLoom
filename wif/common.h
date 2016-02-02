#pragma once

#include <signal.h>
#include <stdint.h>
#define ASSERT(TEST) if(!(TEST)){raise(SIGTRAP);}
#define array_count(a) sizeof(a)/sizeof(a[0])
#ifdef SWIG
    #define EXPORT
#else
    #define EXPORT __attribute__((visibility("default")))
#endif
#define UNUSED __attribute__((unused))
#define PURE __attribute__((pure))
#define CONST __attribute__((const))
#define OVERLOADABLE __attribute__((overloadable))

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint32_t

#define s8  int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int32_t
