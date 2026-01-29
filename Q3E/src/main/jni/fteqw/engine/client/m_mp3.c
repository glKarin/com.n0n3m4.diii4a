//mp3 menu and track selector.
//was origonally an mp3 track selector, now handles lots of media specific stuff - like q3 films!
//should rename to m_media.c
#include "quakedef.h"

#ifdef GLQUAKE
#include "glquake.h"
#endif
#ifdef VKQUAKE
#include "../vk/vkrenderer.h"
#endif
#include "shader.h"
#include "gl_draw.h"

#if defined(HAVE_JUKEBOX)
	#if defined(_WIN32) && !defined(WINRT) && !defined(NOMEDIAMENU)
		//#define WINAMP
	#endif
#endif
#if (defined(HAVE_MEDIA_DECODER) || defined(HAVE_MEDIA_ENCODER)) && defined(_WIN32) && !defined(WINRT)
	#define HAVE_API_VFW
#endif

#ifdef _WIN32
	#include "winquake.h"
#endif
#if defined(AVAIL_MP3_ACM) || defined(HAVE_API_VFW)
//equivelent to
//#include <msacm.h>
#undef CDECL	//windows is stupid at times.
#define CDECL __cdecl

#if defined(_MSC_VER) && (_MSC_VER < 1300)
	#define DWORD_PTR DWORD
#endif

DECLARE_HANDLE(HACMSTREAM);
typedef HACMSTREAM *LPHACMSTREAM;
DECLARE_HANDLE(HACMDRIVER);
typedef struct {
	DWORD     cbStruct;
	DWORD     fdwStatus;
	DWORD_PTR dwUser;
	LPBYTE    pbSrc;
	DWORD     cbSrcLength;
	DWORD     cbSrcLengthUsed;
	DWORD_PTR dwSrcUser;
	LPBYTE    pbDst;
	DWORD     cbDstLength;
	DWORD     cbDstLengthUsed;
	DWORD_PTR dwDstUser;
	DWORD     dwReservedDriver[10];
} ACMSTREAMHEADER, *LPACMSTREAMHEADER;
#define ACM_STREAMCONVERTF_BLOCKALIGN   0x00000004




//mingw workarounds
#define LPWAVEFILTER void *
#include <objbase.h>

MMRESULT (WINAPI *qacmStreamUnprepareHeader) (HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwUnprepare);
MMRESULT (WINAPI *qacmStreamConvert) (HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert);
MMRESULT (WINAPI *qacmStreamPrepareHeader) (HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwPrepare);
MMRESULT (WINAPI *qacmStreamOpen) (LPHACMSTREAM phas, HACMDRIVER had, LPWAVEFORMATEX pwfxSrc, LPWAVEFORMATEX pwfxDst, LPWAVEFILTER pwfltr, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
MMRESULT (WINAPI *qacmStreamClose) (HACMSTREAM has, DWORD fdwClose);

static qboolean qacmStartup(void)
{
	static int inited;
	static dllhandle_t *module;
	if (!inited)
	{
		dllfunction_t funcs[] =
		{
			{(void*)&qacmStreamUnprepareHeader,	"acmStreamUnprepareHeader"},
			{(void*)&qacmStreamConvert,			"acmStreamConvert"},
			{(void*)&qacmStreamPrepareHeader,	"acmStreamPrepareHeader"},
			{(void*)&qacmStreamOpen,			"acmStreamOpen"},
			{(void*)&qacmStreamClose,			"acmStreamClose"},
			{NULL,NULL}
		};
		inited = true;
		module = Sys_LoadLibrary("msacm32.dll", funcs);
	}

	return module?true:false;
}
#endif

static char media_currenttrack[MAX_QPATH];
static cvar_t music_fade = CVAR("music_fade", "1");

//higher bits have priority (if they have something to play).
#define MEDIA_GAMEMUSIC (1u<<0)	//cd music. also music command etc.
#ifdef HAVE_JUKEBOX
#define MEDIA_CVARLIST	(1u<<1)	//cvar abuse. handy for preserving times when switching tracks.
#define MEDIA_PLAYLIST	(1u<<2)	//
static unsigned int media_playlisttypes;
static unsigned int media_playlistcurrent;

//cvar abuse
static int music_playlist_last;
static cvar_t music_playlist_index = CVAR("music_playlist_index", "-1");
//	created dynamically: CVAR("music_playlist_list0+", ""),
//	created dynamically: CVAR("music_playlist_sampleposition0+", "-1"),


typedef struct mediatrack_s{
	char filename[MAX_QPATH];
	char nicename[MAX_QPATH];
	int length;
	struct mediatrack_s *next;
} mediatrack_t;

//info about the current stuff that is playing.
static char media_friendlyname[MAX_QPATH];



int lasttrackplayed;

cvar_t media_shuffle = CVAR("media_shuffle", "1");
cvar_t media_repeat = CVAR("media_repeat", "1");
#ifdef WINAMP
cvar_t media_hijackwinamp = CVAR("media_hijackwinamp", "0");
#endif

int selectedoption=-1;
int numtracks;
int nexttrack=-1;
mediatrack_t *tracks;

char media_iofilename[MAX_OSPATH]="";

#if !defined(NOMEDIAMENU) && !defined(NOBUILTINMENUS)
static int mouseselectedoption=-1;
void Media_LoadTrackNames (char *listname);
void Media_SaveTrackNames (char *listname);
int loadedtracknames;
#endif
qboolean Media_EvaluateNextTrack(void);

#else
#define NOMEDIAMENU	//the media menu requires us to be able to queue up random tracks and stuff.
#endif

#ifdef HAVE_CDPLAYER
static int cdplayingtrack;	//currently playing cd track (becomes 0 when paused)
static int cdpausedtrack;	//currently paused cd track

//info about (fake)cd tracks that we want to play
int cdplaytracknum;

//info about (fake)cd tracks that we could play if asked.
#define REMAPPED_TRACKS 256
static struct
{
	char fname[MAX_QPATH];
} cdremap[REMAPPED_TRACKS];
static qboolean cdenabled;
static int cdnumtracks;		//maximum cd track we can play.
#endif


static char media_playtrack[MAX_QPATH];		//name of track to play next/now
static char media_loopingtrack[MAX_QPATH];	//name of track to loop afterwards
qboolean media_fadeout;
float media_fadeouttime;

qboolean Media_CleanupTrackName(const char *track, int *out_track, char *result, size_t resultsize);

//whatever music track was previously playing has terminated.
//return value is the new sample to start playing.
//*starttime says the time into the track that we should resume playing at
//this is on the main thread with the mixer locked, its safe to do stuff, but try not to block
sfx_t *Media_NextTrack(int musicchannelnum, float *starttime)
{
	sfx_t *s = NULL;
	if (bgmvolume.value <= 0 || mastervolume.value <= 0)
		return NULL;
	media_fadeout = false;	//it has actually ended now, at least on one device. don't fade the new track too...

	Q_strncpyz(media_currenttrack, "", sizeof(media_currenttrack));
#ifdef HAVE_JUKEBOX
	Q_strncpyz(media_friendlyname, "", sizeof(media_friendlyname));

	music_playlist_index.modified = false;
	music_playlist_last = -1;
	media_playlistcurrent = 0;

	if (!media_playlistcurrent && (media_playlisttypes & MEDIA_PLAYLIST))
	{
#if !defined(NOMEDIAMENU) && !defined(NOBUILTINMENUS)
		if (!loadedtracknames)
			Media_LoadTrackNames("sound/media.m3u");
#endif
		if (Media_EvaluateNextTrack())
		{
			media_playlistcurrent = MEDIA_PLAYLIST;
			if (*media_currenttrack == '#')
				return S_PrecacheSound2(media_currenttrack+1, true);
			else
				return S_PrecacheSound(media_currenttrack);
		}
	}
	if (!media_playlistcurrent && (media_playlisttypes & MEDIA_CVARLIST))
	{
		if (music_playlist_index.ival >= 0)
		{
			cvar_t *list = Cvar_Get(va("music_playlist_list%i", music_playlist_index.ival), "", 0, "compat");
			if (list)
			{
				cvar_t *sampleposition = Cvar_Get(va("music_playlist_sampleposition%i", music_playlist_index.ival), "-1", 0, "compat");
				Q_snprintfz(media_currenttrack, sizeof(media_currenttrack), "sound/cdtracks/%s", list->string);
				Q_strncpyz(media_friendlyname, "", sizeof(media_friendlyname));
				media_playlistcurrent = MEDIA_CVARLIST;
				music_playlist_last = music_playlist_index.ival;
				if (sampleposition)
				{
					*starttime = sampleposition->value;
					if (*starttime == -1)
						*starttime = 0;
				}
				else
					*starttime = 0;

				s = S_PrecacheSound(media_currenttrack);
			}
		}
	}

	if (!media_playlistcurrent && (media_playlisttypes & MEDIA_GAMEMUSIC))
#endif
	{
#ifdef HAVE_CDPLAYER
		if (cdplaytracknum)
		{
			if (cdplayingtrack != cdplaytracknum && cdpausedtrack != cdplaytracknum)
			{
				CDAudio_Play(cdplaytracknum);
				cdplayingtrack = cdplaytracknum;
			}
#ifdef HAVE_JUKEBOX
			media_playlistcurrent = MEDIA_GAMEMUSIC;
#endif
			return NULL;
		}
#endif
		if (*media_playtrack || *media_loopingtrack)
		{
			if (*media_playtrack)
			{
				Q_strncpyz(media_currenttrack, media_playtrack, sizeof(media_currenttrack));
				*media_playtrack = 0;
			}
			else
			{
				if (!Media_CleanupTrackName(media_loopingtrack, NULL, media_currenttrack, sizeof(media_currenttrack)))
					Q_strncpyz(media_currenttrack, "", sizeof(media_currenttrack));
			}
#ifdef HAVE_JUKEBOX
			Q_strncpyz(media_friendlyname, "", sizeof(media_friendlyname));
			media_playlistcurrent = MEDIA_GAMEMUSIC;
#endif
			s = S_PrecacheSound(media_currenttrack);
		}
	}
	return s;
}

//begin cross-fading
static qboolean Media_Changed (unsigned int mediatype)
{
#ifdef HAVE_JUKEBOX
	//something changed, but it has a lower priority so we don't care
	if (mediatype < media_playlistcurrent)
		return false;
#endif

#ifdef HAVE_CDPLAYER
	//make sure we're not playing any cd music.
	if (cdplayingtrack || cdpausedtrack)
	{
		cdplayingtrack = 0;
		cdpausedtrack = 0;
		CDAudio_Stop();
	}
#endif
	media_fadeout = music_fade.ival;
	media_fadeouttime = realtime;
	return true;
}

//returns the new volume the sample should be at, to support fading.
//if we return < 0, the mixer will know to kill whatever is currently playing, ready for a new track.
//this is on the main thread with the mixer locked, its safe to do stuff, but try not to block
float Media_CrossFade(int musicchanel, float vol, float time)
{
	if (media_fadeout)
	{
		float fadetime = 1;
		float frac = (fadetime + media_fadeouttime - realtime)/fadetime;
		vol *= frac;
	}
#ifdef HAVE_JUKEBOX
	else if (music_playlist_index.modified)
	{
		if (Media_Changed(MEDIA_CVARLIST))
		{
			if (music_playlist_last >= 0)
			{
				cvar_t *sampleposition = Cvar_Get(va("music_playlist_sampleposition%i", music_playlist_last), "-1", 0, "compat");
				if (sampleposition && sampleposition->value != -1)
					Cvar_SetValue(sampleposition, time);
			}
			vol = -1;	//kill it NOW
		}
	}
#endif
	return vol;
}

void Media_WriteCurrentTrack(sizebuf_t *buf)
{
	//fixme: for demo playback
	MSG_WriteByte (buf, svc_cdtrack);
	MSG_WriteByte (buf, 0);
}

qboolean Media_CleanupTrackName(const char *track, int *out_track, char *result, size_t resultsize)
{
	//FIXME: for q2, gog uses ../music/Track%02i.ogg, with various remapping requirements for the mission packs.
	static char *path[] =
	{
		"music/",
		"sound/cdtracks/",
		"",
		NULL
	};
	static char *ext[] =
	{
		"",
#if defined(AVAIL_OGGOPUS) || defined(FTE_TARGET_WEB)
		".opus",	//opus might be the future, but ogg is the present
#endif
#if defined(AVAIL_OGGVORBIS) || defined(FTE_TARGET_WEB)
		".ogg",
#endif
#if defined(AVAIL_MP3_ACM) || defined(FTE_TARGET_WEB)
		".mp3",
#endif
		".wav",
#if defined(PLUGINS)	//ffmpeg plugin? woo.
	#if !(defined(AVAIL_OGGOPUS) || defined(FTE_TARGET_WEB))
		".opus",	//opus might be the future, but ogg is the present
	#endif
	#if !(defined(AVAIL_OGGVORBIS) || defined(FTE_TARGET_WEB))
		".ogg",
	#endif
	#if !(defined(AVAIL_MP3_ACM) || defined(FTE_TARGET_WEB))
		".mp3",
	#endif
		".flac",	//supported by QS at least.
		//".s3m",	//some variant of mod that noone cares about. listed because of qs.
		//".umx",	//wtf? qs is weird.
#endif
		NULL
	};
	unsigned int tracknum;
	char *trackend;
	unsigned int ip, ie;
	int bestdepth = 0x7fffffff, d;
	char tryname[MAX_QPATH];

	//check if its a proper number (0123456789 without any other weird stuff. if so, we can use fake track paths or actual cd tracks)
	tracknum = strtoul(track, &trackend, 10);
	if (*trackend)
		tracknum = 0;	//not a single integer
#ifdef HAVE_CDPLAYER
	if (tracknum > 0 && tracknum < REMAPPED_TRACKS && *cdremap[tracknum].fname)
	{	//remap the track if its remapped.
		track = cdremap[tracknum].fname;
		tracknum = strtoul(track, &trackend, 0);
		if (*trackend)
			tracknum = 0;
	}
#endif

#ifndef HAVE_LEGACY
	if (!tracknum)	//might as well require exact file
	{
		Q_snprintfz(result, resultsize, "%s", track);
		d = COM_FCheckExists(result);
	}
	else
#endif
	for(ip = 0; path[ip]; ip++)
	{
		if (tracknum)
		{
			if (*path[ip])
			{
				if (tracknum <= 999)
				{
					for(ie = 0; ext[ie]; ie++)
					{
						Q_snprintfz(tryname, sizeof(tryname), "%strack%03u%s", path[ip], tracknum, ext[ie]);
						d = COM_FDepthFile(tryname, false);
						if (d < bestdepth)
						{
							bestdepth = d;
							Q_strncpy(result, tryname, resultsize);
						}
					}
				}
				if (tracknum <= 99)
				{
					for(ie = 0; ext[ie]; ie++)
					{
						Q_snprintfz(tryname, sizeof(tryname), "%strack%02u%s", path[ip], tracknum, ext[ie]);
						d = COM_FDepthFile(tryname, false);
						if (d < bestdepth)
						{
							bestdepth = d;
							Q_strncpy(result, tryname, resultsize);
						}
					}
				}
			}
		}
		else
		{
			for(ie = 0; ext[ie]; ie++)
			{
				Q_snprintfz(tryname, sizeof(tryname), "%s%s%s", path[ip], track, ext[ie]);
				d = COM_FDepthFile(tryname, false);
				if (d < bestdepth)
				{
					bestdepth = d;
					Q_strncpy(result, tryname, resultsize);
				}
			}
		}
	}

	if (out_track)
		*out_track = tracknum;
	if (bestdepth < 0x7fffffff)
		return true;
	return false;
}

//controls which music track should be playing right now
//track and looptrack will usually be the same thing, track is what to play NOW, looptrack is what to keep re-playing after, or "-" for stop.
qboolean Media_NamedTrack(const char *track, const char *looptrack)
{
	unsigned int tracknum;
	char trackname[MAX_QPATH];

	if (!track && !looptrack)
	{
		*media_playtrack = *media_loopingtrack = 0;
		Media_Changed(MEDIA_GAMEMUSIC);
		return true;
	}

	if (!track || !*track)			//ignore calls if the primary track is invalid. whatever is already playing will continue to play.
		return false;
	if (!looptrack || !*looptrack)	//null or empty looptrack loops using the primary track, for compat with q3.
		looptrack = track;

	if (!strcmp(looptrack, "-"))	//- for the looptrack argument can be used to prevent looping.
		looptrack = "";

	//okay, do that faketrack thing if we got one.
	if (Media_CleanupTrackName(track, &tracknum, trackname, sizeof(trackname)))
	{
#ifdef HAVE_CDPLAYER
		cdplaytracknum = 0;
#endif
		Q_strncpyz(media_playtrack, trackname, sizeof(media_playtrack));
		Q_strncpyz(media_loopingtrack, looptrack, sizeof(media_loopingtrack));
		Media_Changed(MEDIA_GAMEMUSIC);
		return true;
	}

#ifdef HAVE_CDPLAYER
	//couldn't do a faketrack, resort to actual cd tracks, if we're allowed
	if (tracknum && cdenabled)
	{
		Q_strncpyz(media_loopingtrack, looptrack, sizeof(media_loopingtrack));

		if (!CDAudio_Startup())
			return false;
		if (cdnumtracks <= 0)
			cdnumtracks = CDAudio_GetAudioDiskInfo();

		if (tracknum > cdnumtracks)
		{
			Con_DPrintf("CDAudio: Bad track number %u.\n", tracknum);
			return false;	//can't play that, sorry. its not an available track
		}

		if (cdplayingtrack == tracknum)
			return true;	//already playing, don't need to do anything

		cdplaytracknum = tracknum;
		Q_strncpyz(media_playtrack, "", sizeof(media_playtrack));
		Media_Changed(MEDIA_GAMEMUSIC);
		return true;
	}
#endif
	return false;
}

//for q3
void Media_NamedTrack_f(void)
{
	if (Cmd_Argc() == 3)
		Media_NamedTrack(Cmd_Argv(1), Cmd_Argv(2));
	else
		Media_NamedTrack(Cmd_Argv(1), Cmd_Argv(1));
}
void Media_StopTrack_f(void)
{
	*media_playtrack = *media_loopingtrack = 0;
	Media_Changed(MEDIA_GAMEMUSIC);
}

void Media_NumberedTrack(unsigned int initialtrack, unsigned int looptrack)
{
	char *init = initialtrack?va("%u", initialtrack):NULL;
	char *loop = looptrack?va("%u", looptrack):NULL;

	Media_NamedTrack(init, loop);
}

void Media_EndedTrack(void)
{
#ifdef HAVE_CDPLAYER
	cdplayingtrack = 0;
	cdpausedtrack = 0;
#endif

	if (*media_loopingtrack)
		Media_NamedTrack(media_loopingtrack, media_loopingtrack);
}




void Media_SetPauseTrack(qboolean paused)
{
#ifdef HAVE_CDPLAYER
	if (paused)
	{
		if (!cdplayingtrack)
			return;
		cdpausedtrack = cdplayingtrack;
		cdplayingtrack = 0;
		CDAudio_Pause();
	}
	else
	{
		if (!cdpausedtrack)
			return;
		cdplayingtrack = cdpausedtrack;
		cdpausedtrack = 0;
		CDAudio_Resume();
	}
#endif
}

#ifdef HAVE_CDPLAYER
void Media_SaveTracks(vfsfile_t *outcfg)
{
	unsigned int n;
	char buf[MAX_QPATH*4];
	for (n = 1; n < REMAPPED_TRACKS; n++)
		if (*cdremap[n].fname)
			Con_Printf("cd remap %u %s\n", n, COM_QuotedString(cdremap[n].fname, buf, sizeof(buf), false));
}
void CD_f (void)
{
	char	*command;
	int		ret;
	int		n;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "play") == 0)
	{
		Media_NamedTrack(Cmd_Argv(2), "-");
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		if (Cmd_Argc() < 4)
			Media_NamedTrack(Cmd_Argv(2), NULL);
		else
			Media_NamedTrack(Cmd_Argv(2), Cmd_Argv(3));
		return;
	}

	if (Q_strcasecmp(command, "remap") == 0)
	{
		ret = Cmd_Argc() - 2;
		if (ret <= 0)
		{
			for (n = 1; n < REMAPPED_TRACKS; n++)
				if (*cdremap[n].fname)
					Con_Printf("  %u -> %s\n", n, cdremap[n].fname);
			return;
		}
		for (n = 1; n <= ret && n < REMAPPED_TRACKS; n++)
			Q_strncpyz(cdremap[n].fname, Cmd_Argv (n+1), sizeof(cdremap[n].fname));
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		*media_playtrack = *media_loopingtrack = 0;
		cdplaytracknum = 0;
		Media_Changed(MEDIA_GAMEMUSIC);
		return;
	}

	if (!bgmvolume.value)
	{
		Con_Printf("Background music is disabled: %s is 0\n", bgmvolume.name);
		return;
	}

	if (!CDAudio_Startup())
	{
		Con_Printf("No cd drive detected\n");
		return;
	}

	if (Q_strcasecmp(command, "on") == 0)
	{
		cdenabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		if (cdplayingtrack || cdpausedtrack)
			CDAudio_Stop();
		cdenabled = false;

		*media_playtrack = *media_loopingtrack = 0;
		cdplaytracknum = 0;
		Media_Changed(MEDIA_GAMEMUSIC);
		return;
	}

	if (Q_strcasecmp(command, "reset") == 0)
	{
		cdenabled = true;
		if (cdplayingtrack || cdpausedtrack)
			CDAudio_Stop();
		for (n = 0; n < REMAPPED_TRACKS; n++)
			strcpy(cdremap[n].fname, "");
		cdnumtracks = CDAudio_GetAudioDiskInfo();
		return;
	}

	if (Q_strcasecmp(command, "close") == 0)
	{
		CDAudio_CloseDoor();
		return;
	}

	if (cdnumtracks <= 0)
	{
		cdnumtracks = CDAudio_GetAudioDiskInfo();
		if (cdnumtracks < 0)
		{
			Con_Printf("No CD in player.\n");
			return;
		}
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		Media_SetPauseTrack(true);
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		Media_SetPauseTrack(false);
		return;
	}

	if (Q_strcasecmp(command, "eject") == 0)
	{
		if (Cmd_IsInsecure())
			return;

		if (cdplayingtrack || cdpausedtrack)
			CDAudio_Stop();
		CDAudio_Eject();
		cdnumtracks = -1;
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		Con_Printf("%u tracks\n", cdnumtracks);
		if (cdplayingtrack > 0)
			Con_Printf("Currently %s track %u\n", *media_loopingtrack ? "looping" : "playing", cdplayingtrack);
		else if (cdpausedtrack > 0)
			Con_Printf("Paused %s track %u\n", *media_loopingtrack ? "looping" : "playing", cdpausedtrack);
		return;
	}
}
#endif




#ifdef HAVE_JUKEBOX

#ifdef WINAMP

#include "winamp.h"
HWND hwnd_winamp;

#endif

qboolean Media_EvaluateNextTrack(void);


#ifdef WINAMP
qboolean WinAmp_GetHandle (void)
{
	if ((hwnd_winamp = FindWindowW(L"Winamp", NULL)))
		return true;
	if ((hwnd_winamp = FindWindowW(L"Winamp v1.x", NULL)))
		return true;

	*currenttrack.nicename = '\0';

	return false;
}

qboolean WinAmp_StartTune(char *name)
{
	int trys;
	int pos;
	COPYDATASTRUCT cds;
	if (!WinAmp_GetHandle())
		return false;

	//FIXME: extract from fs if it's in a pack.
	//FIXME: always give absolute path
	cds.dwData = IPC_PLAYFILE;
	cds.lpData = (void *) name;
	cds.cbData = strlen((char *) cds.lpData)+1; // include space for null char
	SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_DELETE);
	SendMessage(hwnd_winamp,WM_COPYDATA,(WPARAM)NULL,(LPARAM)&cds);
	SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)0,IPC_STARTPLAY );

	for (trys = 1000; trys; trys--)
	{
		pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
		if (pos>100 && SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME)>=0)	//tune has started
			break;

		Sleep(10);	//give it a chance.
		if (!WinAmp_GetHandle())
			break;
	}

	return true;
}

void WinAmp_Think(void)
{
	int pos;
	int len;

	if (!WinAmp_GetHandle())
		return;

	pos = bgmvolume.value*255;
	if (pos > 255) pos = 255;
	if (pos < 0) pos = 0;
	PostMessage(hwnd_winamp, WM_WA_IPC,pos,IPC_SETVOLUME);

//optimise this to reduce calls?
	pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
	len = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETOUTPUTTIME)*1000;

	if ((pos > len || pos <= 100) && len != -1)
	if (Media_EvaluateNextTrack())
		WinAmp_StartTune(currenttrack.filename);
}
#endif
void Media_Seek (float time)
{
#ifdef WINAMP
	if (media_hijackwinamp.value)
	{
		int pos;
		if (WinAmp_GetHandle())
		{
			pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
			pos += time*1000;
			PostMessage(hwnd_winamp,WM_WA_IPC,pos,IPC_JUMPTOTIME);

			WinAmp_Think();
		}
	}
#endif
	S_Music_Seek(time);
}

void Media_FForward_f(void)
{
	float time = atoi(Cmd_Argv(1));
	if (!time)
		time = 15;
	Media_Seek(time);
}
void Media_Rewind_f (void)
{
	float time = atoi(Cmd_Argv(1));
	if (!time)
		time = 15;
	Media_Seek(-time);
}

qboolean Media_EvaluateNextTrack(void)
{
	mediatrack_t *track;
	int trnum;
	if (!tracks)
		return false;
	if (nexttrack>=0)
	{
		trnum = nexttrack;
		for (track = tracks; track; track=track->next)
		{
			if (!trnum)
			{
				Q_strncpyz(media_currenttrack, track->filename, sizeof(media_currenttrack));
				Q_strncpyz(media_friendlyname, track->nicename, sizeof(media_friendlyname));
				lasttrackplayed = nexttrack;
				break;
			}
			trnum--;
		}
		nexttrack = -1;
	}
	else
	{
		if (media_shuffle.value)
			nexttrack=((float)(rand()&0x7fff)/0x7fff)*numtracks;
		else
		{
			nexttrack = lasttrackplayed+1;
			if (nexttrack >= numtracks)
			{
				if (media_repeat.value)
					nexttrack = 0;
				else
				{
					*media_currenttrack='\0';
					*media_friendlyname='\0';
					return false;
				}
			}
		}
		trnum = nexttrack;
		for (track = tracks; track; track=track->next)
		{
			if (!trnum)
			{
				Q_strncpyz(media_currenttrack, track->filename, sizeof(media_currenttrack));
				Q_strncpyz(media_friendlyname, track->nicename, sizeof(media_friendlyname));
				lasttrackplayed = nexttrack;
				break;
			}
			trnum--;
		}
		nexttrack = -1;
	}

	return true;
}

//actually, this func just flushes and states that it should be playing. the ambientsound func actually changes the track.
void Media_Next_f (void)
{
	Media_Changed(MEDIA_PLAYLIST);

#ifdef WINAMP
	if (media_hijackwinamp.value)
	{
		if (WinAmp_GetHandle())
		if (Media_EvaluateNextTrack())
			WinAmp_StartTune(currenttrack.filename);
	}
#endif
}





void Media_AddTrack(const char *fname)
{
	mediatrack_t *newtrack;
	if (!*fname)
		return;
	for (newtrack = tracks; newtrack; newtrack = newtrack->next)
	{
		if (!strcmp(newtrack->filename, fname))
			return;	//already added. ho hum
	}
	newtrack = Z_Malloc(sizeof(mediatrack_t));
	Q_strncpyz(newtrack->filename, fname, sizeof(newtrack->filename));
	Q_strncpyz(newtrack->nicename, COM_SkipPath(fname), sizeof(newtrack->nicename));
	newtrack->length = 0;
	newtrack->next = tracks;
	tracks = newtrack;
	numtracks++;

	if (numtracks == 1)
		Media_Changed(MEDIA_PLAYLIST);
}
void Media_RemoveTrack(const char *fname)
{
	mediatrack_t **link, *newtrack;
	if (!*fname)
		return;
	for (link = &tracks; *link; link = &(*link)->next)
	{
		newtrack = *link;
		if (!strcmp(newtrack->filename, fname))
		{
			*link = newtrack->next;
			numtracks--;

			if (!strcmp(media_currenttrack, newtrack->filename))
				Media_Changed(MEDIA_PLAYLIST);
			Z_Free(newtrack);
			return;
		}
	}
}

#if !defined(NOMEDIAMENU) && !defined(NOBUILTINMENUS)
void M_Media_Add_f (void)
{
	char *fname = Cmd_Argv(1);

	if (!loadedtracknames)
		Media_LoadTrackNames("sound/media.m3u");

	if (Cmd_Argc() == 1)
		Con_Printf("%s <track>\n", Cmd_Argv(0));
	else
		Media_AddTrack(fname);

	Media_SaveTrackNames("sound/media.m3u");
}
void M_Media_Remove_f (void)
{
	char *fname = Cmd_Argv(1);

	if (Cmd_Argc() == 1)
		Con_Printf("%s <track>\n", Cmd_Argv(0));
	else
		Media_RemoveTrack(fname);

	Media_SaveTrackNames("sound/media.m3u");
}


void Media_LoadTrackNames (char *listname);

#define MEDIA_MIN	-7
#define MEDIA_VOLUME -7
#define MEDIA_FASTFORWARD -6
#define MEDIA_CLEARLIST -5
#define MEDIA_ADDTRACK -4
#define MEDIA_ADDLIST -3
#define MEDIA_SHUFFLE -2
#define MEDIA_REPEAT -1

void M_Media_Draw (emenu_t *menu)
{
	mediatrack_t *track;
	int y;
	int op, i, mop;
	qboolean playing;

	float time, duration;
	char title[256];
	playing = S_GetMusicInfo(0, &time, &duration, title, sizeof(title));

#define MP_Hightlight(x,y,text,hl) (hl?M_PrintWhite(x, y, text):M_Print(x, y, text))

	if (!bgmvolume.value)
		M_Print (12, 32, "Not playing - no volume");
	else if (!*media_currenttrack)
	{
		if (!tracks)
			M_Print (12, 32, "Not playing - no track to play");
		else
		{
#ifdef WINAMP
			if (!WinAmp_GetHandle())
				M_Print (12, 32, "Please start WinAmp 2");
			else
#endif
				M_Print (12, 32, "Not playing - switched off");
		}
	}
	else
	{
		M_Print (12, 32, "Currently playing:");
		if (*title)
		{
			M_Print (12, 40, title);
			M_Print (12, 48, media_currenttrack);
		}
		else
			M_Print (12, 40, *media_friendlyname?media_friendlyname:media_currenttrack);
	}

	y=60;
	op = selectedoption - (vid.height-y)/16;
	if (op + (vid.height-y)/8>numtracks)
		op = numtracks - (vid.height-y)/8;
	if (op < MEDIA_MIN)
		op = MEDIA_MIN;
	mop = (mousecursor_y - y)/8;
	mop += MEDIA_MIN;
	mouseselectedoption = MEDIA_MIN-1;
	if (mousecursor_x < 12 + ((vid.width - 320)>>1) || mousecursor_x > 320-24 + ((vid.width - 320)>>1))
		mop = mouseselectedoption;
	while(op < 0)
	{
		if (op == mop)
		{
			float alphamax = 0.5, alphamin = 0.2;
			mouseselectedoption = op;
			R2D_ImageColours(.5,.4,0,(sin(realtime*2)+1)*0.5*(alphamax-alphamin)+alphamin);
			R2D_FillBlock(12 + ((vid.width - 320)>>1), y, 320-24, 8);
			R2D_ImageColours(1.0, 1.0, 1.0, 1.0);
		}
		switch(op)
		{
		case MEDIA_VOLUME:
			MP_Hightlight (12, y, va("<< Volume %2i%%    >>", (int)(100*bgmvolume.value)), op == selectedoption);
			y+=8;
			break;
		case MEDIA_CLEARLIST:
			MP_Hightlight (12, y, "Clear all", op == selectedoption);
			y+=8;
			break;
		case MEDIA_FASTFORWARD:
			{
				if (playing)
				{
					int itime = time;
					int iduration = duration;
					if (iduration)
					{
						int pct = (int)((100*time)/duration);
						MP_Hightlight (12, y, va("<< %i:%02i / %i:%02i - %i%% >>", itime/60, itime%60, iduration/60, iduration%60, pct), op == selectedoption);
					}
					else
						MP_Hightlight (12, y, va("<< %i:%02i >>", itime/60, itime%60), op == selectedoption);
				}
				else
					MP_Hightlight (12, y, "<<    skip		 >>", op == selectedoption);
			}
			y+=8;
			break;
		case MEDIA_ADDTRACK:
			MP_Hightlight (12, y, "Add Track", op == selectedoption);
			if (op == selectedoption)
				M_PrintWhite (12+9*8, y, media_iofilename);
			y+=8;
			break;
		case MEDIA_ADDLIST:
			MP_Hightlight (12, y, "Add List", op == selectedoption);
			if (op == selectedoption)
				M_PrintWhite (12+9*8, y, media_iofilename);
			y+=8;
			break;
		case MEDIA_SHUFFLE:
			if (media_shuffle.value)
				MP_Hightlight (12, y, "Shuffle on", op == selectedoption);
			else
				MP_Hightlight (12, y, "Shuffle off", op == selectedoption);
			y+=8;
			break;
		case MEDIA_REPEAT:
			if (!media_shuffle.value)
			{
				if (media_repeat.value)
					MP_Hightlight (12, y, "Repeat on", op == selectedoption);
				else
					MP_Hightlight (12, y, "Repeat off", op == selectedoption);
			}
			else
			{
				if (media_repeat.value)
					MP_Hightlight (12, y, "(Repeat on)", op == selectedoption);
				else
					MP_Hightlight (12, y, "(Repeat off)", op == selectedoption);
			}
			y+=8;
			break;
		}
		op++;
	}

	for (track = tracks, i=0; track && i<op; track=track->next, i++);
	for (; track; track=track->next, y+=8, op++)
	{
		if (track->length != (int)duration && *title && !strcmp(track->filename, media_currenttrack))
		{
			Q_strncpyz(track->nicename, title, sizeof(track->nicename));
			track->length = duration;
			Media_SaveTrackNames("sound/media.m3u");
		}

		if (op == mop)
		{
			float alphamax = 0.5, alphamin = 0.2;
			mouseselectedoption = op;
			R2D_ImageColours(.5,.4,0,(sin(realtime*2)+1)*0.5*(alphamax-alphamin)+alphamin);
			R2D_FillBlock(12 + ((vid.width - 320)>>1), y, 320-24, 8);
			R2D_ImageColours(1.0, 1.0, 1.0, 1.0);
		}
		if (op == selectedoption)
			M_PrintWhite (12, y, track->nicename);
		else
			M_Print (12, y, track->nicename);
	}
}

char compleatenamepath[MAX_OSPATH];
char compleatenamename[MAX_OSPATH];
qboolean compleatenamemultiple;
int QDECL Com_CompleatenameCallback(const char *name, qofs_t size, time_t mtime, void *data, searchpathfuncs_t *spath)
{
	if (*compleatenamename)
		compleatenamemultiple = true;
	Q_strncpyz(compleatenamename, name, sizeof(compleatenamename));

	return true;
}
void Com_CompleateOSFileName(char *name)
{
	char *ending;
	compleatenamemultiple = false;

	strcpy(compleatenamepath, name);
	ending = COM_SkipPath(compleatenamepath);
	if (compleatenamepath!=ending)
		ending[-1] = '\0';	//strip a slash
	*compleatenamename='\0';

	Sys_EnumerateFiles(NULL, va("%s*", name), Com_CompleatenameCallback, NULL, NULL);
	Sys_EnumerateFiles(NULL, va("%s*.*", name), Com_CompleatenameCallback, NULL, NULL);

	if (*compleatenamename)
		strcpy(name, compleatenamename);
}

qboolean M_Media_Key (emenu_t *menu, int key, unsigned int unicode)
{
	int dir;
	if (key == K_ESCAPE || key == K_GP_BACK || key == K_MOUSE2)
	{
		return false;
	}
	else if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW || key == K_GP_DPAD_RIGHT || key == K_LEFTARROW || key == K_KP_LEFTARROW || key == K_GP_DPAD_LEFT)
	{
		if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW || key == K_GP_DPAD_RIGHT)
			dir = 1;
		else dir = -1;
		switch(selectedoption)
		{
		case MEDIA_VOLUME:
			bgmvolume.value += dir * 0.1;
			if (bgmvolume.value < 0)
				bgmvolume.value = 0;
			if (bgmvolume.value > 1)
				bgmvolume.value = 1;
			Cvar_SetValue (&bgmvolume, bgmvolume.value);
			break;
		case MEDIA_FASTFORWARD:
			Media_Seek(15*dir);
			break;
		default:
			if (selectedoption >= 0)
				Media_Next_f();
			break;
		}
	}
	else if (key == K_DOWNARROW || key == K_KP_DOWNARROW || key == K_GP_DPAD_DOWN)
	{
		selectedoption++;
		if (selectedoption>=numtracks)
			selectedoption = numtracks-1;
	}
	else if (key == K_PGDN)
	{
		selectedoption+=10;
		if (selectedoption>=numtracks)
			selectedoption = numtracks-1;
	}
	else if (key == K_UPARROW || key == K_KP_UPARROW || key == K_GP_DPAD_UP)
	{
		selectedoption--;
		if (selectedoption < MEDIA_MIN)
			selectedoption = MEDIA_MIN;
	}
	else if (key == K_PGUP)
	{
		selectedoption-=10;
		if (selectedoption < MEDIA_MIN)
			selectedoption = MEDIA_MIN;
	}
	else if (key == K_DEL|| key == K_GP_DIAMOND_ALTCONFIRM)
	{
		if (selectedoption>=0)
		{
			mediatrack_t *prevtrack=NULL, *tr;
			int num=0;
			tr=tracks;
			while(tr)
			{
				if (num == selectedoption)
				{
					if (prevtrack)
						prevtrack->next = tr->next;
					else
						tracks = tr->next;
					numtracks--;
					if (!strcmp(media_currenttrack, tr->filename))
						Media_Changed(MEDIA_PLAYLIST);
					Z_Free(tr);
					break;
				}

				prevtrack = tr;
				tr=tr->next;
				num++;
			}
		}
	}
	else if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_MOUSE1 || key == K_TOUCHTAP)
	{
		if (key == K_MOUSE1)
		{
			if (mouseselectedoption < MEDIA_MIN)
				return false;
			selectedoption = mouseselectedoption;
		}
		switch(selectedoption)
		{
		case MEDIA_FASTFORWARD:
			Media_Seek(15);
			break;
		case MEDIA_CLEARLIST:
			{
				mediatrack_t *prevtrack;
				while(tracks)
				{
					prevtrack = tracks;
					tracks=tracks->next;
					Z_Free(prevtrack);
					numtracks--;
				}
				if (numtracks!=0)
				{
					numtracks=0;
					Con_SafePrintf("numtracks should be 0\n");
				}
				Media_Changed(MEDIA_PLAYLIST);
			}
			break;
		case MEDIA_ADDTRACK:
			if (*media_iofilename)
				Media_AddTrack(media_iofilename);
			else
				Cmd_ExecuteString("menu_mediafiles", RESTRICT_LOCAL);
			break;
		case MEDIA_ADDLIST:
			if (*media_iofilename)
				Media_LoadTrackNames(media_iofilename);
			break;
		case MEDIA_SHUFFLE:
			Cvar_Set(&media_shuffle, media_shuffle.value?"0":"1");
			break;
		case MEDIA_REPEAT:
			Cvar_Set(&media_repeat, media_repeat.value?"0":"1");
			break;
		default:
			if (selectedoption>=0)
			{
				nexttrack = selectedoption;
				Media_Next_f();
				return true;
			}
			return false;
		}
		return true;
	}
	else
	{
		if (selectedoption == MEDIA_ADDLIST || selectedoption == MEDIA_ADDTRACK)
		{
			if (key == K_TAB)
			{
				Com_CompleateOSFileName(media_iofilename);
				return true;
			}
			else if (key == K_BACKSPACE)
			{
				dir = strlen(media_iofilename);
				if (dir)
					media_iofilename[dir-1] = '\0';
				return true;
			}
			else if ((key >= 'a' && key <= 'z') || (key >= '0' && key <= '9') || key == '/' || key == '_' || key == '.' || key == ':')
			{
				dir = strlen(media_iofilename);
				media_iofilename[dir] = key;
				media_iofilename[dir+1] = '\0';
				return true;
			}
		}
		else if (selectedoption>=0)
		{
			mediatrack_t *tr;
			int num=0;
			tr=tracks;
			while(tr)
			{
				if (num == selectedoption)
					break;

				tr=tr->next;
				num++;
			}
			if (!tr)
				return false;

			if (key == K_BACKSPACE)
			{
				dir = strlen(tr->nicename);
				if (dir)
					tr->nicename[dir-1] = '\0';
				return true;
			}
			else if ((key >= 'a' && key <= 'z') || (key >= '0' && key <= '9') || key == '/' || key == '_' || key == '.' || key == ':'  || key == '&' || key == '|' || key == '#' || key == '\'' || key == '\"' || key == '\\' || key == '*' || key == '@' || key == '!' || key == '(' || key == ')' || key == '%' || key == '^' || key == '?' || key == '[' || key == ']' || key == ';' || key == ':' || key == '+' || key == '-' || key == '=')
			{
				dir = strlen(tr->nicename);
				tr->nicename[dir] = key;
				tr->nicename[dir+1] = '\0';
				return true;
			}
		}
	}
	return false;
}

void M_Menu_Media_f (void)
{
	emenu_t *menu;
	if (!loadedtracknames)
		Media_LoadTrackNames("sound/media.m3u");

	menu = M_CreateMenu(0);

//	MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
	MC_AddCenterPicture(menu, 4, 24, "gfx/p_option.lmp");

	menu->key = M_Media_Key;
	menu->postdraw = M_Media_Draw;
}



void Media_SaveTrackNames (char *listname)
{
	mediatrack_t *tr;
	vfsfile_t *f = FS_OpenVFS(listname, "wb", FS_GAMEONLY);
	if (!f)
		return;
	VFS_PRINTF(f, "#EXTM3U\n");
	for (tr = tracks; tr; tr = tr->next)
	{
		VFS_PRINTF(f, "#EXTINF:%i,%s\n%s\n", tr->length, tr->nicename, tr->filename);
	}
	VFS_CLOSE(f);
}

//safeprints only.
void Media_LoadTrackNames (char *listname)
{
	char *lineend;
	char *len;
	char *filename;
	char *trackname;
	mediatrack_t *newtrack;
	size_t fsize;
	char *data = COM_LoadTempFile(listname, FSLF_IGNOREPURE, &fsize);

	loadedtracknames=true;

	if (!data)
		return;

	if (!Q_strncasecmp(data, "#extm3u", 7))
	{
		data = strchr(data, '\n')+1;
		for(;;)
		{
			lineend = strchr(data, '\n');

			if (Q_strncasecmp(data, "#extinf:", 8))
			{
				if (!lineend)
					return;
				Con_SafePrintf("Bad m3u file\n");
				return;
			}
			len = data+8;
			trackname = strchr(data, ',')+1;
			if (!trackname)
				return;

			if (lineend > data && lineend[-1] == '\r')
				lineend[-1]='\0';
			else
				lineend[0]='\0';

			filename = data = lineend+1;

			lineend = strchr(data, '\n');

			if (lineend)
			{
				if (lineend > data && lineend[-1] == '\r')
					lineend[-1]='\0';
				else
					lineend[0]='\0';
				data = lineend+1;
			}

			newtrack = Z_Malloc(sizeof(mediatrack_t));
			Q_strncpyz(newtrack->filename, filename, sizeof(newtrack->filename));
			Q_strncpyz(newtrack->nicename, trackname, sizeof(newtrack->nicename));
			newtrack->length = atoi(len);
			newtrack->next = tracks;
			tracks = newtrack;
			numtracks++;

			if (!lineend)
				return;
		}
	}
	else
	{
		for(;;)
		{
			trackname = filename = data;
			lineend = strchr(data, '\n');

			if (!lineend && !*data)
				break;
			lineend[-1]='\0';
			data = lineend+1;

			newtrack = Z_Malloc(sizeof(mediatrack_t));
			Q_strncpyz(newtrack->filename, filename, sizeof(newtrack->filename));
			Q_strncpyz(newtrack->nicename, COM_SkipPath(trackname), sizeof(newtrack->nicename));
			newtrack->length = 0;
			newtrack->next = tracks;
			tracks = newtrack;
			numtracks++;

			if (!lineend)
				break;
		}
	}
}
#endif

#endif





///temporary residence for media handling

#ifdef HAVE_API_VFW
#if 0
#include <vfw.h>
#else
typedef struct 
{
	DWORD fccType;
	DWORD fccHandler;
	DWORD dwFlags;
	DWORD dwCaps;
	WORD  wPriority;
	WORD  wLanguage;
	DWORD dwScale;
	DWORD dwRate;
	DWORD dwStart;
	DWORD dwLength;
	DWORD dwInitialFrames;
	DWORD dwSuggestedBufferSize;
	DWORD dwQuality;
	DWORD dwSampleSize;
	RECT  rcFrame;
	DWORD dwEditCount;
	DWORD dwFormatChangeCount;
	TCHAR szName[64];
} AVISTREAMINFOA, *LPAVISTREAMINFOA;
typedef struct AVISTREAM *PAVISTREAM;
typedef struct AVIFILE *PAVIFILE;
typedef struct GETFRAME *PGETFRAME;
typedef struct	 
{
	DWORD  fccType;
	DWORD  fccHandler;
	DWORD  dwKeyFrameEvery;
	DWORD  dwQuality;
	DWORD  dwBytesPerSecond;
	DWORD  dwFlags;
	LPVOID lpFormat;
	DWORD  cbFormat;
	LPVOID lpParms;
	DWORD  cbParms;
	DWORD  dwInterleaveEvery;
} AVICOMPRESSOPTIONS;
#define streamtypeVIDEO         mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         mmioFOURCC('a', 'u', 'd', 's')
#define AVISTREAMREAD_CONVENIENT	(-1L)
#define AVIIF_KEYFRAME	0x00000010L
#endif

ULONG	(WINAPI *qAVIStreamRelease)			(PAVISTREAM pavi);
HRESULT	(WINAPI *qAVIStreamEndStreaming)	(PAVISTREAM pavi);
HRESULT	(WINAPI *qAVIStreamGetFrameClose)	(PGETFRAME pg);
HRESULT	(WINAPI *qAVIStreamRead)			(PAVISTREAM pavi, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG FAR * plBytes, LONG FAR * plSamples);
LPVOID	(WINAPI *qAVIStreamGetFrame)		(PGETFRAME pg, LONG lPos);
HRESULT	(WINAPI *qAVIStreamReadFormat)		(PAVISTREAM pavi, LONG lPos,LPVOID lpFormat,LONG FAR *lpcbFormat);
LONG	(WINAPI *qAVIStreamStart)			(PAVISTREAM pavi);
PGETFRAME(WINAPI*qAVIStreamGetFrameOpen)	(PAVISTREAM pavi, LPBITMAPINFOHEADER lpbiWanted);
HRESULT	(WINAPI *qAVIStreamBeginStreaming)	(PAVISTREAM pavi, LONG lStart, LONG lEnd, LONG lRate);
LONG	(WINAPI *qAVIStreamSampleToTime)	(PAVISTREAM pavi, LONG lSample);
LONG	(WINAPI *qAVIStreamLength)			(PAVISTREAM pavi);
HRESULT	(WINAPI *qAVIStreamInfoA)			(PAVISTREAM pavi, LPAVISTREAMINFOA psi, LONG lSize);
ULONG	(WINAPI *qAVIFileRelease)			(PAVIFILE pfile);
HRESULT	(WINAPI *qAVIFileGetStream)			(PAVIFILE pfile, PAVISTREAM FAR * ppavi, DWORD fccType, LONG lParam);
HRESULT	(WINAPI *qAVIFileOpenA)				(PAVIFILE FAR *ppfile, LPCSTR szFile, UINT uMode, LPCLSID lpHandler);
void	(WINAPI *qAVIFileInit)				(void);
HRESULT	(WINAPI *qAVIStreamWrite)			(PAVISTREAM pavi, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, LONG FAR *plBytesWritten);
HRESULT	(WINAPI *qAVIStreamSetFormat)		(PAVISTREAM pavi, LONG lPos,LPVOID lpFormat,LONG cbFormat);
HRESULT	(WINAPI *qAVIMakeCompressedStream)	(PAVISTREAM FAR * ppsCompressed, PAVISTREAM ppsSource, AVICOMPRESSOPTIONS FAR * lpOptions, CLSID FAR *pclsidHandler);
HRESULT	(WINAPI *qAVIFileCreateStreamA)		(PAVIFILE pfile, PAVISTREAM FAR *ppavi, AVISTREAMINFOA FAR * psi);

static qboolean qAVIStartup(void)
{
	static int aviinited;
	static dllhandle_t *avimodule;
	if (!aviinited)
	{
		dllfunction_t funcs[] =
		{
			{(void*)&qAVIFileInit,				"AVIFileInit"},
			{(void*)&qAVIStreamRelease,			"AVIStreamRelease"},
			{(void*)&qAVIStreamEndStreaming,	"AVIStreamEndStreaming"},
			{(void*)&qAVIStreamGetFrameClose,	"AVIStreamGetFrameClose"},
			{(void*)&qAVIStreamRead,			"AVIStreamRead"},
			{(void*)&qAVIStreamGetFrame,		"AVIStreamGetFrame"},
			{(void*)&qAVIStreamReadFormat,		"AVIStreamReadFormat"},
			{(void*)&qAVIStreamStart,			"AVIStreamStart"},
			{(void*)&qAVIStreamGetFrameOpen,	"AVIStreamGetFrameOpen"},
			{(void*)&qAVIStreamBeginStreaming,	"AVIStreamBeginStreaming"},
			{(void*)&qAVIStreamSampleToTime,	"AVIStreamSampleToTime"},
			{(void*)&qAVIStreamLength,			"AVIStreamLength"},
			{(void*)&qAVIStreamInfoA,			"AVIStreamInfoA"},
			{(void*)&qAVIFileRelease,			"AVIFileRelease"},
			{(void*)&qAVIFileGetStream,			"AVIFileGetStream"},
			{(void*)&qAVIFileOpenA,				"AVIFileOpenA"},
			{(void*)&qAVIStreamWrite,			"AVIStreamWrite"},
			{(void*)&qAVIStreamSetFormat,		"AVIStreamSetFormat"},
			{(void*)&qAVIMakeCompressedStream,	"AVIMakeCompressedStream"},
			{(void*)&qAVIFileCreateStreamA,		"AVIFileCreateStreamA"},
			{NULL,NULL}
		};
		aviinited = true;
		avimodule = Sys_LoadLibrary("avifil32.dll", funcs);

		if (avimodule)
			qAVIFileInit();
	}

	return avimodule?true:false;
}
#endif

#ifdef HAVE_MEDIA_DECODER

static char *cin_prop_buf;
static size_t cin_prop_bufsize;
struct cin_s
{
	qboolean (*decodeframe)(cin_t *cin, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ctx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ctx);
	void (*shutdown)(cin_t *cin);	//warning: doesn't free cin_t
	void (*rewind)(cin_t *cin);
	//these are any interactivity functions you might want...
	void (*cursormove) (struct cin_s *cin, float posx, float posy);	//pos is 0-1
	void (*key) (struct cin_s *cin, int code, int unicode, int event);
	qboolean (*setsize) (struct cin_s *cin, int width, int height);
	void (*getsize) (struct cin_s *cin, int *width, int *height, float *aspect);
	void (*changestream) (struct cin_s *cin, const char *streamname);
	qboolean (*getproperty) (struct cin_s *cin, const char *key, char *buffer, size_t *buffersize);



	//these are the outputs (not always power of two!)
	cinstates_t playstate;
	qboolean forceframe;
	float filmpercentage;

	texid_t texture;


#ifdef HAVE_API_VFW
	struct {
		AVISTREAMINFOA		vidinfo;
		PAVISTREAM			pavivideo;
		AVISTREAMINFOA		audinfo;
		PAVISTREAM			pavisound;
		PAVIFILE			pavi;
		PGETFRAME			pgf;

		HACMSTREAM			audiodecoder;

		LPWAVEFORMATEX pWaveFormat;

		//sound stuff
		int soundpos;
		int currentframe;	//previously displayed frame, to avoid excess decoding

		//source info
		float filmfps;
		int num_frames;
	} avi;
#endif

#ifdef PLUGINS
	struct {
		void *ctx;
		struct plugin_s *plug;
		media_decoder_funcs_t *funcs;	/*fixme*/
		struct cin_s *next;
		struct cin_s *prev;
	} plugin;
#endif

	struct {
		qbyte		*data;
		uploadfmt_t	format;
		int			width;
		int			height;
	} image;

	struct {
		struct roq_info_s *roqfilm;
//		float lastmediatime;
		float nextframetime;
	} roq;

	struct {
		struct cinematics_s *cin;
	} q2cin;

	float filmstarttime;

	qbyte *framedata;	//Z_Malloced buffer
};

static menu_t videomenu;	//to capture keys+draws+etc. a singleton - multiple videos will be queued instead of simultaneous.
static shader_t *videoshader;

//////////////////////////////////////////////////////////////////////////////////
//AVI Support (windows)
#ifdef HAVE_API_VFW
static void Media_WinAvi_Shutdown(struct cin_s *cin)
{
	qAVIStreamGetFrameClose(cin->avi.pgf);
	qAVIStreamEndStreaming(cin->avi.pavivideo);
	qAVIStreamRelease(cin->avi.pavivideo);
	//we don't need to free the file (we freed it immediatly after getting the stream handles)
}
static void Media_WinAvi_Rewind (cin_t *cin)
{
	cin->avi.soundpos = 0;
	cin->avi.currentframe = -1;
}
static qboolean Media_WinAvi_DecodeFrame (cin_t *cin, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ctx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ctx)
{
	LPBITMAPINFOHEADER lpbi;									// Holds The Bitmap Header Information
	float newframe;
	int newframei;
	int wantsoundtime;
	extern cvar_t _snd_mixahead;


	newframe = ((mediatime * cin->avi.vidinfo.dwRate) / cin->avi.vidinfo.dwScale) + cin->avi.vidinfo.dwInitialFrames;
	newframei = newframe;

	if (newframei>=cin->avi.num_frames)
		return false;

	if (newframei == cin->avi.currentframe && !forcevideo)
		return true;

	if (cin->avi.currentframe < newframei-1)
		Con_DPrintf("Dropped %i frame(s)\n", (newframei - cin->avi.currentframe)-1);

	cin->avi.currentframe = newframei;
	cin->filmpercentage = newframe / cin->avi.num_frames;

	if (newframei>=cin->avi.num_frames)
		return false;
	
	lpbi = (LPBITMAPINFOHEADER)qAVIStreamGetFrame(cin->avi.pgf, newframei);	// Grab Data From The AVI Stream
	if (!lpbi || lpbi->biBitCount != 24)//oops
		return false;

	uploadtexture(ctx, TF_BGR24_FLIP, lpbi->biWidth, lpbi->biHeight, (char*)lpbi+lpbi->biSize, NULL);

	if(nosound)
		wantsoundtime = 0;
	else
		wantsoundtime = (((mediatime + _snd_mixahead.value + 0.02) * cin->avi.audinfo.dwRate) / cin->avi.audinfo.dwScale) + cin->avi.audinfo.dwInitialFrames;

	while (cin->avi.pavisound && cin->avi.soundpos < wantsoundtime)
	{
		LONG lSize;
		LPBYTE pBuffer;
		LONG samples;

		/*if the audio skipped more than a second, drop it all and start at a sane time, so our raw audio playing code doesn't buffer too much*/
		if (cin->avi.soundpos + (1*cin->avi.audinfo.dwRate / cin->avi.audinfo.dwScale) < wantsoundtime)
		{
			cin->avi.soundpos = wantsoundtime;
			break;
		}

		qAVIStreamRead(cin->avi.pavisound, cin->avi.soundpos, AVISTREAMREAD_CONVENIENT, NULL, 0, &lSize, &samples);
		pBuffer = cin->framedata;
		qAVIStreamRead(cin->avi.pavisound, cin->avi.soundpos, AVISTREAMREAD_CONVENIENT, pBuffer, lSize, NULL, &samples);

		cin->avi.soundpos+=samples;

		/*if no progress, stop!*/
		if (!samples)
			break;

		if (cin->avi.audiodecoder)
		{
			ACMSTREAMHEADER strhdr;
			char buffer[1024*256];

			memset(&strhdr, 0, sizeof(strhdr));
			strhdr.cbStruct = sizeof(strhdr);
			strhdr.pbSrc = pBuffer;
			strhdr.cbSrcLength = lSize;
			strhdr.pbDst = buffer;
			strhdr.cbDstLength = sizeof(buffer);

			qacmStreamPrepareHeader(cin->avi.audiodecoder, &strhdr, 0);
			qacmStreamConvert(cin->avi.audiodecoder, &strhdr, ACM_STREAMCONVERTF_BLOCKALIGN);
			qacmStreamUnprepareHeader(cin->avi.audiodecoder, &strhdr, 0);

			S_RawAudio(SOURCEID_CINEMATIC, strhdr.pbDst, cin->avi.pWaveFormat->nSamplesPerSec, strhdr.cbDstLengthUsed/4, cin->avi.pWaveFormat->nChannels, 2, volume.value);
		}
		else
			S_RawAudio(SOURCEID_CINEMATIC, pBuffer, cin->avi.pWaveFormat->nSamplesPerSec, samples, cin->avi.pWaveFormat->nChannels, 2, volume.value);
	}
	return true;
}

static cin_t *Media_WinAvi_TryLoad(char *name)
{
	cin_t *cin;
	PAVIFILE			pavi;
	flocation_t loc;

	if (strchr(name, ':'))
		return NULL;

	if (!qAVIStartup())
		return NULL;


	FS_FLocateFile(name, FSLF_IFFOUND, &loc);

	if (!loc.offset && *loc.rawname && !qAVIFileOpenA(&pavi, loc.rawname, OF_READ, NULL))//!AVIStreamOpenFromFile(&pavi, name, streamtypeVIDEO, 0, OF_READ, NULL))
	{
		int filmwidth;
		int filmheight;

		cin = Z_Malloc(sizeof(cin_t));
		cin->avi.pavi = pavi;

		if (qAVIFileGetStream(cin->avi.pavi, &cin->avi.pavivideo, streamtypeVIDEO, 0))	//retrieve video stream
		{
			qAVIFileRelease(pavi);
			Con_Printf("%s contains no video stream\n", name);
			return NULL;
		}
		if (qAVIFileGetStream(cin->avi.pavi, &cin->avi.pavisound, streamtypeAUDIO, 0))	//retrieve audio stream
		{
			Con_DPrintf("%s contains no audio stream\n", name);
			cin->avi.pavisound=NULL;
		}
		qAVIFileRelease(cin->avi.pavi);

//play with video
		qAVIStreamInfoA(cin->avi.pavivideo, &cin->avi.vidinfo, sizeof(cin->avi.vidinfo));
		filmwidth=cin->avi.vidinfo.rcFrame.right-cin->avi.vidinfo.rcFrame.left;					// Width Is Right Side Of Frame Minus Left
		filmheight=cin->avi.vidinfo.rcFrame.bottom-cin->avi.vidinfo.rcFrame.top;					// Height Is Bottom Of Frame Minus Top
		cin->framedata = BZ_Malloc(filmwidth*filmheight*4);

		cin->avi.num_frames=qAVIStreamLength(cin->avi.pavivideo);							// The Last Frame Of The Stream
		cin->avi.filmfps=1000.0f*(float)cin->avi.num_frames/(float)qAVIStreamSampleToTime(cin->avi.pavivideo,cin->avi.num_frames);		// Calculate Rough Milliseconds Per Frame

		qAVIStreamBeginStreaming(cin->avi.pavivideo, 0, cin->avi.num_frames, 100);

		cin->avi.pgf=qAVIStreamGetFrameOpen(cin->avi.pavivideo, NULL);

		if (!cin->avi.pgf)
		{
			Con_Printf("AVIStreamGetFrameOpen failed. Please install a vfw codec for '%c%c%c%c'. Try ffdshow.\n", 
				((unsigned char*)&cin->avi.vidinfo.fccHandler)[0],
				((unsigned char*)&cin->avi.vidinfo.fccHandler)[1],
				((unsigned char*)&cin->avi.vidinfo.fccHandler)[2],
				((unsigned char*)&cin->avi.vidinfo.fccHandler)[3]
				);
		}

		cin->avi.currentframe=-1;
		cin->filmstarttime = Sys_DoubleTime();

		cin->avi.soundpos=0;


//play with sound
		if (cin->avi.pavisound)
		{
			LONG lSize;
			LPBYTE pChunk;
			qAVIStreamInfoA(cin->avi.pavisound, &cin->avi.audinfo, sizeof(cin->avi.audinfo));

			qAVIStreamRead(cin->avi.pavisound, 0, AVISTREAMREAD_CONVENIENT, NULL, 0, &lSize, NULL);

			if (!lSize)
				cin->avi.pWaveFormat = NULL;
			else
			{
				pChunk = BZ_Malloc(sizeof(qbyte)*lSize);

				if(qAVIStreamReadFormat(cin->avi.pavisound, qAVIStreamStart(cin->avi.pavisound), pChunk, &lSize))
				{
				   // error
					Con_Printf("Failure reading sound info\n");
				}
				cin->avi.pWaveFormat = (LPWAVEFORMATEX)pChunk;
			}

			if (!cin->avi.pWaveFormat)
			{
				Con_Printf("VFW: unable to decode audio\n");
				qAVIStreamRelease(cin->avi.pavisound);
				cin->avi.pavisound=NULL;
			}
			else if (cin->avi.pWaveFormat->wFormatTag != 1)
			{
				WAVEFORMATEX pcm_format;
				HACMDRIVER drv = NULL;

				memset (&pcm_format, 0, sizeof(pcm_format));
				pcm_format.wFormatTag = WAVE_FORMAT_PCM;
				pcm_format.nChannels = cin->avi.pWaveFormat->nChannels;
				pcm_format.nSamplesPerSec = cin->avi.pWaveFormat->nSamplesPerSec;
				pcm_format.nBlockAlign = 4;
				pcm_format.nAvgBytesPerSec = pcm_format.nSamplesPerSec*4;
				pcm_format.wBitsPerSample = 16;
				pcm_format.cbSize = 0;

				if (!qacmStartup() || 0!=qacmStreamOpen(&cin->avi.audiodecoder, drv, cin->avi.pWaveFormat, &pcm_format, NULL, 0, 0, 0))
				{
					Con_Printf("Failed to open audio decoder\n");	//FIXME: so that it no longer is...
					qAVIStreamRelease(cin->avi.pavisound);
					cin->avi.pavisound=NULL;
				}
			}
		}

		cin->decodeframe = Media_WinAvi_DecodeFrame;
		cin->rewind = Media_WinAvi_Rewind;
		cin->shutdown = Media_WinAvi_Shutdown;
		return cin;
	}
	return NULL;
}
#endif

//AVI Support (windows)
//////////////////////////////////////////////////////////////////////////////////
//Plugin Support
#ifdef PLUGINS
static media_decoder_funcs_t *plugindecodersfunc[8];
static struct plugin_s *plugindecodersplugin[8];
static cin_t *active_cin_plugins;

qboolean Media_RegisterDecoder(struct plugin_s *plug, media_decoder_funcs_t *funcs)
{
	int i;
	if (!funcs || funcs->structsize != sizeof(*funcs))
		return false;

	for (i = 0; i < sizeof(plugindecodersfunc)/sizeof(plugindecodersfunc[0]); i++)
	{
		if (plugindecodersfunc[i] == NULL)
		{
			plugindecodersfunc[i] = funcs;
			plugindecodersplugin[i] = plug;
			return true;
		}
	}
	return false;
}
/*funcs==null closes ALL decoders from this plugin*/
qboolean Media_UnregisterDecoder(struct plugin_s *plug, media_decoder_funcs_t *funcs)
{
	qboolean success = true;
	int i;
	static media_decoder_funcs_t deadfuncs;
	struct plugin_s *oldplug = currentplug;
	cin_t *cin, *next;

	for (i = 0; i < sizeof(plugindecodersfunc)/sizeof(plugindecodersfunc[0]); i++)
	{
		if (plugindecodersfunc[i] == funcs || (!funcs && plugindecodersplugin[i] == plug))
		{
			//kill any cinematics currently using that decoder
			for (cin = active_cin_plugins; cin; cin = next)
			{
				next = cin->plugin.next;

				if (cin->plugin.plug == plug && cin->plugin.funcs == plugindecodersfunc[i])
				{
					//we don't kill the engine's side of it, not just yet anyway.
					currentplug = cin->plugin.plug;
					if (cin->plugin.funcs->shutdown)
						cin->plugin.funcs->shutdown(cin->plugin.ctx);
					cin->plugin.funcs = &deadfuncs;
					cin->plugin.plug = NULL;
					cin->plugin.ctx = NULL;
				}
			}
			currentplug = oldplug;


			plugindecodersfunc[i] = NULL;
			plugindecodersplugin[i] = NULL;
			if (funcs)
				return success;
		}
	}

	if (!funcs)
	{
		static media_decoder_funcs_t deadfuncs;
		struct plugin_s *oldplug = currentplug;
		cin_t *cin, *next;

		for (cin = active_cin_plugins; cin; cin = next)
		{
			next = cin->plugin.next;

			if (cin->plugin.plug == plug)
			{
				//we don't kill the engine's side of it, not just yet anyway.
				currentplug = cin->plugin.plug;
				if (cin->plugin.funcs->shutdown)
					cin->plugin.funcs->shutdown(cin->plugin.ctx);
				cin->plugin.funcs = &deadfuncs;
				cin->plugin.plug = NULL;
				cin->plugin.ctx = NULL;
			}
		}
		currentplug = oldplug;
	}
	return success;
}

static qboolean Media_Plugin_DecodeFrame (cin_t *cin, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ctx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ctx)
{
	qboolean okay;
	struct plugin_s *oldplug = currentplug;
	if (!cin->plugin.funcs->decodeframe)
		return false;	//plugin closed or something

	currentplug = cin->plugin.plug;
	okay = cin->plugin.funcs->decodeframe(cin->plugin.ctx, nosound, forcevideo, mediatime, uploadtexture, ctx);
	currentplug = oldplug;
	return okay;
}
static void Media_Plugin_Shutdown(cin_t *cin)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->shutdown)
		cin->plugin.funcs->shutdown(cin->plugin.ctx);
	currentplug = oldplug;

	if (cin->plugin.prev)
		cin->plugin.prev->plugin.next = cin->plugin.next;
	else
		active_cin_plugins = cin->plugin.next;
	if (cin->plugin.next)
		cin->plugin.next->plugin.prev = cin->plugin.prev;
}
static void Media_Plugin_Rewind(cin_t *cin)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->rewind)
		cin->plugin.funcs->rewind(cin->plugin.ctx);
	currentplug = oldplug;
}

void Media_Plugin_MoveCursor(cin_t *cin, float posx, float posy)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->cursormove)
		cin->plugin.funcs->cursormove(cin->plugin.ctx, posx, posy);
	currentplug = oldplug;
}
void Media_Plugin_KeyPress(cin_t *cin, int code, int unicode, int event)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->key)
		cin->plugin.funcs->key(cin->plugin.ctx, code, unicode, event);
	currentplug = oldplug;
}
qboolean Media_Plugin_SetSize(cin_t *cin, int width, int height)
{
	qboolean result = false;
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->setsize)
		result = cin->plugin.funcs->setsize(cin->plugin.ctx, width, height);
	currentplug = oldplug;
	return result;
}
void Media_Plugin_GetSize(cin_t *cin, int *width, int *height, float *aspect)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->getsize)
		cin->plugin.funcs->getsize(cin->plugin.ctx, width, height);
	currentplug = oldplug;
}
void Media_Plugin_ChangeStream(cin_t *cin, const char *streamname)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->changestream)
		cin->plugin.funcs->changestream(cin->plugin.ctx, streamname);
	currentplug = oldplug;
}
qboolean Media_Plugin_GetProperty(cin_t *cin, const char *propname, char *data, size_t *datasize)
{
	qboolean ret = false;
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->getproperty)
		ret = cin->plugin.funcs->getproperty(cin->plugin.ctx, propname, data, datasize);
	currentplug = oldplug;
	return ret;
}

cin_t *Media_Plugin_TryLoad(char *name)
{
	cin_t *cin;
	int i;
	media_decoder_funcs_t *funcs = NULL;
	struct plugin_s *plug = NULL;
	void *ctx = NULL;
	struct plugin_s *oldplug = currentplug;
	for (i = 0; i < sizeof(plugindecodersfunc)/sizeof(plugindecodersfunc[0]); i++)
	{
		funcs = plugindecodersfunc[i];
		if (funcs)
		{
			plug = plugindecodersplugin[i];
			currentplug = plug;
			ctx = funcs->createdecoder(name);
			if (ctx)
				break;
		}
	}
	currentplug = oldplug;

	if (ctx)
	{
		cin = Z_Malloc(sizeof(cin_t));
		cin->plugin.funcs = funcs;
		cin->plugin.plug = plug;
		cin->plugin.ctx = ctx;
		cin->plugin.next = active_cin_plugins;
		cin->plugin.prev = NULL;
		if (cin->plugin.next)
			cin->plugin.next->plugin.prev = cin;
		active_cin_plugins = cin;
		cin->decodeframe = Media_Plugin_DecodeFrame;
		cin->shutdown = Media_Plugin_Shutdown;
		cin->rewind = Media_Plugin_Rewind;

		if (funcs->cursormove)
			cin->cursormove		= Media_Plugin_MoveCursor;
		if (funcs->key)
			cin->key			= Media_Plugin_KeyPress;
		if (funcs->setsize)
			cin->setsize		= Media_Plugin_SetSize;
		if (funcs->getsize)
			cin->getsize		= Media_Plugin_GetSize;
		if (funcs->changestream)
			cin->changestream	= Media_Plugin_ChangeStream;
		if (funcs->getproperty)
			cin->getproperty	= Media_Plugin_GetProperty;

		return cin;
	}
	return NULL;
}
#endif
//Plugin Support
//////////////////////////////////////////////////////////////////////////////////
//Quake3 RoQ Support

#ifdef Q3CLIENT
#include "roq.h"
static void Media_Roq_Shutdown(struct cin_s *cin)
{
	roq_close(cin->roq.roqfilm);
	cin->roq.roqfilm=NULL;
}

static void Media_Roq_Rewind(struct cin_s *cin)
{
	roq_rewind(cin->roq.roqfilm);
	cin->roq.nextframetime = 0;
}

static qboolean Media_Roq_DecodeFrame (cin_t *cin, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ctx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ctx)
{
	qboolean doupdate = forcevideo;

	//seeking could probably be done a bit better...
	//key frames or something, I dunno
	while (mediatime >= cin->roq.nextframetime)
	{
		switch (roq_read_frame(cin->roq.roqfilm)) //0 if end, -1 if error, 1 if success
		{
		default:	//error
		case 0:		//eof
			if (!doupdate)
				return false;
			break;
		case 1:
			break;	//success
		}
		cin->roq.nextframetime += 1.0/30;
		doupdate = true;
	}

	if (doupdate)
	{
		if (cin->roq.roqfilm->num_frames)
			cin->filmpercentage = cin->roq.roqfilm->frame_num / cin->roq.roqfilm->num_frames;
		else
			cin->filmpercentage = 0;

		uploadtexture(ctx, TF_RGBX32, cin->roq.roqfilm->width, cin->roq.roqfilm->height, cin->roq.roqfilm->rgba[0], NULL);

		if (!nosound)
		{
			while (cin->roq.roqfilm->audio_channels && S_HaveOutput() && cin->roq.roqfilm->aud_pos < cin->roq.roqfilm->vid_pos)
			{
				if (roq_read_audio(cin->roq.roqfilm)>0)		
					S_RawAudio(SOURCEID_CINEMATIC, cin->roq.roqfilm->audio, 22050, cin->roq.roqfilm->audio_size/cin->roq.roqfilm->audio_channels, cin->roq.roqfilm->audio_channels, 2, volume.value );
				else
					break;
			}
		}
	}
	return true;
}

void Media_Roq_GetSize(struct cin_s *cin, int *width, int *height, float *aspect)
{
	*width = cin->roq.roqfilm->width;
	*height = cin->roq.roqfilm->height;
	*aspect = (float)cin->roq.roqfilm->width/cin->roq.roqfilm->height;
}

static cin_t *Media_RoQ_TryLoad(char *name)
{
	cin_t *cin;
	roq_info *roqfilm;
	if (strchr(name, ':'))
		return NULL;

	if ((roqfilm = roq_open(name)))
	{
		cin = Z_Malloc(sizeof(cin_t));
		cin->decodeframe = Media_Roq_DecodeFrame;
		cin->rewind = Media_Roq_Rewind;
		cin->shutdown = Media_Roq_Shutdown;
		cin->getsize = Media_Roq_GetSize;

		cin->roq.roqfilm = roqfilm;

		return cin;
	}
	return NULL;
}
#endif

//Quake3 RoQ Support
//////////////////////////////////////////////////////////////////////////////////
//Static Image Support

#ifndef MINIMAL
static void Media_Static_Shutdown(struct cin_s *cin)
{
	BZ_Free(cin->image.data);
	cin->image.data = NULL;
}

static qboolean Media_Static_DecodeFrame (cin_t *cin, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ctx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ctx)
{
	if (forcevideo)
		uploadtexture(ctx, cin->image.format, cin->image.width, cin->image.height, cin->image.data, NULL);
	return true;
}

static cin_t *Media_Static_TryLoad(char *name)
{
	cin_t *cin;
	char *dot = strrchr(name, '.');

	if (dot && (!strcmp(dot, ".pcx") || !strcmp(dot, ".tga") || !strcmp(dot, ".png") || !strcmp(dot, ".jpg")))
	{
		qbyte *staticfilmimage;
		int imagewidth;
		int imageheight;
		uploadfmt_t format = PTI_RGBA8;

		int fsize;
		char fullname[MAX_QPATH];
		qbyte *file;

		Q_snprintfz(fullname, sizeof(fullname), "%s", name);
		fsize = FS_LoadFile(fullname, (void **)&file);
		if (!file)
		{
			Q_snprintfz(fullname, sizeof(fullname), "pics/%s", name);
			fsize = FS_LoadFile(fullname, (void **)&file);
			if (!file)
				return NULL;
		}

		staticfilmimage = ReadRawImageFile(file, fsize, &imagewidth, &imageheight, &format, false, fullname);
		FS_FreeFile(file); //done with the file now
		if (!staticfilmimage)
		{
			Con_Printf("Static cinematic format not supported.\n");	//not supported format
			return NULL;
		}

		cin = Z_Malloc(sizeof(cin_t));
		cin->decodeframe = Media_Static_DecodeFrame;
		cin->shutdown = Media_Static_Shutdown;

		cin->image.data = staticfilmimage;
		cin->image.format = format;
		cin->image.width = imagewidth;
		cin->image.height = imageheight;

		return cin;
	}
	return NULL;
}
#endif

//Static Image Support
//////////////////////////////////////////////////////////////////////////////////
//Quake2 CIN Support

#ifdef Q2CLIENT
//fixme: this code really shouldn't be in this file.
static void Media_Cin_Shutdown(struct cin_s *cin)
{
	CIN_StopCinematic(cin->q2cin.cin);
}
static void Media_Cin_Rewind(struct cin_s *cin)
{
	CIN_Rewind(cin->q2cin.cin);
}

static qboolean Media_Cin_DecodeFrame(cin_t *cin, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ctx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ctx)
{
	qbyte *outdata;
	int outwidth;
	int outheight;
	qbyte *outpalette;

	switch (CIN_RunCinematic(cin->q2cin.cin, mediatime, &outdata, &outwidth, &outheight, &outpalette))
	{
	default:
	case 0:
		return false;
	case 1:
		break;
	case 2:
		if (!forcevideo)
			return true;
		break;
	}

	uploadtexture(ctx, TF_8PAL24, outwidth, outheight, outdata, outpalette);
	return true;
}

static cin_t *Media_Cin_TryLoad(char *name)
{
	struct cinematics_s *q2cin;
	cin_t *cin;
	char *dot = strrchr(name, '.');

	if (dot && (!strcmp(dot, ".cin")))
	{
		q2cin = CIN_PlayCinematic(name);
		if (q2cin)
		{
			cin = Z_Malloc(sizeof(cin_t));
			cin->q2cin.cin = q2cin;
			cin->decodeframe = Media_Cin_DecodeFrame;
			cin->shutdown = Media_Cin_Shutdown;
			cin->rewind = Media_Cin_Rewind;
			return cin;
		}
	}

	return NULL;
}
#endif

//Quake2 CIN Support
//////////////////////////////////////////////////////////////////////////////////

void Media_ShutdownCin(cin_t *cin)
{
	if (!cin)
		return;

	if (cin->shutdown)
		cin->shutdown(cin);

	if (TEXVALID(cin->texture))
		Image_UnloadTexture(cin->texture);

	if (cin->framedata)
	{
		BZ_Free(cin->framedata);
		cin->framedata = NULL;
	}

	Z_Free(cin);

	Z_Free(cin_prop_buf);
	cin_prop_buf = NULL;
	cin_prop_bufsize = 0;
}

cin_t *Media_StartCin(char *name)
{
	cin_t *cin = NULL;

	if (!name || !*name)	//clear only.
		return NULL;

#ifndef MINIMAL
	if (!cin)
		cin = Media_Static_TryLoad(name);
#endif
#ifdef Q2CLIENT
	if (!cin)
		cin = Media_Cin_TryLoad(name);
#endif
#ifdef Q3CLIENT
	if (!cin)
		cin = Media_RoQ_TryLoad(name);
#endif
#ifdef HAVE_API_VFW
	if (!cin)
		cin = Media_WinAvi_TryLoad(name);
#endif
#ifdef PLUGINS
	if (!cin)
		cin = Media_Plugin_TryLoad(name);
#endif
	if (cin)
		cin->filmstarttime = realtime;
	return cin;
}

static struct pendingfilms_s
{
	struct pendingfilms_s *next;
	char name[1];
} *pendingfilms;

static void MediaView_DrawFilm(menu_t *m)
{
	if (videoshader)
	{
		cin_t *cin = R_ShaderGetCinematic(videoshader);
		if (cin && cin->playstate == CINSTATE_INVALID)
			Media_SetState(cin, CINSTATE_PLAY);	//err... wot? must have vid_reloaded or something
		if (cin && cin->playstate == CINSTATE_ENDED)
			Media_StopFilm(false);
		else if (cin)
		{
			int cw, ch;
			float aspect;
			if (cin->cursormove)
				cin->cursormove(cin, mousecursor_x/(float)vid.width, mousecursor_y/(float)vid.height);
			if (cin->setsize)
				cin->setsize(cin, vid.pixelwidth, vid.pixelheight);

			//FIXME: should have a proper aspect ratio setting. RoQ files are always power of two, which makes things ugly.
			if (cin->getsize)
				cin->getsize(cin, &cw, &ch, &aspect);
			else
			{
				cw = 4;
				ch = 3;
			}

			R2D_Letterbox(0, 0, vid.fbvwidth, vid.fbvheight, videoshader, cw, ch);

			SCR_SetUpToDrawConsole();
			if  (scr_con_current)
				SCR_DrawConsole (false);
		}
	}
	else if (!videoshader)
		Menu_Unlink(m, true);
}
static qboolean MediaView_KeyEvent(menu_t *m, qboolean isdown, unsigned int devid, int key, int unicode)
{
	if (isdown && (key == K_ESCAPE || key == K_GP_GUIDE || key == K_GP_DIAMOND_CANCEL || key == K_TOUCHLONG))
	{
		Media_StopFilm(false);	//skip to the next.
		return true;
	}
	Media_Send_KeyEvent(NULL, key, unicode, !isdown);
	return true;
}
static qboolean MediaView_MouseMove(menu_t *m, qboolean isabs, unsigned int devid, float x, float y)
{
	cin_t *cin;
	cin = R_ShaderGetCinematic(videoshader);
	if (!cin || !cin->cursormove)
		return false;
	if (isabs)
		cin->cursormove(cin, x, y);
	return true;
}
static void MediaView_StopFilm(menu_t *m, qboolean forced)
{	//display is going away for some reason. might as well kill them all
	Media_StopFilm(true);
}

static qboolean Media_BeginNextFilm(void)
{
	cin_t *cin;
	char sname[MAX_QPATH];
	struct pendingfilms_s *p;

	if (!pendingfilms)
		return false;
	p = pendingfilms;
	pendingfilms = p->next;
	snprintf(sname, sizeof(sname), "cinematic/%s", p->name);

	if (!qrenderer)
	{
		Z_Free(p);
		return false;
	}

	videoshader = R_RegisterCustom(NULL, sname, SUF_NONE, Shader_DefaultCinematic, p->name);

	cin = R_ShaderGetCinematic(videoshader);
	if (cin)
	{
		Media_SetState(cin, CINSTATE_PLAY);
		Media_Send_Reset(cin);
		if (cin->changestream)
			cin->changestream(cin, "cmd:focus");
	}
	else
	{
		Con_Printf("Unable to play cinematic %s\n", p->name);
		R_UnloadShader(videoshader);
		videoshader = NULL;
	}
	Z_Free(p);

	if (videoshader)
	{
		videomenu.cursor = NULL;
		videomenu.release = MediaView_StopFilm;
		videomenu.drawmenu = MediaView_DrawFilm;
		videomenu.mousemove = MediaView_MouseMove;
		videomenu.keyevent = MediaView_KeyEvent;
		videomenu.isopaque = true;
		Menu_Push(&videomenu, false);
	}
	else
		Menu_Unlink(&videomenu, true);
	return !!videoshader;
}
qboolean Media_StopFilm(qboolean all)
{
	if (!pendingfilms && !videoshader)
		return false;	//nothing to do

	if (all)
	{
		struct pendingfilms_s *p;
		while(pendingfilms)
		{
			p = pendingfilms;
			pendingfilms = p->next;
			Z_Free(p);
		}
	}
	if (videoshader)
	{
		R_UnloadShader(videoshader);
		videoshader = NULL;

		S_RawAudio(SOURCEID_CINEMATIC, NULL, 0, 0, 0, 0, 0);
	}

	while (pendingfilms && !videoshader)
	{
		if (Media_BeginNextFilm())
			break;
	}

	//for q2 cinematic-maps.
	//q2 sends 'nextserver' when the client's cinematics end so that the game can progress to the next map.
	//qc might want to make use of it too, but its probably best to just send a playfilm command at the start of the map and then just wait it out. maybe treat nextserver as an unpause request.
	if (!videoshader && cls.state == ca_active)
	{
		CL_SendClientCommand(true, "nextserver %i", cl.servercount);
	}
	return true;
}
qboolean Media_PlayFilm(char *name, qboolean enqueue)
{
	if (!enqueue || !*name)
		Media_StopFilm(true);

	if (*name)
	{
		struct pendingfilms_s **p;
		for (p = &pendingfilms; *p; p = &(*p)->next)
			;
		(*p) = Z_Malloc(sizeof(**p) + strlen(name));
		strcpy((*p)->name, name);

		while (pendingfilms && !videoshader)
		{
			if (Media_BeginNextFilm())
				break;
		}
	}

	if (videoshader)
	{
#ifdef HAVE_CDPLAYER
		CDAudio_Stop();
#endif
		SCR_EndLoadingPlaque();

		if (!Key_Dest_Has(kdm_console))
			scr_con_current=0;
		return true;
	}
	else
		return false;
}

#ifndef SERVERONLY
void QDECL Media_UpdateTexture(void *ctx, uploadfmt_t fmt, int width, int height, void *data, void *palette)
{
	cin_t *cin = ctx;
	if (!TEXVALID(cin->texture))
		TEXASSIGN(cin->texture, Image_CreateTexture("***cin***", NULL, IF_CLAMP|IF_NOMIPMAP));
	Image_Upload(cin->texture, fmt, data, palette, width, height, 1, IF_CLAMP|IF_NOMIPMAP|IF_NOGAMMA);
}
texid_tf Media_UpdateForShader(cin_t *cin)
{
	float mediatime;
	qboolean force = false;
	if (!cin)	//caller fucked up.
		return r_nulltex;

	if (cin->playstate == CINSTATE_FLUSHED)
	{
		cin->playstate = CINSTATE_PLAY;
		mediatime = 0;
		cin->filmstarttime = realtime;
	}
	else if (cin->playstate == CINSTATE_INVALID)
	{
		cin->playstate = CINSTATE_LOOP;
		mediatime = 0;
		cin->filmstarttime = realtime;
	}
	else if (cin->playstate == CINSTATE_ENDED)
		mediatime = 0;	//if we reach the end of the video, we give up and just show the first frame for subsequent frames
	else if (cin->playstate == CINSTATE_PAUSE)
		mediatime = cin->filmstarttime;
	else
		mediatime = realtime - cin->filmstarttime;

	if (!TEXVALID(cin->texture))
	{
		force = true;
	}

	if (!cin->decodeframe(cin, false, force, mediatime, Media_UpdateTexture, cin))
	{
		//we got eof / error
		if (cin->playstate != CINSTATE_LOOP)
			Media_SetState(cin, CINSTATE_ENDED);
		if (cin->rewind)
		{
			Media_Send_Reset(cin);
			mediatime = 0;
			if (!cin->decodeframe(cin, true, force, mediatime, Media_UpdateTexture, cin))
				return r_nulltex;
		}
	}

	return cin->texture;
}
#endif

void QDECL Media_Send_KeyEvent(cin_t *cin, int button, int unicode, int event)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (!cin)
		return;
	if (cin->key)
		cin->key(cin, button, unicode, event);
	else if ((button == K_SPACE || button == K_GP_DIAMOND_ALTCONFIRM) && !event)
	{
		if (cin->playstate == CINSTATE_PAUSE)
			Media_SetState(cin, CINSTATE_PLAY);
		else
			Media_SetState(cin, CINSTATE_PAUSE);
	}
	else if (button == K_BACKSPACE && !event)
	{
		Media_Send_Reset(cin);
	}
	else if ((button == K_LEFTARROW || button == K_KP_LEFTARROW || button == K_GP_DPAD_LEFT) && !event)
		cin->filmstarttime += (cin->playstate == CINSTATE_PAUSE)?-10:10;
	else if ((button == K_RIGHTARROW || button == K_KP_RIGHTARROW || button == K_GP_DPAD_RIGHT) && !event)
		cin->filmstarttime -= (cin->playstate == CINSTATE_PAUSE)?-10:10;
}
void QDECL Media_Send_MouseMove(cin_t *cin, float x, float y)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (!cin || !cin->cursormove)
		return;
	cin->cursormove(cin, x, y);
}
void QDECL Media_Send_Resize(cin_t *cin, int x, int y)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (!cin || !cin->setsize)
		return;
	cin->setsize(cin, x, y);
}
void QDECL Media_Send_GetSize(cin_t *cin, int *x, int *y, float *aspect)
{
	*x = 0;
	*y = 0;
	*aspect = 0;
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (!cin || !cin->getsize)
		return;
	cin->getsize(cin, x, y, aspect);
}
void QDECL Media_Send_Reset(cin_t *cin)
{
	if (!cin)
		return;
	if (cin->playstate == CINSTATE_PAUSE)
		cin->filmstarttime = 0;	//fixed on resume
	else
		cin->filmstarttime = realtime;
	if (cin->rewind)
		cin->rewind(cin);
}
void QDECL Media_Send_Command(cin_t *cin, const char *command)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (cin && cin->changestream)
		cin->changestream(cin, command);
	else if (cin && !strcmp(command, "cmd:rewind"))
		Media_Send_Reset(cin);
}
const char *QDECL Media_Send_GetProperty(cin_t *cin, const char *key)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (cin && cin->getproperty)
	{
		size_t needsize = 0;
		if (cin->getproperty(cin, key, NULL, &needsize))
		{
			if (needsize+1 > cin_prop_bufsize)
			{
				cin_prop_buf = BZ_Realloc(cin_prop_buf, needsize+1);
				cin_prop_bufsize = needsize+1;
			}
			cin_prop_buf[needsize] = 0;
			if (cin->getproperty(cin, key, cin_prop_buf, &needsize))
				return cin_prop_buf;
		}
	}

	return NULL;
}
void Media_SetState(cin_t *cin, cinstates_t newstate)
{
	if (cin->playstate == newstate)
		return;

	if (newstate == CINSTATE_FLUSHED || newstate == CINSTATE_ENDED)
		cin->filmstarttime = realtime;	//these technically both rewind it.
	else if (newstate == CINSTATE_PAUSE)
		cin->filmstarttime = realtime - cin->filmstarttime;
	else if (cin->playstate == CINSTATE_PAUSE)
		cin->filmstarttime = realtime - cin->filmstarttime;

	cin->playstate = newstate;
}
cinstates_t Media_GetState(cin_t *cin)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (!cin)
		return CINSTATE_INVALID;
	return cin->playstate;
}

void Media_PlayFilm_f (void)
{
	int i;
	if (Cmd_Argc() < 2)
	{
		Con_Printf("playfilm <filename>");
	}
	if (!strcmp(Cmd_Argv(0), "cinematic"))
		Media_PlayFilm(va("video/%s", Cmd_Argv(1)), false);
	else
	{
		Media_PlayFilm(Cmd_Argv(1), false);
		for (i = 2; i < Cmd_Argc(); i++)
			Media_PlayFilm(Cmd_Argv(i), true);
	}
}

void Media_PlayVideoWindowed_f (void)
{
	char *videomap = Cmd_Argv(1);
	shader_t *s;
	console_t *con;
	if (!qrenderer)
		return;
	s = R_RegisterCustom(NULL, va("consolevid_%s", videomap), SUF_NONE, Shader_DefaultCinematic, videomap);
	if (!R_ShaderGetCinematic(s))
	{
		R_UnloadShader(s);
		Con_Printf("Unable to load video %s\n", videomap);
		return;
	}

	con = Con_Create(videomap, 0);
	if (!con)
		return;
	con->parseflags = PFS_FORCEUTF8;
	con->flags = CONF_ISWINDOW;
	con->wnd_x = vid.width/4;
	con->wnd_y = vid.height/4;
	con->wnd_w = vid.width/2;
	con->wnd_h = vid.height/2;
	con->linebuffered = NULL;

	Q_strncpyz(con->backimage, "", sizeof(con->backimage));
	if (con->backshader)
		R_UnloadShader(con->backshader);
	con->backshader = s;

	Con_SetActive(con);
}
#endif	//HAVE_MEDIA_DECODER














#ifdef HAVE_MEDIA_ENCODER

soundcardinfo_t *capture_fakesounddevice;

double captureframeinterval;	//interval between video frames
double capturelastvideotime;	//time of last video frame
int captureframe;
fbostate_t capturefbo;
int captureoldfbo;
qboolean capturingfbo;
texid_t	capturetexture;
qboolean captureframeforce;

#if defined(GLQUAKE) && !defined(GLESONLY)
#define GLQUAKE_PBOS
#endif
struct
{
#ifdef GLQUAKE_PBOS
	int pbo_handle;
#endif
	enum uploadfmt format;
	int stride;
	int width;
	int height;
} offscreen_queue[4];	//ringbuffer of offscreen_captureframe...captureframe
int offscreen_captureframe;
enum uploadfmt offscreen_format;

#define CAPTURECODECDESC_DEFAULT	"tga"
#ifdef HAVE_API_VFW
#define CAPTUREDRIVERDESC_AVI "avi: capture directly to avi (capturecodec should be a fourcc value).\n"
#define CAPTURECODECDESC_AVI "With (win)avi, this should be a fourcc code like divx or xvid, may be blank for raw video.\n"
#else
#define CAPTUREDRIVERDESC_AVI
#define CAPTURECODECDESC_AVI
#endif

qboolean capturepaused;
extern cvar_t vid_conautoscale;
cvar_t capturerate = CVARD("capturerate", "30", "The framerate of the video to capture");
cvar_t capturewidth = CVARD("capturedemowidth", "0", "When using capturedemo, this specifies the width of the FBO image used. Can be larger than your physical monitor.");
cvar_t captureheight = CVARD("capturedemoheight", "0", "When using capturedemo, this specifies the height of the FBO image used.");
cvar_t capturedriver = CVARD("capturedriver", "", "The driver to use to capture the demo. Use the capture command with no arguments for a list of drivers.");
cvar_t capturecodec = CVARD("capturecodec", CAPTURECODECDESC_DEFAULT, "the compression/encoding codec to use.\n"CAPTURECODECDESC_AVI"With raw capturing, this should be one of tga,png,jpg,pcx (ie: screenshot extensions).");
cvar_t capturesound = CVARD("capturesound", "1", "Enables the capturing of game voice. If not using capturedemo, this can be combined with cl_voip_test to capture your own voice.");
cvar_t capturesoundchannels = CVAR("capturesoundchannels", "2");
cvar_t capturesoundbits = CVAR("capturesoundbits", "16");
cvar_t capturemessage = CVAR("capturemessage", "");
cvar_t capturethrottlesize = CVARD("capturethrottlesize", "0", "If set, capturing will slow down significantly if there is less disk space available than this cvar (in mb). This is useful when recording a stream to (ram)disk while another program is simultaneously consuming frames.");
qboolean recordingdemo;

media_encoder_funcs_t *currentcapture_funcs;
void *currentcapture_ctx;


#if 1
/*screenshot capture*/
struct capture_raw_ctx
{
	int frames;
	enum fs_relative fsroot;
	char videonameprefix[MAX_QPATH];
	char videonameextension[16];
	vfsfile_t *audio;
};

qboolean FS_FixPath(char *path, size_t pathsize);
static void *QDECL capture_raw_begin (char *streamname, int videorate, int width, int height, int *sndkhz, int *sndchannels, int *sndbits)
{
	char filename[MAX_OSPATH];
	struct capture_raw_ctx *ctx = Z_Malloc(sizeof(*ctx));

	if (!strcmp(capturecodec.string, "png") || !strcmp(capturecodec.string, "jpeg") || !strcmp(capturecodec.string, "jpg") || !strcmp(capturecodec.string, "bmp") || !strcmp(capturecodec.string, "pcx") || !strcmp(capturecodec.string, "tga"))
		Q_strncpyz(ctx->videonameextension, capturecodec.string, sizeof(ctx->videonameextension));
	else
		Q_strncpyz(ctx->videonameextension, "tga", sizeof(ctx->videonameextension));

	if (!FS_SystemPath(va("%s", streamname), FS_GAMEONLY, ctx->videonameprefix, sizeof(ctx->videonameprefix)))
	{
		Z_Free(ctx);
		return NULL;
	}
	if (!FS_FixPath(ctx->videonameprefix, sizeof(ctx->videonameprefix)))
	{
		Z_Free(ctx);
		return NULL;
	}
	ctx->fsroot = FS_SYSTEM;

	FS_CreatePath(ctx->videonameprefix, ctx->fsroot);

	ctx->audio = NULL;
	if (*sndkhz)
	{
		if (*sndbits < 8)
			*sndbits = 8;
		if (*sndbits != 8)
			*sndbits = 16;
		if (*sndchannels > 6)
			*sndchannels = 6;
		if (*sndchannels < 1)
			*sndchannels = 1;
		Q_snprintfz(filename, sizeof(filename), "%saudio_%ichan_%ikhz_%ib.raw", ctx->videonameprefix, *sndchannels, *sndkhz/1000, *sndbits);
		ctx->audio = FS_OpenVFS(filename, "wb", ctx->fsroot);
	}
	if (!ctx->audio)
	{
		*sndkhz = 0;
		*sndchannels = 0;
		*sndbits = 0;
	}
	return ctx;
}
static void QDECL capture_raw_video (void *vctx, int frame, void *data, int stride, int width, int height, enum uploadfmt fmt)
{
	struct capture_raw_ctx *ctx = vctx;
	char filename[MAX_OSPATH];
	ctx->frames = frame+1;
	Q_snprintfz(filename, sizeof(filename), "%s%8.8i.%s", ctx->videonameprefix, frame, ctx->videonameextension);
	if (!SCR_ScreenShot(filename, ctx->fsroot, &data, 1, stride, width, height, fmt, true))
	{
		Sys_Sleep(1);
		if (!SCR_ScreenShot(filename, ctx->fsroot, &data, 1, stride, width, height, fmt, true))
			Con_DPrintf("Error writing frame %s\n", filename);
	}

	if (capturethrottlesize.value)
	{
		char base[MAX_QPATH];
		Q_strncpyz(base, ctx->videonameprefix, sizeof(base));
		if (FS_SystemPath(base, ctx->fsroot, filename, sizeof(filename)))
		{
			quint64_t diskfree = 0;
			if (Sys_GetFreeDiskSpace(filename, &diskfree))
			{
				Con_DLPrintf(2, "Free Space: %"PRIu64", threshhold %"PRIu64"\n", diskfree, (quint64_t)(1024*1024*capturethrottlesize.value));
				if (diskfree < (quint64_t)(1024*1024*capturethrottlesize.value))
					Sys_Sleep(1);	//throttle
			}
			else
			{
				Con_Printf("%s: unable to query free disk space for %s. Disabling\n", capturethrottlesize.name, filename);
				capturethrottlesize.ival = capturethrottlesize.value = 0;
			}
		}
	}
}
static void QDECL capture_raw_audio (void *vctx, void *data, int bytes)
{
	struct capture_raw_ctx *ctx = vctx;

	if (ctx->audio)
		VFS_WRITE(ctx->audio, data, bytes);
}
static void QDECL capture_raw_end (void *vctx)
{
	struct capture_raw_ctx *ctx = vctx;
	if (ctx->audio)
		VFS_CLOSE(ctx->audio);
	Con_Printf("%d video frames captured\n", ctx->frames);
	Z_Free(ctx);
}
static media_encoder_funcs_t capture_raw =
{
	sizeof(media_encoder_funcs_t),
	"raw",
	"Saves the video as a series of image files within the named sub directory, one for each frame. Audio is recorded as raw pcm.",
	NULL,
	capture_raw_begin,
	capture_raw_video,
	capture_raw_audio,
	capture_raw_end
};
#endif
#if defined(HAVE_API_VFW)

/*screenshot capture*/
struct capture_avi_ctx
{
	PAVIFILE file;
	#define avi_video_stream(ctx) (ctx->codec_fourcc?ctx->compressed_video_stream:ctx->uncompressed_video_stream)
	PAVISTREAM uncompressed_video_stream;
	PAVISTREAM compressed_video_stream;
	PAVISTREAM uncompressed_audio_stream;
	WAVEFORMATEX wave_format;
	unsigned long codec_fourcc;

	int audio_frame_counter;
};

static void QDECL capture_avi_end(void *vctx)
{
	struct capture_avi_ctx *ctx = vctx;

	if (ctx->uncompressed_video_stream)	qAVIStreamRelease(ctx->uncompressed_video_stream);
	if (ctx->compressed_video_stream)	qAVIStreamRelease(ctx->compressed_video_stream);
	if (ctx->uncompressed_audio_stream)	qAVIStreamRelease(ctx->uncompressed_audio_stream);
	if (ctx->file)						qAVIFileRelease(ctx->file);
	Z_Free(ctx);
}

static void *QDECL capture_avi_begin (char *streamname, int videorate, int width, int height, int *sndkhz, int *sndchannels, int *sndbits)
{
	struct capture_avi_ctx *ctx = Z_Malloc(sizeof(*ctx));
	HRESULT hr;
	BITMAPINFOHEADER bitmap_info_header;
	AVISTREAMINFOA stream_header;
	FILE *f;
	char aviname[256];
	char nativepath[256];

	char *fourcc = capturecodec.string;

	if (strlen(fourcc) == 4)
		ctx->codec_fourcc = mmioFOURCC(*(fourcc+0), *(fourcc+1), *(fourcc+2), *(fourcc+3));
	else
		ctx->codec_fourcc = 0;

	if (!qAVIStartup())
	{
		Con_Printf("vfw support not available.\n");
		capture_avi_end(ctx);
		return NULL;
	}

	/*convert to foo.avi*/
	COM_StripExtension(streamname, aviname, sizeof(aviname));
	COM_DefaultExtension (aviname, ".avi", sizeof(aviname));
	/*find the system location of that*/
	FS_SystemPath(aviname, FS_GAMEONLY, nativepath, sizeof(nativepath));

	//wipe it.
	f = fopen(nativepath, "rb");
	if (f)
	{
		fclose(f);
		unlink(nativepath);
	}

	hr = qAVIFileOpenA(&ctx->file, nativepath, OF_WRITE | OF_CREATE, NULL);
	if (FAILED(hr))
	{
		Con_Printf("Failed to open %s\n", nativepath);
		capture_avi_end(ctx);
		return NULL;
	}


	memset(&bitmap_info_header, 0, sizeof(BITMAPINFOHEADER));
	bitmap_info_header.biSize = 40;
	bitmap_info_header.biWidth = width;
	bitmap_info_header.biHeight = height;
	bitmap_info_header.biPlanes = 1;
	bitmap_info_header.biBitCount = 24;
	bitmap_info_header.biCompression = BI_RGB;
	bitmap_info_header.biSizeImage = width*height * 3;


	memset(&stream_header, 0, sizeof(stream_header));
	stream_header.fccType = streamtypeVIDEO;
	stream_header.fccHandler = ctx->codec_fourcc;
	stream_header.dwScale = 100;
	stream_header.dwRate = (unsigned long)(0.5 + 100.0/captureframeinterval);
	SetRect(&stream_header.rcFrame, 0, 0, width, height);

	hr = qAVIFileCreateStreamA(ctx->file, &ctx->uncompressed_video_stream, &stream_header);
	if (FAILED(hr))
	{
		Con_Printf("Couldn't initialise the stream, check codec\n");
		capture_avi_end(ctx);
		return NULL;
	}

	if (ctx->codec_fourcc)
	{
		AVICOMPRESSOPTIONS opts;
		memset(&opts, 0, sizeof(opts));
		opts.fccType = stream_header.fccType;
		opts.fccHandler = ctx->codec_fourcc;
		// Make the stream according to compression
		hr = qAVIMakeCompressedStream(&ctx->compressed_video_stream, ctx->uncompressed_video_stream, &opts, NULL);
		if (FAILED(hr))
		{
			Con_Printf("AVIMakeCompressedStream failed. check video codec.\n");
			capture_avi_end(ctx);
			return NULL;
		}
	}


	hr = qAVIStreamSetFormat(avi_video_stream(ctx), 0, &bitmap_info_header, sizeof(BITMAPINFOHEADER));
	if (FAILED(hr))
	{
		Con_Printf("AVIStreamSetFormat failed\n");
		capture_avi_end(ctx);
		return NULL;
	}

	if (*sndbits != 8 && *sndbits != 16)
		*sndbits = 8;
	if (*sndchannels < 1 && *sndchannels > 6)
		*sndchannels = 1;

	if (*sndkhz)
	{
		memset(&ctx->wave_format, 0, sizeof(WAVEFORMATEX));
		ctx->wave_format.wFormatTag = WAVE_FORMAT_PCM;
		ctx->wave_format.nChannels = *sndchannels;
		ctx->wave_format.nSamplesPerSec = *sndkhz;
		ctx->wave_format.wBitsPerSample = *sndbits;
		ctx->wave_format.nBlockAlign = ctx->wave_format.wBitsPerSample/8 * ctx->wave_format.nChannels;
		ctx->wave_format.nAvgBytesPerSec = ctx->wave_format.nSamplesPerSec * ctx->wave_format.nBlockAlign;
		ctx->wave_format.cbSize = 0;


		memset(&stream_header, 0, sizeof(stream_header));
		stream_header.fccType = streamtypeAUDIO;
		stream_header.dwScale = ctx->wave_format.nBlockAlign;
		stream_header.dwRate = stream_header.dwScale * (unsigned long)ctx->wave_format.nSamplesPerSec;
		stream_header.dwSampleSize = ctx->wave_format.nBlockAlign;

		//FIXME: be prepared to capture audio to mp3.

		hr = qAVIFileCreateStreamA(ctx->file, &ctx->uncompressed_audio_stream, &stream_header);
		if (FAILED(hr))
		{
			capture_avi_end(ctx);
			return NULL;
		}

		hr = qAVIStreamSetFormat(ctx->uncompressed_audio_stream, 0, &ctx->wave_format, sizeof(WAVEFORMATEX));
		if (FAILED(hr))
		{
			capture_avi_end(ctx);
			return NULL;
		}
	}
	return ctx;
}

static void QDECL capture_avi_video(void *vctx, int frame, void *vdata, int stride, int width, int height, enum uploadfmt fmt)
{
	//vfw api is bottom up.
	struct capture_avi_ctx *ctx = vctx;
	qbyte *data, *in, *out;
	int x, y;

	if (stride < 0)	//if stride is negative, then its bottom-up, but the data pointer is at the start of the buffer (rather than 'first row')
		vdata = (char*)vdata - stride*(height-1);

	//we need to output a packed bottom-up bgr image.

	//switch the input from logical-top-down to bottom-up (regardless of the physical ordering of its rows)
	in = (qbyte*)vdata + stride*(height-1);
	stride = -stride;

	if (fmt == TF_BGR24 && stride == width*-3)
	{	//no work needed!
		data = in;
	}
	else
	{
		int ipx = (fmt == TF_BGR24||fmt == TF_RGB24)?3:4;
		data = out = Hunk_TempAlloc(width*height*3);
		if (fmt == TF_RGB24 || fmt == TF_RGBX32 || fmt == TF_RGBA32)
		{	//byteswap + strip alpha
			for (y = height; y --> 0; out += width*3, in += stride)
			{
				for (x = 0; x < width; x++)
				{
					out[x*3+0] = in[x*ipx+2];
					out[x*3+1] = in[x*ipx+1];
					out[x*3+2] = in[x*ipx+0];
				}
			}
		}
		else if (fmt == TF_BGR24 || fmt == TF_BGRX32 || fmt == TF_BGRA32)
		{	//just strip alpha (or just flip)
			for (y = height; y --> 0; out += width*3, in += stride)
			{
				for (x = 0; x < width; x++)
				{
					out[x*3+0] = in[x*ipx+0];
					out[x*3+1] = in[x*ipx+1];
					out[x*3+2] = in[x*ipx+2];
				}
			}
		}
		else
		{	//probably spammy, but oh well
			Con_Printf("capture_avi_video: Unsupported image format\n");
			return;
		}
	}

	//FIXME: if we're allocating memory anyway, can we not push this to a thread?

	//write it
	if (FAILED(qAVIStreamWrite(avi_video_stream(ctx), frame, 1, data, width*height * 3, ((frame%15) == 0)?AVIIF_KEYFRAME:0, NULL, NULL)))
		Con_DPrintf("Recoring error\n");
}

static void QDECL capture_avi_audio(void *vctx, void *data, int bytes)
{
	struct capture_avi_ctx *ctx = vctx;
	if (ctx->uncompressed_audio_stream)
		qAVIStreamWrite(ctx->uncompressed_audio_stream, ctx->audio_frame_counter++, 1, data, bytes, AVIIF_KEYFRAME, NULL, NULL);
}

static media_encoder_funcs_t capture_avi =
{
	sizeof(media_encoder_funcs_t),
	"avi",
	"Uses Windows' VideoForWindows API to record avi files. Codecs are specified with a fourcc, for instance 'xvid' or 'x264', depending on which ones you have installed.",
	".avi",
	capture_avi_begin,
	capture_avi_video,
	capture_avi_audio,
	capture_avi_end
};
#endif

#ifdef _DEBUG
struct capture_null_context
{
	float starttime;
	int frames;
};
static void QDECL capture_null_end(void *vctx)
{
	struct capture_null_context *ctx = vctx;
	float duration = Sys_DoubleTime() - ctx->starttime;
	Con_Printf("%d video frames ignored, %g secs, %gfps\n", ctx->frames, duration, ctx->frames/duration);
	Z_Free(ctx);
}
static void *QDECL capture_null_begin (char *streamname, int videorate, int width, int height, int *sndkhz, int *sndchannels, int *sndbits)
{
	struct capture_null_context *ctx = Z_Malloc(sizeof(*ctx));
	*sndkhz = 11025;
	*sndchannels = 2;
	*sndbits = 32;	//floats!
	ctx->starttime = Sys_DoubleTime();
	return ctx;
}
static void QDECL capture_null_video(void *vctx, int frame, void *vdata, int stride, int width, int height, enum uploadfmt fmt)
{
	struct capture_null_context *ctx = vctx;
	ctx->frames = frame+1;
}
static void QDECL capture_null_audio(void *vctx, void *data, int bytes)
{
}
static media_encoder_funcs_t capture_null =
{
	sizeof(media_encoder_funcs_t),
	"null",
	"An encoder that doesn't actually encode or write anything, for debugging.",
	".null",
	capture_null_begin,
	capture_null_video,
	capture_null_audio,
	capture_null_end
};
#endif

static media_encoder_funcs_t *pluginencodersfunc[8];
static struct plugin_s *pluginencodersplugin[8];
qboolean Media_RegisterEncoder(struct plugin_s *plug, media_encoder_funcs_t *funcs)
{
	int i;
	if (!funcs || funcs->structsize != sizeof(*funcs))
		return false;
	for (i = 0; i < sizeof(pluginencodersfunc)/sizeof(pluginencodersfunc[0]); i++)
	{
		if (pluginencodersfunc[i] == NULL)
		{
			pluginencodersfunc[i] = funcs;
			pluginencodersplugin[i] = plug;
			return true;
		}
	}
	return false;
}
void Media_StopRecordFilm_f(void);
/*funcs==null closes ALL decoders from this plugin*/
qboolean Media_UnregisterEncoder(struct plugin_s *plug, media_encoder_funcs_t *funcs)
{
	qboolean success = false;
	int i;

	for (i = 0; i < sizeof(pluginencodersfunc)/sizeof(pluginencodersfunc[0]); i++)
	{
		if (pluginencodersplugin[i])
		if (pluginencodersfunc[i] == funcs || (!funcs && pluginencodersplugin[i] == plug))
		{
			if (currentcapture_funcs == pluginencodersfunc[i])
				Media_StopRecordFilm_f();
			success = true;
			pluginencodersfunc[i] = NULL;
			pluginencodersplugin[i] = NULL;
			if (funcs)
				return success;
		}
	}
	return success;
}
 
//returns 0 if not capturing. 1 if capturing live. 2 if capturing a demo (where frame timings are forced).
int Media_Capturing (void)
{
	if (!currentcapture_funcs)
		return 0;
	return captureframeforce?2:1;
}

void Media_CapturePause_f (void)
{
	capturepaused = !capturepaused;
}

qboolean Media_PausedDemo (qboolean fortiming)
{
	//if fortiming is set, then timing might need to advance if we still need to parse the demo to get the first valid data out of it.

	if (capturepaused)
		return true;

	//capturedemo doesn't record any frames when the console is visible
	//but that's okay, as we don't load any demo frames either.
	if ((cls.demoplayback && Media_Capturing()))
		if (Key_Dest_Has(~kdm_game) || scr_con_current > 0 || (!fortiming&&!cl.validsequence))
			return true;

	return false;
}

static qboolean Media_ForceTimeInterval(void)
{
	return (cls.demoplayback && Media_Capturing() && captureframeinterval>0 && !Media_PausedDemo(false));
}

double Media_TweekCaptureFrameTime(double oldtime, double time)
{
	if (Media_ForceTimeInterval())
	{
		captureframeforce = true;
		//if we're forcing time intervals, then we use fixed time increments and generate a new video frame for every single frame.
		return capturelastvideotime;
	}
	return oldtime + time;
}

#ifdef VKQUAKE
static void Media_CapturedFrame (void *data, qintptr_t bytestride, size_t width, size_t height, enum uploadfmt fmt)
{
	if (currentcapture_funcs)
		currentcapture_funcs->capture_video(currentcapture_ctx, offscreen_captureframe, data, bytestride, width, height, fmt);
	offscreen_captureframe++;
}
#endif

void Media_RecordFrame (void)
{
	char *buffer;
	int bytestride, truewidth, trueheight;
	enum uploadfmt fmt;

	if (!currentcapture_funcs)
		return;

/*	if (*capturecutoff.string && captureframe * captureframeinterval > capturecutoff.value*60)
	{
		currentcapture_funcs->capture_end(currentcapture_ctx);
		currentcapture_ctx = currentcapture_funcs->capture_begin(Cmd_Argv(1), capturerate.value, vid.pixelwidth, vid.pixelheight, &sndkhz, &sndchannels, &sndbits);
		if (!currentcapture_ctx)
		{
			currentcapture_funcs = NULL;
			return;
		}
		captureframe = 0;
	}
*/
	if (Media_PausedDemo(false))
	{
		int y = vid.height -32-16;
		if (y < scr_con_current) y = scr_con_current;
		if (y > vid.height-8)
			y = vid.height-8;

#ifdef GLQUAKE
		if (capturingfbo && qrenderer == QR_OPENGL)
		{
			shader_t *pic;
			GLBE_FBO_Pop(captureoldfbo);
			vid.framebuffer = NULL;
			GL_Set2D(false);

			pic = R_RegisterShader("capturdemofeedback", SUF_NONE,
						"{\n"
							"program default2d\n"
							"{\n"
								"map $diffuse\n"
							"}\n"
						"}\n");
			pic->defaulttextures->base = capturetexture;
			//pulse green slightly, so its a bit more obvious
			R2D_ImageColours(1, 1+0.2*sin(realtime), 1, 1);
			R2D_Image(0, 0, vid.width, vid.height, 0, 1, 1, 0, pic);
			R2D_ImageColours(1, 1, 1, 1);
			Draw_FunString(0, 0, S_COLOR_RED"PAUSED");
			if (R2D_Flush)
				R2D_Flush();

			captureoldfbo = GLBE_FBO_Push(&capturefbo);
			vid.framebuffer = capturetexture;
			GL_Set2D(false);
		}
		else
#endif
			Draw_FunString((strlen(capturemessage.string)+1)*8, y, S_COLOR_RED "PAUSED");

		if (captureframeforce)
			capturelastvideotime += captureframeinterval;
		return;
	}

	//don't capture frames while we're loading.
	if (cl.sendprespawn || (cls.state < ca_active && COM_HasWork()))
	{
		capturelastvideotime += captureframeinterval;
		return;
	}

//overlay this on the screen, so it appears in the film
	if (*capturemessage.string)
	{
		int y = vid.height -32-16;
		if (y < scr_con_current) y = scr_con_current;
		if (y > vid.height-8)
			y = vid.height-8;
		Draw_FunString(0, y, capturemessage.string);
	}

	//time for another frame?
	if (!captureframeforce)
	{
		if (capturelastvideotime > realtime+1)
			capturelastvideotime = realtime;	//urm, wrapped?..
		if (capturelastvideotime > realtime)
			goto skipframe;
	}
	
	if (cls.findtrack)
	{
		capturelastvideotime += captureframeinterval;
		return;	//skip until we're tracking the right player.
	}

	if (R2D_Flush)
		R2D_Flush();

#ifdef GLQUAKE_PBOS
	if (offscreen_format != TF_INVALID && qrenderer == QR_OPENGL)
	{
		int frame;
		//encode the frame if we're about to stomp on it
		while (offscreen_captureframe + countof(offscreen_queue) <= captureframe)
		{
			frame = offscreen_captureframe%countof(offscreen_queue);
			qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, offscreen_queue[frame].pbo_handle);
			buffer = qglMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
			if (buffer)
			{
				//FIXME: thread these (with audio too, to avoid races)
				currentcapture_funcs->capture_video(currentcapture_ctx, offscreen_captureframe, buffer, offscreen_queue[frame].stride, offscreen_queue[frame].width, offscreen_queue[frame].height, offscreen_queue[frame].format);
				qglUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
			}
			offscreen_captureframe++;
		}

		frame = captureframe%countof(offscreen_queue);
		//if we have no pbo yet, create one.
		if (!offscreen_queue[frame].pbo_handle || offscreen_queue[frame].width != vid.fbpwidth || offscreen_queue[frame].height != vid.fbpheight)
		{
			int imagesize = 0;
			if (offscreen_queue[frame].pbo_handle)
				qglDeleteBuffersARB(1, &offscreen_queue[frame].pbo_handle);
			offscreen_queue[frame].format = offscreen_format;
			offscreen_queue[frame].width = vid.fbpwidth;
			offscreen_queue[frame].height = vid.fbpheight;
			switch(offscreen_queue[frame].format)
			{
			case TF_BGR24:
			case TF_RGB24:
				imagesize = 3;
				break;
			case TF_BGRA32:
			case TF_RGBA32:
				imagesize = 4;
				break;
			default:
				break;
			}

			offscreen_queue[frame].stride = vid.fbpwidth*-imagesize;//gl is upside down

			imagesize *= offscreen_queue[frame].width * offscreen_queue[frame].height;

			qglGenBuffersARB(1, &offscreen_queue[frame].pbo_handle);
			qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, offscreen_queue[frame].pbo_handle);
			qglBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, imagesize, NULL, GL_STREAM_READ_ARB);
		}

		//get the gpu to copy the texture into the pbo. the driver should pipeline this read until we actually map the pbo, hopefully avoiding stalls
		qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, offscreen_queue[frame].pbo_handle);
		switch(offscreen_queue[frame].format)
		{
		case TF_BGR24:
			qglReadPixels(0, 0, offscreen_queue[frame].width, offscreen_queue[frame].height, GL_BGR_EXT, GL_UNSIGNED_BYTE, 0);
			break;
		case TF_BGRA32:
			qglReadPixels(0, 0, offscreen_queue[frame].width, offscreen_queue[frame].height, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
			break;
		case TF_RGB24:
			qglReadPixels(0, 0, offscreen_queue[frame].width, offscreen_queue[frame].height, GL_RGB, GL_UNSIGNED_BYTE, 0);
			break;
		case TF_RGBA32:
			qglReadPixels(0, 0, offscreen_queue[frame].width, offscreen_queue[frame].height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			break;
		default:
			break;
		}
		qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	}
	else
#endif
#ifdef VKQUAKE
	if (qrenderer == QR_VULKAN)
		VKVID_QueueGetRGBData(Media_CapturedFrame);
	else
#endif
	{
		offscreen_captureframe = captureframe+1;
		//submit the current video frame. audio will be mixed to match.
		buffer = VID_GetRGBInfo(&bytestride, &truewidth, &trueheight, &fmt);
		if (buffer)
		{
			qbyte *firstrow = (bytestride<0)?buffer - bytestride*(trueheight-1):buffer;
			currentcapture_funcs->capture_video(currentcapture_ctx, captureframe, firstrow, bytestride, truewidth, trueheight, fmt);
			BZ_Free (buffer);
		}
		else
		{
			Con_DPrintf("Unable to grab video image\n");
			currentcapture_funcs->capture_video(currentcapture_ctx, captureframe, NULL, 0, 0, 0, TF_INVALID);
		}
	}
	captureframe++;
	capturelastvideotime += captureframeinterval;

	captureframeforce = false;

	//this is drawn to the screen and not the film
skipframe:
{
	int y = vid.height -32-16;
	if (y < scr_con_current) y = scr_con_current;
	if (y > vid.height-8)
		y = vid.height-8;

#ifdef GLQUAKE
	if (capturingfbo && qrenderer == QR_OPENGL)
	{
		shader_t *pic;
		GLBE_FBO_Pop(captureoldfbo);
		vid.framebuffer = NULL;
		GL_Set2D(false);

		pic = R_RegisterShader("capturdemofeedback", SUF_NONE,
					"{\n"
						"program default2d\n"
						"{\n"
							"map $diffuse\n"
						"}\n"
					"}\n");
		pic->defaulttextures->base = capturetexture;
		//pulse green slightly, so its a bit more obvious
		R2D_ImageColours(1, 1+0.2*sin(realtime), 1, 1);
		R2D_Image(0, 0, vid.width, vid.height, 0, 1, 1, 0, pic);
		R2D_ImageColours(1, 1, 1, 1);
		Draw_FunString(0, 0, va(S_COLOR_RED"RECORDING OFFSCREEN %g", capturelastvideotime));
		if (R2D_Flush)
			R2D_Flush();

		captureoldfbo = GLBE_FBO_Push(&capturefbo);
		vid.framebuffer = capturetexture;
		GL_Set2D(false);
	}
	else
#endif
		Draw_FunString((strlen(capturemessage.string)+1)*8, y, S_COLOR_RED"RECORDING");
}
}

static void *MSD_Lock (soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}
static void MSD_Unlock (soundcardinfo_t *sc, void *buffer)
{
}

static unsigned int MSD_GetDMAPos(soundcardinfo_t *sc)
{
	int		s;

	s = captureframe*(sc->sn.speed*captureframeinterval);


//	s >>= sc->sn.samplebytes - 1;
	s *= sc->sn.numchannels;
	return s;
}

static void MSD_Submit(soundcardinfo_t *sc, int start, int end)
{
	//Fixme: support outputting to wav
	//http://www.borg.com/~jglatt/tech/wave.htm


	int lastpos;
	int newpos;
	int framestosubmit;
	int offset;
	int bytesperframe;
	int minframes = 576;	//mp3 requires this (which we use for vfw)
	int maxframes = 576;

	newpos = sc->paintedtime;

	while(1)
	{
		lastpos = sc->snd_completed;
		framestosubmit = newpos - lastpos;
		if (framestosubmit < minframes)
			return;
		if (framestosubmit > maxframes)
			framestosubmit = maxframes;

		bytesperframe = sc->sn.numchannels*sc->sn.samplebytes;

		offset = (lastpos % (sc->sn.samples/sc->sn.numchannels));

		//we could just use a buffer size equal to the number of samples in each frame
		//but that isn't as robust when it comes to floating point imprecisions
		//namly: that it would loose a sample each frame with most framerates.

		if ((sc->snd_completed % (sc->sn.samples/sc->sn.numchannels)) < offset)
		{
			framestosubmit = ((sc->sn.samples/sc->sn.numchannels)) - offset;
			offset = 0;
		}
		currentcapture_funcs->capture_audio(currentcapture_ctx, sc->sn.buffer+offset*bytesperframe, framestosubmit*bytesperframe);
		sc->snd_completed += framestosubmit;
	}
}

static void MSD_Shutdown (soundcardinfo_t *sc)
{
	Z_Free(sc->sn.buffer);
	capture_fakesounddevice = NULL;
}

void Media_InitFakeSoundDevice (int speed, int channels, int samplebits)
{
	soundcardinfo_t *sc;

	if (capture_fakesounddevice)
		return;

	//when we're recording a demo, we'll be timedemoing it as it were.
	//this means that the actual sound devices and the fake device will be going at different rates
	//which really confuses any stream decoding, like music.
	//so just kill all actual sound devices.
	if (recordingdemo)
	{
		soundcardinfo_t *next;
		for (sc = sndcardinfo; sc; sc=next)
		{
			next = sc->next;
			sc->Shutdown(sc);
			Z_Free(sc);
			sndcardinfo = next;
		}
	}

	if (!snd_speed)
		snd_speed = speed;

	sc = Z_Malloc(sizeof(soundcardinfo_t));

	sc->seat = -1;
	sc->snd_sent = 0;
	sc->snd_completed = 0;

	sc->sn.samples = speed*0.5;
	sc->sn.speed = speed;
	switch(samplebits)
	{
	case 32:
		sc->sn.samplebytes = 4;
		sc->sn.sampleformat = QSF_F32;
		break;
	default:
	case 16:
		sc->sn.samplebytes = 2;
		sc->sn.sampleformat = QSF_S16;
		break;
	case 8:
		sc->sn.samplebytes = 1;
		sc->sn.sampleformat = QSF_U8;
		break;
	}
	sc->sn.samplepos = 0;
	sc->sn.numchannels = channels;
	sc->inactive_sound = true;

	sc->sn.samples -= sc->sn.samples%1152;	//truncate slightly to keep vfw happy.
	sc->samplequeue = -1;

	sc->sn.buffer = (unsigned char *) BZ_Malloc(sc->sn.samples*sc->sn.numchannels*sc->sn.samplebytes);

	Z_ReallocElements((void**)&sc->channel, &sc->max_chans, NUM_AMBIENTS+NUM_MUSICS, sizeof(*sc->channel));

	sc->Lock		= MSD_Lock;
	sc->Unlock		= MSD_Unlock;
	sc->Submit		= MSD_Submit;
	sc->Shutdown	= MSD_Shutdown;
	sc->GetDMAPos	= MSD_GetDMAPos;

	sc->next = sndcardinfo;
	sndcardinfo = sc;

	capture_fakesounddevice = sc;

	S_DefaultSpeakerConfiguration(sc);
}

//stops capturing and destroys everything.
static void Media_FlushCapture(void)
{
#ifdef GLQUAKE_PBOS
	if (offscreen_format && qrenderer == QR_OPENGL)
	{
		int frame;
		qbyte *buffer;
		while (offscreen_captureframe < captureframe)
		{
			frame = offscreen_captureframe%countof(offscreen_queue);
			qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, offscreen_queue[frame].pbo_handle);
			buffer = qglMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
			if (buffer)
			{
				currentcapture_funcs->capture_video(currentcapture_ctx, offscreen_captureframe, buffer, offscreen_queue[frame].stride, offscreen_queue[frame].width, offscreen_queue[frame].height, offscreen_queue[frame].format);
				qglUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
			}
			offscreen_captureframe++;
		}

		for (frame = 0; frame < countof(offscreen_queue); frame++)
		{
			if (offscreen_queue[frame].pbo_handle)
				qglDeleteBuffersARB(1, &offscreen_queue[frame].pbo_handle);
			memset(&offscreen_queue[frame], 0, sizeof(offscreen_queue[frame]));
		}
	}
#endif
#if 0//def VKQUAKE
	if (pbo_format && qrenderer == QR_VULKAN)
	{
		int frame;
		while (offscreen_captureframe < captureframe)
		{
			frame = offscreen_captureframe%countof(offscreen_queue);
			qbyte *buffer;
			qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, offscreen_queue[frame].pbo_handle);
			buffer = qglMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
			if (buffer)
			{
				currentcapture_funcs->capture_video(currentcapture_ctx, buffer, offscreen_captureframe, offscreen_queue[frame].width, offscreen_queue[frame].height, offscreen_queue[frame].format);
				qglUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
			}
			offscreen_captureframe++;
		}

		for (frame = 0; frame < countof(offscreen_queue); frame++)
		{
			if (offscreen_queue[frame].pbo_handle)
				qglDeleteBuffersARB(1, &offscreen_queue[frame].pbo_handle);
			memset(&offscreen_queue[frame], 0, sizeof(offscreen_queue[frame]));
		}
	}
#endif

#ifdef GLQUAKE
	if (capturingfbo && qrenderer == QR_OPENGL)
	{
		GLBE_FBO_Pop(captureoldfbo);
		GLBE_FBO_Destroy(&capturefbo);
		vid.framebuffer = NULL;
	}
#endif
}

void Media_StopRecordFilm_f (void)
{
	Media_FlushCapture();
	capturingfbo = false;

	if (capture_fakesounddevice)
		S_ShutdownCard(capture_fakesounddevice);
	capture_fakesounddevice = NULL;

	if (recordingdemo)	//start up their regular audio devices again.
		S_DoRestart(false);

	recordingdemo=false;

	if (currentcapture_funcs)
		currentcapture_funcs->capture_end(currentcapture_ctx);
	currentcapture_ctx = NULL;
	currentcapture_funcs = NULL;

	Cvar_ForceCallback(&vid_conautoscale);
}
void Media_VideoRestarting(void)
{
	Media_FlushCapture();
}
void Media_VideoRestarted(void)
{
#ifdef GLQUAKE
	if (capturingfbo && qrenderer == QR_OPENGL && gl_config.ext_framebuffer_objects)
	{	//restore it how it was, if we can.
		int w = capturefbo.rb_size[0], h = capturefbo.rb_size[1];
		capturingfbo = true;
		capturetexture = R2D_RT_Configure("$democapture", w, h, TF_BGRA32, RT_IMAGEFLAGS);
		captureoldfbo = GLBE_FBO_Update(&capturefbo, FBO_RB_DEPTH|(Sh_StencilShadowsActive()?FBO_RB_STENCIL:0), &capturetexture, 1, r_nulltex, capturetexture->width, capturetexture->height, 0);
		vid.fbpwidth = capturetexture->width;
		vid.fbpheight = capturetexture->height;
		vid.framebuffer = capturetexture;
	}
	else
#endif
		capturingfbo = false;

	if (capturingfbo)
		Cvar_ForceCallback(&vid_conautoscale);
}
static void Media_RecordFilm (char *recordingname, qboolean demo)
{
	int sndkhz, sndchannels, sndbits;
	int i;

	Media_StopRecordFilm_f();

	if (capturerate.value<=0)
	{
		Con_Printf("Invalid capturerate\n");
		capturerate.value = 15;
	}

	captureframeinterval = 1/capturerate.value;
	if (captureframeinterval < 0.001)
		captureframeinterval = 0.001;	//no more than 1000 images per second.
	capturelastvideotime = realtime = 0;

	Con_ClearNotify();

	captureframe = offscreen_captureframe = 0;
	for (i = 0; i < sizeof(pluginencodersfunc)/sizeof(pluginencodersfunc[0]); i++)
	{
		if (pluginencodersfunc[i])
			if (!strcmp(pluginencodersfunc[i]->drivername, capturedriver.string))
				currentcapture_funcs = pluginencodersfunc[i];
	}
	if (!currentcapture_funcs)
	{	//try to find one based upon the explicit extension given.
		char captext[8];
		const char *outext = COM_GetFileExtension(recordingname, NULL);

		for (i = 0; i < countof(pluginencodersfunc); i++)
		{
			if (pluginencodersfunc[i] && pluginencodersfunc[i]->extensions)
			{
				const char *t = pluginencodersfunc[i]->extensions;
				while (*t)
				{
					t = COM_ParseStringSetSep (t, ';', captext, sizeof(captext));
					if (wildcmp(captext, outext))
					{	//matches the wildcard...
						currentcapture_funcs = pluginencodersfunc[i];
						break;
					}
				}
			}
		}
	}
	if (!currentcapture_funcs)
	{	//otherwise just find the first valid one that's in a plugin.
		for (i = 0; i < countof(pluginencodersfunc); i++)
		{
			if (pluginencodersfunc[i] && pluginencodersfunc[i]->extensions && pluginencodersplugin[i])
				currentcapture_funcs = pluginencodersfunc[i];
		}
	}
	if (!currentcapture_funcs)
	{	//otherwise just find the first valid one that's from anywhere...
		for (i = 0; i < countof(pluginencodersfunc); i++)
		{
			if (pluginencodersfunc[i] && pluginencodersfunc[i]->extensions)
				currentcapture_funcs = pluginencodersfunc[i];
		}
	}
	if (capturesound.ival && !nosound.ival)
	{
		sndkhz = snd_speed?snd_speed:48000;
		sndchannels = capturesoundchannels.ival;
		sndbits = capturesoundbits.ival;
	}
	else
	{
		sndkhz = 0;
		sndchannels = 0;
		sndbits = 0;
	}

	vid.fbpwidth = vid.pixelwidth;
	vid.fbpheight = vid.pixelheight;
	if (demo && capturewidth.ival && captureheight.ival)
	{
#ifdef GLQUAKE
		if (qrenderer == QR_OPENGL && gl_config.ext_framebuffer_objects)
		{
			capturingfbo = true;
			capturetexture = R2D_RT_Configure("$democapture", capturewidth.ival, captureheight.ival, TF_BGRA32, RT_IMAGEFLAGS);
			captureoldfbo = GLBE_FBO_Update(&capturefbo, FBO_RB_DEPTH|(Sh_StencilShadowsActive()?FBO_RB_STENCIL:0), &capturetexture, 1, r_nulltex, capturewidth.ival, captureheight.ival, 0);
			vid.fbpwidth = capturetexture->width;
			vid.fbpheight = capturetexture->height;
			vid.framebuffer = capturetexture;
		}
#endif
	}

	offscreen_format = TF_INVALID;
#ifdef GLQUAKE_PBOS
	if (qrenderer == QR_OPENGL && !gl_config.gles && gl_config.glversion >= 2.1)
	{	//both tgas and vfw favour bgr24, so lets get the gl drivers to suffer instead of us, where possible.
		if (vid.fbpwidth & 3)
			offscreen_format = TF_BGRA32;	//don't bother changing pack alignment, just use something that is guarenteed to not need anything.
		else
			offscreen_format = TF_BGR24;
	}
#endif
#ifdef VKQUAKE
//	if (qrenderer == QR_VULKAN)
//		offscreen_format = TF_BGRA32;	//use the native format, the driver won't do byteswapping for us.
#endif

	recordingdemo = demo;
	
	if (!currentcapture_funcs->capture_begin)
		currentcapture_ctx = NULL;
	else
	{
		char fname[MAX_QPATH];
		char ext[8];
		COM_FileExtension(recordingname, ext, sizeof(ext));
		if (!strcmp(ext, "dem") || !strcmp(ext, "qwd") || !strcmp(ext, "mvd") || !strcmp(ext, "dm2") || !strcmp(ext, "gz") || !strcmp(ext, "xz") || !*ext)
		{	//extensions are evil, but whatever.
			//make sure there is actually an extension there, and don't break if they try overwriting a known demo format...
			COM_StripExtension(recordingname, fname, sizeof(fname));
			if (currentcapture_funcs->extensions)
			{
				COM_ParseStringSetSep (currentcapture_funcs->extensions, ';', ext, sizeof(ext));
				Q_strncatz(fname, ext, sizeof(fname));
			}
			recordingname = fname;
		}

		currentcapture_ctx = currentcapture_funcs->capture_begin(recordingname, capturerate.value, vid.fbpwidth, vid.fbpheight, &sndkhz, &sndchannels, &sndbits);
	}
	if (!currentcapture_ctx)
	{
		recordingdemo = false;
		currentcapture_funcs = NULL;
		Con_Printf("Unable to initialise capture driver\n");
		Media_StopRecordFilm_f();
	}
	else if (sndkhz)
		Media_InitFakeSoundDevice(sndkhz, sndchannels, sndbits);

	Cvar_ForceCallback(&vid_conautoscale);
}
static void Media_RecordFilm_f (void)
{
	if (Cmd_Argc() != 2)
	{
		int i;
		Con_Printf("capture <filename>\nRecords video output in an avi file.\nUse capturerate and capturecodec to configure.\n\n");
		for (i = 0; i < countof(pluginencodersfunc); i++)
		{
			if (pluginencodersfunc[i])
				Con_Printf("%s%s^7: %s\n", !strcmp(pluginencodersfunc[i]->drivername, capturedriver.string)?"^2":"^3", pluginencodersfunc[i]->drivername, pluginencodersfunc[i]->description);
		}
		Con_Printf("\n");

		Con_Printf("Current capture settings:\n");
		Con_Printf(" ^[/capturedriver %s^]\n", capturedriver.string);
		Con_Printf(" ^[/capturecodec %s^]\n", capturecodec.string);
		Con_Printf(" ^[/capturedemowidth %s^]\n", capturewidth.string);
		Con_Printf(" ^[/capturedemoheight %s^]\n", captureheight.string);
		Con_Printf(" ^[/capturerate %s^]\n", capturerate.string);
		Con_Printf(" ^[/capturesound %s^]\n", capturesound.string);
		if (capturesound.value)
		{
			Con_Printf(" ^[/capturesoundchannels %s^]\n", capturesoundchannels.string);
			Con_Printf(" ^[/capturesoundbits %s^]\n", capturesoundbits.string);
		}
		return;
	}
	if (Cmd_IsInsecure())	//err... don't think so sonny.
		return;

	Media_RecordFilm(Cmd_Argv(1), false);
}
void Media_CaptureDemoEnd(void)
{
	if (recordingdemo)
		Media_StopRecordFilm_f();
}
void CL_PlayDemo(char *demoname, qboolean usesystempath);
void Media_RecordDemo_f(void)
{
	char *demoname = Cmd_Argv(1);
	char *videoname = Cmd_Argv(2);

	if (Cmd_FromGamecode())
		return;

	if (Cmd_Argc()<=1)
	{
		Con_Printf(	"capturedemo demoname outputname\n"
					"captures a demo to video frames using offline rendering for smoother results\n"
					"see also: apropos capture\n");
		return;
	}

	if (!Renderer_Started() && !isDedicated)
	{
		Cbuf_AddText(va("wait;%s %s\n", Cmd_Argv(0), Cmd_Args()), Cmd_ExecLevel);
		return;
	}

	if (Cmd_Argc()<=2)
		videoname = demoname;

	CL_Stopdemo_f();	//capturing failed for some reason

	CL_PlayDemo(demoname, false);
	if (!cls.demoplayback)
	{
		Con_Printf("unable to play demo, not capturing\n");
		return;
	}
	//FIXME: make sure it loaded okay
	Media_RecordFilm(videoname, true);
	scr_con_current=0;

	Menu_PopAll();
	Key_Dest_Remove(kdm_console);

	if (!currentcapture_funcs)
		CL_Stopdemo_f();	//capturing failed for some reason
}
#else

void Media_CaptureDemoEnd(void) {}
qboolean Media_PausedDemo(qboolean fortiming) {return false;}
int Media_Capturing (void) { return 0; }
double Media_TweekCaptureFrameTime(double oldtime, double time) { return oldtime+time ; }
void Media_RecordFrame (void) {}
void Media_VideoRestarting(void) {}
void Media_VideoRestarted(void) {}

#endif





#ifdef HAVE_SPEECHTOTEXT
typedef struct ISpNotifySink ISpNotifySink;
typedef void *ISpNotifyCallback;
typedef void __stdcall SPNOTIFYCALLBACK(WPARAM wParam, LPARAM lParam);
typedef struct SPEVENT
{
    WORD        eEventId : 16;
    WORD  elParamType : 16;
    ULONG       ulStreamNum;
    ULONGLONG   ullAudioStreamOffset;
    WPARAM      wParam;
    LPARAM      lParam;
} SPEVENT;

#define SPEVENTSOURCEINFO void
#define ISpObjectToken void
#define ISpStreamFormat void
#define SPVOICESTATUS void
#define SPVPRIORITY int
#define SPEVENTENUM int

typedef struct ISpVoice ISpVoice;
typedef struct ISpVoiceVtbl
{
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
        ISpVoice * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
        ISpVoice * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
        ISpVoice * This);

    HRESULT ( STDMETHODCALLTYPE *SetNotifySink )(
        ISpVoice * This,
        /* [in] */ ISpNotifySink *pNotifySink);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyWindowMessage )(
        ISpVoice * This,
        /* [in] */ HWND hWnd,
        /* [in] */ UINT Msg,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyCallbackFunction )(
        ISpVoice * This,
        /* [in] */ SPNOTIFYCALLBACK *pfnCallback,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyCallbackInterface )(
        ISpVoice * This,
        /* [in] */ ISpNotifyCallback *pSpCallback,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyWin32Event )(
        ISpVoice * This);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *WaitForNotifyEvent )(
        ISpVoice * This,
        /* [in] */ DWORD dwMilliseconds);

    /* [local] */ HANDLE ( STDMETHODCALLTYPE *GetNotifyEventHandle )(
        ISpVoice * This);

    HRESULT ( STDMETHODCALLTYPE *SetInterest )(
        ISpVoice * This,
        /* [in] */ ULONGLONG ullEventInterest,
        /* [in] */ ULONGLONG ullQueuedInterest);

    HRESULT ( STDMETHODCALLTYPE *GetEvents )(
        ISpVoice * This,
        /* [in] */ ULONG ulCount,
        /* [size_is][out] */ SPEVENT *pEventArray,
        /* [out] */ ULONG *pulFetched);

    HRESULT ( STDMETHODCALLTYPE *GetInfo )(
        ISpVoice * This,
        /* [out] */ SPEVENTSOURCEINFO *pInfo);

    HRESULT ( STDMETHODCALLTYPE *SetOutput )(
        ISpVoice * This,
        /* [in] */ IUnknown *pUnkOutput,
        /* [in] */ BOOL fAllowFormatChanges);

    HRESULT ( STDMETHODCALLTYPE *GetOutputObjectToken )(
        ISpVoice * This,
        /* [out] */ ISpObjectToken **ppObjectToken);

    HRESULT ( STDMETHODCALLTYPE *GetOutputStream )(
        ISpVoice * This,
        /* [out] */ ISpStreamFormat **ppStream);

    HRESULT ( STDMETHODCALLTYPE *Pause )(
        ISpVoice * This);

    HRESULT ( STDMETHODCALLTYPE *Resume )(
        ISpVoice * This);

    HRESULT ( STDMETHODCALLTYPE *SetVoice )(
        ISpVoice * This,
        /* [in] */ ISpObjectToken *pToken);

    HRESULT ( STDMETHODCALLTYPE *GetVoice )(
        ISpVoice * This,
        /* [out] */ ISpObjectToken **ppToken);

    HRESULT ( STDMETHODCALLTYPE *Speak )(
        ISpVoice * This,
        /* [string][in] */ const WCHAR *pwcs,
        /* [in] */ DWORD dwFlags,
        /* [out] */ ULONG *pulStreamNumber);

    HRESULT ( STDMETHODCALLTYPE *SpeakStream )(
        ISpVoice * This,
        /* [in] */ IStream *pStream,
        /* [in] */ DWORD dwFlags,
        /* [out] */ ULONG *pulStreamNumber);

    HRESULT ( STDMETHODCALLTYPE *GetStatus )(
        ISpVoice * This,
        /* [out] */ SPVOICESTATUS *pStatus,
        /* [string][out] */ WCHAR **ppszLastBookmark);

    HRESULT ( STDMETHODCALLTYPE *Skip )(
        ISpVoice * This,
        /* [string][in] */ WCHAR *pItemType,
        /* [in] */ long lNumItems,
        /* [out] */ ULONG *pulNumSkipped);

    HRESULT ( STDMETHODCALLTYPE *SetPriority )(
        ISpVoice * This,
        /* [in] */ SPVPRIORITY ePriority);

    HRESULT ( STDMETHODCALLTYPE *GetPriority )(
        ISpVoice * This,
        /* [out] */ SPVPRIORITY *pePriority);

    HRESULT ( STDMETHODCALLTYPE *SetAlertBoundary )(
        ISpVoice * This,
        /* [in] */ SPEVENTENUM eBoundary);

    HRESULT ( STDMETHODCALLTYPE *GetAlertBoundary )(
        ISpVoice * This,
        /* [out] */ SPEVENTENUM *peBoundary);

    HRESULT ( STDMETHODCALLTYPE *SetRate )(
        ISpVoice * This,
        /* [in] */ long RateAdjust);

    HRESULT ( STDMETHODCALLTYPE *GetRate )(
        ISpVoice * This,
        /* [out] */ long *pRateAdjust);

    HRESULT ( STDMETHODCALLTYPE *SetVolume )(
        ISpVoice * This,
        /* [in] */ USHORT usVolume);

    HRESULT ( STDMETHODCALLTYPE *GetVolume )(
        ISpVoice * This,
        /* [out] */ USHORT *pusVolume);

    HRESULT ( STDMETHODCALLTYPE *WaitUntilDone )(
        ISpVoice * This,
        /* [in] */ ULONG msTimeout);

    HRESULT ( STDMETHODCALLTYPE *SetSyncSpeakTimeout )(
        ISpVoice * This,
        /* [in] */ ULONG msTimeout);

    HRESULT ( STDMETHODCALLTYPE *GetSyncSpeakTimeout )(
        ISpVoice * This,
        /* [out] */ ULONG *pmsTimeout);

    /* [local] */ HANDLE ( STDMETHODCALLTYPE *SpeakCompleteEvent )(
        ISpVoice * This);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *IsUISupported )(
        ISpVoice * This,
        /* [in] */ const WCHAR *pszTypeOfUI,
        /* [in] */ void *pvExtraData,
        /* [in] */ ULONG cbExtraData,
        /* [out] */ BOOL *pfSupported);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *DisplayUI )(
        ISpVoice * This,
        /* [in] */ HWND hwndParent,
        /* [in] */ const WCHAR *pszTitle,
        /* [in] */ const WCHAR *pszTypeOfUI,
        /* [in] */ void *pvExtraData,
        /* [in] */ ULONG cbExtraData);

    END_INTERFACE
} ISpVoiceVtbl;

struct ISpVoice
{
    struct ISpVoiceVtbl *lpVtbl;
};
void TTS_SayUnicodeString(wchar_t *stringtosay)
{
	static CLSID CLSID_SpVoice = {0x96749377, 0x3391, 0x11D2,
								{0x9E,0xE3,0x00,0xC0,0x4F,0x79,0x73,0x96}};
	static GUID IID_ISpVoice = {0x6C44DF74,0x72B9,0x4992,
								{0xA1,0xEC,0xEF,0x99,0x6E,0x04,0x22,0xD4}};
	static ISpVoice *sp = NULL;

	if (!sp)
		CoCreateInstance(
				&CLSID_SpVoice,
				NULL,
				CLSCTX_SERVER,
				&IID_ISpVoice,
				(void*)&sp);

	if (sp)
	{
		sp->lpVtbl->Speak(sp, stringtosay, 1, NULL);
	}
}
void TTS_SayAsciiString(char *stringtosay)
{
	wchar_t bigbuffer[8192];
	mbstowcs(bigbuffer, stringtosay, sizeof(bigbuffer)/sizeof(bigbuffer[0]) - 1);
	bigbuffer[sizeof(bigbuffer)/sizeof(bigbuffer[0]) - 1] = 0;
	TTS_SayUnicodeString(bigbuffer);
}

cvar_t tts_mode = CVARD("tts_mode", "1", "Text to speech\n0: off\n1: Read only chat messages with a leading 'tts ' prefix.\n2: Read all chat messages\n3: Read every single console print.");
void TTS_SayChatString(char **stringtosay)
{
	if (!strncmp(*stringtosay, "tts ", 4))
	{
		*stringtosay += 4;
		if (tts_mode.ival != 1 && tts_mode.ival != 2)
			return;
	}
	else
	{
		if (tts_mode.ival != 2)
			return;
	}

	TTS_SayAsciiString(*stringtosay);
}
void TTS_SayConString(conchar_t *stringtosay)
{
	wchar_t bigbuffer[8192];
	int i;

	if (tts_mode.ival < 3)
		return;
	
	for (i = 0; i < 8192-1 && *stringtosay; i++, stringtosay++)
	{
		if ((*stringtosay & 0xff00) == 0xe000)
			bigbuffer[i] = *stringtosay & 0x7f;
		else
			bigbuffer[i] = *stringtosay & CON_CHARMASK;
	}
	bigbuffer[i] = 0;
	if (i)
		TTS_SayUnicodeString(bigbuffer);
}
void TTS_Say_f(void)
{
	TTS_SayAsciiString(Cmd_Args());
}

#define ISpRecognizer void
#define SPPHRASE void
#define SPSERIALIZEDPHRASE void
#define SPSTATEHANDLE void*
#define SPGRAMMARWORDTYPE int
#define SPPROPERTYINFO void
#define SPLOADOPTIONS void*
#define SPBINARYGRAMMAR void*
#define SPRULESTATE int
#define SPTEXTSELECTIONINFO void
#define SPWORDPRONOUNCEABLE void
#define SPGRAMMARSTATE int
typedef struct ISpRecoResult ISpRecoResult;
typedef struct ISpRecoContext ISpRecoContext;
typedef struct ISpRecoGrammar ISpRecoGrammar;

typedef struct ISpRecoContextVtbl
{
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
		ISpRecoContext * This,
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void **ppvObject);

	ULONG ( STDMETHODCALLTYPE *AddRef )( 
		ISpRecoContext * This);

	ULONG ( STDMETHODCALLTYPE *Release )( 
		ISpRecoContext * This);

    HRESULT ( STDMETHODCALLTYPE *SetNotifySink )( 
        ISpRecoContext * This,
        /* [in] */ ISpNotifySink *pNotifySink);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyWindowMessage )( 
        ISpRecoContext * This,
        /* [in] */ HWND hWnd,
        /* [in] */ UINT Msg,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyCallbackFunction )( 
        ISpRecoContext * This,
        /* [in] */ SPNOTIFYCALLBACK *pfnCallback,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyCallbackInterface )( 
        ISpRecoContext * This,
        /* [in] */ ISpNotifyCallback *pSpCallback,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyWin32Event )( 
        ISpRecoContext * This);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *WaitForNotifyEvent )( 
        ISpRecoContext * This,
        /* [in] */ DWORD dwMilliseconds);
    
    /* [local] */ HANDLE ( STDMETHODCALLTYPE *GetNotifyEventHandle )( 
        ISpRecoContext * This);
    
    HRESULT ( STDMETHODCALLTYPE *SetInterest )( 
        ISpRecoContext * This,
        /* [in] */ ULONGLONG ullEventInterest,
        /* [in] */ ULONGLONG ullQueuedInterest);
    
    HRESULT ( STDMETHODCALLTYPE *GetEvents )( 
        ISpRecoContext * This,
        /* [in] */ ULONG ulCount,
        /* [size_is][out] */ SPEVENT *pEventArray,
        /* [out] */ ULONG *pulFetched);

    HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
        ISpRecoContext * This,
        /* [out] */ SPEVENTSOURCEINFO *pInfo);
    
    HRESULT ( STDMETHODCALLTYPE *GetRecognizer )( 
        ISpRecoContext * This,
        /* [out] */ ISpRecognizer **ppRecognizer);
    
    HRESULT ( STDMETHODCALLTYPE *CreateGrammar )( 
        ISpRecoContext * This,
        /* [in] */ ULONGLONG ullGrammarId,
        /* [out] */ ISpRecoGrammar **ppGrammar);
} ISpRecoContextVtbl;
struct ISpRecoContext
{
    struct ISpRecoContextVtbl *lpVtbl;
};

typedef struct ISpRecoResultVtbl
{
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ISpRecoResult * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);
    
    ULONG ( STDMETHODCALLTYPE *AddRef )( 
        ISpRecoResult * This);
    
    ULONG ( STDMETHODCALLTYPE *Release )( 
        ISpRecoResult * This);
    
    HRESULT ( STDMETHODCALLTYPE *GetPhrase )( 
        ISpRecoResult * This,
        /* [out] */ SPPHRASE **ppCoMemPhrase);
    
    HRESULT ( STDMETHODCALLTYPE *GetSerializedPhrase )( 
        ISpRecoResult * This,
        /* [out] */ SPSERIALIZEDPHRASE **ppCoMemPhrase);
    
    HRESULT ( STDMETHODCALLTYPE *GetText )( 
        ISpRecoResult * This,
        /* [in] */ ULONG ulStart,
        /* [in] */ ULONG ulCount,
        /* [in] */ BOOL fUseTextReplacements,
        /* [out] */ WCHAR **ppszCoMemText,
        /* [out] */ BYTE *pbDisplayAttributes);
    
    HRESULT ( STDMETHODCALLTYPE *Discard )( 
        ISpRecoResult * This,
        /* [in] */ DWORD dwValueTypes);
#if 0
    HRESULT ( STDMETHODCALLTYPE *GetResultTimes )( 
        ISpRecoResult * This,
        /* [out] */ SPRECORESULTTIMES *pTimes);
    
    HRESULT ( STDMETHODCALLTYPE *GetAlternates )( 
        ISpRecoResult * This,
        /* [in] */ ULONG ulStartElement,
        /* [in] */ ULONG cElements,
        /* [in] */ ULONG ulRequestCount,
        /* [out] */ ISpPhraseAlt **ppPhrases,
        /* [out] */ ULONG *pcPhrasesReturned);
    
    HRESULT ( STDMETHODCALLTYPE *GetAudio )( 
        ISpRecoResult * This,
        /* [in] */ ULONG ulStartElement,
        /* [in] */ ULONG cElements,
        /* [out] */ ISpStreamFormat **ppStream);
    
    HRESULT ( STDMETHODCALLTYPE *SpeakAudio )( 
        ISpRecoResult * This,
        /* [in] */ ULONG ulStartElement,
        /* [in] */ ULONG cElements,
        /* [in] */ DWORD dwFlags,
        /* [out] */ ULONG *pulStreamNumber);
    
    HRESULT ( STDMETHODCALLTYPE *Serialize )( 
        ISpRecoResult * This,
        /* [out] */ SPSERIALIZEDRESULT **ppCoMemSerializedResult);
    
    HRESULT ( STDMETHODCALLTYPE *ScaleAudio )( 
        ISpRecoResult * This,
        /* [in] */ const GUID *pAudioFormatId,
        /* [in] */ const WAVEFORMATEX *pWaveFormatEx);
    
    HRESULT ( STDMETHODCALLTYPE *GetRecoContext )( 
        ISpRecoResult * This,
        /* [out] */ ISpRecoContext **ppRecoContext);
    
#endif
} ISpRecoResultVtbl;
struct ISpRecoResult
{
    struct ISpRecoResultVtbl *lpVtbl;
};

typedef struct ISpRecoGrammarVtbl
{
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ISpRecoGrammar * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);
    
    ULONG ( STDMETHODCALLTYPE *AddRef )( 
        ISpRecoGrammar * This);
    
    ULONG ( STDMETHODCALLTYPE *Release )( 
        ISpRecoGrammar * This);
    
    HRESULT ( STDMETHODCALLTYPE *ResetGrammar )( 
        ISpRecoGrammar * This,
        /* [in] */ WORD NewLanguage);
    
    HRESULT ( STDMETHODCALLTYPE *GetRule )( 
        ISpRecoGrammar * This,
        /* [in] */ const WCHAR *pszRuleName,
        /* [in] */ DWORD dwRuleId,
        /* [in] */ DWORD dwAttributes,
        /* [in] */ BOOL fCreateIfNotExist,
        /* [out] */ SPSTATEHANDLE *phInitialState);
    
    HRESULT ( STDMETHODCALLTYPE *ClearRule )( 
        ISpRecoGrammar * This,
        SPSTATEHANDLE hState);
    
    HRESULT ( STDMETHODCALLTYPE *CreateNewState )( 
        ISpRecoGrammar * This,
        SPSTATEHANDLE hState,
        SPSTATEHANDLE *phState);
    
    HRESULT ( STDMETHODCALLTYPE *AddWordTransition )( 
        ISpRecoGrammar * This,
        SPSTATEHANDLE hFromState,
        SPSTATEHANDLE hToState,
        const WCHAR *psz,
        const WCHAR *pszSeparators,
        SPGRAMMARWORDTYPE eWordType,
        float Weight,
        const SPPROPERTYINFO *pPropInfo);
    
    HRESULT ( STDMETHODCALLTYPE *AddRuleTransition )( 
        ISpRecoGrammar * This,
        SPSTATEHANDLE hFromState,
        SPSTATEHANDLE hToState,
        SPSTATEHANDLE hRule,
        float Weight,
        const SPPROPERTYINFO *pPropInfo);
    
    HRESULT ( STDMETHODCALLTYPE *AddResource )( 
        ISpRecoGrammar * This,
        /* [in] */ SPSTATEHANDLE hRuleState,
        /* [in] */ const WCHAR *pszResourceName,
        /* [in] */ const WCHAR *pszResourceValue);
    
    HRESULT ( STDMETHODCALLTYPE *Commit )( 
        ISpRecoGrammar * This,
        DWORD dwReserved);
    
    HRESULT ( STDMETHODCALLTYPE *GetGrammarId )( 
        ISpRecoGrammar * This,
        /* [out] */ ULONGLONG *pullGrammarId);
    
    HRESULT ( STDMETHODCALLTYPE *GetRecoContext )( 
        ISpRecoGrammar * This,
        /* [out] */ ISpRecoContext **ppRecoCtxt);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromFile )( 
        ISpRecoGrammar * This,
        /* [string][in] */ const WCHAR *pszFileName,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromObject )( 
        ISpRecoGrammar * This,
        /* [in] */ REFCLSID rcid,
        /* [string][in] */ const WCHAR *pszGrammarName,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromResource )( 
        ISpRecoGrammar * This,
        /* [in] */ HMODULE hModule,
        /* [string][in] */ const WCHAR *pszResourceName,
        /* [string][in] */ const WCHAR *pszResourceType,
        /* [in] */ WORD wLanguage,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromMemory )( 
        ISpRecoGrammar * This,
        /* [in] */ const SPBINARYGRAMMAR *pGrammar,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromProprietaryGrammar )( 
        ISpRecoGrammar * This,
        /* [in] */ REFGUID rguidParam,
        /* [string][in] */ const WCHAR *pszStringParam,
        /* [in] */ const void *pvDataPrarm,
        /* [in] */ ULONG cbDataSize,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *SetRuleState )( 
        ISpRecoGrammar * This,
        /* [string][in] */ const WCHAR *pszName,
        void *pReserved,
        /* [in] */ SPRULESTATE NewState);
    
    HRESULT ( STDMETHODCALLTYPE *SetRuleIdState )( 
        ISpRecoGrammar * This,
        /* [in] */ ULONG ulRuleId,
        /* [in] */ SPRULESTATE NewState);
    
    HRESULT ( STDMETHODCALLTYPE *LoadDictation )( 
        ISpRecoGrammar * This,
        /* [string][in] */ const WCHAR *pszTopicName,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *UnloadDictation )( 
        ISpRecoGrammar * This);
    
    HRESULT ( STDMETHODCALLTYPE *SetDictationState )( 
        ISpRecoGrammar * This,
        /* [in] */ SPRULESTATE NewState);
    
    HRESULT ( STDMETHODCALLTYPE *SetWordSequenceData )( 
        ISpRecoGrammar * This,
        /* [in] */ const WCHAR *pText,
        /* [in] */ ULONG cchText,
        /* [in] */ const SPTEXTSELECTIONINFO *pInfo);
    
    HRESULT ( STDMETHODCALLTYPE *SetTextSelection )( 
        ISpRecoGrammar * This,
        /* [in] */ const SPTEXTSELECTIONINFO *pInfo);
    
    HRESULT ( STDMETHODCALLTYPE *IsPronounceable )( 
        ISpRecoGrammar * This,
        /* [string][in] */ const WCHAR *pszWord,
        /* [out] */ SPWORDPRONOUNCEABLE *pWordPronounceable);
    
    HRESULT ( STDMETHODCALLTYPE *SetGrammarState )( 
        ISpRecoGrammar * This,
        /* [in] */ SPGRAMMARSTATE eGrammarState);
    
    HRESULT ( STDMETHODCALLTYPE *SaveCmd )( 
        ISpRecoGrammar * This,
        /* [in] */ IStream *pStream,
        /* [optional][out] */ WCHAR **ppszCoMemErrorText);
    
    HRESULT ( STDMETHODCALLTYPE *GetGrammarState )( 
        ISpRecoGrammar * This,
        /* [out] */ SPGRAMMARSTATE *peGrammarState);
} ISpRecoGrammarVtbl;
struct ISpRecoGrammar
{
	struct ISpRecoGrammarVtbl *lpVtbl;
};

static ISpRecoContext *stt_recctx = NULL;
static ISpRecoGrammar *stt_gram = NULL;
void STT_Event(void)
{
	WCHAR *wstring, *i;
	struct SPEVENT ev;
	ISpRecoResult *rr;
	HRESULT hr;
	char asc[2048], *o;
	int l;
	unsigned short c;
	char *nib = "0123456789abcdef";
	if (!stt_gram)
		return;

	while (SUCCEEDED(hr = stt_recctx->lpVtbl->GetEvents(stt_recctx, 1, &ev, NULL)) && hr != S_FALSE)
	{
		rr = (ISpRecoResult*)ev.lParam;
		rr->lpVtbl->GetText(rr, -1, -1, TRUE, &wstring, NULL);
		for (l = sizeof(asc)-1, o = asc, i = wstring; l > 0 && *i; )
		{
			c = *i++;
			if (c == '\n' || c == ';')
			{
			}
			else if (c < 128)
			{
				*o++ = c;
				l--;
			}
			else if (l > 6)
			{
				*o++ = '^';
				*o++ = 'U';
				*o++ = nib[(c>>12)&0xf];
				*o++ = nib[(c>>8)&0xf];
				*o++ = nib[(c>>4)&0xf];
				*o++ = nib[(c>>0)&0xf];
			}
			else
				break;
		}
		*o = 0;
		CoTaskMemFree(wstring);
		Cbuf_AddText("say tts ", RESTRICT_LOCAL);
		Cbuf_AddText(asc, RESTRICT_LOCAL);
		Cbuf_AddText("\n", RESTRICT_LOCAL);
		rr->lpVtbl->Release(rr);
	}
}
void STT_Init_f(void)
{
	static CLSID CLSID_SpSharedRecoContext	=	{0x47206204, 0x5ECA, 0x11D2, {0x96, 0x0F, 0x00, 0xC0, 0x4F, 0x8E, 0xE6, 0x28}};
	static CLSID IID_SpRecoContext			=	{0xF740A62F, 0x7C15, 0x489E, {0x82, 0x34, 0x94, 0x0A, 0x33, 0xD9, 0x27, 0x2D}};

	if (stt_gram)
	{
		stt_gram->lpVtbl->Release(stt_gram);
		stt_recctx->lpVtbl->Release(stt_recctx);
		stt_gram = NULL;
		stt_recctx = NULL;
		Con_Printf("Speech-to-text disabled\n");
		return;
	}

	if (SUCCEEDED(CoCreateInstance(&CLSID_SpSharedRecoContext, NULL, CLSCTX_SERVER, &IID_SpRecoContext, (void*)&stt_recctx)))
	{
		ULONGLONG ev = (((ULONGLONG)1) << 38) | (((ULONGLONG)1) << 30) | (((ULONGLONG)1) << 33);
		if (SUCCEEDED(stt_recctx->lpVtbl->SetNotifyWindowMessage(stt_recctx, mainwindow, WM_USER_SPEECHTOTEXT, 0, 0)))
		if (SUCCEEDED(stt_recctx->lpVtbl->SetInterest(stt_recctx, ev, ev)))
		if (SUCCEEDED(stt_recctx->lpVtbl->CreateGrammar(stt_recctx, 0, &stt_gram)))
		{
			if (SUCCEEDED(stt_gram->lpVtbl->LoadDictation(stt_gram, NULL, 0)))
			if (SUCCEEDED(stt_gram->lpVtbl->SetDictationState(stt_gram, 1)))
			{
				//success!
				Con_Printf("Speech-to-text active\n");
				return;
			}
			stt_gram->lpVtbl->Release(stt_gram);
		}
		stt_recctx->lpVtbl->Release(stt_recctx);
	}
	stt_gram = NULL;
	stt_recctx = NULL;

	Con_Printf("Speech-to-text unavailable\n");
}
#endif






#ifdef AVAIL_MP3_ACM
typedef struct
{
	HACMSTREAM acm;

	unsigned int dstbuffer; /*in frames*/
	unsigned int dstcount; /*in frames*/
	unsigned int dststart; /*in frames*/
	qbyte *dstdata;

	unsigned int srcspeed;
	qaudiofmt_t  srcformat;
	unsigned int srcchannels;
	unsigned int srcoffset; /*in bytes*/
	unsigned int srclen;	/*in bytes*/
	qbyte srcdata[1];

	char title[256];
} mp3decoder_t;

static void QDECL S_MP3_Purge(sfx_t *sfx)
{
	mp3decoder_t *dec = sfx->decoder.buf;

	sfx->decoder.buf = NULL;
	sfx->decoder.ended = NULL;
	sfx->decoder.purge = NULL;
	sfx->decoder.decodedata = NULL;

	qacmStreamClose(dec->acm, 0);

	if (dec->dstdata)
		BZ_Free(dec->dstdata);
	BZ_Free(dec);

	sfx->loadstate = SLS_NOTLOADED;
}

float QDECL S_MP3_Query(sfx_t *sfx, sfxcache_t *buf, char *title, size_t titlesize)
{
	mp3decoder_t *dec = sfx->decoder.buf;
	//we don't know unless we decode it all
	if (buf)
	{
	}
	if (titlesize && dec->srclen >= 128)
	{	//id3v1 is a 128 byte blob at the end of the file.
		char trimartist[31];
		char trimtitle[31];
		char *p;
		struct
		{
			char tag[3];	//TAG
			char title[30];
			char artist[30];
			char album[30];
			char year[4];
			char comment[30];//[28]+null+track
			qbyte genre;
		} *id3v1 = (void*)(dec->srcdata + dec->srclen-128);
		if (id3v1->tag[0] == 'T' && id3v1->tag[1] == 'A' && id3v1->tag[2] == 'G')
		{	//yup, there's an id3v1 tag there
			memcpy(trimartist, id3v1->artist, 30);
			for(p = trimartist+30; p>trimartist && p[-1] == ' '; )
				p--;
			*p = 0;
			memcpy(trimtitle, id3v1->title, 30);
			for(p = trimtitle+30; p>trimtitle && p[-1] == ' '; )
				p--;
			*p = 0;
			if (*trimartist && *trimtitle)
				Q_snprintfz(title, titlesize, "%.30s - %.30s", trimartist, trimtitle);
			else if (*id3v1->title)
				Q_snprintfz(title, titlesize, "%.30s", trimtitle);
		}
		return 1;//no real idea.
	}
	return 0;
}

/*must be thread safe*/
sfxcache_t *QDECL S_MP3_Locate(sfx_t *sfx, sfxcache_t *buf, ssamplepos_t start, int length)
{
	int newlen;
	if (buf)
	{
		mp3decoder_t *dec = sfx->decoder.buf;
		ACMSTREAMHEADER strhdr;
		char buffer[8192];
		extern cvar_t snd_linearresample_stream;
		int framesz = (QAF_BYTES(dec->srcformat) * dec->srcchannels);

		if (length)
		{
			if (dec->dststart > start)
			{
				/*I don't know where the compressed data is for each sample. acm doesn't have a seek. so reset to start, for music this should be the most common rewind anyway*/
				dec->dststart = 0;
				dec->dstcount = 0;
				dec->srcoffset = 0;
			}

			if (dec->dstcount > snd_speed*6)
			{
				int trim = dec->dstcount - snd_speed; //retain a second of buffer in case we have multiple sound devices
				if (dec->dststart + trim > start)
				{
					trim = start - dec->dststart;
					if (trim < 0)
						trim = 0;
				}
	//			if (trim < 0)
	//				trim = 0;
	///			if (trim > dec->dstcount)
	//				trim = dec->dstcount;
				memmove(dec->dstdata, dec->dstdata + trim*framesz, (dec->dstcount - trim)*framesz);
				dec->dststart += trim;
				dec->dstcount -= trim;
			}

			while(start+length >= dec->dststart+dec->dstcount)
			{
				memset(&strhdr, 0, sizeof(strhdr));
				strhdr.cbStruct = sizeof(strhdr);
				strhdr.pbSrc = dec->srcdata + dec->srcoffset;
				strhdr.cbSrcLength = dec->srclen - dec->srcoffset;
				if (!strhdr.cbSrcLength)
					break;
				strhdr.pbDst = buffer;
				strhdr.cbDstLength = sizeof(buffer);

				qacmStreamPrepareHeader(dec->acm, &strhdr, 0);
				qacmStreamConvert(dec->acm, &strhdr, ACM_STREAMCONVERTF_BLOCKALIGN);
				qacmStreamUnprepareHeader(dec->acm, &strhdr, 0);
				dec->srcoffset += strhdr.cbSrcLengthUsed;
				if (!strhdr.cbDstLengthUsed)
				{
					if (strhdr.cbSrcLengthUsed)
						continue;
					break;
				}

				newlen = dec->dstcount + (strhdr.cbDstLengthUsed * ((float)snd_speed / dec->srcspeed))/framesz;
				if (dec->dstbuffer < newlen+64)
				{
					dec->dstbuffer = newlen+64 + snd_speed;
					dec->dstdata = BZ_Realloc(dec->dstdata, dec->dstbuffer*framesz);
				}

				SND_ResampleStream(strhdr.pbDst, 
					dec->srcspeed, 
					dec->srcformat,
					dec->srcchannels, 
					strhdr.cbDstLengthUsed / framesz,
					dec->dstdata+dec->dstcount*framesz,
					snd_speed,
					dec->srcformat,
					dec->srcchannels,
					snd_linearresample_stream.ival);
				dec->dstcount = newlen;
			}
		}

		buf->data = dec->dstdata;
		buf->length = dec->dstcount;
		buf->numchannels = dec->srcchannels;
		buf->soundoffset = dec->dststart;
		buf->speed = snd_speed;
		buf->format = dec->srcformat;

		if (dec->srclen == dec->srcoffset && start >= dec->dststart+dec->dstcount)
			return NULL;	//once we reach the EOF, start reporting errors.
	}
	return buf;
}

#ifndef WAVE_FORMAT_MPEGLAYER3
#define WAVE_FORMAT_MPEGLAYER3 0x0055
typedef struct
{
	WAVEFORMATEX  wfx;
	WORD          wID;
	DWORD         fdwFlags;
	WORD          nBlockSize;
	WORD          nFramesPerBlock;
	WORD          nCodecDelay;
} MPEGLAYER3WAVEFORMAT;
#endif
#ifndef MPEGLAYER3_ID_MPEG
#define MPEGLAYER3_WFX_EXTRA_BYTES 12
#define MPEGLAYER3_FLAG_PADDING_OFF 2
#define MPEGLAYER3_ID_MPEG 1
#endif

static qboolean QDECL S_LoadMP3Sound (sfx_t *s, qbyte *data, size_t datalen, int sndspeed, qboolean forcedecode)
{
	WAVEFORMATEX pcm_format;
	MPEGLAYER3WAVEFORMAT mp3format;
	HACMDRIVER drv = NULL;
	mp3decoder_t *dec;

	char ext[8];
	COM_FileExtension(s->name, ext, sizeof(ext));
	if (stricmp(ext, "mp3"))
		return false;

	dec = BZF_Malloc(sizeof(*dec) + datalen);
	if (!dec)
		return false;
	memcpy(dec->srcdata, data, datalen);
	dec->srclen = datalen;
	s->decoder.buf = dec;
	s->decoder.ended = S_MP3_Purge;
	s->decoder.purge = S_MP3_Purge;
	s->decoder.decodedata = S_MP3_Locate;
	s->decoder.querydata = S_MP3_Query;
	s->loopstart = -1;
	
	dec->dstdata = NULL;
	dec->dstcount = 0;
	dec->dststart = 0;
	dec->dstbuffer = 0;
	dec->srcoffset = 0;

	dec->srcspeed = 44100;
	dec->srcchannels = 2;
	dec->srcformat = QAF_S16;

	memset (&pcm_format, 0, sizeof(pcm_format));
	pcm_format.wFormatTag = WAVE_FORMAT_PCM;
	pcm_format.nChannels = dec->srcchannels;
	pcm_format.nSamplesPerSec = dec->srcspeed;
	pcm_format.nBlockAlign = QAF_BYTES(dec->srcformat)*dec->srcchannels;
	pcm_format.nAvgBytesPerSec = pcm_format.nSamplesPerSec*QAF_BYTES(dec->srcformat)*dec->srcchannels;
	pcm_format.wBitsPerSample = QAF_BYTES(dec->srcformat)*8;
	pcm_format.cbSize = 0;

	mp3format.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
	mp3format.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
	mp3format.wfx.nChannels = dec->srcchannels;
	mp3format.wfx.nAvgBytesPerSec = 128 * (1024 / 8);  // not really used but must be one of 64, 96, 112, 128, 160kbps
	mp3format.wfx.wBitsPerSample = 0;                  // MUST BE ZERO
	mp3format.wfx.nBlockAlign = 1;                     // MUST BE ONE
	mp3format.wfx.nSamplesPerSec = dec->srcspeed;       // 44.1kHz
	mp3format.fdwFlags = MPEGLAYER3_FLAG_PADDING_OFF;
	mp3format.nBlockSize = 522;					       // voodoo value #1 - 144 x (bitrate / sample rate) + padding
	mp3format.nFramesPerBlock = 1;                     // MUST BE ONE
	mp3format.nCodecDelay = 0;//1393;                      // voodoo value #2
	mp3format.wID = MPEGLAYER3_ID_MPEG;

	if (!qacmStartup() || 0!=qacmStreamOpen(&dec->acm, drv, (WAVEFORMATEX*)&mp3format, &pcm_format, NULL, 0, 0, 0))
	{
		Con_Printf("Couldn't init decoder\n");
		return false;
	}

	S_MP3_Locate(s, NULL, 0, 100);


	return true;
}
#endif



void Media_Init(void)
{
#ifdef HAVE_JUKEBOX
	int i;
	Cmd_AddCommand("music_fforward", Media_FForward_f);
	Cmd_AddCommand("music_rewind", Media_Rewind_f);
	Cmd_AddCommand("music_next", Media_Next_f);
	Cmd_AddCommand("media_next", Media_Next_f);

	Cvar_Register(&music_playlist_index, "compat");
	for (i = 0; i < 6; i++)
	{
		Cvar_Get(va("music_playlist_list%i", i), "", 0, "compat");
		Cvar_Get(va("music_playlist_sampleposition%i", i), "-1", 0, "compat");
	}
	music_playlist_last = -1;
	media_playlisttypes = MEDIA_PLAYLIST | MEDIA_GAMEMUSIC | MEDIA_CVARLIST;

	#ifdef WINAMP
		Cvar_Register(&media_hijackwinamp,	"Media player things");
	#endif
	Cvar_Register(&media_shuffle,	"Media player things");
	Cvar_Register(&media_repeat,	"Media player things");
	#if !defined(NOMEDIAMENU) && !defined(NOBUILTINMENUS)
		Cmd_AddCommand ("media_add", M_Media_Add_f);
		Cmd_AddCommand ("media_remove", M_Media_Remove_f);
		Cmd_AddCommand ("menu_media", M_Menu_Media_f);
	#endif
#endif
	Cvar_Register(&music_fade,	"Media player things");

#ifdef HAVE_SPEECHTOTEXT
	Cmd_AddCommand("tts", TTS_Say_f);
	Cmd_AddCommand("stt", STT_Init_f);
	Cvar_Register(&tts_mode, "Gimmicks");
#endif
#ifdef AVAIL_MP3_ACM
	S_RegisterSoundInputPlugin(NULL, S_LoadMP3Sound);
#endif


#ifdef HAVE_CDPLAYER
	Cmd_AddCommand("cd", CD_f);
	cdenabled = false;
	if (COM_CheckParm("-nocdaudio"))
		cdenabled = false;
	if (COM_CheckParm("-cdaudio"))
		cdenabled = true;
#endif

#ifdef HAVE_MEDIA_ENCODER
	#ifdef _DEBUG
		Media_RegisterEncoder(NULL, &capture_null);
	#endif
	Media_RegisterEncoder(NULL, &capture_raw);
	#if defined(HAVE_API_VFW)
		Media_RegisterEncoder(NULL, &capture_avi);
	#endif

	Cmd_AddCommandD("capture", Media_RecordFilm_f, "Captures realtime action to a named video file. Check the capture* cvars to control driver/codecs/rates.");
	Cmd_AddCommandD("capturedemo", Media_RecordDemo_f, "capturedemo foo.dem foo.avi - Captures a named demo to a named video file.\nDemo capturing is performed offscreen when possible, allowing arbitrary video sizes or smooth captures on underpowered hardware.");
	Cmd_AddCommandD("capturestop", Media_StopRecordFilm_f, "Aborts the current video capture.");
	Cmd_AddCommandD("capturepause", Media_CapturePause_f, "Pauses the video capture, allowing you to avoid capturing uninteresting parts. This is a toggle, so reuse the same command to resume capturing again.");

	Cvar_Register(&capturemessage,			"Video Capture Controls");
	Cvar_Register(&capturesound,			"Video Capture Controls");
	Cvar_Register(&capturerate,				"Video Capture Controls");
	Cvar_Register(&capturewidth,			"Video Capture Controls");
	Cvar_Register(&captureheight,			"Video Capture Controls");
	Cvar_Register(&capturedriver,			"Video Capture Controls");
	Cvar_Register(&capturecodec,			"Video Capture Controls");
	Cvar_Register(&capturethrottlesize,		"Video Capture Controls");
	Cvar_Register(&capturesoundbits,		"Video Capture Controls");
	Cvar_Register(&capturesoundchannels,	"Video Capture Controls");
#endif

#ifdef HAVE_MEDIA_DECODER
	Cmd_AddCommand("playclip", Media_PlayVideoWindowed_f);
	Cmd_AddCommand("playvideo", Media_PlayFilm_f);
	Cmd_AddCommand("playfilm", Media_PlayFilm_f);
	Cmd_AddCommand("cinematic", Media_PlayFilm_f);	//q3: name <1:hold, 2:loop>
#endif

	Cmd_AddCommand("music", Media_NamedTrack_f);
	Cmd_AddCommand("stopmusic", Media_StopTrack_f);
}
