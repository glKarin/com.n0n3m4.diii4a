#ifndef CAP_H
#define CAP_H

#ifdef CONFIG_VIDEO_CAPTURE

#include "qtypes.h"
#include "qdefs.h"
#include "fs.h"
#include "snd_main.h"
#include "glquake.h"

typedef enum capturevideoformat_e
{
	CAPTUREVIDEOFORMAT_AVI_I420,
	CAPTUREVIDEOFORMAT_OGG_VORBIS_THEORA
}
capturevideoformat_t;

typedef struct capturevideostate_s
{
	double startrealtime;
	double framerate;
	int framestep;
	int framestepframe;
	qbool active;
	qbool realtime;
	qbool error;
	int soundrate;
	int soundchannels;
	int frame;
	double starttime;
	double lastfpstime;
	int lastfpsframe;
	int soundsampleframe;
	unsigned char *outbuffer;
	char basename[MAX_QPATH];
	int width, height;

	// precomputed RGB to YUV tables
	// converts the RGB values to YUV (see cap_avi.c for how to use them)
	short rgbtoyuvscaletable[3][3][256];
	unsigned char yuvnormalizetable[3][256];

	// precomputed gamma ramp (only needed if the capturevideo module uses RGB output)
	// note: to map from these values to RGB24, you have to multiply by 255.0/65535.0, then add 0.5, then cast to integer
	unsigned short vidramp[256 * 3];

	// stuff to be filled in by the video format module
	capturevideoformat_t format;
	const char *formatextension;
	qfile_t *videofile;
		// always use this:
		//   cls.capturevideo.videofile = FS_OpenRealFile(va(vabuf, sizeof(vabuf), "%s.%s", cls.capturevideo.basename, cls.capturevideo.formatextension), "wb", false);
	void (*writeEndVideo) (void);
	void (*writeVideoFrame) (int num, u8 *in);
	void (*writeSoundFrame) (const portable_sampleframe_t *paintbuffer, size_t length);

	// format specific data
	void *formatspecific;

	// GL backend
#define PBO_COUNT 3 // bones_was_here: slightly faster than double buffering
	GLuint PBOs[PBO_COUNT];
	GLuint PBOindex;
	GLuint FBO;
	GLuint FBOtex;
}
capturevideostate_t;
#endif

#endif
