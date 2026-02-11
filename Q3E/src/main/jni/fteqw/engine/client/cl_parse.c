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
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"
#include "cl_ignore.h"
#include "shader.h"
#include "fs.h"

void CL_GetNumberedEntityInfo (int num, float *org, float *ang);
void CLDP_ParseDarkPlaces5Entities(void);
void CLH2_ParseEntities(void);
static void CL_SetStatNumeric (int pnum, unsigned int stat, int ivalue, float fvalue);
#define CL_SetStatInt(pnum,stat,ival) do{int thevalue=ival; CL_SetStatNumeric(pnum,stat,thevalue,thevalue);}while(0)
#define CL_SetStatFloat(pnum,stat,fval) do{float thevalue=fval; CL_SetStatNumeric(pnum,stat,thevalue,thevalue);}while(0)
static qboolean CL_CheckModelResources (char *name);
#ifdef NQPROT
static char *CLNQ_ParseProQuakeMessage (char *s);
#endif
static void DLC_Poll(qdownload_t *dl);
static void CL_ProcessUserInfo (int slot, player_info_t *player);
static void CL_ParseStuffCmd(char *msg, int destsplit);

#define MSG_ReadBigIndex() ((cls.fteprotocolextensions2&PEXT2_LONGINDEXES)?(unsigned int)MSG_ReadUInt64():MSG_ReadByte ())
#define MSG_ReadPlayer() MSG_ReadBigIndex()

#ifdef NQPROT
char *cl_dp_packagenames;
static char cl_dp_csqc_progsname[128];
static int cl_dp_csqc_progssize;
static int cl_dp_csqc_progscrc;
static int cl_dp_serverextension_download;
#endif

//tracks which svcs are using what data (per second)
static size_t packetusage_saved[256];
static size_t packetusage_pending[256];
static double packetusageflushtime;
static const double packetusage_interval=10;

#ifdef AVAIL_ZLIB
#ifndef ZEXPORT
	#define ZEXPORT VARGS
#endif
#include <zlib.h>
#endif


static const char *svc_qwstrings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svcqw_updatestatbyte",
	"svc_version",		// [long] server version
	"svc_setview",		// [short] entity number
	"svcqw_sound",			// <see code>
	"svc_time",			// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"svc_setangle",		// [vec3] set the view angle to this absolute value

	"svc_serverdata",		// [long] version ...
	"svc_lightstyle",		// [qbyte] [string]
	"svc_updatename",		// [qbyte] [string]
	"svc_updatefrags",	// [qbyte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [qbyte] [qbyte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",			// [qbyte] impact [qbyte] blood [vec3] from

	"svc_spawnstatic",
	"svcfte_spawnstatic2",
	"svc_spawnbaseline",

	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",

	"svc_cdtrack",
	"svc_sellscreen",

	"svc_smallkick",
	"svc_bigkick",

	"svc_updateping",
	"svc_updateentertime",

	"svc_updatestatlong",
	"svc_muzzleflash",
	"svc_updateuserinfo",
	"svc_download",
	"svc_playerinfo",
	"svc_nails",
	"svc_choke",
	"svc_modellist",
	"svc_soundlist",
	"svc_packetentities",
 	"svc_deltapacketentities",
	"svc_maxspeed",
	"svc_entgravity",

	"svc_setinfo",
	"svc_serverinfo",
	"svc_updatepl",
	"MVD svc_nails2",
	"svcfte_soundextended",
	"svcfte_soundlistshort",
	"FTE svc_lightstylecol",
	"FTE svc_bulletentext", // obsolete
	"FTE svc_lightnings",
	"FTE svc_modellistshort",
	"FTE svc_ftesetclientpersist",
	"FTE svc_setportalstate",
	"FTE svc_particle2",
	"FTE svc_particle3",
	"FTE svc_particle4",
	"FTE svc_spawnbaseline2",
	"FTE svc_customtempent",
	"FTE svc_choosesplitclient",

	"svcfte_showpic",
	"svcfte_hidepic",
	"svcfte_movepic",
	"svcfte_updatepic",

	"NEW PROTOCOL(73)",

	"svcfte_effect",
	"svcfte_effect2",

	"svcfte_csqcentities",

	"svcfte_precache",

	"svcfte_updatestatstring",
	"svcfte_updatestatfloat",

	"svcfte_trailparticles",
	"svcfte_pointparticles",
	"svcfte_pointparticles1",

	"svcfte_cgamepacket",
	"svcfte_voicechat",
	"svcfte_setangledelta",
	"svcfte_updateentities",
	"svcfte_brushedit",
	"svcfte_updateseats",
	"svcfte_setinfoblob",			//89
	"svcfte_cgamepacket_sized",		//90
	"svcfte_temp_entity_sized",		//91
	"svcfte_csqcentities_sized",	//92
	"NEW PROTOCOL(93)",
	"NEW PROTOCOL(94)",
	"NEW PROTOCOL(95)",
	"NEW PROTOCOL(96)",
	"NEW PROTOCOL(97)",
	"NEW PROTOCOL(98)",
	"NEW PROTOCOL(99)",
	"NEW PROTOCOL(100)",
	"NEW PROTOCOL(101)",
	"NEW PROTOCOL(102)",
	"NEW PROTOCOL(103)",
	"NEW PROTOCOL(104)",
	"NEW PROTOCOL(105)",
	"NEW PROTOCOL(106)",
	"NEW PROTOCOL(107)",
	"NEW PROTOCOL(108)",
};

#ifdef NQPROT
static const char *svc_nqstrings[] =
{
	"nqsvc_bad",
	"nqsvc_nop",
	"nqsvc_disconnect",
	"nqsvc_updatestatlong",
	"nqsvc_version",		// [long] server version
	"nqsvc_setview",		// [short] entity number
	"nqsvc_sound",			// <see code>
	"nqsvc_time",			// [float] server time
	"nqsvc_print",			// [string] null terminated string
	"nqsvc_stufftext",		// [string] stuffed into client's console buffer
							// the string should be \n terminated
	"nqsvc_setangle",		// [vec3] set the view angle to this absolute value

	"nqsvc_serverinfo",		// [long] version
							// [string] signon string
							// [string]..[0]model cache [string]...[0]sounds cache
							// [string]..[0]item cache
	"nqsvc_lightstyle",		// [qbyte] [string]
	"nqsvc_updatename",		// [qbyte] [string]
	"nqsvc_updatefrags",	// [qbyte] [short]
	"nqsvc_clientdata",		// <shortbits + data>
	"nqsvc_stopsound",		// <see code>
	"nqsvc_updatecolors",	// [qbyte] [qbyte]
	"nqsvc_particle",		// [vec3] <variable>
	"nqsvc_damage",			// [qbyte] impact [qbyte] blood [vec3] from

	"nqsvc_spawnstatic",
	"ftenq_spawnstatic2(21)",
	"nqsvc_spawnbaseline",

	"nqsvc_temp_entity",	// <variable>
	"nqsvc_setpause",
	"nqsvc_signonnum",
	"nqsvc_centerprint",
	"nqsvc_killedmonster",
	"nqsvc_foundsecret",
	"nqsvc_spawnstaticsound",
	"nqsvc_intermission",
	"nqsvc_finale",			// [string] music [string] text
	"nqsvc_cdtrack",		// [qbyte] track [qbyte] looptrack
	"nqsvc_sellscreen",
	"nqsvc_cutscene",	//34

	"NEW PROTOCOL",	//35
	"NEW PROTOCOL",	//36
	"fitz_skybox",	//37
	"NEW PROTOCOL",	//38
	"NEW PROTOCOL",	//39
	"fitz_bf",		//40
	"fitz_fog",	//41
	"fitz_spawnbaseline2",	//42
	"fitz_spawnstatic2",	//43
	"fitz_spawnstaticsound2",	//44
	"NEW PROTOCOL",	//45
	"qex_updateping",	//46
	"qex_updatesocial",	//47
	"qex_updateplinfo",	//48
	"qex_print",	//49
	"dp_downloaddata / neh_skyboxsize / qex_servervars",		//50
	"dp_updatestatubyte / neh_fog / qex_seq",	//51
	"dp_effect / qex_achievement",				//52
	"dp_effect2 / qex_chat",			//53
	"dp6_precache / dp5_sound2 / qex_levelcompleted",	//54
	"dp_spawnbaseline2 / qex_backtolobby",		//55
	"dp_spawnstatic2 / qex_localsound",	//56 obsolete
	"dp_entities / qex_prompt",		//57
	"dp_csqcentities / qex_loccenterprint",			//58
	"dp_spawnstaticsound2",	//59
	"dp_trailparticles",	//60
	"dp_pointparticles",	//61
	"dp_pointparticles1",	//62
	"NEW PROTOCOL(63)",	//63
	"NEW PROTOCOL(64)",	//64
	"NEW PROTOCOL(65)",	//65
	"ftenq_spawnbaseline2",	//66
	"NEW PROTOCOL(67)",	//67
	"NEW PROTOCOL(68)",	//68
	"NEW PROTOCOL(69)",	//69
	"NEW PROTOCOL(70)",	//70
	"NEW PROTOCOL(71)",	//71
	"NEW PROTOCOL(72)",	//72
	"NEW PROTOCOL(73)",	//73
	"NEW PROTOCOL(74)",	//74
	"NEW PROTOCOL(75)",	//75
	"NEW PROTOCOL(76)",	//76
	"NEW PROTOCOL(77)",	//77
	"ftenq_updatestatstring",	//78
	"ftenq_updatestatfloat",	//79
	"NEW PROTOCOL(80)",	//80
	"NEW PROTOCOL(81)",	//81
	"NEW PROTOCOL(82)",	//82
	"ftenq_cgamepacket",	//83
	"ftenq_voicechat",	//84
	"ftenq_setangledelta",	//85
	"ftenq_updateentities",	//86
	"NEW PROTOCOL(87)",	//87
	"NEW PROTOCOL(88)",	//88
	"ftenq_setinfoblob",			//89
	"ftenq_cgamepacket_sized",		//90
	"ftenq_temp_entity_sized",		//91
	"ftenq_csqcentities_sized",	//92
};
#endif

extern cvar_t requiredownloads, mod_precache, snd_precache, cl_standardchat, msg_filter, msg_filter_frags, msg_filter_pickups, cl_countpendingpl, cl_download_mapsrc;
int	oldparsecountmod;
int	parsecountmod;
double	parsecounttime;

int		cl_spikeindex, cl_playerindex, cl_h_playerindex, cl_flagindex, cl_rocketindex, cl_grenadeindex, cl_gib1index, cl_gib2index, cl_gib3index;

//called after disconnect, purges all memory that was allocated etc
void CL_Parse_Disconnected(void)
{
	if (cls.download)
	{
		//note: not all downloads abort when the server disconnects, as they're fully out of bounds (ie: http)
		if (cls.download->method <= DL_QWPENDING)
			DL_Abort(cls.download, QDL_DISCONNECT);
	}

	{
		downloadlist_t *next;
		while(cl.downloadlist)
		{
			next = cl.downloadlist->next;
			Z_Free(cl.downloadlist);
			cl.downloadlist = next;
		}
		while(cl.faileddownloads)
		{
			next = cl.faileddownloads->next;
			Z_Free(cl.faileddownloads);
			cl.faileddownloads = next;
		}
	}

	CL_ClearParseState();
}

//=============================================================================

float packet_latency[NET_TIMINGS];

int CL_CalcNet (float scale)
{
	int		i;
	outframe_t	*frame;
	int lost = 0;
	int percent;
	int sent;
//	char st[80];

	sent = NET_TIMINGS;

	for (i=cl.movesequence-UPDATE_BACKUP+1
		; i <= cl.movesequence
		; i++)
	{
		frame = &cl.outframes[i&UPDATE_MASK];
		if (i > cl.ackedmovesequence)
		{
			// no response yet
			if (cl_countpendingpl.ival)
			{
				packet_latency[i&NET_TIMINGSMASK] = 9999;
				lost++;
			}
			else
				packet_latency[i&NET_TIMINGSMASK] = 10000;
		}
		else if (frame->latency == -1)
		{
			packet_latency[i&NET_TIMINGSMASK] = 9999;	// dropped
			lost++;
		}
		else if (frame->latency == -2)
			packet_latency[i&NET_TIMINGSMASK] = 10000;	// choked
		else if (frame->latency == -3)
		{
			packet_latency[i&NET_TIMINGSMASK] = 9997;	// c2spps
			sent--;
		}
//		else if (frame->invalid)
//			packet_latency[i&NET_TIMINGSMASK] = 9998;	// invalid delta
		else
			packet_latency[i&NET_TIMINGSMASK] = frame->latency * 60 * scale;
	}

	if (sent < 1)
		percent = 100;	//shouldn't ever happen.
	else
		percent = lost * 100 / sent;

	return percent;
}

void CL_CalcNet2 (float *pings, float *pings_min, float *pings_max, float *pingms_stddev, float *pingfr, int *pingfr_min, int *pingfr_max, float *dropped, float *choked, float *invalid)
{
	int		i;
	outframe_t	*frame;
	int lost = 0;
	int pending = 0;
	int sent;
	int valid = 0;
	int fr;
	int nchoked = 0;
	int ninvalid = 0;
//	char st[80];

	*pings = 0;
	*pings_max = 0;
	*pings_min = FLT_MAX;
	*pingfr = 0;
	*pingfr_max = 0;
	*pingfr_min = 0x7fffffff;
	*pingms_stddev = 0;


	sent = NET_TIMINGS;

	for (i=cl.movesequence-UPDATE_BACKUP+1
		; i <= cl.movesequence
		; i++)
	{
		frame = &cl.outframes[i&UPDATE_MASK];
		if (i > cl.lastackedmovesequence)
		{	// no response yet
			if (cl_countpendingpl.ival)
				lost++;
		}
		else if (frame->latency == -1)
			lost++;										// lost
		else if (frame->latency == -2)
			nchoked++;									// choked
		else if (frame->latency == -3)
			sent--;										// c2spps
		else if (frame->latency == -4)
			ninvalid++;									//corrupt/wrong/dodgy/egads
		else
		{
			*pings += frame->latency;
			if (*pings_max < frame->latency)
				*pings_max = frame->latency;
			if (*pings_min > frame->latency)
				*pings_min = frame->latency;

			fr = frame->cmd_sequence-frame->server_message_num;
			*pingfr += fr;
			if (*pingfr_max < fr)
				*pingfr_max = fr;
			if (*pingfr_min > fr)
				*pingfr_min = fr;
			valid++;
		}
	}

	if (valid)
	{
		*pings /= valid;
		*pingfr /= valid;

		//determine stddev, in milliseconds instead of seconds.
		for (i=cl.movesequence-UPDATE_BACKUP+1; i <= cl.movesequence; i++)
		{
			frame = &cl.outframes[i&UPDATE_MASK];
			if (i <= cl.lastackedmovesequence && frame->latency >= 0)
			{
				float dev = (frame->latency - *pings) * 1000;
				*pingms_stddev += dev*dev;
			}
		}
		*pingms_stddev = sqrt(*pingms_stddev/valid);
	}

	if (pending == sent || sent < 1)
		*dropped = 1;	//shouldn't ever happen.
	else
		*dropped = (float)lost / sent;
	*choked = (float)nchoked / sent;
	*invalid = (float)ninvalid / sent;
}

void CL_AckedInputFrame(int inseq, int outseq, qboolean worldstateokay)
{
	unsigned int i;
	unsigned int newmod;
	outframe_t *frame;

	//calc the latency for this frame, but only if its not a dupe ack. we want the youngest, not the oldest, so we can calculate network latency rather than simply packet frequency
	if (outseq != cl.lastackedmovesequence)
	{
		newmod = outseq & UPDATE_MASK;
		frame = &cl.outframes[newmod];
	// calculate latency
		frame->latency = realtime - frame->senttime;
		if (frame->latency < 0 || frame->latency > 1.0)
		{
	//		Con_Printf ("Odd latency: %5.2f\n", latency);
		}
		else
		{
		// drift the average latency towards the observed latency
			if (frame->latency < cls.latency)
				cls.latency = frame->latency;
			else
				cls.latency += 0.001;	// drift up, so correction are needed
		}

		if (cls.protocol != CP_NETQUAKE && cl.inframes[inseq&UPDATE_MASK].invalid)
			frame->latency = -4;

		//and mark any missing ones as dropped
		for (i = (cl.lastackedmovesequence+1) & UPDATE_MASK; i != newmod; i=(i+1)&UPDATE_MASK)
		{
//nq has no concept of choking. outbound packets that are accepted during a single frame will be erroneoulsy considered dropped. nq never had a netgraph based upon outgoing timings.
//			Con_Printf("Dropped moveframe %i\n", i);
			if (cls.protocol == CP_NETQUAKE && CPNQ_IS_DP)
			{	//dp doesn't ack every single packet. trying to report packet loss correctly is futile, we'll just get bad-mouthed.
				cl.outframes[i].latency = -2;	//flag as choked
			}
			else
				cl.outframes[i].latency = -1;	//flag as dropped
		}
	}
	cl.inframes[inseq&UPDATE_MASK].ackframe = outseq;
	if (worldstateokay)
		cl.ackedmovesequence = outseq;
	cl.lastackedmovesequence = outseq;
}

//=============================================================================

int CL_IsDownloading(const char *localname)
{
	downloadlist_t *dl;
	/*check for dupes*/
	for (dl = cl.downloadlist; dl; dl = dl->next)	//It's already on our list. Ignore it.
	{
		if (!strcmp(dl->localname, localname))
			return 2;	//queued
	}

	if (cls.download)
		if (!strcmp(cls.download->localname, localname))
			return 1;	//downloading
	return 0;
}

//note: this will overwrite existing files.
//returns true if the download is going to be downloaded after the call.
qboolean CL_EnqueDownload(const char *filename, const char *localname, unsigned int flags)
{
	extern cvar_t cl_downloads;
	downloadlist_t *dl;
	qboolean webdl = false;
	char ext[8];
	if ((flags & DLLF_TRYWEB) || !strncmp(filename, "http://", 7) || !strncmp(filename, "https://", 8))
	{
		flags |= DLLF_TRYWEB;
		if (!localname)
			return false;

		webdl = true;
	}
	else
	{
		if (!localname)
			localname = filename;

		if (cls.state < ca_connected)
			return false;
		if (cls.demoplayback && !(cls.demoplayback == DPB_MVD && (cls.demoeztv_ext&EZTV_DOWNLOAD)))
			return false;
	}
	COM_FileExtension(localname, ext, sizeof(ext));
	if (!stricmp(ext, "dll") || !stricmp(ext, "so") || strchr(localname, '\\') || strchr(localname, ':') || strstr(localname, ".."))
	{
		CL_DownloadFailed(filename, NULL, DLFAIL_UNTRIED);
		Con_Printf("Denying download of \"%s\"\n", filename);
		return false;
	}

	if (!(flags & DLLF_USEREXPLICIT) && !cl_downloads.ival)
	{
		CL_DownloadFailed(filename, NULL, DLFAIL_CLIENTCVAR);
		if (flags & DLLF_VERBOSE)
			Con_Printf("cl_downloads setting prevents download of \"%s\"\n", filename);
		return false;
	}

	/*reject if it already failed*/
	if (!(flags & DLLF_IGNOREFAILED))
	{
#ifdef NQPROT
		if (!webdl && cls.protocol == CP_NETQUAKE)
			if (!cl_dp_serverextension_download)
			{
				CL_DownloadFailed(filename, NULL, DLFAIL_UNSUPPORTED);
				return false;
			}
#endif

		for (dl = cl.faileddownloads; dl; dl = dl->next)	//yeah, so it failed... Ignore it.
		{
			if (!strcmp(dl->rname, filename))
			{
				if (flags & DLLF_VERBOSE)
					Con_Printf("We've failed to download \"%s\" already\n", filename);
				return false;
			}
		}
	}

	/*check for dupes*/
	switch(CL_IsDownloading(localname))
	{
	case 2:
		if (flags & DLLF_VERBOSE)
			Con_Printf("Already waiting for \"%s\"\n", filename);
		return true;
	default:
	case 1:
		if (flags & DLLF_VERBOSE)
			Con_Printf("Already downloading \"%s\"\n", filename);
		return true;
	case 0:
		break;
	}

	if (!*filename)
	{
		Con_Printf("Download \"\"? Huh?\n");
		return true;
	}

	dl = Z_Malloc(sizeof(downloadlist_t));
	Q_strncpyz(dl->rname, filename, sizeof(dl->rname));
	Q_strncpyz(dl->localname, localname, sizeof(dl->localname));
	dl->next = cl.downloadlist;
	dl->size = 0;
	dl->flags = flags | DLLF_SIZEUNKNOWN;

	if (!cl.downloadlist)
		flags &= ~DLLF_VERBOSE;

	cl.downloadlist = dl;

	if (!webdl && (cls.fteprotocolextensions & (PEXT_CHUNKEDDOWNLOADS
#ifdef PEXT_PK3DOWNLOADS
		| PEXT_PK3DOWNLOADS
#endif
		)) && !(dl->flags & DLLF_TEMPORARY))
	{
		CL_SendClientCommand(true, "dlsize \"%s\"", dl->rname);
	}

	if (flags & DLLF_VERBOSE)
		Con_Printf("Enqued download of \"%s\"\n", filename);

	return true;
}

void CL_GetDownloadSizes(unsigned int *filecount, qofs_t *totalsize, qboolean *somesizesunknown)
{
	downloadlist_t *dl;
	qdownload_t *d;
	*filecount = 0;
	*totalsize = 0;
	*somesizesunknown = false;
	for(dl = cl.downloadlist; dl; dl = dl->next)
	{
		*filecount += 1;
		if (dl->flags & DLLF_SIZEUNKNOWN)
			*somesizesunknown = true;
		else
			*totalsize += dl->size;
	}

	d = cls.download;
	if (d)
	{
		if (d->sizeunknown)
			*somesizesunknown = true;
		*totalsize += d->size;
	}
}

static void CL_DisenqueDownload(char *filename)
{
	downloadlist_t *dl, *nxt;
	if(cl.downloadlist)	//remove from enqued download list
	{
		if (!strcmp(cl.downloadlist->rname, filename))
		{
			dl = cl.downloadlist;
			cl.downloadlist = cl.downloadlist->next;
			Z_Free(dl);
		}
		else
		{
			for (dl = cl.downloadlist; dl->next; dl = dl->next)
			{
				if (!strcmp(dl->next->rname, filename))
				{
					nxt = dl->next->next;
					Z_Free(dl->next);
					dl->next = nxt;
					break;
				}
			}
		}
	}
}

#ifdef WEBCLIENT
static void CL_WebDownloadFinished(struct dl_download *dl)
{
	if (dl->status == DL_FAILED)
	{
		if (dl->replycode == 404)	//regular file-not-found
			CL_DownloadFailed(dl->url, &dl->qdownload, DLFAIL_SERVERFILE);
		else	//other stuff is PROBABLY 403forbidden, but lets blame the server's config if its a tls issue etc.
			CL_DownloadFailed(dl->url, &dl->qdownload, DLFAIL_SERVERCVAR);
		if (dl->qdownload.flags & DLLF_ALLOWWEB)	//re-enqueue it if allowed, but this time not from the web server.
			CL_EnqueDownload(dl->qdownload.localname, dl->qdownload.localname, dl->qdownload.flags & ~(DLLF_ALLOWWEB|DLLF_TRYWEB));
	}
	else if (dl->status == DL_FINISHED)
	{
		if (dl->file)
			VFS_CLOSE(dl->file);
		dl->file = NULL;
		CL_DownloadFinished(&dl->qdownload);
	}
}
#endif

static void CL_SendDownloadStartRequest(downloadlist_t *pending)
{
	char *filename = pending->rname;
	char *localname = pending->localname;
	unsigned int flags = pending->flags;
	static int dlsequence;
	qdownload_t *dl;

	//don't download multiple things at once... its leaky if nothing else.
	if (cls.download)
		return;

#ifdef WEBCLIENT
	if (flags & DLLF_TRYWEB)
	{
		struct dl_download *wdl = HTTP_CL_Get(filename, localname, CL_WebDownloadFinished);
		if (wdl)
		{
			if (flags & DLLF_NONGAME)
			{
				wdl->fsroot = FS_ROOT;
				if (!strncmp(localname, "package/", 8))
					Q_strncpyz(wdl->localname, localname+8, sizeof(wdl->localname));
			}
			if (!(flags & DLLF_TEMPORARY))
				Con_TPrintf ("Downloading %s to %s...\n", wdl->url, wdl->localname);
			wdl->qdownload.flags = flags;

			CL_DisenqueDownload(filename);

			cls.download = &wdl->qdownload;
		}
		else
			CL_DownloadFailed(filename, NULL, DLFAIL_CLIENTCVAR);
		return;
	}
#endif
	
	dl = Z_Malloc(sizeof(*dl));
	dl->filesequence = ++dlsequence;

	Q_strncpyz(dl->remotename, filename, sizeof(dl->remotename));
	Q_strncpyz(dl->localname, localname, sizeof(dl->localname));
	if (!(flags & DLLF_TEMPORARY))
		Con_TPrintf ("Downloading %s...\n", dl->localname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (localname, dl->tempname, sizeof(dl->tempname)-5);
	Q_strncatz (dl->tempname, ".tmp", sizeof(dl->tempname));

#ifdef AVAIL_ZLIB
	if (cls.protocol == CP_QUAKE2 && cls.protocol_q2 == PROTOCOL_VERSION_R1Q2)
		CL_SendClientCommand(true, "download %s 0 udp-zlib", filename);
	else
#endif
		CL_SendClientCommand(true, "download %s", filename);

	dl->method = DL_QWPENDING;
	dl->percent = 0;
	dl->sizeunknown = true;
	dl->flags = flags&DLLF_OVERWRITE;

	CL_DisenqueDownload(filename);

	cls.download = dl;
}

//Do any reloading for the file that just reloaded.
void CL_DownloadFinished(qdownload_t *dl)
{
	int i;
	char ext[8];

	char filename[MAX_QPATH];
	char tempname[MAX_QPATH];

	Q_strncpyz(filename, dl->localname, sizeof(filename));
	Q_strncpyz(tempname, dl->tempname, sizeof(tempname));

	DL_Abort(dl, QDL_COMPLETED);

	FS_FlushFSHashWritten(filename);

	COM_FileExtension(filename, ext, sizeof(ext));


	//should probably ask the filesytem code if its a package format instead.
	if (!strncmp(filename, "package/", 8) || !strncmp(ext, "pk4", 3) || !strncmp(ext, "pk3", 3) || !strncmp(ext, "pak", 3) || (dl->fsroot == FS_ROOT))
	{
		FS_ReloadPackFiles();
		CL_CheckServerInfo();
	}
	else if (!strcmp(filename, "gfx/palette.lmp"))
	{
		Cbuf_AddText("vid_restart\n", RESTRICT_LOCAL);
	}
	else
	{
		CL_CheckModelResources(filename);

		Mod_FileWritten(filename);
		{
			for (i = 0; i < MAX_PRECACHE_MODELS; i++)	//go and load this model now.
			{
				if (cl.model_name[i] && !strcmp(cl.model_name[i], filename))
				{
					if (cl.model_precache[i] && cl.model_precache[i]->loadstate == MLS_FAILED)
						cl.model_precache[i]->loadstate = MLS_NOTLOADED;
					CL_CheckModelResources(cl.model_name[i]);
					cl.model_precache[i] = Mod_ForName(cl.model_name[i], ((i==1)?MLV_WARNSYNC:MLV_WARN));
					if (i == 1)
					{
						cl.worldmodel = cl.model_precache[i];
						//just in case.
						if (cl.model_precache[1] && cl.model_precache[1]->loadstate == MLS_LOADED)
							FS_LoadMapPackFile(cl.model_precache[1]->name, cl.model_precache[1]->archive);
					}
					break;
				}
			}
			for (i = 0; i < MAX_CSMODELS; i++)	//go and load this model now.
			{
				if (!strcmp(cl.model_csqcname[i], filename))
				{
					if (cl.model_csqcprecache[i] && cl.model_csqcprecache[i]->loadstate == MLS_FAILED)
						cl.model_csqcprecache[i]->loadstate = MLS_NOTLOADED;
					CL_CheckModelResources(cl.model_csqcname[i]);
					cl.model_csqcprecache[i] = Mod_ForName(cl.model_csqcname[i], MLV_WARN);
					break;
				}
			}
#ifdef HAVE_LEGACY
			for (i = 0; i < MAX_VWEP_MODELS; i++)
			{
				if (cl.model_name_vwep[i] && !strcmp(cl.model_name_vwep[i], filename))
				{
					if (cl.model_precache_vwep[i] && cl.model_precache_vwep[i]->loadstate == MLS_FAILED)
						cl.model_precache_vwep[i]->loadstate = MLS_NOTLOADED;
					CL_CheckModelResources(cl.model_name_vwep[i]);
					cl.model_precache_vwep[i] = Mod_ForName(cl.model_name_vwep[i], MLV_WARN);
					break;
				}
			}
#endif
		}
		S_ResetFailedLoad();	//okay, so this can still get a little spammy in bad places...

#ifdef QWSKINS
		//this'll do the magic for us
		Skin_FlushSkin(filename);
#endif
	}
}

static qboolean CL_CheckFile(const char *filename)
{
	if (strstr (filename, ".."))
	{
		Con_TPrintf ("Refusing to download a path with ..\n");
		return true;
	}

	if (COM_FCheckExists (filename))
	{	// it exists, no need to download
		return true;
	}
	return false;
}

qboolean CL_CheckDLFile(const char *filename)
{
	if (!strncmp(filename, "package/", 8))
	{
		vfsfile_t *f;
		f = FS_OpenVFS(filename+8, "rb", FS_ROOT);
		if (f)
		{
			VFS_CLOSE(f);
			return true;
		}
		return false;
	}
	else
		return COM_FCheckExists(filename);
}
/*
===============
CL_CheckOrEnqueDownloadFile

Returns true if the file exists, returns false if it triggered a download.
===============
*/

qboolean	CL_CheckOrEnqueDownloadFile (const char *filename, const char *localname, unsigned int flags)
{	//returns false if we don't have the file yet.
	COM_AssertMainThread("CL_CheckOrEnqueDownloadFile");
	if (flags & DLLF_NONGAME)
	{
		/*pak/pk3 downloads have an explicit leading package/ as an internal/network marker*/
		if (!strchr(filename, ':'))
			filename = va("package/%s", filename);
		localname = va("package/%s", localname);
	}
	/*files with a leading * should not be downloaded (inline models, sexed sounds, etc). also block anyone trying to explicitly download a package/ because our code (wrongly) uses that name internally*/
	else if (*filename == '*' || !strncmp(filename, "package/", 8))
		return true;

	if (!localname)
		localname = filename;

#ifndef CLIENTONLY
	/*no downloading if we're the one we'd be downloading from*/
	if (sv.state)
		return true;
#endif

	if (!(flags & DLLF_OVERWRITE))
	{
		if (CL_CheckDLFile(localname))
			return true;
	}

	//ZOID - can't download when recording
	if (cls.demorecording)
	{
		Con_TPrintf ("Unable to download %s in record mode.\n", filename);
#if defined(MVD_RECORDING) && defined(HAVE_SERVER)
		if (sv_demoAutoRecord.ival)
			Con_TPrintf ("Note that ^[%s\\cmd\\%s 0\\^] is enabled.\n", sv_demoAutoRecord.name, sv_demoAutoRecord.name);
#endif
		return true;
	}
	//ZOID - can't download when playback
//	if (cls.demoplayback && cls.demoplayback != DPB_EZTV)
//		return true;

	SCR_EndLoadingPlaque();	//release console.

	if (flags & DLLF_ALLOWWEB)
	{
		const char *dlURL = InfoBuf_ValueForKey(&cl.serverinfo, "sv_dlURL");
		if (!*dlURL)
			dlURL = cls.downloadurl;
		if (!*dlURL)
			dlURL = fs_dlURL.string;
		if (strncmp(dlURL, "http://", 7) && strncmp(dlURL, "https://", 8))
			dlURL = "";	//only allow http+https here. just paranoid.
		flags &= ~(DLLF_TRYWEB|DLLF_ALLOWWEB);
		if (*dlURL && (flags & DLLF_NONGAME) && !strncmp(filename, "package/", 8))
		{	//filename is something like: package/GAMEDIR/foo.pk3
			filename = va("%s%s%s", dlURL, ((dlURL[strlen(dlURL)-1]=='/')?"":"/"), filename+8);
			flags |= DLLF_TRYWEB|DLLF_ALLOWWEB;
		}
		else if (*dlURL)
		{	//we don't really know which gamedir its meant to be for...
#ifdef Q2CLIENT	//ffs
			if (!strncmp(filename, "pics/../", 8))
				filename += 8;
#endif
			filename = va("%s%s%s/%s", dlURL, ((dlURL[strlen(dlURL)-1]=='/')?"":"/"), FS_GetGamedir(true), filename);
			flags |= DLLF_TRYWEB|DLLF_ALLOWWEB;
		}
		else if (*cl_download_mapsrc.string &&
			!strcmp(filename, localname) &&
			!strncmp(filename, "maps/", 5) &&
			!strcmp(filename + strlen(filename)-4, ".bsp"))
		{
			char base[MAX_QPATH];
			COM_FileBase(filename, base, sizeof(base));
#ifndef FTE_TARGET_WEB //don't care about prefixes in the web build, for site-relative uris.
			if (strncmp(cl_download_mapsrc.string, "http://", 7) && strncmp(cl_download_mapsrc.string, "https://", 8))
			{
				Con_Printf("%s: Scheme not specified, assuming https.\n", cl_download_mapsrc.name);
				filename = va("https://%s/%s", cl_download_mapsrc.string, filename+5);
			}
			else
#endif
				filename = va("%s%s", cl_download_mapsrc.string, filename+5);
			flags |= DLLF_TRYWEB|DLLF_ALLOWWEB;
		}
	}

	if (!CL_EnqueDownload(filename, localname, flags))
		return true;	/*don't stall waiting for it if it failed*/


	if (!(flags & DLLF_IGNOREFAILED))
	{
		downloadlist_t *dl;
		for (dl = cl.faileddownloads; dl; dl = dl->next)
		{
			if (!strcmp(dl->rname, filename))
			{
				//if its on the failed list, don't block waiting for it to download
				return true;
			}
		}
	}
	return false;
}



static qboolean CL_CheckMD2Skins (qbyte *precache_model)
{
	qboolean ret = false;
	md2_t *pheader;
	int skin = 1;
	char *str;

	pheader = (md2_t *)precache_model;
	if (LittleLong (pheader->version) != MD2ALIAS_VERSION)
	{
		//bad version.
		return false;
	}

	pheader = (md2_t *)precache_model;
	for (skin = 0; skin < LittleLong(pheader->num_skins); skin++)
	{
		str = (char *)precache_model +
			LittleLong(pheader->ofs_skins) +
			skin*MD2MAX_SKINNAME;
		COM_CleanUpPath(str);
		if (!CL_CheckOrEnqueDownloadFile(str, str, 0))
			ret = true;
	}
	return ret;
}

static qboolean CL_CheckHLBspWads(char *file)
{
	lump_t lump;
	dheader_t *dh;
	char *s;
	char *w;
	char key[256];
	char wads[4096];
	dh = (dheader_t *)file;

	lump.fileofs = LittleLong(dh->lumps[LUMP_ENTITIES].fileofs);
	lump.filelen = LittleLong(dh->lumps[LUMP_ENTITIES].filelen);

	s = file + lump.fileofs;

	s = COM_Parse(s);
	if (strcmp(com_token, "{"))
		return false;

	while (*s)
	{
		s = COM_ParseOut(s, key, sizeof(key));
		if (!strcmp(key, "}"))
			break;

		s = COM_ParseOut(s, wads, sizeof(wads));

		if (!strcmp(key, "wad"))
		{
			s = wads;
			while ((s = COM_ParseToken(s, ";")))
			{
				if (!strcmp(com_token, ";"))
					continue;
				while ((w = strchr(com_token, '\\')))
					*w = '/';
				w = COM_SkipPath(com_token);
				if (!CL_CheckFile(w))
				{
					Con_Printf("missing wad: %s\n", w);
					CL_CheckOrEnqueDownloadFile(va("textures/%s", w), NULL, DLLF_REQUIRED);
				}
			}
			return false;
		}
	}
	return false;
}

static qboolean CL_CheckQ2BspWals(char *file)
{
	qboolean gotone = false;
#ifdef Q2BSPS
	q2dheader_t *dh;
	lump_t lump;
	q2texinfo_t *tinf;
	unsigned int i, j, count;

	dh = (q2dheader_t*)file;
	if (LittleLong(dh->version) != BSPVERSION_Q2)
	{
		//quake3? unknown?
		return false;
	}
	lump.fileofs = LittleLong(dh->lumps[Q2LUMP_TEXINFO].fileofs);
	lump.filelen = LittleLong(dh->lumps[Q2LUMP_TEXINFO].filelen);

	count = lump.filelen / sizeof(*tinf);
	if (lump.filelen != count*sizeof(*tinf))
		return false;

	//grab the appropriate palette, just in case... but only if this won't confuse anything.
	if (CL_CheckDLFile("gfx/palette.lmp"))
		if (!CL_CheckOrEnqueDownloadFile("pics/colormap.pcx", NULL, 0))
			gotone = true;

	tinf = (q2texinfo_t*)(file + lump.fileofs);
	for (i = 0; i < count; i++)
	{
		//ignore duplicate files (to save filesystem hits)
		for (j = 0; j < i; j++)
			if (!strcmp(tinf[i].texture, tinf[j].texture))
				break;

		if (i == j)
		{	//note: we do support formats other than .wal but we still need the .wal to figure out the correct scaling.
			//we make a special exception for .tga-without-.wal because other q2 engines already expect that, with pre-scaled textures (and thus lightmaps too).
			if (!CL_CheckDLFile(va("textures/%s.wal", tinf[i].texture)))
				if (!CL_CheckDLFile(va("textures/%s.tga", tinf[i].texture)))
					if (!CL_CheckOrEnqueDownloadFile(va("textures/%s.wal", tinf[i].texture), NULL, DLLF_ALLOWWEB))
						gotone = true;
		}
	}

	//FIXME: parse entity lump for sky name.
#endif
	return gotone;
}

static qboolean CL_CheckModelResources (char *name)
{
	//returns true if we triggered a download
	qboolean ret;
	qbyte *file;

	if (!(strstr(name, ".md2") || strstr(name, ".bsp")))
		return false;

	// checking for skins in the model

	FS_LoadFile(name, (void **)&file);
	if (!file)
	{
		return false; // couldn't load it
	}
	if (!memcmp(file, MD2IDALIASHEADER))
		ret = CL_CheckMD2Skins(file);
	else if (!memcmp(file, BSPVERSIONHL))
		ret = CL_CheckHLBspWads(file);
	else if (!memcmp(file, IDBSPHEADER))
		ret = CL_CheckQ2BspWals(file);
	else
		ret = false;
	FS_FreeFile(file);

	return ret;
}

/*
=================
Model_NextDownload
=================
*/
static void Model_CheckDownloads (void)
{
	char	*s;
	int		i;
	char ext[8];

//	Con_TPrintf (TLC_CHECKINGMODELS);

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		for (i = 0; i < Q2MAX_IMAGES; i++)
		{
			char picname[256];
			if (!cl.image_name[i] || !*cl.image_name[i])
				continue;
#if defined(HAVE_LEGACY)
			Q_snprintfz(picname, sizeof(picname), "pics/%s.png", cl.image_name[i]);
			if (COM_FCheckExists(picname))
				return;
#endif
			Q_snprintfz(picname, sizeof(picname), "pics/%s.pcx", cl.image_name[i]);
			if (!strncmp(cl.image_name[i], "../", 3))	//some servers are just awkward.
				CL_CheckOrEnqueDownloadFile(picname, picname+8, DLLF_ALLOWWEB);
			else
				CL_CheckOrEnqueDownloadFile(picname, picname, DLLF_ALLOWWEB);
		}
		if (!CLQ2_RegisterTEntModels())
			return;
	}
#endif

	for (i = 1; i < countof(cl.model_name) && cl.model_name[i]; i++)
	{
		s = cl.model_name[i];
		if (s[0] == '*')
			continue;	// inline brush model

		if (!stricmp(COM_FileExtension(s, ext, sizeof(ext)), "dsp"))	//doom sprites are weird, and not really downloadable via this system
			continue;

#ifdef Q2CLIENT
		if (cls.protocol == CP_QUAKE2 && s[0] == '#')	//this is a vweap
			continue;
#endif

		CL_CheckOrEnqueDownloadFile(s, s, ((i==1)?DLLF_REQUIRED:0)|DLLF_ALLOWWEB);	//world is required to be loaded.
		CL_CheckModelResources(s);
	}

#ifdef HAVE_LEGACY
	for (i = 0; i < MAX_VWEP_MODELS; i++)
	{
		s = cl.model_name_vwep[i];
		if (!s)
			continue;
		if (!stricmp(COM_FileExtension(s, ext, sizeof(ext)), "dsp"))	//doom sprites are weird, and not really downloadable via this system
			continue;

		CL_CheckOrEnqueDownloadFile(s, s, DLLF_ALLOWWEB);
		CL_CheckModelResources(s);
	}
#endif
}

static int CL_LoadModels(int stage, qboolean dontactuallyload)
{
	int i;

	float giveuptime = Sys_DoubleTime()+1;	//small things get padded into a single frame

#define atstage() ((cl.contentstage == stage++ && !dontactuallyload)?true:false)
#define endstage() ++cl.contentstage;if (!cls.timedemo && giveuptime<Sys_DoubleTime()) return -1
#define skipstage() if (atstage())++cl.contentstage;else

	pmove.numphysent = 0;
	pmove.physents[0].model = NULL;

#if defined(CSQC_DAT) && defined(NQPROT)
	if (cls.protocol == CP_NETQUAKE && atstage())
	{	//we only need this for nq. for qw we checked for downloads with the other stuff.
		//there are also too many possible names to load... :(
		extern cvar_t  cl_nocsqc;
		if (!cl_nocsqc.ival && !cls.demoplayback)
		{
			const char *cscrc = InfoBuf_ValueForKey(&cl.serverinfo, "*csprogs");
			const char *cssize = InfoBuf_ValueForKey(&cl.serverinfo, "*csprogssize");
			const char *csname = InfoBuf_ValueForKey(&cl.serverinfo, "*csprogsname");
			unsigned int chksum = strtoul(cscrc, NULL, 0);
			size_t chksize = strtoul(cssize, NULL, 0);
			SCR_SetLoadingFile("csprogs");
			if (!*csname)
				csname = "csprogs.dat";
			if (*cscrc && !CSQC_CheckDownload(csname, chksum, chksize))	//only allow csqc if the server says so, and the 'checksum' matches.
			{
				extern cvar_t cl_download_csprogs;
				unsigned int chksum = strtoul(cscrc, NULL, 0);
				if (cl_download_csprogs.ival)
				{
					char *str = va("csprogsvers/%x.dat", chksum);
					if (CL_IsDownloading(str))
						return -1;	//don't progress to loading it while we're still downloading it.
					if (CL_CheckOrEnqueDownloadFile(csname, str, DLLF_REQUIRED))
						return -1;	//its kinda required
				}
				else
				{
					Con_Printf("Not downloading csprogs.dat due to %s\n", cl_download_csprogs.name);
				}
			}
		}
		endstage();
	}
#endif

#ifdef HLCLIENT
	if (atstage())
	{
		SCR_SetLoadingFile("hlclient");
		CLHL_LoadClientGame();
		endstage();
	}
#endif

#ifdef CSQC_DAT
	if (atstage())
	{
		char *s;
		qboolean anycsqc;
		char *endptr;
		unsigned int chksum;
		size_t progsize;
		const char *progsname;
		anycsqc = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "anycsqc"));
		if (cls.demoplayback)
			anycsqc = true;
		s = InfoBuf_ValueForKey(&cl.serverinfo, "*csprogssize");
		progsize = strtoul(s, NULL, 0);
		s = InfoBuf_ValueForKey(&cl.serverinfo, "*csprogs");
		chksum = strtoul(s, &endptr, 0);
		progsname = InfoBuf_ValueForKey(&cl.serverinfo, "*csprogsname");
		if (*endptr)
		{
			Con_Printf("corrupt *csprogs key in serverinfo\n");
			anycsqc = true;
			chksum = 0;
		}
		progsname = *s?InfoBuf_ValueForKey(&cl.serverinfo, "*csprogsname"):NULL;
		SCR_SetLoadingFile("csprogs");
		if (!CSQC_Init(anycsqc, progsname, chksum, progsize))
		{
			Sbar_Start();	//try and start this before we're actually on the server,
							//this'll stop the mod from sending so much stuffed data at us, whilst we're frozen while trying to load.
							//hopefully this'll make it more robust.
							//csqc is expected to use it's own huds, or to run on decent servers. :p
		}
		endstage();
	}
#endif

	if (atstage())
	{
		SCR_SetLoadingFile("prenewmap");
		Surf_PreNewMap();

		endstage();
	}

	if (cl.playerview[0].playernum == -1)
	{	//q2 cinematic - don't load the models.
		cl.worldmodel = cl.model_precache[1] = Mod_ForName ("", MLV_WARN);
	}
	else
	{
		for (i=1 ; i<MAX_PRECACHE_MODELS ; i++)
		{
			if (!cl.model_name[i])
				skipstage();
			else if (atstage())
			{
#if 0
				SCR_SetLoadingFile(cl.model_name[i]);
#ifdef CSQC_DAT
				if (i == 1)
					CSQC_LoadResource(cl.model_name[i], "map");
				else
					CSQC_LoadResource(cl.model_name[i], "model");
#endif
#endif
#ifdef Q2CLIENT
				if (cls.protocol == CP_QUAKE2 && *cl.model_name[i] == '#')
					cl.model_precache[i] = NULL;
				else
#endif
				if (!cls.timedemo && i!=1 && mod_precache.ival != 1)
					cl.model_precache[i] = Mod_FindName (Mod_FixName(cl.model_name[i], cl.model_name[1]));
				else
					cl.model_precache[i] = Mod_ForName (Mod_FixName(cl.model_name[i], cl.model_name[1]), MLV_WARN);

				S_ExtraUpdate();

				endstage();
			}
		}
#ifdef HAVE_LEGACY
		for (i = 0; i < MAX_VWEP_MODELS; i++)
		{
			if (!cl.model_name_vwep[i])
				skipstage();
			else if (atstage())
			{
#if 0
				SCR_SetLoadingFile(cl.model_name_vwep[i]);
#ifdef CSQC_DAT
				CSQC_LoadResource(cl.model_name_vwep[i], "vwep");
#endif
#endif
				cl.model_precache_vwep[i] = Mod_ForName (cl.model_name_vwep[i], MLV_WARN);
				endstage();
			}
		}
#endif
	}



	if (atstage())
	{
		cl.worldmodel = cl.model_precache[1];
		if (!cl.worldmodel || cl.worldmodel->type == mod_dummy)
		{
			if (!cl.model_name[1])
				Host_EndGame("Worldmodel name wasn't sent\n");
//			else
//				return stage;
//				Host_EndGame("Worldmodel wasn't loaded\n");
		}
		//the worldmodel can take a while to load, so be sure to wait.
		if (cl.worldmodel && cl.worldmodel->loadstate == MLS_LOADING)
			return -1;

		FS_LoadMapPackFile(cl.worldmodel->name, cl.worldmodel->archive);

		SCR_SetLoadingFile("csprogs world");

		endstage();
	}

	for (i=1 ; i<MAX_CSMODELS ; i++)
	{
		if (!cl.model_csqcname[i])
			skipstage();
		else if (atstage())
		{
#if 0
			SCR_SetLoadingFile(cl.model_csqcname[i]);
#ifdef CSQC_DAT
			if (i == 1)
				CSQC_LoadResource(cl.model_csqcname[i], "map");
			else
				CSQC_LoadResource(cl.model_csqcname[i], "model");
#endif
#endif
			cl.model_csqcprecache[i] = Mod_ForName (cl.model_csqcname[i], MLV_WARN);

			S_ExtraUpdate();

			endstage();
		}
	}

	if (atstage())
	{
		SCR_SetLoadingFile("wads");
		if (cl.worldmodel && cl.worldmodel->loadstate == MLS_LOADING)
			return -1;
		if (cl.worldmodel && cl.worldmodel->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(cl.worldmodel, &cl.worldmodel->loadstate, MLS_LOADING);
		Mod_ParseInfoFromEntityLump(cl.worldmodel);

		Wad_NextDownload();

		endstage();
	}

	if (atstage())
	{
		SCR_SetLoadingFile("external textures");
		if (cl.worldmodel && cl.worldmodel->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(cl.worldmodel, &cl.worldmodel->loadstate, MLS_LOADING);
		CL_CheckServerInfo(); //some serverinfo rules can change with map type, so make sure they're updated now we're sure we know it properly.
		if (cl.worldmodel && cl.worldmodel->loadstate == MLS_LOADED)
			Mod_NowLoadExternal(cl.worldmodel);

/*#ifdef Q2CLIENT
		if (cls.protocol == CP_QUAKE2 && cl.worldmodel && !cls.demoplayback &&
				cl.worldmodel->checksum  != cl.q2mapchecksum &&
				cl.worldmodel->checksum2 != cl.q2mapchecksum)
			Host_EndGame("Local map version differs from server: %i != '%i'\n", cl.worldmodel->checksum2, cl.q2mapchecksum);
#endif*/

		endstage();
	}


	// all done
	if (atstage())
	{
		SCR_SetLoadingFile("newmap");

//		if (!cl.worldmodel || cl.worldmodel->type == mod_dummy)
//			Host_EndGame("No worldmodel was loaded\n");
		Surf_NewMap (cl.worldmodel);

		pmove.physents[0].model = cl.worldmodel;

		endstage();
	}

#ifdef CSQC_DAT
	if (atstage())
	{		
		SCR_SetLoadingFile("csqc init");
		CSQC_WorldLoaded();

		if (CSQC_Inited())
		{
			if (cls.fteprotocolextensions & PEXT_CSQC)
				CL_SendClientCommand(true, "enablecsqc");
		}
		else
		{
			if (cls.fteprotocolextensions & PEXT_CSQC)
				CL_SendClientCommand(true, "disablecsqc");
		}
		endstage();
	}
#endif

	return stage;
}

static int CL_LoadSounds(int stage, qboolean dontactuallyload)
{
	int i;
	float giveuptime = Sys_DoubleTime()+0.1;	//small things get padded into a single frame

//#define atstage() ((cl.contentstage == stage++)?++cl.contentstage:false)
//#define endstage() if (giveuptime<Sys_DoubleTime()) return -1;

	for (i=1 ; i<MAX_PRECACHE_SOUNDS ; i++)
	{
		if (!cl.sound_name[i])
			break;

		if (atstage())
		{
#if 0
			SCR_SetLoadingFile(cl.sound_name[i]);
#ifdef CSQC_DAT
			CSQC_LoadResource(cl.sound_name[i], "sound");
#endif
#endif
			cl.sound_precache[i] = S_PrecacheSound (cl.sound_name[i]);

			S_ExtraUpdate();
			endstage();
		}
	}
	return stage;
}

void Sound_CheckDownload(const char *s)
{
#if defined(HAVE_LEGACY) && defined(AVAIL_OGGVORBIS)
	char mangled[512];
#endif
	if (*s == '*')	//q2 sexed sound
		return;

	if (!S_HaveOutput())
		return;

	//check without the sound/ prefix
	if (CL_CheckFile(s))
		return;	//we have it already

#if defined(HAVE_LEGACY) && defined(AVAIL_OGGVORBIS)
	//the things I do for nexuiz... *sigh*
	COM_StripExtension(s, mangled, sizeof(mangled));
	COM_DefaultExtension(mangled, ".ogg", sizeof(mangled));
	if (CL_CheckFile(mangled))
		return;
#endif

	//check with the sound/ prefix
	s = va("sound/%s",s);

	if (CL_CheckFile(s))
		return;	//we have it already

#if defined(HAVE_LEGACY) && defined(AVAIL_OGGVORBIS)
	//the things I do for nexuiz... *sigh*
	COM_StripExtension(s, mangled, sizeof(mangled));
	COM_DefaultExtension(mangled, ".ogg", sizeof(mangled));
	if (CL_CheckFile(mangled))
		return;
#endif
	//download the one the server said.
	CL_CheckOrEnqueDownloadFile(s, NULL, DLLF_ALLOWWEB);
}

/*
=================
Sound_NextDownload
=================
*/
static void Sound_CheckDownloads (void)
{
	int		i;


//	Con_TPrintf (TLC_CHECKINGSOUNDS);

#ifdef CSQC_DAT
//	if (cls.fteprotocolextensions & PEXT_CSQC)
	{
		char	*s;
		s = InfoBuf_ValueForKey(&cl.serverinfo, "*csprogs");
		if (*s)	//only allow csqc if the server says so, and the 'checksum' matches.
		{
			extern cvar_t cl_download_csprogs, cl_nocsqc;
			char *endptr;
			unsigned int chksum = strtoul(s, &endptr, 0);
			if (cl_nocsqc.ival || cls.demoplayback || *endptr)
			{
			}
			else if (cl_download_csprogs.ival)
			{
				char *str = va("csprogsvers/%x.dat", chksum);
				CL_CheckOrEnqueDownloadFile("csprogs.dat", str, DLLF_REQUIRED);
			}
			else
			{
				Con_Printf("Not downloading csprogs.dat\n");
			}
		}
	}
#endif

	for (i = 1; i < countof(cl.model_name) && cl.sound_name[i]; i++)
	{
		Sound_CheckDownload(cl.sound_name[i]);
	}
}

/*
======================
CL_RequestNextDownload
======================
*/
void CL_RequestNextDownload (void)
{

	int stage;
	/*already downloading*/
	if (cls.download && !cls.demoplayback)
		return;

	/*request downloads only if we're at the point where we've received a complete list of them*/
	if (cl.sendprespawn || cls.state == ca_active)
	{
		if (cl.downloadlist)
		{
			downloadlist_t *dl;

			//download required downloads first
			for (dl = cl.downloadlist; dl; dl = dl->next)
			{
				if (dl->flags & DLLF_NONGAME)
					break;
			}
			if (!dl)
			{
				for (dl = cl.downloadlist; dl; dl = dl->next)
				{
					if (dl->flags & DLLF_REQUIRED)
						break;
				}
				if (!dl)
					dl = cl.downloadlist;
			}

			/*if we don't require downloads don't queue requests until we're actually on the server, slightly more deterministic*/
			if (cls.state == ca_active || (requiredownloads.value && !(cls.demoplayback && !(dl->flags&DLLF_TRYWEB))) || (dl->flags & DLLF_REQUIRED))
			{
				if ((dl->flags & DLLF_OVERWRITE) || !CL_CheckFile (dl->localname))
				{
					CL_SendDownloadStartRequest(dl);
					return;
				}
				else
				{
					//we already got this file somehow? must have come from a pak or something. don't spam.
					Con_DPrintf("Already have %s\n", dl->localname);
					CL_DisenqueDownload(dl->rname);

					//recurse a bit.
					CL_RequestNextDownload();
					return;
				}
			}
		}
		else if (cls.download && requiredownloads.value)
			return;
	}

	if (cl.sendprespawn)
	{	// get next signon phase
		extern int total_loading_size, current_loading_size;

		if (!cl.contentstage)
		{
			int pure;
			stage = 0;
			stage = CL_LoadModels(stage, true);
			stage = CL_LoadSounds(stage, true);
			total_loading_size = stage;
			cl.contentstage = 0;

			//might be safer to do it later, but kinder to do it before wasting time.
			pure = FS_PureOkay();
			if (pure < 0 || (pure==0 && (cls.download || cl.downloadlist)))
				return;	//we're downloading something and may still be able to satisfy it.
			if (pure == 0 && !cls.demoplayback)
			{	//failure!
				Con_Printf(CON_ERROR"You are missing pure packages, and they could not be autodownloaded.\nYou may need to purchase an update.\n");
	#ifdef HAVE_MEDIA_ENCODER
				if (cls.demoplayback && Media_Capturing())
				{
					Con_Printf(CON_ERROR "Aborting capture\n");
					CL_StopPlayback();
				}
	#endif
				SCR_SetLoadingStage(LS_NONE);
				CL_Disconnect("Game Content differs from server");
				return;
			}
		}

		stage = 0;
		stage = CL_LoadModels(stage, false);
		current_loading_size = cl.contentstage;
		if (stage < 0)
			return;	//not yet
		stage = CL_LoadSounds(stage, false);
		current_loading_size = cl.contentstage;
		if (stage < 0)
			return;
		//if (cls.userinfosync.numkeys)
		//	return;	//don't prespawn until we've actually sent all our initial userinfo.
		if (requiredownloads.ival && COM_HasWork())
		{
			SCR_SetLoadingFile("loading content");
			return;
		}
		SCR_SetLoadingFile("receiving game state");

		cl.sendprespawn = false;

		if (cl_splitscreen.ival)
		{
			if (cls.fteprotocolextensions & PEXT_SPLITSCREEN)
				;
			else if (cls.protocol == CP_QUAKE2 && cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
				;
			else
				Con_TPrintf(CON_WARNING "Splitscreen requested but not available on this server.\n");
		}

		if (cl.worldmodel && cl.worldmodel->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(cl.worldmodel, &cl.worldmodel->loadstate, MLS_LOADING);

		if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED)
		{
			downloadlist_t *dl = NULL;
			const char *worldname = cl.worldmodel?cl.worldmodel->name:"unknown";
			if (cl.worldmodel)
				for (dl = cl.faileddownloads; dl; dl = dl->next)	//yeah, so it failed... Ignore it.
					if (!strcmp(dl->rname, cl.worldmodel->name))
						break;
			Con_Printf("\n\n-------------\n");
			switch (dl?dl->failreason:DLFAIL_UNTRIED)
			{
			case DLFAIL_UNSUPPORTED:
				Con_Printf(CON_ERROR "Download of \"%s\" not supported on this server - cannot fully connect\n", worldname);
				break;
			case DLFAIL_CORRUPTED:
				Con_Printf(CON_ERROR "Download of \"%s\" corrupt/failed - cannot fully connect\n", worldname);
				break;
			case DLFAIL_CLIENTCVAR:
				Con_Printf(CON_ERROR "Downloading of \"%s\" blocked by clientside cvars - tweak cl_download* before retrying\n", worldname);
				break;
			case DLFAIL_CLIENTFILE:
				Con_Printf(CON_ERROR "Disk error downloading \"%s\" - cannot fully connect\n", worldname);
				break;
			case DLFAIL_SERVERCVAR:
				Con_Printf(CON_ERROR "Download of \"%s\" denied by server - cannot fully connect\n", worldname);
				break;
			case DLFAIL_SERVERFILE:
				Con_Printf(CON_ERROR "Download of \"%s\" unavailable - cannot fully connect\n", worldname);
				break;
			case DLFAIL_REDIRECTED:
				Con_Printf(CON_ERROR "Redirection failure downloading \"%s\" - cannot fully connect\n", worldname);
				break;
			case DLFAIL_UNTRIED:
				if (COM_FCheckExists(worldname))
				{
					if (!cl.worldmodel)
						Con_Printf(CON_ERROR "Couldn't load \"%s\" - worldmodel not set - cannot fully connect\n", worldname);
					else if (cl.worldmodel->loadstate == MLS_FAILED)
						Con_Printf(CON_ERROR "Couldn't load \"%s\" - corrupt? - cannot fully connect\n", worldname);
					else if (cl.worldmodel->loadstate == MLS_LOADING)
						Con_Printf(CON_ERROR "Couldn't load \"%s\" - still loading - cannot fully connect\n", worldname);
					else if (cl.worldmodel->loadstate == MLS_NOTLOADED)
						Con_Printf(CON_ERROR "Couldn't load \"%s\" - worldmodel not loaded - cannot fully connect\n", worldname);
					else
						Con_Printf(CON_ERROR "Couldn't load \"%s\" - corrupt? - cannot fully connect\n", worldname);
				}
				else
					Con_Printf(CON_ERROR "Couldn't find \"%s\" - cannot fully connect\n", worldname);
				break;
			}
#ifdef HAVE_MEDIA_ENCODER
			if (cls.demoplayback && Media_Capturing())
			{
				Con_Printf(CON_ERROR "Aborting capture\n");
				CL_StopPlayback();
			}
#endif
			//else should probably force the demo speed really fast or something

			SCR_SetLoadingStage(LS_NONE);
			return;
		}

		Cvar_ForceCallback(Cvar_FindVar("r_particlesdesc"));

#ifdef Q2CLIENT
		if (cls.protocol == CP_QUAKE2)
		{
			if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2)
			{	//fixme: make dynamic...
//				MSG_WriteByte (&cls.netchan.message, clcr1q2_setting);
//				MSG_WriteShort (&cls.netchan.message, R1Q2_CLSET_NOGUN);
//				MSG_WriteShort (&cls.netchan.message, r_drawviewmodel.value <= 0);

//				MSG_WriteByte (&cls.netchan.message, clcr1q2_setting);
//				MSG_WriteShort (&cls.netchan.message, R1Q2_CLSET_PLAYERUPDATES);
//				MSG_WriteShort (&cls.netchan.message, 1);

//				MSG_WriteByte (&cls.netchan.message, clcr1q2_setting);
//				MSG_WriteShort (&cls.netchan.message, R1Q2_CLSET_FPS);
//				MSG_WriteShort (&cls.netchan.message, 30);
			}

			Skin_NextDownload();
			SCR_SetLoadingStage(LS_NONE);
			CL_SendClientCommand(true, "begin %i\n", cl.servercount);
		}
		else
#endif
		{
			if (cls.demoplayback == DPB_MVD && cls.demoeztv_ext)
			{
				if (CL_RemoveClientCommands("qtvspawn"))
					Con_DPrintf("Multiple prespawns\n");
				CL_SendClientCommand(true, "qtvspawn %i 0 %i", cl.servercount, COM_RemapMapChecksum(cl.worldmodel, LittleLong(cl.worldmodel->checksum2)));
				SCR_SetLoadingStage(LS_NONE);
			}
			else
			{
		// done with modellist, request first of static signon messages
				if (CL_RemoveClientCommands("prespawn"))
					Con_DPrintf("Multiple prespawns\n");
				if (cls.protocol == CP_NETQUAKE)
					CL_SendClientCommand(true, "prespawn");
				else
				{
		//			CL_SendClientCommand("prespawn %i 0 %i", cl.servercount, cl.worldmodel->checksum2);
					CL_SendClientCommand(true, prespawn_name, cl.servercount, COM_RemapMapChecksum(cl.worldmodel, LittleLong(cl.worldmodel->checksum2)));
				}
			}
		}


		if (mod_precache.ival >= 2)
		{
			int i;
			for (i=1 ; i<MAX_PRECACHE_MODELS ; i++)
			{
				if (cl.model_precache[i] && cl.model_precache[i]->loadstate == MLS_NOTLOADED)
					Mod_LoadModel(cl.model_precache[i], MLV_WARN);
			}
		}
		if (snd_precache.ival >= 2)
		{
			int i;
			for (i=1 ; i<MAX_PRECACHE_SOUNDS ; i++)
			{
				if (cl.sound_precache[i] && cl.sound_precache[i]->loadstate == SLS_NOTLOADED)
					S_LoadSound(cl.sound_precache[i], false);
			}
		}
	}
}

int CL_RequestADownloadChunk(void);
void CL_SendDownloadReq(sizebuf_t *msg)
{
	if (cls.demoplayback == DPB_MVD)
		return;	//tcp connection, so no need to constantly ask

	if (!cls.download)
	{
		if (cl.downloadlist)
			CL_RequestNextDownload();
		return;
	}

#ifdef PEXT_CHUNKEDDOWNLOADS
	if (cls.download->method == DL_QWCHUNKS)
		DLC_Poll(cls.download);
#endif
}

#ifdef PEXT_ZLIBDL
#include <zlib.h>

static char *ZLibDownloadDecode(int *messagesize, char *input, int finalsize)
{
	char *outbuf = Hunk_TempAlloc(finalsize);
	z_stream zs;

	*messagesize = (*(short*)input);
	input+=2;

	if (!*messagesize)
	{
		*messagesize = finalsize+2;
		return input;
	}

	memset(&zs, 0, sizeof(zs));


	zs.next_in = input;
    zs.avail_in = *messagesize;	//tell it that it has a lot. Possibly a bad idea.
    zs.total_in = 0;

    zs.next_out = outbuf;
    zs.avail_out = finalsize;	//this is the limiter.
    zs.total_out = 0;

    zs.data_type = Z_BINARY;

	inflateInit(&zs);
	inflate(&zs, Z_FINISH);	//decompress it in one go.
	inflateEnd(&zs);

	*messagesize = zs.total_in+2;
	return outbuf;
}
#endif

downloadlist_t *CL_DownloadFailed(const char *name, qdownload_t *qdl, enum dlfailreason_e failreason)
{
	//add this to our failed list. (so we don't try downloading it again...)
	downloadlist_t *failed, **link, *dl;
	failed = Z_Malloc(sizeof(downloadlist_t));
	failed->next = cl.faileddownloads;
	cl.faileddownloads = failed;
	Q_strncpyz(failed->rname, name, sizeof(failed->rname));
	failed->failreason = failreason;

	//if this is what we're currently downloading, close it up now.
	//don't do this if we're just marking the file as unavailable for download.
	if (qdl && (!stricmp(qdl->remotename, name) || !*name))
	{
		DL_Abort(qdl, QDL_FAILED);
	}

	link = &cl.downloadlist;
	while(*link)
	{
		dl = *link;
		if (!strcmp(dl->rname, name))
		{
			*link = dl->next;
			failed->flags |= dl->flags;
			Z_Free(dl);
		}
		else
			link = &(*link)->next;
	}

	return failed;
}

#ifdef PEXT_CHUNKEDDOWNLOADS

int CL_DownloadRate(void)
{
	qdownload_t *dl = cls.download;
	if (dl)
	{
		double curtime = Sys_DoubleTime();
		if (!dl->ratetime)
		{
			dl->ratetime = curtime;
			return dl->completedbytes/(Sys_DoubleTime() - dl->starttime);
		}
		if (curtime - dl->ratetime > 1)
		{
			dl->rate = dl->ratebytes / (curtime - dl->ratetime);
			dl->ratetime = curtime;
			dl->ratebytes = 0;
		}
		return dl->rate;
	}
	return 0;
}

//called when the server acks the download. opens the local file and stuff. returns false on failure
qboolean DL_Begun(qdownload_t *dl)
{
	//figure out where the file is meant to be going.
	dl->prefixbytes = 0;
	if (!strncmp(dl->tempname, "package/", 8))
	{
		dl->prefixbytes = 8;	//ignore the package/ part
		dl->fsroot = FS_ROOT;	//and put it in the root dir (-basedir), and hope the name includes a gamedir part
	}
	else if (!strncmp(dl->tempname,"skins/",6))
		dl->fsroot = FS_PUBBASEGAMEONLY;	//shared between gamedirs, so only use the basegame.
	else
		dl->fsroot = FS_PUBGAMEONLY;//FS_GAMEONLY;	//other files are relative to the active gamedir.

	Q_snprintfz(dl->dclname, sizeof(dl->dclname), "%s.dcl", dl->tempname);

	if (dl->method == DL_QWCHUNKS)
	{
		qboolean error = false;
		char partline[256];
		char partterm[128];
		char *p, t;
		qofs_t lastend = 0;
		qofs_t start, end;
		struct dlblock_s **link = &dl->dlblocks;
		vfsfile_t *parts = FS_OpenVFS(dl->dclname+dl->prefixbytes, "rb", dl->fsroot);
		if (!parts)
			error = true;
		while(!error && VFS_GETS(parts, partline, sizeof(partline)))
		{
			p = COM_ParseOut(partline, partterm, sizeof(partterm));
			t = *partterm;
			p = COM_ParseOut(p, partterm, sizeof(partterm));
			start = strtoull(partterm, NULL, 0);
			p = COM_ParseOut(p, partterm, sizeof(partterm));
			end = strtoull(partterm, NULL, 0);

			(*link) = Z_Malloc(sizeof(**link));
			(*link)->start = start;
			(*link)->end = end;
			(*link)->state = (t == 'c')?DLB_RECEIVED:DLB_MISSING;
			link = &(*link)->next;

			if (t == 'c')
				dl->completedbytes += end - start;

			if (start != lastend)
				error = true;
			lastend = end;
		}
		if (lastend != dl->size)
			error = true;
		if (parts)
			VFS_CLOSE(parts);
		if (!error)
			dl->file = FS_OpenVFS(dl->tempname+dl->prefixbytes, "w+b", dl->fsroot);
	}
	if (!dl->file)
	{
		struct dlblock_s *b;
		//make sure we don't get confused if someone end-tasks us before the download is complete.
		FS_Remove(dl->dclname+dl->prefixbytes, dl->fsroot);
		dl->completedbytes = 0;
		while (dl->dlblocks)
		{
			b = dl->dlblocks;
			dl->dlblocks = b->next;
			Z_Free(b);
		}
		FS_CreatePath(dl->tempname+dl->prefixbytes, dl->fsroot);
		dl->file = FS_OpenVFS(dl->tempname+dl->prefixbytes, "wb", dl->fsroot);
	}
	if (!dl->file)
	{
		char displaypath[MAX_OSPATH];
		FS_DisplayPath(dl->tempname+dl->prefixbytes, dl->fsroot, displaypath, sizeof(displaypath));
		Con_TPrintf("Unable to open \"%s\"\n", displaypath);
		return false;
	}

	if (dl->method == DL_QWPENDING)
		Con_TPrintf("method is still 'pending'\n");

	if (dl->method == DL_QWCHUNKS && !dl->dlblocks)
	{
		dl->dlblocks = Z_Malloc(sizeof(*dl->dlblocks));
		dl->dlblocks->start = 0;
		dl->dlblocks->end = dl->size;
		dl->dlblocks->state = DLB_MISSING;
	}
	dl->flags |= DLLF_BEGUN;

	dl->starttime = Sys_DoubleTime();
	return true;
}

static void DL_Completed(qdownload_t *dl, qofs_t start, qofs_t end)
{
	struct dlblock_s *prev = NULL, *b, *n, *e;
	if (end <= start)
		return;	//ignore invalid ranges.
	for (b = dl->dlblocks; b; )
	{
		if (b->state == DLB_RECEIVED)
		{
			//nothing to be done. dupe. somehow. or simply a different range.
		}
		else if (b->start >= start && b->end <= end)
		{
			//whole block
//			Con_Printf("Whole block\n");
			b->state = DLB_RECEIVED;
			dl->completedbytes += b->end - b->start;
			dl->ratebytes += b->end - b->start;
		}
		else if (start > b->start && end < b->end)
		{
//			Con_Printf("chop out middle\n");
			//in the middle, no need to merge
			n = Z_Malloc(sizeof(*n));
			e = Z_Malloc(sizeof(*e));
			e->next = b->next;
			n->next = e;
			b->next = n;

			e->state = b->state;
			e->sequence = b->sequence;
			n->state = DLB_RECEIVED;

			e->end = b->end;
			b->end = start;
			n->start = start;
			n->end = end;
			e->start = end;

			dl->completedbytes += n->end - n->start;
			dl->ratebytes += n->end - n->start;
		}
		//data overlaps the start (data end must be smaller than block end)
		else if (start <= b->start && end > b->start)
		{
//			Con_Printf("complete start\n");

			//split it. new(non-complete) block is second.
			n = Z_Malloc(sizeof(*n));
			n->next = b->next;
			b->next = n;
			//second block keeps original block's state. first block gets completed
			n->state = b->state;
			n->sequence = b->sequence;
			b->state = DLB_RECEIVED;

			n->start = end;
			n->end = b->end;
			b->end = end;

			dl->completedbytes += b->end - b->start;
			dl->ratebytes += b->end - b->start;
		}
		//new data overlaps the end
		else if (start > b->start && start < b->end)
		{
//			Con_Printf("complete end\n");
			//split it. new(completed) block is second.
			n = Z_Malloc(sizeof(*n));
			n->next = b->next;
			b->next = n;
			//second block keeps original block's state. first block gets completed
			n->state = DLB_RECEIVED;
			n->sequence = 0;

			n->start = end;
			n->end = b->end;
			b->end = end;

			dl->completedbytes += n->end - n->start;
			dl->ratebytes += n->end - n->start;

			prev = b;
			b = n;
		}
		else
		{//don't bother merging, as nothing changed
			prev = b;
			b = b->next;
			continue;
		}

		//merge with next block
		if (b->next && b->next->state == DLB_RECEIVED)
		{
			n = b->next;
			b->next = n->next;
			b->end = n->end;
			Z_Free(n);
		}
		//merge with previous block if possible
		if (prev && prev->state == DLB_RECEIVED)
		{
			n = b;
			prev->end = b->end;
			prev->next = b->next;
			Z_Free(b);
			b = prev->next;
			continue;//careful here
		}
		prev = b;
		b = b->next;
	}
}

static float chunkrate;

static int CL_CountQueuedDownloads(void);
static void CL_ParseChunkedDownload(qdownload_t *dl)
{
	qbyte	*svname;
	int flag;
	qofs_t filesize;
	qofs_t chunknum;
	char data[DLBLOCKSIZE];

	chunknum = MSG_ReadLong();
	if (chunknum == -1)
	{
		flag = MSG_ReadLong();
		if (flag == 0x80000000)
		{	//really big files need special handling here.
			flag = MSG_ReadLong();
			filesize = qofs_Make(flag, MSG_ReadLong());
			flag = 0;
		}
		else
			filesize = flag;

		svname = MSG_ReadString();
		if (cls.demoplayback)
		{	//downloading in demos is allowed ONLY for csprogs.dat
			extern cvar_t cl_downloads, cl_download_csprogs;
			if (!cls.download && !dl &&
					!strcmp(svname, "csprogs.dat") && filesize && filesize == strtoul(InfoBuf_ValueForKey(&cl.serverinfo, "*csprogssize"), NULL, 0) &&
					cl_downloads.ival && cl_download_csprogs.ival)
			{
				//FIXME: should probably save this to memory instead of bloating it on disk.
				dl = Z_Malloc(sizeof(*dl));

				Q_strncpyz(dl->remotename, svname, sizeof(dl->remotename));
				Q_strncpyz(dl->localname, va("csprogsvers/%x.dat", (unsigned int)strtoul(InfoBuf_ValueForKey(&cl.serverinfo, "*csprogs"), NULL, 0)), sizeof(dl->localname));

				// download to a temp name, and only rename
				// to the real name when done, so if interrupted
				// a runt file wont be left
				COM_StripExtension (dl->localname, dl->tempname, sizeof(dl->tempname)-5);
				Q_strncatz (dl->tempname, ".tmp", sizeof(dl->tempname));

				dl->method = DL_QWPENDING;
				dl->percent = 0;
				dl->sizeunknown = true;
				dl->flags = DLLF_OVERWRITE;

				if (COM_FCheckExists(dl->localname))
				{
					Con_DPrintf("Demo embeds redundant %s\n", dl->localname);
					Z_Free(dl);
					return;
				}
				cls.download = dl;
				Con_Printf("Saving recorded file %s (%lu bytes)\n", dl->localname, (unsigned long)filesize);
			}
			else
				return;
		}

		if (!*svname)
		{
			//stupid mvdsv.
			/*if (totalsize < 0)
				svname = cls.downloadname;
			else*/
			{
				Con_Printf("ignoring nameless download\n");
				return;
			}
		}

		if (flag < 0)
		{
			enum dlfailreason_e failreason;
			if (flag == DLERR_REDIRECTFILE)
			{
				failreason = DLFAIL_REDIRECTED;
				if (CL_AllowArbitaryDownload(dl->remotename, svname))
				{
					Con_Printf("Download of \"%s\" redirected to \"%s\"\n", dl->remotename, svname);
					if (!strncmp(svname, "package/", 8))
					{
						int i, c;
						char *pn;
						Cmd_TokenizeString(cl.serverpacknames, false, false);
						c = Cmd_Argc();
						for (i = 0; i < c; i++)
						{
							pn = Cmd_Argv(i);
							if (*pn == '*')
								pn++;	//'required'... so shouldn't really be missing.
							if (!strcmp(pn, svname+8))
								break;
						}
						if (i == c)
							Con_Printf("However, package \"%s\" is unknown.\n", svname+8);
						else
						{
							char localname[MAX_OSPATH];
							char *hash;
							Cmd_TokenizeString(cl.serverpackhashes, false, false);
							hash = Cmd_Argv(i);
							if (FS_GenCachedPakName(svname+8, hash, localname, sizeof(localname)))
							{
								if (CL_CheckOrEnqueDownloadFile(svname+8, localname, DLLF_NONGAME))
									if (!CL_CheckDLFile(dl->localname))
										Con_Printf("However, \"%s\" already exists. You may need to delete it.\n", svname);
							}
							else
								Con_Printf("However, package \"%s\" is invalid.\n", svname+8);
						}
					}
					else if (CL_CheckOrEnqueDownloadFile(svname, NULL, 0))
					{
						Con_Printf("However, \"%s\" already exists. You may need to delete it.\n", svname);
						failreason = DLFAIL_CLIENTFILE;
					}
				}
				svname = dl->remotename;
			}
			else if (flag == DLERR_UNKNOWN)
			{
				Con_Printf("Server reported an error when downloading file \"%s\"\n", svname);
				failreason = DLFAIL_CORRUPTED;
			}
			else if (flag == DLERR_PERMISSIONS)
			{
				Con_Printf("Server permissions deny downloading file \"%s\"\n", svname);
				failreason = DLFAIL_SERVERCVAR;
			}
			else //if (flag == DLERR_FILENOTFOUND)
			{
				Con_Printf("Couldn't find file \"%s\" on the server\n", svname);
				failreason = DLFAIL_SERVERFILE;
			}

			if (dl)
			{
				CL_DownloadFailed(svname, dl, failreason);

				CL_RequestNextDownload();
			}
			return;
		}

		if (!dl)
		{
			Con_Printf("ignoring download start. we're not meant to be downloading\n");
			return;
		}

		if (dl->method == DL_QWCHUNKS)
			Host_EndGame("Received second download - \"%s\"\n", svname);

		if (stricmp(dl->remotename, svname))
		{
			//fixme: we should allow extension changes, in the case of ogg/mp3/wav, or tga/png/jpg/pcx, or the addition of .gz or whatever
			Host_EndGame("Server sent the wrong download - \"%s\" instead of \"%s\"\n", svname, dl->remotename);
		}


		//start the new download
		dl->method = DL_QWCHUNKS;
		dl->percent = 0;
		dl->size = filesize;
		dl->sizeunknown = false;

		dl->starttime = Sys_DoubleTime();

		/*
		strcpy(cls.downloadname, svname);
		COM_StripExtension(svname, cls.downloadtempname);
		COM_DefaultExtension(cls.downloadtempname, ".tmp");
		*/

		if (!DL_Begun(dl))
		{
			CL_DownloadFailed(svname, dl, DLFAIL_CLIENTFILE);
			return;
		}

		return;
	}

//	Con_Printf("Received dl block %i: ", chunknum);

	MSG_ReadData(data, DLBLOCKSIZE);

	if (!dl)
	{
		if (!cls.demoplayback)	//mute it in demos.
			Con_Printf("ignoring download data packet\n");
		return;
	}

	if (chunknum*DLBLOCKSIZE > dl->size+DLBLOCKSIZE)
		return;

	if (!dl->file)
		return;

	VFS_SEEK(dl->file, chunknum*DLBLOCKSIZE);
	if (dl->size - chunknum*DLBLOCKSIZE < DLBLOCKSIZE)	//final block is actually meant to be smaller than we recieve.
		VFS_WRITE(dl->file, data, dl->size - chunknum*DLBLOCKSIZE);
	else
		VFS_WRITE(dl->file, data, DLBLOCKSIZE);

	DL_Completed(dl, chunknum*DLBLOCKSIZE, (chunknum+1)*DLBLOCKSIZE);

	dl->percent = dl->completedbytes/(float)dl->size*100;

	chunkrate += 1;

	if (dl->completedbytes == dl->size)
		CL_DownloadFinished(dl);
}

static int CL_CountQueuedDownloads(void)
{
	int count = 0;
	downloadlist_t *dl;
	for (dl = cl.downloadlist; dl; dl = dl->next)
		count++;

	return count;
}

static void DLC_RequestDownloadChunks(qdownload_t *dl, float frametime)
{
	char *cmd;
	qofs_t chunk;
	struct dlblock_s *b, *n;
	qboolean stillpending = false;
	qboolean haveloss = false;
	int chunks, chunksremaining;
	static float slop;	//try to keep things as integers
//	int cmds = 20;
	if (frametime < 0)
		frametime = 0;
	if (frametime > 0.1)
		frametime = 0.1;	//erg?

	if (chunkrate < 0)
		chunkrate = 0;
	slop += chunkrate*frametime;
	chunksremaining = slop;
	slop -= chunksremaining;
	if (chunksremaining < 1)
	{
		if (chunkrate < 30)
			chunksremaining = 1;
		else
			return;
/*		if (!chunkrate)
			chunkrate = 72;
		else
			chunkrate+=frametime;
		return;
*/	}
	if (chunksremaining > 100)
	{	//we're going to need some sanity limit, for cpu resources.
		chunkrate -= (chunksremaining-100);
		chunksremaining = 100;
	}
//Con_DPrintf("%i\n", chunksremaining);
	for (b = dl->dlblocks; b; b = b->next)
	{
		//packetloss reverts blocks to missing.
		if (b->state == DLB_PENDING)
		{
			if (b->sequence < cls.netchan.incoming_sequence-10)
			{
				haveloss = true;
				b->state = DLB_MISSING;
				for (;;)	//merge it with the next if they're all invalid
				{
					n = b->next;
					if (!n)
						break;
					if (n->state == DLB_MISSING || (n->state == DLB_PENDING && n->sequence < cls.netchan.incoming_sequence-10))
					{
						b->next = n->next;
						b->end = n->end;
						Z_Free(n);
						continue;
					}
					break;
				}
			}
			else
				stillpending = true;
		}
		if (b->state == DLB_MISSING && chunksremaining)
		{
			chunk = b->start / DLBLOCKSIZE;
			chunks = 1;//((b->end+DLBLOCKSIZE-1)/DLBLOCKSIZE) - (b->start / DLBLOCKSIZE);
			if (chunks > chunksremaining)
				chunks = chunksremaining;

			//if this block is bigger than a chunk, split the two blocks.
			if (b->end - b->start > DLBLOCKSIZE*chunks)
			{
				n = Z_Malloc(sizeof(*n));
				n->next = b->next;
				n->start = (chunk+chunks)*DLBLOCKSIZE;
				n->end = b->end;
				b->end = n->start;
				n->state = DLB_MISSING;
				b->next = n;
			}
			b->state = DLB_PENDING;
			b->sequence = cls.netchan.outgoing_sequence;
			stillpending = true;

			if (chunks > 1)
				cmd = va("nextdl %u %3g %i %i\n", (unsigned int)chunk, dl->percent, dl->filesequence, chunks);
			else
				cmd = va("nextdl %u %3g %i\n", (unsigned int)chunk, dl->percent, dl->filesequence);
			CL_RemoveClientCommands(cmd);
			CL_SendClientCommand(false, "%s", cmd);
			chunksremaining -= chunks;
			if (chunksremaining <= 0)
				break;
			/*if (--cmds <= 0)
			{
				chunkrate -= chunksremaining;
//				haveloss = true;
				break;
			}*/
		}
	}
	if (haveloss)
	{
		chunkrate *= 0.98;
	}
	if (!stillpending)
	{	//when there's nothing still pending, the download is complete.
		Con_DPrintf("Download took %i seconds (%i more)\n", (int)(Sys_DoubleTime() - dl->starttime), CL_CountQueuedDownloads());
		CL_DownloadFinished(dl);
	}
}

static void DLC_Poll(qdownload_t *dl)
{
	static float lasttime;
	DLC_RequestDownloadChunks(dl, realtime - lasttime);
	lasttime = realtime;
}

#endif

void DL_Abort(qdownload_t *dl, enum qdlabort aborttype)
{
	struct dlblock_s *b, *n;

	if (dl->file)
	{
		VFS_CLOSE(dl->file);
		dl->file = NULL;
	}

	if (dl->flags & DLLF_BEGUN)
	{
		dl->flags &= ~DLLF_BEGUN;
		if (aborttype == QDL_COMPLETED)
		{
			//this file isn't needed now the download has finished.
			FS_Remove(dl->dclname+dl->prefixbytes, dl->fsroot);

			if (dl->flags & DLLF_TEMPORARY)
			{
#ifdef TERRAIN
				if (!Terr_DownloadedSection(dl->tempname+dl->prefixbytes))
#endif
					Con_Printf("Downloaded unusable temporary file\n");
				FS_Remove(dl->tempname+dl->prefixbytes, dl->fsroot);
			}
			else if (Q_strcasecmp(dl->tempname, dl->localname))
			{
				if (dl->flags & DLLF_OVERWRITE)
					FS_Remove(dl->localname+dl->prefixbytes, dl->fsroot);
				if (!FS_Rename(dl->tempname+dl->prefixbytes, dl->localname+dl->prefixbytes, dl->fsroot))
				{
					char displaytmp[MAX_OSPATH], displayfinal[MAX_OSPATH];
					FS_DisplayPath(dl->tempname+dl->prefixbytes, dl->fsroot, displaytmp, sizeof(displaytmp));
					FS_DisplayPath(dl->localname+dl->prefixbytes, dl->fsroot, displayfinal, sizeof(displayfinal));
					Con_Printf("Couldn't rename %s to %s\n", displaytmp, displayfinal);
				}
			}
#ifdef PACKAGEMANAGER
			if (!strncmp(dl->localname, "package/", 8) && dl->fsroot == FS_ROOT)
				PM_FileInstalled(dl->localname+8, dl->fsroot, NULL, false);
#endif
		}
		else
		{
			//file was aborted half way through...
			if (dl->dlblocks)
			{
				//save the list of valid chunks so we don't have to redownload those.
				vfsfile_t *parts;
				parts = FS_OpenVFS(dl->dclname+dl->prefixbytes, "wb", dl->fsroot);
				if (parts)
				{
					for (b = dl->dlblocks; b; b = n)
					{
						if (b->state == DLB_RECEIVED)
							VFS_PRINTF(parts, "c %"PRIx64" %"PRIx64"\n", (quint64_t)b->start, (quint64_t)b->end);
						else
						{
							for(;;)
							{
								n = b->next;
								if (n && n->state != DLB_RECEIVED)
								{
									b->end = n->end;
									b->next = n->next;
									Z_Free(n);
									continue;
								}
								break;
							}
							VFS_PRINTF(parts, "m %"PRIx64" %"PRIx64"\n", (quint64_t)b->start, (quint64_t)b->end);
						}

						n = b->next;
						Z_Free(b);
					}
					dl->dlblocks = NULL;
					VFS_CLOSE(parts);
				}
				else
					FS_Remove(dl->tempname + dl->prefixbytes, dl->fsroot);
			}
			else
			{
				//download looks like it was non-resumable. just delete it.
				FS_Remove(dl->tempname + dl->prefixbytes, dl->fsroot);
			}
		}

		if (aborttype != QDL_DISCONNECT)
		{
			switch(dl->method)
			{
			default:
				break;
#ifdef Q3CLIENT
			case DL_Q3:
				q3->cl.SendClientCommand("stopdl");
				break;
#endif
			case DL_QW:
				break;
			case DL_DARKPLACES:
				CL_SendClientCommand(true, "stopdownload");
				break;
			case DL_QWCHUNKS:
				{
					//char *serverversion = InfoBuf_ValueForKey(&cl.serverinfo, "*version");
					//if (!strncmp(serverversion , "MVDSV ", 6))	//mvdsv will spam if we use stopdownload. and it'll misreport packetloss if we send nothing. grr.
						CL_SendClientCommand(true, "nextdl -1 100 %i", dl->filesequence);
					//else
					//	CL_SendClientCommand(true, "stopdownload");
				}
				break;
			}
		}
	}

	for (b = dl->dlblocks; b; b = n)
	{
		n = b->next;
		Z_Free(b);
	}
	dl->dlblocks = NULL;

	if (dl->method != DL_HTTP)
		Z_Free(dl);
	if (cls.download == dl)
		cls.download = NULL;
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
static void CL_ParseDownload (qboolean zlib)
{
	extern cvar_t cl_dlemptyterminate;
	int		size, percent;
	qbyte	name[1024];
	qdownload_t *dl = cls.download;

#ifdef PEXT_CHUNKEDDOWNLOADS
	if (cls.fteprotocolextensions & PEXT_CHUNKEDDOWNLOADS)
	{
		if (cls.demoplayback == DPB_MVD && cls.demoeztv_ext)
			Host_EndGame("CL_ParseDownload: chunked download on qtv proxy.");
		CL_ParseChunkedDownload(dl);
		return;
	}
#endif

	// read the data
	size = MSG_ReadShort ();
	percent = MSG_ReadByte ();

	if (size == -2)
	{
		/*quakeforge*/
		MSG_ReadString();
		return;
	}
	if (size == -3)
	{
		char *requestedname;
		Q_strncpyz(name, MSG_ReadString(), sizeof(name));
		requestedname = MSG_ReadString();
		Con_DPrintf("Download for %s redirected to %s\n", requestedname, name);
		/*quakeforge http download redirection*/
		if (dl)
			CL_DownloadFailed(dl->remotename, dl, DLFAIL_REDIRECTED);
		//FIXME: find some safe way to do this and actually test it. we should already know the local name, but we might have gained a .gz or something (this is quakeforge after all).
//		CL_CheckOrEnqueDownloadFile(name, localname, DLLF_IGNOREFAILED);
		return;
	}

	if (cls.demoplayback && !cls.demoeztv_ext)
	{
		if (size > 0)
			MSG_ReadSkip(size);
		return; // not in demo playback, we don't know the name of the file.
	}
	if (!dl)
	{
		//download packet without file requested.
		if (size > 0)
			MSG_ReadSkip(size);
		return; // not in demo playback
	}

	if (size < 0)
	{
		Con_TPrintf ("File not found.\n");

		if (dl)
			CL_DownloadFailed(dl->remotename, dl, DLFAIL_SERVERFILE);
		return;
	}

	// open the file if not opened yet
	if (dl->method == DL_QWPENDING)
	{
		dl->method = DL_QW;
		if (!DL_Begun(dl))
		{
			MSG_ReadSkip(size);
			Con_TPrintf ("Failed to open %s\n", dl->tempname);
			CL_DownloadFailed(dl->remotename, dl, DLFAIL_CLIENTFILE);
			CL_RequestNextDownload ();
			return;
		}
		SCR_EndLoadingPlaque();
	}

	if (zlib)
	{
#if defined(AVAIL_ZLIB) && defined(Q2CLIENT)
		z_stream s;
		unsigned short clen = size;
		unsigned short ulen = MSG_ReadShort();
		char cdata[8192];
		unsigned int done = 0;
		memset(&s, 0, sizeof(s));
		s.next_in = net_message.data + MSG_GetReadCount();
		s.avail_in = clen;
		if (inflateInit2(&s, -15) != Z_OK)
			Host_EndGame ("CL_ParseZDownload: unable to initialise zlib");
		for(;;)
		{
			int zerr;
			s.next_out = cdata;
			s.avail_out = sizeof(cdata);
			zerr = inflate(&s, Z_FULL_FLUSH);
			VFS_WRITE (dl->file, cdata, s.total_out - done);
			done = s.total_out;
			if (zerr == Z_STREAM_END)
				break;
			else if (zerr == Z_OK)
				continue;
			else
				Host_EndGame ("CL_ParseZDownload: stream truncated");
		}
		if (inflateEnd(&s) != Z_OK)
			Host_EndGame ("CL_ParseZDownload: stream truncated");
		VFS_WRITE (dl->file, cdata, s.total_out - done);
		done = s.total_out;
		if (s.total_out != ulen || s.total_in != clen)
			Host_EndGame ("CL_ParseZDownload: stream truncated");

#else
		Host_EndGame("Unable to handle zlib downloads, zlib is not supported in this build");
#endif
		MSG_ReadSkip(size);
	}
	else
#ifdef PEXT_ZLIBDL
	if (percent >= 101 && percent <= 201)// && cls.fteprotocolextensions & PEXT_ZLIBDL)
	{
		int compsize;

		percent = percent - 101;

		VFS_WRITE (cls.download, ZLibDownloadDecode(&compsize, net_message.data + MSG_GetReadCount(), size), size);

		MSG_ReadSkip(compsize);
	}
	else
#endif
	{
		VFS_WRITE (dl->file, net_message.data + MSG_GetReadCount(), size);
		MSG_ReadSkip(size);
	}

	dl->completedbytes += size;
	dl->ratebytes += size;
	if (dl->percent != percent)	//try and guess the size (its most acurate when the percent value changes)
		dl->size = ((float)dl->completedbytes*100)/percent;

	if (percent != 100 && size == 0 && cl_dlemptyterminate.ival)
	{
		Con_Printf(CON_WARNING "WARNING: Client received empty svc_download, assuming EOF.\n");
		percent = 100;
	}

	if (percent != 100)
	{
		// request next block
		dl->percent = percent;

		CL_SendClientCommand(true, "nextdl");
	}
	else
	{
		Con_DPrintf("Download took %i seconds\n", (int)(Sys_DoubleTime() - dl->starttime));

		CL_DownloadFinished(dl);

		// get another file if needed

		CL_RequestNextDownload ();
	}
}

qboolean CL_ParseOOBDownload(void)
{
	qdownload_t *dl = cls.download;
	if (!dl)
		return false;

	if (MSG_ReadLong() != dl->filesequence)
		return false;

	if (MSG_ReadChar() != svc_download)
		return false;

	CL_ParseDownload(false);
	return true;
}

#ifdef NQPROT
static void CLDP_ParseDownloadData(void)
{
	qdownload_t *dl = cls.download;
	unsigned char buffer[1<<16];
	qofs_t start;
	int size;
	start = MSG_ReadLong();
	size = (unsigned short)MSG_ReadShort();

	MSG_ReadData(buffer, size);

	if (!dl)
		return;

	if (dl->file)
	{
		if (start > dl->completedbytes)
		{	//this protocol cannot deal with gaps. we might as well wait until its repeated later.
			//don't ack values ahead of what we completed, we won't get good results if we do that. servers are dumb.
			start = dl->completedbytes;
			size = 0;
		}
		else if (start+size < dl->completedbytes)
			;	//already completed this data
		else
		{
			int offset = dl->completedbytes-start;	//we may already have completed some chunk already

			VFS_WRITE(dl->file, buffer+offset, size-offset);
			dl->completedbytes += size-offset;
			dl->ratebytes += size-offset;	//for download rate calcs
		}

		dl->percent = (dl->completedbytes) / (float)dl->size * 100;
	}

	//we need to ack in order.
	//the server doesn't actually track packets, only position, however there's no way to tell it that we already have a chunk
	//we could send the acks unreliably, but any cl->sv loss would involve a sv->cl resend (because we can't dupe).
	MSG_WriteByte(&cls.netchan.message, clcdp_ackdownloaddata);
	MSG_WriteLong(&cls.netchan.message, start);
	MSG_WriteShort(&cls.netchan.message, size);
}

static void CLDP_ParseDownloadBegin(char *s)
{
	qdownload_t *dl = cls.download;
	char buffer[8192];
	qofs_t size, pos, chunk;
	char *fname;
	Cmd_TokenizeString(s, false, false);
	size = (qofs_t)strtoull(Cmd_Argv(1), NULL, 0);
	fname = Cmd_Argv(2);

	if (!dl || strcmp(fname, dl->remotename))
	{
#ifdef CSQC_DAT
		if (cls.demoplayback && !dl && cl_dp_csqc_progssize && size == cl_dp_csqc_progssize && !strcmp(fname, cl_dp_csqc_progsname))
		{	//its somewhat common for demos to contain a copy of the csprogs, so that the same version is available when trying to play the demo back.
			extern cvar_t cl_download_csprogs, cl_nocsqc;
			if (!cl_nocsqc.ival && cl_download_csprogs.ival)
			{
				fname = va("csprogsvers/%x.dat", cl_dp_csqc_progscrc);
				if (CL_CheckDLFile(fname))
					return;	//we already have this version

				//Begin downloading it...
			}
			else
				return;	//silently ignore it
		}
		else
#endif
		{
			Con_Printf("Warning: server started sending a file we did not request. Ignoring.\n");
			return;
		}
	}

	if (!dl)
	{
		dl = Z_Malloc(sizeof(*dl));
		dl->filesequence = 0;

		Q_strncpyz(dl->remotename, fname, sizeof(dl->remotename));
		Q_strncpyz(dl->localname, fname, sizeof(dl->localname));
		Con_TPrintf ("Downloading %s...\n", dl->localname);

		// download to a temp name, and only rename
		// to the real name when done, so if interrupted
		// a runt file wont be left
		COM_StripExtension (dl->localname, dl->tempname, sizeof(dl->tempname)-5);
		Q_strncatz (dl->tempname, ".tmp", sizeof(dl->tempname));

		dl->method = DL_DARKPLACES;
		dl->percent = 0;
		dl->sizeunknown = true;
		dl->flags = DLLF_REQUIRED;
		cls.download = dl;
	}

	if (dl->method == DL_QWPENDING)
		dl->method = DL_DARKPLACES;
	if (dl->method != DL_DARKPLACES)
	{
		Con_Printf("Warning: download method isn't right.\n");
		return;
	}

	dl->sizeunknown = false;
	dl->size = size;
	if (!DL_Begun(dl))
	{
		CL_DownloadFailed(dl->remotename, dl, DLFAIL_CLIENTFILE);
		return;
	}

	CL_SendClientCommand(true, "sv_startdownload");

	//fill the file with 0 bytes, for some reason
	memset(buffer, 0, sizeof(buffer));
	for (pos = 0, chunk = 1; chunk; pos += chunk)
	{
		chunk = size - pos;
		if (chunk > sizeof(buffer))
			chunk = sizeof(buffer);
		VFS_WRITE(dl->file, buffer, chunk);
	}
	VFS_SEEK(dl->file, 0);
}

static void CLDP_ParseDownloadFinished(char *s)
{
	qdownload_t *dl = cls.download;
	unsigned int runningcrc = 0;
	const hashfunc_t *hfunc = &hash_crc16;
	char buffer[8192];
	qofs_t size, pos, chunk;
	if (!dl || !dl->file)
		return;

	Cmd_TokenizeString(s, false, false);

	VFS_CLOSE (dl->file);

	dl->file = FS_OpenVFS (dl->tempname+dl->prefixbytes, "rb", dl->fsroot);
	if (dl->file)
	{
		void *hashctx = alloca(hfunc->contextsize);
		size = dl->size;
		hfunc->init(hashctx);
		for (pos = 0; pos < size; pos += chunk)
		{
			chunk = min(sizeof(buffer), size - pos);
			if (chunk != VFS_READ(dl->file, buffer, chunk))
				break;
			hfunc->process(hashctx, buffer, chunk);
		}
		VFS_CLOSE (dl->file);
		dl->file = NULL;

		runningcrc = hashfunc_terminate_uint(hfunc, hashctx);
	}
	else
	{
		Con_Printf("Download failed: unable to check CRC of download\n");
		CL_DownloadFailed(dl->remotename, dl, DLFAIL_CLIENTFILE);
		return;
	}

	if (size != atoi(Cmd_Argv(1)))
	{
		Con_Printf("Download failed: wrong file size\n");
		CL_DownloadFailed(dl->remotename, dl, DLFAIL_CORRUPTED);
		return;
	}
	if (runningcrc != atoi(Cmd_Argv(2)))
	{
		Con_Printf("Download failed: wrong crc\n");
		CL_DownloadFailed(dl->remotename, dl, DLFAIL_CORRUPTED);
		return;
	}

	Con_DPrintf("Download took %i seconds\n", (int)(Sys_DoubleTime() - dl->starttime));

	CL_DownloadFinished(dl);

	// get another file if needed

	CL_RequestNextDownload ();
}
#endif

static vfsfile_t *upload_file;
static qbyte *upload_data;
static int upload_pos;
static int upload_size;

void CL_NextUpload(void)
{
	qbyte	buffer[1024];
	int		r;
	int		percent;
	int		size;

	r = upload_size - upload_pos;
	if (r > 768)
		r = 768;

	if (upload_data)
	{
		memcpy(buffer, upload_data + upload_pos, r);
	}
	else if (upload_file)
	{
		r = VFS_READ(upload_file, buffer, r);
		if (r == 0)
		{
			CL_StopUpload();
			return;
		}
	}
	else
		return;
	MSG_WriteByte (&cls.netchan.message, clc_upload);
	MSG_WriteShort (&cls.netchan.message, r);

	upload_pos += r;
	size = upload_size;
	if (!size)
		size = 1;
	percent = upload_pos*100/size;
	MSG_WriteByte (&cls.netchan.message, percent);
	SZ_Write (&cls.netchan.message, buffer, r);

Con_DPrintf ("UPLOAD: %6d: %d written\n", upload_pos - r, r);

	if (upload_pos != upload_size)
		return;

	Con_TPrintf ("Upload completed\n");

	CL_StopUpload();
}

void CL_StartUpload (qbyte *data, int size)
{
	if (cls.state < ca_onserver)
		return; // gotta be connected

	// override
	CL_StopUpload();

Con_DPrintf("Upload starting of %d...\n", size);

	upload_data = BZ_Malloc(size);
	memcpy(upload_data, data, size);
	upload_size = size;
	upload_pos = 0;

	CL_NextUpload();
}

qboolean CL_IsUploading(void)
{
	if (upload_data || upload_file)
		return true;
	return false;
}

void CL_StopUpload(void)
{
	if (upload_data)
		BZ_Free(upload_data);
	if (upload_file)
		VFS_CLOSE(upload_file);
	upload_file = NULL;
	upload_data = NULL;
	upload_pos = upload_size = 0;
}

#if 0	//in case we ever want to add any uploads other than snaps
static qboolean CL_StartUploadFile(char *filename)
{
	if (!COM_CheckParm("-fileul"))
	{
		Con_Printf("You must currently use the -fileul commandline parameter in order to use this functionality\n");
		return false;
	}

	if (cls.state < ca_onserver)
	{
		Con_Printf("not connected\n");
		return false; // gotta be connected
	}

	CL_StopUpload();

	upload_file = FS_OpenVFS(filename, "rb", FS_ROOT);
	upload_size = VFS_GETLEN(upload_file);
	upload_pos = 0;

	if (upload_file)
	{
		CL_NextUpload();
		return true;
	}
	return false;
}
#endif

/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

void CL_ClearParseState(void)
{
	// done with sounds, request models now
	memset (cl.model_precache, 0, sizeof(cl.model_precache));
	cl_playerindex = -1;
	cl_h_playerindex = -1;
	cl_spikeindex = -1;
	cl_flagindex = -1;
	cl_rocketindex = -1;
	cl_grenadeindex = -1;
	cl_gib1index = -1;
	cl_gib2index = -1;
	cl_gib3index = -1;

	if (cl_baselines)
	{
		BZ_Free(cl_baselines);
		cl_baselines = NULL;
	}
	cl_baselines_count = 0;

	cl_max_static_entities = 0;
	if (cl_static_entities)
	{
		BZ_Free(cl_static_entities);
		cl_static_entities = NULL;
	}
}

/*
==================
CL_ParseServerData
==================
*/
static void CLQW_ParseServerData (void)
{
	int pnum;
	int clnum;
	char	*str;
	int protover, svcnt;

	float maxspeed, entgrav;

	if (cls.download && cls.download->method == DL_QWPENDING)
	{
		//if we didn't actually start downloading it yet, cancel the current download.
		//this is to avoid qizmo not responding to the download command, resulting in hanging downloads that cause the client to then be unable to connect anywhere simply because someone's skin was set.
		CL_DownloadFailed(cls.download->remotename, cls.download, DLFAIL_CORRUPTED);
	}

	Con_DPrintf ("Serverdata packet %s.\n", cls.demoplayback?"read":"received");
//
// wipe the client_state_t struct
//

	SCR_SetLoadingStage(LS_CLIENT);
	SCR_BeginLoadingPlaque();

// parse protocol version number
// allow 2.2 and 2.29 demos to play
	cls.fteprotocolextensions = 0;
	cls.fteprotocolextensions2 = 0;
	cls.ezprotocolextensions1 = 0;
	for(;;)
	{
		protover = MSG_ReadLong ();
		if (protover == PROTOCOL_VERSION_FTE1)
		{
			cls.fteprotocolextensions = MSG_ReadLong();
			continue;
		}
		if (protover == PROTOCOL_VERSION_FTE2)
		{
			cls.fteprotocolextensions2 = MSG_ReadLong();
			continue;
		}
		if (protover == PROTOCOL_VERSION_EZQUAKE1)
		{
			cls.ezprotocolextensions1 = MSG_ReadLong();
			continue;
		}
		if (protover == PROTOCOL_VERSION_VARLENGTH)
		{
			int ident;
			int len;
			char data[1024];
			ident = MSG_ReadLong();
			len = MSG_ReadLong();
			if (len <= sizeof(data))
			{
				MSG_ReadData(data, len);
				switch(ident)
				{
				default:
					break;
				}
				continue;
			}
		}
		if (protover == PROTOCOL_VERSION_QW)	//this ends the version info
			break;
		if (cls.demoplayback && (protover >= 24 && protover <= 28))	//older versions, maintain demo compatability.
			break;
		Host_EndGame ("Server returned version %i, not %i\n", protover, PROTOCOL_VERSION_QW);
	}

	if (developer.ival || cl_shownet.ival)
	{
		if (cls.fteprotocolextensions2||cls.fteprotocolextensions||cls.ezprotocolextensions1)
			Con_TPrintf ("Using FTE extensions 0x%x%08x %#x\n", cls.fteprotocolextensions2, cls.fteprotocolextensions, cls.ezprotocolextensions1);
	}
	if (cls.fteprotocolextensions & ~PEXT_CLIENTSUPPORT)
		Con_TPrintf (CON_WARNING"Using unknown fte-pext1 extensions (%#x)\n", cls.fteprotocolextensions&~PEXT_CLIENTSUPPORT);
	if (cls.fteprotocolextensions2 & ~PEXT2_CLIENTSUPPORT)
		Con_TPrintf (CON_WARNING"Using unknown fte-pext2 extensions (%#x)\n", cls.fteprotocolextensions2&~PEXT2_CLIENTSUPPORT);
	if (cls.ezprotocolextensions1 & ~EZPEXT1_CLIENTSUPPORT)
		Con_TPrintf (CON_WARNING"Using unknown ezquake extensions (%#x)\n", cls.ezprotocolextensions1&~EZPEXT1_CLIENTSUPPORT);

	if ((cls.ezprotocolextensions1 & EZPEXT1_HIDDEN_MESSAGES) && !cls.demoplayback)
		Con_TPrintf (CON_WARNING"Server's EZPEXT1_HIDDEN_MESSAGES extension does not make sense outside of demos\n");	//we do not request this at all. its safe to blame the server.

	if (cls.fteprotocolextensions & PEXT_FLOATCOORDS)
	{
		cls.netchan.netprim.coordtype = COORDTYPE_FLOAT_32;
		cls.netchan.netprim.anglesize = 2;
	}
	else
	{
		cls.netchan.netprim.coordtype = COORDTYPE_FIXED_13_3;
		cls.netchan.netprim.anglesize = 1;
	}
	cls.netchan.message.prim = cls.netchan.netprim;
	MSG_ChangePrimitives(cls.netchan.netprim);

	svcnt = MSG_ReadLong ();

	// game directory
	str = MSG_ReadString ();
	Con_DPrintf("Server is using gamedir \"%s\"\n", str);
	if (!*str)
		str = "qw";	//FIXME: query active manifest's basegamedir

#ifndef CLIENTONLY
	if (!sv.state)
#endif
		COM_Gamedir(str, NULL);

	CL_ClearState (true);
#ifdef QUAKEHUD
	Stats_NewMap();
#endif
	cl.servercount = svcnt;
	cl.protocol_qw = protover;

	Cvar_ForceCallback(Cvar_FindVar("r_particlesdesc"));

	cl.teamfortress = !!Q_strcasestr(str, "fortress");

	if (cl.gamedirchanged)
	{
		cl.gamedirchanged = false;
#ifndef CLIENTONLY
		if (!sv.state)
#endif
			Wads_Flush();
	}

	/*mvds have different parsing*/
	if (cls.demoplayback == DPB_MVD)
	{
		extern float olddemotime;
		int i,j;

		if (cls.fteprotocolextensions2 & PEXT2_MAXPLAYERS)
		{
			cl.allocated_client_slots = MSG_ReadPlayer();
			if (cl.allocated_client_slots > MAX_CLIENTS)
			{
				Con_Printf(CON_ERROR"Server has too many client slots (%u > %u)\n", cl.allocated_client_slots, MAX_CLIENTS);
				cl.allocated_client_slots = MAX_CLIENTS;
			}
		}

		cl.gametime = MSG_ReadFloat();
		cl.gametimemark = realtime;
		cl.oldgametime = cl.gametime;
		cl.oldgametimemark = realtime;

		cl.demogametimebias = cl.gametime - olddemotime;

		for (j = 0; j < MAX_SPLITS; j++)
		{
			cl.playerview[j].playernum = cl.allocated_client_slots + j;
			cl.playerview[j].viewentity = 0;	//free floating.
			cl.playerview[j].spectator = true;
			for (i = 0; i < UPDATE_BACKUP; i++)
			{
				cl.inframes[i].playerstate[cl.playerview[j].playernum].pm_type = PM_SPECTATOR;
				cl.inframes[i].playerstate[cl.playerview[j].playernum].messagenum = 1;
			}
		}

		cl.splitclients = 1;
	}
	else if (cls.fteprotocolextensions2 & PEXT2_MAXPLAYERS)
	{
//		qboolean spec = false;
		cl.allocated_client_slots = MSG_ReadPlayer();
		if (cl.allocated_client_slots > MAX_CLIENTS)
		{
			Con_Printf(CON_ERROR"Server has too many client slots (%u > %u)\n", cl.allocated_client_slots, MAX_CLIENTS);
			cl.allocated_client_slots = MAX_CLIENTS;
		}

		/*parsing here is slightly different to allow us 255 max players instead of 127*/
		cl.splitclients = (qbyte)MSG_ReadByte();
		if (cls.fteprotocolextensions2 & PEXT2_VRINPUTS)
			;
		else if (cl.splitclients & 128)
		{
//			spec = true;
			cl.splitclients &= ~128;
		}
		if (cl.splitclients > MAX_SPLITS)
			Host_EndGame("Server sent us too many seats (%u > %u)\n", cl.splitclients, MAX_SPLITS);
		for (pnum = 0; pnum < cl.splitclients; pnum++)
		{
			cl.playerview[pnum].spectator = true;
#ifdef QUAKESTATS
			if (cls.z_ext & Z_EXT_VIEWHEIGHT)
				cl.playerview[pnum].viewheight = cl.playerview[pnum].statsf[STAT_VIEWHEIGHT];
#endif
			cl.playerview[pnum].playernum = (qbyte)MSG_ReadByte();
			if (cl.playerview[pnum].playernum >= cl.allocated_client_slots)
				Host_EndGame("unsupported local player slot\n");
			cl.playerview[pnum].viewentity = cl.playerview[pnum].playernum+1;
		}
	}
	else 
	{
		// parse player slot, high bit means spectator
		pnum = MSG_ReadByte ();
		for (clnum = 0; ; clnum++)
		{
			if (clnum == MAX_SPLITS)
				Host_EndGame("Server sent us over %u seats\n", MAX_SPLITS);
#ifdef QUAKESTATS
			if (cls.z_ext & Z_EXT_VIEWHEIGHT)
				cl.playerview[pnum].viewheight = cl.playerview[pnum].statsf[STAT_VIEWHEIGHT];
#endif
			cl.playerview[clnum].playernum = pnum;
			if (cl.playerview[clnum].playernum & 128)
			{
				cl.playerview[clnum].spectator = true;
				cl.playerview[clnum].playernum &= ~128;
			}
			else
				cl.playerview[clnum].spectator = false;

			if (cl.playerview[clnum].playernum >= cl.allocated_client_slots)
				Host_EndGame("unsupported local player slot\n");

			cl.playerview[clnum].viewentity = cl.playerview[clnum].playernum+1;
			if (!(cls.fteprotocolextensions & PEXT_SPLITSCREEN))
				break;

			pnum = MSG_ReadByte ();
			if (pnum == 128)
				break;
		}
		cl.splitclients = clnum+1;
	}

	// get the full level name
	str = MSG_ReadString ();
	Q_strncpyz (cl.levelname, str, sizeof(cl.levelname));

	if (cl.protocol_qw >= 25)
	{
		// get the movevars
		movevars.gravity			= MSG_ReadFloat();
		movevars.stopspeed			= MSG_ReadFloat();
		maxspeed					= MSG_ReadFloat();
		movevars.spectatormaxspeed	= MSG_ReadFloat();
		movevars.accelerate			= MSG_ReadFloat();
		movevars.airaccelerate		= MSG_ReadFloat();
		movevars.wateraccelerate	= MSG_ReadFloat();
		movevars.friction			= MSG_ReadFloat();
		movevars.waterfriction		= MSG_ReadFloat();
		entgrav						= MSG_ReadFloat();
	}
	else
	{
		movevars.gravity			= 800;
		movevars.stopspeed			= 100;
		maxspeed					= 320;
		movevars.spectatormaxspeed	= 500;
		movevars.accelerate			= 10;
		movevars.airaccelerate		= 0.7f;
		movevars.wateraccelerate	= 10;
		movevars.friction			= 6.0f;
		movevars.waterfriction		= 1;
		entgrav						= 1;
	}
	movevars.flags = MOVEFLAG_QWCOMPAT;

	for (clnum = 0; clnum < cl.splitclients; clnum++)
	{
		cl.playerview[clnum].maxspeed = maxspeed;
		cl.playerview[clnum].entgravity = entgrav;
	}

	// seperate the printfs so the server message can have a color
#if 1
	Con_Printf ("\n\n");
	Con_Printf ("^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f");
	Con_Printf ("\n\n");
	Con_Printf ("\1%s\n", str);
#else
	Con_TPrintf ("\n\n^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f\n\n");
	Con_Printf ("%c%s\n", 2, str);
#endif

	if (CL_RemoveClientCommands("new"))	//mvdsv is really appaling some times.
	{
	//	Con_Printf("Multiple 'new' commands?!?!? This server needs reinstalling!\n");
	}

	memset(cl.sound_name, 0, sizeof(cl.sound_name));
	if (cls.demoplayback == DPB_MVD && (cls.demoeztv_ext&EZTV_DOWNLOAD))
	{
		if (CL_RemoveClientCommands("qtvsoundlist"))
			Con_DPrintf("Multiple soundlists\n");
		CL_SendClientCommand (true, "qtvsoundlist %i 0", cl.servercount);
	}
	else
	{
		if (CL_RemoveClientCommands("soundlist") && !(cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))
			Con_DPrintf("Multiple soundlists\n");
		// ask for the sound list next
//		CL_SendClientCommand ("soundlist %i 0", cl.servercount);
		CL_SendClientCommand (true, soundlist_name, cl.servercount, 0);
	}

	// now waiting for downloads, etc
	cls.state = ca_onserver;
	Cam_AutoTrack_Update(NULL);

	cl.sendprespawn = false;

#ifdef VOICECHAT
	S_Voip_MapChange();
#endif

#ifdef CSQC_DAT
	CSQC_Shutdown();	//revive it when we get the serverinfo saying the checksum.
#endif
}

#ifdef Q2CLIENT
static void CLQ2_ParseServerData (void)
{
	char	*str;
	int		i;
	int svcnt;
	int rate;
//	int cflag;

	memset(&cls.netchan.netprim, 0, sizeof(cls.netchan.netprim));
	cls.netchan.netprim.coordtype = COORDTYPE_FIXED_13_3;
	cls.netchan.netprim.anglesize = 1;
	cls.fteprotocolextensions = 0;
	cls.fteprotocolextensions2 = 0;
	cls.ezprotocolextensions1 = 0;
	cls.demohadkeyframe = true;	//assume that it did, so this stuff all gets recorded.

	Con_DPrintf ("Serverdata packet %s.\n", cls.demoplayback?"read":"received");
//
// wipe the client_state_t struct
//
	SCR_SetLoadingStage(LS_CLIENT);
	SCR_BeginLoadingPlaque();
//	CL_ClearState ();
	cls.state = ca_onserver;

// parse protocol version number
	i = MSG_ReadLong ();

	if (i == PROTOCOL_VERSION_FTE1)
	{
		cls.fteprotocolextensions = i = MSG_ReadLong();
		if (i & PEXT_FLOATCOORDS)
			i -= PEXT_FLOATCOORDS;
		if (i & PEXT_SOUNDDBL)
			i -= PEXT_SOUNDDBL;
		if (i & PEXT_MODELDBL)
			i -= PEXT_MODELDBL;
		if (i & PEXT_SPLITSCREEN)
			i -= PEXT_SPLITSCREEN;
		if (i)
			Host_EndGame ("Unsupported q2 protocol extensions: %x", i);
		i = MSG_ReadLong ();

		if (cls.fteprotocolextensions & PEXT_FLOATCOORDS)
		{
			cls.netchan.netprim.coordtype = COORDTYPE_FLOAT_32;
			cls.netchan.netprim.anglesize = 2;
		}
	}

	if (i == PROTOCOL_VERSION_R1Q2)
		Con_DPrintf("Using R1Q2 protocol\n");
	else if (i == PROTOCOL_VERSION_Q2PRO)
		Con_DPrintf("Using Q2PRO protocol\n");
	else if (i == PROTOCOL_VERSION_Q2EXDEMO)
	{
		i = PROTOCOL_VERSION_Q2EX;	//close enough, don't distinguish. only real difference is 16bit coords for WritePos etc.
		Con_DPrintf("Using Q2EXDemo protocol\n");
	}
	else if (i == PROTOCOL_VERSION_Q2EX)
	{
		cls.netchan.netprim.coordtype = COORDTYPE_FLOAT_32;
		cls.netchan.netprim.anglesize = 2;
		Con_DPrintf("Using Q2EX protocol\n");
	}
	else if (i > PROTOCOL_VERSION_Q2 || i < (cls.demoplayback?PROTOCOL_VERSION_Q2_DEMO_MIN:PROTOCOL_VERSION_Q2_MIN))
		Host_EndGame ("Q2 Server returned version %i, not %i", i, PROTOCOL_VERSION_Q2);
	cls.protocol_q2 = i;

	svcnt = MSG_ReadLong ();
	/*cl.attractloop =*/ MSG_ReadByte ();

	if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		rate = MSG_ReadByte();
	else
		rate = 10;	//fixed to a poopy 10hz.

	// game directory
	str = MSG_ReadString ();
	// set gamedir
	if (!*str)
		COM_Gamedir("baseq2", NULL);
	else
		COM_Gamedir(str, NULL);

	Cvar_Get("timescale", "1", 0, "Q2Admin hacks");	//Q2Admin will kick players who have a timescale set to something other than 1
													//FTE doesn't actually have a timescale cvar, so create one to 'fool' q2admin.
													//I can't really blame q2admin for rejecting engines that don't have this cvar, as it could have been renamed via a hex-edit.

	CL_ClearState (true);
	CLQ2_ClearState ();
	cl.minpitch = -89;
	cl.maxpitch = 89;
	cl.servercount = svcnt;
	Cam_AutoTrack_Update(NULL);
	
#ifdef QUAKEHUD
	Stats_NewMap();
#endif

	// parse player entity number
	cl.playerview[0].playernum = MSG_ReadShort ();
	cl.playerview[0].viewentity = cl.playerview[0].playernum+1;
	cl.playerview[0].spectator = false;
	cl.splitclients = 1;

	if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX && cl.playerview[0].playernum==-2)
	{
		i = MSG_ReadShort();
		if (i > MAX_SPLITS)
			Host_EndGame ("server's splitscreen count too high (%u > %u)\n", i, MAX_SPLITS);
		cl.splitclients = i;
		for (i = 0; i < cl.splitclients; i++)
		{
			cl.playerview[i].playernum = MSG_ReadShort ();
			cl.playerview[i].viewentity = cl.playerview[i].playernum+1;
			cl.playerview[i].spectator = false;
		}
	}

	cl.numq2visibleweapons = 1;	//give it a default.
	cl.q2visibleweapons[0] = "weapon.md2";
	cl.q2svnetrate = rate;
	if (cl.q2svnetrate < 1)	//wut?...
		cl.q2svnetrate = 10;

	// get the full level name
	str = MSG_ReadString ();
	Q_strncpyz (cl.levelname, str, sizeof(cl.levelname));

	if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2)
	{
		unsigned short r1q2ver;
		qboolean isenhanced = MSG_ReadByte();
		if (isenhanced)
			Host_EndGame ("R1Q2 server is running an unsupported mod");
		r1q2ver = MSG_ReadShort();	//protocol version... limit... yeah, buggy.
		if (r1q2ver > 1905)
			Host_EndGame ("R1Q2 server version %i not supported", r1q2ver);

		if (r1q2ver >= 1903)
		{
			MSG_ReadByte();	//'used to be advanced deltas'
			MSG_ReadByte(); //strafejump hack
		}
		if (r1q2ver >= 1904)
			cls.netchan.netprim.flags |= NPQ2_R1Q2_UCMD;
		if (r1q2ver >= 1905)
			cls.netchan.netprim.flags |= NPQ2_SOLID32;
	}
	else if (cls.protocol_q2 == PROTOCOL_VERSION_Q2PRO)
	{
		unsigned short q2prover = MSG_ReadShort();	//q2pro protocol version
		if (q2prover < 1011 || q2prover > 1021)
			Host_EndGame ("Q2PRO server version %i not supported", q2prover);
		MSG_ReadByte();	//server state (ie: demo playback vs actual game)
		MSG_ReadByte(); //strafejump hack
		MSG_ReadByte(); //q2pro qw-mode. kinda silly for us tbh.
		if (q2prover >= 1014)
			cls.netchan.netprim.flags |= NPQ2_SOLID32;
		if (q2prover >= 1018)
			cls.netchan.netprim.flags |= NPQ2_ANG16;
		if (q2prover >= 1015)
			MSG_ReadByte();	//some kind of waterjump hack enable
	}

	cls.netchan.message.prim = cls.netchan.netprim;
	MSG_ChangePrimitives(cls.netchan.netprim);

	if (cl.playerview[0].playernum == -1)
	{	// playing a cinematic or showing a pic, not a level
		SCR_EndLoadingPlaque();
		CL_MakeActive("Quake2");
		if (!COM_FCheckExists(str) && !COM_FCheckExists(va("video/%s", str)))
		{
			int i;
			char basename[64], *t;
			char *exts[] = {".ogv", ".roq", ".cin"};
			COM_StripExtension(COM_SkipPath(str), basename, sizeof(basename));
			for(i = 0; i < countof(exts); i++)
			{
				t = va("video/%s%s", basename, exts[i]);
				if (COM_FCheckExists(t))
				{
					str = t;
					break;
				}
			}
		}
		if (!Media_PlayFilm(str, false))
		{
			CL_SendClientCommand(true, "nextserver %i", cl.servercount);
		}
	}
	else
	{
		// seperate the printfs so the server message can have a color
		Con_TPrintf ("\n\n^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f\n\n");
		Con_Printf ("%c%s\n", 2, str);

		Media_StopFilm(true);

		// need to prep refresh at next oportunity
		//cl.refresh_prepped = false;
	}

	Cvar_ForceCallback(Cvar_FindVar("r_particlesdesc"));

	Surf_PreNewMap();
	CL_CheckServerInfo();
}
#endif


void CL_ParseEstablished(void)
{
#ifdef NQPROT
	cls.qex = false;
	Z_Free(cl_dp_packagenames);
	cl_dp_packagenames = NULL;
	cl_dp_serverextension_download = false;
	*cl_dp_csqc_progsname = 0;
	cl_dp_csqc_progscrc = 0;
	cl_dp_csqc_progssize = 0;
#endif

	if (cls.netchan.remote_address.type != NA_LOOPBACK)
	{
		char fp[DIGEST_MAXSIZE*3+1+4];
		char dig[DIGEST_MAXSIZE+1];
		char cert[8192];
		const char *security;
		switch(cls.protocol)
		{
		case CP_QUAKEWORLD:	Con_DPrintf(S_COLOR_GRAY"QW ");	break;
		case CP_NETQUAKE:	Con_Printf (S_COLOR_GRAY"NQ ");	break;
		case CP_QUAKE2:		Con_Printf (S_COLOR_GRAY"Q2 ");	break;
		case CP_QUAKE3:		Con_Printf (S_COLOR_GRAY"Q3 ");	break;
		default: break;
		}
		*fp = 0;
		if (NET_IsEncrypted(&cls.netchan.remote_address))
		{
			if (cls.netchan.remote_address.prot == NP_DTLS)
			{
				int sz = NET_GetConnectionCertificate(cls.sockets, &cls.netchan.remote_address, QCERT_PEERCERTIFICATE, cert,sizeof(cert));
				if (sz >= 0)
				{
					Q_strncpyz(fp, "?fp=",sizeof(fp));
					sz = 4+Base64_EncodeBlockURI(dig, CalcHash(&hash_certfp, dig,sizeof(dig), cert, sz), fp+4,sizeof(fp)-4);
					fp[sz] = 0;
				}
			}
			security = localtext("^["S_COLOR_GREEN"encrypted\\tip\\Any passwords will be sent securely, but will still be readable by the server admin\n^]");
		}
		else
			security = localtext("^["S_COLOR_RED"plain-text\\tip\\"CON_WARNING"Do not type passwords as they can potentially be seen by network sniffers^]");

		Con_Printf ("\r");
		Con_TPrintf ("Connected to ^["S_COLOR_BLUE"%s"S_COLOR_GRAY"%s\\type\\connect %s%s^] (%s).\n", cls.servername, fp, cls.servername, fp, security);
	}
}

#ifdef NQPROT
static void CLNQ_ParseProtoVersion(void)
{
	int protover;
	struct netprim_s netprim;

	cls.fteprotocolextensions = 0;
	cls.fteprotocolextensions2 = 0;
	cls.ezprotocolextensions1 = 0;
	for(;;)
	{
		protover = MSG_ReadLong ();
		switch(protover)
		{
		case PROTOCOL_VERSION_FTE1:
			cls.fteprotocolextensions = MSG_ReadLong();
			continue;
		case PROTOCOL_VERSION_FTE2:
			cls.fteprotocolextensions2 = MSG_ReadLong();
			continue;
		default:
			break;
		}
		break;
	}

	netprim.coordtype = COORDTYPE_FIXED_13_3;
	netprim.anglesize = 1;

	cls.protocol_nq = CPNQ_ID;
	cls.z_ext = 0;

#ifdef HAVE_LEGACY
	if (protover == PROTOCOL_VERSION_NQ && cls.demoplayback)
	{
		if (!Q_strcasecmp(FS_GetGamedir(true), "nehahra"))
			protover = PROTOCOL_VERSION_NEHD;	//if we're using the nehahra gamedir, pretend that it was the nehahra protocol version. clients should otherwise be using a different protocol.
	}
#endif

	if (protover == PROTOCOL_VERSION_NEHD)
	{
		cls.protocol_nq = CPNQ_NEHAHRA;
		Con_DPrintf("Nehahra demo net protocol\n");
	}
	else if (protover == PROTOCOL_VERSION_FITZ)
	{
		//fitzquake 0.85
		cls.protocol_nq = CPNQ_FITZ666;
		Con_DPrintf("FitzQuake 666 protocol\n");
	}
	else if (protover == PROTOCOL_VERSION_RMQ)
	{
		int fl;
		cls.protocol_nq = CPNQ_FITZ666;
		Con_DPrintf("RMQ extensions to FitzQuake's protocol\n");
		fl = MSG_ReadLong();

		if (((fl & RMQFL_SHORTANGLE) && (fl & RMQFL_FLOATANGLE)) ||
			((fl & RMQFL_24BITCOORD) && (fl & RMQFL_INT32COORD)) ||
			((fl & RMQFL_24BITCOORD) && (fl & RMQFL_FLOATCOORD)) ||
			((fl & RMQFL_INT32COORD) && (fl & RMQFL_FLOATCOORD)) )
			Host_EndGame("Server is using conflicting RMQ protocol bits - %#x\n", fl);

		if (fl & RMQFL_SHORTANGLE)
			netprim.anglesize = 2;
		if (fl & RMQFL_FLOATANGLE)
			netprim.anglesize = 4;
		if (fl & RMQFL_24BITCOORD)
			netprim.coordtype = COORDTYPE_FIXED_16_8;
		if (fl & RMQFL_INT32COORD)
			netprim.coordtype = COORDTYPE_FIXED_28_4;
		if (fl & RMQFL_FLOATCOORD)
			netprim.coordtype = COORDTYPE_FLOAT_32;

		fl &= ~(RMQFL_SHORTANGLE|RMQFL_FLOATANGLE|RMQFL_24BITCOORD|RMQFL_INT32COORD|RMQFL_FLOATCOORD|RMQFL_EDICTSCALE);
		if (fl)
			Host_EndGame("Server is using unsupported RMQ extensions - %#x\n", fl);
	}
	else if (protover == PROTOCOL_VERSION_DP5)
	{
		//darkplaces5
		cls.protocol_nq = CPNQ_DP5;
		netprim.coordtype = COORDTYPE_FLOAT_32;
		netprim.anglesize = 2;

		Con_DPrintf("DP5 protocols\n");
	}
	else if (protover == PROTOCOL_VERSION_DP6)
	{
		//darkplaces6 (it's a small difference from dp5)
		cls.protocol_nq = CPNQ_DP6;
		netprim.coordtype = COORDTYPE_FLOAT_32;
		netprim.anglesize = 2;

		cls.z_ext = Z_EXT_VIEWHEIGHT;

		Con_DPrintf("DP6 protocols\n");
	}
	else if (protover == PROTOCOL_VERSION_DP7)
	{
		//darkplaces7 (it's a small difference from dp5)
		cls.protocol_nq = CPNQ_DP7;
		netprim.coordtype = COORDTYPE_FLOAT_32;
		netprim.anglesize = 2;

		cls.z_ext = Z_EXT_VIEWHEIGHT;

		Con_DPrintf("DP7 protocols\n");
	}
	else if (protover == PROTOCOL_VERSION_H2)
	{
		if (cls.demoplayback)
			cls.protocol_nq = CPNQ_H2MP;
		else
			Host_EndGame ("\nUnable to connect to standard Hexen2 servers. Host the game with "DISTRIBUTION"\n");
	}
	else if (protover == PROTOCOL_VERSION_BJP1)
	{
		cls.protocol_nq = CPNQ_BJP1;
		Con_DPrintf("bjp1 %i protocol\n", PROTOCOL_VERSION_BJP1);
	}
	else if (protover == PROTOCOL_VERSION_BJP2)
	{
		cls.protocol_nq = CPNQ_BJP2;
		Con_DPrintf("bjp2 %i protocol\n", PROTOCOL_VERSION_BJP2);
	}
	else if (protover == PROTOCOL_VERSION_BJP3)
	{
		cls.protocol_nq = CPNQ_BJP3;
		Con_DPrintf("bjp3 %i protocol\n", PROTOCOL_VERSION_BJP3);
	}
	else if (protover == PROTOCOL_VERSION_NQ)
		Con_DPrintf("Standard NQ protocols\n");
	else
		Host_EndGame ("Server is using protocol version %i, which is not supported by this version of " FULLENGINENAME ".", protover);
	if (cls.fteprotocolextensions & PEXT_FLOATCOORDS)
	{
		if (netprim.anglesize < 2)
			netprim.anglesize = 2;
		if (netprim.coordtype < COORDTYPE_FLOAT_32)
			netprim.coordtype = COORDTYPE_FLOAT_32;
	}
	cls.netchan.message.prim = cls.netchan.netprim = netprim;
	MSG_ChangePrimitives(netprim);
}

static int CL_Darkplaces_Particle_Precache(const char *pname)
{
	int i;
	for (i = 1; i < MAX_SSPARTICLESPRE; i++)
	{
		if (!cl.particle_ssname[i])
		{
			cl.particle_ssname[i] = strdup(pname);
			cl.particle_ssprecache[i] = P_FindParticleType(pname);
			cl.particle_ssprecaches = true;
			return i;
		}
		if (!strcmp(cl.particle_ssname[i], pname))
			return i;
	}
	return 0;	//failed
}

//FIXME: move to header
void CL_KeepaliveMessage(void){}
static void CLNQ_ParseServerData(void)		//Doesn't change gamedir - use with caution.
{
	int	nummodels, numsounds;
	char	*str = NULL;
	int gametype;
	Con_DPrintf ("Serverdata packet %s.\n", cls.demoplayback?"read":"received");
	SCR_SetLoadingStage(LS_CLIENT);
	CL_ClearState (true);
#ifdef QUAKEHUD
	Stats_NewMap();
#endif
	Cvar_ForceCallback(Cvar_FindVar("r_particlesdesc"));

	CLNQ_ParseProtoVersion();

	if (cls.qex)
	{
		cl.allocated_client_slots = MSG_ReadPlayer();
		str = MSG_ReadString();
	}
	else
	{
		if (cls.fteprotocolextensions2 & PEXT2_PREDINFO)
			str = MSG_ReadString();
		cl.allocated_client_slots = MSG_ReadPlayer();
	}
	if (str)
	{
#ifndef CLIENTONLY
		if (!sv.state)
#endif
			COM_Gamedir(str, NULL);
	}
	if (cl.allocated_client_slots > MAX_CLIENTS)
	{
		cl.allocated_client_slots = MAX_CLIENTS;
		Con_Printf ("\nWarning, this server supports more than %i clients, additional clients will do bad things\n", MAX_CLIENTS);
	}

	cl.splitclients = 1;


	gametype = MSG_ReadByte ();

	str = MSG_ReadString ();
	Q_strncpyz (cl.levelname, str, sizeof(cl.levelname));

	// seperate the printfs so the server message can have a color
#if 1
	Con_Printf ("\n\n");
	Con_Printf ("^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f");
	Con_Printf ("\n\n");
	Con_Printf ("\1%s\n", str);
#else
	Con_TPrintf ("\n\n^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f\n\n");
	Con_Printf ("%c%s\n", 2, str);
#endif

	SCR_BeginLoadingPlaque();

	Surf_PreNewMap();

	for (nummodels=0; nummodels < countof(cl.model_name); nummodels++)
	{
		if (cl.model_name[nummodels])
		{
			Z_Free(cl.model_name[nummodels]);
			cl.model_name[nummodels] = NULL;
		}
	}
	for (nummodels=1 ; ; nummodels++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (nummodels==MAX_PRECACHE_MODELS)
		{
			Con_TPrintf ("Server sent too many model precaches\n");
			return;
		}
		Z_StrDupPtr(&cl.model_name[nummodels], str);
		if (*str != '*' && strcmp(str, "null"))	//not inline models!
			CL_CheckOrEnqueDownloadFile(str, NULL, ((nummodels==1)?DLLF_REQUIRED|DLLF_ALLOWWEB:0));

		//qw has a special network protocol for spikes.
		if (!strcmp(cl.model_name[nummodels],"progs/spike.mdl"))
			cl_spikeindex = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/player.mdl"))
			cl_playerindex = nummodels;
#ifdef HAVE_LEGACY
		if (cl.model_name_vwep[0] && !strcmp(cl.model_name[nummodels],cl.model_name_vwep[0]) && cl_playerindex == -1)
			cl_playerindex = nummodels;
#endif
		if (!strcmp(cl.model_name[nummodels],"progs/h_player.mdl"))
			cl_h_playerindex = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/flag.mdl"))
			cl_flagindex = nummodels;

		//rocket to grenade
		if (!strcmp(cl.model_name[nummodels],"progs/missile.mdl"))
			cl_rocketindex = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/grenade.mdl"))
			cl_grenadeindex = nummodels;

		//cl_gibfilter
		if (!strcmp(cl.model_name[nummodels],"progs/gib1.mdl"))
			cl_gib1index = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/gib2.mdl"))
			cl_gib2index = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/gib3.mdl"))
			cl_gib3index = nummodels;

		Mod_TouchModel (str);
	}

	for (numsounds=0; numsounds < countof(cl.sound_name); numsounds++)
	{
		if (cl.sound_name[numsounds])
		{
			Z_Free(cl.sound_name[numsounds]);
			cl.sound_name[numsounds] = NULL;
		}
	}
	for (numsounds=1 ; ; numsounds++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (numsounds==MAX_PRECACHE_SOUNDS)
		{
			Con_TPrintf ("Server sent too many sound precaches\n");
			return;
		}
		Z_StrDupPtr(&cl.sound_name[numsounds], str);
		cl.sound_precache[numsounds] = S_FindName(cl.sound_name[numsounds], true, false);

		Sound_CheckDownload(str);
	}

	cls.signon = 0;
	cls.state = ca_onserver;
	Cam_AutoTrack_Update(NULL);


	//fill in the csqc stuff
	if (!cl_dp_csqc_progscrc)
	{
		InfoBuf_RemoveKey(&cl.serverinfo, "*csprogs");
		InfoBuf_RemoveKey(&cl.serverinfo, "*csprogssize");
		InfoBuf_RemoveKey(&cl.serverinfo, "*csprogsname");
	}
	else
	{
		InfoBuf_SetStarKey(&cl.serverinfo, "*csprogs",		va("%i", cl_dp_csqc_progscrc));
		InfoBuf_SetStarKey(&cl.serverinfo, "*csprogssize", va("%i", cl_dp_csqc_progssize));
		InfoBuf_SetStarKey(&cl.serverinfo, "*csprogsname", va("%s", cl_dp_csqc_progsname));
	}

	if (cl_dp_packagenames)
	{
		char *in = cl_dp_packagenames;
		if (cl.serverpacknames)
			Z_StrCat(&cl.serverpacknames, " ");
		Z_StrCat(&cl.serverpacknames, in);
		while ((in = COM_Parse(in)))
		{
			Z_StrCat(&cl.serverpackhashes, cl.serverpackhashes?" -":"-");	//no hash info.
			cl.serverpakschanged = true;
		}
	}


	//update gamemode
	if (gametype != GAME_COOP)
		InfoBuf_SetKey(&cl.serverinfo, "deathmatch", "1");
	else
		InfoBuf_SetKey(&cl.serverinfo, "deathmatch", "0");
	InfoBuf_SetKey(&cl.serverinfo, "teamplay", "0");

	//allow some things by default that quakeworld bans by default
	InfoBuf_SetKey(&cl.serverinfo, "watervis", "1");

	//prohibit some things that QW/FTE has enabled by default, which would be frowned upon in NQ
	InfoBuf_SetKey(&cl.serverinfo, "fbskins", "0");

	//pretend it came from the server, and update cheat/permissions/etc
	CL_CheckServerInfo();

	#if _MSC_VER > 1200
	Sys_RecentServer("+connectnq", cls.servername, cls.servername, "Join NQ Server");
	#endif

	if (CPNQ_IS_DP)	//DP's protocol requires client+server to have exactly the same data files. this is shit, but in the interests of compatibility...
		COM_Effectinfo_Enumerate(CL_Darkplaces_Particle_Precache);

#ifdef VOICECHAT
	S_Voip_MapChange();
#endif

#ifdef CSQC_DAT
	CSQC_Shutdown();
#endif
}
static void CLQEX_ParseServerVars(void)
{
	unsigned int bits = MSG_ReadULEB128();

	if (bits & QEX_GV_DEATHMATCH)
		InfoBuf_SetStarKey(&cl.serverinfo, "deathmatch", va("%i", MSG_ReadByte ()));
	if (bits & QEX_GV_IDEALPITCHSCALE)
		MSG_ReadFloat ();
	if (bits & QEX_GV_FRICTION)
		movevars.friction = MSG_ReadFloat ();
	if (bits & QEX_GV_EDGEFRICTION)
		InfoBuf_SetStarKey(&cl.serverinfo, "pm_edgefriction", va("%g", MSG_ReadFloat ()));
	if (bits & QEX_GV_STOPSPEED)
		movevars.stopspeed = MSG_ReadFloat ();
	if (bits & QEX_GV_MAXVELOCITY)
		/*movevars.maxvelocity =*/ MSG_ReadFloat ();
	if (bits & QEX_GV_GRAVITY)
		movevars.gravity = MSG_ReadFloat ();
	if (bits & QEX_GV_NOSTEP)
		/*movevars.nostep =*/ MSG_ReadByte ();
	if (bits & QEX_GV_MAXSPEED)
		movevars.maxspeed = MSG_ReadFloat ();
	if (bits & QEX_GV_ACCELERATE)
		movevars.accelerate = MSG_ReadFloat ();
	if (bits & QEX_GV_CONTROLLERONLY)
		InfoBuf_SetStarKey(&cl.serverinfo, "nomouse", va("%i", MSG_ReadByte ()));
	if (bits & QEX_GV_TIMELIMIT)
		InfoBuf_SetStarKey(&cl.serverinfo, "timelimit", va("%g", MSG_ReadFloat ()));
	if (bits & QEX_GV_FRAGLIMIT)
		InfoBuf_SetStarKey(&cl.serverinfo, "fraglimit", va("%g", MSG_ReadFloat ()));
	if (bits & QEX_GV_TEAMPLAY)
		InfoBuf_SetStarKey(&cl.serverinfo, "teamplay", va("%i", MSG_ReadByte ()));
	if (bits & ~QEX_GV_ALL)
		Con_Printf("CLQEX_ParseServerVars: Unknown bits %#x\n", bits & ~QEX_GV_ALL);

	CL_CheckServerInfo();
}
static void CLQEX_ParsePrompt(void)
{
	int a, count = MSG_ReadByte(), imp;
	const char *s;
	char message[65536];
	size_t ofs = 0;

	if (count == 0)
	{
		SCR_CenterPrint(0, NULL, true);
		return;
	}
	*message = 0;
	s = MSG_ReadString();
	Q_strncatz(message+ofs, "/S/C/.", sizeof(message)-ofs);
	ofs += strlen(message+ofs);
	TL_Reformat(com_language, message+ofs, sizeof(message)-ofs, 1, &s);
	ofs += strlen(message+ofs);

	Q_strncatz(message+ofs, "\n", sizeof(message)-ofs);
	ofs += strlen(message+ofs);
	for (a = 0; a < count; a++)
	{
		s = MSG_ReadString();
		imp = MSG_ReadByte();
		Q_strncatz(message+ofs, "^[[", sizeof(message)-ofs);
		ofs += strlen(message+ofs);
		TL_Reformat(com_language, message+ofs, sizeof(message)-ofs, 1, &s);
		ofs += strlen(message+ofs);
		Q_strncatz(message+ofs, va("]\\impulse\\%i^]\n", imp), sizeof(message)-ofs);
		ofs += strlen(message+ofs);
	}
	SCR_CenterPrint(0, message, true);
}
static char *CLQEX_ReadStrings(void)
{
	unsigned short count = MSG_ReadShort(), a;
	const char *arg[256];
	static char formatted[8192];
	char inputs[65536];
	size_t ofs = 0;
	for (a = 0; a < count && a < countof(arg); )
	{
		arg[a++] = MSG_ReadStringBuffer(inputs+ofs, sizeof(inputs)-ofs-1);
		ofs += strlen(inputs+ofs)+1;
		if (ofs >= sizeof(inputs))
			break;
	}
	for (; a < count; a++)
		MSG_ReadString(); //don't lose space, though we can't buffer it.

	TL_Reformat(com_language, formatted, sizeof(formatted), a, arg);
	return formatted;
}

static void CLNQ_SendInitialUserInfo(void *ctx, const char *key, const char *value)
{
	InfoSync_Add(&cls.userinfosync, ctx, key);
}
void CLNQ_SignonReply (void)
{
	extern cvar_t	topcolor;
	extern cvar_t	bottomcolor;
	extern cvar_t	rate;
	extern cvar_t	model;
	extern cvar_t	skin;

Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		cl.sendprespawn = true;
		SCR_SetLoadingFile("loading data");
		CL_RequestNextDownload();	//this sucks, but sometimes mods send csqc-specific messages to us before things are properly inited. if we start doing stuff now then we can minimize the chances of dodgy mods screwing with us. FIXME: warn about receiving csqc messages before begin.
		break;

	case 2:
		CL_SendClientCommand(true, "name \"%s\"\n", name.string);
		CL_SendClientCommand(true, "color %i %i\n", topcolor.ival, bottomcolor.ival);
		if (cl.haveserverinfo)
			InfoBuf_Enumerate(&cls.userinfo[0], &cls.userinfo[0], CLNQ_SendInitialUserInfo);
		else if (CPNQ_IS_DP)
		{	//dp needs a couple of extras to work properly in certain cases. don't send them on other servers because that generally results in error messages.
			CL_SendClientCommand(true, "rate %s", rate.string);
			CL_SendClientCommand(true, "playermodel %s", model.string);
			CL_SendClientCommand(true, "playerskin %s", skin.string);
		}
		CL_SendClientCommand(true, "spawn %s", "");
		break;

	case 3:
		CL_SendClientCommand(true, "begin");
		break;

	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		SCR_SetLoadingStage(LS_NONE);
		break;
	}
}

#define	DEFAULT_VIEWHEIGHT	22
static void CLNQ_ParseClientdata (void)
{
	int		i;
	const int seat = 0;
	player_state_t *pl = &cl.inframes[cl.validsequence&UPDATE_MASK].playerstate[cl.playerview[seat].playernum];

	unsigned int bits;

	bits = (unsigned short)MSG_ReadShort();

	if (bits & SU_EXTEND1)
		bits |= (MSG_ReadByte() << 16);
	if (bits & SU_EXTEND2)
		bits |= (MSG_ReadByte() << 24);

	if (bits & SU_VIEWHEIGHT)
		CL_SetStatInt(0, STAT_VIEWHEIGHT, MSG_ReadChar ());
	else if ((!CPNQ_IS_DP || cls.protocol_nq <= CPNQ_DP5) && cls.protocol_nq != CPNQ_H2MP)
		CL_SetStatInt(0, STAT_VIEWHEIGHT, DEFAULT_VIEWHEIGHT);

	if (bits & SU_IDEALPITCH)
		CL_SetStatInt(0, STAT_IDEALPITCH, MSG_ReadChar ());
	else if (cls.protocol_nq != CPNQ_H2MP)
		CL_SetStatInt(0, STAT_IDEALPITCH, 0);

	if (cls.protocol_nq == CPNQ_H2MP && (bits & (1<<8)/*SU_IDEALROLL*/))
		MSG_ReadChar ();

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i) )
			CL_SetStatFloat(seat, STAT_PUNCHANGLE_X+i, CPNQ_IS_DP?MSG_ReadAngle16():MSG_ReadChar());
		else if (cls.protocol_nq != CPNQ_H2MP)
			CL_SetStatFloat(seat, STAT_PUNCHANGLE_X+i, 0);

		if (CPNQ_IS_DP && bits & (DPSU_PUNCHVEC1<<i))
			CL_SetStatFloat(seat, STAT_PUNCHVECTOR_X+i, MSG_ReadCoord());
		else if (cls.protocol_nq != CPNQ_H2MP)
			CL_SetStatFloat(seat, STAT_PUNCHVECTOR_X+i, 0);

		if (bits & (SU_VELOCITY1<<i))
		{
			if (CPNQ_IS_DP || (cls.qex && (bits & QEX_SU_FLOATCOORDS)))
				pl->velocity[i] = MSG_ReadFloat();
			else
				pl->velocity[i] = MSG_ReadChar()*16;
		}
		else if (cls.protocol_nq != CPNQ_H2MP)
			pl->velocity[i] = 0;
	}

	if ((bits & SU_ITEMS) || cls.protocol_nq == CPNQ_ID)	//hipnotic bug - hipnotic demos don't always have SU_ITEMS set, yet they update STAT_ITEMS anyway.
		CL_SetStatInt(0, STAT_ITEMS, MSG_ReadLong());

	pl->onground = (bits & SU_ONGROUND) != 0;
	if (bits & SU_INWATER)
		pl->flags |= PF_INWATER;	//mostly just means smartjump should be used.
	else
		pl->flags &= ~PF_INWATER;

	if (cls.protocol_nq == CPNQ_DP5)
	{
		CL_SetStatInt(0, STAT_WEAPONFRAME, (bits & SU_WEAPONFRAME)?(unsigned short)MSG_ReadShort():0);
		CL_SetStatInt(0, STAT_ARMOR, (bits & SU_ARMOR)?MSG_ReadShort():0);
		CL_SetStatInt(0, STAT_WEAPONMODELI, (bits & SU_WEAPONMODEL)?MSG_ReadShort():0);

		CL_SetStatInt(0, STAT_HEALTH, MSG_ReadShort());

		CL_SetStatInt(0, STAT_AMMO, MSG_ReadShort());

		CL_SetStatInt(0, STAT_SHELLS, MSG_ReadShort());
		CL_SetStatInt(0, STAT_NAILS, MSG_ReadShort());
		CL_SetStatInt(0, STAT_ROCKETS, MSG_ReadShort());
		CL_SetStatInt(0, STAT_CELLS, MSG_ReadShort());

		CL_SetStatInt(0, STAT_ACTIVEWEAPON, (unsigned short)MSG_ReadShort());
	}
	else if (CPNQ_IS_DP && cls.protocol_nq > CPNQ_DP5)
	{
		/*nothing in dp6+*/
	}
	else if (cls.protocol_nq == CPNQ_H2MP)
	{	//only changed stuff
		if (bits & SU_WEAPONFRAME)	CL_SetStatInt(0, STAT_WEAPONFRAME, MSG_ReadByte());
		if (bits & SU_ARMOR)		CL_SetStatInt(0, STAT_ARMOR, MSG_ReadByte());
		if (bits & SU_WEAPONMODEL)	CL_SetStatInt(0, STAT_WEAPONMODELI, MSG_ReadUInt16());
	}	//nothing else.
	else
	{
		int weaponmodel = 0, armour = 0, weaponframe = 0, health = 0, currentammo = 0, shells = 0, nails = 0, rockets = 0, cells = 0, activeweapon = 0;

		if (bits & SU_WEAPONFRAME)	weaponframe |= (unsigned char)MSG_ReadByte();
		if (bits & SU_ARMOR)		armour |= (unsigned char)MSG_ReadByte();
		if (bits & SU_WEAPONMODEL)
		{
			if (CPNQ_IS_BJP)
				weaponmodel |= (unsigned short)MSG_ReadShort();
			else
				weaponmodel |= (unsigned char)MSG_ReadByte();
		}
		health |= MSG_ReadShort();
		currentammo |= MSG_ReadByte();
		shells |= MSG_ReadByte();
		nails |= MSG_ReadByte();
		rockets |= MSG_ReadByte();
		cells |= MSG_ReadByte();
		activeweapon |= MSG_ReadByte();

		if (cls.protocol_nq == CPNQ_FITZ666)
		{
			if (bits & FITZSU_WEAPONMODEL2)
				weaponmodel |= MSG_ReadByte() << 8;
			if (bits & FITZSU_ARMOR2)
				armour |= MSG_ReadByte() << 8;
			if (bits & FITZSU_AMMO2)
				currentammo |= MSG_ReadByte() << 8;
			if (bits & FITZSU_SHELLS2)
				shells |= MSG_ReadByte() << 8;
			if (bits & FITZSU_NAILS2)
				nails |= MSG_ReadByte() << 8;
			if (bits & FITZSU_ROCKETS2)
				rockets |= MSG_ReadByte() << 8;
			if (bits & FITZSU_CELLS2)
				cells |= MSG_ReadByte() << 8;
			if (bits & FITZSU_WEAPONFRAME2)
				weaponframe |= MSG_ReadByte() << 8;
			if (bits & FITZSU_WEAPONALPHA)
				MSG_ReadByte();

			if (cls.qex)
			{
				if (bits & QEX_SU_ENTFLAGS)	/*entflags =*/ MSG_ReadULEB128();
			}
		}

		CL_SetStatInt(0, STAT_WEAPONFRAME, weaponframe);
		CL_SetStatInt(0, STAT_ARMOR, armour);
		CL_SetStatInt(0, STAT_WEAPONMODELI, weaponmodel);

		CL_SetStatInt(0, STAT_HEALTH, health);

		CL_SetStatInt(0, STAT_AMMO, currentammo);

		CL_SetStatInt(0, STAT_SHELLS, shells);
		CL_SetStatInt(0, STAT_NAILS, nails);
		CL_SetStatInt(0, STAT_ROCKETS, rockets);
		CL_SetStatInt(0, STAT_CELLS, cells);

		CL_SetStatInt(0, STAT_ACTIVEWEAPON, activeweapon);
	}

	if (CPNQ_IS_DP)
	{
		if (bits & DPSU_VIEWZOOM)
		{
			if (cls.protocol_nq >= CPNQ_DP5)
				i = (unsigned short) MSG_ReadShort();
			else
				i = MSG_ReadByte();
			if (i < 2)
				i = 2;
			CL_SetStatFloat(0, STAT_VIEWZOOM, i*(STAT_VIEWZOOM_SCALE/255.0));
		}
		else
			CL_SetStatFloat(0, STAT_VIEWZOOM, STAT_VIEWZOOM_SCALE);
	}
}
#endif
/*
==================
CL_ParseSoundlist
==================
*/
static void CL_ParseSoundlist (qboolean lots)
{
	int	numsounds;
	char	*str;
	int n;

// precache sounds
//	memset (cl.sound_precache, 0, sizeof(cl.sound_precache));

	if (lots)
		numsounds = MSG_ReadShort();
	else
		numsounds = (cl.protocol_qw>=26)?MSG_ReadByte():0;

	for (;;)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		numsounds++;
		if (numsounds >= MAX_PRECACHE_SOUNDS)
			Host_EndGame ("Server sent too many sound_precache");

//		if (strlen(str)>4)
//		if (!strcmp(str+strlen(str)-4, ".mp3"))	//don't let the server send us a specific mp3. convert it to wav and this way we know not to look outside the quake path for it.
//			strcpy(str+strlen(str)-4, ".wav");

		Z_StrDupPtr(&cl.sound_name[numsounds], str);
	}

	n = (cl.protocol_qw>=26)?MSG_ReadByte():0;

	if (n)
	{
		if (cls.demoplayback == DPB_MVD && (cls.demoeztv_ext&EZTV_DOWNLOAD))
			;
		else
		{
			if (CL_RemoveClientCommands("soundlist") && !(cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))
				Con_DPrintf("Multiple soundlists\n");
//			CL_SendClientCommand("soundlist %i %i", cl.servercount, n);
			CL_SendClientCommand(true, soundlist_name, cl.servercount, (numsounds&0xff00) + n);
		}
		return;
	}

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		CL_AllowIndependantSendCmd(false);	//stop it now, the indep stuff *could* require model tracing.

		cl.sendprespawn = true;
		SCR_SetLoadingFile("loading data");
	}
	else
#endif
	{
		if (cls.demoplayback == DPB_MVD && cls.demoeztv_ext)
		{
			if (CL_RemoveClientCommands("qtvmodellist"))
				Con_DPrintf("Multiple modellists\n");
			CL_SendClientCommand (true, "qtvmodellist %i 0", cl.servercount);
		}
		else
		{
			if (CL_RemoveClientCommands("modellist") && !(cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))
				Con_DPrintf("Multiple modellists\n");
//			CL_SendClientCommand ("modellist %i 0", cl.servercount);
			CL_SendClientCommand (true, modellist_name, cl.servercount, 0);
		}
	}
}

/*
==================
CL_ParseModellist
==================
*/
static void CL_ParseModellist (qboolean lots)
{
	int	nummodels;
	char	*str;
	int n;

// precache models and note certain default indexes
	if (lots)
		nummodels = MSG_ReadShort();
	else
		nummodels = (cl.protocol_qw>=26)?MSG_ReadByte():0;

	for (;;)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		nummodels++;
		if (nummodels>=MAX_PRECACHE_MODELS)
			Host_EndGame ("Server sent too many model_precache");
		Z_StrDupPtr(&cl.model_name[nummodels], str);
		if (nummodels==1)
			SCR_ImageName(cl.model_name[nummodels]);

		//qw has a special network protocol for spikes.
		if (!strcmp(cl.model_name[nummodels],"progs/spike.mdl"))
			cl_spikeindex = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/player.mdl"))
			cl_playerindex = nummodels;
#ifdef HAVE_LEGACY
		if (cl.model_name_vwep[0] && !strcmp(cl.model_name[nummodels],cl.model_name_vwep[0]) && cl_playerindex == -1)
			cl_playerindex = nummodels;
#endif
		if (!strcmp(cl.model_name[nummodels],"progs/h_player.mdl"))
			cl_h_playerindex = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/flag.mdl"))
			cl_flagindex = nummodels;

		//rocket to grenade
		if (!strcmp(cl.model_name[nummodels],"progs/missile.mdl"))
			cl_rocketindex = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/grenade.mdl"))
			cl_grenadeindex = nummodels;

		//cl_gibfilter
		if (!strcmp(cl.model_name[nummodels],"progs/gib1.mdl"))
			cl_gib1index = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/gib2.mdl"))
			cl_gib2index = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/gib3.mdl"))
			cl_gib3index = nummodels;

		//we have the names, we might as well START loading them now.
		if (COM_HasWorkers(WG_LOADER))
			Mod_ForName (cl.model_name[nummodels], MLV_SILENT);
	}

	n = (cl.protocol_qw>=26)?MSG_ReadByte():0;

	if (n)
	{
		if (cls.demoplayback == DPB_MVD && cls.demoeztv_ext)
			;
		else
		{
			if (CL_RemoveClientCommands("modellist") && !(cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))
				Con_DPrintf("Multiple modellists\n");
//			CL_SendClientCommand("modellist %i %i", cl.servercount, n);
			CL_SendClientCommand(true, modellist_name, cl.servercount, (nummodels&0xff00) + n);
		}
		return;
	}

#ifdef QUAKESTATS
	if (cls.demoplayback == DPB_MVD && !cl.model_name_vwep[0] && !(cls.fteprotocolextensions2&PEXT2_REPLACEMENTDELTAS))
		CL_ParseStuffCmd("//vwep vwplayer w_axe w_shot w_shot2 w_nail w_nail2 w_rock w_rock2 w_light\n", 0);
#endif

	SCR_SetLoadingFile("loading data");

	//we need to try to load it now if we can, so any embedded archive will be loaded *before* we start looking for other content...
	cl.model_precache[1] = cl.model_name[1]?Mod_ForName (cl.model_name[1], MLV_SILENTSYNC):NULL;
	if (cl.model_precache[1] && cl.model_precache[1]->loadstate == MLS_LOADED)
		FS_LoadMapPackFile(cl.model_precache[1]->name, cl.model_precache[1]->archive);

	Sound_CheckDownloads();
	Model_CheckDownloads();

	CL_AllowIndependantSendCmd(false);	//stop it now, the indep stuff *could* require model tracing.

	//set the flag to load models and send prespawn
	cl.sendprespawn = true;
}

#ifdef Q2CLIENT
static void CLQ2_ParseClientinfo(int i, char *s)
{
	char *model, *name;
	player_info_t *player;
	//s contains "name\model/skin"
	//q2 doesn't really do much with userinfos.

	if (i >= MAX_CLIENTS)
		return;

	player = &cl.players[i];

	InfoBuf_Clear(&player->userinfo, true);
	cl.players[i].userinfovalid = true;

	model = strchr(s, '\\');
	if (model)
	{
		*model = '\0';
		model++;
		name = s;
	}
	else
	{
		name = "Unnammed";
		model = "male";
	}
#if 0
	skin = strchr(model, '/');
	if (skin)
	{
		*skin = '\0';
		skin++;
	}
	else
		skin = "";
	InfoBuf_SetValueForKey(&player->userinfo, "model", model);
	InfoBuf_SetValueForKey(&player->userinfo, "skin", skin);
#else
	InfoBuf_SetValueForKey(&player->userinfo, "skin", model);
#endif
	InfoBuf_SetValueForKey(&player->userinfo, "name", name);

	cl.players[i].userid = i;
	cl.players[i].rbottomcolor = 1;
	cl.players[i].rtopcolor = 1;
	CL_ProcessUserInfo (i, player);
}

void CLQ2EX_ParseLightConfigString(int i, const char *s);
static void CLQ2_UpdateConfigString (unsigned int i, char *s)
{
	if (i >= 0x8000 && i < 0x8000+MAX_PRECACHE_MODELS)
	{
		i -= 0x8000;
		goto parsemodelindex;
	}
	else if (i >= 0xc000 && i < 0xc000+MAX_PRECACHE_SOUNDS)
	{
		i -= 0xc000;
		goto parsesoundindex;
	}
	if (cls.protocol_q2 != PROTOCOL_VERSION_Q2EX)
	{	//remap from vanilla to q2e
#define PASTE(a,b) (a##b)
#define REMAPR(n,l) 		if (i >= Q2CS_##n && i < Q2CS_##n+Q2MAX_##l) i = i-Q2CS_##n+Q2EXCS_##n; else
#define REMAPS(n)			if (i == PASTE(Q2CS_,n)) i = i-PASTE(Q2CS_,n)+PASTE(Q2EXCS_,n); else
#define Q2MAX_STATUSBAR (Q2CS_AIRACCEL-Q2CS_STATUSBAR)
		REMAPS(NAME)
		REMAPS(CDTRACK)
		REMAPS(SKY)
		REMAPS(SKYAXIS)
		REMAPS(SKYROTATE)
		REMAPR(STATUSBAR, STATUSBAR)
		REMAPS(AIRACCEL)
		REMAPS(MAXCLIENTS)
		REMAPS(MAPCHECKSUM)
		REMAPR(MODELS, MODELS)
		REMAPR(SOUNDS, SOUNDS)
		REMAPR(IMAGES, IMAGES)
		REMAPR(LIGHTS, LIGHTSTYLES)
		REMAPR(ITEMS, ITEMS)
		REMAPR(PLAYERSKINS, CLIENTS)
		REMAPR(GENERAL, GENERAL)
		Host_EndGame ("configstring %i > Q2MAX_CONFIGSTRINGS", i);
	}

	if ((unsigned int)i >= Q2EXMAX_CONFIGSTRINGS)
		Host_EndGame ("configstring %i > Q2EXMAX_CONFIGSTRINGS", i);

//	strncpy (olds, cl.configstrings[i], sizeof(olds));
//	olds[sizeof(olds) - 1] = 0;

//	strcpy (cl.configstrings[i], s);

	// do something apropriate

	if (i == Q2EXCS_NAME)
	{
		Q_strncpyz (cl.levelname, s, sizeof(cl.levelname));
	}
	else if (i == Q2EXCS_SKY)
		R_SetSky(s);
	else if (i == Q2EXCS_SKYAXIS || i == Q2EXCS_SKYROTATE)
	{
		if (i == Q2EXCS_SKYROTATE)
		{
			s = COM_Parse(s);
			cl.skyrotate = atof(com_token);
			s = COM_Parse(s);
			if (*com_token)
				cl.skyautorotate = atoi(com_token);
		}
		else
		{
			s = COM_Parse(s);
			if (s)
			{
				cl.skyaxis[0] = atof(com_token);
				s = COM_Parse(s);
				if (s)
				{
					cl.skyaxis[1] = atof(com_token);
					s = COM_Parse(s);
					if (s)
						cl.skyaxis[2] = atof(com_token);
				}
			}
		}

		if (cl.skyrotate)
		{
			if (cl.skyaxis[0]||cl.skyaxis[1]||cl.skyaxis[2])
				Cvar_LockFromServer(&r_skybox_orientation, va("%g %g %g %g", cl.skyaxis[0], cl.skyaxis[1], cl.skyaxis[2], cl.skyrotate));
			else
				Cvar_LockFromServer(&r_skybox_orientation, va("0 0 1 %g", cl.skyrotate));
		}
		else
			Cvar_LockFromServer(&r_skybox_orientation, "");
		Cvar_LockFromServer(&r_skybox_autorotate, va("%i", cl.skyautorotate));
	}
	else if (i == Q2EXCS_STATUSBAR)
	{
		Q_strncpyz(cl.q2statusbar, s, sizeof(cl.q2statusbar));
	}
	else if (i > Q2EXCS_STATUSBAR && i < Q2EXCS_AIRACCEL)
		; //trailing statusbar
	else if (i == Q2EXCS_MAXCLIENTS)
	{
		i = atoi(s);
		if (i > 1)
			cl.allocated_client_slots = i;
	}
	else if (i >= Q2EXCS_LIGHTS && i < Q2EXCS_LIGHTS+Q2EXMAX_LIGHTSTYLES)
	{
		R_UpdateLightStyle(i-Q2EXCS_LIGHTS, s, 1, 1, 1);
	}
	else if (i >= Q2EXCS_RTLIGHTS && i < Q2EXCS_RTLIGHTS+Q2EXMAX_RTLIGHTS)
	{
		i -= Q2EXCS_RTLIGHTS;
		CLQ2EX_ParseLightConfigString(i, s);
	}
	else if (i == Q2EXCS_CDTRACK)
	{
		Media_NamedTrack (s, NULL);
	}
	else if (i == Q2EXCS_AIRACCEL)
		Q_strncpyz(cl.q2airaccel, s, sizeof(cl.q2airaccel));
	else if (i >= Q2EXCS_MODELS && i < Q2EXCS_MODELS+Q2EXMAX_MODELS)
	{
		i-= Q2EXCS_MODELS;
parsemodelindex:
		if ((unsigned int)i >= countof(cl.model_name))
			return;
		if (*s == '/')
			s++;	//*sigh*
		if (i == 255)
			s = "*playermodel";	//something special.
		Z_StrDupPtr(&cl.model_name[i], s);
		if (cl.model_name[i][0] == '#')
		{
			if (cl.numq2visibleweapons < Q2MAX_VISIBLE_WEAPONS)
			{
				cl.q2visibleweapons[cl.numq2visibleweapons] = cl.model_name[i]+1;
				cl.numq2visibleweapons++;
			}
			cl.model_precache[i] = NULL;
		}
		else if (cl.contentstage)
			cl.model_precache[i] = Mod_ForName (cl.model_name[i], MLV_WARN);
	}
	else if (i >= Q2EXCS_SOUNDS && i < Q2EXCS_SOUNDS+Q2MAX_SOUNDS)
	{
		i-= Q2EXCS_SOUNDS;
parsesoundindex:
		if ((unsigned int)i >= countof(cl.sound_name))
			return;
		if (*s == '/')
			s++;	//*sigh*
		Z_StrDupPtr(&cl.sound_name[i], s);
		if (cl.contentstage)
			cl.sound_precache[i] = S_PrecacheSound (s);
	}
	else if (i >= Q2EXCS_IMAGES && i < Q2EXCS_IMAGES+Q2MAX_IMAGES)
	{
		i -= Q2EXCS_IMAGES;
		if ((unsigned int)i >= countof(cl.image_name))
			return;
		Z_StrDupPtr(&cl.image_name[i], s);
	}
	else if (i >= Q2EXCS_ITEMS && i < Q2EXCS_ITEMS+Q2MAX_ITEMS)
	{
		i -= Q2EXCS_ITEMS;
		if ((unsigned int)i >= countof(cl.item_name))
			return;
		Z_StrDupPtr(&cl.item_name[i], s);
	}
	else if (i >= Q2EXCS_GENERAL && i < Q2EXCS_GENERAL+Q2EXMAX_GENERAL)
	{
		i -= Q2EXCS_GENERAL;
		if ((unsigned int)i >= Q2MAX_CLIENTS)//countof(cl.configstring_general))
			return;
		Z_StrDupPtr(&cl.configstring_general[i], s);
	}
	else if (i >= Q2EXCS_PLAYERSKINS && i < Q2EXCS_PLAYERSKINS+Q2EXMAX_CLIENTS)
	{
		i -= Q2EXCS_GENERAL;
		i += Q2EXMAX_CLIENTS;
		if ((unsigned int)i >= countof(cl.configstring_general))
			return;
		Z_StrDupPtr(&cl.configstring_general[i], s);

		CLQ2_ParseClientinfo (i, s);
	}
	else if (i == Q2EXCS_MAPCHECKSUM)
	{
		int serverchecksum = (int)strtol(s, NULL, 10);
		if (cl.worldmodel)
		{
			if (cl.worldmodel->loadstate == MLS_LOADING)
				COM_WorkerPartialSync(cl.worldmodel, &cl.worldmodel->loadstate, MLS_LOADING);

			// the Q2 client normally exits here, however for our purposes we might as well ignore it
			if (cl.worldmodel->checksum != serverchecksum &&
				cl.worldmodel->checksum2!= serverchecksum)
				Con_Printf(CON_WARNING "WARNING: Client checksum does not match server checksum (%i != %i)", cl.worldmodel->checksum2, serverchecksum);
		}

		cl.q2mapchecksum = serverchecksum;
	}
	else if (i >= Q2ECS_WHEEL_WEAPONS && i < Q2ECS_WHEEL_WEAPONS+Q2EXMAX_WWHEEL)
		;
	else if (i >= Q2ECS_WHEEL_AMMO && i < Q2ECS_WHEEL_AMMO+Q2EXMAX_WWHEEL)
		;
	else if (i >= Q2ECS_WHEEL_POWERUPS && i < Q2ECS_WHEEL_POWERUPS+Q2EXMAX_WWHEEL)
		;
	else if (i == Q2ECS_CD_LOOP_COUNT)
		;
	else if (i == Q2ECS_GAME_STYLE)
		;

#define	Q2EXCS_SOUNDS			(Q2EXCS_MODELS			+Q2EXMAX_MODELS)
#define	Q2EXCS_IMAGES			(Q2EXCS_SOUNDS			+Q2EXMAX_SOUNDS)
#define	Q2EXCS_LIGHTS			(Q2EXCS_IMAGES			+Q2EXMAX_IMAGES)
#define	Q2EXCS_RTLIGHTS			(Q2EXCS_LIGHTS			+Q2EXMAX_LIGHTSTYLES)
	else
		Con_Printf(CON_WARNING"Config string %i unsupported\n", i);
}
static void CLQ2_ParseConfigString (void)
{
	unsigned int	i = MSG_ReadUInt16();
	char			*s = MSG_ReadString();
	CLQ2_UpdateConfigString(i, s);
}
#endif


qboolean CL_CheckBaselines (int size)
{
	int i;

	if (size < 0)
		return false;
	if (size > MAX_EDICTS)
		return false;

	size = (size + 64) & ~63; // round up to next 64
	if (size <= cl_baselines_count)
		return true;

	cl_baselines = BZ_Realloc(cl_baselines, sizeof(*cl_baselines)*size);
	for (i = cl_baselines_count; i < size; i++)
	{
		memcpy(cl_baselines + i, &nullentitystate, sizeof(*cl_baselines));
		if (cls.protocol == CP_NETQUAKE && cls.protocol_nq == CPNQ_H2MP)
			cl_baselines[i].hexen2flags = 0;
	}

	cl_baselines_count = size;

	return true;
}

/*
==================
CL_ParseBaseline
==================
*/
static void CL_ParseBaseline (entity_state_t *es, int baselinetype2)
{
	int			i;
	unsigned int bits;

	memcpy(es, &nullentitystate, sizeof(entity_state_t));

	if (baselinetype2 == CPNQ_FITZ666)
		bits = MSG_ReadByte();	//fitzquake has actual flags. yay extensibility. just a shame they're not the same as other entity updates.
	else if (baselinetype2 >= CPNQ_DP5 && baselinetype2 <= CPNQ_DP7)
		bits = FITZ_B_LARGEMODEL|FITZ_B_LARGEFRAME;	//dp's baseline2 always has these (regular baseline is unmodified)
	else if (cls.protocol == CP_NETQUAKE && CPNQ_IS_BJP)
		bits = FITZ_B_LARGEMODEL;	//bjp always uses shorts for models.
	else if (cls.protocol == CP_NETQUAKE && cls.protocol_nq == CPNQ_H2MP)
		bits = FITZ_B_LARGEMODEL;	//urgh
	else
		bits = 0;	//vanilla nq or qw

	es->modelindex = (bits & FITZ_B_LARGEMODEL) ? (unsigned short)MSG_ReadShort() : MSG_ReadByte();
	es->frame = (bits & FITZ_B_LARGEFRAME) ? (unsigned short)MSG_ReadShort() : MSG_ReadByte();
	es->colormap = MSG_ReadByte();
	es->skinnum = MSG_ReadByte();

	if (cls.protocol == CP_NETQUAKE && cls.protocol_nq == CPNQ_H2MP)
	{
		es->scale = (MSG_ReadByte()/100.0)*16;
		es->hexen2flags = MSG_ReadByte();
		es->abslight = MSG_ReadByte();
	}

	for (i=0 ; i<3 ; i++)
	{
		es->origin[i] = MSG_ReadCoord ();
		es->angles[i] = MSG_ReadAngle ();
	}

	es->trans = (bits & FITZ_B_ALPHA) ? MSG_ReadByte() : 255;
#ifdef NQPROT
	if (cls.qex)
	{
		if (bits & QEX_B_SOLID)
			/*es->solidtype =*/ MSG_ReadByte();
		if (bits & QEX_B_UNKNOWN4)
			Con_Printf(CON_WARNING"QEX_B_UNKNOWN4: %x\n", MSG_ReadByte());
		if (bits & QEX_B_UNKNOWN5)
			Con_Printf(CON_WARNING"QEX_B_UNKNOWN5: %x\n", MSG_ReadByte());
		if (bits & QEX_B_UNKNOWN6)
			Con_DPrintf(CON_WARNING"QEX_B_UNKNOWN6: %x\n", MSG_ReadByte());
		if (bits & QEX_B_UNKNOWN7)
			Con_Printf(CON_WARNING"QEX_B_UNKNOWN7: %x\n", MSG_ReadByte());
	}
	else
#endif
		es->scale = (bits & RMQFITZ_B_SCALE) ? MSG_ReadByte() : 16;
}
static void CL_ParseBaselineDelta (void)
{
	entity_state_t es;

	if (cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
		CLFTE_ParseBaseline(&es, true);
	else
		CLQW_ParseDelta(&nullentitystate, &es, (unsigned short)MSG_ReadShort());
	if (!CL_CheckBaselines(es.number))
		Host_EndGame("CL_ParseBaselineDelta: check baselines failed with size %i", es.number);
	memcpy(cl_baselines + es.number, &es, sizeof(es));
}

#ifdef Q2CLIENT
static void CLQ2_Precache_f (void)
{
	Model_CheckDownloads();
	Sound_CheckDownloads();

	cl.contentstage = 0;
	cl.sendprespawn = true;
	SCR_SetLoadingFile("loading data");
}
#endif



/*
=====================
CL_ParseStatic

Static entities are non-interactive world objects
like torches
=====================
*/
void R_StaticEntityToRTLight(int i);
static void CL_ParseStaticProt (int baselinetype)
{
	entity_t *ent;
	int		i;
	entity_state_t	es;
	vec3_t mins,maxs;

	if (baselinetype >= 0)
	{
		CL_ParseBaseline(&es, baselinetype);
		i = cl.num_statics;
		cl.num_statics++;
	}
	else
	{
		//new deltaed style ('full' extension support)
		if (cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
			CLFTE_ParseBaseline(&es, false);
		else
			CLQW_ParseDelta(&nullentitystate, &es, (unsigned short)MSG_ReadShort());

		if (!es.number)
			i = cl.num_statics++;
		else
		{
			es.number+=MAX_EDICTS;

			for (i = 0; i < cl.num_statics; i++)
				if (cl_static_entities[i].ent.keynum == es.number)
				{
					pe->DelinkTrailstate (&cl_static_entities[i].emit);
					break;
				}

			if (i == cl.num_statics)
				cl.num_statics++;
		}
	}

	if (i == cl_max_static_entities)
	{
		cl_max_static_entities += 16;
		cl_static_entities = BZ_Realloc(cl_static_entities, sizeof(*cl_static_entities)*cl_max_static_entities);
	}

	cl_static_entities[i].mdlidx = es.modelindex;
	cl_static_entities[i].emit = trailkey_null;

	cl_static_entities[i].state = es;
	ent = &cl_static_entities[i].ent;
	V_ClearEntity(ent);
	memset(&cl_static_entities[i].ent.pvscache, 0, sizeof(cl_static_entities[i].ent.pvscache));

	ent->keynum = es.number;

// copy it to the current state
	ent->model = cl.model_precache[es.modelindex];
	memset(&ent->framestate, 0, sizeof(ent->framestate));
	ent->framestate.g[FS_REG].frame[0] = ent->framestate.g[FS_REG].frame[1] = es.frame;
	ent->framestate.g[FS_REG].lerpweight[0] = 1;
	ent->skinnum = es.skinnum;
#ifdef HEXEN2
	ent->drawflags = es.hexen2flags;
	ent->abslight = es.abslight;
#endif

#ifdef PEXT_SCALE
	ent->scale = es.scale/16.0;
#endif
	ent->glowmod[0] = (8.0f/256.0f)*es.glowmod[0];
	ent->glowmod[1] = (8.0f/256.0f)*es.glowmod[1];
	ent->glowmod[2] = (8.0f/256.0f)*es.glowmod[2];
	ent->shaderRGBAf[0] = (8.0f/256.0f)*es.colormod[0];
	ent->shaderRGBAf[1] = (8.0f/256.0f)*es.colormod[1];
	ent->shaderRGBAf[2] = (8.0f/256.0f)*es.colormod[2];

	ent->fatness = es.fatness/16.0;

	ent->flags = 0;
	if (es.dpflags & RENDER_VIEWMODEL)
		ent->flags |= RF_WEAPONMODEL|Q2RF_MINLIGHT|RF_DEPTHHACK;
	if (es.dpflags & RENDER_EXTERIORMODEL)
		ent->flags |= RF_EXTERNALMODEL;
	if (es.effects & NQEF_ADDITIVE)
		ent->flags |= RF_ADDITIVE;
	if (es.effects & EF_NODEPTHTEST)
		ent->flags |= RF_NODEPTHTEST;
	if (es.effects & EF_NOSHADOW)
		ent->flags |= RF_NOSHADOW;
	if (es.trans < 0xfe)
	{
		ent->shaderRGBAf[3] = es.trans/(float)0xfe;
		ent->flags |= RF_TRANSLUCENT;
	}
	else
		ent->shaderRGBAf[3] = 1.0;

	VectorCopy (es.origin, ent->origin);
	VectorCopy (es.angles, ent->angles);
	if (ent->model && ent->model->type == mod_alias)
		AngleVectorsMesh(es.angles, ent->axis[0], ent->axis[1], ent->axis[2]);
	else
		AngleVectors(es.angles, ent->axis[0], ent->axis[1], ent->axis[2]);
	VectorInverse(ent->axis[1]);

	if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED)
		return;
	if (ent->model)
	{
		//FIXME: wait for model to load so we know the correct size?
		/*FIXME: compensate for angle*/
		VectorAdd(es.origin, ent->model->mins, mins);
		VectorAdd(es.origin, ent->model->maxs, maxs);
	}
	else
	{
		VectorCopy(es.origin, mins);
		VectorCopy(es.origin, maxs);
	}
	cl.worldmodel->funcs.FindTouchedLeafs(cl.worldmodel, &cl_static_entities[i].ent.pvscache, mins, maxs);

#ifdef RTLIGHTS
	//and now handle any rtlight fields on it
	R_StaticEntityToRTLight(i);
#endif
}

/*
===================
CL_ParseStaticSound
===================
*/
static void CL_ParseStaticSound (unsigned int flags)
{
	extern cvar_t cl_staticsounds;
	vec3_t		org;
	size_t		sound_num;
	float		vol, atten;
	int			i;

	if (flags & ~(1))
		Host_EndGame("CL_ParseStaticSound: unsupported flags & %x\n", flags&~(1));

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	if (flags || (cls.protocol == CP_NETQUAKE && (cls.protocol_nq == CPNQ_BJP2 || cls.protocol_nq == CPNQ_H2MP)))
	{
		if (cls.fteprotocolextensions2&PEXT2_LERPTIME)
			sound_num = (unsigned short)MSG_ReadULEB128();
		else
			sound_num = (unsigned short)MSG_ReadShort();
	}
	else
		sound_num = MSG_ReadByte ();
	vol = MSG_ReadByte ()/255.0;
	atten = MSG_ReadByte ()/64.0;

	if (sound_num >= countof(cl.sound_precache))
		return;	//no crashing, please.

	vol *= cl_staticsounds.value;
	if (vol < 0)
		return;

	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}



/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
static void CLQW_ParseStartSoundPacket(void)
{
	vec3_t  pos;
	int 	channel, ent;
	int 	sound_num;
	int 	volume;
	float 	attenuation;
	int		i;

	channel = MSG_ReadShort();

	if (channel & QWSND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

	if (channel & QWSND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	sound_num = MSG_ReadByte ();

	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();

	ent = (channel>>3)&1023;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_EndGame ("CL_ParseStartSoundPacket: ent = %i", ent);

#ifdef CSQC_DAT
	if (!CSQC_StartSound(ent, channel, cl.sound_name[sound_num], pos, volume/255.0, attenuation, 1, 0, 0))
#endif
	{
		if (!sound_num)
			S_StopSound(ent, channel);
		else
			S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, NULL, volume/255.0, attenuation, 0, 0, 0);
	}

#ifdef QUAKESTATS
	for (i = 0; i < cl.splitclients; i++)
	{
		if (ent == cl.playerview[i].playernum+1)
		{
			TP_CheckPickupSound(cl.sound_name[sound_num], pos, i);
			return;
		}
	}
	TP_CheckPickupSound(cl.sound_name[sound_num], pos, -1);
#endif
}

#ifdef Q2CLIENT
static void CLQ2_ParseStartSoundPacket(void)
{
	vec3_t  pos;
	int 	channel, ent;
	int 	sound_num;
	float 	volume;
	float 	attenuation;
	int		flags;
	float	ofs;
	sfx_t	*sfx;

	flags = MSG_ReadByte ();
	if (flags & Q2SND_EXTRABITS)
		flags |= MSG_ReadByte ()<<8;

	if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX || ((flags & Q2SNDFTE_LARGEIDX) && (cls.fteprotocolextensions & PEXT_SOUNDDBL)))
		sound_num = MSG_ReadUInt16();
	else
		sound_num = MSG_ReadByte ();

	if (flags & Q2SND_VOLUME)
		volume = MSG_ReadByte () / 255.0;
	else
		volume = Q2DEFAULT_SOUND_PACKET_VOLUME;

	if (flags & Q2SND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = Q2DEFAULT_SOUND_PACKET_ATTENUATION;

	if (flags & Q2SND_OFFSET)
		ofs = MSG_ReadByte () / 1000.0;
	else
		ofs = 0;

	if (flags & Q2SND_ENT)
	{	// entity reletive
		if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX && (flags & Q2SNDEX_LARGEENT))
			channel = MSG_ReadLong();
		else
			channel = MSG_ReadShort();
		ent = channel>>3;
		if (ent > MAX_EDICTS)
			Host_EndGame ("CL_ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	if (flags & Q2SND_POS)
	{	// positioned in space
		MSG_ReadPos (pos);

//FIXME
//		if (!(flags & Q2SNDEX_EXPLICITPOS))
//			CL_GetNumberedEntityInfo(ent, pos, NULL);
	}
	else	// use entity number
	{
		CL_GetNumberedEntityInfo(ent, pos, NULL);
	}

	if (!cl.sound_precache[sound_num])
		return;

	sfx = cl.sound_precache[sound_num];
	if (sfx->name[0] == '*')
	{	//a 'sexed' sound
		if (ent > 0 && ent <= MAX_CLIENTS)
		{
			char *model = InfoBuf_ValueForKey(&cl.players[ent-1].userinfo, "skin");
			char *skin;
			skin = strchr(model, '/');
			if (skin)
				*skin = '\0';
			if (*model)
				sfx = S_PrecacheSound(va("players/%s/%s", model, cl.sound_precache[sound_num]->name+1));
		}
		//fall back to male if it failed to load.
		//note: threaded loading can still make it silent the first time we hear it.
		if (sfx->loadstate == SLS_FAILED)
			sfx = S_PrecacheSound(va("players/male/%s", cl.sound_precache[sound_num]->name+1));
	}
	S_StartSound (ent, channel, sfx, pos, NULL, volume, attenuation, ofs, 0, 0);
}
#endif

//returns the player if they're not spectating. 
static int CL_TryTrackNum(playerview_t *pv)
{
	if (pv->spectator && pv->cam_state != CAM_FREECAM && pv->cam_spec_track >= 0)
		return pv->cam_spec_track;
	return pv->playernum;
}

#if defined(NQPROT) || defined(PEXT_SOUNDDBL)
static void CLNQ_ParseStartSoundPacket(void)
{
	vec3_t  pos, vel;
	int 	channel, ent;
	unsigned int 	sound_num;
	int 	volume;
	int 	field_mask;
	float 	attenuation;
 	int		i;
	float	pitchadj;
	float	timeofs;
	unsigned int flags;

	field_mask = MSG_ReadByte();

	if (field_mask & FTESND_MOREFLAGS)
		field_mask |= MSG_ReadUInt64()<<8;

	if (field_mask & NQSND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

	if (field_mask & NQSND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	if (field_mask & FTESND_PITCHADJ)
		pitchadj = MSG_ReadByte()/100.0;
	else
		pitchadj = 1;

	if (field_mask & FTESND_TIMEOFS)
		timeofs = MSG_ReadShort() / 1000.0;
	else
		timeofs = 0;

	if (field_mask & FTESND_VELOCITY)
	{
		vel[0] = MSG_ReadShort()/8.0;
		vel[1] = MSG_ReadShort()/8.0;
		vel[2] = MSG_ReadShort()/8.0;
	}
	else
		VectorClear(vel);

	if (field_mask & DPSND_SPEEDUSHORT4000)
		pitchadj = (unsigned short)MSG_ReadShort() / 4000.0;

	flags = field_mask>>8;
	flags &= CF_NETWORKED;

	if (field_mask & NQSND_LARGEENTITY)
	{
		ent = MSGCL_ReadEntity();
		channel = MSG_ReadByte();
	}
	else
	{	//regular
		channel = MSG_ReadShort ();
		ent = channel >> 3;
		channel &= 7;
	}

	//channel = (channel & 7) | ((channel & 0x0f1) << 1); //this line undoes the reliable=(channel&8) gap from qwssqc... but frankly just pass the flags arg properly. csqc's builtin doesn't use it, so don't give an inconsistent gap at all here.

	if ((field_mask & NQSND_LARGESOUND) || (cls.protocol == CP_NETQUAKE && (cls.protocol_nq == CPNQ_BJP2 || cls.protocol_nq == CPNQ_BJP3))) //bjp kinda sucks
		sound_num = (unsigned short)MSG_ReadShort();
	else
		sound_num = (unsigned char)MSG_ReadByte ();

	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();

	if (ent > MAX_EDICTS)
		Host_EndGame ("CL_ParseStartSoundPacket: ent = %i", ent);
	if (sound_num >= MAX_PRECACHE_SOUNDS)
		Host_EndGame ("CL_ParseStartSoundPacket: sndidx = %i", sound_num);

	if (!cl.sound_name[sound_num])
		return;	//nope, not precached yet... silly raqces.

#ifdef CSQC_DAT
	if (!CSQC_StartSound(ent, channel, cl.sound_name[sound_num], pos, volume/255.0, attenuation, pitchadj, timeofs, flags))
#endif
	{
		if (!sound_num)
			S_StopSound(ent, channel);
		else
			S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, vel, volume/255.0, attenuation, timeofs, pitchadj, flags);
	}

#ifdef QUAKESTATS
	for (i = 0; i < cl.splitclients; i++)
	{
		if (ent == cl.playerview[i].playernum+1)
		{
			TP_CheckPickupSound(cl.sound_name[sound_num], pos, i);
			return;
		}
	}
	TP_CheckPickupSound(cl.sound_name[sound_num], pos, -1);
#endif
}
#endif


/*
==================
CL_ParseClientdata

Server information pertaining to this client only, sent every frame
==================
*/
void CL_ParseClientdata (void)
{
	int				i;

// calculate simulated time of message
	oldparsecountmod = parsecountmod;

	i = cls.netchan.incoming_acknowledged;
#ifdef NQPROT
	if (cls.demoplayback == DPB_NETQUAKE)
	{
		i = cls.netchan.incoming_sequence-1;
		cl.oldparsecount = i - 1;
		oldparsecountmod = cl.oldparsecount & UPDATE_MASK;
	}
	else
#endif
	if (cls.demoplayback == DPB_MVD)
	{
		cl.oldparsecount = i - 1;
		oldparsecountmod = cl.oldparsecount & UPDATE_MASK;
	}
	cl.parsecount = i;
	parsecountmod = i&UPDATE_MASK;
	parsecounttime = realtime;//cl.outframes[i].senttime;

	if (cls.protocol == CP_QUAKEWORLD)
		CL_AckedInputFrame(cls.netchan.incoming_sequence, cl.parsecount, false);
}

#ifdef QWSKINS
static qboolean CLQ2_PlayerSkinIsOkay(skinid_t id)
{
	skinfile_t *sk = Mod_LookupSkin(id);
	if (!sk)	//err...
		return false;
	if (sk->nummappings != 1 || *sk->mappings[0].surface)
		return true;	//looks like its a custom skin, ignore it.
	return R_GetShaderSizes(sk->mappings[0].shader, NULL, NULL, true) > 0;
}
static int QDECL CLQ2_EnumeratedSkin(const char *name, qofs_t size, time_t mtime, void *ptr, searchpathfuncs_t *spath)
{
	//follows the form of players/$MODELNAME/$SKINNAME_i.$EXT
	player_info_t	*player = ptr;
	if (!player->skinid)
	{
		char *e;
		e = strstr(name, "_i.");
		if (e)
		{
			*e = 0;
			player->skinid = Mod_ReadSkinFile(va("%s.skin", name), va("replace \"\" \"%s.pcx\"", name));
		}
	}
	return true;
}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation (int slot)
{
	int		top, bottom;
	int local;
	qboolean mayforce;

	char *s;
	player_info_t	*player;

	if (slot >= MAX_CLIENTS)
		Host_Error ("CL_NewTranslation: slot > MAX_CLIENTS");

	player = &cl.players[slot];

	if (cls.protocol == CP_QUAKE2)
	{
		char *mod, *skin, *dogtag;
		player->qwskin = NULL;
		player->skinid = 0;
		player->model = NULL;
		player->ttopcolor = TOP_DEFAULT;
		player->tbottomcolor = BOTTOM_DEFAULT;

		mod = InfoBuf_ValueForKey(&player->userinfo, "skin");
		skin = strchr(mod, '/');
		if (skin)
		{
			*skin++ = 0;
			dogtag = strchr(skin, '\\');
			if (dogtag)
				*dogtag++ = 0;
		}
		if (!mod || !*mod)
			mod = "male";
		if (!skin || !*skin || !COM_FCheckExists(va("players/%s/%s.pcx", mod, skin)))
			skin = "grunt";

		player->model = Mod_ForName(va("players/%s/tris.md2", mod), MLV_WARNSYNC);
		if (player->model->loadstate == MLS_FAILED && strcmp(mod, "male"))
		{	//fall back on male if the model doesn't exist. yes, sexist, but also statistically most likely to represent the actual player.
			mod = "male";
			player->model = Mod_ForName(va("players/%s/tris.md2", mod), 0);
		}
		player->skinid = Mod_RegisterSkinFile(va("players/%s/%s.skin", mod,skin));
		if (!player->skinid)
			player->skinid = Mod_ReadSkinFile(va("players/%s/%s.skin", mod,skin), va("replace \"\" \"players/%s/%s.pcx\"", mod,skin));
		if (!CLQ2_PlayerSkinIsOkay(player->skinid))
		{
			player->skinid = 0;
			COM_EnumerateFiles(va("players/%s/*_i.*", mod), CLQ2_EnumeratedSkin, player);
		}
		return;
	}

	mayforce = !(cl.fpd & FPD_NO_FORCE_COLOR);
#if MAX_SPLITS > 1
	if (mayforce && cl.splitclients > 1 && cl.teamplay)
	{	//if we're using splitscreen, only allow team/enemy forcing if all split clients are on the same team
		char *needteam;
		int i;
		needteam = cl.players[CL_TryTrackNum(&cl.playerview[0])].team;
		for (i = 1; i < cl.splitclients; i++)
		{
			if (strcmp(needteam, cl.players[CL_TryTrackNum(&cl.playerview[i])].team))
			{
				mayforce = false;
				break;
			}
		}
	}
#endif

	s = Skin_FindName (player);
	COM_StripExtension(s, s, MAX_QPATH);
	if (player->qwskin && stricmp(s, player->qwskin->name))
		player->qwskin = NULL;
	player->skinid = 0;
	player->model = NULL;

	top = player->rtopcolor;
	bottom = player->rbottomcolor;

	if (mayforce)
	{
		local = CL_TryTrackNum(&cl.playerview[0]);
		if ((cl.teamplay || cls.protocol == CP_NETQUAKE) && !strcmp(player->team, cl.players[local].team))
		{
			if (cl_teamtopcolor != ~0)
				top = cl_teamtopcolor;
			if (cl_teambottomcolor != ~0)
				bottom = cl_teambottomcolor;

			if (player->colourised)
			{
				if (player->colourised->topcolour != ~0)
					top = player->colourised->topcolour;
				if (player->colourised->bottomcolour != ~0)
					bottom = player->colourised->bottomcolour;
			}
		}
		else
		{
			if (cl_enemytopcolor != ~0)
				top = cl_enemytopcolor;
			if (cl_enemybottomcolor != ~0)
				bottom = cl_enemybottomcolor;
		}
	}
/*
	if (top > 13 || top < 0)
		top = 13;
	if (bottom > 13 || bottom < 0)
		bottom = 13;
*/
	//other renderers still need the team stuff set, but that's all
	player->ttopcolor = top;
	player->tbottomcolor = bottom;
}
#endif

/*
==============
CL_UpdateUserinfo
==============
*/
static void CL_ProcessUserInfo (int slot, player_info_t *player)
{
	int i;
	char *col;
	int ospec = player->spectator;

	if (cls.protocol == CP_NETQUAKE)
		player->userid = slot;
	Q_strncpyz (player->name, InfoBuf_ValueForKey (&player->userinfo, "name"), sizeof(player->name));
	Q_strncpyz (player->team, InfoBuf_ValueForKey (&player->userinfo, "team"), sizeof(player->team));

	Ruleset_Check(InfoBuf_ValueForKey (&player->userinfo, RULESET_USERINFO), player->ruleset, sizeof(player->ruleset));

	col = InfoBuf_ValueForKey (&player->userinfo, "topcolor");
	if (!strncmp(col, "0x", 2))
		player->rtopcolor = 0xff000000|strtoul(col+2, NULL, 16);
	else
		player->rtopcolor = atoi(col);

	col = InfoBuf_ValueForKey (&player->userinfo, "bottomcolor");
	if (!strncmp(col, "0x", 2))
		player->rbottomcolor = 0xff000000|strtoul(col+2, NULL, 16);
	else
		player->rbottomcolor = atoi(col);

	i = atoi(InfoBuf_ValueForKey (&player->userinfo, "*spectator"));
	if (i == 2)
		player->spectator = 2;
	else if (i)
		player->spectator = true;
	else
		player->spectator = false;
/*
	if (player->rtopcolor > 13)
		player->rtopcolor = 13;
	if (player->rbottomcolor > 13)
		player->rbottomcolor = 13;
*/

	player->chatstate = atoi(InfoBuf_ValueForKey (&player->userinfo, "chat"));

#ifdef HEXEN2
	/*if we're running hexen2, they have to be some class...*/
	player->h2playerclass = atoi(InfoBuf_ValueForKey (&player->userinfo, "cl_playerclass"));
	if (player->h2playerclass > 5)
		player->h2playerclass = 5;
	if (player->h2playerclass < 1)
		player->h2playerclass = 1;
#endif

#ifdef QWSKINS
	player->model = NULL;
	player->colourised = TP_FindColours(player->name);
#endif

	// If it's us
	for (i = 0; i < cl.splitclients; i++)
		if (slot == cl.playerview[i].playernum)
			break;
	if (i < cl.splitclients && player->name[0])
	{
		if (cl.playerview[i].spectator != player->spectator)
		{
			cl.playerview[i].spectator = player->spectator;
			for (i = 0; i < cl.splitclients; i++)
			{
				Cam_Unlock(&cl.playerview[i]);
			}
			CL_CheckServerInfo();
		}
		// Update the rules since spectators can bypass everything but players can't
		else if (ospec != player->spectator)
			CL_CheckServerInfo();

		Skin_FlushPlayers();
	}
#ifdef QWSKINS
	else if (cl.teamplay && cl.playerview[0].spectator && slot == Cam_TrackNum(&cl.playerview[0]))	//skin forcing cares about the team of the guy we're tracking.
		Skin_FlushPlayers();
	else if (cls.state >= ca_onserver)
		Skin_Find (player);

	CL_NewTranslation (slot);
#endif
	Sbar_Changed ();

	CSQC_PlayerInfoChanged(slot);
}

/*
==============
CL_UpdateUserinfo
==============
*/
static void CL_UpdateUserinfo (void)
{
	unsigned int		slot;
	player_info_t	*player;

	slot = MSG_ReadPlayer();
	if (slot >= MAX_CLIENTS)
		Host_EndGame ("CL_ParseServerMessage: svc_updateuserinfo > MAX_SCOREBOARD");

	player = &cl.players[slot];
	player->userid = MSG_ReadLong ();
	InfoBuf_FromString(&player->userinfo, MSG_ReadString(), false);
	player->userinfovalid = true;

	CL_ProcessUserInfo (slot, player);



	if (slot == cl.playerview[0].playernum && player->name[0])
	{
		char *qz;
		qz = InfoBuf_ValueForKey(&player->userinfo, "Qizmo");
		if (*qz)
			TP_ExecTrigger("f_qizmoconnect", false);
	}
}

static void CL_ParseSetInfoBlob (void)
{
	unsigned int slot = MSG_ReadPlayer();
	char *key = MSG_ReadString();
	size_t keysize;
	unsigned int offset = MSG_ReadLong();
	qboolean final = !!(offset & 0x80000000);
	unsigned short valsize = MSG_ReadShort();
	char *val = BZ_Malloc(valsize);
	MSG_ReadData(val, valsize);
	offset &= ~0x80000000;
	key = InfoBuf_DecodeString(key, key+strlen(key), &keysize);

	if (slot-- == 0)
	{
		InfoBuf_SyncReceive(&cl.serverinfo, key, keysize, val, valsize, offset, final);
		if (final)
			CL_CheckServerInfo();
	}
	else if (slot >= MAX_CLIENTS)
		Con_Printf("INVALID SETINFO %i: %s=%s\n", slot, key, val);
	else
	{
		player_info_t *player = &cl.players[slot];
		if (offset)
			Con_DLPrintf(2,"SETINFO %s: %s+=%s\n", player->name, key, val);
		else
			Con_DLPrintf(strcmp(key, "chat")?1:2,"SETINFO %s: %s=%s\n", player->name, key, val);

		InfoBuf_SyncReceive(&player->userinfo, key, keysize, val, valsize, offset, final);
		player->userinfovalid = true;

		if (final)
			CL_ProcessUserInfo (slot, player);
	}

	Z_Free(key);
	Z_Free(val);
}
/*
==============
CL_SetInfo
==============
*/
static void CL_ParseSetInfo (void)
{
	unsigned int		slot;
	player_info_t	*player;
	char *val;
	char key[512];

	slot = MSG_ReadPlayer ();

	MSG_ReadStringBuffer(key, sizeof(key));
	val = MSG_ReadString();

	if (slot >= MAX_CLIENTS)
		Con_Printf("INVALID SETINFO %i: %s=%s\n", slot, key, val);
	else
	{
		player = &cl.players[slot];

		if (cl_shownet.value == 3)
			Con_Printf("\t%i(%s): %s=\"%s\"\n", slot, player->name, key, val);
		else
			Con_DLPrintf(strcmp(key, "chat")?1:2,"SETINFO %s: %s=%s\n", player->name, key, val);

		InfoBuf_SetStarKey(&player->userinfo, key, val);
		player->userinfovalid = true;

		CL_ProcessUserInfo (slot, player);
	}
}

/*
==============
CL_ServerInfo
==============
*/
static void CL_ServerInfo (void)
{
//	int		slot;
//	player_info_t	*player;
	char key[MAX_QWMSGLEN];
	char value[MAX_QWMSGLEN];

	Q_strncpyz (key, MSG_ReadString(), sizeof(key));
	Q_strncpyz (value, MSG_ReadString(), sizeof(value));

	if (cl_shownet.value == 3)
		Con_Printf("\t%s=%s\n", key, value);
	else
		Con_DPrintf("SERVERINFO: %s=%s\n", key, value);

	InfoBuf_SetStarKey(&cl.serverinfo, key, value);

	CL_CheckServerInfo();
}

/*
=====================
CL_SetStat
=====================
*/
static void CL_SetStat_Internal (int pnum, int stat, int ivalue, float fvalue)
{
	if (cl.playerview[pnum].stats[stat] != ivalue)
		Sbar_Changed ();

#ifdef QUAKESTATS
	if (stat == STAT_ITEMS)
	{	// set flash times
		int	j;
		for (j=0 ; j<32 ; j++)
			if ( (ivalue & (1<<j)) && !(cl.playerview[pnum].stats[stat] & (1<<j)))
				cl.playerview[pnum].item_gettime[j] = cl.time;
	}

	if (stat == STAT_WEAPONMODELI)
	{
		if (cl.playerview[pnum].stats[stat] != ivalue)
		{
			if (ivalue == 0)
				TP_ExecTrigger ("f_reloadstart", false);
			else if (cl.playerview[pnum].stats[stat] == 0)
				TP_ExecTrigger ("f_reloadend", false);
		}
	}

	if (stat == STAT_VIEWHEIGHT && ((cls.z_ext & Z_EXT_VIEWHEIGHT) || cls.protocol == CP_NETQUAKE))
		cl.playerview[pnum].viewheight = fvalue;
#endif

	cl.playerview[pnum].stats[stat] = ivalue;
	cl.playerview[pnum].statsf[stat] = fvalue;

#ifdef QUAKESTATS
	if (pnum == 0)
		TP_StatChanged(stat, ivalue);
#endif
}

#ifdef NQPROT
static void CL_SetStatMovevar(int pnum, int stat, int ivalue, float value)
{
	switch(stat)
	{
	case STAT_FRAGLIMIT:
		if (cls.protocol == CP_NETQUAKE && CPNQ_IS_DP)
			InfoBuf_SetKey(&cl.serverinfo, "fraglimit", va("%g", value));
		break;
	case STAT_TIMELIMIT:
		if (cls.protocol == CP_NETQUAKE && CPNQ_IS_DP)
			InfoBuf_SetKey(&cl.serverinfo, "timelimit", va("%g", value));
		break;
	case STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR:	//0
	case STAT_MOVEVARS_AIRCONTROL_PENALTY:			//0
	case STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW:			//0
	case STAT_MOVEVARS_AIRSTRAFEACCEL_QW:			//0
	case STAT_MOVEVARS_AIRCONTROL_POWER:			//2
	case STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL:	//0
	case STAT_MOVEVARS_WARSOWBUNNY_ACCEL:			//0
	case STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED:		//0
	case STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL:		//0
	case STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO:	//0
	case STAT_MOVEVARS_AIRSTOPACCELERATE:			//0
	case STAT_MOVEVARS_AIRSTRAFEACCELERATE:			//0
	case STAT_MOVEVARS_MAXAIRSTRAFESPEED:			//0
	case STAT_MOVEVARS_AIRCONTROL:					//0
	case STAT_MOVEVARS_WALLFRICTION:				//0
	case STAT_MOVEVARS_TIMESCALE:					//sv_gamespeed
	case STAT_MOVEVARS_JUMPVELOCITY:				//270
	case STAT_MOVEVARS_EDGEFRICTION:				//2
	case STAT_MOVEVARS_MAXAIRSPEED:					//30
	case STAT_MOVEVARS_AIRACCEL_QW:					//1
	case STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION:	//0
		break;

	case STAT_MOVEVARS_STEPHEIGHT:					//18
		movevars.stepheight = value;
		break;
	case STAT_MOVEVARS_TICRATE:		//cl_maxfps limiter hint
		if (cls.protocol == CP_NETQUAKE && CPNQ_IS_DP)
		{
			if (value <= 0)
				cls.maxfps = 1.0/value;
			else
				cls.maxfps = 72;
		}
		break;
	case STAT_MOVEFLAGS:
		movevars.flags = ivalue;
		break;
	case STAT_MOVEVARS_GRAVITY:
		movevars.gravity = value;
		break;
	case STAT_MOVEVARS_STOPSPEED:
		movevars.stopspeed = value;
		break;
	case STAT_MOVEVARS_MAXSPEED:
		cl.playerview[pnum].maxspeed = value;
		break;
	case STAT_MOVEVARS_SPECTATORMAXSPEED:
		movevars.spectatormaxspeed = value;
		break;
	case STAT_MOVEVARS_ACCELERATE:
		movevars.accelerate = value;
		break;
	case STAT_MOVEVARS_AIRACCELERATE:
		movevars.airaccelerate = value;
		break;
	case STAT_MOVEVARS_WATERACCELERATE:
		movevars.wateraccelerate = value;
		break;
	case STAT_MOVEVARS_FRICTION:
		movevars.friction = value;
		break;
	case STAT_MOVEVARS_WATERFRICTION:
		movevars.waterfriction = value;
		break;
	case STAT_MOVEVARS_ENTGRAVITY:
		cl.playerview[pnum].entgravity = value;
		break;
	}
}
#endif

//the two values are expected to be the same, they're just both provided for precision.
static void CL_SetStatNumeric (int pnum, unsigned int stat, int ivalue, float fvalue)
{
	if (stat < 0 || stat >= MAX_CL_STATS)
		return;
//		Host_EndGame ("CL_SetStat: %i is invalid", stat);

#ifdef QUAKESTATS
	if (stat == STAT_TIME && (cls.fteprotocolextensions & PEXT_ACCURATETIMINGS))
	{
		cl.oldgametime = cl.gametime;
		cl.oldgametimemark = cl.gametimemark;

		cl.gametime = fvalue * 0.001;
		cl.gametimemark = realtime;
	}
#endif

	if (cls.demoplayback == DPB_MVD)
	{
		extern int cls_lastto;
		cl.players[cls_lastto].stats[stat]=ivalue;
		cl.players[cls_lastto].statsf[stat]=fvalue;

		if (cl_shownet.value == 3)
			Con_Printf("\t%i: %i=%g\n", cls_lastto, stat, fvalue);

		for (pnum = 0; pnum < cl.splitclients; pnum++)
			if (cl.playerview[pnum].cam_spec_track == cls_lastto && cl.playerview[pnum].cam_state != CAM_FREECAM)
				CL_SetStat_Internal(pnum, stat, ivalue, fvalue);
	}
	else
	{
		unsigned int pl = cl.playerview[pnum].playernum;
		if (pl < MAX_CLIENTS)
		{
			cl.players[pl].stats[stat]=ivalue;
			cl.players[pl].statsf[stat]=fvalue;
		}

		if (cl_shownet.value == 3)
			Con_Printf("\t%i(%i): %i=%g\n", pnum, pl, stat, fvalue);

		CL_SetStat_Internal(pnum, stat, ivalue, fvalue);
	}

#ifdef QUAKESTATS
	if (stat == STAT_VIEWHEIGHT && ((cls.z_ext & Z_EXT_VIEWHEIGHT) || cls.protocol == CP_NETQUAKE))
		cl.playerview[pnum].viewheight = fvalue;
#endif

#ifdef NQPROT
	if (cls.protocol == CP_NETQUAKE && (CPNQ_IS_DP || (cls.fteprotocolextensions2 & PEXT2_PREDINFO)))
	{
		if (cls.fteprotocolextensions2 & PEXT2_PREDINFO)
			CL_SetStatMovevar(pnum, stat, ivalue, fvalue);
		else
			CL_SetStatMovevar(pnum, stat, ivalue, *(float*)&ivalue);	//DP sucks.
	}
#endif
}

static void CL_SetStatString (int pnum, int stat, const char *value)
{
	if (stat < 0 || stat >= MAX_CL_STATS)
		return;
//		Host_EndGame ("CL_SetStat: %i is invalid", stat);

	if (cls.demoplayback == DPB_MVD)
	{
		extern int cls_lastto;
		//Z_Free(cl.players[cls_lastto].statsstr[stat]);
		//cl.players[cls_lastto].statsstr[stat]=Z_StrDup(value);

		for (pnum = 0; pnum < cl.splitclients; pnum++)
			if (cl.playerview[pnum].cam_spec_track == cls_lastto && cl.playerview[pnum].cam_state != CAM_FREECAM)
			{
				if (cl.playerview[pnum].statsstr[stat])
					Z_Free(cl.playerview[pnum].statsstr[stat]);
				cl.playerview[pnum].statsstr[stat] = Z_StrDup(value);
			}
	}
	else
	{
		if (cl.playerview[pnum].statsstr[stat])
			Z_Free(cl.playerview[pnum].statsstr[stat]);
		cl.playerview[pnum].statsstr[stat] = Z_StrDup(value);
	}
}

/*
//if we're going to 'spend' another byte for longer indexes, we might as well spend an extra 4 bits on the type too, allowing for 64bit types etc.
static void CL_ParseExtendedStat(int destsplit)
{
//float/double/sint/uint
//string
	quint64_t id = MSG_ReadUInt64();
	unsigned int type;
	type = id&0xf;
	id>>=4;	//we're never going to have that many stats.
	switch(type)
	{
	case ev_void:	//might as well.
		CL_SetStatNumeric(destsplit, id, 0, 0);
		break;
	case ev_string:
		CL_SetStatString(destsplit, id, MSG_ReadString());
		break;
	case ev_float:
		{
			float f = MSG_ReadFloat();
			CL_SetStatNumeric(destsplit, id, f, f);
		}
		break;
	case ev_vector:
		{
			float f;
			f = MSG_ReadFloat();CL_SetStatNumeric(destsplit, id+0, f, f);
			f = MSG_ReadFloat();CL_SetStatNumeric(destsplit, id+1, f, f);
			f = MSG_ReadFloat();CL_SetStatNumeric(destsplit, id+2, f, f);
		}
		break;
	case ev_entity:
		{
			unsigned int i = MSGCL_ReadEntity();
			CL_SetStatNumeric(destsplit, id, i, i);
		}
		break;
//	case ev_field:
//	case ev_function:
//	case ev_pointer:
	case ev_integer:
		{
			signed int i = MSG_ReadLong();
			CL_SetStatNumeric(destsplit, id, i, i);
		}
		break;
	case ev_uint:
		{
			unsigned int i = MSG_ReadLong();
			CL_SetStatNumeric(destsplit, id, i, i);
		}
		break;
	case ev_int64:
		{
			qint64_t i = MSG_ReadInt64();
			CL_SetStatNumeric(destsplit, id, i, i);
		}
		break;
	case ev_uint64:
		{
			quint64_t i = MSG_ReadUInt64();
			CL_SetStatNumeric(destsplit, id, i, i);
		}
		break;
	case ev_double:
		{
			double f = MSG_ReadDouble();
			CL_SetStatNumeric(destsplit, id, f, f);
		}
		break;
	default:
		Host_EndGame("CL_ParseExtendedStat: type %i is unsupported", type);
		break;
	}
}*/

/*
==============
CL_MuzzleFlash
==============
*/
static void CL_MuzzleFlash (int entnum)
{
	dlight_t	*dl;
	player_state_t	*pl;

	packet_entities_t *pack;
	entity_state_t *s1;
	int pnum;
	vec3_t org = {0,0,0};
	vec3_t axis[3] = {{0,0,0}};
	int dlightkey = 0;
	extern int pt_muzzleflash;
	extern cvar_t cl_muzzleflash;

	//was it us?
	if (!cl_muzzleflash.ival) // remove all muzzleflashes
		return;

	if (cl_muzzleflash.value == 2)
	{
		//muzzleflash 2 removes muzzleflashes on us
		for (pnum = 0; pnum < cl.splitclients; pnum++)
			if (entnum-1 == cl.playerview[pnum].playernum)
				return;
	}

	if (!dlightkey)
	{
		pack = &cl.inframes[cl.validsequence&UPDATE_MASK].packet_entities;

		for (pnum=0 ; pnum<pack->num_entities ; pnum++)	//try looking for an entity with that id first
		{
			s1 = &pack->entities[pnum];

			if (s1->number == entnum)
			{
				dlightkey = entnum;
				VectorCopy(s1->origin, org);
				AngleVectors(s1->angles, axis[0], axis[1], axis[2]);
				break;
			}
		}
	}
	if (!dlightkey)
	{	//that ent number doesn't exist, go for a player with that number
		if ((unsigned)(entnum) <= cl.allocated_client_slots && entnum > 0)
		{
			pl = &cl.inframes[cl.validsequence&UPDATE_MASK].playerstate[entnum-1];

			if (pl->messagenum == cl.validsequence)
			{
				dlightkey = -entnum;
				VectorCopy(pl->origin, org);
				AngleVectors(pl->viewangles, axis[0], axis[1], axis[2]);
				if (pl->szmins[2] == 0)	/*hull is 0-based, so origin is bottom of model, move the light up slightly*/
					org[2] += pl->szmaxs[2]/2;
			}
		}
	}

	if (!dlightkey)
		return;

	if (P_RunParticleEffectType(org, axis[0], 1, pt_muzzleflash))
	{
		extern cvar_t r_muzzleflash_colour;
		extern cvar_t r_muzzleflash_fade;

		dl = CL_AllocDlight (dlightkey);
		VectorMA (org, 15, axis[0], dl->origin);
		memcpy(dl->axis, axis, sizeof(dl->axis));

		dl->minlight = 32;
		dl->die = cl.time + 0.1;

		VectorCopy(r_muzzleflash_colour.vec4, dl->color);
		dl->radius = r_muzzleflash_colour.vec4[3] + (rand()&31);
		VectorCopy(r_muzzleflash_fade.vec4, dl->channelfade);
		dl->decay = r_muzzleflash_fade.vec4[3];
#ifdef RTLIGHTS
		dl->lightcolourscales[2] = 4;
#endif
	}
}

//return if we want to print the message.
static char *CL_ParseChat(char *text, player_info_t **player, int *msgflags)
{
	extern cvar_t cl_chatsound, cl_nofake, cl_teamchatsound, cl_enemychatsound;
	int flags;
	int offset=0;
	qboolean	suppress_talksound;
	char *p;
	char *s;
	int check_flood;

	flags = TP_CategorizeMessage (text, &offset, player);
	*msgflags = flags;

	s = text + offset;

	if (flags)
	{
		if (!cls.demoplayback)
			Sys_ServerActivity();	//chat always flashes the screen..

		if (*player && Ignore_Message((*player)->name, s, flags))
			return NULL;

		//check f_ stuff
		if (*player && (!strncmp(s, "f_", 2)|| !strncmp(s, "q_", 2)))
		{
			Validation_Auto_Response(*player - cl.players, s);
			return s;
		}

		Validation_CheckIfResponse(text);

#ifdef PLUGINS
		if (!Plug_ChatMessage(text + offset, *player ? (int)(*player - cl.players) : -1, flags))
			return NULL;
#endif

		if (flags & (TPM_TEAM|TPM_OBSERVEDTEAM) && !TP_FilterMessage(text + offset))
			return NULL;

#ifdef QUAKEHUD
		if (flags & (TPM_TEAM|TPM_OBSERVEDTEAM) && Sbar_UpdateTeamStatus(*player, text+offset))
			return NULL;
#endif


		if ((int)msg_filter.value & flags)
			return NULL;	//filter chat

		check_flood = Ignore_Check_Flood(*player, s, flags);
		if (check_flood == IGNORE_NO_ADD)
			return NULL;
		else if (check_flood == NO_IGNORE_ADD)
			Ignore_Flood_Add(*player, s);
	}
#ifdef PLUGINS
	else
	{
		if (!Plug_ServerMessage(text + offset, PRINT_CHAT))
			return NULL;
	}
#endif

	suppress_talksound = false;

	if (flags == 2 || (!cl.teamplay && flags))
		suppress_talksound = TP_CheckSoundTrigger (text + offset);

	if (cls.demoseeking ||
		!cl_chatsound.value ||		// no sound at all
		(cl_chatsound.value == 2 && flags != 2))	// only play sound in mm2
		suppress_talksound = true;


	if (!suppress_talksound)
	{
		if (flags & (TPM_OBSERVEDTEAM|TPM_TEAM) && cl.teamplay)
			S_LocalSound (cl_teamchatsound.string);
		else
			S_LocalSound (cl_enemychatsound.string);
	}

	if (flags)
	{
		if (cl_nofake.value == 1 || (cl_nofake.value == 2 && !(flags & (TPM_OBSERVEDTEAM | TPM_TEAM))))
		{
			for (p = s; *p; p++)
				if (*p == 13 || (*p == 10 && p[1]))
					*p = ' ';
		}
	}

	return s;
}

// CL_PlayerColor: returns color and mask for player_info_t
static int CL_PlayerColor(player_info_t *plr, qboolean *name_coloured)
{
	char *t;
	unsigned int c;

	*name_coloured = false;

	if (cl.teamfortress) //override based on team
	{
		//damn spies
		if (!Q_strcasecmp(plr->team, "red"))
			c = 1;
		else if (!Q_strcasecmp(plr->team, "blue"))
			c = 5;
		else
		// TODO: needs some work
		switch (plr->rbottomcolor)
		{	//translate q1 skin colours to console colours
		case 10:
		case 1:
			*name_coloured = true;
		case 4:	//red
			c = 1;
			break;
		case 11:
			*name_coloured = true;
		case 3: // green
			c = 2;
			break;
		case 5:
			*name_coloured = true;
		case 12:
			c = 3;
			break;
		case 6:
		case 7:
			*name_coloured = true;
		case 8:
		case 9:
			c = 6;
			break;
		case 2: // light blue
			*name_coloured = true;
		case 13: //blue
		case 14: //blue
			c = 5;
			break;
		default:
			*name_coloured = true;
		case 0: // white
			c = 7;
			break;
		}
	}
	else if (cl.teamplay)
	{
		// team name hacks
		if (!strcmp(plr->team, "red"))
			c = 1;
		else if (!strcmp(plr->team, "blue"))
			c = 5;
		else
		{
			char *t;

			t = plr->team;
			c = 0;

			for (t = plr->team; *t; t++)
			{
				c >>= 1;
				c ^= *t; // TODO: very weak hash, replace
			}

			if ((c / 7) & 1)
				*name_coloured = true;

			c = 1 + (c % 7);
		}
	}
	else
	{
		// override chat color with tc infokey
		// 0-6 is standard colors (red to white)
		// 7-13 is using secondard charactermask
		// 14 and afterwards repeats
		t = InfoBuf_ValueForKey(&plr->userinfo, "tc");
		if (*t)
			c = atoi(t);
		else
			c = plr->userid; // Quake2 can start from 0

		if ((c / 7) & 1)
			*name_coloured = true;

		c = 1 + (c % 7);
	}

	return c;
}

void TTS_SayChatString(char **stringtosay);

// CL_PrintChat: takes chat strings and performs name coloring and cl_parsewhitetext parsing
// NOTE: text in rawmsg/msg is assumed destroyable and should not be used afterwards
void CL_PrintChat(player_info_t *plr, char *msg, int plrflags)
{
	extern cvar_t con_separatechat;
	char *name = NULL;
	int c;
	qboolean name_coloured = false;
	extern cvar_t cl_parsewhitetext;
	qboolean memessage = false;
	char fullchatmessage[2048];

	fullchatmessage[0] = 0;
	/*if (plrflags & TPM_FAKED)
	{
		name = rawmsg; // use rawmsg pointer and msg modification to generate null-terminated string
		if (msg)
			*(msg - 2) = 0; // it's assumed that msg has 2 chars before it due to strstr
	}*/

	if (0)//*msg == '\r')
	{
		name = msg;
		msg = strstr(msg, ": ");
		if (msg)
		{
			name++;
			*msg = 0;
			msg+=2;
			plrflags &= ~TPM_TEAM|TPM_OBSERVEDTEAM;
		}
		else
		{
			msg = name;
			name = NULL;
		}
	}

	if (msg[0] == '/' && msg[1] == 'm' && msg[2] == 'e' && msg[3] == ' ')
	{
		msg += 4;
		memessage = true; // special /me formatting
	}

	if (plr && !name) // use special formatting with a real chat message
		name = plr->name; // use player's name

	if (cl_standardchat.ival)
	{
		name_coloured = true;
		c = 7;
	}
	else
	{
		if (plrflags & TPM_SPECTATOR) // is an observer
		{
			// TODO: we don't even check for this yet...
			if (plrflags & (TPM_TEAM | TPM_OBSERVEDTEAM)) // is on team
				c = 0; // blacken () on observers
			else
			{
				name_coloured = true;
				c = 7;
			}
		}
		else if (plr)
			c = CL_PlayerColor(plr, &name_coloured);
		else
		{
			// defaults for fake clients
			name_coloured = true;
			c = 7;
		}
	}

	c = '0' + c;

	if (plrflags & TPM_QTV)
		Q_strncatz(fullchatmessage, "QTV ^m", sizeof(fullchatmessage));
	else if (name)
	{
		if (memessage)
		{
			if (!cl_standardchat.value && (plrflags & TPM_SPECTATOR))
				Q_strncatz(fullchatmessage, "^0*^7 ", sizeof(fullchatmessage));
			else
				Q_strncatz(fullchatmessage, "* ", sizeof(fullchatmessage));
		}
		else
			Q_strncatz(fullchatmessage, "\1", sizeof(fullchatmessage));

#if defined(HAVE_SPEECHTOTEXT)
		TTS_SayChatString(&msg);
#endif

		if (plrflags & (TPM_TEAM|TPM_OBSERVEDTEAM)) // for team chat don't highlight the name, just the brackets
		{
			Q_strncatz(fullchatmessage, va("(^[^7%s%s^d\\player\\%i^])", name_coloured?"^m":"", name, (int)(plr-cl.players)), sizeof(fullchatmessage));
		}
		else if (cl_standardchat.ival)
		{
			Q_strncatz(fullchatmessage, va("^[^7%s%s^d\\player\\%i^]", name_coloured?"^m":"", name, (int)(plr-cl.players)), sizeof(fullchatmessage));
		}
		else
		{
			Q_strncatz(fullchatmessage, va("^[^7%s^%c%s^d\\player\\%i^]", name_coloured?"^m":"", c, name, (int)(plr-cl.players)), sizeof(fullchatmessage));
		}

		if (!memessage)
		{
			// only print seperator with an actual player name
			if (!cl_standardchat.value && (plrflags & TPM_SPECTATOR))
				Q_strncatz(fullchatmessage, "^0: ^d", sizeof(fullchatmessage));
			else
				Q_strncatz(fullchatmessage, ": ", sizeof(fullchatmessage));
		}
		else
			Q_strncatz(fullchatmessage, " ", sizeof(fullchatmessage));
	}
	else
		Q_strncatz(fullchatmessage, "\1", sizeof(fullchatmessage));

	// print message
	if (cl_parsewhitetext.value && (cl_parsewhitetext.value == 1 || (plrflags & (TPM_TEAM|TPM_OBSERVEDTEAM))))
	{
		char *t, *u;

		while ((t = strchr(msg, '{')))
		{
			int c;
			if (t > msg && t[-1] == '^')
			{
				for (c = 1; t-c > msg; c++)
				{
					if (t[-c] == '^')
						break;
				}
				if (c & 1)
				{
					*t = '\0';
					Q_strncatz(fullchatmessage, va("%s{", msg), sizeof(fullchatmessage));
					msg = t+1;
					continue;
				}
			}
			u = strchr(t, '}');
			if (u)
			{
				*t = 0;
				*u = 0;
				Q_strncatz(fullchatmessage, va("%s", msg), sizeof(fullchatmessage));
				Q_strncatz(fullchatmessage, va("^m%s^m", t+1), sizeof(fullchatmessage));
				msg = u+1;
			}
			else
				break;
		}
		Q_strncatz(fullchatmessage, va("%s", msg), sizeof(fullchatmessage));
	}
	else
	{
		Q_strncatz(fullchatmessage, va("%s", msg), sizeof(fullchatmessage));
	}

#ifdef CSQC_DAT
	if (CSQC_ParsePrint(fullchatmessage, PRINT_CHAT))
		return;
#endif


	if (con_separatechat.ival)
	{
		if (!con_chat)
			con_chat = Con_Create("chat", CONF_HIDDEN|CONF_NOTIFY|CONF_NOTIFY_BOTTOM);
		if (con_chat)
		{
			Con_PrintCon(con_chat, fullchatmessage, con_chat->parseflags);

			if (con_separatechat.ival == 1)
			{
				console_t *c = Con_GetMain();
				Con_PrintCon(c, fullchatmessage, c->parseflags|PFS_NONOTIFY);
				return;
			}
		}
	}

	Con_Printf("%s", fullchatmessage);
}

// CL_PrintStandardMessage: takes non-chat net messages and performs name coloring
// NOTE: msg is considered destroyable
static char acceptedchars[] = {'.', '?', '!', '\'', ',', ':', ' ', '\0'};
static void CL_PrintStandardMessage(char *msgtext, int printlevel)
{
	int i;
	player_info_t *p, *foundp = NULL;
	extern cvar_t cl_standardmsg, msg;
	char *begin = msgtext;
	char fullmessage[2048];

	char *found;

	if (printlevel < msg.ival)
		return;

	fullmessage[0] = 0;

	while(*msgtext)
	{
		found = NULL;
		// search for player names in message
		for (i = 0, p = cl.players; i < cl.allocated_client_slots; p++, i++)
		{
			char *v;
			char *name;
			int len;

			name = p->name;
			if (!(*name))
				continue;
			len = strlen(name);
			v = strstr(msgtext, name);
			while (v)
			{
				// name parsing rules
				if (v != begin && *(v-1) != ' ') // must be space before name
				{
						v = strstr(v+len, name);
						continue;
				}

				{
					int i;
					char aftername = *(v + len);

					// search for accepted chars in char after name in msg
					for (i = 0; i < sizeof(acceptedchars); i++)
					{
						if (acceptedchars[i] == aftername)
							break;
					}

					if (sizeof(acceptedchars) == i)
					{
						v = strstr(v+len, name);
						continue; // no accepted char found
					}
				}

				if (!found || v < found)
				{
					found = v;
					foundp = p;
				}
				break;
			}
		}

		if (found)
		{
			qboolean coloured;
			char c;
			int len = strlen(foundp->name);

			// print msg chunk
			*found = 0; // cut off message
			Q_strncatz(fullmessage, msgtext, sizeof(fullmessage));
			msgtext = found + len; // update search point

			// get name color
			if (foundp->spectator || cl_standardmsg.ival)
			{
				coloured = false;
				c = '7';
			}
			else
				c = '0' + CL_PlayerColor(foundp, &coloured);

			// print name
			Q_strncatz(fullmessage, va("^[%s^%c%s^d\\player\\%i^]", coloured?"^m":"", c, foundp->name, (int)(foundp - cl.players)), sizeof(fullmessage));
		}
		else
			break; //nope, can't find anyone in there...
	}

	// print final chunk
	Q_strncatz(fullmessage, msgtext, sizeof(fullmessage));
#ifdef HAVE_LEGACY
	if (scr_usekfont.ival)
		Con_PrintFlags(fullmessage, PFS_FORCEUTF8, 0);
	else
#endif
		Con_Printf("%s", fullmessage);
}

static char printtext[4096];
static void CL_ParsePrint(const char *msg, int level)
{
	char n, *e;
	if (strlen(printtext) + strlen(msg)+2 >= sizeof(printtext))
	{
		Con_Printf("%s", printtext);
		Q_strncpyz(printtext, msg, sizeof(printtext));
	}
	else
		strcat(printtext, msg);	//safe due to size on if.
#ifdef HAVE_LEGACY	//eztv is fucking nasty.
	if (level == PRINT_CHAT && *printtext == '#' && printtext[1] >= '0' && printtext[1] <= '9' && !strchr(printtext, '\n'))
		strcat(printtext, "\n");
#endif
	while((e = strchr(printtext, '\n')) || (e = strchr(printtext, '\r')))
	{
		n = e[1];
		e[1] = 0;

		if (!cls.demoseeking)
		{
			if (level == PRINT_CHAT)
			{
				char *body;
				int msgflags;
				player_info_t *plr = NULL;

				if (!TP_SuppressMessage(printtext))
				{
					body = CL_ParseChat(printtext, &plr, &msgflags);
					if (body)
						CL_PrintChat(plr, body, msgflags);
				}
			}
			else
			{
#ifdef CSQC_DAT
				if (!CSQC_ParsePrint(printtext, level))
#endif
#ifdef PLUGINS
				if (Plug_ServerMessage(printtext, level))
#endif
#ifdef QUAKEHUD
					if (!Stats_ParsePickups(printtext) || !msg_filter_pickups.ival)
						if (!Stats_ParsePrintLine(printtext) || !msg_filter_frags.ival)
#else
					if (!msg_filter_pickups.ival)
						if (!msg_filter_frags.ival)
#endif
							CL_PrintStandardMessage(printtext, level);
			}
		}

		TP_SearchForMsgTriggers(printtext, level);
		e[1] = n;
		e++;

		memmove(printtext, e, strlen(e)+1);
	}
}

static void CL_ParseWeaponStats(void)
{
#ifdef QUAKEHUD
	int pl = atoi(Cmd_Argv(0));
	char *wname = Cmd_Argv(1);
	unsigned int total = strtoul(Cmd_Argv(2), NULL, 0);
	unsigned int hit = strtoul(Cmd_Argv(3), NULL, 0);
	unsigned int idx;

	if (pl >= cl.allocated_client_slots)
		return;

	for (idx = 0; idx < countof(cl.players[pl].weaponstats); idx++)
	{
		if (!strcmp(cl.players[pl].weaponstats[idx].wname, wname) || !*cl.players[pl].weaponstats[idx].wname)
		{
			Q_strncpyz(cl.players[pl].weaponstats[idx].wname, wname, sizeof(cl.players[pl].weaponstats[idx].wname));
			cl.players[pl].weaponstats[idx].total = total;
			cl.players[pl].weaponstats[idx].hit = hit;
			return;
		}
	}
#endif
}

static void CL_ParseItemTimer(void)
{
	//it [cur/]duration x y z radius 0xRRGGBB "timername" owningent
	float timeout;// = atof(Cmd_Argv(0));
	vec3_t org = {	atof(Cmd_Argv(1)),
					atof(Cmd_Argv(2)),
					atof(Cmd_Argv(3))};
	float radius =	atof(Cmd_Argv(4));
	unsigned int rgb = (Cmd_Argc() > 5)?strtoul(Cmd_Argv(5), NULL, 16):0x202020;
//	char *timername =	Cmd_Argv(6);
	unsigned int entnum = strtoul(Cmd_Argv(7), NULL, 0);
	struct itemtimer_s *timer;
	float start = cl.time;
	char *e;
	timeout = strtod(Cmd_Argv(0), &e);
	if (*e == '/')
	{
		start += timeout;
		timeout = atof(e+1);
		start -= timeout;
	}

	if (!timeout)
		timeout = FLT_MAX;
	if (!radius)
		radius = 32;

	for (timer = cl.itemtimers; timer; timer = timer->next)
	{
		if (entnum)
		{
			if (timer->entnum == entnum)
				break;
		}
		else if (VectorCompare(timer->origin, org))
			break;
	}
	if (!timer)
	{	//didn't find it.
		timer = Z_Malloc(sizeof(*timer));
		timer->next = cl.itemtimers;
		cl.itemtimers = timer;
	}

	VectorCopy(org, timer->origin);
	timer->radius = radius;
	timer->duration = timeout;
	timer->entnum = entnum;
	timer->start = start;
	timer->end = start + timer->duration;
	timer->rgb[0] = ((rgb>>16)&0xff)/255.0;
	timer->rgb[1] = ((rgb>> 8)&0xff)/255.0;
	timer->rgb[2] = ((rgb    )&0xff)/255.0;
}

#ifdef PLUGINS
static void CL_ParseTeamInfo(void)
{
	unsigned int pidx = atoi(Cmd_Argv(1));
	vec3_t org =
	{
		atof(Cmd_Argv(2)),
		atof(Cmd_Argv(3)),
		atof(Cmd_Argv(4))
	};
	float health = atof(Cmd_Argv(5));
	float armour = atof(Cmd_Argv(6));
	unsigned int items = strtoul(Cmd_Argv(7), NULL, 0);
	char *nick = Cmd_Argv(8);

	if (pidx < cl.allocated_client_slots)
	{
		player_info_t *pl = &cl.players[pidx];
		pl->tinfo.time = cl.time+5;
		pl->tinfo.health = health;
		pl->tinfo.armour = armour;
		pl->tinfo.items = items;
		VectorCopy(org, pl->tinfo.org);
		Q_strncpyz(pl->tinfo.nick, nick, sizeof(pl->tinfo.nick));
	}
}
#endif


static char stufftext[4096];
static void CL_ParseStuffCmd(char *msg, int destsplit)	//this protects stuffcmds from network segregation.
{
	int cbuflevel;
#ifdef NQPROT
	if (!*stufftext && *msg == 1)
	{
		if (developer.ival)
		{
			Con_DPrintf("Proquake Message:\n");
			Con_HexDump(msg, strlen(msg), 1, 16);
		}
		msg = CLNQ_ParseProQuakeMessage(msg);
	}
#endif

	Q_strncatz(stufftext, msg, sizeof(stufftext)-1);
	while((msg = strchr(stufftext, '\n')))
	{
		*msg = '\0';
		cbuflevel = RESTRICT_SERVERSEAT(destsplit);
		Con_DLPrintf((cls.state==ca_active)?1:2, "stufftext%i: %s\n", destsplit, stufftext);
		if (!strncmp(stufftext, "fullserverinfo ", 15) || !strncmp(stufftext, "//fullserverinfo ", 17))
		{
			Cmd_TokenizeString(stufftext+2, false, false);
			if (Cmd_Argc() == 2)
			{
				cl.haveserverinfo = true;
				InfoBuf_FromString(&cl.serverinfo, Cmd_Argv(1), false);
				CL_CheckServerInfo();
			}

			#if _MSC_VER > 1200
			if (cls.netchan.remote_address.type != NA_LOOPBACK)
				Sys_RecentServer("+connect", cls.servername, va("%s (%s)", InfoBuf_ValueForKey(&cl.serverinfo, "hostname"), cls.servername), "Join QW Server");
			#endif
		}
		else if (!strncmp(stufftext, "//svi ", 6))	//for serverinfo over NQ protocols
		{
			Cmd_TokenizeString(stufftext+2, false, false);
			Con_DPrintf("SERVERINFO: %s=%s\n", Cmd_Argv(1), Cmd_Argv(2));
			InfoBuf_SetStarKey(&cl.serverinfo, Cmd_Argv(1), Cmd_Argv(2));
			CL_CheckServerInfo();
		}
		else if (!strncmp(stufftext, "//ls ", 5))	//for extended lightstyles
		{
			vec3_t rgb;
			Cmd_TokenizeString(stufftext+2, false, false);
			rgb[0] = ((Cmd_Argc()>3)?atof(Cmd_Argv(3)):1);
			rgb[1] = ((Cmd_Argc()>5)?atof(Cmd_Argv(4)):rgb[0]);
			rgb[2] = ((Cmd_Argc()>5)?atof(Cmd_Argv(5)):rgb[0]);
			R_UpdateLightStyle(atoi(Cmd_Argv(1)), Cmd_Argv(2), rgb[0], rgb[1], rgb[2]);
		}

#ifdef NQPROT
		//DP's download protocol
		else if (cls.protocol == CP_NETQUAKE && !strncmp(stufftext, "cl_serverextension_download ", 28))	//<supported>. server lets us know that it supports it.
			cl_dp_serverextension_download = true;	//warning, this is sent BEFORE svc_serverdata, so cannot use cl.foo
		else if (cls.protocol == CP_NETQUAKE && !strncmp(stufftext, "cl_downloadbegin ", 17))		//<size> <name>.  server [reliably] lets us know that its going to start sending data.
			CLDP_ParseDownloadBegin(stufftext);
		else if (cls.protocol == CP_NETQUAKE && !strncmp(stufftext, "cl_downloadfinished ", 20))		//<size> <crc>. server [reliably] lets us know that we acked the entire thing
			CLDP_ParseDownloadFinished(stufftext);
		else if (cls.protocol == CP_NETQUAKE && !strcmp(stufftext, "stopdownload"))						//download command reported failure. safe to request the next.
		{
			if (cls.download)
				CL_DownloadFailed(cls.download->remotename, cls.download, DLFAIL_CORRUPTED);
		}

		//DP servers use these to report the correct csprogs.dat file+version to use.
		//WARNING: these are sent BEFORE svc_serverdata, so we cannot store this state into cl.foo
		//we poke the data into cl.serverinfo once we get the following svc_serverdata.
		//we then clobber it from a fullserverinfo message if its an fte server running dpp7, but hey.
		else if (cls.protocol == CP_NETQUAKE && !strncmp(stufftext, "csqc_progname ", 14))
			COM_ParseOut(stufftext+14, cl_dp_csqc_progsname, sizeof(cl_dp_csqc_progsname));
		else if (cls.protocol == CP_NETQUAKE && !strncmp(stufftext, "csqc_progsize ", 14))
			cl_dp_csqc_progssize = atoi(stufftext+14);
		else if (cls.protocol == CP_NETQUAKE && !strncmp(stufftext, "csqc_progcrc ", 13))
			cl_dp_csqc_progscrc = atoi(stufftext+13);

		//NQ servers/mods like spamming this. Its annoying, but we might as well use it if we can, while also muting it.
		else if (!strncmp(stufftext, "cl_fullpitch ", 13) || !strncmp(stufftext, "pq_fullpitch ", 13))
		{
			if (!cl.haveserverinfo)
			{
				InfoBuf_SetKey(&cl.serverinfo, "maxpitch", (atoi(stufftext+13))? "90":"");
				InfoBuf_SetKey(&cl.serverinfo, "minpitch", (atoi(stufftext+13))?"-90":"");
				CL_CheckServerInfo();
			}
		}
#endif

		else if (!strncmp(stufftext, "//paknames ", 11))	//so that the client knows what to download...
		{													//there's a couple of prefixes involved etc
			Z_StrCat(&cl.serverpacknames, stufftext+(cl.serverpackhashes?11:10));
			cl.serverpakschanged = true;
		}
		else if (!strncmp(stufftext, "//paks ", 7))			//gives the client a list of hashes to match against
		{													//the client can re-order for cl_pure support, or download dupes to avoid version mismatches
			Z_StrCat(&cl.serverpackhashes, stufftext+(cl.serverpackhashes?7:6));
			cl.serverpakschanged = true;
			CL_CheckServerPacks();
		}
#ifdef HAVE_LEGACY
		else if (!strncmp(stufftext, "//vwep ", 7))			//list of vwep model indexes, because using the normal model precaches wasn't cool enough
		{													//(from zquake/ezquake)
			int i;
			char *mname;
			Cmd_TokenizeString(stufftext+7, false, false);
			for (i = 0; i < Cmd_Argc(); i++)
			{
				mname = Cmd_Argv(i);
				if (strcmp(mname, "-"))
				{
					mname = va("progs/%s.mdl", Cmd_Argv(i));
					Z_StrDupPtr(&cl.model_name_vwep[i], mname);
					if (cls.state == ca_active)
					{
						CL_CheckOrEnqueDownloadFile(cl.model_name_vwep[i], NULL, 0);
						cl.model_precache_vwep[i] = Mod_ForName(cl.model_name_vwep[i], MLV_WARN);
					}
				}
				else
				{
					Z_Free(cl.model_name_vwep[i]);
					cl.model_name_vwep[i] = NULL;
				}
			}
		}
#endif
		else if (cls.demoplayback && !strncmp(stufftext, "playdemo ", 9))
		{	//some demos (like speed-demos-archive's marathon runs) chain multiple demos with playdemo commands
			//these should still chain properly even when the demo is in some archive(like .dz) or subdir
			char newdemo[MAX_OSPATH], temp[MAX_OSPATH], *s;
			Cmd_TokenizeString(stufftext, false, false);
			s = Cmd_Argv(1);
			if (strchr(s, ':') || strchr(s, '/') || strchr(s, '\\'))
				Q_strncpyz(newdemo, s, sizeof(newdemo));
			else
			{
				newdemo[0] = 0;
				if (cls.lastdemowassystempath)
					Q_strncatz(newdemo, "#", sizeof(newdemo));
				Q_strncatz(newdemo, cls.lastdemoname, sizeof(newdemo));
				*COM_SkipPath(newdemo) = 0;
				Q_strncatz(newdemo, Cmd_Argv(1), sizeof(newdemo));
			}

			Cbuf_AddText ("playdemo ", cbuflevel);
			Cbuf_AddText (COM_QuotedString(newdemo, temp, sizeof(temp), false), cbuflevel);
			Cbuf_AddText ("\n", cbuflevel);
		}
#ifdef CSQC_DAT
		else if (CSQC_StuffCmd(destsplit, stufftext, msg))
		{
		}
#endif
		else if (!strncmp(stufftext, "//querycmd ", 11))	//for servers to check if a command exists or not.
		{
			COM_Parse(stufftext + 11);
			if (Cmd_Exists(com_token))
			{
				Cbuf_AddText ("cmd cmdsupported ", cbuflevel);
				Cbuf_AddText (com_token, cbuflevel);
				Cbuf_AddText ("\n", cbuflevel);
			}
		}
		else if (!strncmp(stufftext, "//exectrigger ", 14))		//so that mods can add whatever 'alias grabbedarmour' or whatever triggers that users might want to script responses for, without errors about unknown commands
		{
			COM_Parse(stufftext + 14);
			if (Cmd_AliasExist(com_token, cbuflevel))
				Cmd_ExecuteString(com_token, cbuflevel);	//do this NOW so that it's done before any models or anything are loaded
		}
		else if (!strncmp(stufftext, "//set ", 6))				//equivelent to regular set, except non-spammy if it doesn't exist, and happens instantly without extra latency.
		{
			Cmd_ExecuteString(stufftext+2, cbuflevel);	//do this NOW so that it's done before any models or anything are loaded
		}
		else if (!strncmp(stufftext, "//at ", 5))				//ktx autotrack hints
		{
			Cam_SetModAutoTrack(atoi(stufftext+5));
		}
		else if (!strncmp(stufftext, "//wps ", 5))				//ktx weapon statistics
		{
			Cmd_TokenizeString(stufftext+5, false, false);
			CL_ParseWeaponStats();
		}
		else if (!strncmp(stufftext, "//kickfile ", 11))		//FTE sends this to give a more friendly error about modified BSP files, although it could be used for more stuff.
		{
			flocation_t loc;
			Cmd_TokenizeString(stufftext+2, false, false);
			if (FS_FLocateFile(Cmd_Argv(1), FSLF_IFFOUND, &loc))
			{
				if (!*loc.rawname)
					Con_Printf("You have been kicked due to the file "U8("%s")" being modified, inside "U8("%s")"\n", Cmd_Argv(1), loc.search->logicalpath);
				else
					Con_Printf("You have been kicked due to the file "U8("%s")" being modfied, located at "U8("%s")"\n", Cmd_Argv(1), loc.rawname);
			}
		}
		else if (!strncmp(stufftext, "//it ", 5))				//it <timeout> <org xyz> <radius> <rgb> <timername> <entnum>
		{
			Cmd_TokenizeString(stufftext+5, false, false);
			CL_ParseItemTimer();
		}
		else if (!strncmp(stufftext, "//fui ", 6))				//ui <slot> <key> <value>. Full user info updates.
		{
			unsigned int slot;
			const char *value;
			Cmd_TokenizeString(stufftext+6, false, false);
			slot = atoi(Cmd_Argv(0));
			value = Cmd_Argv(1);
			if (slot < MAX_CLIENTS)
			{
				player_info_t *player = &cl.players[slot];
				Con_DPrintf("SETINFO %s: %s\n", player->name, value);
				InfoBuf_FromString(&player->userinfo, value, false);
				player->userinfovalid = true;
				CL_ProcessUserInfo (slot, player);
			}
		}
		else if (!strncmp(stufftext, "//ui ", 5))				//ui <slot> <key> <value>. Partial user info updates.
		{
			unsigned int slot;
			const char *key, *value;
			Cmd_TokenizeString(stufftext+5, false, false);
			slot = atoi(Cmd_Argv(0));
			key = Cmd_Argv(1);
			value = Cmd_Argv(2);
			if (slot < MAX_CLIENTS)
			{
				player_info_t *player = &cl.players[slot];
				Con_DPrintf("SETINFO %s: %s=%s\n", player->name, key, value);
				InfoBuf_SetValueForStarKey (&player->userinfo, key, value);
				CL_ProcessUserInfo (slot, player);
			}
		}
		else if (!strncmp(stufftext, "//qul ", 6))	//qtv user list
		{
			unsigned int cmd, id;
			const char *name;
			struct qtvviewers_s **link, *v;
			Cmd_TokenizeString(stufftext+5, false, false);
			cmd = atoi(Cmd_Argv(0));
			id = atoi(Cmd_Argv(1));
			name = Cmd_Argv(2);
			for (link = &cls.qtvviewers; (v=*link); link = &v->next)
			{
				if (v->userid == id)
					break;
			}
			switch(cmd)
			{
			case 3://del
				if (v)
				{
					*link = v->next;
					Z_Free(v);
				}
				break;
			case 2://change
			case 1://add
				if (!v)
				{	//new...
					v = Z_Malloc(sizeof(*v));
					v->next = cls.qtvviewers;
					cls.qtvviewers = v;
					v->userid = id;
				}
				Q_strncpyz(v->name, name, sizeof(v->name));
				break;
			}
		}
#ifdef PLUGINS
		else if (!strncmp(stufftext, "//tinfo ", 8))			//ktx-team-info <pidx> <org xyz> <health> <armour> <STAT_ITEMS> <nickname>
		{
			Cmd_TokenizeString(stufftext+2, false, false);
			CL_ParseTeamInfo();
			Plug_Command_f();	//FIXME: deprecate this call
		}
		else if (!strncmp(stufftext, "//sn ", 5))
		{
			Cmd_TokenizeString(stufftext+2, false, false);
			Plug_Command_f();
		}
#endif
		else if (!strncmp(stufftext, "//demomark", 10) && (stufftext[10]==0||stufftext[10]==' ') && cls.demoseeking == DEMOSEEK_MARK)
		{	//found the next marker. we're done seeking.
			cls.demoseeking = DEMOSEEK_NOT;
			//FIXME: pause it.
		}
		else
		{
			if (!strncmp(stufftext, "cmd ", 4))
				Cbuf_AddText (va("p%i ", destsplit+1), cbuflevel);	//without this, in_forceseat can break directed cmds.
			Cbuf_AddText (stufftext, cbuflevel);
			Cbuf_AddText ("\n", cbuflevel);
		}
		msg++;

		memmove(stufftext, msg, strlen(msg)+1);
	}
}

static void CL_ParsePrecache(void)
{
	int i, code = (unsigned short)MSG_ReadShort();
	char *s = MSG_ReadString();
	i = code & ~PC_TYPE;
	switch(code & PC_TYPE)
	{
	case PC_MODEL:
		if (i >= 1 && i < MAX_PRECACHE_MODELS)
		{
			model_t *model;
			Z_StrDupPtr(&cl.model_name[i], s);

			CL_CheckOrEnqueDownloadFile(s, s, DLLF_ALLOWWEB);
			model = Mod_ForName(Mod_FixName(s, cl.model_name[1]), (i == 1&&!cl.sendprespawn)?MLV_ERROR:MLV_WARN);
//			if (!model)
//				Con_Printf("svc_precache: Mod_ForName(\"%s\") failed\n", s);
			cl.model_precache[i] = model;
		}
		else
			Con_Printf("svc_precache: model index %i outside range %i...%i\n", i, 1, MAX_PRECACHE_MODELS);
		break;
	case PC_UNUSED:
		break;
	case PC_SOUND:
		if (i >= 1 && i < MAX_PRECACHE_SOUNDS)
		{
			sfx_t *sfx;
			if (S_HaveOutput())
				CL_CheckOrEnqueDownloadFile(va("sound/%s", s), NULL, DLLF_ALLOWWEB);
			sfx = S_PrecacheSound (s);
//			if (!sfx)
//				Con_Printf("svc_precache: S_PrecacheSound(\"%s\") failed\n", s);
			cl.sound_precache[i] = sfx;
			Z_StrDupPtr(&cl.sound_name[i], s);
		}
		else
			Con_Printf("svc_precache: sound index %i outside range %i...%i\n", i, 1, MAX_PRECACHE_SOUNDS);
		break;
	case PC_PARTICLE:
		if (i >= 1 && i < MAX_SSPARTICLESPRE)
		{
			Z_StrDupPtr(&cl.particle_ssname[i], s);
			cl.particle_ssprecache[i] = P_FindParticleType(s);
			cl.particle_ssprecaches = true;
		}
		else
			Con_Printf("svc_precache: particle index %i outside range %i...%i\n", i, 1, MAX_SSPARTICLESPRE);
		break;
	}
}

void Con_HexDump(qbyte *packet, size_t len, size_t badoffset, size_t stride)
{
	int i;
	int pos;

	pos = 0;
	while(pos < len)
	{
		Con_Printf("%5i ", pos);
		for (i = 0; i < stride; i++)
		{
			if (pos >= len)
				Con_Printf(" - ");
			else if (pos == badoffset)
				Con_Printf("^b^1%2x ", packet[pos]);
			else
				Con_Printf("%2x ", packet[pos]);
			pos++;
		}
		pos-=stride;
		for (i = 0; i < stride; i++)
		{
			if (pos >= len)
				Con_Printf("X");
			else if (packet[pos] == 0 || packet[pos] == '\t' || packet[pos] == '\r' || packet[pos] == '\n')
			{
				if (pos == badoffset)
					Con_Printf("^b^1.");
				else
					Con_Printf(".");
			}
			else
			{
				if (pos == badoffset)
					Con_Printf("^b^1%c", packet[pos]);
				else
					Con_Printf("%c", packet[pos]);
			}
			pos++;
		}
		Con_Printf("\n");
	}

}
void CL_DumpPacket(void)
{
	Con_HexDump(net_message.data, net_message.cursize, MSG_GetReadCount()-1, 16);
}

static void CL_ParsePortalState(void)
{
	int mode = MSG_ReadByte();
	int p = -1, a1 = -1, a2 = -1, state = -1;
#define PS_NEW			(1<<7)
#define PS_AREANUMS		(1<<6)	//q3 style
#define PS_PORTALNUM	(1<<5)	//q2 style
#define PS_LARGE		(1<<1)
#define PS_OPEN			(1<<0)

	if (mode & PS_NEW)
	{
		state = mode&1;
		if (!(mode & PS_AREANUMS) && !(mode & PS_PORTALNUM))
			mode |= PS_PORTALNUM;	//legacy crap

		if (mode & PS_PORTALNUM)
		{	//q2 style
			if (mode&PS_LARGE)
				p = MSG_ReadShort();
			else
				p = MSG_ReadByte();
		}
		if (mode & PS_AREANUMS)
		{	//q3 style
			if (mode&PS_LARGE)
			{
				a1 = MSG_ReadShort();
				a2 = MSG_ReadShort();
			}
			else
			{
				a1 = MSG_ReadByte();
				a2 = MSG_ReadByte();
			}
		}
	}
	else
	{	//legacy crap
		Con_Printf(CON_WARNING"svc_setportalstate: legacy mode\n");
		mode |= MSG_ReadByte()<<8;
		p = (mode & 0x7fff);
		state = !!(mode & 0x8000);
	}

#ifdef HAVE_SERVER
	//reduce race conditions when we're both client+server.
	if (sv.active)
		return;
#endif

	if (cl.worldmodel && cl.worldmodel->loadstate==MLS_LOADED && cl.worldmodel->funcs.SetAreaPortalState)
		cl.worldmodel->funcs.SetAreaPortalState(cl.worldmodel, p, a1, a2, state);
}

static void CL_ParseBaseAngle(int seat)
{
	int i;
	short diff[3];
	short newbase[3];
	vec3_t newang;

	inframe_t *inf = &cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK];
	qbyte fl = MSG_ReadByte();	//pitch yaw roll lock
	for (i=0 ; i<3 ; i++)
	{
		if (fl & (1u<<i))
			newbase[i] = MSG_ReadShort();
		else
			newbase[i] = 0;

		diff[i] = newbase[i]-cl.playerview[seat].baseangles[i];
		cl.playerview[seat].baseangles[i] = newbase[i];

		if (fl & 8)	//locking the view
			newang[i] = SHORT2ANGLE(newbase[i]);
		else	//free-look
			newang[i] = cl.playerview[seat].viewangles[i] + SHORT2ANGLE(diff[i]);
	}

	if (cl.ackedmovesequence)
	{
		//tweak all unacked input frames to the new base. the server will do similar on receipt of them.
		i = min(64, cl.movesequence-cl.ackedmovesequence);
		for (i = cl.movesequence-i; i < cl.movesequence; i++)
		{
			cl.outframes[i & UPDATE_MASK].cmd[seat].angles[0] += diff[0];
			cl.outframes[i & UPDATE_MASK].cmd[seat].angles[1] += diff[1];
			cl.outframes[i & UPDATE_MASK].cmd[seat].angles[2] += diff[2];
		}
	}

	if (!CSQC_Parse_SetAngles(seat, newang, false))
	{
		if (fl & 8)
		{
			inf->packet_entities.fixangles[seat] = true;
			VectorCopy (newang, inf->packet_entities.fixedangles[seat]);
		}
		VectorCopy (newang, cl.playerview[seat].viewangles);
	}
	VectorCopy (newang, cl.playerview[seat].intermissionangles);

	if (fl & 8)
		VRUI_SnapAngle();
}

void CLEZ_ParseHiddenDemoMessage(void)
{
	for(;;)
	{
		static float throttle;
		int size = MSG_ReadLong();
		unsigned int cmd;
		if (size == -1 || msg_badread)
			break;

		cmd = MSG_ReadUInt16();
		switch(cmd)
		{
		case 0xFFFF://mvdhidden_extended
			return;	//can't handle it... protocol is stupid.

		case 0x0003://mvdhidden_demoinfo
			MSG_ReadUInt16();		//'more'
			MSG_ReadSkip(size-2);	//probably json
			break;
		case 0x0007://mvdhidden_dmgdone
			{
				lerpents_t *le;
				unsigned short typeandflags = MSG_ReadUInt16();
				unsigned short attacker	= MSG_ReadUInt16();
				unsigned short targ = MSG_ReadUInt16();
				short dmg = MSG_ReadShort();
				unsigned short issplash = !!(typeandflags&0x8000);
				unsigned short isteamdamage = (attacker==targ) || (cl.teamplay && attacker-1<countof(cl.players)&&targ-1<countof(cl.players)&&!strcmp(cl.players[attacker].team, cl.players[targ].team));

				typeandflags &= ~0x8000;

				//let csqc handle it consistently with other ktx quirks.
				for (cmd = 0; cmd < cl.splitclients; cmd++)
					if (CL_TryTrackNum(&cl.playerview[cmd]) == attacker)
					{
						if (targ-1u < countof(cl.lerpplayers) && cl.lerpplayers[targ-1].sequence == cl.lerpentssequence)
							le = &cl.lerpplayers[targ-1];	//favour use player indexes... stoopid qw.
						else if (targ < cl.maxlerpents && cl.lerpents[targ].sequence == cl.lerpentssequence)
							le = &cl.lerpents[targ];
						else
							break;	//nope, can't place it... despite it being an mvd.
						CL_ParseStuffCmd(va("//ktx di %g %g %g %d %d %d %d\n", le->origin[0], le->origin[1], le->origin[2], typeandflags, dmg, issplash, isteamdamage), cmd);
					}
			}
			break;

/*		case 0x0000://mvdhidden_antilag_position
		case 0x0001://mvdhidden_usercmd						// <byte: source playernum> <todo>
		case 0x0002://mvdhidden_usercmd_weapons				// <byte: source playernum> <int: items> <byte[4]: ammo> <byte: result> <byte*: weapon priority (nul terminated)>
		case 0x0004://mvdhidden_commentary_track
		case 0x0005://mvdhidden_commentary_data
		case 0x0006://mvdhidden_commentary_text_segment
		case 0x0008://mvdhidden_usercmd_weapons_ss			// (same format as mvdhidden_usercmd_weapons)
		case 0x0009://mvdhidden_usercmd_weapon_instruction	// <byte: playernum> <byte: flags> <int: sequence#> <int: mode> <byte[10]: weaponlist>
		case 0x000A://mvdhidden_paused_duration
*/
		default:
			Con_ThrottlePrintf(&throttle, 1, "CL_ParseHiddenDemoMessage: Unknown cmd %i\n", cmd);
			MSG_ReadSkip(size);
			break;
		}
	}
}


#define SHOWNETEOM(x) if(cl_shownet.value>=2)Con_Printf ("%3i:%s\n", MSG_GetReadCount(), x);
#define SHOWNET(x) if(cl_shownet.value>=2)Con_Printf ("%3i:%s\n", MSG_GetReadCount()-1, x);
#define SHOWNET2(x, y) if(cl_shownet.value>=2)Con_Printf ("%3i:%3i:%s\n", MSG_GetReadCount()-1, y, x);
/*
=====================
CL_ParseServerMessage
=====================
*/
void CLQW_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	unsigned int u;
	int			i, j;
	int			destsplit;
	vec3_t ang;
	float f;
	qboolean	suggestcsqcdebug = false;
	inframe_t	*inf;
	extern vec3_t demoangles;
	unsigned int cmdstart;

	cl.last_servermessage = realtime;
	CL_ClearProjectiles ();

	//clear out fixangles stuff
	inf = &cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK];
	for (j = 0; j < MAX_SPLITS; j++)
		inf->packet_entities.fixangles[j] = false;
	if (cls.demoplayback == DPB_QUAKEWORLD)
	{
		inf->packet_entities.fixangles[0] = 2;
		VectorCopy(demoangles, inf->packet_entities.fixedangles[0]);
	}
	else if (cl.intermissionmode != IM_NONE)
	{
		for (destsplit = 0; destsplit < cl.splitclients; destsplit++)
		{
			inf->packet_entities.fixangles[destsplit] = 2;
			VectorCopy(cl.playerview[destsplit].intermissionangles, inf->packet_entities.fixedangles[destsplit]);
		}
	}

//
// if recording demos, copy the message out


	//
	if (cl_shownet.value == 1)
		Con_Printf ("%i ",net_message.cursize);
	else if (cl_shownet.value >= 2)
		Con_Printf ("------------------\n");


	CL_ParseClientdata ();

	//vanilla QW has no timing info in the client and depends upon the client for all timing.
	//using the demo's timing for interpolation prevents unneccesary drift, and solves issues with demo seeking and other such things.
	if (cls.demoplayback == DPB_QUAKEWORLD && !(cls.fteprotocolextensions & PEXT_ACCURATETIMINGS))
	{
		extern float demtime;
		if (cl.gametime != demtime)
		{
			cl.oldgametime = cl.gametime;
			cl.oldgametimemark = cl.gametimemark;
			cl.gametime = demtime;
			cl.gametimemark = realtime;
		}
	}

	if (realtime > packetusageflushtime)
	{
		memcpy(packetusage_saved, packetusage_pending, sizeof(packetusage_saved));
		memset(packetusage_pending, 0, sizeof(packetusage_pending));
		packetusageflushtime = realtime + packetusage_interval;
	}

//
// parse the message
//
	while (1)
	{
		if (msg_badread)
		{
			CL_DumpPacket();
			Host_EndGame ("CLQW_ParseServerMessage: Bad server message");
			break;
		}

		cmdstart = MSG_GetReadCount();
		cmd = MSG_ReadByte ();

		if (cmd == svcfte_choosesplitclient)
		{
			SHOWNET2(svc_qwstrings[cmd], cmd);

			destsplit = MSG_ReadByte() % MAX_SPLITS;
			cmd = MSG_ReadByte();
		}
		else
			destsplit = cl.defaultnetsplit;

		if (cmd == -1)
		{
			SHOWNETEOM("END OF MESSAGE");
			break;
		}

		SHOWNET2(svc_qwstrings[cmd], cmd);

	// other commands
		switch (cmd)
		{
		default:
			CL_DumpPacket();
			Host_EndGame ("CLQW_ParseServerMessage: Illegible server message (%i@%i)%s", cmd, MSG_GetReadCount()-1, (!cl.csqcdebug && suggestcsqcdebug)?"\n'sv_csqcdebug 1' might aid in debugging this.":"" );
			return;

		case svc_time:
			cl.oldgametime = cl.gametime;
			cl.gametime = MSG_ReadFloat();
			cl.gametimemark = realtime;
			break;

		case svc_nop:
//			Con_Printf ("svc_nop\n");
			//If we're using cl_shownet 2, combine large padding blocks into a single line, otherwise we'll get 1000+ lines from a single packet.
			while (svc_nop == MSG_PeekByte())
				MSG_ReadByte();
			break;

		case svc_disconnect:
			if (cls.demoplayback == DPB_MVD)	//eztv fails to detect the end of demos.
			{
				s = MSG_ReadString();
				Con_Printf(CON_WARNING"svc_disconnect: %s\n", s);
			}
			else if (cls.demoplayback)
			{
				CL_Disconnect(NULL);
				CL_NextDemo();
				return;
			}
			else if (cls.state == ca_connected)
			{
				Host_EndGame ("Server disconnected\n");
			}
			else
				Host_EndGame ("Server disconnected");
			break;

		case svc_print:
			i = MSG_ReadByte ();
			s = MSG_ReadString ();
			CL_ParsePrint(s, i);
			break;

		case svc_centerprint:
			s = MSG_ReadString ();

#ifdef PLUGINS
			if (Plug_CenterPrintMessage(s, destsplit))
#endif
				SCR_CenterPrint (destsplit, s, false);
			break;

		case svc_stufftext:
			s = MSG_ReadString ();
			CL_ParseStuffCmd(s, destsplit);
			break;

		case svc_damage:
			V_ParseDamage (&cl.playerview[destsplit]);
			break;

		case svc_serverdata:
			Cbuf_Execute ();		// make sure any stuffed commands are done
 			CLQW_ParseServerData ();
			break;
		case svcfte_splitscreenconfig:
			j = cl.splitclients;
			cl.splitclients = MSG_ReadByte();
			for (i = 0; i < cl.splitclients && i < MAX_SPLITS; i++)
			{
				cl.playerview[i].playernum = MSG_ReadByte();
				cl.playerview[i].viewentity = cl.playerview[i].playernum+1;
				if (i>=j)	//its new.
					cl.playerview[i].chatstate = 0;
			}
			if (i < cl.splitclients)
			{
				Con_Printf("Server sent us too many seats!\n");
				for (; i < cl.splitclients; i++)
				{	//svcfte_choosesplitclient has a modulo that is also broken, but at least there's no parse errors this way
					MSG_ReadByte();
//					CL_SendSeatClientCommand(true, i, drop");
				}
				cl.splitclients = MAX_SPLITS;
			}
			break;
#ifdef PEXT_SETVIEW
		case svc_setview:
			if (!(cls.fteprotocolextensions & PEXT_SETVIEW))
				Con_Printf("^1PEXT_SETVIEW is meant to be disabled\n");
			cl.playerview[destsplit].viewentity=MSGCL_ReadEntity();
			break;
#endif
		case svcfte_setanglebase:
			CL_ParseBaseAngle(destsplit);
			break;
		case svcfte_setangledelta:
			for (i=0 ; i<3 ; i++)
				ang[i] = cl.playerview[destsplit].viewangles[i] + MSG_ReadAngle16 ();
			if (!CSQC_Parse_SetAngles(destsplit, ang, true))
				VectorCopy (ang, cl.playerview[destsplit].viewangles);
			VectorCopy (cl.playerview[destsplit].viewangles, cl.playerview[destsplit].simangles);
			VectorCopy (cl.playerview[destsplit].viewangles, cl.playerview[destsplit].intermissionangles);
			break;
		case svc_setangle:
			if (cls.demoplayback == DPB_MVD)
			{
				//I really don't get the point of fixangles in an mvd. just to disable interpolation for that frame?
				int pl = MSG_ReadByte();
				for (i=0 ; i<3 ; i++)
					ang[i] = MSG_ReadAngle();
				for (j = 0; j < cl.splitclients; j++)
				{
					playerview_t *pv = &cl.playerview[j];
					if (Cam_TrackNum(pv) == pl)
					{
						inf->packet_entities.fixangles[j] = true;
						VectorCopy(ang, inf->packet_entities.fixedangles[j]);
					}
				}
			}
			else
			{
				inframe_t *inf = &cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK];
				int fixtype = 2;
				if (cls.ezprotocolextensions1 & EZPEXT1_SETANGLEREASON)
					fixtype = MSG_ReadByte();	//0=unknown, 1=tele, 2=spawn
				for (i=0 ; i<3 ; i++)
					ang[i] = MSG_ReadAngle();
				if (fixtype == 1)
				{	//relative
					VectorAdd(ang, cl.playerview[destsplit].viewangles, ang);
					VectorSubtract(ang, cl.outframes[cls.netchan.incoming_sequence&UPDATE_MASK].cmd->angles, ang);
				}
				else fixtype = 2;	//snap
				if (!CSQC_Parse_SetAngles(destsplit, ang, fixtype==1))
				{
					inf->packet_entities.fixangles[destsplit] = true;
					VectorCopy (ang, cl.playerview[destsplit].viewangles);
					VectorCopy (ang, inf->packet_entities.fixedangles[destsplit]);
				}
				VectorCopy (cl.playerview[destsplit].viewangles, cl.playerview[destsplit].intermissionangles);
			}
			break;

		case svc_lightstyle:
			i = MSG_ReadByte ();
			if (i >= MAX_NET_LIGHTSTYLES)
				Host_EndGame ("svc_lightstyle > MAX_LIGHTSTYLES");
			s = MSG_ReadString();
			if (cl_shownet.value == 3)
				Con_Printf("\t%i=\"%s\"\n", i, s);
			R_UpdateLightStyle(i, s, 1, 1, 1);
			break;
#ifdef PEXT_LIGHTSTYLECOL
		case svcfte_lightstylecol:
			if (!(cls.fteprotocolextensions & PEXT_LIGHTSTYLECOL))
				Host_EndGame("PEXT_LIGHTSTYLECOL is meant to be disabled\n");
			{
				int bits;
				vec3_t rgb;
				i = MSG_ReadByte ();
				bits = MSG_ReadByte();
				if (bits & 0x40)
					i |= MSG_ReadByte()<<8;	//high bits of style index.
				if (bits & 0x80)
				{
					rgb[0] = (bits&1)?MSG_ReadShort()/1024.0:0;
					rgb[1] = (bits&2)?MSG_ReadShort()/1024.0:0;
					rgb[2] = (bits&4)?MSG_ReadShort()/1024.0:0;
				}
				else
				{
					rgb[0] = (bits&1)?1:0;
					rgb[1] = (bits&2)?1:0;
					rgb[2] = (bits&4)?1:0;
				}

				if (i >= MAX_NET_LIGHTSTYLES)
					Host_EndGame ("svc_lightstyle > MAX_LIGHTSTYLES");
				R_UpdateLightStyle(i, MSG_ReadString(), rgb[0], rgb[1], rgb[2]);
			}
			break;
#endif

		case svc_sound:
			CLQW_ParseStartSoundPacket();
			break;
#ifdef PEXT_SOUNDDBL
		case svcfte_soundextended:
			CLNQ_ParseStartSoundPacket();
			break;
#endif

		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i>>3, i&7);
			break;

#ifdef PEXT2_VOICECHAT
		case svcfte_voicechat:
			S_Voip_Parse();
			break;
#endif

#ifdef TERRAIN
		case svcfte_brushedit:
			CL_Parse_BrushEdit();
			break;
#endif

		case svc_updatefrags:
			Sbar_Changed ();
			u = MSG_ReadPlayer();
			if (u >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			cl.players[u].frags = MSG_ReadShort ();
			break;

		case svc_updateping:
			u = MSG_ReadPlayer();
			if (u >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateping > MAX_SCOREBOARD");
			cl.players[u].ping = MSG_ReadShort ();
			break;

		case svc_updatepl:
			u = MSG_ReadPlayer();
			if (u >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatepl > MAX_SCOREBOARD");
			cl.players[u].pl = MSG_ReadByte ();
			break;

		case svc_updateentertime:
		// time is sent over as seconds ago
			u = MSG_ReadPlayer();
			if (u >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateentertime > MAX_SCOREBOARD");
			cl.players[u].realentertime = realtime - MSG_ReadFloat ();
			break;

		case svc_spawnbaseline:
			i = MSGCL_ReadEntity ();
			if (!CL_CheckBaselines(i))
				Host_EndGame("CL_ParseServerMessage: svc_spawnbaseline failed with size %i", i);
			CL_ParseBaseline (cl_baselines + i, CPNQ_ID);
			break;
		case svcfte_spawnbaseline2:
			CL_ParseBaselineDelta ();
			break;
		case svc_spawnstatic:
			CL_ParseStaticProt (CPNQ_ID);
			break;
		case svcfte_spawnstatic2:
			CL_ParseStaticProt (-1);
			break;
		case svc_temp_entity:
#ifdef NQPROT
			CL_ParseTEnt (false);
#else
			CL_ParseTEnt ();
#endif
			break;
		case svcfte_temp_entity_sized:
			CL_ParseTEnt_Sized();
			break;
		case svcfte_customtempent:
			CL_ParseCustomTEnt();
			break;

		case svc_particle:
			CLNQ_ParseParticleEffect ();
			break;
		case svcfte_particle2:
			CL_ParseParticleEffect2 ();
			break;
		case svcfte_particle3:
			CL_ParseParticleEffect3 ();
			break;
		case svcfte_particle4:
			CL_ParseParticleEffect4 ();
			break;

		case svc_killedmonster:
			//fixme: update all player stats
#ifdef QUAKESTATS
			cl.playerview[destsplit].stats[STAT_MONSTERS]++;
			cl.playerview[destsplit].statsf[STAT_MONSTERS]++;
#endif
			break;
		case svc_foundsecret:
			//fixme: update all player stats
#ifdef QUAKESTATS
			cl.playerview[destsplit].stats[STAT_SECRETS]++;
			cl.playerview[destsplit].statsf[STAT_SECRETS]++;
#endif
			break;

		case svcqw_updatestatbyte:
			i = MSG_ReadByte ();
			j = MSG_ReadByte ();
			CL_SetStatNumeric(destsplit, i, j, j);
			break;
		case svcqw_updatestatlong:
			i = MSG_ReadByte ();
			j = MSG_ReadLong ();	//make qbyte if nq compatability?
			CL_SetStatNumeric (destsplit, i, j, j);
			break;

		case svcfte_updatestatstring:
			i = MSG_ReadByte();
			s = MSG_ReadString();
			CL_SetStatString (destsplit, i, s);
			break;
		case svcfte_updatestatfloat:
			i = MSG_ReadByte();
			f = MSG_ReadFloat();
			CL_SetStatNumeric (destsplit, i, f, f);
			break;
/*		case svcfte_updatebigstat:
			CL_ParseExtendedStat();
			break;*/

		case svc_spawnstaticsound:
			CL_ParseStaticSound (false);
			break;
		case svcfte_spawnstaticsound2:
			CL_ParseStaticSound (MSG_ReadByte());
			break;

		case svc_cdtrack:
			{
				//quakeworld got a crippled svc_cdtrack.
				unsigned int firsttrack;
				firsttrack = MSG_ReadByte ();
				Media_NumberedTrack (firsttrack, firsttrack);
			}
			break;

		case svc_intermission:
			if (cl.intermissionmode == IM_NONE)
			{
				TP_ExecTrigger ("f_mapend", false);
				if (cl.playerview[destsplit].spectator || cls.demoplayback)
					TP_ExecTrigger ("f_specmapend", true);
				else
					TP_ExecTrigger ("f_playmapend", true);
				cl.completed_time = cl.gametime;
			}
			cl.intermissionmode = IM_QWSCORES;
			for (i=0 ; i<3 ; i++)
				cl.playerview[destsplit].simorg[i] = MSG_ReadCoord ();
			for (i=0 ; i<3 ; i++)
				cl.playerview[destsplit].intermissionangles[i] = MSG_ReadAngle ();

			if (cls.demoseeking == DEMOSEEK_INTERMISSION)
			{
				cls.demoseeking = DEMOSEEK_NOT; //reached it.
				//FIXME: pause it.
			}
			break;

		case svc_finale:
			if (cl.intermissionmode == IM_NONE)
			{
				for (i = 0; i < MAX_SPLITS; i++)
					cl.playerview[i].simorg[2] += cl.playerview[i].viewheight;
				VectorCopy (cl.playerview[destsplit].simangles, cl.playerview[destsplit].intermissionangles);
				cl.completed_time = cl.gametime;
			}
			cl.intermissionmode = IM_NQFINALE;
			SCR_CenterPrint (destsplit, TL_Translate(com_language, MSG_ReadString ()), false);
			break;

		case svc_sellscreen:
			Cmd_ExecuteString ("help", RESTRICT_SERVER);
			break;

		case svc_smallkick:
			cl.playerview[destsplit].punchangle_cl = -2;
			break;
		case svc_bigkick:
			cl.playerview[destsplit].punchangle_cl = -4;
			break;

		case svc_muzzleflash:
			CL_MuzzleFlash (MSGCL_ReadEntity());
			break;

		case svc_updateuserinfo:
			CL_UpdateUserinfo ();
			break;

		case svc_setinfo:
			CL_ParseSetInfo ();
			break;
		case svc_serverinfo:
			CL_ServerInfo ();
			break;
		case svcfte_setinfoblob:
			CL_ParseSetInfoBlob();
			break;

		case svc_download:
			CL_ParseDownload (false);
			break;

		case svc_playerinfo:
			CLQW_ParsePlayerinfo ();
			break;

		case svc_nails:
			CL_ParseProjectiles (cl_spikeindex, false);
			break;
		case svc_nails2:
			CL_ParseProjectiles (cl_spikeindex, true);
			break;

		case svc_chokecount:		// some preceding packets were choked
			i = MSG_ReadByte ();
			for (j=0 ; j<i ; j++)
				cl.outframes[(cls.netchan.incoming_acknowledged-1-j)&UPDATE_MASK].latency = -2;
			break;

		case svc_modellist:
			CL_ParseModellist (false);
			break;
		case svcfte_modellistshort:
			CL_ParseModellist (true);
			break;

		case svc_soundlist:
			CL_ParseSoundlist (false);
			break;
#ifdef PEXT_SOUNDDBL
		case svcfte_soundlistshort:
			CL_ParseSoundlist (true);
			break;
#endif

		case svc_packetentities:
			CLQW_ParsePacketEntities (false);
			break;

		case svc_deltapacketentities:
			CLQW_ParsePacketEntities (true);
			break;
		case svcfte_updateentities:
			CLFTE_ParseEntities();
			break;

		case svc_maxspeed:
			cl.playerview[destsplit].maxspeed = MSG_ReadFloat();
			break;

		case svc_entgravity:
			cl.playerview[destsplit].entgravity = MSG_ReadFloat();
			break;

		case svc_setpause:
			cl.paused = MSG_ReadByte ();
//			Media_SetPauseTrack(!!cl.paused);
			break;

//		case svc_ftesetclientpersist:
//			CL_ParseClientPersist();
//			break;
		case svc_setportalstate:
			CL_ParsePortalState();
			break;

		case svcfte_showpic:
			SCR_ShowPic_Create();
			break;
		case svcfte_hidepic:
			SCR_ShowPic_Hide();
			break;
		case svcfte_movepic:
			SCR_ShowPic_Move();
			break;
		case svcfte_updatepic:
			SCR_ShowPic_Update();
			break;

		case svcfte_effect:
			CL_ParseEffect(false);
			break;
		case svcfte_effect2:
			CL_ParseEffect(true);
			break;

#ifdef CSQC_DAT
		case svcfte_csqcentities:
			suggestcsqcdebug = true;
			CSQC_ParseEntities(cl.csqcdebug);
			break;
		case svcfte_csqcentities_sized:
			CSQC_ParseEntities(true);
			break;
#endif
		case svcfte_precache:
			CL_ParsePrecache();
			break;

		case svcfte_trailparticles:
			CL_ParseTrailParticles();
			break;
		case svcfte_pointparticles:
			CL_ParsePointParticles(false);
			break;
		case svcfte_pointparticles1:
			CL_ParsePointParticles(true);
			break;

		case svcfte_cgamepacket_sized:
#ifdef CSQC_DAT
			if (CSQC_ParseGamePacket(destsplit, true))
				break;
#endif
			Con_Printf("Unable to parse gamecode packet\n");
			break;
		case svcfte_cgamepacket:
			suggestcsqcdebug = true;
#ifdef HLCLIENT
			if (CLHL_ParseGamePacket())
				break;
#endif
#ifdef CSQC_DAT
			if (CSQC_ParseGamePacket(destsplit, cl.csqcdebug))
				break;
#endif
			Con_Printf("Unable to parse gamecode packet\n");
			break;
		}

		packetusage_pending[cmd] += MSG_GetReadCount()-cmdstart;
	}
}

#ifdef Q2CLIENT
static void CLQ2_ParseZPacket(void)
{
#ifndef AVAIL_ZLIB
	Host_EndGame ("CLQ2_ParseZPacket: zlib not supported in this build");
#else
	z_stream s;
	char *indata, *outdata;	//we're hacking stuff onto the end of the current buffer, to avoid issues if something errors out and doesn't leave net_message in a clean state
	unsigned short clen = MSG_ReadShort();
	unsigned short ulen = MSG_ReadShort();
	sizebuf_t restoremsg;
	if (clen > net_message.cursize-MSG_GetReadCount())
		Host_EndGame ("CLQ2_ParseZPacket: svcr1q2_zpacket truncated");
	if (ulen > net_message.maxsize-net_message.cursize)
		Host_EndGame ("CLQ2_ParseZPacket: svcr1q2_zpacket overflow");
	indata = net_message.data + MSG_GetReadCount();
	outdata = net_message.data + net_message.cursize;
	MSG_ReadSkip(clen);
	restoremsg = net_message;
	net_message.currentbit = net_message.cursize<<3;
	net_message.cursize += ulen;

	memset(&s, 0, sizeof(s));
	s.next_in = indata;
	s.avail_in = clen;
	s.total_in = 0;
	s.next_out = outdata;
	s.avail_out = ulen;
	s.total_out = 0;
	if (inflateInit2(&s, -15) != Z_OK)
		Host_EndGame ("CLQ2_ParseZPacket: unable to initialise zlib");
	if (inflate(&s, Z_FINISH) != Z_STREAM_END)
		Host_EndGame ("CLQ2_ParseZPacket: stream truncated");
	if (inflateEnd(&s) != Z_OK)
		Host_EndGame ("CLQ2_ParseZPacket: stream truncated");
	if (s.total_out != ulen || s.total_in != clen)
		Host_EndGame ("CLQ2_ParseZPacket: stream truncated");

	CLQ2_ParseServerMessage();
	net_message = restoremsg;
	msg_badread = false;
#endif
}
static void CLR1Q2_ParseSetting(void)
{
	int setting = MSG_ReadLong();
	int value = MSG_ReadLong();

	if (setting == R1Q2_SVSET_FPS)
	{
		cl.q2svnetrate = value;
		if (cl.validsequence)
			Con_Printf("warning: fps rate changed mid-game\n");	//fixme: we need to clean up lerping stuff. if its now lower, we might have a whole load of things waiting ages for a timeout.
	}
}
static void CLQ2EX_ParseFog(void)
{
	float d=0;
	qbyte r=255, g=255, b=255;
	unsigned short t=0;
	unsigned int flags = MSG_ReadByte();
	if (flags & (1<<7))
		flags |= MSG_ReadByte()<<8;

	if (flags & (1<<0))		d = MSG_ReadFloat();	//density
	if (flags & (1<<0))		MSG_ReadByte();	//density
	if (flags & (1<<1))		r = MSG_ReadByte();		//r
	if (flags & (1<<2))		g = MSG_ReadByte();		//g
	if (flags & (1<<3))		b = MSG_ReadByte();		//b
	if (flags & (1<<4))		t = MSG_ReadUInt16();	//time

	if (flags & (1<<5))		MSG_ReadFloat();	//h falloff
	if (flags & (1<<6))		MSG_ReadFloat();	//h density
	if (flags & (1<<8))		MSG_ReadByte();		//h s r
	if (flags & (1<<9))		MSG_ReadByte();		//h s g
	if (flags & (1<<10))	MSG_ReadByte();		//h s b
	if (flags & (1<<11))	MSG_ReadLong();		//h s dist
	if (flags & (1<<12))	MSG_ReadByte();		//h e r
	if (flags & (1<<13))	MSG_ReadByte();		//h e g
	if (flags & (1<<14))	MSG_ReadByte();		//h e b
	if (flags & (1<<15))	MSG_ReadLong();		//h e dist

	CL_ResetFog(FOGTYPE_AIR);
	cl.fog[FOGTYPE_AIR].density = d;
	cl.fog[FOGTYPE_AIR].colour[0] = SRGBf(r/255.0f);
	cl.fog[FOGTYPE_AIR].colour[1] = SRGBf(g/255.0f);
	cl.fog[FOGTYPE_AIR].colour[2] = SRGBf(b/255.0f);
	cl.fog[FOGTYPE_AIR].time += (t) / 100.0;
	cl.fog_locked = !!cl.fog[FOGTYPE_AIR].density;
}
static char *CLQ2EX_ReformatBinds(int seat, const char *source, char *buffer, size_t buffersize)
{	//q2e has various "%bind:cmd:$desc%" tags that should be expanded to some "<BUTTON> Desc\n" string instead, for tutorialy areas.
	//we're too dumb to know about any artwork for each key.
	//note that multiple can exist in any centerprint.
	char *s = buffer;
	if (s != source)
		Q_strncpyz(buffer, source, buffersize);
	while (!strncmp(s, "%bind:", 6))
	{
		int keys[1], keymods[countof(keys)];
		char *cmd = s+6;
		char *desc = strchr(cmd, ':');
		char *e;
		const char *rep, *key;
		if (!desc)
			break;
		e = strchr(desc, '%');
		if (!e)
			break;
		*desc++ = 0;
		*e++ = 0;
		rep = TL_Translate(com_language, desc);	//sigh

		if (M_FindKeysForCommand(0, seat, cmd, keys, keymods, countof(keys)) > 0)
			key = Key_KeynumToLocalString (keys[0], keymods[0]);
		else
			key = "UNBOUND";

		rep = va("<%s> %s\n", key, rep);

		if (s+strlen(rep) >= buffer+buffersize)
			rep = "";	//would overflow, just strip it out instead.
		memmove(s+strlen(rep), e, strlen(e)+1);
		memcpy(s, rep, strlen(rep));
		s += strlen(rep);
	}
	return buffer;
}
static void CLQ2EX_LocPrint(int targetseat)
{
	int seat;
	qbyte flags = MSG_ReadByte();
	qbyte level = flags&7;
	qboolean broadcast = !!(flags & (1<<3));	//meaningful clientside for splitscreen, at least with centerprints.
	qboolean nonotify = flags & (1<<4);
	unsigned short count, a;
	const char *arg[256];
	static char formatted[8192];
	char inputs[8192];
	size_t ofs = 0;
	char *s;
	arg[0] = MSG_ReadString();
	count = MSG_ReadByte();
	for (a = 1; a <= count && a < countof(arg); )
	{
		arg[a] = MSG_ReadStringBuffer(inputs+ofs, sizeof(inputs)-ofs-1);
		if (!strncmp(arg[a], "##P", 3))
		{
			int p = atoi(arg[a]+3);
			if (p < cl.allocated_client_slots)
			{
				inputs[ofs] = 0;
				arg[a] = cl.players[p].name;	//FIXME: do colours here instead of CL_ParsePrint?
			}
		}
		a++;
		ofs += strlen(inputs+ofs)+1;
		if (ofs >= sizeof(inputs))
			break;
	}
	for (; a <= count; a++)
		MSG_ReadString(); //don't lose space, though we can't buffer it.

	TL_Reformat(com_language, formatted, sizeof(formatted), a, arg);

	if (level == 4)
	{	//'typewriter'...
		for (seat = (broadcast?0:targetseat); (broadcast?(seat < cl.splitclients):(seat==targetseat)); seat++)
		{
			s = va("/Q/W%s/.%s",nonotify?"/N0":"", CLQ2EX_ReformatBinds(seat, formatted, inputs,sizeof(inputs)));
			SCR_CenterPrint(seat, s, false);
		}
		return;
	}
	else if (level == 5)
	{	//centerprint
		for (seat = (broadcast?0:targetseat); (broadcast?(seat < cl.splitclients):(seat==targetseat)); seat++)
		{
			s = va("%s/.%s",nonotify?"/N0":"", CLQ2EX_ReformatBinds(seat, formatted, inputs,sizeof(inputs)));
			SCR_CenterPrint(seat, s, false);
		}
		return;
	}
	else if (level >= 6)
		level = 3;	//TTS
	CL_ParsePrint(formatted, level);
}
static void CLQ2EX_ParseDamage(int seat)
{	//not really sure what this is useful for, when there's already damage blend info in the playerstate messages.
	vec3_t dir;
	qbyte count = MSG_ReadByte();
	while (count --> 0)
	{
		/*bits =*/ MSG_ReadByte();
		//dam = (bits&0x1f)*3;
		//health= !!(bits&0x20);
		//armour= !!(bits&0x40);
		//shield= !!(bits&0x80);
		MSG_ReadDir(dir);
	}
}
extern sizebuf_t	*msg_readmsg;
#ifdef AVAIL_ZLIB
size_t ZLib_DecompressBuffer(qbyte *in, size_t insize, qbyte *out, size_t maxoutsize);
#endif
static void CLQ2EX_ParseBlast(void(*func)(void))
{
#ifdef AVAIL_ZLIB
	sizebuf_t tmp, *prevmsg = msg_readmsg;
	unsigned int csize = MSG_ReadUInt16();
	unsigned int usize = MSG_ReadUInt16();
	void *cdata = msg_readmsg->data + (msg_readmsg->currentbit>>3);
	MSG_ReadSkip(csize);
	if (msg_badread)
		return;	//erk?!?

	//create a new uncompressed buffer...
	MSG_BeginWriting (&tmp, msg_readmsg->prim, alloca(usize), usize);
	if (ZLib_DecompressBuffer(cdata, csize, SZ_GetSpace (&tmp, usize), usize) != usize)
	{
		msg_badread = true;
		return;	//wut?
	}

	//and read it...
	MSG_BeginReading(&tmp, msg_readmsg->prim);
	while(tmp.currentbit>>3 < msg_readmsg->cursize)
		func();

	//before returning to our original buffer.
	msg_readmsg = prevmsg;
#else
	msg_badread = true;
#endif
}
static void CLQ2EX_ParseRouteMarker(void)
{
	qboolean isfirst;
	vec3_t pos;
	vec3_t dir;
	float timeout = 10;
	float radius = 16;

	struct itemtimer_s *timer;
	float start = cl.time;

	isfirst = MSG_ReadByte();
	MSG_ReadPos(pos);
	MSG_ReadDir(dir);
//	pos[2]-=24;

	if (isfirst)
		radius*=2;

	timer = Z_Malloc(sizeof(*timer));
	timer->next = cl.itemtimers;
	cl.itemtimers = timer;

	VectorCopy(pos, timer->origin);
	timer->start = cl.time;
	timer->duration = timeout;
	timer->radius = radius;
	timer->duration = timeout;
	timer->entnum = 0;
	timer->start = start;
	timer->end = start + timer->duration;
	timer->rgb[0] = 0.15;
	timer->rgb[1] = 1;
	timer->rgb[2] = 0.1;
}
static void CLQ2EX_ParseLevelRestart(void)
{	//fast restart... allowing us to skip (full)configstrings and baselines
	int id;
	char *str;
	while (!msg_badread)
	{
		id = MSG_ReadShort();
		if (id==-1)
			break;	//end of our svc.
		str = MSG_ReadString();
		CLQ2_UpdateConfigString(id, str);
	}

	//clear out some stuff that should not be lingering...
	for (id = 0; id < cl.splitclients; id++)
		SCR_CenterPrint(id, NULL, true);
}

void CL_WriteDemoMessage (sizebuf_t *msg, int payloadoffset);
static const char *q2svcnames[] = {
	"svcq2_bad",				//0

	// these ops are known to the game dll
	"svcq2_muzzleflash",		//1
	"svcq2_muzzleflash2",		//2
	"svcq2_temp_entity",		//3
	"svcq2_layout",				//4
	"svcq2_inventory",			//5

	// the rest are private to the client and server
	"q2_nop",					//6
	"q2_disconnect",			//7
	"q2_reconnect",				//8
	"q2_sound",					//9
	"q2_print",					//10
	"q2_stufftext",				//11
	"q2_serverdata",			//12
	"q2_configstring",			//13
	"q2_spawnbaseline",			//14
	"q2_centerprint",			//15
	"q2_download",				//16
	"q2_playerinfo",			//17
	"q2_packetentities",		//18
	"q2_deltapacketentities",	//19
	"q2_frame",					//20

	//extended tat.
	"q2ex_splitclient/r1q2_zpacket",	//21
	"q2ex_configblast/r1q2_zdownload",	//22
	"q2ex_spawnbaselineblast/r1q2_playerupdate/q2pro_gamestate",	//23
	"q2ex_Levelrestart/r1q2_setting/q2pro_setting",	//24
	"q2ex_danage",				//25
	"q2ex_locprint",			//26
	"q2ex_fog",					//27
	"q2ex_waiting",				//28
	"q2ex_botchat",				//29
	"q2ex_mapmarker",			//30
	"q2ex_routemarker",			//31
	"q2ex_achievement",			//32
};
void CLQ2_ParseServerMessage (void)
{
	int				cmd, last=-1;
	const char		*s;
	int				i;
	unsigned int	seat;
//	int				j;
	int startpos = MSG_GetReadCount();

	cl.last_servermessage = realtime;
	CL_ClearProjectiles ();

//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1)
		Con_Printf ("%i ",net_message.cursize);
	else if (cl_shownet.value >= 2)
		Con_Printf ("------------------\n");


	CL_ParseClientdata ();

//
// parse the message
//
	while (1)
	{
		if (msg_badread)
		{
			if (cl_shownet.ival)
				CL_DumpPacket();
			Host_EndGame ("CLQ2_ParseServerMessage: Bad server message (last %i)", last);
			break;
		}

		cmd = MSG_ReadByte ();

		seat = 0;
		if (cmd == svcq2_playerinfo && (cls.fteprotocolextensions & PEXT_SPLITSCREEN))
		{	//playerinfo should not normally be seen here.
			//so we can just 'borrow' it for seat numbers for targetted svcs.
			SHOWNET(va("%i", cmd));
			seat = MSG_ReadByte ();
seatedcommand:
			if (seat >= MAX_SPLITS)
				Host_EndGame ("CLQ2_ParseServerMessage: Unsupported seat (%i)", seat);
			cmd = MSG_ReadByte ();
		}

		if (cmd == -1)
		{
			SHOWNETEOM("END OF MESSAGE");
			break;
		}

		SHOWNET(cmd>=countof(q2svcnames)?va("%i", cmd):q2svcnames[cmd]);

	// other commands
		switch (cmd)
		{
		default:
isillegible:
			if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2 || cls.protocol_q2 == PROTOCOL_VERSION_Q2PRO)
			{
				switch(cmd & 0x1f)
				{
				case svcq2_frame:			//20 (the bastard to implement.)
					CLQ2_ParseFrame(cmd>>5);
					break;
				default:
					if (cl_shownet.ival)
						CL_DumpPacket();
					Host_EndGame ("CLQ2_ParseServerMessage: Illegible server message (%i, last %i)", cmd, last);
					return;
				}
				break;
			}
			if (cl_shownet.ival)
				CL_DumpPacket();
			Host_EndGame ("CLQ2_ParseServerMessage: Illegible server message (%i, last %i)", cmd, last);
			return;

	//known to game
		case svcq2_muzzleflash:
			CLQ2_ParseMuzzleFlash();
			break;
		case svcq2_muzzleflash2:
			CLQ2_ParseMuzzleFlash2();
			return;
		case svcq2_temp_entity:
			CLQ2_ParseTEnt();
			break;
		case svcq2_layout:
			s = MSG_ReadString ();
			Q_strncpyz (cl.q2layout[seat], s, sizeof(cl.q2layout[seat]));
			break;
		case svcq2_inventory:
			CLQ2_ParseInventory(seat);
			break;

	// the rest are private to the client and server
		case svcq2_nop:			//6
			break;	//q2e sends these when it has nothing else to send, for some reason.
			//or they're never seen and thus probably a bug.
			Host_EndGame ("CL_ParseServerMessage: svcq2_nop not implemented");
			return;
		case svcq2_disconnect:
			if (cls.state == ca_connected)
				Host_EndGame ("Server disconnected\n"
					"Server version may not be compatible");
			else
				Host_EndGame ("Server disconnected");
			return;
		case svcq2_reconnect:	//8. this is actually kinda weird to have
			Con_TPrintf ("reconnecting...\n");
#if 1
			CL_Disconnect("Reconnect request");
			CL_BeginServerReconnect();
			return;
#else
			CL_SendClientCommand(true, "new");
			break;
#endif
		case svcq2_sound:		//9			// <see code>
			CLQ2_ParseStartSoundPacket();
			break;
		case svcq2_print:		//10			// [qbyte] id [string] null terminated string
			i = MSG_ReadByte ();
			s = MSG_ReadString ();
			if (i != PRINT_CHAT)
				s = TL_Translate(com_language, s);

			CL_ParsePrint(s, i);
			break;
		case svcq2_stufftext:	//11			// [string] stuffed into client's console buffer, should be \n terminated
			s = MSG_ReadString ();
			Con_DPrintf ("stufftext: %s\n", s);
			if (!strncmp(s, "precache", 8))	//big major hack. Q2 uses a command that q1 has as a cvar.
			{	//call the q2 precache function.
				CLQ2_Precache_f();
			}
			else
				Cbuf_AddText (s, RESTRICT_SERVER);	//don't let the local user cheat
			break;
		case svcq2_serverdata:	//12			// [long] protocol ...
			Cbuf_Execute ();		// make sure any stuffed commands are done
			CLQ2_ParseServerData ();
			break;
		case svcq2_configstring:	//13		// [short] [string]
			CLQ2_ParseConfigString();
			break;
		case svcq2_spawnbaseline://14
			CLQ2_ParseBaseline();
			break;
		case svcq2_centerprint:	//15		// [string] to put in center of the screen
			s = TL_Translate(com_language, MSG_ReadString ());
			if (*s == '%')	//grr!
				s = CLQ2EX_ReformatBinds(seat, s, alloca(8192), 8192);

#ifdef PLUGINS
			if (Plug_CenterPrintMessage(s, seat))
#endif
				SCR_CenterPrint (seat, s, false);
			break;
		case svcq2_download:		//16		// [short] size [size bytes]
			CL_ParseDownload(false);
			break;
		case svcq2_playerinfo:	//17			// variable
			Host_EndGame ("CL_ParseServerMessage: svcq2_playerinfo not as part of svcq2_frame");
			return;
		case svcq2_packetentities://18			// [...]
			Host_EndGame ("CL_ParseServerMessage: svcq2_packetentities not as part of svcq2_frame");
			return;
		case svcq2_deltapacketentities://19	// [...]
			Host_EndGame ("CL_ParseServerMessage: svcq2_deltapacketentities not as part of svcq2_frame");
			return;
		case svcq2_frame:			//20 (the bastard to implement.)
			CLQ2_ParseFrame(0);
			break;

		case 21:
			if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2 || cls.protocol_q2 == PROTOCOL_VERSION_Q2PRO)
		//case svcr1q2_zpacket:	//r1q2, just try to ignore it.
				CLQ2_ParseZPacket();
			else if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
			{
		//case svcq2ex_splitclient:
				seat = MSG_ReadByte()-1;
				goto seatedcommand;
			}
			else
				goto isillegible;
			break;
		case 22:
			if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2 || cls.protocol_q2 == PROTOCOL_VERSION_Q2PRO)
		//case svcr1q2_zdownload:
				CL_ParseDownload(true);
			else if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_configblast:
				CLQ2EX_ParseBlast(CLQ2_ParseConfigString);
			else
				goto isillegible;
			break;
		case 23:
			if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2)
		//case svcr1q2_playerupdate:
				CLR1Q2_ParsePlayerUpdate();
			else if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_spawnbaselineblast:
				CLQ2EX_ParseBlast(CLQ2_ParseBaseline);
			else
				goto isillegible;
			break;
		case 24:
			if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2)
		//case svcr1q2_setting:
				CLR1Q2_ParseSetting();
			else if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_levelrestart:
				CLQ2EX_ParseLevelRestart();
			else
				goto isillegible;
			break;
		case 25:
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_danage:
				CLQ2EX_ParseDamage(seat);
			else
				goto isillegible;
			break;
		case 26:
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_locprint:
				CLQ2EX_LocPrint(seat);
			else
				goto isillegible;
			break;
		case 27:
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_fog:
				CLQ2EX_ParseFog();
			else
				goto isillegible;
			break;
		case 28:
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_waiting:
				MSG_ReadByte();	//number of players remaining
			else
				goto isillegible;
			break;
		case 29:
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
			{
		//case svcq2ex_botchat:
				MSG_ReadString();
				MSG_ReadUInt16();
				MSG_ReadString();
			}
			else
				goto isillegible;
			break;
		case 30:
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
			{
			//case svcq2ex_mapmarker:
				vec3_t pos;
				/*id=*/MSG_ReadUInt16();
				/*time=*/MSG_ReadUInt16();
				MSG_ReadPos(pos);
				/*image=*/MSG_ReadUInt16();
				/*paltint=*/MSG_ReadByte();
				/*flags=*/MSG_ReadByte();
			}
			else
				goto isillegible;
			break;
		case 31:
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_routemarker:
				CLQ2EX_ParseRouteMarker();
			else
				goto isillegible;
			break;
		case 32:
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_muzzleflash3:
				CLQ2EX_ParseMuzzleFlash3();
			else
				goto isillegible;
			break;
		case 33:
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		//case svcq2ex_achievement:
				Con_Printf("ACHIEVEMENT! %s\n", MSG_ReadString());
			else
				goto isillegible;
			break;
		}
		last = cmd;
	}
	CL_SetSolidEntities ();

	if (cls.demohadkeyframe)
		CL_WriteDemoMessage(&net_message, startpos);	//FIXME: incomplete frames might be awkward
}
#endif

#ifdef NQPROT
//Proquake specific stuff
#define pqc_nop			1
#define pqc_new_team	2
#define pqc_erase_team	3
#define pqc_team_frags	4
#define	pqc_match_time	5
#define pqc_match_reset	6
#define pqc_ping_times	7
static int MSG_ReadBytePQ (char **s)
{
	int ret = (*s)[0] * 16 + (*s)[1] - 272;
	*s+=2;
	return ret;
}
static int MSG_ReadShortPQ (char **s)
{
	return MSG_ReadBytePQ(s) * 256 + MSG_ReadBytePQ(s);
}
static char *CLNQ_ParseProQuakeMessage (char *s)
{
	int cmd;
	int ping;
	int team, shirt, frags;

	s++;
	cmd = *s++;

	switch (cmd)
	{
	default:
		Con_DPrintf("Unrecognised ProQuake Message %i\n", cmd);
		break;
	case pqc_new_team:
		cl.teamplay = true;
		team = MSG_ReadBytePQ(&s) - 16;
		shirt = MSG_ReadBytePQ(&s) - 16;
		Sbar_PQ_Team_New(team, shirt);
		break;

	case pqc_erase_team:
		team = MSG_ReadBytePQ(&s) - 16;
		Sbar_PQ_Team_New(team, 0);
		Sbar_PQ_Team_Frags(team, 0);
		break;

	case pqc_team_frags:
		team = MSG_ReadBytePQ(&s) - 16;
		frags = MSG_ReadShortPQ(&s);
		if (frags & 32768)
			frags = frags - 65536;
		Sbar_PQ_Team_Frags(team, frags);
		break;

	case pqc_match_time:
		cl.matchgametimestart = MSG_ReadBytePQ(&s)*60;
		cl.matchgametimestart += MSG_ReadBytePQ(&s);
		cl.matchgametimestart = cl.gametime - cl.matchgametimestart;
		break;

	case pqc_match_reset:
		Sbar_PQ_Team_Reset();
		break;

	case pqc_ping_times:
		cl.last_ping_request = realtime;
		while ((ping = MSG_ReadShortPQ(&s)))
		{
			if ((ping / 4096) >= MAX_CLIENTS)
				Host_Error ("CL_ParseProQuakeMessage: pqc_ping_times > MAX_CLIENTS");
			cl.players[ping / 4096].ping = ping & 4095;
		}
		break;
	}
	return s;
}

static qboolean CLNQ_ParseNQPrints(char *s)
{
	int i;
	char *start = s;
	if (!strcmp(s, "Client ping times:\n") && !cls.qex)
	{
		cl.nqparseprint = CLNQPP_PINGS;
		return true;
	}
	else if (cl.nqparseprint == CLNQPP_PINGS)
	{
		char *pingstart;
		cl.nqparseprint = CLNQPP_NONE;
		while(*s == ' ')
			s++;
		pingstart = s;
		if (*s == '-')
			s++;
		if (*s >= '0' && *s <= '9')
		{
			while(*s >= '0' && *s <= '9')
				s++;
			if (*s == ' ' && s-start >= 3)
			{
				s++;
				start = s;
				s = strchr(s, '\n');
				if (!s)
					return false;
				*s = 0;

				for (i = 0; i < cl.allocated_client_slots; i++)
				{
					if (!strcmp(start, cl.players[i].name))
						break;
				}
				if (i == cl.allocated_client_slots)
				{

				}
				if (i != cl.allocated_client_slots)
				{
					cl.players[i].ping = atoi(pingstart);
				}
				cl.nqparseprint = CLNQPP_PINGS;
				return true;
			}
		}

		s = start;
	}

	if (!strncmp(s, "host:    ", 9) && !cls.qex)
	{
		cl.nqparseprint = CLNQPP_STATUS;
		return cls.nqexpectingstatusresponse;
	}
	else if (cl.nqparseprint == CLNQPP_STATUS)
	{
		if (!strncmp(s, "players: ", 9))
		{
			cl.nqparseprint = CLNQPP_STATUSPLAYER;
			return cls.nqexpectingstatusresponse;
		}
		else if (strchr(s, ':'))
			return cls.nqexpectingstatusresponse;
		
		cl.nqparseprint = CLNQPP_NONE;	//error of some kind...
		cls.nqexpectingstatusresponse = false;
	}
	if (cl.nqparseprint == CLNQPP_STATUSPLAYER)
	{
		if (*s == '#')
		{
			cl.nqparseprint = CLNQPP_STATUSPLAYERIP;
			cl.nqparseprintplayer = atoi(s+1)-1;
			if (cl.nqparseprintplayer >= 0 && cl.nqparseprintplayer < cl.allocated_client_slots)
				return cls.nqexpectingstatusresponse;
		}
		cl.nqparseprint = CLNQPP_NONE;	//error of some kind...
		cls.nqexpectingstatusresponse = false;
	}
	if (cl.nqparseprint == CLNQPP_STATUSPLAYERIP)
	{
		if (!strncmp(s, "   ", 3))
		{
			while(*s == ' ')
				s++;
			COM_ParseOut(s, cl.players[cl.nqparseprintplayer].ip, sizeof(cl.players[cl.nqparseprintplayer].ip));
			IPLog_Add(cl.players[cl.nqparseprintplayer].ip, cl.players[cl.nqparseprintplayer].name);
			if (*cl.players[cl.nqparseprintplayer].ip != '[' && *cl.players[cl.nqparseprintplayer].ip < '0' && *cl.players[cl.nqparseprintplayer].ip > '9')
				*cl.players[cl.nqparseprintplayer].ip = 0;	//non-numeric addresses are not useful.
			cl.nqparseprint = CLNQPP_STATUSPLAYER;
			return cls.nqexpectingstatusresponse;
		}
		cl.nqparseprint = CLNQPP_NONE;	//error of some kind...
		cls.nqexpectingstatusresponse = false;
	}

	return false;
}

static void CLNQ_CheckPlayerIsSpectator(unsigned int i)
{
	cl.players[i].spectator =
		(cl.players[i].frags==-999) ||	//DP mods tend to use -999
		(cl.players[i].frags==-99);	//crmod uses -99 for spectators, which is annoying.
	//we can't add any colour checks, as apparently this fucks up too.

	if (!*cl.players[i].name)
		cl.players[i].spectator = false;
}

#ifdef HEXEN2
#define svch2_particle2 34
#define svch2_cutscene 35
#define svch2_midiname 36
#define svch2_updateclass 37
#define svch2_particle3 38
#define svch2_particle4 39
#define svch2_setviewflags 40
#define svch2_clearviewflags 41
#define svch2_starteffect 42
#define svch2_endeffect 43
#define svch2_plaque 44
#define svch2_particleexplosion 45
#define svch2_setviewtint 46
#define svch2_ref 47
#define svch2_clearedicts 48
#define svch2_updateinv 49
#define svch2_setanglelerp 50
#define svch2_updatekoth 51
#define svch2_togglestatbar 52
#define svch2_soundpos 53
static qboolean CLH2_ParseServerSubMessage (int cmd)
{
	const int destsplit = 0;
	unsigned int u;
	int i,j;
	const int svch2_first = svch2_particle2;
	static const char *svc_h2strings[] =
	{
		"h2_particle2",
		"h2_cutscene",
		"h2_midiname",
		"h2_updateclass",
		"h2_particle3",
		"h2_particle4",
		"h2_setviewflags",
		"h2_clearviewflags",
		"h2_starteffect",
		"h2_endeffect",
		"h2_plaque",
		"h2_particleexplosion",
		"h2_setviewtint",
		"h2_ref",
		"h2_clearedicts",
		"h2_updateinv",
		"h2_setanglelerp",
		"h2_updatekoth",
		"h2_togglestatbar",
		"h2_soundpos",
	};
	//no fastupdate checks needed here.
	SHOWNET2((cmd>=svch2_first&&cmd<svch2_first+countof(svc_h2strings))?svc_h2strings[cmd-svch2_first]:svc_nqstrings[cmd>(sizeof(svc_nqstrings)/sizeof(char*))?0:cmd], cmd);

	switch (cmd)
	{
	case svch2_midiname:
		MSG_ReadString();
		break;
	case svch2_particle2:
		CL_ParseParticleEffect2 ();
		break;
	case svch2_particle3:
		CL_ParseParticleEffect3 ();
		break;
	case svch2_particle4:
		CL_ParseParticleEffect4 ();
		break;
	case svch2_starteffect:
		i = MSG_ReadByte(); //handle
		j = MSG_ReadByte();	//type
		switch(j)
		{
		case 1 /*ce_rain*/:
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadShort();
			MSG_ReadShort();
			MSG_ReadFloat();
			break;
		case 4/*ce_white_smoke*/:
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadFloat();
			MSG_ReadFloat();
			MSG_ReadFloat();
			MSG_ReadFloat();
			MSG_ReadFloat();
			break;
		case 5/*ce_bluespark*/:
		case 6/*ce_yellowspark*/:
		case 9/*ce_sm_white_flash*/:
		case 13/*ce_sm_blue_flash*/:
		case 15/*ce_sm_explosion*/:
		case 16/*ce_lg_explosion*/:
		case 17/*ce_floor_explosion*/:
		case 19/*ce_blue_explosion*/:
		case 32/*ce_xbow_explosion*/:
		case 33/*ce_new_explosion*/:
		case 34/*ce_magic_missile_explosion*/:
		case 38/*ce_teleporterpuffs*/:
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			break;
		case 39/*ce_teleporterbody*/:
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadFloat();
			MSG_ReadFloat();
			MSG_ReadFloat();
			MSG_ReadFloat();
			break;
		case 40/*ce_boneshard*/:
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();

			MSG_ReadFloat();
			MSG_ReadFloat();
			MSG_ReadFloat();

			MSG_ReadFloat();
			MSG_ReadFloat();
			MSG_ReadFloat();

			MSG_ReadFloat();
			MSG_ReadFloat();
			MSG_ReadFloat();
			break;
		case 43/*ce_snow*/:
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadByte();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadByte();
			break;
		case 55/*ce_chunk*/:
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadByte();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadCoord();
			MSG_ReadByte();
			break;
		default:
			Host_EndGame ("svch2_starteffect: unknown type %i", j);
			return false;
		}
		break;
	case svch2_endeffect:
		MSG_ReadByte();	//handle
		break;
	case svch2_updateclass:
		u = MSG_ReadPlayer();
		j = MSG_ReadByte();
		if (u < MAX_CLIENTS)
			InfoBuf_SetValueForKey(&cl.players[u].userinfo, "cl_playerclass", va("%i", j));
		break;
	case svch2_updateinv:
		cmd = MSG_ReadByte();
		i = j = 0;
		if (cmd&(1<<0))	i |= MSG_ReadByte()<<0;
		if (cmd&(1<<1))	i |= MSG_ReadByte()<<8;
		if (cmd&(1<<2))	i |= MSG_ReadByte()<<16;
		if (cmd&(1<<3))	i |= MSG_ReadByte()<<24;
		if (cmd&(1<<4))	j |= MSG_ReadByte()<<0;
		if (cmd&(1<<5))	j |= MSG_ReadByte()<<8;
		if (cmd&(1<<6))	j |= MSG_ReadByte()<<16;
		if (cmd&(1<<7))	j |= MSG_ReadByte()<<24;
		if (i & (1u<< 0)) CL_SetStatInt(destsplit, STAT_HEALTH, MSG_ReadShort());
		if (i & (1u<< 1)) CL_SetStatInt(destsplit, STAT_H2_LEVEL, MSG_ReadByte());
		if (i & (1u<< 2)) CL_SetStatInt(destsplit, STAT_H2_INTELLIGENCE, MSG_ReadByte());
		if (i & (1u<< 3)) CL_SetStatInt(destsplit, STAT_H2_WISDOM, MSG_ReadByte());
		if (i & (1u<< 4)) CL_SetStatInt(destsplit, STAT_H2_STRENGTH, MSG_ReadByte());
		if (i & (1u<< 5)) CL_SetStatInt(destsplit, STAT_H2_DEXTERITY, MSG_ReadByte());
		if (i & (1u<< 6)) CL_SetStatInt(destsplit, STAT_ACTIVEWEAPON, MSG_ReadByte());
		if (i & (1u<< 7)) CL_SetStatInt(destsplit, STAT_H2_BLUEMANA, MSG_ReadByte());
		if (i & (1u<< 8)) CL_SetStatInt(destsplit, STAT_H2_GREENMANA, MSG_ReadByte());
		if (i & (1u<< 9)) CL_SetStatInt(destsplit, STAT_H2_EXPERIENCE, MSG_ReadLong());
		if (i & (1u<<10)) CL_SetStatInt(destsplit, STAT_H2_CNT_TORCH, MSG_ReadByte());
		if (i & (1u<<11)) CL_SetStatInt(destsplit, STAT_H2_CNT_H_BOOST, MSG_ReadByte());
		if (i & (1u<<12)) CL_SetStatInt(destsplit, STAT_H2_CNT_SH_BOOST, MSG_ReadByte());
		if (i & (1u<<13)) CL_SetStatInt(destsplit, STAT_H2_CNT_MANA_BOOST, MSG_ReadByte());
		if (i & (1u<<14)) CL_SetStatInt(destsplit, STAT_H2_CNT_TELEPORT, MSG_ReadByte());
		if (i & (1u<<15)) CL_SetStatInt(destsplit, STAT_H2_CNT_TOME, MSG_ReadByte());
		if (i & (1u<<16)) CL_SetStatInt(destsplit, STAT_H2_CNT_SUMMON, MSG_ReadByte());
		if (i & (1u<<17)) CL_SetStatInt(destsplit, STAT_H2_CNT_INVISIBILITY, MSG_ReadByte());
		if (i & (1u<<18)) CL_SetStatInt(destsplit, STAT_H2_CNT_GLYPH, MSG_ReadByte());
		if (i & (1u<<19)) CL_SetStatInt(destsplit, STAT_H2_CNT_HASTE, MSG_ReadByte());
		if (i & (1u<<20)) CL_SetStatInt(destsplit, STAT_H2_CNT_BLAST, MSG_ReadByte());
		if (i & (1u<<21)) CL_SetStatInt(destsplit, STAT_H2_CNT_POLYMORPH, MSG_ReadByte());
		if (i & (1u<<22)) CL_SetStatInt(destsplit, STAT_H2_CNT_FLIGHT, MSG_ReadByte());
		if (i & (1u<<23)) CL_SetStatInt(destsplit, STAT_H2_CNT_CUBEOFFORCE, MSG_ReadByte());
		if (i & (1u<<24)) CL_SetStatInt(destsplit, STAT_H2_CNT_INVINCIBILITY, MSG_ReadByte());
		if (i & (1u<<25)) CL_SetStatFloat(destsplit, STAT_H2_ARTIFACT_ACTIVE, MSG_ReadFloat());
		if (i & (1u<<26)) CL_SetStatFloat(destsplit, STAT_H2_ARTIFACT_LOW, MSG_ReadFloat());
		if (i & (1u<<27)) CL_SetStatInt(destsplit, STAT_H2_MOVETYPE, MSG_ReadByte());
		if (i & (1u<<28)) CL_SetStatInt(destsplit, STAT_H2_CAMERAMODE, MSG_ReadByte());
		if (i & (1u<<29)) CL_SetStatFloat(destsplit, STAT_H2_HASTED, MSG_ReadFloat());
		if (i & (1u<<30)) CL_SetStatInt(destsplit, STAT_H2_INVENTORY, MSG_ReadByte());
		if (i & (1u<<31)) CL_SetStatFloat(destsplit, STAT_H2_RINGS_ACTIVE, MSG_ReadFloat());
		if (j & (1u<< 0)) CL_SetStatFloat(destsplit, STAT_H2_RINGS_LOW, MSG_ReadFloat());
		if (j & (1u<< 1)) CL_SetStatInt(destsplit, STAT_H2_ARMOUR1, MSG_ReadByte());
		if (j & (1u<< 2)) CL_SetStatInt(destsplit, STAT_H2_ARMOUR2, MSG_ReadByte());
		if (j & (1u<< 3)) CL_SetStatInt(destsplit, STAT_H2_ARMOUR3, MSG_ReadByte());
		if (j & (1u<< 4)) CL_SetStatInt(destsplit, STAT_H2_ARMOUR4, MSG_ReadByte());
		if (j & (1u<< 5)) CL_SetStatInt(destsplit, STAT_H2_FLIGHT_T, MSG_ReadByte());
		if (j & (1u<< 6)) CL_SetStatInt(destsplit, STAT_H2_WATER_T, MSG_ReadByte());
		if (j & (1u<< 7)) CL_SetStatInt(destsplit, STAT_H2_TURNING_T, MSG_ReadByte());
		if (j & (1u<< 8)) CL_SetStatInt(destsplit, STAT_H2_REGEN_T, MSG_ReadByte());
		if (j & (1u<< 9)) /*CL_SetStatFloat(destsplit, STAT_H2_HASTE_T,*/( MSG_ReadFloat());
		if (j & (1u<<10)) /*CL_SetStatFloat(destsplit, STAT_H2_TOMB_T,*/( MSG_ReadFloat());
		if (j & (1u<<11)) CL_SetStatString(destsplit, STAT_H2_PUZZLE1, MSG_ReadString());
		if (j & (1u<<12)) CL_SetStatString(destsplit, STAT_H2_PUZZLE2, MSG_ReadString());
		if (j & (1u<<13)) CL_SetStatString(destsplit, STAT_H2_PUZZLE3, MSG_ReadString());
		if (j & (1u<<14)) CL_SetStatString(destsplit, STAT_H2_PUZZLE4, MSG_ReadString());
		if (j & (1u<<15)) CL_SetStatString(destsplit, STAT_H2_PUZZLE5, MSG_ReadString());
		if (j & (1u<<16)) CL_SetStatString(destsplit, STAT_H2_PUZZLE6, MSG_ReadString());
		if (j & (1u<<17)) CL_SetStatString(destsplit, STAT_H2_PUZZLE7, MSG_ReadString());
		if (j & (1u<<18)) CL_SetStatString(destsplit, STAT_H2_PUZZLE8, MSG_ReadString());
		if (j & (1u<<19)) CL_SetStatInt(destsplit, STAT_H2_MAXHEALTH, MSG_ReadShort());
		if (j & (1u<<20)) CL_SetStatInt(destsplit, STAT_H2_MAXMANA, MSG_ReadByte());
		if (j & (1u<<21)) CL_SetStatFloat(destsplit, STAT_H2_FLAGS, MSG_ReadFloat());
		if (j & (1u<<22)) CL_SetStatInt(destsplit, STAT_H2_OBJECTIVE1, MSG_ReadLong());
		if (j & (1u<<23)) CL_SetStatInt(destsplit, STAT_H2_OBJECTIVE2, MSG_ReadLong());
		//if (j & (1u<<24)) CL_SetStat(destsplit, STAT_H2_, MSG_Read());
		//if (j & (1u<<25)) CL_SetStat(destsplit, STAT_H2_, MSG_Read());
		//if (j & (1u<<26)) CL_SetStat(destsplit, STAT_H2_, MSG_Read());
		//if (j & (1u<<27)) CL_SetStat(destsplit, STAT_H2_, MSG_Read());
		//if (j & (1u<<28)) CL_SetStat(destsplit, STAT_H2_, MSG_Read());
		//if (j & (1u<<29)) CL_SetStat(destsplit, STAT_H2_, MSG_Read());
		//if (j & (1u<<30)) CL_SetStat(destsplit, STAT_H2_, MSG_Read());
		//if (j & (1u<<31)) CL_SetStat(destsplit, STAT_H2_, MSG_Read());
		break;
	case svch2_ref:
		if (cls.signon == 4 - 1)
		{	// first update is the final signon stage
			cls.signon = 4;
			CLNQ_SignonReply ();
		}
		CLH2_ParseEntities();
		break;
	case svch2_clearedicts:	//should have been handled by CLH2_ParseEntities (this also applies to fastupdates)
		Host_EndGame ("CLNQ_ParseServerMessage: Unexpected server message (%i@%i)", cmd, MSG_GetReadCount()-1);
		return false;
	case svch2_togglestatbar:
		break;	//wtf
	case svch2_setanglelerp:	//urgh... demo playback has its own angle values, and with greater precision.
		MSG_ReadAngle();
		MSG_ReadAngle();
		MSG_ReadAngle();
		break;
	case svch2_cutscene:
	case svch2_setviewflags:
	case svch2_clearviewflags:
	case svch2_plaque:
	case svch2_particleexplosion:
	case svch2_setviewtint:
	case svch2_updatekoth:
	case svch2_soundpos:
		//we're only aiming for demo compat, so we ignore this tat.
		Host_EndGame ("CLH2_ParseServerMessage: Unimplemented server message (%i@%i)", cmd, MSG_GetReadCount()-1);
		return false;
	default:
		return false;
	}
	return true;	//handled.
}
#endif


void CLNQ_ParseServerMessage (void)
{
	const int	destsplit = 0;
	int			cmd;
	char		*s;
	unsigned int u;
	int			i, j;
	vec3_t		ang;
	unsigned int cmdstart;

//	cl.last_servermessage = realtime;
	CL_ClearProjectiles ();

//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1)
		Con_Printf ("%i ",net_message.cursize);
	else if (cl_shownet.value >= 2)
		Con_Printf ("------------------\n");

//
// parse the message
//
	while (1)
	{
		if (msg_badread)
		{
			CL_DumpPacket();
			Host_EndGame ("CL_ParseServerMessage: Bad server message");
			break;
		}

		cmdstart = MSG_GetReadCount();
		cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			SHOWNETEOM("END OF MESSAGE");
			break;
		}

#ifdef HEXEN2
		if (cls.protocol_nq == CPNQ_H2MP)
		{
			if (CLH2_ParseServerSubMessage(cmd))
			{
				packetusage_pending[cmd] += MSG_GetReadCount()-cmdstart;
				continue;
			}
			//handle as a regular nq packet.
		}
		else
#endif
		{
			if (cmd & 128)
			{
				SHOWNET("fast update");
				CLNQ_ParseEntity(cmd&127);
				packetusage_pending[128] += MSG_GetReadCount()-cmdstart;
				continue;
			}

			SHOWNET2(svc_nqstrings[cmd>(sizeof(svc_nqstrings)/sizeof(char*))?0:cmd], cmd);
		}

	// other commands
		switch (cmd)
		{
		default:
		badsvc:
		case svc_bad:
			CL_DumpPacket();
			Host_EndGame ("CLNQ_ParseServerMessage: Illegible server message (%i@%i)", cmd, MSG_GetReadCount()-1);
			return;

		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;

		case svc_print:
			s = MSG_ReadString ();
			//fallthrough...
		svcprint:
			if (*s == 1 || *s == 2)
			{
				//FIXME: should be using the first char of the line, not the first char of the last segment.
				CL_ParsePrint(s+1, PRINT_CHAT);
			}
			else if (CLNQ_ParseNQPrints(s))
				break;
			else
				CL_ParsePrint(s, PRINT_HIGH);
			break;

		case svc_disconnect:
			CL_Disconnect(cls.demoplayback?NULL:"Server disconnected");	//don't show any errors on end-of-demo.
			CL_NextDemo();
			return;

		case svc_centerprint:
			s = MSG_ReadString ();
			svccentreprint:
#ifdef PLUGINS
			if (Plug_CenterPrintMessage(s, destsplit))
#endif
				SCR_CenterPrint (destsplit, s, false);
			break;

		case svc_stufftext:
			s = MSG_ReadString ();
			CL_ParseStuffCmd(s, destsplit);
			break;

		case svc_version:
			CLNQ_ParseProtoVersion();
			break;
		case svc_serverdata:
			if (*printtext)
			{	//work around a missing-eol issue.
				CL_PrintStandardMessage(printtext, PRINT_HIGH);
				printtext[0] = 0;
			}
			Cbuf_Execute ();		// make sure any stuffed commands are done
			CLNQ_ParseServerData ();
			break;

		case svcdp_precache:
		//also svcqex_levelcompleted
			if (cls.qex)
			{	//svcqex_levelcompleted
				//not really sure why this even exists.
				MSG_ReadSkip(10);
				MSG_ReadString();
				break;
			}
			CL_ParsePrecache();
			break;

		case svc_cdtrack:
			{
				unsigned int firsttrack;
				unsigned int looptrack;
				firsttrack = MSG_ReadByte ();
				looptrack = MSG_ReadByte ();

				if (cls.demotrack != -1)
					firsttrack = looptrack = cls.demotrack;

				Media_NumberedTrack (firsttrack, looptrack);
			}
			break;

		case svc_setview:
			i=MSGCL_ReadEntity();
			if (!cl.playerview[destsplit].viewentity)
			{
				if (!i || i > cl.allocated_client_slots)
					cl.playerview[destsplit].playernum = cl.allocated_client_slots;	//the mvd spectator slot.
				else
					cl.playerview[destsplit].playernum = (unsigned int)i-1;
			}
			cl.playerview[destsplit].viewentity = i;
			break;

		case svcnq_signonnum:
			i = MSG_ReadByte ();

			if (i <= cls.signon)
				Host_EndGame ("Received signon %i when at %i", i, cls.signon);
			cls.signon = i;
			CLNQ_SignonReply ();
			break;
		case svc_setpause:
			cl.paused = MSG_ReadByte ();
//			Media_SetPauseTrack(!!cl.paused);
			break;

		case svc_spawnstaticsound:
			CL_ParseStaticSound (false);
			break;

		case svc_spawnstatic:
			CL_ParseStaticProt (CPNQ_ID);
			break;

		case svc_spawnbaseline:
			i = MSGCL_ReadEntity ();
			if (!CL_CheckBaselines(i))
				Host_EndGame("CLNQ_ParseServerMessage: svc_spawnbaseline failed with size %i", i);
			CL_ParseBaseline (cl_baselines + i, CPNQ_ID);
			break;

		//PEXT_REPLACEMENTDELTAS
		case svcfte_updateentities:
			if (cls.signon == 4 - 1)
			{	// first update is the final signon stage
				cls.signon = 4;
				CLNQ_SignonReply ();
			}
			CL_ParseClientdata ();
			CLFTE_ParseEntities();
			break;
		case svcfte_spawnstatic2:
			CL_ParseStaticProt (-1);
			break;
		case svcfte_spawnbaseline2:
			CL_ParseBaselineDelta ();
			break;

		case svcfte_cgamepacket_sized:
#ifdef CSQC_DAT
			if (CSQC_ParseGamePacket(destsplit, true))
				break;
#endif
			Con_Printf("Unable to parse gamecode packet\n");
			break;
		case svcfte_cgamepacket:
#ifdef HLCLIENT
			if (CLHL_ParseGamePacket())
				break;
#endif
#ifdef CSQC_DAT
			if (CSQC_ParseGamePacket(destsplit, cl.csqcdebug))
				break;
#endif
			Con_Printf("Unable to parse gamecode packet\n");
			break;

		case svc_time:
			CL_ParseClientdata ();

			//fixme: move this stuff to a common place
//			cl.playerview[destsplit].oldfixangle = cl.playerview[destsplit].fixangle;
//			VectorCopy(cl.playerview[destsplit].fixangles, cl.playerview[destsplit].oldfixangles);
//			cl.playerview[destsplit].fixangle = FIXANGLE_NO;
			if (cls.demoplayback)
			{
//				extern vec3_t demoangles;
//				cl.playerview[destsplit].fixangle = FIXANGLE_FIXED;
//				VectorCopy(demoangles, cl.playerview[destsplit].fixangles);
			}

			cls.netchan.outgoing_sequence++;
			cls.netchan.incoming_sequence = cls.netchan.outgoing_sequence-1;
			cl.validsequence = cls.netchan.incoming_sequence;

			cl.last_servermessage = realtime;

			cl.oldgametime = cl.gametime;
			cl.oldgametimemark = cl.gametimemark;
			cl.gametime = MSG_ReadFloat();
			cl.gametimemark = realtime;

			if (cls.fteprotocolextensions2 & PEXT2_PREDINFO)
			{
				unsigned int seq = (cl.ackedmovesequence&0xffff0000) | MSG_ReadShort();
				if (seq > cl.ackedmovesequence)
					seq -= 0x10000;	//protect against wraps.
				cl.ackedmovesequence = seq;
			}

			{
				extern vec3_t demoangles;
				int fr = cls.netchan.incoming_sequence&UPDATE_MASK;
				if (cls.demoplayback)
				{
					cl.inframes[fr&UPDATE_MASK].packet_entities.fixangles[destsplit] = true;
					VectorCopy(demoangles, cl.inframes[fr&UPDATE_MASK].packet_entities.fixedangles[destsplit]);
				}
				else
					cl.inframes[fr&UPDATE_MASK].packet_entities.fixangles[destsplit] = false;
			}
			cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK].receivedtime = realtime;
			cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK].frameid = cls.netchan.incoming_sequence;

			if (CPNQ_IS_DP || cls.protocol_nq == CPNQ_H2MP)
			{
				int n = cls.netchan.incoming_sequence&UPDATE_MASK, o = (cls.netchan.incoming_sequence-1)&UPDATE_MASK;
				cl.inframes[n].packet_entities.num_entities = cl.inframes[o].packet_entities.num_entities;
				if (cl.inframes[n].packet_entities.max_entities < cl.inframes[o].packet_entities.num_entities)
				{
					cl.inframes[n].packet_entities.max_entities = cl.inframes[o].packet_entities.max_entities;
					cl.inframes[n].packet_entities.entities = BZ_Realloc(cl.inframes[n].packet_entities.entities, sizeof(entity_state_t) *  cl.inframes[n].packet_entities.max_entities);
				}
				memcpy(cl.inframes[n].packet_entities.entities, cl.inframes[o].packet_entities.entities, sizeof(entity_state_t) * cl.inframes[o].packet_entities.num_entities);
				cl.inframes[n].packet_entities.servertime = cl.inframes[o].packet_entities.servertime;
			}
			else
			{
//				cl.inframes[(cls.netchan.incoming_sequence-1)&UPDATE_MASK].packet_entities = cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities;
				cl.inframes[cl.validsequence&UPDATE_MASK].packet_entities.num_entities=0;
				cl.inframes[cl.validsequence&UPDATE_MASK].packet_entities.servertime = cl.gametime;
			}
#ifdef QUAKESTATS
			for (i = 0; i < cl.splitclients; i++)
			{
				cl.inframes[cl.validsequence&UPDATE_MASK].packet_entities.punchangle[i][0] = cl.playerview[i].statsf[STAT_PUNCHANGLE_X];
				cl.inframes[cl.validsequence&UPDATE_MASK].packet_entities.punchangle[i][1] = cl.playerview[i].statsf[STAT_PUNCHANGLE_Y];
				cl.inframes[cl.validsequence&UPDATE_MASK].packet_entities.punchangle[i][2] = cl.playerview[i].statsf[STAT_PUNCHANGLE_Z];
				cl.inframes[cl.validsequence&UPDATE_MASK].packet_entities.punchorigin[i][0] = cl.playerview[i].statsf[STAT_PUNCHVECTOR_X];
				cl.inframes[cl.validsequence&UPDATE_MASK].packet_entities.punchorigin[i][1] = cl.playerview[i].statsf[STAT_PUNCHVECTOR_Y];
				cl.inframes[cl.validsequence&UPDATE_MASK].packet_entities.punchorigin[i][2] = cl.playerview[i].statsf[STAT_PUNCHVECTOR_Z];
			}
#endif
			break;

		case svc_updatename:
			Sbar_Changed ();
			u = MSG_ReadPlayer ();
			if (u >= MAX_CLIENTS)
				MSG_ReadString();
			else
			{
				strcpy(cl.players[u].name, MSG_ReadString());
				if (*cl.players[u].name)
					cl.players[u].userid = u+1;
				InfoBuf_SetValueForKey(&cl.players[u].userinfo, "name", cl.players[u].name);
				if (!cl.nqplayernamechanged)
					cl.nqplayernamechanged = realtime+2;

				CLNQ_CheckPlayerIsSpectator(u);
			}
			break;

		case svc_updatefrags:
			Sbar_Changed ();
			u = MSG_ReadPlayer ();
			if (u >= MAX_CLIENTS)
				MSG_ReadShort();
			else
			{
				cl.players[u].frags = MSG_ReadShort();
				CLNQ_CheckPlayerIsSpectator(u);
			}
			break;
		case svc_updatecolors:
			{
				int a;
				u = MSG_ReadPlayer ();
				a = MSG_ReadByte ();
				if (u < cl.allocated_client_slots)
				{
//					cl.players[u].rtopcolor = a&0x0f;
//					cl.players[u].rbottomcolor = (a&0xf0)>>4;
//					sprintf(cl.players[u].team, "%2d", cl.players[u].rbottomcolor);

					InfoBuf_SetValueForKey(&cl.players[u].userinfo, "topcolor", va("%i", (a&0xf0)>>4));
					InfoBuf_SetValueForKey(&cl.players[u].userinfo, "bottomcolor", va("%i", (a&0x0f)));
					InfoBuf_SetValueForKey(&cl.players[u].userinfo, "team", va("%i", (a&0x0f)+1));
					CL_ProcessUserInfo (u, &cl.players[u]);

//					CLNQ_CheckPlayerIsSpectator(u);

#ifdef QWSKINS
					if (cls.state == ca_active)
						Skin_Find (&cl.players[u]);
					if (u == cl.playerview[destsplit].playernum)
						Skin_FlushPlayers();
					CL_NewTranslation (u);
#endif
					Sbar_Changed ();
				}
			}
			break;
		case svc_lightstyle:
			i = MSG_ReadByte ();
			if (i >= MAX_NET_LIGHTSTYLES)
			{
				Con_Printf("svc_lightstyle: %i >= MAX_LIGHTSTYLES\n", i);
				MSG_ReadString();
				break;
			}
			R_UpdateLightStyle(i, MSG_ReadString(), 1, 1, 1);
			break;

		case svcnq_updatestatlong:
			i = MSG_ReadByte ();
			j = MSG_ReadLong ();
			CL_SetStatNumeric (0, i, j, j);
			break;
		case svcdp_updatestatbyte:
		//case svcneh_fog:
		//also svcqex_seq
			if (cls.qex)
			{	//svcqex_seq
				unsigned seq = MSG_ReadULEB128();
				if (!cls.demoplayback && 0)
					CL_AckedInputFrame(cls.netchan.incoming_sequence, seq, true);
				break;
			}
			if (CPNQ_IS_BJP || cls.protocol_nq == CPNQ_NEHAHRA)
			{
				CL_ResetFog(FOGTYPE_AIR);
				if (MSG_ReadByte())
				{
					cl.fog[FOGTYPE_AIR].density = MSG_ReadFloat();
					cl.fog[FOGTYPE_AIR].colour[0] = SRGBf(MSG_ReadByte()/255.0f);
					cl.fog[FOGTYPE_AIR].colour[1] = SRGBf(MSG_ReadByte()/255.0f);
					cl.fog[FOGTYPE_AIR].colour[2] = SRGBf(MSG_ReadByte()/255.0f);
					cl.fog[FOGTYPE_AIR].time += 0.25;	//change fairly fast, but not instantly
				}
				cl.fog_locked = !!cl.fog[FOGTYPE_AIR].density;
			}
			else
			{
				i = MSG_ReadByte ();
				j = MSG_ReadByte ();
				CL_SetStatNumeric (0, i, j, j);
			}
			break;
		case svcfte_updatestatstring:
			i = MSG_ReadByte();
			s = MSG_ReadString();
			CL_SetStatString (destsplit, i, s);
			break;
		case svcfte_updatestatfloat:
			i = MSG_ReadByte();
			{
			float f = MSG_ReadFloat();
			CL_SetStatNumeric (destsplit, i, f, f);
			}
			break;

		case svcfte_setanglebase:
			CL_ParseBaseAngle(destsplit);
			break;
		case svcfte_setangledelta:
			for (i=0 ; i<3 ; i++)
				ang[i] = cl.playerview[destsplit].viewangles[i] + MSG_ReadAngle16 ();
			if (!CSQC_Parse_SetAngles(destsplit, ang, true))
				VectorCopy (ang, cl.playerview[destsplit].viewangles);
			VectorCopy (cl.playerview[destsplit].viewangles, cl.playerview[destsplit].simangles);
			VectorCopy (cl.playerview[destsplit].viewangles, cl.playerview[destsplit].intermissionangles);
			break;
		case svc_setangle:
			{
				inframe_t *inf = &cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK];
				if (cls.ezprotocolextensions1 & EZPEXT1_SETANGLEREASON)
					MSG_ReadByte();	//0=unknown, 1=tele, 2=spawn
				for (i=0 ; i<3 ; i++)
					ang[i] = MSG_ReadAngle();
				if (!CSQC_Parse_SetAngles(destsplit, ang, false))
				{
					inf->packet_entities.fixangles[destsplit] = true;
					VectorCopy (ang, cl.playerview[destsplit].viewangles);
					VectorCopy (ang, inf->packet_entities.fixedangles[destsplit]);
				}
				VectorCopy (cl.playerview[destsplit].viewangles, cl.playerview[destsplit].intermissionangles);
				VRUI_SnapAngle();
			}
			break;

		case svcnq_clientdata:
			CLNQ_ParseClientdata ();
			break;

		case svc_sound:
			CLNQ_ParseStartSoundPacket();
			break;
		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i>>3, i&7);
			break;

#ifdef PEXT2_VOICECHAT
		case svcfte_voicechat:
			S_Voip_Parse();
			break;
#endif

		case svc_temp_entity:
			CL_ParseTEnt (true);
			break;
		case svcfte_temp_entity_sized:
			CL_ParseTEnt_Sized();
			break;

		case svc_particle:
			CLNQ_ParseParticleEffect ();
			break;

		case svc_killedmonster:
			cl.playerview[destsplit].stats[STAT_MONSTERS]++;
			cl.playerview[destsplit].statsf[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.playerview[destsplit].stats[STAT_SECRETS]++;
			cl.playerview[destsplit].statsf[STAT_SECRETS]++;
			break;

		case svc_intermission:
			if (cl.intermissionmode == IM_NONE)
			{
				TP_ExecTrigger ("f_mapend", false);
				if (cl.playerview[destsplit].spectator || cls.demoplayback)
					TP_ExecTrigger ("f_specmapend", true);
				else
					TP_ExecTrigger ("f_playmapend", true);
				cl.completed_time = cl.gametime;
			}
			cl.intermissionmode = IM_NQSCORES;

			if (cls.demoseeking == DEMOSEEK_INTERMISSION)
			{
				cls.demoseeking = DEMOSEEK_NOT; //reached it.
				//FIXME: pause it.
			}
			break;

		case svc_finale:
			if (cl.intermissionmode == IM_NONE)
			{
				TP_ExecTrigger ("f_mapend", false);
				if (cl.playerview[destsplit].spectator || cls.demoplayback)
					TP_ExecTrigger ("f_specmapend", true);
				else
					TP_ExecTrigger ("f_playmapend", true);
				cl.completed_time = cl.gametime;
			}
			cl.intermissionmode = IM_NQFINALE;
			SCR_CenterPrint (destsplit, MSG_ReadString (), false);
			break;

		case svc_cutscene:
			if (cl.intermissionmode == IM_NONE)
			{
				TP_ExecTrigger ("f_mapend", false);
				if (cl.playerview[destsplit].spectator || cls.demoplayback)
					TP_ExecTrigger ("f_specmapend", true);
				else
					TP_ExecTrigger ("f_playmapend", true);
				cl.completed_time = cl.gametime;
			}
			cl.intermissionmode = IM_NQCUTSCENE;
			SCR_CenterPrint (destsplit, MSG_ReadString (), false);
			break;

		case svc_sellscreen:	//pantsie
			Cmd_ExecuteString ("help 0", RESTRICT_SERVER);
			break;

		case svc_damage:
			V_ParseDamage (&cl.playerview[destsplit]);
			break;

		case svcfitz_skybox:
			R_SetSky(MSG_ReadString());
			break;
		case svcfitz_bf:
			Cmd_ExecuteString("bf", RESTRICT_SERVER);
			break;
		case svcfitz_fog:
			CL_ResetFog(FOGTYPE_AIR);
			cl.fog[FOGTYPE_AIR].density = MSG_ReadByte()/255.0f;
			cl.fog[FOGTYPE_AIR].colour[0] = SRGBf(MSG_ReadByte()/255.0f);
			cl.fog[FOGTYPE_AIR].colour[1] = SRGBf(MSG_ReadByte()/255.0f);
			cl.fog[FOGTYPE_AIR].colour[2] = SRGBf(MSG_ReadByte()/255.0f);
			cl.fog[FOGTYPE_AIR].time += ((unsigned short)MSG_ReadShort()) / 100.0;
			cl.fog_locked = !!cl.fog[FOGTYPE_AIR].density;
			break;
		case svcfitz_spawnbaseline2:
			i = MSGCL_ReadEntity ();
			if (!CL_CheckBaselines(i))
				Host_EndGame("CLNQ_ParseServerMessage: svcfitz_spawnbaseline2 failed with ent %i", i);
			CL_ParseBaseline (cl_baselines + i, CPNQ_FITZ666);
			break;
		case svcfitz_spawnstatic2:
			CL_ParseStaticProt (CPNQ_FITZ666);
			break;
		case svcfitz_spawnstaticsound2:
			CL_ParseStaticSound(true);
			break;


		case svcnq_effect:
		//also svcqex_achievement
			if (cls.qex)
			{	//svcqex_achievement
				MSG_ReadString();
				break;
			}
#ifdef HAVE_LEGACY
			if (!memcmp(net_message.data+cmdstart+1, "ACH_", 4))
			{	//HIDEOUS UGLY HACK!
				int l = 0;
				char *s = net_message.data+cmdstart+5;
				while (*s)
				{
					if ((*s >= 'A' && *s <= 'Z') || *s == '_')
					{
						s++;
						l++;
					}
					else break;
				}
				if (!*s && l >= 8) //'ACH_PACIFIST' seems the shortest existing one.
				{	//got to the end of the string and found only capitals... good chance its qe debris
					s = MSG_ReadString();
					Con_Printf(CON_WARNING "Got svcnq_effect - assuming stray svcqe_achievement(%s)\n", s);
					break;
				}
			}
#endif
			CL_ParseEffect(false);
			break;
		case svcnq_effect2:
		//also svcqex_chat
			if (cls.qex)
			{	//svcqex_chat
				//in qex this text is in some small special chat box. which disappears quickly rendering its contents kinda unreadable. and its messagemode stuff seems broken too, so whatever. and it seems to have newline issues.
				//FIXME: figure out the player index so we can kickban/mute/etc them.
				qbyte plcolour = MSG_ReadByte();
				qbyte chatcolour = MSG_ReadByte();
				char *pcols[] = {S_COLOR_WHITE,S_COLOR_GREEN,S_COLOR_CYAN, S_COLOR_YELLOW};
				char *ccols[] = {S_COLOR_WHITE,S_COLOR_CYAN};	//say, say_team
				Con_Printf("^[%s%s"/*"\\player\\%i"*/"^]: ", pcols[plcolour%countof(pcols)], MSG_ReadString());
				Con_Printf("%s%s\n", ccols[chatcolour%countof(ccols)], MSG_ReadString());
				break;
			}
			CL_ParseEffect(true);
			break;

		case svcdp_entities:
			if (cls.qex)
			{	//svcqex_prompt
				CLQEX_ParsePrompt();
				break;
			}
			if (cls.signon == 4 - 1)
			{	// first update is the final signon stage
				cls.signon = 4;
				CLNQ_SignonReply ();
			}
			//well, it's really any protocol, but we're only going to support version 5 (through 7).
			CLDP_ParseDarkPlaces5Entities();
			break;
		case svcdp_spawnbaseline2:
			i = MSGCL_ReadEntity ();
			if (!CL_CheckBaselines(i))
				Host_EndGame("CLNQ_ParseServerMessage: svcdp_spawnbaseline2 failed with ent %i", i);
			CL_ParseBaseline (cl_baselines + i, CPNQ_DP5);
			break;

		case svcdp_spawnstatic2:
			CL_ParseStaticProt (CPNQ_DP5);
			break;
		case svcdp_spawnstaticsound2:
			CL_ParseStaticSound(true);
			break;

#ifdef CSQC_DAT
		case svcdp_csqcentities:
			if (cls.qex)
			{
				s = CLQEX_ReadStrings();
				goto svccentreprint;
			}
			CSQC_ParseEntities(false);
			break;
		case svcfte_csqcentities_sized:
			CSQC_ParseEntities(true);
			break;
#endif

		case svcdp_downloaddata:
		//also svcqex_servervars:
			if (cls.qex)
				CLQEX_ParseServerVars();
			else
				CLDP_ParseDownloadData();
			break;

		case svcdp_trailparticles:
			CL_ParseTrailParticles();
			break;
		case svcdp_pointparticles:
			CL_ParsePointParticles(false);
			break;
		case svcdp_pointparticles1:
			CL_ParsePointParticles(true);
			break;

		case svcqex_updateping:
			if (cls.qex)
			{	//svcqex_updateping
				int ping;
				i = MSG_ReadByte();
				ping = MSG_ReadSignedQEX();
				if (i < MAX_CLIENTS)
					cl.players[i].ping = ping;
				break;
			}
			goto badsvc;
		case svcqex_updatesocial:
			if (cls.qex)
			{	//svcqex_updatesocial
				//both ints are -1 for lan/direct clients, and 0 for the host. I guess this is for chatting to people via steam.
				/*slot =*/ MSG_ReadByte();
				/*??? =*/ MSG_ReadLong();
				/*??? =*/ MSG_ReadLong();
				break;
			}
			goto badsvc;

		case svcqex_updateplinfo:
			if (cls.qex)
			{	//svcqex_updateplinfo
				unsigned int slot = MSG_ReadByte();
				int health = MSG_ReadSignedQEX();
				int armour = MSG_ReadSignedQEX();
				if (slot < MAX_CLIENTS)
				{
					InfoBuf_SetValueForKey(&cl.players[slot].userinfo, "health", va("%i", health));
					InfoBuf_SetValueForKey(&cl.players[slot].userinfo, "health", va("%i", armour));
				}
				break;
			}
			goto badsvc;
		case svcqex_locprint:
			if (cls.qex)
			{	//svcqex_'raw'print
				s = CLQEX_ReadStrings();
				goto svcprint;
			}
			goto badsvc;
		}

		packetusage_pending[cmd] += MSG_GetReadCount()-cmdstart;
	}
}
#endif

struct sortedsvcs_s
{
	const char *name;
	size_t bytes;
};
static int QDECL sorttraffic(const void *l, const void *r)
{
	const struct sortedsvcs_s *a=l, *b=r;

	if (a->bytes==b->bytes)
		return 0;
	if (a->bytes>b->bytes)
		return -1;
	return 1;
}
void CL_ShowTrafficUsage(float x, float y)
{
	const char **svcnames, *n;
	size_t svccount, i, j=0;
	size_t total;
	struct sortedsvcs_s sorted[256];
	switch(cls.protocol)
	{
#ifdef NQPROT
	case CP_NETQUAKE:
		svcnames = svc_nqstrings;
		svccount = countof(svc_nqstrings);
		break;
#endif
	case CP_QUAKEWORLD:
		svcnames = svc_qwstrings;
		svccount = countof(svc_qwstrings);
		break;
	default:
		return;	//panic!
	}
	total = 0;
	for (i = 0; i < 256; i++)
		total += packetusage_saved[i];
	for (i = 0; i < 256; i++)
	{
		if (!packetusage_saved[i])
			continue;	//don't show if there's no point.
		if (i < svccount)
			n = svcnames[i];
		else
			n = va("svc %u", (unsigned)i);
		sorted[j].name = n;
		sorted[j].bytes = packetusage_saved[i];
		j++;
	}
	qsort(sorted, j, sizeof(*sorted), sorttraffic);

	for (i = 0; i < j; i++)
	{
		Draw_FunString(x, y, va("%22s:%5.1f%% (%.0f/s)", sorted[i].name, (100.0*sorted[i].bytes)/total, (sorted[i].bytes/packetusage_interval)));
		y+=8;
	}
}
