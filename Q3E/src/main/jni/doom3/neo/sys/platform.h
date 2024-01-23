// karin: for compat with dhewm3
#ifndef __PLATFORM__
#define __PLATFORM__

#include "../framework/BuildDefines.h"
#include "../idlib/precompiled.h"

#ifdef D3_OSTYPE
#define BUILD_OS D3_OSTYPE
#else // Android
#define BUILD_OS "Android"
#endif

#ifdef D3_ARCH
#define BUILD_CPU D3_ARCH
#else // Android
#ifdef __aarch64__
#define BUILD_CPU "aarch64"
#else
#define BUILD_CPU "arm32"
#endif
#endif

#endif
