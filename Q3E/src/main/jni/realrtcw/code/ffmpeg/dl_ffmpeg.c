#ifdef DL_FFMPEG

#include "../sys/sys_local.h"
#include "../sys/sys_loadlib.h"

#if 0
#define FFMPEGDBG(x) x
#else
#define FFMPEGDBG(x)
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>

#define QFFMPEGPROC(name, rettype, args) rettype (* q##name) args;
#include "qffmpeg.h"

static intptr_t avcodec = 0;
static intptr_t avformat = 0;
static intptr_t swresample = 0;
static intptr_t swrscale = 0;
static intptr_t avutil = 0;
int ffmpeg_available = 0;

static intptr_t FFmpeg_LoadLibrary(const char *so)
{
    intptr_t handle = (intptr_t)Sys_LoadLibrary(so);
    printf("Load FFmpeg library(%s) -> %p\n", so, (void *) handle);
    return handle;
}

static void FFmpeg_UnloadLibrary(intptr_t *handle)
{
    if(*handle)
    {
        printf("Unload FFmpeg library(%p)\n", (void *) handle);
        Sys_UnloadLibrary((void *)*handle);
        *handle = 0;
    }
}

void FFmpeg_Shutdown(void)
{
    printf("----- FFmpeg shutdown -----\n");
    ffmpeg_available = 0;
    FFmpeg_UnloadLibrary(&avcodec);
    FFmpeg_UnloadLibrary(&avformat);
    FFmpeg_UnloadLibrary(&swresample);
    FFmpeg_UnloadLibrary(&swrscale);
    FFmpeg_UnloadLibrary(&avutil);
}

static int FFmpeg_LoadSymbols(void)
{
    assert(avcodec);
#define QFFMPEG_MODULE avcodec
#define QFFMPEGPROC(name, rettype, args) { \
    q##name = (rettype (*) args)Sys_LoadFunction((void *)QFFMPEG_MODULE, #name); \
    FFMPEGDBG(printf("Sys_LoadFunction(%lx, %s) -> %p\n", QFFMPEG_MODULE, #name, q##name)); \
    if(! q##name) {                            \
        return 0; \
    }                                           \
}
#define QFFMPEG_AVCODEC
#include "qffmpeg.h"

    assert(avformat);
#define QFFMPEG_MODULE avformat
#define QFFMPEGPROC(name, rettype, args) { \
    q##name = (rettype (*) args)Sys_LoadFunction((void *)QFFMPEG_MODULE, #name); \
    FFMPEGDBG(printf("Sys_LoadFunction(%lx, %s) -> %p\n", QFFMPEG_MODULE, #name, q##name)); \
    if(! q##name) {                            \
        return 0; \
    }                                           \
}
#define QFFMPEG_AVFORMAT
#include "qffmpeg.h"

    assert(swresample);
#define QFFMPEG_MODULE swresample
#define QFFMPEGPROC(name, rettype, args) { \
    q##name = (rettype (*) args)Sys_LoadFunction((void *)QFFMPEG_MODULE, #name); \
    FFMPEGDBG(printf("Sys_LoadFunction(%lx, %s) -> %p\n", QFFMPEG_MODULE, #name, q##name)); \
    if(! q##name) {                            \
        return 0; \
    }                                           \
}
#define QFFMPEG_SWRESAMPLE
#include "qffmpeg.h"

    assert(swrscale);
#define QFFMPEG_MODULE swrscale
#define QFFMPEGPROC(name, rettype, args) { \
    q##name = (rettype (*) args)Sys_LoadFunction((void *)QFFMPEG_MODULE, #name); \
    FFMPEGDBG(printf("Sys_LoadFunction(%lx, %s) -> %p\n", QFFMPEG_MODULE, #name, q##name)); \
    if(! q##name) {                            \
        return 0; \
    }                                           \
}
#define QFFMPEG_SWRSCALE
#include "qffmpeg.h"

    assert(avutil);
#define QFFMPEG_MODULE avutil
#define QFFMPEGPROC(name, rettype, args) { \
    q##name = (rettype (*) args)Sys_LoadFunction((void *)QFFMPEG_MODULE, #name); \
    FFMPEGDBG(printf("Sys_LoadFunction(%lx, %s) -> %p\n", QFFMPEG_MODULE, #name, q##name)); \
    if(! q##name) {                            \
        return 0; \
    }                                           \
}
#define QFFMPEG_AVUTIL
#include "qffmpeg.h"

    return 1;
}

int FFmpeg_Init(void)
{
    printf("----- FFmpeg initialization -----\n");
	// load libraries
    avcodec = FFmpeg_LoadLibrary("libavcodec.so");
    if(!avcodec)
    {
        FFmpeg_Shutdown();
        return 0;
    }

    avformat = FFmpeg_LoadLibrary("libavformat.so");
    if(!avformat)
    {
        FFmpeg_Shutdown();
        return 0;
    }

    swresample = FFmpeg_LoadLibrary("libswresample.so");
    if(!swresample)
    {
        FFmpeg_Shutdown();
        return 0;
    }

    swrscale = FFmpeg_LoadLibrary("libswscale.so");
    if(!swrscale)
    {
        FFmpeg_Shutdown();
        return 0;
    }

    avutil = FFmpeg_LoadLibrary("libavutil.so");
    if(!avutil)
    {
        FFmpeg_Shutdown();
        return 0;
    }

	// load symbols
    if(!FFmpeg_LoadSymbols())
    {
        FFmpeg_Shutdown();
        return 0;
    }

    ffmpeg_available = 1;
    return 1;
}

#endif
