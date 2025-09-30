/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * =======================================================================
 *
 * The header file for the OGG/Vorbis playback
 *
 * =======================================================================
 */

#ifndef CL_SOUND_VORBIS_H
#define CL_SOUND_VORBIS_H

#include "local.h"

typedef enum
{
	PLAY,
	PAUSE,
	STOP
} ogg_status_t;

int OGG_Status(void);
void OGG_InitTrackList(void);
void OGG_Init(void);
void OGG_PlayTrack(const char* track, qboolean cdtrack, qboolean immediate);
void OGG_RecoverState(void);
void OGG_SaveState(void);
void OGG_Shutdown(void);
void OGG_Stop(void);
void OGG_Stream(void);
void OGG_LoadAsWav(char *filename, wavinfo_t *info, void **buffer);

#endif
