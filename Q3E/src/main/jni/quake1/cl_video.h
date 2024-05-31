
#ifndef CL_VIDEO_H
#define CL_VIDEO_H

#include "qtypes.h"
#include "qdefs.h"

#define CLVIDEOPREFIX	"video/"
#define CLTHRESHOLD		2.0

#define MENUOWNER		1

typedef enum clvideostate_e
{
	CLVIDEO_UNUSED,
	CLVIDEO_PLAY,
	CLVIDEO_LOOP,
	CLVIDEO_PAUSE,
	CLVIDEO_FIRSTFRAME,
	CLVIDEO_RESETONWAKEUP,
	CLVIDEO_STATECOUNT
} clvideostate_t;

#define CLVIDEO_MAX_SUBTITLES 512

extern struct cvar_s cl_video_subtitles;
extern struct cvar_s cl_video_subtitles_lines;
extern struct cvar_s cl_video_subtitles_textsize;
extern struct cvar_s cl_video_scale;
extern struct cvar_s cl_video_scale_vpos;
extern struct cvar_s cl_video_stipple;
extern struct cvar_s cl_video_brightness;
extern struct cvar_s cl_video_keepaspectratio;

typedef struct clvideo_s
{
	int		ownertag;
	clvideostate_t state;

	// private stuff
	void	*stream;

	double	starttime;
	int		framenum;
	double	framerate;

	void	*imagedata;

	// cachepic holds the relevant texture_t and we simply update the texture as needed
	struct cachepic_s *cachepic;
	char	name[MAX_QPATH]; // name of this video UI element (not the filename)
	int		width;
	int		height;

	// VorteX: subtitles array
	int		subtitles;
	char	*subtitle_text[CLVIDEO_MAX_SUBTITLES];
	float	subtitle_start[CLVIDEO_MAX_SUBTITLES];
	float	subtitle_end[CLVIDEO_MAX_SUBTITLES];

	// this functions gets filled by video format module
	void (*close) (void *stream);
	unsigned int (*getwidth) (void *stream);
	unsigned int (*getheight) (void *stream);
	double (*getframerate) (void *stream);
	double (*getaspectratio) (void *stream);
	int (*decodeframe) (void *stream, void *imagedata, unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, int imagebytesperrow);

	// if a video is suspended, it is automatically paused (else we'd still have to process the frames)
	// used to determine whether the video's resources should be freed or not
    double  lasttime;
	// when lasttime - realtime > THRESHOLD, all but the stream is freed
	qbool suspended;

	char	filename[MAX_QPATH];
} clvideo_t;

clvideo_t*	CL_OpenVideo( const char *filename, const char *name, int owner, const char *subtitlesfile );
clvideo_t*	CL_GetVideoByName( const char *name );
void		CL_SetVideoState( clvideo_t *video, clvideostate_t state );
void		CL_RestartVideo( clvideo_t *video );

void		CL_CloseVideo( clvideo_t * video );
void		CL_PurgeOwner( int owner );

void		CL_Video_Frame( void ); // update all videos
void		CL_Video_Init( void );
void		CL_Video_Shutdown( void );

// old interface
extern int cl_videoplaying;

void CL_DrawVideo( void );
void CL_VideoStart( char *filename, const char *subtitlesfile );
void CL_VideoStop( void );

// new function used for fullscreen videos
// TODO: Andreas Kirsch: move this subsystem somewhere else (preferably host) since the cl_video system shouldnt do such work like managing key events..
void CL_Video_KeyEvent( int key, int ascii, qbool down );

#endif
