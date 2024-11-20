// for compat xunwind with dlopen

#include "q_xunwind.h"

#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>

#include <android/log.h>

#define LOG_TAG "Q_XUNWIND"

//#define Q_XUNWIND_STRICT

#define LOGI(fmt, args...) { printf("[" LOG_TAG " info]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args); }
#define LOGW(fmt, args...) { printf("[" LOG_TAG " warning]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_WARN, LOG_TAG, fmt, ##args); }
#define LOGE(fmt, args...) { printf("[" LOG_TAG " error]" fmt "\n", ##args); __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args); }

#ifdef Q_XUNWIND_STRICT
#define Q_XUNWIND_CHECK_INIT(x) if(!handle) { LOGE("q_xunwind is not initialized!\n"); abort(); };
#define Q_XUNWIND_CALL(name, args, def) if(name##_fptr) { return name##_fptr args; } else { LOGE(#name " missing!\n"); abort(); return def; }
#else
#define Q_XUNWIND_CHECK_INIT(x) if(!handle) { LOGW(" q_xunwind is not initialized!\n"); return x; };
#define Q_XUNWIND_CALL(name, args, def) if(name##_fptr) { return name##_fptr args; } else { LOGW(#name " missing!\n"); return def; }
#endif

#define XUNWIND_LIBRARY "libxunwind.so"

#define Q_XUNWIND_PROC(name, ret, args) static ret (* name##_fptr) args;
#include "q_xunwind_proc.h"

static void *handle = NULL;


int q_xunwind_init(void)
{
    if(handle)
    {
        LOGE("q_xunwind has initialized!\n");
        return 1;
    }

    int num = 0;

    handle = dlopen(XUNWIND_LIBRARY, RTLD_NOW | RTLD_GLOBAL);
    if(!handle)
    {
        LOGE("q_xunwind load '" XUNWIND_LIBRARY "' fail -> %s!\n", dlerror());
        return 0;
    }

#define Q_XUNWIND_PROC(name, ret, args) \
    name##_fptr = (ret (*) args) dlsym(handle, #name);       \
    if(!name##_fptr) {                                        \
        LOGE("q_xunwind missing '" #name "' function!\n"); \
    } else num++;
#include "q_xunwind_proc.h"

    LOGI("q_xunwind initialized(%d functions)!\n", num);

    return 1;
}

void q_xunwind_shutdown(void)
{
    if(!handle)
    {
        LOGE("q_xunwind is not initialized!\n");
        return;
    }
#define Q_XUNWIND_PROC(name, ret, args) name##_fptr = NULL;
#include "q_xunwind_proc.h"

    dlclose(handle);
    handle = NULL;

    LOGI("q_xunwind shutdown!\n");
}

int q_xunwind_is_initialized(void)
{
    return handle != NULL ? 1 : 0;
}



void q_xunwind_cfi_log(pid_t pid, pid_t tid, void *context, const char *logtag, q_xunwind_android_LogPriority priority, const char *prefix)
{
    Q_XUNWIND_CHECK_INIT(;);

    Q_XUNWIND_CALL(xunwind_cfi_log, (pid, tid, context, logtag, priority, prefix), ;);
}

void q_xunwind_cfi_dump(pid_t pid, pid_t tid, void *context, int fd, const char *prefix)
{
    Q_XUNWIND_CHECK_INIT(;);

    Q_XUNWIND_CALL(xunwind_cfi_dump, (pid, tid, context, fd, prefix), ;);
}

char *q_xunwind_cfi_get(pid_t pid, pid_t tid, void *context, const char *prefix)
{
    Q_XUNWIND_CHECK_INIT(NULL);

    Q_XUNWIND_CALL(xunwind_cfi_get, (pid, tid, context, prefix), NULL);
}


size_t q_xunwind_fp_unwind(uintptr_t *frames, size_t frames_cap, void *context)
{
    Q_XUNWIND_CHECK_INIT(0);

    Q_XUNWIND_CALL(xunwind_fp_unwind, (frames, frames_cap, context), 0);
}

size_t q_xunwind_eh_unwind(uintptr_t *frames, size_t frames_cap, void *context)
{
    Q_XUNWIND_CHECK_INIT(0);

    Q_XUNWIND_CALL(xunwind_eh_unwind, (frames, frames_cap, context), 0);
}


void q_xunwind_frames_log(uintptr_t *frames, size_t frames_sz, const char *logtag, q_xunwind_android_LogPriority priority, const char *prefix)
{
    Q_XUNWIND_CHECK_INIT(;);

    Q_XUNWIND_CALL(xunwind_frames_log, (frames, frames_sz, logtag, priority, prefix), ;);
}

void q_xunwind_frames_dump(uintptr_t *frames, size_t frames_sz, int fd, const char *prefix)
{
    Q_XUNWIND_CHECK_INIT(;);

    Q_XUNWIND_CALL(xunwind_frames_dump, (frames, frames_sz, fd, prefix), ;);
}

char *q_xunwind_frames_get(uintptr_t *frames, size_t frames_sz, const char *prefix)
{
    Q_XUNWIND_CHECK_INIT(NULL);

    Q_XUNWIND_CALL(xunwind_frames_get, (frames, frames_sz, prefix), NULL);
}

