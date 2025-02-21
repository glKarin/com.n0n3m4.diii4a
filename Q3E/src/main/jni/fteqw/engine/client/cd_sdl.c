#include "quakedef.h"

#include <SDL.h>

#ifndef HAVE_CDPLAYER
	//nothing
#elif SDL_MAJOR_VERSION >= 2
//sdl2 has no cd support. sod off.
#include "cd_null.c"
#else

extern	cvar_t	bgmvolume;

static qboolean	initialized = false;

static SDL_CD	*cddevice;


void CDAudio_Eject(void)
{
	if (SDL_CDEject(cddevice))
		Con_DPrintf("SDL_CDEject failed\n");
}


void CDAudio_CloseDoor(void)
{
	Con_Printf("SDL does not support this\n");
}


int CDAudio_GetAudioDiskInfo(void)
{
	switch (SDL_CDStatus(cddevice))
	{
	case CD_ERROR:
		Con_Printf("SDL_CDStatus returned error\n");
		return -1;
	case CD_TRAYEMPTY:
		return 0;
	default:
		break;
	}
	return cddevice->numtracks;
}


void CDAudio_Play(int track)
{
	if (SDL_CDPlayTracks(cddevice, track, 0, 1, 0))
	{
		Con_Printf("CDAudio: track %i is not audio\n", track);
		return;
	}

	if (!bgmvolume.value)
		CDAudio_Pause ();

	return;
}


void CDAudio_Stop(void)
{
	if (SDL_CDStop(cddevice))
		Con_DPrintf("CDAudio: SDL_CDStop failed");
}


void CDAudio_Pause(void)
{
	if (SDL_CDPause(cddevice))
		Con_DPrintf("CDAudio: SDL_CDPause failed");
}


void CDAudio_Resume(void)
{
	if (SDL_CDResume(cddevice))
	{
		Con_DPrintf("CDAudio: SDL_CDResume failed\n");
		return;
	}
}

void CDAudio_Update(void)
{
}


void CDAudio_Init(void)
{
}

qboolean CDAudio_Startup(void)
{
	if (initialized)
		return !!cddevice;
	if (!bgmvolume.value)
		return false;
	initialized = true;

	SDL_InitSubSystem(SDL_INIT_CDROM|SDL_INIT_NOPARACHUTE);

	if(!SDL_CDNumDrives())
	{
		Con_DPrintf("CDAudio_Init: No CD drives\n");
		return false;
	}

	cddevice = SDL_CDOpen(0);
	if (!cddevice)
	{
		Con_Printf("CDAudio_Init: SDL_CDOpen failed\n");
		return false;
	}

	return true;
}

void CDAudio_Shutdown(void)
{
	if (!initialized)
		return;
	CDAudio_Stop();

	SDL_CDClose(cddevice);
	cddevice = NULL;
	initialized = false;
}
#endif
