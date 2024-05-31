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
// snd_null.c -- include this instead of all the other snd_* files to have
// no sound code whatsoever

#include "quakedef.h"

cvar_t bgmvolume = {CF_ARCHIVE, "bgmvolume", "1", "volume of background music (such as CD music or replacement files such as sound/cdtracks/track002.ogg)"};
cvar_t mastervolume = {CF_ARCHIVE, "mastervolume", "1", "master volume"};
cvar_t volume = {CF_ARCHIVE, "volume", "0.7", "volume of sound effects"};
cvar_t snd_staticvolume = {CF_ARCHIVE, "snd_staticvolume", "1", "volume of ambient sound effects (such as swampy sounds at the start of e1m2)"};
cvar_t snd_initialized = { CF_READONLY, "snd_initialized", "0", "indicates the sound subsystem is active"};
cvar_t snd_mutewhenidle = {CF_ARCHIVE, "snd_mutewhenidle", "1", "whether to disable sound output when game window is inactive"};

void S_Init (void)
{
	Cvar_RegisterVariable(&bgmvolume);
	Cvar_RegisterVariable(&mastervolume);
	Cvar_RegisterVariable(&volume);
	Cvar_RegisterVariable(&snd_staticvolume);
	Cvar_RegisterVariable(&snd_initialized);
	Cvar_RegisterVariable(&snd_mutewhenidle);
}

void S_Terminate (void)
{
}

void S_Startup (void)
{
}

void S_Shutdown (void)
{
}

void S_ClearUsed (void)
{
}

void S_PurgeUnused (void)
{
}


void S_StaticSound (sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
}

int S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
	return -1;
}

int S_StartSound_StartPosition_Flags (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation, float startposition, int flags, float fspeed)
{
	return -1;
}

void S_StopChannel (unsigned int channel_ind, qbool lockmutex, qbool freesfx)
{
}

qbool S_SetChannelFlag (unsigned int ch_ind, unsigned int flag, qbool value)
{
	return false;
}

void S_StopSound (int entnum, int entchannel)
{
}

void S_PauseGameSounds (qbool toggle)
{
}

void S_SetChannelVolume (unsigned int ch_ind, float fvol)
{
}

sfx_t *S_PrecacheSound (const char *sample, qbool complain, qbool levelsound)
{
	return NULL;
}

float S_SoundLength(const char *name)
{
	return -1;
}

qbool S_IsSoundPrecached (const sfx_t *sfx)
{
	return false;
}

void S_UnloadAllSounds_f (cmd_state_t *cmd)
{
}

sfx_t *S_FindName (const char *name)
{
	return NULL;
}

void S_Update(const matrix4x4_t *matrix)
{
}

void S_StopAllSounds (void)
{
}

void S_ExtraUpdate (void)
{
}

qbool S_LocalSound (const char *s)
{
	return false;
}

qbool S_LocalSoundEx (const char *s, int chan, float fvol)
{
	return false;
}

void S_BlockSound (void)
{
}

void S_UnblockSound (void)
{
}

int S_GetSoundRate(void)
{
	return 0;
}

int S_GetSoundChannels(void)
{
	return 0;
}

float S_GetChannelPosition (unsigned int ch_ind)
{
	return -1;
}

float S_GetEntChannelPosition(int entnum, int entchannel)
{
	return -1;
}

void SndSys_SendKeyEvents(void)
{
}
