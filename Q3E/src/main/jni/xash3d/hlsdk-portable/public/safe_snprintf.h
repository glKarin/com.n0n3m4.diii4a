#pragma once
#ifndef SAFE_SNPRINTF_H
#define SAFE_SNPRINTF_H
#include "build.h"
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#if XASH_WIN32
int safe_snprintf(char *buffer, int buffersize, const char *format, ...);
#else // XASH_WIN32
#define safe_snprintf	snprintf
#endif // XASH_WIN32
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // SAFE_SNPRINTF_H
