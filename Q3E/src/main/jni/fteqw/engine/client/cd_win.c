/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "quakedef.h"
#include "winquake.h"

#ifndef HAVE_CDPLAYER
	//nothing
#elif defined(WINRT)
#include "cd_null.c"
#else

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#define DWORD_PTR DWORD
#endif

extern	HWND	mainwindow;
extern	cvar_t	bgmvolume;

static qboolean	initialized;
static qboolean	initializefailed;
static DWORD resumeend;
static qboolean	pollneeded;	//workaround for windows vista/7 bug, where notification simply does not work for end of tracks.

static UINT	wDeviceID;

int CDAudio_GetAudioDiskInfo(void)
{
	DWORD				dwReturn;
	static MCI_STATUS_PARMS	mciStatusParms;

	if (!CDAudio_Startup())
		return -1;

	if (!initialized)
		return -1;

	mciStatusParms.dwItem = MCI_STATUS_READY;
	mciStatusParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD_PTR) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		Con_DPrintf("CDAudio: drive ready test - get status failed\n");
		return -1;
	}
	if (!mciStatusParms.dwReturn)
	{
		Con_DPrintf("CDAudio: drive not ready\n");
		return -1;
	}

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	mciStatusParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD_PTR) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		Con_DPrintf("CDAudio: get tracks - status failed\n");
		return -1;
	}
	if (mciStatusParms.dwReturn < 1)
	{
		Con_DPrintf("CDAudio: no music tracks\n");
		return -1;
	}

	return mciStatusParms.dwReturn;
}
qboolean CDAudio_Startup(void)
{
	DWORD	dwReturn;
	static MCI_OPEN_PARMSA	mciOpenParms;
	static MCI_SET_PARMS	mciSetParms;

	if (initializefailed)
		return false;

	if (initialized)
		return true;

	mciOpenParms.lpstrDeviceType = "cdaudio";
	mciOpenParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD_PTR) (LPVOID) &mciOpenParms);
	if (dwReturn)
	{
		Con_Printf("CDAudio_Init: MCI_OPEN failed (%i)\n", (int)dwReturn);
		initializefailed = true;
		return 0;
	}
	wDeviceID = mciOpenParms.wDeviceID;

	// Set the time format to frames. vista+ simply cannot come with converting to/from seconds, or something (notifies don't work, status stays playing, position stops updating at about 3 frames from the end of the track).
	mciSetParms.dwTimeFormat = MCI_FORMAT_MSF;
	mciSetParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)(LPVOID) &mciSetParms);
	if (dwReturn)
	{
		Con_Printf("MCI_SET_TIME_FORMAT failed (%i)\n", (int)dwReturn);
		mciSendCommand(wDeviceID, MCI_CLOSE, 0, (DWORD_PTR)NULL);
		initializefailed = true;
		return 0;
	}

	initialized = true;

	if (CDAudio_GetAudioDiskInfo() <= 0)
	{
		Con_Printf("CDAudio_Init: No CD in player.\n");
	}

	return true;
}
void CDAudio_Shutdown(void)
{
	if (initialized)
	{
		CDAudio_Stop();
		if (mciSendCommand(wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD_PTR)NULL))
			Con_DPrintf("CDAudio_Shutdown: MCI_CLOSE failed\n");
	}

	initialized = false;
}

void CDAudio_Eject(void)
{
	DWORD	dwReturn;

	dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD_PTR)NULL);
	if (dwReturn)
		Con_DPrintf("MCI_SET_DOOR_OPEN failed (%i)\n", (int)dwReturn);
}

void CDAudio_CloseDoor(void)
{
	DWORD	dwReturn;

	dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, (DWORD_PTR)NULL);
	if (dwReturn)
		Con_DPrintf("MCI_SET_DOOR_CLOSED failed (%i)\n", (int)dwReturn);
}

//try to add time values sensibly because:
//a) microsoft api SUCKS and does not directly support frames.
//b) microsoft buggily stops 3 frames short of the end of the track if we use tmsf...
//c) frames added together will break things
//d) we can subtract an offset so we can actually detect when its reached the end of a track
//e) we need to do frames so we don't break if some track is exactly a multiple of a second long
DWORD MSFToFrames(DWORD base)
{
	int m = MCI_MSF_MINUTE(base);
	int s = MCI_MSF_SECOND(base);
	int f = MCI_MSF_FRAME(base);

	s += m*60;
	f += 75*s;	//75 frames per second.
	return f;
}
DWORD FramesToMSF(DWORD in)
{
	DWORD m, s, f;
	f = in % 75;
	in /= 75;
	s = in % 60;
	in /= 60;
	m = in;

	return MCI_MAKE_MSF(m, s, f);
}

void CDAudio_Play(int track)
{
	DWORD				dwReturn;
	static MCI_PLAY_PARMS		mciPlayParms;
	static MCI_STATUS_PARMS	mciStatusParms;
	DWORD trackstartposition;

	if (track < 1)
	{
		Con_DPrintf("CDAudio: Bad track number %u.\n", track);
		return;
	}

	// don't try to play a non-audio track
	mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = track;
	mciStatusParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD_PTR) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		Con_DPrintf("MCI_STATUS failed (%i)\n", (int)dwReturn);
		return;
	}
	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO)
	{
		Con_Printf("CDAudio: track %i is not audio\n", track);
		return;
	}

	// get the start of the track to be played
	mciStatusParms.dwItem = MCI_STATUS_POSITION;
	mciStatusParms.dwTrack = track;
	mciStatusParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD_PTR) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		Con_DPrintf("MCI_STATUS failed (%i)\n", (int)dwReturn);
		return;
	}
	trackstartposition = mciStatusParms.dwReturn;

	// get the length of the track to be played
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
	mciStatusParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD_PTR) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		Con_DPrintf("MCI_STATUS failed (%i)\n", (int)dwReturn);
		return;
	}

	//set up to play from start to start+length
	mciPlayParms.dwFrom = trackstartposition;
	mciPlayParms.dwTo = resumeend = FramesToMSF(MSFToFrames(trackstartposition) + MSFToFrames(mciStatusParms.dwReturn) - 8);	//-8 to avoid microsoft's potential fuck ups
	mciPlayParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD_PTR)(LPVOID) &mciPlayParms);
	if (dwReturn)
	{
		Con_DPrintf("CDAudio: MCI_PLAY failed (%i)\n", (int)dwReturn);
		return;
	}

	pollneeded = true;

	if (!bgmvolume.value)
		CDAudio_Pause ();

	return;
}


void CDAudio_Stop(void)
{
	DWORD	dwReturn;

	pollneeded = false;

	dwReturn = mciSendCommand(wDeviceID, MCI_STOP, 0, (DWORD_PTR)NULL);
	if (dwReturn)
		Con_DPrintf("MCI_STOP failed (%i)\n", (int)dwReturn);
}


void CDAudio_Pause(void)
{
	DWORD				dwReturn;
	static MCI_GENERIC_PARMS	mciGenericParms;

	if (!pollneeded)
		return;

	mciGenericParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_PAUSE, 0, (DWORD_PTR)(LPVOID) &mciGenericParms);
	if (dwReturn)
		Con_DPrintf("MCI_PAUSE failed (%i)\n", (int)dwReturn);

	pollneeded = false;
}


void CDAudio_Resume(void)
{
	DWORD			dwReturn;
	static MCI_PLAY_PARMS	mciPlayParms;

	if (!bgmvolume.value)
		return;

	mciPlayParms.dwFrom = resumeend;
	mciPlayParms.dwTo = resumeend;
	mciPlayParms.dwCallback = (DWORD_PTR)mainwindow;
	dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD_PTR)(LPVOID) &mciPlayParms);
	if (dwReturn)
	{
		Con_DPrintf("CDAudio: MCI_PLAY failed (%i)\n", (int)dwReturn);
		return;
	}

	pollneeded = true;
}

LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (lParam != wDeviceID)
		return 1;

	switch (wParam)
	{
		case MCI_NOTIFY_SUCCESSFUL:
			pollneeded = false;
			Media_EndedTrack();
			break;

		case MCI_NOTIFY_ABORTED:
		case MCI_NOTIFY_SUPERSEDED:
			break;

		case MCI_NOTIFY_FAILURE:
			Con_DPrintf("MCI_NOTIFY_FAILURE\n");
			CDAudio_Stop ();
			CDAudio_Shutdown();
//			Media_EndedTrack();
			break;

		default:
			Con_DPrintf("Unexpected MM_MCINOTIFY type (%i)\n", (int)wParam);
			return 1;
	}

	return 0;
}

void CDAudio_Update(void)
{
	//workaround for vista bug where MCI_NOTIFY does not work to signal the end of the track.
	if (pollneeded)
	{
		MCI_STATUS_PARMS mciStatusParms;
		mciStatusParms.dwCallback = (DWORD_PTR)mainwindow;
		mciStatusParms.dwItem = MCI_STATUS_POSITION;
		mciStatusParms.dwReturn = resumeend;
		mciStatusParms.dwTrack = 0;
		if (0 == mciSendCommand(wDeviceID, MCI_STATUS, MCI_WAIT|MCI_STATUS_ITEM, (DWORD_PTR)&mciStatusParms))
		{
			unsigned int c, f;
			int cm = MCI_MSF_MINUTE(mciStatusParms.dwReturn);
			int cs = MCI_MSF_SECOND(mciStatusParms.dwReturn);
			int cf = MCI_MSF_FRAME(mciStatusParms.dwReturn);
			int fm = MCI_MSF_MINUTE(resumeend);
			int fs = MCI_MSF_SECOND(resumeend);
			int ff = MCI_MSF_FRAME(resumeend);
			c = cf | (cs<<8) | (cm<<16);
			f = ff | (fs<<8) | (fm<<16);
			if (c >= f)
			{
				pollneeded = false;
				Media_EndedTrack();
			}
		}
	}
}

void CDAudio_Init(void)
{
}
#endif
