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

#ifndef SOUND_H
#define SOUND_H

#include "matrixlib.h"
struct cmd_state_s;


// ====================================================================
// Constants
// ====================================================================

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

// Channel flags
// These channel flags can be used for sound() builtins, with SOUNDFLAG_* names
#define CHANNELFLAG_NONE	0
#define CHANNELFLAG_RELIABLE	(1 << 0) // send as reliable message (only used on server)
#define CHANNELFLAG_FORCELOOP	(1 << 1) // force looping even if the sound is not looped
#define CHANNELFLAG_LOCALSOUND	(1 << 2) // INTERNAL USE. Not settable by S_SetChannelFlag
#define CHANNELFLAG_PAUSED	(1 << 3) // pause status
#define CHANNELFLAG_FULLVOLUME	(1 << 4) // isn't affected by the general volume

// ====================================================================
// Types and variables
// ====================================================================

typedef struct sfx_s sfx_t;

extern struct cvar_s mastervolume;
extern struct cvar_s bgmvolume;
extern struct cvar_s volume;
extern struct cvar_s snd_initialized;
extern struct cvar_s snd_staticvolume;
extern struct cvar_s snd_mutewhenidle;


// ====================================================================
// Functions
// ====================================================================

void S_Init (void);
void S_Terminate (void);

void S_Startup (void);
void S_Shutdown (void);
void S_UnloadAllSounds_f(struct cmd_state_s *cmd);

void S_Update(const matrix4x4_t *listenermatrix);
void S_ExtraUpdate (void);

sfx_t *S_PrecacheSound (const char *sample, qbool complain, qbool levelsound);
float S_SoundLength(const char *name);
void S_ClearUsed (void);
void S_PurgeUnused (void);
qbool S_IsSoundPrecached (const sfx_t *sfx);
sfx_t *S_FindName(const char *name);

// these define the "engine" channel namespace
#define CHAN_MIN_AUTO       -128
#define CHAN_MAX_AUTO          0
#define CHAN_MIN_SINGLE        1
#define CHAN_MAX_SINGLE      127
#define IS_CHAN_AUTO(n)        ((n) >= CHAN_MIN_AUTO && (n) <= CHAN_MAX_AUTO)
#define IS_CHAN_SINGLE(n)      ((n) >= CHAN_MIN_SINGLE && (n) <= CHAN_MAX_SINGLE)
#define IS_CHAN(n)             (IS_CHAN_AUTO(n) || IS_CHAN_SINGLE(n))

// engine channel == network channel
#define CHAN_ENGINE2NET(c)     (c)
#define CHAN_NET2ENGINE(c)     (c)

// engine view of channel encodes the auto flag into the channel number (see CHAN_ constants below)
// user view uses the flags bitmask for it
#define CHAN_USER2ENGINE(c)    (c)
#define CHAN_ENGINE2USER(c)    (c)
#define CHAN_ENGINE2CVAR(c)    (abs(c))

// S_StartSound returns the channel index, or -1 if an error occurred
int S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation);
int S_StartSound_StartPosition_Flags (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation, float startposition, int flags, float fspeed);
qbool S_LocalSoundEx (const char *s, int chan, float fvol);
qbool S_LocalSound (const char *s);

void S_StaticSound (sfx_t *sfx, vec3_t origin, float fvol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_StopAllSounds (void);
void S_StopAllSounds_f(struct cmd_state_s *cmd);
void S_PauseGameSounds (qbool toggle);

void S_StopChannel (unsigned int channel_ind, qbool lockmutex, qbool freesfx);
qbool S_SetChannelFlag (unsigned int ch_ind, unsigned int flag, qbool value);
void S_SetChannelVolume (unsigned int ch_ind, float fvol);
void S_SetChannelSpeed (unsigned int ch_ind, float fspeed);
float S_GetChannelPosition (unsigned int ch_ind);
float S_GetEntChannelPosition(int entnum, int entchannel);

void S_BlockSound (void);
void S_UnblockSound (void);

int S_GetSoundRate (void);
int S_GetSoundChannels (void);
int S_GetSoundWidth (void);

#endif
