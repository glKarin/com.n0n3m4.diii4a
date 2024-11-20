#include "bt.h"

#define Q3E_PRINTF printf

#include <android/log.h>
#include <errno.h>
#include <jni.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>

#include "q_xunwind.h"

#define Q3E_XUNWIND_LOG_TAG        "Q3E_xunwind"
#define Q3E_XUNWIND_LOG_PRIORITY   ANDROID_LOG_INFO
#define Q3E_XUNWIND_LOG(fmt, ...) do { \
__android_log_print(Q3E_XUNWIND_LOG_PRIORITY, Q3E_XUNWIND_LOG_TAG, fmt, ##__VA_ARGS__); \
Q3E_PRINTF(fmt "\n", ##__VA_ARGS__);                               \
} while(0);

#define Q3E_XUNWIND_MAX_FRAMES 128
#define Q3E_XUNWIND_PREFIX "\t"

static int (*g_afterCaughtSignal)(int num);
static void (*g_signalCaughted)(int num, int pid, int tid, int mask, const char *cfi, const char *fp, const char *eh);

static int g_solution = 0xFF;
static bool g_with_context = false;

static void after_caught_signal(int signum)
{
    if (g_afterCaughtSignal)
        g_afterCaughtSignal(signum);
    else
        abort();
}

static void print_frames(int signum, int mask, const char *str)
{
    pid_t pid = getpid();
    pid_t tid = gettid();
    Q3E_XUNWIND_LOG("Signal caught: %d, pid=%d, tid=%d", signum, pid, tid);
    if(mask == SAMPLE_SOLUTION_FP)
    {
        Q3E_XUNWIND_LOG("Backtrace: FP (Frame Pointer)");
        Q3E_XUNWIND_LOG("%s", str ? str : "<NULL>");
    }
    else if(mask == SAMPLE_SOLUTION_EH)
    {
        Q3E_XUNWIND_LOG("Backtrace: EH (Exception handling GCC extension)");
        Q3E_XUNWIND_LOG("%s", str ? str : "<NULL>");
    }
    else if(mask == SAMPLE_SOLUTION_CFI)
    {
        Q3E_XUNWIND_LOG("Backtrace: CFI (Call Frame Info)");
        Q3E_XUNWIND_LOG("%s", str ? str : "<NULL>");
    }
}

static void signal_caughted(int signum, int mask, char *cfi, char *fp, char *eh)
{
    if (g_signalCaughted)
    {
        pid_t pid = getpid();
        pid_t tid = gettid();
        g_signalCaughted(signum, pid, tid, mask, cfi, fp, eh);
    }

    free(cfi);
    free(fp);
    free(eh);
}

static void sample_sigsegv_handler(int signum, siginfo_t *siginfo, void *context) {
    (void) siginfo;

    signal(signum, SIG_DFL); // forbidden double

    Q3E_XUNWIND_LOG("Caught signal: %d", signum);

    int mask = 0;
    char *fp = NULL;
    char *eh = NULL;
    char *cfi = NULL;

    if (g_solution & SAMPLE_SOLUTION_CFI) {
        cfi = q_xunwind_cfi_get(XUNWIND_CURRENT_PROCESS, XUNWIND_CURRENT_THREAD, context, Q3E_XUNWIND_PREFIX);
        mask |= SAMPLE_SOLUTION_CFI;
        print_frames(signum, SAMPLE_SOLUTION_CFI, cfi);
    }

    if (g_solution & SAMPLE_SOLUTION_FP) {
#ifdef __aarch64__
        uintptr_t g_frames[Q3E_XUNWIND_MAX_FRAMES];
        // FP local unwind
        size_t frames_sz = q_xunwind_fp_unwind(g_frames, sizeof(g_frames) / sizeof(g_frames[0]),
                                             g_with_context ? context : NULL);
        fp = q_xunwind_frames_get(g_frames, frames_sz, Q3E_XUNWIND_PREFIX);
        mask |= SAMPLE_SOLUTION_FP;
        print_frames(signum, SAMPLE_SOLUTION_FP, fp);
#else
        (void) context;
        Q3E_XUNWIND_LOG("FP unwinding is only supported on arm64.");
#endif
    }

    if (g_solution & SAMPLE_SOLUTION_EH) {
        uintptr_t g_frames[Q3E_XUNWIND_MAX_FRAMES];
        // EH local unwind
        size_t frames_sz = q_xunwind_eh_unwind(g_frames, sizeof(g_frames) / sizeof(g_frames[0]),
                                             g_with_context ? context : NULL);

        eh = q_xunwind_frames_get(g_frames, frames_sz, Q3E_XUNWIND_PREFIX);
        mask |= SAMPLE_SOLUTION_EH;
        print_frames(signum, SAMPLE_SOLUTION_EH, eh);
    }

    signal_caughted(signum, mask, cfi, fp, eh);

    after_caught_signal(signum);
}

// ----------- lots of signal handling stuff ------------
static const int sigs[] = {SIGILL, SIGABRT, SIGFPE, SIGSEGV};
//static const char* crashSigNames[] = { "SIGILL", "SIGABRT", "SIGFPE", "SIGSEGV" };

void Q3E_BT_Init(void)
{
    if(q_xunwind_is_initialized())
        return;

    q_xunwind_init();

    int i;
    for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
        struct sigaction act;

        memset(&act, 0, sizeof(act));
        sigemptyset(&act.sa_mask);
        //sigfillset(&act.sa_mask);
        //sigdelset(&act.sa_mask, SIGSEGV);
        act.sa_sigaction = sample_sigsegv_handler; //  | SA_SIGINFO
        //act.sa_handler = sigsegv_handler;
        act.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
        sigaction(sigs[i], &act, NULL);
        //signal(sigs[i], sigsegv_handler);
        Q3E_XUNWIND_LOG("[%d] Register signal: %d", i, sigs[i]);
    }
}

void Q3E_BT_Shutdown(void)
{
    if(q_xunwind_is_initialized())
    {
        q_xunwind_shutdown();
        int i;
        for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
            signal(sigs[i], SIG_DFL);
            Q3E_XUNWIND_LOG("[%d] Unregister signal: %d", i, sigs[i]);
        }
    }
}

void Q3E_BT_SetupSolution(int mask) {
    g_solution = mask;
}

void Q3E_BT_AfterCaught(int (*func) (int))
{
    g_afterCaughtSignal = func;
}

void Q3E_BT_SignalCaughted(void (*func) (int num, int pid, int tid, int mask, const char *cfi, const char *fp, const char *eh))
{
    g_signalCaughted = func;
}
