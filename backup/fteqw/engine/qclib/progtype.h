#ifndef QCLIB_PROGTYPE_H
#define QCLIB_PROGTYPE_H

#if _MSC_VER >= 1300
	#define QC_ALIGN(a) __declspec(align(a))
#elif (__GNUC__ >= 3) || defined(__clang__)
	#define QC_ALIGN(a) __attribute__((aligned(a)))
#else
	#define QC_ALIGN(a)	//I hope misaligned accesses are okay...
#endif

#if 0
//64bit primitives allows for:
//	greater precision timers (so maps can last longer without getting restarted)
//	planet-sized maps (with the engine's vec_t types changed too, and with some sort of magic for the gpu's precision).
//TODO: for this to work, someone'll have to go through the code to somehow deal with the vec_t/pvec_t/float differences.
#warning FTE isnt ready for this.
#include <stdint.h>
typedef double pvec_t;
typedef int64_t pint_t;
typedef uint64_t puint_t;

#include <inttypes.h>
#define pPRId PRId64
#define pPRIi PRIi64
#define pPRIu PRIu64
#define pPRIx PRIx64
#define QCVM_64
#else
//use 32bit types, for sanity.
typedef float pvec_t;
typedef int pint_t;
typedef unsigned int puint_t;
#ifdef _MSC_VER
	typedef QC_ALIGN(4) __int64 pint64_t;
	typedef QC_ALIGN(4) unsigned __int64 puint64_t;

	#define pPRId "d"
	#define pPRIi "i"
	#define pPRIu "u"
	#define pPRIx "x"
	#define pPRIi64 "I64i"
	#define pPRIu64 "I64u"
	#define pPRIx64 "I64x"
	#define pPRIuSIZE PRIxPTR
#else
	#include <inttypes.h>
	typedef int64_t pint64_t QC_ALIGN(4);
	typedef uint64_t puint64_t QC_ALIGN(4);

	#define pPRId PRId32
	#define pPRIi PRIi32
	#define pPRIu PRIu32
	#define pPRIx PRIx32
	#define pPRIi64 PRIi64
	#define pPRIu64 PRIu64
	#define pPRIx64 PRIx64
	#define pPRIuSIZE PRIxPTR
#endif
#define QCVM_32
#endif

typedef QC_ALIGN(4) double pdouble_t;	//the qcvm uses vectors and stuff, so any 64bit types are only 4-byte aligned. we don't do atomics so this is fine so long as the compiler handles it for us.
typedef unsigned int pbool;
typedef pvec_t pvec3_t[3];
typedef pint_t progsnum_t;
typedef puint_t func_t;
typedef puint_t string_t;

extern pvec3_t pvec3_origin;

#endif /* QCLIB_PROGTYPE_H */

