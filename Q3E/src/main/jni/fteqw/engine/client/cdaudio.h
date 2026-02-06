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

#ifdef HAVE_CDPLAYER
void CDAudio_Init(void);
qboolean CDAudio_Startup(void);	//called when the cd isn't currently valid. returns if its valid or not.
int CDAudio_GetAudioDiskInfo(void);//returns number of tracks available, or 0 if the cd is not valid.
void CDAudio_Play(int track);
void CDAudio_Stop(void);
void CDAudio_Pause(void);
void CDAudio_Resume(void);
void CDAudio_Eject(void);
void CDAudio_CloseDoor(void);
void CDAudio_Shutdown(void);
void CDAudio_Update(void);
#else
#define CDAudio_Update()
#define CDAudio_Init()
#define CDAudio_Shutdown()
#define CDAudio_Pause()
#define CDAudio_Resume()
#endif

