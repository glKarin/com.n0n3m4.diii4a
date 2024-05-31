/*
	Libavcodec integration for Darkplaces by Timofeyev Pavel

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/

// LadyHavoc: for some reason this is being #include'd rather than treated as its own file...

#include "quakedef.h"
#include "client.h"
#include "cl_video.h"
#include "cl_video_libavw.h"

// scaler type
#define LIBAVW_SCALER_BILINEAR  0
#define LIBAVW_SCALER_BICUBIC   1
#define LIBAVW_SCALER_X         2
#define LIBAVW_SCALER_POINT     3
#define LIBAVW_SCALER_AREA      4
#define LIBAVW_SCALER_BICUBLIN  5
#define LIBAVW_SCALER_GAUSS     6
#define LIBAVW_SCALER_SINC      7
#define LIBAVW_SCALER_LANCZOS   8
#define LIBAVW_SCALER_SPLINE    9
// output format
#define LIBAVW_PIXEL_FORMAT_BGR   0
#define LIBAVW_PIXEL_FORMAT_BGRA  1
// print levels
#define LIBAVW_PRINT_WARNING 1
#define LIBAVW_PRINT_ERROR   2
#define LIBAVW_PRINT_FATAL   3
#define LIBAVW_PRINT_PANIC   4
// exported callback functions:
typedef void    avwCallbackPrint(int, const char *);
typedef int     avwCallbackIoRead(void *, uint8_t *, int);
typedef uint64_t avwCallbackIoSeek(void *, uint64_t, int);
typedef uint64_t avwCallbackIoSeekSize(void *);
// exported functions:
int         (*qLibAvW_Init)(avwCallbackPrint *printfunction); // init library, returns error code
const char *(*qLibAvW_ErrorString)(int errorcode); // get string for error code
const char *(*qLibAvW_AvcVersion)(void); // get a string containing libavcodec version wrapper was built for
float       (*qLibAvW_Version)(void); // get wrapper version
int         (*qLibAvW_CreateStream)(void **stream); // create stream, returns error code
void        (*qLibAvW_RemoveStream)(void *stream); // flush and remove stream
int         (*qLibAvW_StreamGetVideoWidth)(void *stream); // get video parameters of stream
int         (*qLibAvW_StreamGetVideoHeight)(void *stream);
double      (*qLibAvW_StreamGetFramerate)(void *stream);
int         (*qLibAvW_StreamGetError)(void *stream); // get last function errorcode from stream
// simple API to play video
int (*qLibAvW_PlayVideo)(void *stream, void *file, avwCallbackIoRead *IoRead, avwCallbackIoSeek *IoSeek, avwCallbackIoSeekSize *IoSeekSize);
int (*qLibAvW_PlaySeekNextFrame)(void *stream);
int (*qLibAvW_PlayGetFrameImage)(void *stream, int pixel_format, void *imagedata, int imagewidth, int imageheight, int scaler);

static dllfunction_t libavwfuncs[] =
{
	{"LibAvW_Init",                (void **) &qLibAvW_Init },
	{"LibAvW_ErrorString",         (void **) &qLibAvW_ErrorString },
	{"LibAvW_AvcVersion",          (void **) &qLibAvW_AvcVersion },
	{"LibAvW_Version",             (void **) &qLibAvW_Version },
	{"LibAvW_CreateStream",        (void **) &qLibAvW_CreateStream },
	{"LibAvW_RemoveStream",        (void **) &qLibAvW_RemoveStream },
	{"LibAvW_StreamGetVideoWidth", (void **) &qLibAvW_StreamGetVideoWidth },
	{"LibAvW_StreamGetVideoHeight",(void **) &qLibAvW_StreamGetVideoHeight },
	{"LibAvW_StreamGetFramerate",  (void **) &qLibAvW_StreamGetFramerate },
	{"LibAvW_StreamGetError",      (void **) &qLibAvW_StreamGetError },
	{"LibAvW_PlayVideo",           (void **) &qLibAvW_PlayVideo },
	{"LibAvW_PlaySeekNextFrame",   (void **) &qLibAvW_PlaySeekNextFrame },
	{"LibAvW_PlayGetFrameImage",   (void **) &qLibAvW_PlayGetFrameImage },
	{NULL, NULL}
};

const char* dllnames_libavw[] =
{
#if defined(WIN32)
		"libavcodec.dll",
#elif defined(MACOSX)
		"libavcodec.dylib",
#else
		"libavcodec.so",
#endif
		NULL
};

static dllhandle_t libavw_dll = NULL;

// DP videostream
typedef struct libavwstream_s
{
	qfile_t     *file;
	double       info_framerate;
	unsigned int info_imagewidth;
	unsigned int info_imageheight;
	double       info_aspectratio;
	void        *stream;

	// channel the sound file is being played on
	sfx_t *sfx;
	int    sndchan;
	int    sndstarted;
}
libavwstream_t;

cvar_t cl_video_libavw_minwidth  = {CF_ARCHIVE, "cl_video_libavw_minwidth", "0", "if videos width is lesser than minimal, thay will be upscaled"};
cvar_t cl_video_libavw_minheight = {CF_ARCHIVE, "cl_video_libavw_minheight", "0", "if videos height is lesser than minimal, thay will be upscaled"};
cvar_t cl_video_libavw_scaler    = {CF_ARCHIVE, "cl_video_libavw_scaler", "1", "selects a scaler for libavcode played videos. Scalers are: 0 - bilinear, 1 - bicubic, 2 - x, 3 - point, 4 - area, 5 - bicublin, 6 - gauss, 7 - sinc, 8 - lanczos, 9 - spline."};

// video extensions
const char* libavw_extensions[] =
{
	"ogv",
	"avi",
	"mpg",
	"mp4",
	"mkv",
	"webm",
	"bik",
	"roq",
	"flv",
	"wmv",
	"mpeg",
	"mjpeg",
	"mpeg4",
	NULL
};

/*
=================================================================

  Video decoding
  a features that is not supported yet and likely to be done
  - streaming audio from videofiles
  - streaming subtitles

=================================================================
*/

unsigned int libavw_getwidth(void *stream);
unsigned int libavw_getheight(void *stream);
double libavw_getframerate(void *stream);
double libavw_getaspectratio(void *stream);
void libavw_close(void *stream);

static int libavw_decodeframe(void *stream, void *imagedata, unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, int imagebytesperrow)
{
	int pixel_format = LIBAVW_PIXEL_FORMAT_BGR;
	int errorcode;

	libavwstream_t *s = (libavwstream_t *)stream;

	// start sound
	if (!s->sndstarted)
	{
		if (s->sfx != NULL)
			s->sndchan = S_StartSound(-1, 0, s->sfx, vec3_origin, 1.0f, 0);
		s->sndstarted = 1;
	}

	// read frame
	if (!qLibAvW_PlaySeekNextFrame(s->stream))
	{
		// got error or file end
		errorcode = qLibAvW_StreamGetError(s->stream);
		if (errorcode)
			Con_Printf(CON_ERROR "LibAvW: %s\n", qLibAvW_ErrorString(errorcode));
		return 1;
	}

	// decode into bgr texture
	if (bytesperpixel == 4)
		pixel_format = LIBAVW_PIXEL_FORMAT_BGRA;
	else if (bytesperpixel == 3)
		pixel_format = LIBAVW_PIXEL_FORMAT_BGR;
	else
	{
		Con_Printf(CON_ERROR "LibAvW: cannot determine pixel format for bpp %i\n", bytesperpixel);
		return 1;
	}
	if (!qLibAvW_PlayGetFrameImage(s->stream, pixel_format, imagedata, s->info_imagewidth, s->info_imageheight, min(9, max(0, cl_video_libavw_scaler.integer))))
		Con_Printf(CON_ERROR "LibAvW: %s\n", qLibAvW_ErrorString(qLibAvW_StreamGetError(s->stream)));
	return 0;
}

// get stream info
unsigned int libavw_getwidth(void *stream)
{
	return ((libavwstream_t *)stream)->info_imagewidth;
}

unsigned int libavw_getheight(void *stream)
{
	return ((libavwstream_t *)stream)->info_imageheight;
}

double libavw_getframerate(void *stream)
{
	return ((libavwstream_t *)stream)->info_framerate;
}

double libavw_getaspectratio(void *stream)
{
	return ((libavwstream_t *)stream)->info_aspectratio;
}

// close stream
void libavw_close(void *stream)
{
	libavwstream_t *s = (libavwstream_t *)stream;

	if (s->stream)
		qLibAvW_RemoveStream(s->stream);
	s->stream = NULL;
	if (s->file)
		FS_Close(s->file);
	s->file = NULL;
	if (s->sndchan >= 0)
		S_StopChannel(s->sndchan, true, true);
	s->sndchan = -1;
}

// IO wrapper
static int LibAvW_FS_Read(void *opaque, uint8_t *buf, int buf_size)
{
	return FS_Read((qfile_t *)opaque, buf, buf_size);
}
static uint64_t LibAvW_FS_Seek(void *opaque, uint64_t pos, int whence)
{
	return (uint64_t)FS_Seek((qfile_t *)opaque, pos, whence);
}
static uint64_t LibAvW_FS_SeekSize(void *opaque)
{
	return (uint64_t)FS_FileSize((qfile_t *)opaque);
}

// open as DP video stream
void *LibAvW_OpenVideo(clvideo_t *video, char *filename, const char **errorstring)
{
	libavwstream_t *s;
	char filebase[MAX_OSPATH], check[MAX_OSPATH];
	unsigned int i;
	int errorcode;
	char *wavename;
	size_t len;

	if (!libavw_dll)
		return NULL;

	// allocate stream
	s = (libavwstream_t *)Z_Malloc(sizeof(libavwstream_t));
	if (s == NULL)
	{
		*errorstring = "unable to allocate memory for stream info structure";
		return NULL;
	}
	memset(s, 0, sizeof(libavwstream_t));
	s->sndchan = -1;

	// open file
	s->file = FS_OpenVirtualFile(filename, true);
	if (!s->file)
	{
		FS_StripExtension(filename, filebase, sizeof(filebase));
		// we tried .dpv, try another extensions
		for (i = 0; libavw_extensions[i] != NULL; i++)
		{
			dpsnprintf(check, sizeof(check), "%s.%s", filebase, libavw_extensions[i]);
			s->file = FS_OpenVirtualFile(check, true);
			if (s->file)
				break;
		}
		if (!s->file)
		{
			*errorstring = "unable to open videofile";
			libavw_close(s);
			Z_Free(s);
			 return NULL;
		}
	}

	// allocate libavw stream
	if ((errorcode = qLibAvW_CreateStream(&s->stream)))
	{
		*errorstring = qLibAvW_ErrorString(errorcode);
		libavw_close(s);
		Z_Free(s);
        return NULL;
	}

	// open video for playing
	if (!qLibAvW_PlayVideo(s->stream, s->file, &LibAvW_FS_Read, &LibAvW_FS_Seek, &LibAvW_FS_SeekSize))
	{
		*errorstring = qLibAvW_ErrorString(qLibAvW_StreamGetError(s->stream));
		libavw_close(s);
		Z_Free(s);
        return NULL;
	}

	// all right, start codec
	s->info_imagewidth = qLibAvW_StreamGetVideoWidth(s->stream);
	s->info_imageheight = qLibAvW_StreamGetVideoHeight(s->stream);
	s->info_framerate = qLibAvW_StreamGetFramerate(s->stream);
	s->info_aspectratio = (double)s->info_imagewidth / (double)s->info_imageheight;
	video->close = libavw_close;
	video->getwidth = libavw_getwidth;
	video->getheight = libavw_getheight;
	video->getframerate = libavw_getframerate;
	video->decodeframe = libavw_decodeframe;
	video->getaspectratio = libavw_getaspectratio;

	// apply min-width, min-height, keep aspect rate
	if (cl_video_libavw_minwidth.integer > 0)
		s->info_imagewidth = max(s->info_imagewidth, (unsigned int)cl_video_libavw_minwidth.integer);
	if (cl_video_libavw_minheight.integer > 0)
		s->info_imageheight = max(s->info_imageheight, (unsigned int)cl_video_libavw_minheight.integer);
	
	// provide sound in separate .wav
	len = strlen(filename) + 10;
	wavename = (char *)Z_Malloc(len);
	if (wavename)
	{
		FS_StripExtension(filename, wavename, len-1);
		dp_strlcat(wavename, ".wav", len);
		s->sfx = S_PrecacheSound(wavename, false, false);
		s->sndchan = -1;
		Z_Free(wavename);
	}
	return s;
}

static void libavw_message(int level, const char *message)
{
	if (level == LIBAVW_PRINT_WARNING)
		Con_Printf(CON_WARN "LibAvcodec warning: %s\n", message);
	else if (level == LIBAVW_PRINT_ERROR)
		Con_Printf(CON_ERROR "LibAvcodec error: %s\n", message);
	else if (level == LIBAVW_PRINT_FATAL)
		Con_Printf(CON_ERROR "LibAvcodec fatal error: %s\n", message);
	else
		Con_Printf(CON_ERROR "LibAvcodec panic: %s\n", message);
}

qbool LibAvW_OpenLibrary(void)
{
	int errorcode;

	// COMMANDLINEOPTION: Video: -nolibavw disables libavcodec wrapper support
	if (Sys_CheckParm("-nolibavw"))
		return false;

	// load DLL's
	Sys_LoadDependency(dllnames_libavw, &libavw_dll, libavwfuncs);
	if (!libavw_dll)
		return false;

	// initialize libav wrapper
	if ((errorcode = qLibAvW_Init(&libavw_message)))
	{
		Con_Printf(CON_ERROR "LibAvW failed to initialize: %s\n", qLibAvW_ErrorString(errorcode));
		Sys_FreeLibrary(&libavw_dll);
	}

	Cvar_RegisterVariable(&cl_video_libavw_minwidth);
	Cvar_RegisterVariable(&cl_video_libavw_minheight);
	Cvar_RegisterVariable(&cl_video_libavw_scaler);

	return true;
}

void LibAvW_CloseLibrary(void)
{
	Sys_FreeLibrary(&libavw_dll);
}
