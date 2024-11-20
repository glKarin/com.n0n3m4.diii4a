#ifndef Q_XUNWIND_PROC
#error "you must define Q_XUNWIND_PROC before including this file"
#endif

Q_XUNWIND_PROC(xunwind_cfi_log, void, (pid_t pid, pid_t tid, void *context, const char *logtag, q_xunwind_android_LogPriority priority, const char *prefix))
Q_XUNWIND_PROC(xunwind_cfi_dump, void, (pid_t pid, pid_t tid, void *context, int fd, const char *prefix))
Q_XUNWIND_PROC(xunwind_cfi_get, char *, (pid_t pid, pid_t tid, void *context, const char *prefix))

Q_XUNWIND_PROC(xunwind_fp_unwind, size_t, (uintptr_t *frames, size_t frames_cap, void *context))
Q_XUNWIND_PROC(xunwind_eh_unwind, size_t, (uintptr_t *frames, size_t frames_cap, void *context))

Q_XUNWIND_PROC(xunwind_frames_log, void, (uintptr_t *frames, size_t frames_sz, const char *logtag, q_xunwind_android_LogPriority priority, const char *prefix))
Q_XUNWIND_PROC(xunwind_frames_dump, void, (uintptr_t *frames, size_t frames_sz, int fd, const char *prefix))
Q_XUNWIND_PROC(xunwind_frames_get, char *, (uintptr_t *frames, size_t frames_sz, const char *prefix))

#undef Q_XUNWIND_PROC
