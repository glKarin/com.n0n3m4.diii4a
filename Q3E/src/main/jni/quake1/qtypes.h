
#ifndef QTYPES_H
#define QTYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

typedef bool qbool;

#ifndef NULL
#define NULL ((void *)0)
#endif

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2

#if defined(__GNUC__) || (__clang__) || (__TINYC__) || (_MSC_VER >= 1400)
#define RESTRICT __restrict
#else
#define RESTRICT
#endif

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef intptr_t iptr;
typedef uintptr_t uptr;

// LadyHavoc: upgrade the prvm to double precision for better time values
// LadyHavoc: to be enabled when bugs are worked out...
//#define PRVM_64
#ifdef PRVM_64
typedef double prvm_vec_t;
typedef int64_t prvm_int_t;
typedef uint64_t prvm_uint_t;
#define PRVM_PRIi PRIi64
#define PRVM_PRIu PRIu64
#else
typedef float prvm_vec_t;
typedef int32_t prvm_int_t;
typedef uint32_t prvm_uint_t;
#define PRVM_PRIi PRIi32
#define PRVM_PRIu PRIu32
#endif
typedef prvm_vec_t prvm_vec3_t[3];

#ifdef VEC_64
typedef double vec_t;
#else
typedef float vec_t;
#endif
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
#endif
