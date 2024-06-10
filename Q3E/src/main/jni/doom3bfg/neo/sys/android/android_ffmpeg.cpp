#if defined(USE_FFMPEG)
#ifdef _DL_FFMPEG

#include "precompiled.h"

#if 0
#define FFMPEGDBG(x) x
#else
#define FFMPEGDBG(x)
#endif

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/imgutils.h>
}

#define QFFMPEGPROC(name, rettype, args) rettype (* q##name) args;
#include "qffmpeg.h"

static intptr_t avcodec = 0;
static intptr_t avformat = 0;
static intptr_t swresample = 0;
static intptr_t swrscale = 0;
bool ffmpeg_available = false;

extern const char * Sys_DLLDefaultPath(void);

static intptr_t FFmpeg_LoadLibrary(const char *so)
{
    idStr path = Sys_DLLDefaultPath();
    path.AppendPath(so);
    intptr_t handle = Sys_DLL_Load(path.c_str());
    printf("Load FFmpeg library(%s) -> %p\n", path.c_str(), (void *) handle);
    return handle;
}

static void FFmpeg_UnloadLibrary(intptr_t &handle)
{
    if(handle)
    {
        printf("Unload FFmpeg library(%p)\n", (void *) handle);
        Sys_DLL_Unload(handle);
        handle = 0;
    }
}

void FFmpeg_Shutdown(void)
{
    printf("----- FFmpeg shutdown -----\n");
    ffmpeg_available = false;
    FFmpeg_UnloadLibrary(avcodec);
    FFmpeg_UnloadLibrary(avformat);
    FFmpeg_UnloadLibrary(swresample);
    FFmpeg_UnloadLibrary(swrscale);
}

static bool FFmpeg_LoadSymbols(void)
{
    assert(avcodec);
#define QFFMPEG_MODULE avcodec
#define QFFMPEGPROC(name, rettype, args) { \
    q##name = (rettype (*) args)Sys_DLL_GetProcAddress(QFFMPEG_MODULE, #name); \
    FFMPEGDBG(printf("Sys_DLL_GetProcAddress(%lx, %s) -> %p\n", QFFMPEG_MODULE, #name, q##name)); \
    if(! q##name) {                            \
        return false; \
    }                                           \
}
#define QFFMPEG_AVCODEC
#include "qffmpeg.h"

    assert(avformat);
#define QFFMPEG_MODULE avformat
#define QFFMPEGPROC(name, rettype, args) { \
    q##name = (rettype (*) args)Sys_DLL_GetProcAddress(QFFMPEG_MODULE, #name); \
    FFMPEGDBG(printf("Sys_DLL_GetProcAddress(%lx, %s) -> %p\n", QFFMPEG_MODULE, #name, q##name)); \
    if(! q##name) {                            \
        return false; \
    }                                           \
}
#define QFFMPEG_AVFORMAT
#include "qffmpeg.h"

    assert(swresample);
#define QFFMPEG_MODULE swresample
#define QFFMPEGPROC(name, rettype, args) { \
    q##name = (rettype (*) args)Sys_DLL_GetProcAddress(QFFMPEG_MODULE, #name); \
    FFMPEGDBG(printf("Sys_DLL_GetProcAddress(%lx, %s) -> %p\n", QFFMPEG_MODULE, #name, q##name)); \
    if(! q##name) {                            \
        return false; \
    }                                           \
}
#define QFFMPEG_SWRESAMPLE
#include "qffmpeg.h"

    assert(swrscale);
#define QFFMPEG_MODULE swrscale
#define QFFMPEGPROC(name, rettype, args) { \
    q##name = (rettype (*) args)Sys_DLL_GetProcAddress(QFFMPEG_MODULE, #name); \
    FFMPEGDBG(printf("Sys_DLL_GetProcAddress(%lx, %s) -> %p\n", QFFMPEG_MODULE, #name, q##name)); \
    if(! q##name) {                            \
        return false; \
    }                                           \
}
#define QFFMPEG_SWRSCALE
#include "qffmpeg.h"

    return true;
}

bool FFmpeg_Init(void)
{
    printf("----- FFmpeg initialization -----\n");
    avcodec = FFmpeg_LoadLibrary("libavcodec.so");
    if(!avcodec)
    {
        FFmpeg_Shutdown();
        return false;
    }

    avformat = FFmpeg_LoadLibrary("libavformat.so");
    if(!avformat)
    {
        FFmpeg_Shutdown();
        return false;
    }

    swresample = FFmpeg_LoadLibrary("libswresample.so");
    if(!swresample)
    {
        FFmpeg_Shutdown();
        return false;
    }

    swrscale = FFmpeg_LoadLibrary("libswscale.so");
    if(!swrscale)
    {
        FFmpeg_Shutdown();
        return false;
    }

    if(!FFmpeg_LoadSymbols())
    {
        FFmpeg_Shutdown();
        return false;
    }

    ffmpeg_available = true;
    return true;
}

#endif
#endif
