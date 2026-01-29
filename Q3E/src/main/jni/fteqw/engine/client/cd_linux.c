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

/*
 * All Unix porting was primarily focused on getting this to work on FreeBSD 14+
 * but hopefully will give others the ability to further improve upon it.
 * Most information sourced from the following documentation:
 * Linux CDROM ioctl: https://www.kernel.org/doc/html/latest/userspace-api/ioctl/cdrom.html
 * FreeBSD 14.0 cd man page: https://man.freebsd.org/cgi/man.cgi?query=cd&sektion=4&apropos=0&manpath=FreeBSD+14.0-RELEASE+and+Ports
 * The cdio header file found at /usr/include/sys/cdio.h (very well documented about what everything does)
 * - Brad
 */

#include "quakedef.h"

#ifndef HAVE_CDPLAYER
	//nothing
#elif defined(__CYGWIN__)
#include "cd_null.c"
#else


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#if defined(__linux__)
#include <linux/cdrom.h>
#elif defined(__unix__) && !defined(__CYGWIN__)
#include <sys/cdio.h>
#define CDROMEJECT CDDOEJECT
#define CDROMCLOSETRAY CDDOCLOSE
#define CDROMPAUSE CDDOPAUSE
#define CDROMRESUME CDDORESUME
#define CDROMRESET CDDORESET
#define CDROMSTOP CDDOSTOP
#define CDROMSTART CDDOSTART
#define CDROMREADTOCHDR CDREADHEADER
#define CDROMREADTOCENTRY CDIOREADTOCENTRY
#define CDROMSUBCHNL CDREADSUBQ
#define CDROM_MSF CD_MSF_FORMAT
#define CDROM_DATA_TRACK CD_SUBQ_DATA
#define CDROM_AUDIO_PAUSED CD_AS_PLAY_PAUSED
#define CDROM_AUDIO_PLAY CD_AS_PLAY_IN_PROGRESS
#endif

static int cdfile = -1;
#if defined(__unix__) && !defined(__APPLE__) && !defined(__CYGWIN__) && !defined(__linux__)
static char cd_dev[64] = "/dev/cd0";
#else
static char cd_dev[64] = "/dev/cdrom";
#endif
static qboolean playing;

void CDAudio_Eject(void)
{
	if (cdfile == -1)
		return; // no cd init'd

	if ( ioctl(cdfile, CDROMEJECT) == -1 ) 
		Con_DPrintf("ioctl cdromeject failed\n");
}


void CDAudio_CloseDoor(void)
{
	if (cdfile == -1)
		return; // no cd init'd

	if ( ioctl(cdfile, CDROMCLOSETRAY) == -1 ) 
		Con_DPrintf("ioctl cdromclosetray failed\n");
}

int CDAudio_GetAudioDiskInfo(void)
{
#ifdef __linux__
	struct cdrom_tochdr tochdr;
#elif defined(__unix__) && !defined(__CYGWIN__)
	struct ioc_toc_header tochdr;
	struct cd_sub_channel_track_info trk;
#endif

	if (cdfile == -1)
		return -1;

	if ( ioctl(cdfile, CDROMREADTOCHDR, &tochdr) == -1 ) 
	{
		Con_DPrintf("ioctl cdromreadtochdr failed\n");
		return -1;
	}

#ifdef __linux__
	if (tochdr.cdth_trk0 < 1) 
#elif defined(__unix__) && !defined(__CYGWIN__)
	if (trk.track_number < 0) // track_number may not work correctly here - Brad
#endif 
	{
		Con_DPrintf("CDAudio: no music tracks\n");
		return -1;
	}

#ifdef __linux__
	return tochdr.cdth_trk1;
#elif defined(__unix__) && !defined(__CYGWIN__)
	return tochdr.starting_track;
#else
	Con_DPrintf("CDAudio: no music tracks\n");
	return -1;
#endif
}


void CDAudio_Play(int track)
{
#ifdef __linux__
	struct cdrom_tocentry entry;
	struct cdrom_ti ti;
#elif defined(__unix__) && !defined(__CYGWIN__) // This all may be wrong, but it should be close - Brad
	struct ioc_toc_header tochdr; // cd drive header info (for getting start and end track info mostly)
	struct ioc_read_toc_single_entry entry; // individual audio track's entry info
	struct cd_sub_channel_info ti; // individual audio track's subchannel info (for indexing and whatnot)
	struct ioc_play_track play; // cd drive audio indexing
#endif

	if (cdfile == -1)
		return;

	// don't try to play a non-audio track
#ifdef __linux__
	entry.cdte_track = track;
	entry.cdte_format = CDROM_MSF;
#elif defined(__unix__) && !defined(__CYGWIN__)
	entry.track = track;
	entry.address_format = CDROM_MSF;
#endif

	if ( ioctl(cdfile, CDROMREADTOCENTRY, &entry) == -1 )
	{
		Con_DPrintf("ioctl cdromreadtocentry failed\n");
		return;
	}
#ifdef __linux__
	if (entry.cdte_ctrl == CDROM_DATA_TRACK)
#elif defined(__unix__) && !defined(__CYGWIN__)
	if (entry.entry.control == CDROM_DATA_TRACK)
#endif
	{
		Con_Printf("CDAudio: track %i is not audio\n", track);
		return;
	}

#ifdef __linux__
	ti.cdti_trk0 = track;
	ti.cdti_trk1 = track;
	ti.cdti_ind0 = 1;
	ti.cdti_ind1 = 99;
#elif defined(__unix__) && !defined(__CYGWIN__)
	if (ti.what.position.track_number == 0 || ti.what.position.track_number == 1)
	{
		entry.track = track;
	}
	if (ti.what.position.index_number == 0)
	{
		play.start_track = tochdr.starting_track;
	}
	if (ti.what.position.index_number == 1)
	{
		play.end_track = tochdr.ending_track;
	}

#define CDROMPLAYTRKIND ti.what.position.track_number
#endif

	if ( ioctl(cdfile, CDROMPLAYTRKIND, &ti) == -1 ) 
	{
		Con_DPrintf("ioctl cdromplaytrkind failed\n");
		return;
	}

	if ( ioctl(cdfile, CDROMRESUME) == -1 ) 
		Con_DPrintf("ioctl cdromresume failed\n");

	playing = true;

	if (!bgmvolume.value || !mastervolume.value)
		CDAudio_Pause ();
}


void CDAudio_Stop(void)
{
	if (cdfile == -1)
		return;

	if ( ioctl(cdfile, CDROMSTOP) == -1 )
		Con_DPrintf("ioctl cdromstop failed (%d)\n", errno);
}

void CDAudio_Pause(void)
{
	if (cdfile == -1)
		return;

	if ( ioctl(cdfile, CDROMPAUSE) == -1 ) 
		Con_DPrintf("ioctl cdrompause failed\n");
}


void CDAudio_Resume(void)
{
	if (cdfile == -1)
		return;
	
	if ( ioctl(cdfile, CDROMRESUME) == -1 ) 
		Con_DPrintf("ioctl cdromresume failed\n");
}

void CDAudio_Update(void)
{
#ifdef __linux__
	struct cdrom_subchnl subchnl;
	static time_t lastchk;
#elif defined(__unix__) && !defined(__CYGWIN__)
	struct cd_sub_channel_info subchnl;
	struct ioc_read_subchannel cdsc; // subchn.cdsc_format workaround - Brad
	// Note: there doesn't seem to be a way to check how much time is left for playing
	// cd audio without doing some extra manual work, so I'll be omitting it from
	// the unix checks for the time being - Brad
#endif

	if (playing 
#ifdef __linux__
		&& lastchk < time(NULL)
#endif
	   )
	{
#ifdef __linux__
		lastchk = time(NULL) + 2; //two seconds between checks
		subchnl.cdsc_format = CDROM_MSF;
#elif defined(__unix__) && !defined(__CYGWIN__)
		cdsc.address_format = CDROM_MSF;
#endif
		if (ioctl(cdfile, CDROMSUBCHNL, &subchnl) == -1 )
		{
			Con_DPrintf("ioctl cdromsubchnl failed\n");
			playing = false;
			return;
		}
#ifdef __linux__
		if (subchnl.cdsc_audiostatus != CDROM_AUDIO_PLAY &&
			subchnl.cdsc_audiostatus != CDROM_AUDIO_PAUSED)
#elif defined(__unix__) && !defined(__CYGWIN__)
		if (subchnl.header.audio_status != CDROM_AUDIO_PLAY &&
			subchnl.header.audio_status != CDROM_AUDIO_PAUSED)
#endif
		{
			playing = false;
			Media_EndedTrack();
		}
	}
}

qboolean CDAudio_Startup(void)
{
	int i;

	if (cdfile != -1)
		return true;

	if ((i = COM_CheckParm("-cddev")) != 0 && i < com_argc - 1)
	{
		Q_strncpyz(cd_dev, com_argv[i + 1], sizeof(cd_dev));
		cd_dev[sizeof(cd_dev) - 1] = 0;
	}

	if ((cdfile = open(cd_dev, O_RDONLY|O_NONBLOCK)) == -1)
	{
		Con_Printf("CDAudio_Init: open of \"%s\" failed (%i)\n", cd_dev, errno);
		cdfile = -1;
		return false;
	}

	return true;
}

void CDAudio_Init(void)
{
}


void CDAudio_Shutdown(void)
{
	if (cdfile == -1)
		return;
	CDAudio_Stop();
	close(cdfile);
	cdfile = -1;
}
#endif
