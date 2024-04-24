/*
	Copyright (C) 2014 nyov <nyov@nexnode.net>

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * libxmp is licensed under the terms of the Lesser General Public License 2.1
 */


#include "darkplaces.h"
#include "snd_main.h"
#include "snd_xmp.h"
#include "sound.h"

#ifdef LINK_TO_LIBXMP
#include <xmp.h>
#if ((XMP_VERCODE+0) < 0x040200)
#error libxmp version 4.2 or newer is required when linking to libxmp
#endif

/* libxmp API */
// Version and player information
#define qxmp_version xmp_version // const char *xmp_version
#define qxmp_vercode xmp_vercode // const unsigned int xmp_vercode
//#define qxmp_get_format_list xmp_get_format_list // char **xmp_get_format_list()
// Context creation
#define qxmp_create_context xmp_create_context // xmp_context xmp_create_context()
#define qxmp_free_context xmp_free_context // void xmp_free_context(xmp_context c)
// Module loading
//#define qxmp_test_module xmp_test_module // int xmp_test_module(char *path, struct xmp_test_info *test_info)
//#define qxmp_load_module xmp_load_module // int xmp_load_module(xmp_context c, char *path)
#define qxmp_load_module_from_memory xmp_load_module_from_memory // int xmp_load_module_from_memory(xmp_context c, void *mem, long size)
//#define qxmp_load_module_from_file xmp_load_module_from_file // int xmp_load_module_from_file(xmp_context c, FILE *f, long size)
#define qxmp_release_module xmp_release_module // void xmp_release_module(xmp_context c)
//#define qxmp_scan_module xmp_scan_module // void xmp_scan_module(xmp_context c)
#define qxmp_get_module_info xmp_get_module_info // void xmp_get_module_info(xmp_context c, struct xmp_module_info *info)
// Module playing
#define qxmp_start_player xmp_start_player // int xmp_start_player(xmp_context c, int rate, int format)
#define qxmp_play_frame xmp_play_frame // int xmp_play_frame(xmp_context c)
#define qxmp_play_buffer xmp_play_buffer // int xmp_play_buffer(xmp_context c, void *buffer, int size, int loop)
#define qxmp_get_frame_info xmp_get_frame_info // void xmp_get_frame_info(xmp_context c, struct xmp_frame_info *info)
#define qxmp_end_player xmp_end_player // void xmp_end_player(xmp_context c)
// Player control
//#define qxmp_next_position xmp_next_position // int xmp_next_position(xmp_context c)
//#define qxmp_prev_position xmp_prev_position// int xmp_prev_position(xmp_context c)
//#define qxmp_set_position xmp_set_position // int xmp_set_position(xmp_context c, int pos)
//#define qxmp_stop_module xmp_stop_module // void xmp_stop_module(xmp_context c)
//#define qxmp_restart_module xmp_restart_module // void xmp_restart_module(xmp_context c)
//#define qxmp_seek_time xmp_seek_time // int xmp_seek_time(xmp_context c, int time)
//#define qxmp_channel_mute xmp_channel_mute // int xmp_channel_mute(xmp_context c, int channel, int status)
//#define qxmp_channel_vol xmp_channel_vol // int xmp_channel_vol(xmp_context c, int channel, int vol)
//#define qxmp_inject_event xmp_inject_event // void xmp_inject_event(xmp_context c, int channel, struct xmp_event *event)
// Player parameter setting
//#define qxmp_set_instrument_path xmp_set_instrument_path // int xmp_set_instrument_path(xmp_context c, char *path)
#define qxmp_get_player	xmp_get_player // int xmp_get_player(xmp_context c, int param)
#define qxmp_set_player xmp_set_player // int xmp_set_player(xmp_context c, int param, int val)

#define xmp_dll 1

qbool XMP_OpenLibrary (void) {return true;}
void XMP_CloseLibrary (void) {}
#else

/* libxmp ABI */
/*
=================================================================

  definitions from xmp.h

=================================================================
*/

// constants from libxmp
#define XMP_NAME_SIZE		64	/* Size of module name and type */

/* sample format flags */
#define XMP_FORMAT_8BIT		(1 << 0) /* Mix to 8-bit instead of 16 */
#define XMP_FORMAT_UNSIGNED	(1 << 1) /* Mix to unsigned samples */
#define XMP_FORMAT_MONO		(1 << 2) /* Mix to mono instead of stereo */

/* player parameters */
#define XMP_PLAYER_AMP		0	/* Amplification factor */
#define XMP_PLAYER_MIX		1	/* Stereo mixing */
#define XMP_PLAYER_INTERP	2	/* Interpolation type */
#define XMP_PLAYER_DSP		3	/* DSP effect flags */
#define XMP_PLAYER_FLAGS	4	/* Player flags */
#define XMP_PLAYER_CFLAGS	5	/* Player flags for current module */
#define XMP_PLAYER_SMPCTL	6	/* Sample control flags */
#define XMP_PLAYER_VOLUME	7	/* Player module volume */
#define XMP_PLAYER_STATE	8	/* Internal player state */
#define XMP_PLAYER_SMIX_VOLUME	9	/* SMIX volume */
#define XMP_PLAYER_DEFPAN	10	/* Default pan setting */

/* interpolation types */
#define XMP_INTERP_NEAREST	0	/* Nearest neighbor */
#define XMP_INTERP_LINEAR	1	/* Linear (default) */
#define XMP_INTERP_SPLINE	2	/* Cubic spline */

/* player state */
#define XMP_STATE_UNLOADED	0	/* Context created */
#define XMP_STATE_LOADED	1	/* Module loaded */
#define XMP_STATE_PLAYING	2	/* Module playing */

/* sample flags */
#define XMP_SMPCTL_SKIP		(1 << 0) /* Don't load samples */

/* limits */
//#define XMP_MAX_KEYS		121	/* Number of valid keys */
//#define XMP_MAX_ENV_POINTS	32	/* Max number of envelope points */
#define XMP_MAX_MOD_LENGTH	256	/* Max number of patterns in module */
//#define XMP_MAX_CHANNELS	64	/* Max number of channels in module */
#define XMP_MAX_SRATE		49170	/* max sampling rate (Hz) */
#define XMP_MIN_SRATE		4000	/* min sampling rate (Hz) */
//#define XMP_MIN_BPM		20	/* min BPM */
#define XMP_MAX_FRAMESIZE	(5 * XMP_MAX_SRATE * 2 / XMP_MIN_BPM)

/* error codes */
#define XMP_END			1
#define XMP_ERROR_INTERNAL	2	/* Internal error */
#define XMP_ERROR_FORMAT	3	/* Unsupported module format */
#define XMP_ERROR_LOAD		4	/* Error loading file */
#define XMP_ERROR_DEPACK	5	/* Error depacking file */
#define XMP_ERROR_SYSTEM	6	/* System error */
#define XMP_ERROR_INVALID	7	/* Invalid parameter */
#define XMP_ERROR_STATE		8	/* Invalid player state */

// types from libxmp
typedef char *xmp_context;

static const char **qxmp_version;
static const unsigned int *qxmp_vercode;

struct xmp_channel {
	int pan;			/* Channel pan (0x80 is center) */
	int vol;			/* Channel volume */
#define XMP_CHANNEL_SYNTH	(1 << 0)  /* Channel is synthesized */
#define XMP_CHANNEL_MUTE  	(1 << 1)  /* Channel is muted */
	int flg;			/* Channel flags */
};

//struct xmp_sequence {
//	int entry_point;
//	int duration;
//};

struct xmp_module {
	char name[XMP_NAME_SIZE];	/* Module title */
	char type[XMP_NAME_SIZE];	/* Module format */
	int pat;			/* Number of patterns */
	int trk;			/* Number of tracks */
	int chn;			/* Tracks per pattern */
	int ins;			/* Number of instruments */
	int smp;			/* Number of samples */
	int spd;			/* Initial speed */
	int bpm;			/* Initial BPM */
	int len;			/* Module length in patterns */
	int rst;			/* Restart position */
	int gvl;			/* Global volume */

	struct xmp_pattern **xxp;	/* Patterns */
	struct xmp_track **xxt;		/* Tracks */
	struct xmp_instrument *xxi;	/* Instruments */
	struct xmp_sample *xxs;		/* Samples */
	struct xmp_channel xxc[64];	/* Channel info */
	unsigned char xxo[XMP_MAX_MOD_LENGTH];	/* Orders */
};

//struct xmp_test_info {
//	char name[XMP_NAME_SIZE];	/* Module title */
//	char type[XMP_NAME_SIZE];	/* Module format */
//};

struct xmp_module_info {
	unsigned char md5[16];		/* MD5 message digest */
	int vol_base;			/* Volume scale */
	struct xmp_module *mod;		/* Pointer to module data */
	char *comment;			/* Comment text, if any */
	int num_sequences;		/* Number of valid sequences */
	struct xmp_sequence *seq_data;	/* Pointer to sequence data */
};

struct xmp_frame_info
// {			/* Current frame information */
//	int pos;			/* Current position */
//	int pattern;			/* Current pattern */
//	int row;			/* Current row in pattern */
//	int num_rows;			/* Number of rows in current pattern */
//	int frame;			/* Current frame */
//	int speed;			/* Current replay speed */
//	int bpm;			/* Current bpm */
//	int time;			/* Current module time in ms */
//	int total_time;			/* Estimated replay time in ms*/
//	int frame_time;			/* Frame replay time in us */
//	void *buffer;			/* Pointer to sound buffer */
//	int buffer_size;		/* Used buffer size */
//	int total_size;			/* Total buffer size */
//	int volume;			/* Current master volume */
//	int loop_count;			/* Loop counter */
//	int virt_channels;		/* Number of virtual channels */
//	int virt_used;			/* Used virtual channels */
//	int sequence;			/* Current sequence */
//
//	struct xmp_channel_info {	/* Current channel information */
//		unsigned int period;	/* Sample period */
//		unsigned int position;	/* Sample position */
//		short pitchbend;	/* Linear bend from base note*/
//		unsigned char note;	/* Current base note number */
//		unsigned char instrument; /* Current instrument number */
//		unsigned char sample;	/* Current sample number */
//		unsigned char volume;	/* Current volume */
//		unsigned char pan;	/* Current stereo pan */
//		unsigned char reserved;	/* Reserved */
//		struct xmp_event event;	/* Current track event */
//	} channel_info[XMP_MAX_CHANNELS];
//}
;

// Functions exported from libxmp
static xmp_context (*qxmp_create_context)  (void);
static void        (*qxmp_free_context)    (xmp_context);
//static int         (*qxmp_test_module)     (char *, struct xmp_test_info *);
//static int         (*qxmp_load_module)     (xmp_context, char *);
//static void        (*qxmp_scan_module)     (xmp_context);
static void        (*qxmp_release_module)  (xmp_context);
static int         (*qxmp_start_player)    (xmp_context, int, int);
static int         (*qxmp_play_frame)      (xmp_context);
static int         (*qxmp_play_buffer)     (xmp_context, void *, int, int);
static void        (*qxmp_get_frame_info)  (xmp_context, struct xmp_frame_info *);
static void        (*qxmp_end_player)      (xmp_context);
//static void        (*qxmp_inject_event)    (xmp_context, int, struct xmp_event *);
static void        (*qxmp_get_module_info) (xmp_context, struct xmp_module_info *);
//static char      **(*qxmp_get_format_list) (void); // FIXME: did I do this right?
//static int         (*qxmp_next_position)   (xmp_context);
//static int         (*qxmp_prev_position)   (xmp_context);
//static int         (*qxmp_set_position)    (xmp_context, int);
//static void        (*qxmp_stop_module)     (xmp_context);
//static void        (*qxmp_restart_module)  (xmp_context);
//static int         (*qxmp_seek_time)       (xmp_context, int);
//static int         (*qxmp_channel_mute)    (xmp_context, int, int);
//static int         (*qxmp_channel_vol)     (xmp_context, int, int);
static int         (*qxmp_set_player)      (xmp_context, int, int);
static int         (*qxmp_get_player)      (xmp_context, int);
//static int         (*qxmp_set_instrument_path) (xmp_context, char *);
static int         (*qxmp_load_module_from_memory) (xmp_context, void *, long);
//static int         (*qxmp_load_module_from_file) (xmp_context, void *, long);
//static int        (XMP_EXPORT *qxmp_load_module_from_file) (xmp_context, void *, long);

/* External sample mixer API */
/*
static int         (*qxmp_start_smix)       (xmp_context, int, int);
static void        (*qxmp_end_smix)         (xmp_context);
static int         (*qxmp_smix_play_instrument)(xmp_context, int, int, int, int);
static int         (*qxmp_smix_play_sample) (xmp_context, int, int, int, int);
static int         (*qxmp_smix_channel_pan) (xmp_context, int, int);
static int         (*qxmp_smix_load_sample) (xmp_context, int, char *);
static int         (*qxmp_smix_release_sample) (xmp_context, int);
// end Functions exported from libxmp
*/

/*
=================================================================

  DarkPlaces definitions

=================================================================
*/

static dllfunction_t xmpfuncs[] =
{
	/* libxmp ABI */
	// Version and player information
	{"xmp_version",                 (void **) &qxmp_version},
	{"xmp_vercode",                 (void **) &qxmp_vercode},
//	{"xmp_get_format_list",         (void **) &qxmp_get_format_list},
	// Context creation
	{"xmp_create_context",          (void **) &qxmp_create_context},
	{"xmp_free_context",            (void **) &qxmp_free_context},
	// Module loading
//	{"xmp_test_module",             (void **) &qxmp_test_module},
//	{"xmp_load_module",             (void **) &qxmp_load_module},
	{"xmp_load_module_from_memory", (void **) &qxmp_load_module_from_memory}, // since libxmp 4.2.0
//	{"xmp_load_module_from_file",   (void **) &qxmp_load_module_from_file},   // since libxmp 4.3.0
	{"xmp_release_module",          (void **) &qxmp_release_module},
//	{"xmp_scan_module",             (void **) &qxmp_scan_module},
	{"xmp_get_module_info",         (void **) &qxmp_get_module_info},
	// Module playing
	{"xmp_start_player",            (void **) &qxmp_start_player},
	{"xmp_play_frame",              (void **) &qxmp_play_frame},
	{"xmp_play_buffer",             (void **) &qxmp_play_buffer},
	{"xmp_get_frame_info",          (void **) &qxmp_get_frame_info},
	{"xmp_end_player",              (void **) &qxmp_end_player},
	// Player control
//	{"xmp_next_position",           (void **) &qxmp_next_position},
//	{"xmp_prev_position",           (void **) &qxmp_prev_position},
//	{"xmp_set_position",            (void **) &qxmp_set_position},
//	{"xmp_stop_module",             (void **) &qxmp_stop_module},
//	{"xmp_restart_module",          (void **) &qxmp_restart_module},
//	{"xmp_seek_time",               (void **) &qxmp_seek_time},
//	{"xmp_channel_mute",            (void **) &qxmp_channel_mute},
//	{"xmp_channel_vol",             (void **) &qxmp_channel_vol},
//	{"xmp_inject_event",            (void **) &qxmp_inject_event},
	// Player parameter setting
//	{"xmp_set_instrument_path",     (void **) &qxmp_set_instrument_path},
	{"xmp_get_player",              (void **) &qxmp_get_player},
	{"xmp_set_player",              (void **) &qxmp_set_player},
	/* smix */ // for completeness sake only, right now
//	{"xmp_start_smix",              (void **) &qxmp_start_smix},
//	{"xmp_end_smix",                (void **) &qxmp_end_smix},
//	{"xmp_smix_play_instrument",    (void **) &qxmp_smix_play_instrument},
//	{"xmp_smix_play_sample",        (void **) &qxmp_smix_play_sample},
//	{"xmp_smix_channel_pan",        (void **) &qxmp_smix_channel_pan},
//	{"xmp_smix_load_sample",        (void **) &qxmp_smix_load_sample},
//	{"xmp_smix_release_sample",     (void **) &qxmp_smix_release_sample},
	{NULL, NULL}
};

// libxmp DLL handle
static dllhandle_t xmp_dll = NULL;


/*
=================================================================

  DLL load & unload

=================================================================
*/

/*
====================
XMP_OpenLibrary

Try to load the libxmp DLL
====================
*/
qbool XMP_OpenLibrary (void)
{
	const char* dllnames_xmp [] =
	{
#if defined(WIN32)
		"libxmp-4.dll",
		"libxmp.dll",
#elif defined(MACOSX) // FIXME: untested, please test a mac os build
		"libxmp.4.dylib",
		"libxmp.dylib",
#else
		"libxmp.so.4",
		"libxmp.so",
#endif
		NULL
	};

	if (xmp_dll) // Already loaded?
		return true;

// COMMANDLINEOPTION: Sound: -noxmp disables xmp module sound support
	if (Sys_CheckParm("-noxmp"))
		return false;

	// Load the DLL
	if (Sys_LoadDependency (dllnames_xmp, &xmp_dll, xmpfuncs))
	{
		if (*qxmp_vercode < 0x040200)
		{
			Con_Printf("Found incompatible XMP library version %s, not loading. (4.2.0 or higher required)\n", *qxmp_version);
			Sys_FreeLibrary (&xmp_dll);
			return false;
		}
		if (developer_loading.integer >= 1)
			Con_Printf("XMP library loaded, version %s (0x0%x)\n", *qxmp_version, *qxmp_vercode);
		return true;
	}
	else
		return false;
}


/*
====================
XMP_CloseLibrary

Unload the libxmp DLL
====================
*/
void XMP_CloseLibrary (void)
{
	Sys_FreeLibrary (&xmp_dll);
}

#endif

/*
=================================================================

	Module file decoding

=================================================================
*/

// Per-sfx data structure
typedef struct
{
	unsigned char	*file;
	size_t		filesize;
} xmp_stream_persfx_t;

// Per-channel data structure
typedef struct
{
	xmp_context	playercontext;
	int		bs;
	int		buffer_firstframe;
	int		buffer_numframes;
	unsigned char	buffer[STREAM_BUFFERSIZE*4];
} xmp_stream_perchannel_t;


/*
====================
XMP_GetSamplesFloat
====================
*/
static void XMP_GetSamplesFloat(channel_t *ch, sfx_t *sfx, int firstsampleframe, int numsampleframes, float *outsamplesfloat)
{
	int i, len = numsampleframes * sfx->format.channels;
	int f = sfx->format.width * sfx->format.channels; // bytes per frame in the buffer
	xmp_stream_perchannel_t* per_ch = (xmp_stream_perchannel_t *)ch->fetcher_data;
	xmp_stream_persfx_t* per_sfx = (xmp_stream_persfx_t *)sfx->fetcher_data;
	const short *buf;
	int newlength, done;
	unsigned int format = 0;

	// if this channel does not yet have a channel fetcher, make one
	if (per_ch == NULL)
	{
		// allocate a struct to keep track of our file position and buffer
		per_ch = (xmp_stream_perchannel_t *)Mem_Alloc(snd_mempool, sizeof(*per_ch));

		// create an xmp file context
		if ((per_ch->playercontext = qxmp_create_context()) == NULL)
		{
			//Con_Printf("error getting a libxmp file context; while trying to load file \"%s\"\n", filename);
			Mem_Free(per_ch);
			return;
		}
		// copy file to xmp
		if (qxmp_load_module_from_memory(per_ch->playercontext, (void *)per_sfx->file, (long)per_sfx->filesize) < 0)
		{
			qxmp_free_context(per_ch->playercontext);
			Mem_Free(per_ch);
			return;
		}

		// start playing the loaded file
		if (sfx->format.width == 1)    { format |= XMP_FORMAT_8BIT | XMP_FORMAT_UNSIGNED; } // else 16bit
		if (sfx->format.channels == 1) { format |= XMP_FORMAT_MONO; } // else stereo

		if (qxmp_start_player(per_ch->playercontext, sfx->format.speed, format) < 0)
		{
			Mem_Free(per_ch);
			return;
		}
		/* percentual left/right channel separation, default is 70. */
		if (sfx->format.channels == 2 && (qxmp_set_player(per_ch->playercontext, XMP_PLAYER_MIX, 50) != 0))
		{
			Mem_Free(per_ch);
			return;
		}
		/* interpolation type, default is XMP_INTERP_LINEAR */
		if (qxmp_set_player(per_ch->playercontext, XMP_PLAYER_INTERP, XMP_INTERP_SPLINE) != 0)
		{
			Mem_Free(per_ch);
			return;
		}

		per_ch->bs = 0;
		per_ch->buffer_firstframe = 0;
		per_ch->buffer_numframes = 0;
		// attach the struct to our channel
		ch->fetcher_data = (void *)per_ch;

		// reset internal xmp state / syncs buffer start with frame start
		qxmp_play_buffer(per_ch->playercontext, NULL, 0, 0);
	}

	// if the request is too large for our buffer, loop...
	while (numsampleframes * f > (int)sizeof(per_ch->buffer))
	{
		done = sizeof(per_ch->buffer) / f;
		XMP_GetSamplesFloat(ch, sfx, firstsampleframe, done, outsamplesfloat);
		firstsampleframe += done;
		numsampleframes -= done;
		outsamplesfloat += done * sfx->format.channels;
	}

	// seek if the request is before the current buffer (loop back)
	// seek if the request starts beyond the current buffer by at least one frame (channel was zero volume for a while)
	// do not seek if the request overlaps the buffer end at all (expected behavior)
	if (per_ch->buffer_firstframe > firstsampleframe || per_ch->buffer_firstframe + per_ch->buffer_numframes < firstsampleframe)
	{
		// we expect to decode forward from here so this will be our new buffer start
		per_ch->buffer_firstframe = firstsampleframe;
		per_ch->buffer_numframes = 0;
		// no seeking at this time
	}

	// render the file to pcm as needed
	if (firstsampleframe + numsampleframes > per_ch->buffer_firstframe + per_ch->buffer_numframes)
	{
		// first slide the buffer back, discarding any data preceding the range we care about
		int offset = firstsampleframe - per_ch->buffer_firstframe;
		int keeplength = per_ch->buffer_numframes - offset;
		if (keeplength > 0)
			memmove(per_ch->buffer, per_ch->buffer + offset * sfx->format.width * sfx->format.channels, keeplength * sfx->format.width * sfx->format.channels);
		per_ch->buffer_firstframe = firstsampleframe;
		per_ch->buffer_numframes -= offset;
		// render as much as we can fit in the buffer
		newlength = sizeof(per_ch->buffer) - per_ch->buffer_numframes * f;
		done = 0;
//		while (newlength > done && qxmp_play_buffer(per_ch->playercontext, (void *)((char *)per_ch->buffer + done), (int)(newlength - done), 1) == 0) // don't loop by default (TODO: fix pcm duration calculation first)
		while (newlength > done && qxmp_play_buffer(per_ch->playercontext, (void *)((char *)per_ch->buffer + done), (int)(newlength - done), 0) == 0) // loop forever
		{
			done += (int)(newlength - done);
		}
		// clear the missing space if any
		if (done < newlength)
		{
			memset(per_ch->buffer + done, 0, newlength - done);
		}
		// we now have more data in the buffer
		per_ch->buffer_numframes += done / f;
	}

	// convert the sample format for the caller
	buf = (short *)((char *)per_ch->buffer + (firstsampleframe - per_ch->buffer_firstframe) * f);
	for (i = 0;i < len;i++)
		outsamplesfloat[i] = buf[i] * (1.0f / 32768.0f);
}

/*
====================
XMP_StopChannel
====================
*/
static void XMP_StopChannel(channel_t *ch)
{
	xmp_stream_perchannel_t *per_ch = (xmp_stream_perchannel_t *)ch->fetcher_data;
	if (per_ch != NULL)
	{
		// stop the player
		qxmp_end_player(per_ch->playercontext);
		// free the module
		qxmp_release_module(per_ch->playercontext);
		// free the xmp playercontext
		qxmp_free_context(per_ch->playercontext);
		Mem_Free(per_ch);
	}
}

/*
====================
XMP_FreeSfx
====================
*/
static void XMP_FreeSfx(sfx_t *sfx)
{
	xmp_stream_persfx_t* per_sfx = (xmp_stream_persfx_t *)sfx->fetcher_data;
	// free the complete file we were keeping around
	Mem_Free(per_sfx->file);
	// free the file information structure
	Mem_Free(per_sfx);
}

static const snd_fetcher_t xmp_fetcher = { XMP_GetSamplesFloat, XMP_StopChannel, XMP_FreeSfx };

/*
===============
XMP_LoadModFile

Load an XMP module file into memory
===============
*/
qbool XMP_LoadModFile(const char *filename, sfx_t *sfx)
{
	fs_offset_t filesize;
	unsigned char *data;
	xmp_context xc;
	xmp_stream_persfx_t* per_sfx;
	struct xmp_module_info mi;

#ifndef LINK_TO_LIBXMP
	if (!xmp_dll)
		return false;
#endif

// COMMANDLINEOPTION: Sound: -noxmp disables xmp module sound support
	if (Sys_CheckParm("-noxmp"))
		return false;

	// Return if already loaded
	if (sfx->fetcher != NULL)
		return true;

	// Load the file
	data = FS_LoadFile(filename, snd_mempool, false, &filesize);
	if (data == NULL)
		return false;

	// Create an xmp file context
	if ((xc = qxmp_create_context()) == NULL)
	{
		Con_Printf("error creating a libxmp file context; while trying to load file \"%s\"\n", filename);
		Mem_Free(data);
		return false;
	}

	if (developer_loading.integer >= 2)
		Con_Printf("Loading Module file (libxmp) \"%s\"\n", filename);

	if (qxmp_load_module_from_memory(xc, (void *)data, (long)filesize) < 0) // Added in libxmp 4.2
	{
		Con_Printf("error while trying to load xmp module \"%s\"\n", filename);
		qxmp_free_context(xc);
		Mem_Free(data);
		return false;
	}

	if (developer_loading.integer >= 2)
		Con_Printf ("\"%s\" will be streamed\n", filename);

	// keep the file around
	per_sfx = (xmp_stream_persfx_t *)Mem_Alloc (snd_mempool, sizeof (*per_sfx));
	per_sfx->file = data;
	per_sfx->filesize = filesize;
	// set dp sfx
	sfx->memsize += sizeof(*per_sfx);
	sfx->memsize += filesize; // total memory used (including sfx_t and fetcher data)
	if (S_GetSoundRate() > XMP_MAX_SRATE)
		sfx->format.speed = 48000;
	else if (S_GetSoundRate() < XMP_MIN_SRATE)
		sfx->format.speed = 8000;
	else
		sfx->format.speed = S_GetSoundRate();
	sfx->format.width = S_GetSoundWidth();  // 2 = 16 bit samples
	sfx->format.channels = S_GetSoundChannels();
	sfx->flags |= SFXFLAG_STREAMED; // cf SFXFLAG_* defines
	sfx->total_length = 1<<30; // 2147384647; // in (pcm) sample frames - they always loop (FIXME this breaks after 6 hours, we need support for a real "infinite" value!)
	sfx->loopstart = sfx->total_length; // (modplug does it) in sample frames. equals total_length if not looped
	sfx->fetcher_data = per_sfx;
	sfx->fetcher = &xmp_fetcher;
	sfx->volume_peak = 0;

	qxmp_get_module_info(xc, &mi);
	if (developer_loading.integer >= 2)
	{
		Con_Printf("Decoding module (libxmp):\n"
			"    Module name  : %s\n"
			"    Module type  : %s\n"
			"    Module length: %i patterns\n"
			"    Patterns     : %i\n"
			"    Instruments  : %i\n"
			"    Samples      : %i\n"
			"    Channels     : %i\n"
			"    Initial Speed: %i\n"
			"    Initial BPM  : %i\n"
			"    Restart Pos. : %i\n"
			"    Global Volume: %i\n",
			mi.mod->name, mi.mod->type,
			mi.mod->len, mi.mod->pat, mi.mod->ins, mi.mod->smp, mi.mod->chn,
			mi.mod->spd, mi.mod->bpm, mi.mod->rst, mi.mod->gvl
		);
	}
	else if (developer_loading.integer == 1)
		Con_Printf("Decoding module (libxmp) \"%s\" (%s)\n", mi.mod->name, mi.mod->type);

	qxmp_free_context(xc);
	return true;
}
