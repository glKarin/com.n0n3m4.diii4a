// for compat xunwind with dlopen

#ifndef _Q3E_Q_XUNWIND_H
#define _Q3E_Q_XUNWIND_H

// #include <android/log.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#if !defined(XUNWIND_CURRENT_PROCESS)
#define XUNWIND_CURRENT_PROCESS (-1)
#endif
#if !defined(XUNWIND_CURRENT_THREAD)
#define XUNWIND_CURRENT_THREAD  (-1)
#endif
#if !defined(XUNWIND_ALL_THREADS)
#define XUNWIND_ALL_THREADS     (-2)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int q_xunwind_android_LogPriority;

int q_xunwind_init(void);
void q_xunwind_shutdown(void);
int q_xunwind_is_initialized(void);

void q_xunwind_cfi_log(pid_t pid, pid_t tid, void *context, const char *logtag, q_xunwind_android_LogPriority priority,
                     const char *prefix);
void q_xunwind_cfi_dump(pid_t pid, pid_t tid, void *context, int fd, const char *prefix);
char *q_xunwind_cfi_get(pid_t pid, pid_t tid, void *context, const char *prefix);

size_t q_xunwind_fp_unwind(uintptr_t *frames, size_t frames_cap, void *context);
size_t q_xunwind_eh_unwind(uintptr_t *frames, size_t frames_cap, void *context);

void q_xunwind_frames_log(uintptr_t *frames, size_t frames_sz, const char *logtag, q_xunwind_android_LogPriority priority,
                        const char *prefix);
void q_xunwind_frames_dump(uintptr_t *frames, size_t frames_sz, int fd, const char *prefix);
char *q_xunwind_frames_get(uintptr_t *frames, size_t frames_sz, const char *prefix);

#ifdef __cplusplus
}
#endif

#endif //_Q3E_Q_XUNWIND_H
