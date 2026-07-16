#ifndef _DL_FFMPEG_INTERFACE_H
#define _DL_FFMPEG_INTERFACE_H

#ifdef DL_FFMPEG

extern int ffmpeg_available;
#define FFMPEG_AVAILABLE() (ffmpeg_available)
#define FFMPEG_IF_AVAILABLE if(FFMPEG_AVAILABLE())
#define FFMPEG_IF_NOT_AVAILABLE(x) if(!FFMPEG_AVAILABLE()) { x }
int FFmpeg_Init(void);
void FFmpeg_Shutdown(void);
#define FFMPEG_INIT() FFmpeg_Init()
#define FFMPEG_SHUTDOWN() FFmpeg_Shutdown()

#else

#define FFMPEG_AVAILABLE() (true)
#define FFMPEG_IF_AVAILABLE
#define FFMPEG_IF_NOT_AVAILABLE(x)
#define FFMPEG_INIT()
#define FFMPEG_SHUTDOWN()

#endif

#endif //_DL_FFMPEG_INTERFACE_H

