/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef SND_MAIN_H
#define SND_MAIN_H

#include <stddef.h>
#include "qtypes.h"
#include "qdefs.h"
struct mempool_s;
struct sfx_s;

typedef struct snd_format_s
{
	unsigned int	speed;
	unsigned short	width;
	unsigned short	channels;
} snd_format_t;

typedef struct snd_buffer_s
{
	snd_format_t		format;
	unsigned int		nbframes;	// current size, in sample frames
	unsigned int		maxframes;	// max size (buffer size), in sample frames
	unsigned char		samples[4];	// variable sized
} snd_buffer_t;

typedef struct snd_ringbuffer_s
{
	snd_format_t		format;
	unsigned char*		ring;
	unsigned int		maxframes;	// max size (buffer size), in sample frames
	unsigned int		startframe;	// index of the first frame in the buffer
									// if startframe == endframe, the bufffer is empty
	unsigned int		endframe;	// index of the first EMPTY frame in the "ring" buffer
									// may be smaller than startframe if the "ring" buffer has wrapped
} snd_ringbuffer_t;

// struct sfx_s flags
#define SFXFLAG_NONE			0
#define SFXFLAG_FILEMISSING		(1 << 0) // wasn't able to load the associated sound file
#define SFXFLAG_LEVELSOUND		(1 << 1) // the sfx is part of the server or client precache list for this level
#define SFXFLAG_STREAMED		(1 << 2) // informative only. You shouldn't need to know that
#define SFXFLAG_MENUSOUND		(1 << 3) // not freed during level change (menu sounds, music, etc)

typedef struct snd_fetcher_s snd_fetcher_t;
struct sfx_s
{
	char				name[MAX_QPATH];
	struct sfx_s				*next;
	size_t				memsize;		// total memory used (including struct sfx_s and fetcher data)

	snd_format_t		format;			// format describing the audio data that fetcher->getsamplesfloat will return
	unsigned int		flags;			// cf SFXFLAG_* defines
	unsigned int		loopstart;		// in sample frames. equals total_length if not looped
	unsigned int		total_length;	// in sample frames
	const snd_fetcher_t	*fetcher;
	void				*fetcher_data;	// Per-sfx data for the sound fetching functions

	float				volume_mult;    // for replay gain (multiplier to apply)
	float				volume_peak;    // for replay gain (highest peak); if set to 0, ReplayGain isn't supported
};

// maximum supported speakers constant
#define SND_LISTENERS 8

typedef struct channel_s
{
	// provided sound information
	struct sfx_s			*sfx;			// pointer to sound sample being used
	float			basevolume;		// 0-1 master volume
	unsigned int	flags;			// cf CHANNELFLAG_* defines
	int				entnum;			// makes sound follow entity origin (allows replacing interrupting existing sound on same id)
	int				entchannel;		// which channel id on the entity
	vec3_t			origin;			// origin of sound effect
	vec_t			distfade;		// distance multiplier (attenuation/clipK)
	void			*fetcher_data;	// Per-channel data for the sound fetching function
	int				prologic_invert;// whether a sound is played on the surround channels in prologic
	float			basespeed;		// playback rate multiplier for pitch variation

	// these are often updated while mixer is running, glitching should be minimized (mismatched channel volumes from spatialization is okay)
	// spatialized playback speed (speed * doppler ratio)
	float			mixspeed;
	// spatialized volume per speaker (mastervol * distanceattenuation * channelvolume cvars)
	float			volume[SND_LISTENERS];

	// updated ONLY by mixer
	// position in sfx, starts at 0, loops or stops at sfx->total_length
	double			position;
} channel_t;

// Sound fetching functions
// "start" is both an input and output parameter: it returns the actual start time of the sound buffer
typedef void (*snd_fetcher_getsamplesfloat_t) (channel_t *ch, struct sfx_s *sfx, int firstsampleframe, int numsampleframes, float *outsamplesfloat);
typedef void (*snd_fetcher_stopchannel_t) (channel_t *ch);
typedef void (*snd_fetcher_freesfx_t) (struct sfx_s *sfx);
struct snd_fetcher_s
{
	snd_fetcher_getsamplesfloat_t		getsamplesfloat;
	snd_fetcher_stopchannel_t		stopchannel;
	snd_fetcher_freesfx_t		freesfx;
};

extern unsigned int total_channels;
extern channel_t channels[MAX_CHANNELS];

extern snd_ringbuffer_t *snd_renderbuffer;
extern qbool snd_threaded; // enables use of snd_usethreadedmixing, provided that no sound hacks are in effect (like timedemo)
extern qbool snd_usethreadedmixing; // if true, the main thread does not mix sound, soundtime does not advance, and neither does snd_renderbuffer->endframe, instead the audio thread will call S_MixToBuffer as needed

extern struct cvar_s snd_waterfx;
extern struct cvar_s _snd_mixahead;
extern struct cvar_s snd_swapstereo;
extern struct cvar_s snd_streaming;
extern struct cvar_s snd_streaming_length;

#define SND_CHANNELLAYOUT_AUTO		0
#define SND_CHANNELLAYOUT_STANDARD	1
#define SND_CHANNELLAYOUT_ALSA		2
extern struct cvar_s snd_channellayout;

extern int snd_blocked;		// counter. When > 0, we stop submitting sound to the audio device

extern struct mempool_s *snd_mempool;

// If simsound is true, the sound card is not initialized and no sound is submitted to it.
// More generally, all arch-dependent operations are skipped or emulated.
// Used for isolating performance in the renderer.
extern qbool simsound;


#define STREAM_BUFFERSIZE 16384 // in sampleframes


// ====================================================================
//         Architecture-independent functions
// ====================================================================

void S_SetUnderwaterIntensity(void);

void S_MixToBuffer(void *stream, unsigned int frames);

qbool S_LoadSound (struct sfx_s *sfx, qbool complain);

// If "buffer" is NULL, the function allocates one buffer of "sampleframes" sample frames itself
// (if "sampleframes" is 0, the function chooses the size).
snd_ringbuffer_t *Snd_CreateRingBuffer (const snd_format_t* format, unsigned int sampleframes, void* buffer);


// ====================================================================
//         Architecture-dependent functions
// ====================================================================

// Create "snd_renderbuffer", attempting to use the chosen sound format, but accepting if the driver wants to change it (e.g. 7.1 to stereo or lowering the speed)
// Note: SDL automatically converts all formats, so this only fails if there is no audio
qbool SndSys_Init (snd_format_t* fmt);

// Stop the sound card, delete "snd_renderbuffer" and free its other resources
void SndSys_Shutdown (void);

// Submit the contents of "snd_renderbuffer" to the sound card
void SndSys_Submit (void);

// Returns the number of sample frames consumed since the sound started
unsigned int SndSys_GetSoundTime (void);

// Get the exclusive lock on "snd_renderbuffer"
qbool SndSys_LockRenderBuffer (void);

// Release the exclusive lock on "snd_renderbuffer"
void SndSys_UnlockRenderBuffer (void);

// if the sound system can generate events, send them
void SndSys_SendKeyEvents(void);

// exported for capturevideo so ogg can see all channels
typedef struct portable_samplepair_s
{
	float sample[SND_LISTENERS];
} portable_sampleframe_t;

typedef struct listener_s
{
	int channel_unswapped; // for un-swapping
	float yawangle;
	float dotscale;
	float dotbias;
	float ambientvolume;
}
listener_t;
typedef struct speakerlayout_s
{
	const char *name;
	unsigned int channels;
	listener_t listeners[SND_LISTENERS];
}
speakerlayout_t;

#endif
