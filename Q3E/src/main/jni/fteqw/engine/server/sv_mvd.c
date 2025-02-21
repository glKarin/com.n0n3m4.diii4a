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

#include "quakedef.h"
#ifdef MVD_RECORDING
#ifndef CLIENTONLY

#include "winquake.h"
#include "fs.h"

#include "netinc.h"

void SV_MVDStop_f (void);

#define demo_size_padding 0x1000

static void QDECL SV_DemoDir_Callback(struct cvar_s *var, char *oldvalue);

cvar_t	sv_demoAutoRecord = CVARAD("sv_demoAutoRecord", "0", "cl_autodemo", "If set, automatically record demos.\n-1: record on client connection only.\n1+: record once there's this many active players on the server (or if connected to a remote server).");
cvar_t	sv_demoUseCache = CVARD("sv_demoUseCache", "", "If set, demo data will be flushed only periodically");
cvar_t	sv_demoCacheSize = CVAR("sv_demoCacheSize", "0x80000"); //half a meg
cvar_t	sv_demoMaxDirSize = CVARD("sv_demoMaxDirSize", "100mb", "Maximum allowed serverside storage space for mvds. set to blank to remove the limit. New demos cannot be recorded once this size is reached.");	//so ktpro autorecords.
cvar_t	sv_demoMaxDirCount = CVARD("sv_demoMaxDirCount", "500", "Maximum allowed serverside mvds to record. Set to 0 to remove the limit. New demos cannot be recorded once this many demos have already been recorded.");	//so ktpro autorecords.
cvar_t	sv_demoMaxDirAge = CVARD("sv_demoMaxDirAge", "0", "Maximum allowed age for demos, any older demos will be deleted when sv_demoClearOld is set (this doesn't prevent recording new demos).");
cvar_t	sv_demoClearOld = CVARD("sv_demoClearOld", "0", "Automatically delete demos to keep the demos count reasonable.");
cvar_t	sv_demoDir = CVARC("sv_demoDir", "demos", SV_DemoDir_Callback);
cvar_t	sv_demoDirAlt = CVARCD("sv_demoDirAlt", "", SV_DemoDir_Callback, "Provides a fallback directory name for demo downloads, for when sv_demoDir doesn't contain the requested demo.");
cvar_t	sv_demofps = CVAR("sv_demofps", "30");
cvar_t	sv_demoPings = CVARD("sv_demoPings", "10", "Interval between ping updates in mvds");
cvar_t	sv_demoMaxSize = CVARD("sv_demoMaxSize", "", "Demos will be truncated to be no larger than this size.");
cvar_t	sv_demoExtraNames = CVAR("sv_demoExtraNames", "");
cvar_t	sv_demoExtensions = CVARD("sv_demoExtensions", "1", "Enables protocol extensions within MVDs. This will cause older/non-fte clients to error upon playback.\n0: off.\n1: all extensions.\n2: extensions also supported by a certain other engine.");
cvar_t	sv_demoAutoCompress = CVARD("sv_demoAutoCompress", "", "Specifies whether to compress demos as they're recorded.\n0 = no compression.\n1 = gzip compression.");
cvar_t	sv_demo_write_csqc = CVARD("sv_demo_write_csqc", "", "Writes a copy of the csprogs into recorded demos. This ensures that the demo can be played back despite future gamecode changes.");

cvar_t qtv_password		= CVAR(		"qtv_password", "");
cvar_t qtv_maxstreams	= CVARAFD(	"qtv_maxstreams", "0",
									"mvd_maxstreams",  0, "This is the maximum number of QTV clients/proxies that may be directly connected to the server. If empty then there is no limit. 0 disallows any streaming.");

cvar_t			sv_demoAutoPrefix = CVAR("sv_demoAutoPrefix", "auto_");
cvar_t			sv_demoPrefix = CVAR("sv_demoPrefix", "");
cvar_t			sv_demoSuffix = CVAR("sv_demoSuffix", "");
cvar_t			sv_demotxt = CVAR("sv_demotxt", "1");

void SV_WriteMVDMessage (sizebuf_t *msg, int type, int to, float time);
void SV_WriteRecordMVDMessage (sizebuf_t *msg);
#ifdef TCPCONNECT
void tobase64(unsigned char *out, int outlen, unsigned char *in, int inlen);
#endif


static struct
{	//tracks the previously recorded demos, so we don't have to content with dates and filesystem ordering and stuff.
#define DEMOLOG_LENGTH 16
	unsigned int sequence;	//incremented
	struct 
	{
		char filename[MAX_QPATH];
	} log[DEMOLOG_LENGTH];
} demolog;

demo_t			demo;
static float			demo_prevtime;
//static dbuffer_t	*demobuffer;
//static int	header = (char *)&((header_t*)0)->data - (char *)NULL;
static sizebuf_t demomsg;
int demomsgtype;
int demomsgto;
static char demomsgbuf[MAX_OVERALLMSGLEN];

static void SV_MVD_Stopped(void);

static mvddest_t *singledest;	//used when a stream is starting up so redundant data doesn't get dumped into other streams
static struct reversedest_s
{
	struct reversedest_s *next;
	qtvpendingstate_t info;
	vfsfile_t *stream;
	char inbuffer[2048];
	int inbuffersize;
	double timeout;
} *reversedest;	//used when a reverse stream is starting up

static mvddest_t *SV_MVD_InitStream(vfsfile_t *stream, const char *info);
qboolean SV_MVD_Record (mvddest_t *dest);
char *SV_MVDName2Txt(char *name);
extern cvar_t qtv_password;

//does not unlink.
static void DestClose(mvddest_t *d, enum mvdclosereason_e reason)
{
	if (d->desttype == DEST_THREADEDFILE)
	{
		while(d->flushing == true)
			COM_WorkerPartialSync(d, &d->flushing, true);
	}

	if (d->cache)
		BZ_Free(d->cache);
	if (d->file)
	{
		VFS_CLOSE(d->file);
		if (d->desttype != DEST_STREAM)
			FS_FlushFSHashWritten(d->filename);
	}

	if (d->desttype != DEST_STREAM)
	{
		if (reason == MVD_CLOSE_CANCEL)
		{
			FS_Remove(d->filename, FS_GAMEONLY);

			FS_Remove(SV_MVDName2Txt(d->filename), FS_GAMEONLY);

			//SV_BroadcastPrintf (PRINT_CHAT, "Server recording canceled, demo removed\n");
		}
		else
		{
			char buf[512];
			Q_strncpyz(demolog.log[demolog.sequence%DEMOLOG_LENGTH].filename, d->simplename, sizeof(demolog.log[demolog.sequence%DEMOLOG_LENGTH].filename));
			demolog.sequence++;
			SV_BroadcastPrintf (PRINT_CHAT, "Server recording complete\n^[/download %s^]\n", COM_QuotedString(va("demos/%s",d->simplename), buf, sizeof(buf), false));
		}
	}

	Z_Free(d);
}

static void MVD_FlushDest_Flushed(void *ctx, void *data, size_t a, size_t b)
{
	mvddest_t *d = ctx;
	d->flushing = false;
}
static void MVD_FlushDest_Worker(void *ctx, void *data, size_t datasize, size_t b)
{
	mvddest_t *d = ctx;
	int len = VFS_WRITE(d->file, data, datasize);
	VFS_FLUSH(d->file);

	if (len != datasize)
		d->error = true;

	d->altcache = data;
	COM_AddWork(WG_MAIN, MVD_FlushDest_Flushed, d, NULL, 0, 0);
}

void DestFlush(qboolean compleate)
{
	int len;
	mvddest_t *d, *t;

	if (compleate)
	{
		//make sure everything is flushed.
		MVDWrite_Begin(255, -1, 0);
	}

	if (!demo.dest)
		return;
	while (demo.dest->error)
	{
		d = demo.dest;
		demo.dest = d->nextdest;

		DestClose(d, MVD_CLOSE_FSERROR);

		if (!demo.dest)
		{
			SV_MVDStop(MVD_CLOSE_DISCONNECTED, false);
			return;
		}
	}
	for (d = demo.dest; d; d = d->nextdest)
	{
		switch(d->desttype)
		{
		case DEST_FILE:
			VFS_FLUSH (d->file);
			break;
		case DEST_BUFFEREDFILE:
			if (d->cacheused+demo_size_padding > d->maxcachesize || compleate)
			{
				len = VFS_WRITE(d->file, d->cache, d->cacheused);
				if (len < d->cacheused)
					d->error = true;
				VFS_FLUSH(d->file);

				d->cacheused = 0;
			}
			break;
		case DEST_THREADEDFILE:
			if (d->cacheused+demo_size_padding > d->maxcachesize || compleate)
			{
				void *data = d->cache;
				while(d->flushing == true)
					COM_WorkerPartialSync(d, &d->flushing, true);
				d->cache = d->altcache;
				d->altcache = NULL;
				d->flushing = true;
				COM_AddWork(WG_LOADER, MVD_FlushDest_Worker, d, data, d->cacheused, 0);
				d->cacheused = 0;
			}
			break;

		case DEST_STREAM:
			if (d->cacheused && !d->error)
			{
				len = VFS_WRITE(d->file, d->cache, d->cacheused);
				if (len < 0) //client died
					d->error = true;
				else if (len > 0)	//we put some data through
				{	//move up the buffer
					d->cacheused -= len;
					memmove(d->cache, d->cache+len, d->cacheused);
				}
			}
			break;

		case DEST_NONE:
			Sys_Error("DestFlush encoundered bad dest.");
		}

		if (sv_demoMaxSize.value && d->totalsize > sv_demoMaxSize.value*1024)
			d->error = 2;	//abort, but don't kill it.

		while (d->nextdest && d->nextdest->error)
		{
			t = d->nextdest;
			d->nextdest = t->nextdest;

			DestClose(t, MVD_CLOSE_FSERROR);
		}
	}
}

enum qtvstatus_e
{
	QTV_ERROR = -1,	//corrupt/bad request that should be dropped.
	QTV_RETRY = 0,	//still handshaking.
	QTV_ACCEPT = 1	//stream is now owned by the qtv code
};
int SV_MVD_GotQTVRequest(vfsfile_t *clientstream, char *headerstart, char *headerend, qtvpendingstate_t *p)
{
	char *e;

	qboolean server = false;
	char *start, *lineend;
	int versiontouse = 0;
	int raw = 0;
	char password[256] = "";
	char userinfo[1024];
	static struct
	{
		const char *name;	//as seen in protocol
		hashfunc_t *func;
		int base;
	} hashes[] = {
		{"NONE", NULL, -1},			//for annonymous connections
		{"PLAIN", NULL, 0},
//		{"CCITT", &hash_crc16, 16},	//'the CCITT standard CRC used by XMODEM'. 16bit anyway, don't allow, too easy to guess.
//		{"MD4", &hash_md4, 15},		//md4 is available to all QW clients, but probably too weak to really use.
//		{"MD5", &hash_md5, 16},		//blurgh
		{"SHA1", &hash_sha1, 16},
		{"SHA2_256", &hash_sha2_256, 64},
		{"SHA2_512", &hash_sha2_512, 64},
//		{"SHA3_512", &hash_sha3_512, 16},	//eztv apparently allows this
	};
	int authmethod = 0;	//which of the above we're trying to use...

	start = headerstart;

	lineend = strchr(start, '\n');
	if (!lineend)
		return QTV_ERROR;

	*lineend = '\0';
	COM_ParseToken(start, NULL);
	start = lineend+1;
	if (strcmp(com_token, "QTV"))
	{	//it's an error if it's not qtv.
		if (!strcmp(com_token, "QTVSV"))
			server = true;
		else
			return QTV_ERROR;
	}

	if (server != p->isreverse)
	{	//just a small check
		return QTV_ERROR;
	}

	*userinfo = 0;
	for(;;)
	{
		lineend = strchr(start, '\n');
		if (!lineend)
			break;
		*lineend = '\0';
		start = COM_ParseToken(start, NULL);
		if (start && *start == ':')
		{
//VERSION: a list of the different qtv protocols supported. Multiple versions can be specified. The first is assumed to be the prefered version.
//RAW: if non-zero, send only a raw mvd with no additional markup anywhere (for telnet use). Doesn't work with challenge-based auth, so will only be accepted when proxy passwords are not required.
//AUTH: specifies an auth method, the exact specs varies based on the method
//		PLAIN: the password is sent as a PASSWORD line
//		MD4: the server responds with an "AUTH: MD4\n" line as well as a "CHALLENGE: somerandomchallengestring\n" line, the client sends a new 'initial' request with CHALLENGE: MD4\nRESPONSE: hexbasedmd4checksumhere\n"
//		MD5: same as md4
//		CCITT: same as md4, but using the CRC stuff common to all quake engines.
//		if the supported/allowed auth methods don't match, the connection is silently dropped.
//SOURCE: which stream to play from, DEFAULT is special. Without qualifiers, it's assumed to be a tcp address.
//COMPRESSION: Suggests a compression method (multiple are allowed). You'll get a COMPRESSION response, and compression will begin with the binary data.

			start = start+1;
			while(*start == ' ' || *start == '\t')
				start++;
			Con_DPrintf("qtv, got (%s) (%s)\n", com_token, start);
			if (!strcmp(com_token, "VERSION"))
			{
				start = COM_ParseToken(start, NULL);
				if (atoi(com_token) == 1)
					versiontouse = 1;
			}
			else if (!strcmp(com_token, "RAW"))
			{
				start = COM_ParseToken(start, NULL);
				raw = atoi(com_token);
			}
			else if (!strcmp(com_token, "PASSWORD"))
			{
				start = COM_ParseToken(start, NULL);
				Q_strncpyz(password, com_token, sizeof(password));
			}
			else if (!strcmp(com_token, "AUTH"))
			{
				int thisauth;
				start = COM_ParseToken(start, NULL);
				for (thisauth = 1; ; thisauth++)
				{
					if (thisauth == countof(hashes))
					{
						Con_DPrintf("qtv: received unrecognised auth method (%s)\n", com_token);
						break;
					}
					if (!strcmp(com_token, hashes[thisauth].name))
					{	//we know this one.
						if (authmethod < thisauth)
							authmethod = thisauth;	//and its better than the previous one we saw
						break;
					}
				}
			}
			else if (!strcmp(com_token, "SOURCE"))
			{
				//servers don't support source, and ignore it.
				//source is only useful for qtv proxy servers.
			}
			else if (!strcmp(com_token, "COMPRESSION"))
			{
				//compression not supported yet
			}
			else if (!strcmp(com_token, "QTV_EZQUAKE_EXT"))
			{
				//if we were treating this as a regular client over tcp (qizmo...)
			}
			else if (!strcmp(com_token, "USERINFO"))
			{
				//if we were treating this as a regular client over tcp (qizmo...)
				start = COM_ParseTokenOut(start, NULL, userinfo, sizeof(userinfo), &com_tokentype);
			}
			else
			{
				//not recognised.
			}
		}
		start = lineend+1;
	}

	/*len = (headerend - headerstart)+2;
	p->insize -= len;
	memmove(p->inbuffer, p->inbuffer + len, p->insize);
	p->inbuffer[p->insize] = 0;
	*/

	e = NULL;
	if (p->hasauthed)
	{
	}
	else if (p->isreverse)
		p->hasauthed = true;	//reverse connections do not need to auth.
	else if (!*qtv_password.string)
		p->hasauthed = true;	//no password, no need to auth.
	else if (*password)
	{
		char hash[512];
		qbyte digest[DIGEST_MAXSIZE];
		if (!*p->challenge && hashes[authmethod].func)
			e =	("QTVSV 1\n"
				 "PERROR: Challenge wasn't given...\n\n");
		switch(hashes[authmethod].base)
		{
		default:
		case -1:	//no auth at all
			e = ("QTVSV 1\n"
				 "PERROR: You need to provide a password.\n\n");
			break;
		case 0:		//plain text. challenge is not used.
			Q_snprintfz(hash, sizeof(hash), "%s", qtv_password.string);
			break;
		case 15:	//fucked encoding(missing some leading 0s)
			Q_snprintfz(hash, sizeof(hash), "%s%s", p->challenge, qtv_password.string);
			CalcHash(hashes[authmethod].func, digest, sizeof(digest), hash, strlen(hash));
			Q_snprintfz(hash, sizeof(hash), "%X%X%X%X", ((quint32_t*)digest)[0], ((quint32_t*)digest)[1], ((quint32_t*)digest)[2], ((quint32_t*)digest)[3]);
			break;
		case 16:
			Q_snprintfz(hash, sizeof(hash), "%s%s", p->challenge, qtv_password.string);
			CalcHash(hashes[authmethod].func, digest, sizeof(digest), hash, strlen(hash));
			Base16_EncodeBlock(digest, hashes[authmethod].func->digestsize, hash, sizeof(hash));
			break;
		case 64:
			Q_snprintfz(hash, sizeof(hash), "%s%s", p->challenge, qtv_password.string);
			CalcHash(hashes[authmethod].func, digest, sizeof(digest), hash, strlen(hash));
			Base64_EncodeBlock(digest, hashes[authmethod].func->digestsize, hash, sizeof(hash));
			break;
		}
		p->hasauthed = !strcmp(password, hash);
		if (!p->hasauthed && !e)
		{
			if (raw)
				e = "";
			else
				e =	("QTVSV 1\n"
					 "PERROR: Bad password.\n\n");
		}
	}
	else
	{
		//no password, and not automagically authed
		switch (hashes[authmethod].base)
		{
		case -1:
			if (raw)
				e = "";
			else
				e = ("QTVSV 1\n"
					 "PERROR: You need to provide a common auth method.\n\n");
			break;
		case 0:
			p->hasauthed = !strcmp(qtv_password.string, password);
			break;
		default:
			{
				char tmp[32];
				Sys_RandomBytes(tmp, sizeof(tmp));
				Base64_EncodeBlock(tmp, sizeof(tmp), p->challenge, sizeof(p->challenge));
			}

			e = va("QTVSV 1\n"
				"AUTH: %s\n"
				"CHALLENGE: %s\n\n",
					hashes[authmethod].name, p->challenge);
			VFS_WRITE(clientstream, e, strlen(e));
			return QTV_RETRY;
		}
	}

	if (*qtv_maxstreams.string && !p->isreverse)
	{
		int count = 0;
		mvddest_t *dest;
		for (dest = demo.dest; dest; dest = dest->nextdest)
		{
			if (dest->desttype == DEST_STREAM)
				count++;
		}

		if (count >= qtv_maxstreams.value) //sorry
		{
			if (!qtv_maxstreams.value)
				e = "QTVSV 1\nTERROR: QTV streaming from this server is blocked by qtv_maxstreams.\n\n";
			else
				e = "QTVSV 1\nTERROR: This server enforces a limit on the number of proxies connected at any one time. Please try again later.\n\n";
		}
	}

	if (e)
	{
	}
	else if (!versiontouse)
	{
		e =	("QTVSV 1\n"
			 "PERROR: Incompatible version (valid version is v1)\n\n");
	}
	else if (raw)
	{
		if (p->hasauthed == true)
		{
			if (!SV_MVD_Record(SV_MVD_InitStream(clientstream, userinfo)))
				return QTV_ERROR;
			return QTV_ACCEPT;
		}
	}
	else
	{
		if (p->hasauthed == true)
		{
			mvddest_t *dst;
			e =	("QTVSV 1\n"
				 "BEGIN\n"
				 "\n");
			VFS_WRITE(clientstream, e, strlen(e));
			e = NULL;
			dst = SV_MVD_InitStream(clientstream, userinfo);
			dst->droponmapchange = p->isreverse;
			if (!SV_MVD_Record(dst))
				return QTV_ERROR;
			return QTV_ACCEPT;
		}
		else
		{
			e =	("QTVSV 1\n"
				"PERROR: You need to provide a password.\n\n");
		}
	}

	if (e && !raw)	//don't write any error messages to raw requests. that would confuse stuff.
		VFS_WRITE(clientstream, e, strlen(e));
	return QTV_ERROR;
}

static int DestCloseAllFlush(enum mvdclosereason_e reason, qboolean mvdonly)
{
	int numclosed = 0;
	mvddest_t *d, **prev, *next;
	DestFlush(true);	//make sure it's all written.

	prev = &demo.dest;
	d = demo.dest;
	while(d)
	{
		next = d->nextdest;
		if (!mvdonly || d->droponmapchange)
		{
			*prev = d->nextdest;
			DestClose(d, reason);
			numclosed++;
		}
		else
			prev = &d->nextdest;

		d = next;
	}

	return numclosed;
}


static int DemoWriteDest(void *data, int len, mvddest_t *d)
{
	if (d->error)
		return 0;
	d->totalsize += len;
	switch(d->desttype)
	{
	case DEST_FILE:
		VFS_WRITE(d->file, data, len);
		break;
	case DEST_BUFFEREDFILE:	//these write to a cache, which is flushed later
	case DEST_THREADEDFILE:
	case DEST_STREAM:
		if (d->cacheused+len > d->maxcachesize)
		{
			d->error = true;
			return 0;
		}
		memcpy(d->cache+d->cacheused, data, len);
		d->cacheused += len;
		break;
	default:
	case DEST_NONE:
		Sys_Error("DemoWriteDest encoundered bad dest.");
	}
	return len;
}

static int DemoWrite(void *data, int len)	//broadcast to all proxies/mvds
{
	mvddest_t *d;
	for (d = demo.dest; d; d = d->nextdest)
	{
		if (singledest && singledest != d)
			continue;
		DemoWriteDest(data, len, d);
	}
	return len;
}

void DemoWriteQTVTimePad(int msecs)	//broadcast to all proxies
{
	mvddest_t *d;
	unsigned char buffer[6];
	while (msecs > 0)
	{
		//duration
		if (msecs > 255)
			buffer[0] = 255;
		else
			buffer[0] = msecs;
		msecs -= buffer[0];
		//message type
		buffer[1] = dem_read;
		//length
		buffer[2] = 0;
		buffer[3] = 0;
		buffer[4] = 0;
		buffer[5] = 0;

		for (d = demo.dest; d; d = d->nextdest)
		{
			if (d->desttype == DEST_STREAM)
			{
				DemoWriteDest(buffer, sizeof(buffer), d);
			}
		}
	}
}




// returns the file size
// return -1 if file is not present
// the file should be in BINARY mode for stupid OSs that care
#define MAX_MVD_NAME 64

typedef struct
{
	char	name[MAX_MVD_NAME];
	qofs_t	size;
	time_t	mtime;
	searchpathfuncs_t *path;
} file_t;

typedef struct
{
	file_t *files;
	qofs_t	size;
	int		numfiles;
	int		numdirs;

	int		maxfiles;
} dir_t;

#define SORT_NO 0
#define SORT_BY_DATE 1

static int QDECL Sys_listdirFound(const char *fname, qofs_t fsize, time_t mtime, void *uptr, searchpathfuncs_t *spath)
{
	file_t *f;
	dir_t *dir = uptr;
	fname = COM_SkipPath(fname);
	if (!*fname)
	{
		dir->numdirs++;
		return true;
	}
	if (dir->numfiles == dir->maxfiles)
	{
		int nc = dir->numfiles + 256;
		file_t *n = realloc(dir->files, nc*sizeof(*dir->files));
		if (!n)
			return false;
		dir->files = n;
		dir->maxfiles = nc;
	}
	f = &dir->files[dir->numfiles++];
	Q_strncpyz(f->name, fname, sizeof(f->name));
	f->size = fsize;
	f->mtime = mtime;
	f->path = spath;
	dir->size += fsize;

	return true;
}

static int QDECL Sys_listdir_Sort(const void *va, const void *vb)
{
	const file_t *fa = va;
	const file_t *fb = vb;

	if (fa->mtime == fb->mtime)
		return 0;
	if (fa->mtime >= fb->mtime)
		return 1;
	return -1;
}

static dir_t *Sys_listdemos (char *path, int ispublic, qboolean usesorting)
{
	const char *exts[] = {
		".mvd", ".mvd.gz",
		".qwd", ".qwd.gz",
		".qwz", ".qwz.gz",	//our client doesn't support them, but others do any they still might want to download them if there's any there...
#ifdef NQPROT
		".dem", ".dem.gz",
		//don't bother with .dz, that's more of an archive format (and should at least show up with .dem files)
#endif
#if defined(Q2SERVER) || defined(Q2CLIENT)
		".dm2", ".dm2.gz"
#endif
	};
	char searchterm[MAX_QPATH];
	size_t i;

	dir_t *dir = malloc(sizeof(*dir));
	memset(dir, 0, sizeof(*dir));
	dir->files = NULL;
	dir->maxfiles = 0;

	for (i = 0; i < (ispublic?2:countof(exts)); i++)
	{
		Q_strncpyz(searchterm, va("%s/*%s", path, exts[i]), sizeof(searchterm));
		COM_EnumerateFiles(searchterm, Sys_listdirFound, dir);
	}

	if (usesorting)
		qsort(dir->files, dir->numfiles, sizeof(*dir->files), Sys_listdir_Sort);

	return dir;
}
static void Sys_freedir(dir_t *dir)
{
	if (dir)
		free(dir->files);
	free(dir);
}









// only one .. is allowed (so we can get to the same dir as the quake exe)
static void QDECL SV_DemoDir_Callback(struct cvar_s *var, char *oldvalue)
{
	char *value;

	value = var->string;
	if (!value[0] || value[0] == '/' || (value[0] == '\\' && value[1] == '\\'))
	{
		Cvar_ForceSet(var, var->enginevalue);
		return;
	}
	if (value[0] == '.' && value[1] == '.')
		value += 2;
	if (strstr(value,".."))
	{
		Cvar_ForceSet(var, var->enginevalue);
		return;
	}
}

void SV_MVDPings (void)
{
	sizebuf_t *msg;
	client_t *client;
	int		j;

	for (j = 0, client = svs.clients; j < demo.recorder.max_net_clients && j < svs.allocated_client_slots; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;

		msg = MVDWrite_Begin (dem_all, 0, 7);
		MSG_WriteByte(msg, svc_updateping);
		MSG_WriteByte(msg,  j);
		MSG_WriteShort(msg,  SV_CalcPing(client, false));
		MSG_WriteByte(msg, svc_updatepl);
		MSG_WriteByte (msg, j);
		MSG_WriteByte (msg, client->lossage);
	}
}
void SV_MVD_FullClientUpdate(sizebuf_t *msg, client_t *player)
{
	char info[EXTENDED_INFO_STRING];
	qboolean dosizes;

	if (!sv.mvdrecording)
		return;

	dosizes = !msg;

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 4);
	MSG_WriteByte (msg, svc_updatefrags);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteShort (msg, player->old_frags);

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 4);
	MSG_WriteByte (msg, svc_updateping);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteShort (msg, SV_CalcPing(player, false)&0xffff);

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 3);
	MSG_WriteByte (msg, svc_updatepl);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteByte (msg, player->lossage);

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 6);
	MSG_WriteByte (msg, svc_updateentertime);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteFloat (msg, realtime - player->connection_started);

	InfoBuf_ToString(&player->userinfo, info, sizeof(info), basicuserinfos, privateuserinfos, NULL, &demo.recorder.infosync, player);

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 7 + strlen(info));
	MSG_WriteByte (msg, svc_updateuserinfo);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteLong (msg, player->userid);
	MSG_WriteString (msg, info);
}

sizebuf_t *MVDWrite_Begin(qbyte type, int to, int size)
{
	if (demomsg.cursize && (demomsgtype != type || demomsgto != to || demomsg.cursize+size > sizeof(demomsgbuf)))
	{
		SV_WriteMVDMessage(&demomsg, demomsgtype, demomsgto, demo_prevtime);
		demomsg.cursize = 0;
	}

	demomsgtype = type;
	demomsgto = to;

	demomsg.maxsize = demomsg.cursize+size;
	demomsg.data = demomsgbuf;
	demomsg.prim = demo.recorder.netchan.netprim;
	return &demomsg;
}

/*
====================
SV_WriteMVDMessage

Dumps the current net message, along with framing
====================
*/
void SV_WriteMVDMessage (sizebuf_t *msg, int type, int to, float time)
{
	int		len, i, msec;
	qbyte	c;

	if (!sv.mvdrecording)
		return;

	if (msg->overflowed)
	{
		msg->overflowed = false;
		Con_Printf("SV_WriteMVDMessage: message overflowed\n");
		return;
	}

	msec = (time - demo_prevtime)*1000;
	if (abs(msec) > 1000)
	{
		//catastoptic slip. debugging? reset any sync
		msec = 1;
		demo_prevtime = time;
	}
	else if (msec > 0)
	{	//if there was any progress, make sure we write msecs >0
		if (msec > 255)
			msec = 255;
		if (msec < 1)
			msec = 1;
		demo_prevtime += msec*0.001;
	}
	else
		msec = 0;

	c = msec;
	DemoWrite(&c, sizeof(c));

	if (demo.lasttype != type || demo.lastto != to)
	{
		demo.lasttype = type;
		demo.lastto = to;
		switch (demo.lasttype)
		{
		case dem_all:
			c = dem_all;
			DemoWrite (&c, sizeof(c));
			break;
		case dem_multiple:
			c = dem_multiple;
			DemoWrite (&c, sizeof(c));

			i = LittleLong(demo.lastto);
			DemoWrite (&i, sizeof(i));
			break;
		case dem_single:
		case dem_stats:
			c = demo.lasttype + (demo.lastto << 3);
			DemoWrite (&c, sizeof(c));
			break;
		default:
			SV_MVDStop_f ();
			Con_Printf("bad demo message type:%d", type);
			return;
		}
	} else {
		c = dem_read;
		DemoWrite (&c, sizeof(c));
	}


	len = LittleLong (msg->cursize);
	DemoWrite (&len, 4);
	DemoWrite (msg->data, msg->cursize);

	DestFlush(false);
}

//if you use ClientReliable to write to demo.recorder's message buffer (for code reuse) call this function to ensure its flushed.
void SV_MVD_WriteReliables(qboolean writebroadcasts)
{
	int i;

	if (writebroadcasts)
	{
		//chuck in the broadcast reliables
		if (sv.reliable_datagram.cursize)
		{
			ClientReliableCheckBlock(&demo.recorder, sv.reliable_datagram.cursize);
			ClientReliableWrite_SZ(&demo.recorder, sv.reliable_datagram.data, sv.reliable_datagram.cursize);
		}
		//and the broadcast unreliables. everything is reliables when it comes to mvds
		if (sv.datagram.cursize)
		{
			ClientReliableCheckBlock(&demo.recorder, sv.datagram.cursize);
			ClientReliableWrite_SZ(&demo.recorder, sv.datagram.data, sv.datagram.cursize);
		}
	}

	if (demo.recorder.netchan.message.cursize)
	{
		SV_WriteMVDMessage(&demo.recorder.netchan.message, dem_all, 0, demo_prevtime);
		demo.recorder.netchan.message.cursize = 0;
	}
	for (i = 0; i < demo.recorder.num_backbuf; i++)
	{
		demo.recorder.backbuf.data = demo.recorder.backbuf_data[i];
		demo.recorder.backbuf.cursize = demo.recorder.backbuf_size[i];
		if (demo.recorder.backbuf.cursize)
			SV_WriteMVDMessage(&demo.recorder.backbuf, dem_all, 0, demo_prevtime);
		demo.recorder.backbuf_size[i] = 0;
	}
	demo.recorder.num_backbuf = 0;
	demo.recorder.backbuf.cursize = 0;
}

/*
====================
SV_MVDWritePackets

Interpolates to get exact players position for current frame
and writes packets to the disk/memory
====================
*/

float adjustangle(float current, float ideal, float fraction)
{
	float move;

	move = ideal - current;
	if (ideal > current)
	{

		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}

	move *= fraction;

	return (current + move);
}

qboolean SV_MVDWritePackets (int num)
{
	demo_frame_t	*frame, *nextframe;
	demo_client_t	*cl, *nextcl = NULL;
	int				i, j, flags;
	qboolean		valid;
	double			time, playertime, nexttime;
	float			f;
	vec3_t			origin, angles;
	sizebuf_t		msg;
	qbyte			msg_buf[MAX_QWMSGLEN];
	demoinfo_t		*demoinfo;

	if (!sv.mvdrecording)
		return false;
	if (demo.recorder.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
		return false;

	//flush any intermediate data
	MVDWrite_Begin(255, -1, 0);

	msg.allowoverflow = true;	//fixme
	msg.overflowed = false;
	msg.prim = svs.netprim;
	msg.data = msg_buf;
	msg.maxsize = sizeof(msg_buf);

	if (num > demo.parsecount - demo.lastwritten + 1)
		num = demo.parsecount - demo.lastwritten + 1;

	// 'num' frames to write
	for ( ; num; num--, demo.lastwritten++)
	{
		frame = &demo.frames[demo.lastwritten&DEMO_FRAMES_MASK];
		time = frame->time;
		nextframe = frame;
		msg.cursize = 0;

		// find two frames
		// one before the exact time (time - msec) and one after,
		// then we can interpolte exact position for current frame
		for (i = 0, cl = frame->clients, demoinfo = demo.info; i < demo.recorder.max_net_clients ; i++, cl++, demoinfo++)
		{
			if (cl->parsecount != demo.lastwritten)
				continue; // not valid

			nexttime = playertime = time - cl->sec;

			for (j = demo.lastwritten+1, valid = false; nexttime < time && j < demo.parsecount; j++)
			{
				nextframe = &demo.frames[j&DEMO_FRAMES_MASK];
				nextcl = &nextframe->clients[i];

				if (nextcl->parsecount != j)
					break; // disconnected?
				if (nextcl->fixangle)
					break; // respawned, or walked into teleport, do not interpolate!
				if (!(nextcl->flags & DF_DEAD) && (cl->flags & DF_DEAD))
					break; // respawned, do not interpolate

				nexttime = nextframe->time - nextcl->sec;

				if (nexttime >= time)
				{
					// good, found what we were looking for
					valid = true;
					break;
				}
			}

			if (valid)
			{
				f = (time - nexttime)/(nexttime - playertime);
				for (j=0;j<3;j++) {
					angles[j] = adjustangle(cl->info.angles[j], nextcl->info.angles[j],1.0+f);
					origin[j] = nextcl->info.origin[j] + f*(nextcl->info.origin[j]-cl->info.origin[j]);
				}
			} else {
				VectorCopy(cl->info.origin, origin);
				VectorCopy(cl->info.angles, angles);
			}

			// now write it to buf
			flags = cl->flags;	//df_dead/df_gib

			if (demo.playerreset[i])
			{
				demo.playerreset[i] = false;
				flags |= DF_RESET;
			}

			if (cl->fixangle)
			{
				demo.fixangletime[i] = cl->cmdtime;
			}

			for (j=0; j < 3; j++)
				if (origin[j] != demoinfo->origin[i])
					flags |= DF_ORIGINX << j;

			if (cl->fixangle || demo.fixangletime[i] != cl->cmdtime)
			{
				for (j=0; j < 3; j++)
					if (angles[j] != demoinfo->angles[j])
						flags |= DF_ANGLEX << j;
			}

			if (cl->info.model != demoinfo->model)
				flags |= DF_MODEL;
			if (cl->info.effects != demoinfo->effects)
				flags |= DF_EFFECTS;
			if (cl->info.skinnum != demoinfo->skinnum)
				flags |= DF_SKINNUM;
			if (cl->info.weaponframe != demoinfo->weaponframe)
				flags |= DF_WEAPONFRAME;

			MSG_WriteByte (&msg, svc_playerinfo);
			MSG_WriteByte (&msg, i);
			MSG_WriteShort (&msg, flags);

			MSG_WriteByte (&msg, cl->frame);

			for (j=0 ; j<3 ; j++)
				if (flags & (DF_ORIGINX << j))
					MSG_WriteCoord (&msg, origin[j]);

			for (j=0 ; j<3 ; j++)
				if (flags & (DF_ANGLEX << j))
					MSG_WriteAngle16 (&msg, angles[j]);


			if (flags & DF_MODEL)
				MSG_WriteByte (&msg, cl->info.model);

			if (flags & DF_SKINNUM)
				MSG_WriteByte (&msg, cl->info.skinnum);

			if (flags & DF_EFFECTS)
				MSG_WriteByte (&msg, cl->info.effects & 0xff);

			if (flags & DF_WEAPONFRAME)
				MSG_WriteByte (&msg, cl->info.weaponframe);

			VectorCopy(cl->info.origin, demoinfo->origin);
			VectorCopy(cl->info.angles, demoinfo->angles);
			demoinfo->skinnum = cl->info.skinnum;
			demoinfo->effects = cl->info.effects;
			demoinfo->weaponframe = cl->info.weaponframe;
			demoinfo->model = cl->info.model;
		}

		if (msg.cursize)
			SV_WriteMVDMessage(&msg, dem_all, 0, (float)time);

		/* The above functions can set this variable to false, but that's a really bad thing. Let's try to fix it. */
		if (!sv.mvdrecording)
			return false;
	}

	if (demo.lastwritten > demo.parsecount)
		demo.lastwritten = demo.parsecount;

	return true;
}

void MVD_Init (void)
{
#define MVDVARGROUP "Server MVD cvars"

	Cvar_Register (&sv_demofps,			MVDVARGROUP);
	Cvar_Register (&sv_demoPings,		MVDVARGROUP);
	Cvar_Register (&sv_demoUseCache,	MVDVARGROUP);
	Cvar_Register (&sv_demoCacheSize,	MVDVARGROUP);
	Cvar_Register (&sv_demoMaxSize,		MVDVARGROUP);
	Cvar_Register (&sv_demoMaxDirSize,	MVDVARGROUP);
	Cvar_Register (&sv_demoMaxDirCount,	MVDVARGROUP);
	Cvar_Register (&sv_demoMaxDirAge,	MVDVARGROUP);
	Cvar_Register (&sv_demoClearOld,	MVDVARGROUP);
	Cvar_Register (&sv_demoDir,			MVDVARGROUP);
	Cvar_Register (&sv_demoDirAlt,		MVDVARGROUP);
	Cvar_Register (&sv_demoPrefix,		MVDVARGROUP);
	Cvar_Register (&sv_demoSuffix,		MVDVARGROUP);
	Cvar_Register (&sv_demotxt,			MVDVARGROUP);
	Cvar_Register (&sv_demoExtraNames,	MVDVARGROUP);
	Cvar_Register (&sv_demoExtensions,	MVDVARGROUP);
	Cvar_Register (&sv_demoAutoCompress,MVDVARGROUP);
	Cvar_Register (&sv_demoAutoRecord,	MVDVARGROUP);
	Cvar_Register (&sv_demoAutoPrefix,	MVDVARGROUP);
	Cvar_Register (&sv_demo_write_csqc,MVDVARGROUP);
}

static char *SV_PrintTeams(void)
{
	char *teams[MAX_CLIENTS];
//	char *p;
	int	i, j, numcl = 0, numt = 0;
	client_t *clients[MAX_CLIENTS];
	char buf[2048] = {0};
	extern cvar_t teamplay;
//	extern char chartbl2[];

	// count teams and players
	for (i=0; i < sv.allocated_client_slots; i++)
	{
		if (svs.clients[i].state != cs_spawned)
			continue;
		if (svs.clients[i].spectator)
			continue;

		clients[numcl++] = &svs.clients[i];
		for (j = 0; j < numt; j++)
			if (!strcmp(InfoBuf_ValueForKey(&svs.clients[i].userinfo, "team"), teams[j]))
				break;
		if (j != numt)
			continue;

		teams[numt++] = InfoBuf_ValueForKey(&svs.clients[i].userinfo, "team");
	}

	// create output

	if (numcl == 2) // duel
	{
		snprintf(buf, sizeof(buf), "team1 %s\nteam2 %s\n", clients[0]->name, clients[1]->name);
	}
	else if (!teamplay.value) // ffa
	{
		snprintf(buf, sizeof(buf), "players:\n");
		for (i = 0; i < numcl; i++)
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "  %s\n", clients[i]->name);
	}
	else
	{ // teamplay
		for (j = 0; j < numt; j++)
		{
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "team %s:\n", teams[j]);
			for (i = 0; i < numcl; i++)
				if (!strcmp(InfoBuf_ValueForKey(&clients[i]->userinfo, "team"), teams[j]))
					snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "  %s\n", clients[i]->name);
		}
	}

	if (!numcl)
		return "\n";
//	for (p = buf; *p; p++) *p = chartbl2[(qbyte)*p];
	return va("%s",buf);
}


mvddest_t *SV_FindRecordFile(char *match, mvddest_t ***link_out)
{
	mvddest_t **link, *f;
	for (link = &demo.dest; *link; link = &(*link)->nextdest)
	{
		f = *link;
		if (f->desttype == DEST_FILE || f->desttype == DEST_BUFFEREDFILE || f->desttype == DEST_THREADEDFILE)
		{
			if (!match || !strcmp(match, f->simplename))
			{
				if (link_out)
					*link_out = link;
				return f;
			}
		}
	}
	return NULL;
}

/*
====================
SV_InitRecord
====================
*/

mvddest_t *SV_MVD_InitRecordFile (char *name)
{
	char *s, *txtname;
	mvddest_t *dst;
	vfsfile_t *file;

	if (strlen(name) >= countof(dst->filename))
	{
		Con_Printf ("ERROR: couldn't open \"%s\". Too long.\n", name);
		return NULL;
	}

	file = FS_OpenVFS (name, "wb", FS_GAMEONLY);
	if (!file)
	{
		Con_Printf ("ERROR: couldn't open \"%s\"\n", name);
		return NULL;
	}

#ifdef AVAIL_GZDEC
	if (!Q_strcasecmp(".gz", COM_GetFileExtension(name, NULL)))
		file = FS_GZ_WriteFilter(file, true, true);
#endif

	dst = Z_Malloc(sizeof(mvddest_t));
	strcpy(dst->filename, name);

#ifdef LOADERTHREAD
	if (!*sv_demoUseCache.string)
		dst->desttype = DEST_THREADEDFILE;
	else
#endif
		if (sv_demoUseCache.value <= 0)
		dst->desttype = DEST_FILE;
	else
		dst->desttype = DEST_BUFFEREDFILE;

	if (dst->desttype == DEST_FILE)
	{
		dst->desttype = DEST_FILE;
		dst->file = file;
		dst->maxcachesize = 0;
	}
	else
	{	//cached or threaded
		dst->file = file;
		if (sv_demoCacheSize.ival < 0x8000)
			dst->maxcachesize = 0x8000;
		else
			dst->maxcachesize = sv_demoCacheSize.ival;
		dst->cache = BZ_Malloc(dst->maxcachesize);
		if (dst->desttype == DEST_THREADEDFILE)
			dst->altcache = BZ_Malloc(dst->maxcachesize);
		else
			dst->altcache = NULL;
	}
	dst->droponmapchange = true;

	s = name + strlen(name);
	while (*s != '/') s--;
	Q_strncpyz(dst->simplename, s+1, sizeof(dst->simplename));

	switch(dst->desttype)
	{
	default:
	case DEST_NONE:
		SV_BroadcastPrintf (PRINT_CHAT, "Server starts recording (%s):\n%s\n", "ERROR", name);
		break;
	case DEST_STREAM:
		SV_BroadcastPrintf (PRINT_CHAT, "Server starts recording (%s):\n%s\n", "ERROR: STREAM", name);
		break;
	case DEST_BUFFEREDFILE:
		SV_BroadcastPrintf (PRINT_CHAT, "Server starts recording (%s):\n%s\n", "memory", name);
		break;
	case DEST_THREADEDFILE:
		//SV_BroadcastPrintf (PRINT_CHAT, "Server starts recording (%s):\n%s\n", "worker thread", name);
		SV_BroadcastPrintf (PRINT_CHAT, "Server starts recording:\n%s\n", name);
		break;
	case DEST_FILE:
		SV_BroadcastPrintf (PRINT_CHAT, "Server starts recording (%s):\n%s\n", "disk", name);
		break;
	}

	txtname = SV_MVDName2Txt(name);
	if (sv_demotxt.value)
	{
		vfsfile_t *f;

		if (sv_demotxt.value == 2)
		{
			//this is a special mode for mods that want to write it instead (done via the sv_demoinfoadd command).
			f = FS_OpenVFS (txtname, "wt", FS_GAMEONLY);
			if (f)
				VFS_CLOSE(f);
		}
		else
		{
			f = FS_OpenVFS (txtname, "wt", FS_GAMEONLY);
			if (f != NULL)
			{
				char buf[2000];
				date_t date;

				COM_TimeOfDay(&date);

				snprintf(buf, sizeof(buf), "date %s\nmap %s\nteamplay %d\ndeathmatch %d\ntimelimit %d\n%s",date.str, svs.name, (int)teamplay.value, (int)deathmatch.value, (int)timelimit.value, SV_PrintTeams());
				VFS_WRITE(f, buf, strlen(buf));
				VFS_FLUSH(f);
				VFS_CLOSE(f);
			}
		}
	}
	else
	{
		FS_Remove(txtname, FS_GAMEONLY);
		FS_FlushFSHashRemoved(txtname);
	}

	return dst;
}

char *SV_Demo_CurrentOutput(void)
{
	mvddest_t *d;
	for (d = demo.dest; d; d = d->nextdest)
	{
		if (d->desttype == DEST_FILE || d->desttype == DEST_BUFFEREDFILE || d->desttype == DEST_THREADEDFILE)
			return d->simplename;
	}
	return "";
}
void SV_Demo_PrintOutputs(void)
{
	mvddest_t *d;
	for (d = demo.dest; d; d = d->nextdest)
	{
		if (d->desttype == DEST_FILE || d->desttype == DEST_BUFFEREDFILE || d->desttype == DEST_THREADEDFILE)
			Con_Printf("recording        : %s\n", d->simplename);
		else if (d->desttype == DEST_STREAM)
			Con_Printf("streaming        : %s\n", d->simplename);
	}
}

static mvddest_t *SV_MVD_InitStream(vfsfile_t *stream, const char *info)
{
	mvddest_t *dst;

	for (dst = demo.dest; dst; dst = dst->nextdest)
	{
		if (dst->desttype == DEST_STREAM)
			break;
	}
	if (!dst)
		SV_BroadcastPrintf (PRINT_CHAT, "Smile, you're on QTV!\n");

	dst = Z_Malloc(sizeof(mvddest_t));

	dst->desttype = DEST_STREAM;
	dst->file = stream;
	dst->maxcachesize = 0x8000;	//is this too small?
	dst->cache = BZ_Malloc(dst->maxcachesize);
	dst->droponmapchange = false;
	*dst->filename = 0;
	*dst->simplename = 0;

	if (info)
	{
		char *s = Info_ValueForKey(info, "name");
		Q_strncpyz(dst->simplename, s, sizeof(dst->simplename));

		s = Info_ValueForKey(info, "streamid");
		Q_strncpyz(dst->filename, s, sizeof(dst->filename));
		s = Info_ValueForKey(info, "address");
		if (*dst->filename && *s)
			Q_strncatz(dst->filename, "@", sizeof(dst->filename));
		Q_strncatz(dst->filename, s, sizeof(dst->filename));
	}

	return dst;
}

/*
====================
SV_Stop

stop recording a demo
====================
*/
void SV_MVDStop (enum mvdclosereason_e reason, qboolean mvdonly)
{
	sizebuf_t *msg;
	if (!sv.mvdrecording)
	{
		Con_Printf ("Not recording a demo.\n");
		return;
	}

	if (reason == MVD_CLOSE_CANCEL || reason == MVD_CLOSE_DISCONNECTED)
	{
		DestCloseAllFlush(reason, mvdonly);
		// stop and remove

		if (!demo.dest)
			SV_MVD_Stopped();

		if (reason == MVD_CLOSE_DISCONNECTED)
			SV_BroadcastPrintf (PRINT_CHAT, "QTV disconnected\n");
		else
			SV_BroadcastPrintf (PRINT_CHAT, "Server recording canceled, demo removed\n");

		Cvar_ForceSet(Cvar_Get("serverdemo", "", CVAR_NOSET, ""), "");

		return;
	}

// write a disconnect message to the demo file
	msg = MVDWrite_Begin(dem_all, 0, 2+strlen("EndOfDemo"));
	MSG_WriteByte (msg, svc_disconnect);
	MSG_WriteString (msg, "EndOfDemo");

	SV_MVDWritePackets(demo.parsecount - demo.lastwritten + 1);
// finish up

	DestCloseAllFlush(reason, mvdonly);

	if (!demo.dest)	//might still be streaming qtv.
		SV_MVD_Stopped();

	Cvar_ForceSet(Cvar_Get("serverdemo", "", CVAR_NOSET, ""), "");
}

/*
====================
SV_Stop_f
====================
*/
void SV_MVDStop_f (void)
{
	SV_MVDStop(MVD_CLOSE_STOPPED, true);
}

/*
====================
SV_Cancel_f

Stops recording, and removes the demo
====================
*/
void SV_MVD_Cancel_f (void)
{
	SV_MVDStop(MVD_CLOSE_CANCEL, true);
}

/*
====================
SV_WriteMVDMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/

void SV_WriteRecordMVDMessage (sizebuf_t *msg)
{
	int		len;
	qbyte	c;

	if (!sv.mvdrecording)
		return;

	if (!msg->cursize)
		return;

	c = 0;
	DemoWrite (&c, sizeof(c));

	c = dem_read;
	DemoWrite (&c, sizeof(c));

	len = LittleLong (msg->cursize);
	DemoWrite (&len, 4);

	DemoWrite (msg->data, msg->cursize);

	DestFlush(false);
}

void SV_WriteSetMVDMessage (void)
{
	int		len;
	qbyte	c;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, realtime);

	if (!sv.mvdrecording)
		return;

	c = 0;
	DemoWrite (&c, sizeof(c));

	c = dem_set;
	DemoWrite (&c, sizeof(c));


	len = LittleLong(0);
	DemoWrite (&len, 4);
	len = LittleLong(0);
	DemoWrite (&len, 4);

	DestFlush(false);
}

void SV_MVD_SendInitialGamestate(mvddest_t *dest);
qboolean SV_MVD_Record (mvddest_t *dest)
{
	static int destid;
	if (!dest)
		return false;

	dest->id = ++destid;	//give each stream a unique id, for no real reason other than for other people to track it via SVC_Status(|32).

	SV_MVD_WriteReliables(false);
	DestFlush(true);

	if (!sv.mvdrecording)
	{
		memset(&demo, 0, sizeof(demo));
		demo.recorder.protocol = SCP_QUAKEWORLD;
		demo.recorder.netchan.netprim = sv.datagram.prim;

		demo.datagram.maxsize = sizeof(demo.datagram_data);
		demo.datagram.data = demo.datagram_data;
		demo.datagram.prim = demo.recorder.netchan.netprim;

		demo.recorder.netchan.message.maxsize = sizeof(demo.recorder.netchan.message_buf);
		demo.recorder.netchan.message.data = demo.recorder.netchan.message_buf;
		demo.recorder.netchan.message.prim = demo.recorder.netchan.netprim;

		if (sv_demoExtensions.ival == 2 || !*sv_demoExtensions.string)
		{	/*more limited subset supported by ezquake, but not fuhquake/fodquake. sorry.*/
			demo.recorder.fteprotocolextensions = /*PEXT_CHUNKEDDOWNLOADS|*/PEXT_256PACKETENTITIES|/*PEXT_FLOATCOORDS|*/PEXT_MODELDBL|PEXT_ENTITYDBL|PEXT_ENTITYDBL2|PEXT_SPAWNSTATIC2;
//			demo.recorder.fteprotocolextensions |= PEXT_HLBSP;	/*ezquake DOES have this, but it is pointless and should have been in some feature mask rather than protocol extensions*/
//			demo.recorder.fteprotocolextensions |= PEXT_ACCURATETIMINGS;	/*ezquake does not support this any more. pointless in an mvd anyway*/
			demo.recorder.fteprotocolextensions |= PEXT_TRANS;	/*ezquake's support for alpha is buggyaf on players, but mvd streams change svc_playerinfo which sidesteps the issue*/
//			demo.recorder.fteprotocolextensions |= PEXT_COLOURMOD;	/*nano is working on adding this*/
//			demo.recorder.fteprotocolextensions |= PEXT_DPFLAGS;	/*nano is working on adding this*/
			demo.recorder.fteprotocolextensions2 = PEXT2_VOICECHAT;
			demo.recorder.zquake_extensions = Z_EXT_PM_TYPE | Z_EXT_PM_TYPE_NEW | Z_EXT_VIEWHEIGHT | Z_EXT_SERVERTIME | Z_EXT_PITCHLIMITS | Z_EXT_JOIN_OBSERVE | Z_EXT_VWEP;
		}
		else if (sv_demoExtensions.ival)
		{	/*everything*/
			extern cvar_t pext_replacementdeltas;
			demo.recorder.fteprotocolextensions = PEXT_CHUNKEDDOWNLOADS | PEXT_CSQC | PEXT_COLOURMOD | PEXT_DPFLAGS | PEXT_CUSTOMTEMPEFFECTS | PEXT_ENTITYDBL | PEXT_ENTITYDBL2 | PEXT_FATNESS | PEXT_HEXEN2 | PEXT_HULLSIZE | PEXT_LIGHTSTYLECOL | PEXT_MODELDBL | PEXT_SCALE | PEXT_SETATTACHMENT | PEXT_SETVIEW | PEXT_SOUNDDBL | PEXT_SPAWNSTATIC2 | PEXT_TRANS;
#ifdef PEXT_VIEW2
			demo.recorder.fteprotocolextensions |= PEXT_VIEW2;
#endif
			demo.recorder.fteprotocolextensions2 = PEXT2_VOICECHAT | PEXT2_SETANGLEDELTA | /*PEXT2_PRYDONCURSOR |*/ (pext_replacementdeltas.ival?PEXT2_REPLACEMENTDELTAS|PEXT2_PREDINFO|PEXT2_NEWSIZEENCODING:0);
			/*enable these, because we might as well (stat ones are always useful)*/
			demo.recorder.zquake_extensions = Z_EXT_PM_TYPE | Z_EXT_PM_TYPE_NEW | Z_EXT_VIEWHEIGHT | Z_EXT_SERVERTIME | Z_EXT_PITCHLIMITS | Z_EXT_JOIN_OBSERVE | Z_EXT_VWEP;

//			if (demo.recorder.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)	//replacementdeltas makes a number of earlier extensions obsolete...
//				demo.recorder.fteprotocolextensions &= ~(PEXT_COLOURMOD|PEXT_DPFLAGS|PEXT_ENTITYDBL|PEXT_ENTITYDBL2|PEXT_FATNESS|PEXT_HEXEN2|PEXT_HULLSIZE|PEXT_MODELDBL|PEXT_SCALE|PEXT_SETATTACHMENT|PEXT_SOUNDDBL|PEXT_SPAWNSTATIC2|PEXT_TRANS);
		}
		else
		{
			demo.recorder.fteprotocolextensions = 0;
			demo.recorder.fteprotocolextensions2 = 0;
			demo.recorder.zquake_extensions = Z_EXT_PM_TYPE | Z_EXT_PM_TYPE_NEW | Z_EXT_VIEWHEIGHT | Z_EXT_SERVERTIME | Z_EXT_PITCHLIMITS | Z_EXT_JOIN_OBSERVE | Z_EXT_VWEP;
		}

		//pointless extensions that are redundant with mvds
		demo.recorder.fteprotocolextensions &= ~PEXT_ACCURATETIMINGS | PEXT_CHUNKEDDOWNLOADS;
		demo.recorder.fteprotocolextensions &= ~PEXT1_HIDEPROTOCOLS;
	}
//	else
//		SV_WriteRecordMVDMessage(&buf, dem_read);

	dest->nextdest = demo.dest;
	demo.dest = dest;

	Cvar_ForceSet(Cvar_Get("serverdemo", "", CVAR_NOSET, ""), SV_Demo_CurrentOutput());

	SV_ClientProtocolExtensionsChanged(&demo.recorder);

	SV_MVD_SendInitialGamestate(dest);
	return true;
}

static void SV_MVD_Stopped(void)
{	//all recording has stopped. clean up any demo.recorder state
	if (demo.recorder.frameunion.frames)
	{
		Z_Free(demo.recorder.frameunion.frames);
		demo.recorder.frameunion.frames = NULL;
	}
	sv.mvdrecording = false;
	memset(&demo, 0, sizeof(demo));
}

void SV_EnableClientsCSQC(void);
void SV_MVD_SendInitialGamestate(mvddest_t *dest)
{
	sizebuf_t	buf;
	char buf_data[MAX_QWMSGLEN];
	int i, j;
//	int n;
//	const char *s;

	client_t *player;
	char *gamedir;

	if (!demo.dest)
		return;

	SV_MVD_WriteReliables(false);

	sv.mvdrecording = true;
	demo.resetdeltas = true;

	host_client = &demo.recorder;
	if (demo.recorder.fteprotocolextensions & PEXT_CSQC)
		SV_EnableClientsCSQC();


	demo.pingtime = demo.time = sv.time;


	singledest = dest;

/*-------------------------------------------------*/

// serverdata
	// send the info about the new client to all connected clients
	memset(&buf, 0, sizeof(buf));
	buf.data = buf_data;
	buf.maxsize = sizeof(buf_data);
	buf.prim = svs.netprim;

// send the serverdata

	gamedir = InfoBuf_ValueForKey (&svs.info, "*gamedir");
	if (!gamedir[0])
		gamedir = FS_GetGamedir(true);

	//generate some meta info so the file can be identified later
	{
		char timestr[64];
		time_t t;
		MSG_WriteByte (&buf, svc_stufftext);
		MSG_WriteString(&buf, va("//protocolname %s\n", com_protocolname.string));	//so that the game is known when playing back, to deal with games that conventionally have entirely separate installations.

		MSG_WriteByte (&buf, svc_stufftext);
		t = time(NULL);
		strftime(timestr, sizeof(timestr), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
		MSG_WriteString(&buf, va("//recorddate %s\n", timestr));					//in order to avoid needing to depend upon file times that get destroyed in many different ways.
	}

	MSG_WriteByte (&buf, svc_serverdata);

	//fix up extensions to match sv_bigcoords correctly. sorry for old clients not working.
	if (buf.prim.coordtype == COORDTYPE_FLOAT_32)
		demo.recorder.fteprotocolextensions |= PEXT_FLOATCOORDS;
	else
		demo.recorder.fteprotocolextensions &= ~PEXT_FLOATCOORDS;

	if (demo.recorder.fteprotocolextensions)
	{
		MSG_WriteLong(&buf, PROTOCOL_VERSION_FTE1);
		MSG_WriteLong(&buf, demo.recorder.fteprotocolextensions);
	}
	if (demo.recorder.fteprotocolextensions2)
	{
		MSG_WriteLong(&buf, PROTOCOL_VERSION_FTE2);
		MSG_WriteLong(&buf, demo.recorder.fteprotocolextensions2);
	}
	MSG_WriteLong (&buf, PROTOCOL_VERSION_QW);
	MSG_WriteLong (&buf, svs.spawncount);
	MSG_WriteString (&buf, gamedir);

	if (demo.recorder.fteprotocolextensions2 & PEXT2_MAXPLAYERS)
		MSG_WriteByte(&buf, demo.recorder.max_net_ents);

	MSG_WriteFloat (&buf, sv.time);

	// send full levelname
	MSG_WriteString (&buf, sv.mapname);

	// send the movevars
	MSG_WriteFloat(&buf, movevars.gravity);
	MSG_WriteFloat(&buf, movevars.stopspeed);
	MSG_WriteFloat(&buf, movevars.maxspeed);
	MSG_WriteFloat(&buf, movevars.spectatormaxspeed);
	MSG_WriteFloat(&buf, movevars.accelerate);
	MSG_WriteFloat(&buf, movevars.airaccelerate);
	MSG_WriteFloat(&buf, movevars.wateraccelerate);
	MSG_WriteFloat(&buf, movevars.friction);
	MSG_WriteFloat(&buf, movevars.waterfriction);
	MSG_WriteFloat(&buf, movevars.entgravity);

	SV_WriteRecordMVDMessage (&buf);
	SZ_Clear (&buf);

	demo.recorder.prespawn_stage = PRESPAWN_SERVERINFO;
	demo.recorder.prespawn_idx = 0;
	while (demo.recorder.prespawn_stage != PRESPAWN_COMPLETED)
	{
		if (demo.recorder.prespawn_stage == PRESPAWN_MAPCHECK)
		{
			demo.recorder.prespawn_stage++;//client won't reply, so don't wait.
			demo.recorder.prespawn_idx = 0;
		}
		demo.recorder.prespawn_allow_soundlist = true;	//normally set for the server to wait for ack. we don't want to wait.
		demo.recorder.prespawn_allow_modellist = true;	//normally set for the server to wait for ack. we don't want to wait.

		SV_SendClientPrespawnInfo(&demo.recorder);
		SV_MVD_WriteReliables(false);
	}

// send current status of all other players

	for (i = 0; i < demo.recorder.max_net_clients && i < svs.allocated_client_slots; i++)
	{
		player = &svs.clients[i];

		SV_MVD_FullClientUpdate(&buf, player);

		if (buf.cursize > MAX_QWMSGLEN/2)
		{
			//flush backbuffer
			SV_WriteRecordMVDMessage (&buf);
			SZ_Clear (&buf);
		}
	}

// send all current light styles
	for (i=0 ; i<sv.maxlightstyles || i < MAX_STANDARDLIGHTSTYLES; i++)
		SV_SendLightstyle(&demo.recorder, &buf, i, true);

	//invalidate stats+players somehow
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		for (j = 0; j < MAX_CL_STATS; j++)
		{
			demo.statsi[i][j] = 0x7fffffff;
			demo.statsf[i][j] = -FLT_MAX;
		}
		demo.playerreset[i] = true;
	}

	// get the client to check and download skins
	// when that is completed, a begin command will be issued
	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, "skins\n");

	SV_WriteRecordMVDMessage (&buf);
	SV_MVD_WriteReliables(false);
	SV_WriteSetMVDMessage();

	singledest = NULL;
}

//double-underscores will get merged together.
const char *SV_GenCleanTable(void)
{
	static char tab[256];
	static int tabbuilt = -1;
	int mode = com_parseutf8.ival>0;
	int i;

	if (tabbuilt == mode)
		return tab;

	//identity
	for(i = 0; i < 32; i++)
		tab[i] = '_';	//unprintables.
	for(     ; i < 128; i++)
		tab[i] = i;

	//cheesy way around NUL.mvd etc filenames.
	for(i = 'A'; i <= 'Z'; i++)
		tab[i] = i + ('a'-'A');

	//these chars are reserved by windows, so its generally best to not use them, even on loonix
	tab['<'] = '[';
	tab['>'] = ']';
	tab['|'] = '_';
	tab[':'] = '_';
	tab['*'] = '_';
	tab['?'] = '_';
	tab['\\']= '_';
	tab['/'] = '_';
	tab['\"']= '_';
	//some extra ones to make unix scripts nicer.
	tab['&'] = '_';
	tab['~'] = '_';
	tab['`'] = '_';
	tab[','] = '_';
	tab[' '] = '_';	//don't use spaces, it means files need quotes, and then stuff bugs out.
	tab['.'] = '_';	//many windows programs can't properly deal with multiple dots

	if (mode)
	{
		//high chars are regular utf-8. yay
		for(i = 128; i < 256; i++)
			tab[i] = i;
	}
	else
	{
		//second row contains coloured numbers for the hud
		tab[16] = '[';
		tab[17] = ']';
		for(i = 0; i < 10; i++)
			tab[18+i] = '0'+i;
		tab[28] = '_';	//'.'
		tab[29] =	//line breaks
		tab[30] =
		tab[31] = '_';

		//high chars

		//the first 16 chars of the high range are actually different.
		tab[128] = '_';	//scrollbars
		tab[129] = '_';
		tab[130] = '_';
		tab[130] = '_';
		for(i = 132; i < 128+16; i++)
			tab[18+i] = '_';	//LEDs mostly

		//but the rest of the table is just recoloured.
		for(i = 128+16; i < 256; i++)
			tab[i] = tab[i&127];
	}
	return tab;
}

/*
====================
SV_CleanName

Cleans the demo name, removes restricted chars, makes name lowercase
====================
*/

char *SV_CleanName (unsigned char *name)
{
	static char text[1024];
	char *out = text;
	const char *chartbl = SV_GenCleanTable();

	*out = chartbl[*name++];

	while (*name && out - text < sizeof(text))
		if (*out == '_' && chartbl[*name] == '_')
			name++;
		else *++out = chartbl[*name++];

	*++out = 0;


	out = text;
	while (*out == '.')
		out++;	//leading dots (which could be caused by all sorts of things) are bad. boo hidden files.
	return out;
}

//figure out the actual size limit. this is somewhat approximate anyway.
qofs_t MVD_DemoMaxDirSize(void)
{
	char *e;
	double maxdirsize = strtod(sv_demoMaxDirSize.string, &e);
	if (*e == ' ' || *e == '\t')
		e++;
	//that will be trailed by g[b], m[b], k[b], or b
	if (*e == 'b' || *e == 'B')
		return maxdirsize;
	else if (*e == 'k' || *e == 'K')
		return maxdirsize * 1024;
	else if (*e == 'm' || *e == 'M')
		return maxdirsize * 1024*1024;
	else if (*e == 'g' || *e == 'G')
		return maxdirsize * 1024*1024*1024;
	else
		return maxdirsize * 1024;	//assume kb.
}
//returns if there's enough disk space to record another demo.
qboolean MVD_CheckSpace(qboolean broadcastwarnings)
{
	dir_t	*dir;

	qofs_t maxdirsize = MVD_DemoMaxDirSize();
	if (maxdirsize > 0 || sv_demoMaxDirCount.ival > 0 || sv_demoMaxDirAge.ival > 0)
	{
		dir = Sys_listdemos(sv_demoDir.string, false, SORT_BY_DATE);
		if (sv_demoClearOld.ival && *sv_demoDir.string)
		{
			time_t removebeforetime = time(NULL) - sv_demoMaxDirAge.value*60*60*24;
			while (dir->numfiles && (
				(maxdirsize>0 && dir->size > maxdirsize) ||
				(sv_demoMaxDirCount.ival>0 && dir->numfiles >= sv_demoMaxDirCount.ival) ||
				(sv_demoMaxDirAge.ival && dir->files[dir->numfiles-1].mtime && dir->files[dir->numfiles-1].mtime - removebeforetime < 0)))
			{
				file_t *f = &dir->files[dir->numfiles-1];	//this is the file we want to kill.
				if (!f->path || !f->path->RemoveFile)
				{	//erm, can't remove it...
					dir->size -= f->size;
					dir->numfiles--;
					continue;
				}
				if (f->path->RemoveFile(f->path, f->name))
				{	//okay, looks like we managed to kill it.
					Con_Printf(CON_WARNING"Removed demo \"%s\"\n", f->name);
					dir->size -= f->size;
					dir->numfiles--;

					//Try to take the .txt too.
					f->path->RemoveFile(f->path, SV_MVDName2Txt(f->name));
					continue;
				}
			}
		}

		if (dir->numfiles && sv_demoMaxDirCount.ival>0 && dir->numfiles >= sv_demoMaxDirCount.ival)
		{
			if (broadcastwarnings)
				SV_BroadcastPrintf(PRINT_MEDIUM, CON_WARNING"insufficient directory space, increase server's sv_demoMaxDirCount\n");
			else
				Con_Printf(CON_WARNING"insufficient demo space, increase sv_demoMaxDirCount\n");
			Sys_freedir(dir);
			return false;
		}
		if (dir->numfiles && maxdirsize>0 && dir->size > maxdirsize)
		{
			if (broadcastwarnings)
				SV_BroadcastPrintf(PRINT_MEDIUM, CON_WARNING"insufficient directory space, increase server's sv_demoMaxDirSize\n");
			else
				Con_Printf(CON_WARNING"insufficient demo space, increase sv_demoMaxDirSize\n");
			Sys_freedir(dir);
			return false;
		}

		Sys_freedir(dir);
	}
	return true;
}

/*
====================
SV_Record_f

record <demoname>
====================
*/
void SV_MVD_Record_f (void)
{
	int		c;
	char	name[MAX_OSPATH+MAX_MVD_NAME];
	char	newname[MAX_MVD_NAME];

	c = Cmd_Argc();
	if (c != 2)
	{
		Con_Printf ("mvdrecord <demoname>\n");
		return;
	}

	if (sv.state != ss_active){
		Con_Printf ("Not active yet.\n");
		return;
	}

	if (!MVD_CheckSpace(Cmd_FromGamecode()))
		return;

	Q_strncpyz(newname, va("%s%s", sv_demoPrefix.string, SV_CleanName(Cmd_Argv(1))),
			sizeof(newname) - strlen(sv_demoSuffix.string) - 5);
	Q_strncatz(newname, sv_demoSuffix.string, MAX_MVD_NAME);

	snprintf (name, MAX_OSPATH+MAX_MVD_NAME, "%s/%s", sv_demoDir.string, newname);


	COM_StripExtension(name, name, sizeof(name));
#ifdef AVAIL_GZDEC
	if (sv_demoAutoCompress.ival == 1)
		COM_DefaultExtension(name, ".mvd.gz", sizeof(name));
	else
#endif
		COM_DefaultExtension(name, ".mvd", sizeof(name));
	FS_CreatePath (name, FS_GAMEONLY);

	//
	// open the demo file and start recording
	//
	SV_MVD_Record (SV_MVD_InitRecordFile(name));
}

//called when a connecting player becomes active.
void SV_MVD_AutoRecord (void)
{
	//not enabled (for the server) anyway.
	if (sv_demoAutoRecord.ival <= 0)
		return;

	//don't record multiple...
	if (sv.mvdrecording)
		return;

	//only do it if we're underneath our quotas.
	if (!MVD_CheckSpace(true))
		return;

	if (sv_demoAutoRecord.ival > 0)
	{
		int playercount = 0, i;
		for (i = 0; i < svs.allocated_client_slots; i++)
		{
			if (svs.clients[i].state >= cs_spawned)
				playercount++;
		}
		if (playercount >= sv_demoAutoRecord.ival)
		{	//okay, we've reached our player count, its time to start recording now.
			char name[MAX_OSPATH];
			char timestamp[64];
			time_t tm = time(NULL);
			strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&tm));
			Q_snprintfz(name, sizeof(name), "%s/%s%s_%s", sv_demoDir.string, sv_demoAutoPrefix.string, svs.name, timestamp);
#ifdef AVAIL_GZDEC
			if (sv_demoAutoCompress.ival == 1 || !*sv_demoAutoCompress.string)	//default is to gzip.
				Q_strncatz(name, ".mvd.gz", sizeof(name));
			else
#endif
				Q_strncatz(name, ".mvd", sizeof(name));
			FS_CreatePath (name, FS_GAMEONLY);

			SV_MVD_Record (SV_MVD_InitRecordFile(name));
		}
	}
}

void SV_MVD_CheckReverse(void)
{
	struct reversedest_s *rd, **link;
	enum qtvstatus_e s;
	int len;
	for (link = &reversedest; *link; link = &rd->next)
	{
		rd = *link;
		if (realtime > rd->timeout)
			len = -1;
		else
			len = VFS_READ(rd->stream, rd->inbuffer+rd->inbuffersize, sizeof(rd->inbuffer)-1-rd->inbuffersize);
		if (len < 0)
			s = QTV_ERROR;
		else if (!len)
			continue; //keep waiting...
		else
		{
			rd->inbuffersize += len;
			rd->inbuffer[rd->inbuffersize] = 0;
			if (rd->inbuffersize >= 3 && strncmp(rd->inbuffer, "QTV", 3))
				s = QTV_ERROR;	//not qtv server...
			else
			{
				char *e = strstr(rd->inbuffer, "\n\n");
				if (e)
					s = SV_MVD_GotQTVRequest(rd->stream, rd->inbuffer, rd->inbuffer+rd->inbuffersize, &rd->info);
				else
					continue; //not nuff data yet
			}
		}
		switch(s)
		{
		case QTV_RETRY:	//need to parse new stuff.
			continue;
		case QTV_ACCEPT:
			rd->stream = NULL;
			//fallthrough
		case QTV_ERROR:
			if (rd->stream)
				VFS_CLOSE(rd->stream);
			*link = rd->next;
			Z_Free(rd);
			return;
		}
	}
}

void SV_MVD_QTVReverse_f (void)
{
#if 1//ndef HAVE_TCP
//	Con_Printf ("%s is not supported in this build\n", Cmd_Argv(0));

	const char *ip = Cmd_Argv(1);
	vfsfile_t *f;
	const char *msg =	"QTV\n"
						"VERSION: 1\n"
						"REVERSE\n"
						"\n";
	struct reversedest_s *rd;
	if (sv.state<ss_loading)
		return;

	f = FS_OpenTCP(ip, 27599, false);
	if (!f)
		return;

	VFS_WRITE(f, msg, strlen(msg));

	rd = Z_Malloc(sizeof(*rd));
	rd->stream = f;
	rd->info.isreverse = true;
	rd->timeout = realtime + 10;
	reversedest = rd;
#else
	char *ip;
	if (sv.state != ss_active)
	{
		Con_Printf ("Server is not running\n");
		return;
	}
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("%s ip:port\n", Cmd_Argv(0));
		return;
	}

	ip = Cmd_Argv(1);



{
	char *data;
	int sock;

	struct sockaddr_qstorage	remote;
//	int fromlen;

	int adrfam;
	int adrsz;
	unsigned int nonblocking = true;


	if (!NET_StringToSockaddr(ip, 0, &remote, &adrfam, &adrsz))
	{
		Con_Printf ("qtvreverse: failed to resolve address\n");
		return;
	}

	if ((sock = socket (adrfam, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		Con_Printf ("qtvreverse: socket: %s\n", strerror(neterrno()));
		return;
	}
	if (connect(sock, (void*)&remote, adrsz) == INVALID_SOCKET)
	{
		closesocket(sock);
		Con_Printf ("qtvreverse: connect: %s\n", strerror(neterrno()));
		return;
	}

	if (ioctlsocket (sock, FIONBIO, (u_long *)&nonblocking) == INVALID_SOCKET)
	{
		closesocket(sock);
		Con_Printf ("qtvreverse: ioctl FIONBIO: %s\n", strerror(neterrno()));
		return;
	}

	data =	"QTV\n"
			"REVERSE\n"
			"\n";
	if (send(sock, data, strlen(data), 0) == INVALID_SOCKET)
	{
		closesocket(sock);
		Con_Printf ("qtvreverse: send: %s\n", strerror(neterrno()));
		return;
	}


	SV_MVD_InitPendingStream(sock, ip)->isreverse = true;
}

	//SV_MVD_Record (dest);
#endif
}

/*
====================
SV_EasyRecord_f

easyrecord [demoname]
====================
*/

int	Dem_CountPlayers (void)
{
	int	i, count;

	count = 0;
	for (i = 0; i < sv.allocated_client_slots ; i++)
	{
		if (svs.clients[i].name[0] && !svs.clients[i].spectator)
			count++;
	}

	return count;
}

char *Dem_Team(int num)
{
	int i;
	static char *lastteam[2];
	qboolean first = true;
	client_t *client;
	static int index = 0;

	index = 1 - index;

	for (i = 0, client = svs.clients; num && i < sv.allocated_client_slots; i++, client++)
	{
		if (!client->name[0] || client->spectator)
			continue;

		if (first || strcmp(lastteam[index], InfoBuf_ValueForKey(&client->userinfo, "team")))
		{
			first = false;
			num--;
			lastteam[index] = InfoBuf_ValueForKey(&client->userinfo, "team");
		}
	}

	if (num)
		return "";

	return lastteam[index];
}

char *Dem_PlayerName(int num)
{
	int i;
	client_t *client;

	for (i = 0, client = svs.clients; i < sv.allocated_client_slots; i++, client++)
	{
		if (!client->name[0] || client->spectator)
			continue;

		if (!--num)
			return client->name;
	}

	return "";
}

// -> scream
char *Dem_PlayerNameTeam(char *t)
{
	int	i;
	client_t *client;
	static char	n[1024];
	int	sep;

	n[0] = 0;

	sep = 0;

	for (i = 0, client = svs.clients; i < sv.allocated_client_slots; i++, client++)
	{
		if (!client->name[0] || client->spectator)
			continue;

		if (strcmp(t, InfoBuf_ValueForKey(&client->userinfo, "team"))==0)
		{
			if (sep >= 1)
				Q_strncatz (n, "_", sizeof(n));
//				snprintf (n, sizeof(n), "%s_", n);
			Q_strncatz (n, client->name, sizeof(n));
//			snprintf (n, sizeof(n),"%s%s", n, client->name);
			sep++;
		}
	}

	return n;
}

int	Dem_CountTeamPlayers (char *t)
{
	int	i, count;

	count = 0;
	for (i = 0; i < sv.allocated_client_slots ; i++)
	{
		if (svs.clients[i].name[0] && !svs.clients[i].spectator)
			if (strcmp(InfoBuf_ValueForKey(&svs.clients[i].userinfo, "team"), t)==0)
				count++;
	}

	return count;
}

//takes a quake-mark-up string (subject to com_parseutf and ^ etc) and spits out a usable utf-8 name
//in and out may overlap.
static char *FS_UTF8FromQuakeFilename(const char *in, qboolean dequake, qboolean keepmarkup, qboolean blockdirsep, char *out, size_t outsize)
{
	conchar_t cline[8192], *c;
	char *outend = out+outsize-1;	//-1 for our null
	unsigned int charflags, codepoint;

	COM_ParseFunString(CON_WHITEMASK, in, cline, sizeof(cline), keepmarkup);
	for (c = cline; *c; )
	{
		c = Font_Decode(c, &charflags, &codepoint);
		if (charflags & CON_HIDDEN)
			continue;
		if (dequake)
			codepoint = COM_DeQuake(codepoint);

		if (codepoint == '/' && blockdirsep) codepoint = '-';	//spreading across multiple dirs is just awkward
		else if (codepoint <  ' ' )	codepoint = '-';	//C0 chars are all kinds of problematic
		else if (codepoint == '\\')	codepoint = '-';	//windows sucks. or string escapes do.
		else if (codepoint == ':' )	codepoint = '-';	//drives, or Alternative Data Streams (read: often hidden)
		else if (codepoint == '\"')	codepoint = '-';	//erk! escapes necessitate escapes...
		else if (codepoint == '<' )	codepoint = '-';	//pipe stuff sucks
		else if (codepoint == '>' )	codepoint = '-';	//pipe stuff sucks
		else if (codepoint == '|' )	codepoint = '-';	//pipe stuff sucks
		else if (codepoint == '?' )	codepoint = '-';	//wildcards complicate things
		else if (codepoint == '*' )	codepoint = '-';	//wildcards complicate things

		out += utf8_encode(out, codepoint, outend-out);
	}
	*out = 0;
	return out;
}
// <-

void SV_MVDEasyRecord_f (void)
{
	int		c;
	char	name[1024];
	char	name2[MAX_OSPATH*7]; // scream
	//char	name2[MAX_OSPATH*2];
	int		i;
	vfsfile_t	*f;

	c = Cmd_Argc();
	if (c > 2)
	{
		Con_Printf ("easyrecord [demoname]\n");
		return;
	}

	if (sv.state < ss_active)
	{
		Con_Printf("Server isn't running or is still loading\n");
		return;
	}

	if (!MVD_CheckSpace(Cmd_FromGamecode()))
		return;

	if (c == 2)
	{
		Q_strncpyz (name, Cmd_Argv(1), sizeof(name));
		FS_UTF8FromQuakeFilename(name, true, false, false, name, sizeof(name));
	}
	else
	{
		i = Dem_CountPlayers();
		/*if (!deathmatch.ival)
		{
			if (coop.ival || i>1)
				snprintf (name, sizeof(name), "coop_%s_%d(%d)", sv.name, skill.ival, i);
			else
				snprintf (name, sizeof(name), "sp_%s_%d_%s", sv.name, skill.ival, Dem_PlayerName(0));
		}
		else*/ if (teamplay.value >= 1 && i > 2)
		{
			// Teamplay
			snprintf (name, sizeof(name), "%don%d_", Dem_CountTeamPlayers(Dem_Team(1)), Dem_CountTeamPlayers(Dem_Team(2)));
			if (sv_demoExtraNames.value > 0)
			{
				Q_strncatz (name, va("[%s]_%s_vs_[%s]_%s_%s",
									Dem_Team(1), Dem_PlayerNameTeam(Dem_Team(1)),
									Dem_Team(2), Dem_PlayerNameTeam(Dem_Team(2)),
									svs.name), sizeof(name));
			} else
				Q_strncatz (name, va("%s_vs_%s_%s", Dem_Team(1), Dem_Team(2), svs.name), sizeof(name));
		} else {
			if (i == 2) {
				// Duel
				snprintf (name, sizeof(name), "duel_%s_vs_%s_%s",
					Dem_PlayerName(1),
					Dem_PlayerName(2),
					svs.name);
			} else {
				// FFA
				snprintf (name, sizeof(name), "ffa_%s(%d)", svs.name, i);
			}
		}

		//convert to utf-8, so its readable on most systems (we convert utf-8 to utf-16 for windows, while linux tends to just use utf-8 in the first place)
		FS_UTF8FromQuakeFilename(name, true, false, true, name, sizeof(name));
	}

	// <-

// Make sure the filename doesn't contain illegal characters
	Q_strncpyz(name, va("%s%s", sv_demoPrefix.string, SV_CleanName(name)),
			MAX_MVD_NAME - strlen(sv_demoSuffix.string) - 7);
	Q_strncatz(name, sv_demoSuffix.string, sizeof(name));
	Q_strncpyz(name, va("%s/%s", sv_demoDir.string, name), sizeof(name));
// find a filename that doesn't exist yet
	Q_strncpyz(name2, name, sizeof(name2));
//	COM_StripExtension(name2, name2);
	FS_CreatePath (name2, FS_GAMEONLY);
	Q_strncatz(name2, ".mvd", sizeof(name2));
	if ((f = FS_OpenVFS(name2, "rb", FS_GAMEONLY)) == 0)
		f = FS_OpenVFS(va("%s.gz", name2), "rb", FS_GAMEONLY);

	if (f)
	{
		i = 1;
		do {
			VFS_CLOSE (f);
			snprintf(name2, sizeof(name2), "%s_%02i", name, i);
//			COM_StripExtension(name2, name2);
			Q_strncatz(name2, ".mvd", sizeof(name2));
			if ((f = FS_OpenVFS (name2, "rb", FS_GAMEONLY)) == 0)
				f = FS_OpenVFS(va("%s.gz", name2), "rb", FS_GAMEONLY);
			i++;
		} while (f);
	}

#ifdef AVAIL_GZDEC
	if (sv_demoAutoCompress.ival == 1)
		Q_strncatz(name2, ".gz", sizeof(name2));
#endif
	SV_MVD_Record (SV_MVD_InitRecordFile(name2));
}

//console command for servers/admins
void SV_MVDList_f (void)
{
	char buf[32];
	mvddest_t *d;
	dir_t	*dir;
	file_t	*list;
	int		i,j,show;
	qofs_t maxdirsize = MVD_DemoMaxDirSize();

	Con_Printf("content of %s/*.mvd\n", sv_demoDir.string);
	dir = Sys_listdemos(sv_demoDir.string, true, SORT_BY_DATE);
	list = dir->files;
	if (!dir->numfiles)
	{
		Con_Printf("no demos\n");
	}

	for (i = 1; i <= dir->numfiles; i++, list++)
	{
		for (j = 1; j < Cmd_Argc(); j++)
			if (strstr(list->name, Cmd_Argv(j)) == NULL)
				break;
		show = Cmd_Argc() == j;

		if (show)
		{
			for (d = demo.dest; d; d = d->nextdest)
			{
				if (d->desttype != DEST_STREAM && !strcmp(list->name, d->simplename))
					Con_Printf("*%d: ^[^7%s\\demo\\%s/%s^] %uk\n", i, list->name, sv_demoDir.string, list->name, (unsigned int)(d->totalsize/1024));
			}
			if (!d)
				Con_Printf("%d: ^[^7%s\\demo\\%s/%s^] %uk\n", i, list->name, sv_demoDir.string, list->name, (unsigned int)(list->size/1024));
		}
	}

	for (d = demo.dest; d; d = d->nextdest)
		dir->size += d->totalsize;

	Con_Printf("\ndirectory size: %s\n",FS_AbbreviateSize(buf,sizeof(buf), dir->size));
	if (maxdirsize)
	{
		if (maxdirsize >= dir->size)
			Con_Printf("space available: %s\n", FS_AbbreviateSize(buf,sizeof(buf), maxdirsize - dir->size));
		else
			Con_Printf("limit exceeded by %s\n", FS_AbbreviateSize(buf,sizeof(buf), dir->size - maxdirsize));
	}

	Sys_freedir(dir);
}

//console command used to print to connected clients (we're acting as a dedicated server)
void SV_UserCmdMVDList_f (void)
{
	char buf[32];
	mvddest_t *d;
	dir_t	*dir;
	file_t	*list;
	int		i,j,show;
	qofs_t maxdirsize = MVD_DemoMaxDirSize();

	SV_ClientPrintf(host_client, PRINT_HIGH, "available demos:\n");
	dir = Sys_listdemos(sv_demoDir.string, true, SORT_BY_DATE);
	list = dir->files;
	if (!dir->numfiles)
	{
		SV_ClientPrintf(host_client, PRINT_HIGH, "no demos\n");
	}

	for (i = 1; i <= dir->numfiles; i++, list++)
	{
		for (j = 1; j < Cmd_Argc(); j++)
			if (strstr(list->name, Cmd_Argv(j)) == NULL)
				break;
		show = Cmd_Argc() == j;

		if (show)
		{
			for (d = demo.dest; d; d = d->nextdest)
			{
				if (d->desttype != DEST_STREAM && !strcmp(list->name, d->simplename))
					SV_ClientPrintf(host_client, PRINT_HIGH, "*%d: %s %dk\n", i, list->name, d->totalsize/1024);
			}
			if (!d)
			{
				if (host_client->fteprotocolextensions2 & PEXT_CSQC)	//its a hack to use csqc this way, but oh well, but other clients don't want the gibberish.
					SV_ClientPrintf(host_client, PRINT_HIGH, "%d: ^[%s\\type\\/download demos/%s^] %dk\n", i, list->name, list->name, (unsigned int)(list->size/1024));
				else
					SV_ClientPrintf(host_client, PRINT_HIGH, "%d: %s %dk\n", i, list->name, (unsigned int)(list->size/1024));
			}
		}

		if (host_client->num_backbuf >= MAX_BACK_BUFFERS/2)
		{
			SV_ClientPrintf(host_client, PRINT_HIGH, "*MORE*\n");
			break;
		}
	}

	for (d = demo.dest; d; d = d->nextdest)
		dir->size += d->totalsize;

	SV_ClientPrintf(host_client, PRINT_HIGH, "\ndirectory size: %s\n",FS_AbbreviateSize(buf,sizeof(buf), dir->size));
	if (maxdirsize)
	{
		if (maxdirsize >= dir->size)
			SV_ClientPrintf(host_client, PRINT_HIGH, "space available: %s\n", FS_AbbreviateSize(buf,sizeof(buf), maxdirsize - dir->size));
		else
			SV_ClientPrintf(host_client, PRINT_HIGH, "limit exceeded by %s\n", FS_AbbreviateSize(buf,sizeof(buf), dir->size - maxdirsize));
	}

	Sys_freedir(dir);
}

void SV_UserCmdMVDList_HTML (vfsfile_t *pipe)
{
//#define EMBEDGAME
	mvddest_t *d;
	dir_t	*dir;
	file_t	*list;
	float	f;
	int		i;
	qofs_t maxdirsize = MVD_DemoMaxDirSize();

	VFS_PRINTF(pipe,
		"<html>"
			"<head>"
				"<title>%s - %s</title>"
				"<meta charset='UTF-8'>"
				"<style>"
					".mydiv { width: 20%%; height: 100%%; padding: 0px; margin: 0px; border: 0px solclass #aaaaaa; float:left; }"
					".game { width: 80%%; height: 100%%; padding: 0px; margin: 0px; border: 0px solclass #aaaaaa; float:left; }"
				"</style>"
#ifdef EMBEDGAME
				"<script>"
					"function playdemo(demo)"
					"{"
						"demo = window.location.origin+'/demos/'+demo;"
						"thegame.postMessage({cmd:'playdemo',url:demo}, '*');"
					"}"
				"</script>"
#endif
			"</head>"
			"<body>"
			"<div class='mydiv'>\n"
		, fs_manifest->formalname, hostname.string);

	VFS_PRINTF(pipe, "available demos:<br/>\n");
	dir = Sys_listdemos(sv_demoDir.string, true, SORT_BY_DATE);
	list = dir->files;
	if (!dir->numfiles)
	{
		VFS_PRINTF(pipe, "no demos<br/>\n");
	}

	for (i = 1; i <= dir->numfiles; i++, list++)
	{
		for (d = demo.dest; d; d = d->nextdest)
		{
			if (d->desttype != DEST_STREAM && !strcmp(list->name, d->simplename))
				VFS_PRINTF(pipe, "*%d: %s %dk<br/>\n", i, list->name, d->totalsize/1024);
		}
		if (!d)
		{
			char datetime[64];
			strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime(&list->mtime));
#ifdef EMBEDGAME
			VFS_PRINTF(pipe, "%d: <a href='/demos/%s'>%s</a> %uk <a href='javascript:void(0)' onclick='playdemo(\"%s\")'>play</a> %s<br/>\n", i, list->name, list->name, (unsigned int)(list->size/1024), list->name, datetime);
#else
			VFS_PRINTF(pipe, "%d: <a href='/demos/%s'>%s</a> %uk %s<br/>\n", i, list->name, list->name, (unsigned int)(list->size/1024), datetime);
#endif
		}
	}

	for (d = demo.dest; d; d = d->nextdest)
		dir->size += d->totalsize;

	VFS_PRINTF(pipe, "<br/>\ndirectory size: %.1fMB<br/>\n",(float)dir->size/(1024*1024));
	if (maxdirsize)
	{
		f = (maxdirsize - dir->size)/(1024*1024);
		if ( f < 0)
			f = 0;
		VFS_PRINTF(pipe, "space available: %.1fMB<br/>\n", f);
	}

	VFS_PRINTF(pipe,
				"</div>"
#ifdef EMBEDGAME
				"<div class='game'>"
					"<iframe name='thegame'"	//the name of the game is... thegame!
						" src='"ENGINEWEBSITE"/quake' allowfullscreen=true"
						" frameborder='0' scrolling='no' marginheight='0' marginwidth='0' width='100%%' height='100%%'"
						" onerror=\"alert('Failed to load engine')\">"
					"</iframe>"
				"</div>"
#endif
			"</body>\n"
		"</html>\n");

	Sys_freedir(dir);
}

const char *SV_MVDLastNum(unsigned int num)
{
	if (!num || num > DEMOLOG_LENGTH)
		return NULL;
	num = demolog.sequence - num;
	return demolog.log[num % DEMOLOG_LENGTH].filename;
}
char *SV_MVDNum(char *buffer, int bufferlen, int num)	//lame number->name lookup according to a list generated at an arbitrary time
{
	file_t	*list;
	dir_t	*dir;

	dir = Sys_listdemos(sv_demoDir.string, true, SORT_BY_DATE);
	list = dir->files;

	if (num < 0)
		num = dir->numfiles + num;
	if (num > dir->numfiles || num <= 0)
	{
		Sys_freedir(dir);
		return NULL;
	}
	num--;

	list += num;

	Q_strncpyz(buffer, list->name, bufferlen);
	Sys_freedir(dir);
	return buffer;
}

char *SV_MVDName2Txt(char *name)
{
	char s[MAX_OSPATH];
	const char *ext;

	if (!name)
		return NULL;

	Q_strncpyz(s, name, MAX_OSPATH);

	ext = COM_GetFileExtension(s, NULL);
	if (!Q_strcasecmp(ext, ".gz") || !Q_strcasecmp(ext, ".xz"))
		ext = COM_GetFileExtension(s, ext);
	if (!ext || !*ext)	//if there's no extension on there, then make sure we're pointing to the end of the string.
		ext = s+strlen(s);
	if (ext > s+sizeof(s)-4)	//make sure we don't overflow the buffer by truncating the base/path, ensuring that we don't write some other type of file.
		ext = s+sizeof(s)-4;	//should probably make this an error case and abort instead.
	strcpy((char*)ext, ".txt");

	return va("%s", s);
}

char *SV_MVDTxTNum(char *buffer, int bufferlen, int num)
{
	return SV_MVDName2Txt(SV_MVDNum(buffer, bufferlen, num));
}

void SV_MVDRemove_f (void)
{
	char name[MAX_MVD_NAME], *ptr;
	char path[MAX_OSPATH];
	int i;
	mvddest_t *active;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("%s <demoname> - removes the demo\nrmdemo *<token>   - removes demo with <token> in the name\nrmdemo *          - removes all demos\n", Cmd_Argv(0));
		return;
	}

	ptr = Cmd_Argv(1);
	if (*ptr == '*')
	{
		dir_t *dir;
		file_t *list;

		// remove all demos with specified token
		ptr++;

		dir = Sys_listdemos(sv_demoDir.string, true, SORT_BY_DATE);
		list = dir->files;
		for (i = 0;i < dir->numfiles; list++)
		{
			if (strstr(list->name, ptr))
			{
				mvddest_t *active = SV_FindRecordFile(list->name, NULL);
				if (active)
					SV_MVDStop_f();

				// stop recording first;
				snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, list->name);
				if (FS_Remove(path, FS_GAMEONLY))
				{
					Con_Printf("removing %s...\n", list->name);
					i++;
				}

				FS_Remove(SV_MVDName2Txt(path), FS_GAMEONLY);
			}
		}
		Sys_freedir(dir);

		if (i)
		{
			Con_Printf("%d demos removed\n", i);
		}
		else
		{
			Con_Printf("no matching found\n");
		}

		return;
	}

	Q_strncpyz(name, Cmd_Argv(1), MAX_MVD_NAME);
	COM_DefaultExtension(name, ".mvd", sizeof(name));

	snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);

	active = SV_FindRecordFile(name, NULL);
	if (active)
		SV_MVDStop_f();

	if (FS_Remove(path, FS_GAMEONLY))
	{
		Con_Printf("demo %s successfully removed\n", name);
	}
	else
		Con_Printf("unable to remove demo %s\n", name);

	FS_Remove(SV_MVDName2Txt(path), FS_GAMEONLY);
}

void SV_MVDRemoveNum_f (void)
{
	int		num;
	char namebuf[MAX_QPATH];
	char	*val, *name;
	char path[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Con_Printf("%s <#>\n", Cmd_Argv(0));
		return;
	}

	val = Cmd_Argv(1);
	if ((num = atoi(val)) == 0 && val[0] != '0')
	{
		Con_Printf("%s <#>\n", Cmd_Argv(0));
		return;
	}

	name = SV_MVDNum(namebuf, sizeof(namebuf), num);

	if (name != NULL)
	{
		mvddest_t *active = SV_FindRecordFile(name, NULL);
		if (active)
			SV_MVDStop_f();

		snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);
		if (FS_Remove(path, FS_GAMEONLY))
		{
			Con_Printf("demo %s succesfully removed\n", name);
		}
		else
			Con_Printf("unable to remove demo %s\n", name);

		FS_Remove(SV_MVDName2Txt(path), FS_GAMEONLY);
	}
	else
		Con_Printf("invalid demo num\n");
}

void SV_MVDInfoAdd_f (void)
{
	char namebuf[MAX_QPATH];
	char *name, *args, path[MAX_OSPATH];
	vfsfile_t *f;

	//** is a special hack for ktx
	if (Cmd_Argc() < 3) {
		Con_Printf("%s <demonum> <info string>\n<demonum> = * for currently recorded demo\n", Cmd_Argv(0));
		return;
	}

	if (!strcmp(Cmd_Argv(1), "*") || !strcmp(Cmd_Argv(1), "**"))
	{
		mvddest_t *active = SV_FindRecordFile(NULL, NULL);
		if (!active)
		{
			Con_Printf("Not recording demo!\n");
			return;
		}

		Q_strncpyz(path, SV_MVDName2Txt(active->filename), sizeof(path));
	}
	else
	{
		name = SV_MVDTxTNum(namebuf, sizeof(namebuf), atoi(Cmd_Argv(1)));

		if (!name)
		{
			Con_Printf("invalid demo num\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);
	}

	if ((f = FS_OpenVFS(path, "ab", FS_GAMEONLY)) == NULL)
	{
		Con_Printf("%s: failed to open \"%s\"\n", Cmd_Argv(0), path);
		return;
	}

	if (!strcmp(Cmd_Argv(1), "**"))
	{
		size_t fsize;
		args = FS_LoadMallocFile(Cmd_Argv(2), &fsize);
		if (args)
		{
			VFS_WRITE(f, args, fsize);
			FS_FreeFile(args);
		}
		else
			Con_Printf("%s: failed to open input file\n", Cmd_Argv(0));
	}
	else
	{
		// skip demonum
		args = Cmd_Args();
		while (*args > 32) args++;
		while (*args && *args <= 32) args++;

		VFS_WRITE(f, args, strlen(args));
		VFS_WRITE(f, "\n", 1);
	}
	VFS_FLUSH(f);
	VFS_CLOSE(f);
}

void SV_MVDInfoRemove_f (void)
{
	char namebuf[MAX_QPATH];
	char *name, path[MAX_OSPATH];

	if (Cmd_Argc() < 2)
	{
		Con_Printf("%s <demonum>\n<demonum> = * for currently recorded demo\n", Cmd_Argv(0));
		return;
	}

	if (!strcmp(Cmd_Argv(1), "*"))
	{
		mvddest_t *active = SV_FindRecordFile(NULL, NULL);
		if (!active)
		{
			Con_Printf("Not recording demo!\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s", SV_MVDName2Txt(active->filename));
	}
	else
	{
		name = SV_MVDTxTNum(namebuf, sizeof(namebuf), atoi(Cmd_Argv(1)));

		if (!name)
		{
			Con_Printf("invalid demo num\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);
	}

	if (FS_Remove(path, FS_GAMEONLY))
	{
		FS_FlushFSHashRemoved(path);
		Con_Printf("file removed\n");
	}
	else
		Con_Printf("failed to remove the file\n");

}

static void SV_MVDInfo_f (void)
{	//callable by client, so be careful.
	int len;
	char buf[64];
	vfsfile_t *f = NULL;
	char *name, path[MAX_OSPATH];

	if (Cmd_Argc() < 2)
	{
		Con_Printf("%s <demonum>\n<demonum> = * for currently recorded demo\n", Cmd_Argv(0));
		return;
	}

	if (!strcmp(Cmd_Argv(1), "*"))
	{
		mvddest_t *active = SV_FindRecordFile(NULL, NULL);
		if (!active)
		{
			Con_Printf("Not recording demo!\n");
			return;
		}

		Q_strncpyz(path, SV_MVDName2Txt(active->filename), sizeof(path));
	}
	else
	{
		name = SV_MVDTxTNum(buf, sizeof(buf), atoi(Cmd_Argv(1)));

		if (!name)
		{
			Con_Printf("invalid demo num\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);
	}

	if ((f = FS_OpenVFS(path, "rt", FS_GAMEONLY)) == NULL)
	{
		Con_Printf("(empty)\n");
		return;
	}

	for(;;)
	{
		len = VFS_READ (f, buf, sizeof(buf)-1);
		if (len <= 0)
			break;
		buf[len] = 0;
		Con_Printf("%s", buf);
	}

	VFS_CLOSE(f);
}
void SV_UserMVDInfo_f (void)
{
	SV_BeginRedirect(RD_CLIENT, host_client->language);
	SV_MVDInfo_f();
	SV_EndRedirect();
}






#ifdef SERVER_DEMO_PLAYBACK
void SV_MVDPlayNum_f(void)
{
	char namebuf[MAX_QPATH];
	char *name;
	int		num;
	char	*val;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("%s <#>\n", Cmd_Argv(0));
		return;
	}

	val = Cmd_Argv(1);
	if ((num = atoi(val)) == 0 && val[0] != '0')
	{
		Con_Printf("%s <#>\n", Cmd_Argv(0));
		return;
	}

	name = SV_MVDNum(namebuf, sizeof(namebuf), atoi(val));

	if (name)
		Cbuf_AddText(va("mvdplay %s\n", name), Cmd_ExecLevel);
	else
		Con_Printf("invalid demo num\n");
}
#endif



void SV_MVDInit(void)
{
	MVD_Init();

	//names that conflict with the client and thus only exist in dedicated servers (and thus shouldn't be used by mods that want mvds).
#ifdef SERVERONLY
	Cmd_AddCommand ("record",			SV_MVD_Record_f);
	Cmd_AddCommand ("stop",				SV_MVDStop_f);	//client version should still work for mvds too.
#endif
	//these don't currently conflict, but hey...
	Cmd_AddCommand ("cancel",			SV_MVD_Cancel_f);
	Cmd_AddCommand ("easyrecord",		SV_MVDEasyRecord_f);
	Cmd_AddCommand ("demolist",			SV_MVDList_f);
	Cmd_AddCommand ("rmdemo",			SV_MVDRemove_f);
	Cmd_AddCommand ("rmdemonum",		SV_MVDRemoveNum_f);

	//serverside only names that won't conflict (matching mvdsv)
	Cmd_AddCommand ("sv_demorecord",	SV_MVD_Record_f);
	Cmd_AddCommand ("sv_demostop",		SV_MVDStop_f);
	Cmd_AddCommand ("sv_democancel",	SV_MVD_Cancel_f);
	Cmd_AddCommand ("sv_demoeasyrecord",SV_MVDEasyRecord_f);
	Cmd_AddCommand ("sv_demolist",		SV_MVDList_f);
	Cmd_AddCommand ("sv_demoremove",	SV_MVDRemove_f);
	Cmd_AddCommand ("sv_demonumremove",	SV_MVDRemoveNum_f);

	//old fte names to avoid conflicts.
	Cmd_AddCommand ("mvdrecord",		SV_MVD_Record_f);
	Cmd_AddCommand ("mvdstop",			SV_MVDStop_f);
	Cmd_AddCommand ("mvdcancel",		SV_MVD_Cancel_f);
	Cmd_AddCommand ("mvdlist",			SV_MVDList_f);
#ifdef SERVER_DEMO_PLAYBACK
	Cmd_AddCommand ("mvdplaynum",		SV_MVDPlayNum_f);
#endif

	Cmd_AddCommand ("sv_demoinfoadd",	SV_MVDInfoAdd_f);
	Cmd_AddCommand ("sv_demoinforemove",SV_MVDInfoRemove_f);
	Cmd_AddCommand ("sv_demoinfo",		SV_MVDInfo_f);

	Cmd_AddCommand ("qtvreverse",		SV_MVD_QTVReverse_f);
	Cvar_Register(&qtv_maxstreams, "MVD Streaming");
	Cvar_Register(&qtv_password, "MVD Streaming");
}

#endif
#endif
