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

#include "qtv.h"
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "bsd_string.h"

#define MAX_INFO_KEY 64

#if defined(SVNREVISION) && defined(SVNDATE)
	#define QTVBUILD STRINGIFY(SVNREVISION)", "STRINGIFY(SVNDATE)
#elif defined(SVNREVISION)
	#define QTVBUILD STRINGIFY(SVNREVISION)", "__DATE__
#else
	#define QTVBUILD __DATE__
#endif

//I apologise for this if it breaks your formatting or anything
#define HELPSTRING "\
FTEQTV proxy commands: (build "QTVBUILD")\n\
----------------------\n\
connect, qtv, addserver\n\
  connect to a MVD stream (TCP)\n\
qtvlist\n\
  lists available streams on a proxy\n\
qw\n\
  connect to a server as a player (UDP)\n\
adddemo\n\
  play a demo from a MVD file\n\
port\n\
  UDP port for QuakeWorld client connections\n\
mvdport\n\
  specify TCP port for MVD broadcasting\n\
maxviewers, maxproxies\n\
  limit number of connections\n\
status, choke, late, talking, nobsp, reconnect, exec, password, master, hostname, record, stop, quit\n\
  other random commands\n\
\n"





char *Info_ValueForKey (char *s, const char *key, char *buffer, int buffersize)
{
	char	pkey[1024];
	char	*o;

	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				*buffer='\0';
				return buffer;
			}
			*o++ = *s++;
			if (o+2 >= pkey+sizeof(pkey))	//hrm. hackers at work..
			{
				*buffer='\0';
				return buffer;
			}
		}
		*o = 0;
		s++;

		o = buffer;

		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				*buffer='\0';
				return buffer;
			}
			*o++ = *s++;

			if (o+2 >= buffer+buffersize)	//hrm. hackers at work..
			{
				*buffer='\0';
				return buffer;
			}
		}
		*o = 0;

		if (!strcmp (key, pkey) )
			return buffer;

		if (!*s)
		{
			*buffer='\0';
			return buffer;
		}
		s++;
	}
}

void Info_RemoveKey (char *s, const char *key)
{
	char	*start;
	char	pkey[1024];
	char	value[1024];
	char	*o;

	if (strstr (key, "\\"))
	{
//		printf ("Key has a slash\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			//strip out the value by copying the next string over the top of this one
			//(we were using strcpy, but valgrind moans and glibc fucks it up. they should not overlap... so use our own crappy inefficient alternative)
			while(*s)
				*start++ = *s++;
			*start = 0;
			return;
		}

		if (!*s)
			return;
	}

}

void Info_SetValueForStarKey (char *s, const char *key, const char *value, int maxsize)
{
	char	newv[1024], *v;
	int		c;
#ifdef SERVERONLY
	extern cvar_t sv_highchars;
#endif

	if (strstr (key, "\\") || strstr (value, "\\") )
	{
//		printf ("Key has a slash\n");
		return;
	}

	if (strstr (key, "\"") || strstr (value, "\"") )
	{
//		printf ("Key has a quote\n");
		return;
	}

	if (strlen(key) >= MAX_INFO_KEY || strlen(value) >= MAX_INFO_KEY)
	{
//		printf ("Key or value is too long\n");
		return;
	}

	// this next line is kinda trippy
	if (*(v = Info_ValueForKey(s, key, newv, sizeof(newv))))
	{
		// key exists, make sure we have enough room for new value, if we don't,
		// don't change it!
		if (strlen(value) - strlen(v) + strlen(s) + 1 > maxsize)
		{
	//		Con_Printf ("Info string length exceeded\n");
			return;
		}
	}


	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	snprintf (newv, sizeof(newv)-1, "\\%s\\%s", key, value);

	if ((int)(strlen(newv) + strlen(s) + 1) > maxsize)
	{
//		printf ("info buffer is too small\n");
		return;
	}

	// only copy ascii values
	s += strlen(s);
	v = newv;
	while (*v)
	{
		c = (unsigned char)*v++;

//		c &= 127;		// strip high bits
		if (c > 13) // && c < 127)
			*s++ = c;
	}
	*s = 0;
}




#define DEFAULT_PUNCTUATION "(,{})(\':;=!><&|+"

char *COM_ParseToken (char *data, char *out, int outsize, const char *punctuation)
{
	int		c;
	int		len;

	if (!punctuation)
		punctuation = DEFAULT_PUNCTUATION;

	len = 0;
	out[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
		else if (data[1] == '*')
		{
			data+=2;
			while (*data && (*data != '*' || data[1] != '/'))
				data++;
			data+=2;
			goto skipwhite;
		}
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if (len >= outsize-1)
			{
				out[len] = '\0';
				return data;
			}
			c = *data++;
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return data;
			}
			out[len] = c;
			len++;
		}
	}

// parse single characters
	if (strchr(punctuation, c))
	{
		out[len] = c;
		len++;
		out[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		if (len >= outsize-1)
			break;
		out[len] = c;
		data++;
		len++;
		c = *data;
		if (strchr(punctuation, c))
			break;
	} while (c>32);

	out[len] = 0;
	return data;
}




void Cmd_Printf(cmdctxt_t *ctx, char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];

	va_start (argptr, fmt);
	vsnprintf (string, sizeof(string)-1, fmt,argptr);
	string[sizeof(string)-1] = 0;
	va_end (argptr);

	if (ctx->printfunc)
		ctx->printfunc(ctx, string);
	else if (ctx->qtv)
		QTV_Printf(ctx->qtv, "%s", string);
	else
		Sys_Printf(ctx->cluster, "%s", string);
}

void Cmd_Hostname(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		if (*ctx->cluster->hostname)
			Cmd_Printf(ctx, "Current hostname is \"%s\"\n", ctx->cluster->hostname);
		else
			Cmd_Printf(ctx, "No master server is currently set.\n");
	}
	else
	{
		strlcpy(ctx->cluster->hostname, Cmd_Argv(ctx, 1), sizeof(ctx->cluster->hostname));

		Cmd_Printf(ctx, "hostname set to \"%s\"\n", ctx->cluster->hostname);
	}
}

void Cmd_Master(cmdctxt_t *ctx)
{
	SOCKET s;
	char *newval = Cmd_Argv(ctx, 1);
	netadr_t addr;

	if (Cmd_Argc(ctx) < 2)
	{
		if (*ctx->cluster->master)
			Cmd_Printf(ctx, "Subscribed to a master server (use '-' to clear)\n");
		else
			Cmd_Printf(ctx, "No master server is currently set.\n");
		return;
	}

	if (!strcmp(newval, "-"))
	{
		strlcpy(ctx->cluster->master, "", sizeof(ctx->cluster->master));
		Cmd_Printf(ctx, "Master server cleared\n");
		return;
	}

	if (!NET_StringToAddr(newval, &addr, 27000))	//send a ping like a qw server does. this is kinda pointless of course.
	{
		Cmd_Printf(ctx, "Couldn't resolve address\n");
		return;
	}

	strlcpy(ctx->cluster->master, newval, sizeof(ctx->cluster->master));
	ctx->cluster->mastersendtime = ctx->cluster->curtime;

	s = NET_ChooseSocket(ctx->cluster->qwdsocket, &addr, addr);
	if (s != INVALID_SOCKET)
		NET_SendPacket (ctx->cluster, s, 1, "k", addr);
	Cmd_Printf(ctx, "Master server set.\n");
}

void Cmd_UDPPort(cmdctxt_t *ctx)
{
	int newp = atoi(Cmd_Argv(ctx, 1));
	ctx->cluster->qwlistenportnum = newp;
	NET_InitUDPSocket(ctx->cluster, newp, SG_IPV6);
	NET_InitUDPSocket(ctx->cluster, newp, SG_IPV4);
}
void Cmd_AdminPassword(cmdctxt_t *ctx)
{
	if (!Cmd_IsLocal(ctx))
	{
		Cmd_Printf(ctx, "Rejecting remote password change.\n");
		return;
	}

	if (Cmd_Argc(ctx) < 2)
	{
		if (*ctx->cluster->adminpassword)
			Cmd_Printf(ctx, "An admin password is currently set\n");
		else
			Cmd_Printf(ctx, "No admin password is currently set\n");
	}
	else
	{
		strlcpy(ctx->cluster->adminpassword, Cmd_Argv(ctx, 1), sizeof(ctx->cluster->adminpassword));
		Cmd_Printf(ctx, "Password changed.\n");
	}
}

void Cmd_GenericQuery(cmdctxt_t *ctx, int dataset)
{
	char *method = "tcp:";
	char *address, *password;
	if (Cmd_Argc(ctx) < 2)
	{
		Cmd_Printf(ctx, "%s requires an ip:port parameter\n", Cmd_Argv(ctx, 0));
		return;
	}

	address = Cmd_Argv(ctx, 1);
	password = Cmd_Argv(ctx, 2);
	//this is evil
	/* Holy crap, Spike... Holy crap. */
	memmove(address+strlen(method), address, ARG_LEN-(1+strlen(method)));
	strncpy(address, method, strlen(method));

	if (!QTV_NewServerConnection(ctx->cluster, ctx->streamid, address, password, false, AD_NO, false, dataset))
		Cmd_Printf(ctx, "Failed to connect to \"%s\", connection aborted\n", address);

	Cmd_Printf(ctx, "Querying \"%s\"\n", address);
}


void Cmd_QTVList(cmdctxt_t *ctx)
{
	Cmd_GenericQuery(ctx, 1);
}
void Cmd_QTVDemoList(cmdctxt_t *ctx)
{
	Cmd_GenericQuery(ctx, 2);
}

void Cmd_GenericConnect(cmdctxt_t *ctx, char *method, enum autodisconnect_e autoclose)
{
	sv_t *sv;
	char *address, *password;
	if (Cmd_Argc(ctx) < 2)
	{
		if (!strncmp(method, "file", 4))
			Cmd_Printf(ctx, "%s requires a demo name parameter\n", Cmd_Argv(ctx, 0));
		else if (!strncmp(method, "dir", 3))
			Cmd_Printf(ctx, "%s requires a demo directory parameter\n", Cmd_Argv(ctx, 0));
		else
			Cmd_Printf(ctx, "%s requires an ip:port parameter\n", Cmd_Argv(ctx, 0));
		return;
	}

	address = Cmd_Argv(ctx, 1);
	password = Cmd_Argv(ctx, 2);
	//this is evil
	/* Holy crap, Spike... Holy crap. */
	memmove(address+strlen(method), address, ARG_LEN-(1+strlen(method)));
	strncpy(address, method, strlen(method));

	sv = QTV_NewServerConnection(ctx->cluster, ctx->streamid?ctx->streamid:1, address, password, false, autoclose, false, false);
	if (!sv)
		Cmd_Printf(ctx, "Failed to connect to \"%s\", connection aborted\n", address);
	else
		Cmd_Printf(ctx, "Source registered \"%s\" as stream %i\n", address, sv->streamid);
}

void Cmd_QTVConnect(cmdctxt_t *ctx)
{
	Cmd_GenericConnect(ctx, "tcp:", AD_NO);
}
void Cmd_QWConnect(cmdctxt_t *ctx)
{
	Cmd_GenericConnect(ctx, "udp:", AD_STATUSPOLL);
}
void Cmd_MVDConnect(cmdctxt_t *ctx)
{
	Cmd_GenericConnect(ctx, "file:", AD_NO);
}
void Cmd_DirMVDConnect(cmdctxt_t *ctx)
{
	srand(time(NULL));
	Cmd_GenericConnect(ctx, "dir:", AD_NO);
}

void Cmd_Exec(cmdctxt_t *ctx)
{
	FILE *f;
	char line[512], *l;
	char *fname = Cmd_Argv(ctx, 1);

	if (!Cmd_IsLocal(ctx))
	{
		if (*fname == '\\' || *fname == '/' || strstr(fname, "..") || fname[1] == ':')
		{
			Cmd_Printf(ctx, "Absolute paths are prohibited.\n");
			return;
		}
		if (!strncmp(fname, "usercfg/", 8))	//this is how we stop users from execing a 50gb pk3..
		{
			Cmd_Printf(ctx, "Remote-execed configs must be in the usercfg directory\n");
			return;
		}
	}

	f = fopen(fname, "rt");
	if (!f)
	{
		Cmd_Printf(ctx, "Couldn't exec \"%s\"\n", fname);
		return;
	}
	else
	{
		Cmd_Printf(ctx, "Execing \"%s\"\n", fname);
		while(fgets(line, sizeof(line)-1, f))
		{
			l = line;
			while(*(unsigned char*)l <= ' ' && *l)
				l++;
			if (*l && l[0] != '/' && l[1] != '/')
			{
				Cmd_ExecuteNow(ctx, l);
			}
		}
		fclose(f);
	}
}

void catbuffer(char *buffer, int bufsize, char *format, ...)
{
	va_list argptr;
	unsigned int buflen;

	buflen = strlen(buffer);

	va_start(argptr, format);
	vsnprintf(buffer + buflen, bufsize - buflen, format, argptr);
	va_end(argptr);
}

void Cmd_Say(cmdctxt_t *ctx)
{
	int i;
	viewer_t *v;
	char message[8192];
	message[0] = '\0';

	for (i = 1; i < Cmd_Argc(ctx); i++)
		catbuffer(message, sizeof(message)-1, "%s%s", i==1?"":" ", Cmd_Argv(ctx, i));

	if (ctx->qtv)
	{
		if (!SV_SayToUpstream(ctx->qtv, message))
			SV_SayToViewers(ctx->qtv, message);
	}
	else
	{
		//we don't have to remember the client proxies here... no streams = no active client proxies
		for (v = ctx->cluster->viewers; v; v = v->next)
		{
			QW_PrintfToViewer(v, "proxy: %s\n", message);
		}
	}

	Cmd_Printf(ctx, "proxy: %s\n", message);
}

void Cmd_Status(cmdctxt_t *ctx)
{
	Cmd_Printf(ctx, "QTV Status:\n");
	Cmd_Printf(ctx, " %i sources%s\n", ctx->cluster->numservers, ctx->cluster->nouserconnects?" (admin only)":" (user allowed)");
	Cmd_Printf(ctx, " %i udp clients %s\n", ctx->cluster->numviewers, ctx->cluster->allownqclients?" (qw+nq)":" (qw only)");
	if (ctx->cluster->maxproxies)
		Cmd_Printf(ctx, " %i tcp clients (of %i)\n", ctx->cluster->numproxies, ctx->cluster->maxproxies);
	else
		Cmd_Printf(ctx, " %i tcp clients\n", ctx->cluster->numproxies);
	TURN_RelayStatus(ctx);

	Cmd_Printf(ctx, "Common Options:\n");
	Cmd_Printf(ctx, " Hostname %s\n", ctx->cluster->hostname);

	if (ctx->cluster->chokeonnotupdated)
		Cmd_Printf(ctx, " Choke\n");
	if (ctx->cluster->lateforward)
		Cmd_Printf(ctx, " Late forwarding (delayed streams)\n");
	if (!ctx->cluster->notalking)
		Cmd_Printf(ctx, " Talking allowed\n");
	if (ctx->cluster->nobsp)
		Cmd_Printf(ctx, " No BSP loading\n");
	if (ctx->cluster->tcpsocket[SG_UNIX] != INVALID_SOCKET)
		Cmd_Printf(ctx, " unix socket open\n");
	if (ctx->cluster->tcpsocket[SG_IPV4] != INVALID_SOCKET || ctx->cluster->tcpsocket[SG_IPV6] != INVALID_SOCKET)
		Cmd_Printf(ctx, " tcp port %i\n", ctx->cluster->tcplistenportnum);
	if (ctx->cluster->qwdsocket[SG_IPV4] != INVALID_SOCKET || ctx->cluster->qwdsocket[SG_IPV6] != INVALID_SOCKET)
		Cmd_Printf(ctx, " udp port %i\n", ctx->cluster->qwlistenportnum);
	Cmd_Printf(ctx, "\n");


	if (ctx->qtv)
	{
		Cmd_Printf(ctx, "Selected server: %s\n", ctx->qtv->server);
		if (ctx->qtv->sourcefile)
			Cmd_Printf(ctx, " Playing from file\n");
		if (ctx->qtv->sourcesock != INVALID_SOCKET)
			Cmd_Printf(ctx, " Connected\n");
		if (ctx->qtv->parsingqtvheader || ctx->qtv->parsingconnectiondata)
			Cmd_Printf(ctx, " Waiting for gamestate\n");
		if (ctx->qtv->usequakeworldprotocols)
		{
			Cmd_Printf(ctx, " QuakeWorld protocols\n");
			if (ctx->qtv->controller)
			{
				Cmd_Printf(ctx, " Controlled by %s\n", ctx->qtv->controller->name);
			}
		}
		else if (ctx->qtv->sourcesock == INVALID_SOCKET && !ctx->qtv->sourcefile)
			Cmd_Printf(ctx, " Connection not established\n");

		if (*ctx->qtv->map.modellist[1].name)
		{
			Cmd_Printf(ctx, " Map name %s\n", ctx->qtv->map.modellist[1].name);
		}
		if (*ctx->qtv->connectpassword)
			Cmd_Printf(ctx, " Using a password\n");

		if (ctx->qtv->errored == ERR_DISABLED)
			Cmd_Printf(ctx, " Stream is disabled\n");

		if (ctx->qtv->autodisconnect == AD_WHENEMPTY)
			Cmd_Printf(ctx, " Stream is user created\n");
		else if (ctx->qtv->autodisconnect == AD_REVERSECONNECT)
			Cmd_Printf(ctx, " Stream is server created\n");

/*		if (ctx->qtv->tcpsocket != INVALID_SOCKET)
		{
			Cmd_Printf(ctx, " Listening for proxies (%i)\n", ctx->qtv->tcplistenportnum);
		}
*/

		if (ctx->qtv->map.bsp)
		{
			Cmd_Printf(ctx, " BSP (%s) is loaded\n", ctx->qtv->map.mapname);
		}
	}

}

void Cmd_UserConnects(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		Cmd_Printf(ctx, "userconnects is set to %i\n", !ctx->cluster->nouserconnects);
	}
	else
	{
		ctx->cluster->nouserconnects = !atoi(Cmd_Argv(ctx, 1));
		Cmd_Printf(ctx, "userconnects is now %i\n", !ctx->cluster->nouserconnects);
	}
}
void Cmd_Choke(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		if (ctx->cluster->chokeonnotupdated)
			Cmd_Printf(ctx, "proxy will not interpolate packets\n");
		else
			Cmd_Printf(ctx, "proxy will smooth action at the expense of extra packets\n");
		return;
	}
	ctx->cluster->chokeonnotupdated = !!atoi(Cmd_Argv(ctx, 1));
	Cmd_Printf(ctx, "choke-until-update set to %i\n", ctx->cluster->chokeonnotupdated);
}
void Cmd_Late(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		if (ctx->cluster->lateforward)
			Cmd_Printf(ctx, "forwarded streams will be artificially delayed\n");
		else
			Cmd_Printf(ctx, "forwarded streams are forwarded immediatly\n");
		return;
	}
	ctx->cluster->lateforward = !!atoi(Cmd_Argv(ctx, 1));
	Cmd_Printf(ctx, "late forwarding set\n");
}
void Cmd_ReverseAllowed(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) >= 2)
		ctx->cluster->reverseallowed = !!atoi(Cmd_Argv(ctx, 1));
	Cmd_Printf(ctx, "reverse connections are %s\n", ctx->cluster->reverseallowed?"enabled":"disabled");
}

void Cmd_Talking(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		if (ctx->cluster->notalking)
			Cmd_Printf(ctx, "viewers may not talk\n");
		else
			Cmd_Printf(ctx, "viewers may talk freely\n");
		return;
	}
	ctx->cluster->notalking = !atoi(Cmd_Argv(ctx, 1));
	Cmd_Printf(ctx, "talking permissions set\n");
}
void Cmd_NoBSP(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		if (ctx->cluster->nobsp)
			Cmd_Printf(ctx, "no bsps will be loaded\n");
		else
			Cmd_Printf(ctx, "attempting to load bsp files\n");
	}
	else
	{
		ctx->cluster->nobsp = !!atoi(val);
		Cmd_Printf(ctx, "nobsp will change at start of next map\n");
	}
}

void Cmd_MaxViewers(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		if (ctx->cluster->maxviewers)
			Cmd_Printf(ctx, "maxviewers is currently %i\n", ctx->cluster->maxviewers);
		else
			Cmd_Printf(ctx, "maxviewers is currently unlimited\n");
	}
	else
	{
		ctx->cluster->maxviewers = atoi(val);
		Cmd_Printf(ctx, "maxviewers set\n");
	}
}
void Cmd_AllowNQ(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		Cmd_Printf(ctx, "allownq is currently %i\n", ctx->cluster->allownqclients);
	}
	else
	{
		ctx->cluster->allownqclients = !!atoi(val);
		Cmd_Printf(ctx, "allownq set\n");
	}
}

void Cmd_InitialDelay(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		Cmd_Printf(ctx, "initialdelay is currently %g seconds\n", ctx->cluster->anticheattime/1000.f);
	}
	else
	{
		ctx->cluster->anticheattime = atof(val)*1000;
		if (ctx->cluster->anticheattime < 1)
			ctx->cluster->anticheattime = 1;
		Cmd_Printf(ctx, "initialdelay set\n");
	}
}

void Cmd_SlowDelay(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		Cmd_Printf(ctx, "slowdelay is currently %g seconds\n", ctx->cluster->tooslowdelay/1000.f);
	}
	else
	{
		ctx->cluster->tooslowdelay = atof(val)*1000;
		if (ctx->cluster->tooslowdelay < 1)
			ctx->cluster->tooslowdelay = 1;
		Cmd_Printf(ctx, "slowdelay set\n");
	}
}

void Cmd_MaxProxies(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		if (ctx->cluster->maxproxies)
			Cmd_Printf(ctx, "maxproxies is currently %i\n", ctx->cluster->maxproxies);
		else
			Cmd_Printf(ctx, "maxproxies is currently unlimited\n");
	}
	else
	{
		ctx->cluster->maxproxies = atoi(val);
		Cmd_Printf(ctx, "maxproxies set\n");
	}
}


void Cmd_Ping(cmdctxt_t *ctx)
{
	netadr_t addr;
	char *val = Cmd_Argv(ctx, 1);
	if (NET_StringToAddr(val, &addr, 27500))
	{
		NET_SendPacket (ctx->cluster, NET_ChooseSocket(ctx->cluster->qwdsocket, &addr, addr), 1, "k", addr);
		Cmd_Printf(ctx, "pinged\n");
	}
	Cmd_Printf(ctx, "couldn't resolve\n");
}

void Cmd_Help(cmdctxt_t *ctx)
{
	Cmd_Printf(ctx, HELPSTRING);
}

void Cmd_Echo(cmdctxt_t *ctx)
{
	Cmd_Printf(ctx, "%s", Cmd_Argv(ctx, 1));
}

void Cmd_Quit(cmdctxt_t *ctx)
{
	if (!Cmd_IsLocal(ctx))
		Cmd_Printf(ctx, "Remote shutdown refused.\n");
	ctx->cluster->wanttoexit = true;
	Cmd_Printf(ctx, "Shutting down.\n");
}

















void Cmd_Streams(cmdctxt_t *ctx)
{
	sv_t *qtv;
	char *status;
	Cmd_Printf(ctx, "Streams:\n");

	for (qtv = ctx->cluster->servers; qtv; qtv = qtv->next)
	{
		switch (qtv->errored)
		{
		case ERR_NONE:
			if (qtv->controller)
				status = " (player controlled)";
			else if (qtv->autodisconnect == AD_STATUSPOLL)
				status = " (polling)";
			else if (qtv->parsingconnectiondata)
				status = " (connecting)";
			else
				status = "";
			break;
		case ERR_PAUSED:
			status = " (paused)";
			break;
		case ERR_DISABLED:
			status = " (disabled)";
			break;
		case ERR_DROP:	//a user should never normally see this, but there is a chance
			status = " (dropping)";
			break;
		case ERR_RECONNECT:	//again, rare
			status = " (reconnecting)";
			break;
		default:	//some other kind of error, transitioning
			status = " (errored)";
			break;
		}
		Cmd_Printf(ctx, "%i: %s%s\n", qtv->streamid, qtv->server, status);

		if (qtv->upstreamacceptschat)
			Cmd_Printf(ctx, "  (dbg) can chat!\n");
	}
}




void Cmd_DemoSpeed(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (*val)
	{
		ctx->qtv->parsespeed = atof(val)*1000;
		Cmd_Printf(ctx, "Setting demo speed to %f\n", ctx->qtv->parsespeed/1000.0f);
	}
	else
		Cmd_Printf(ctx, "Playing demo at %f speed\n", ctx->qtv->parsespeed/1000.0f);
}

void Cmd_Disconnect(cmdctxt_t *ctx)
{
	QTV_ShutdownStream(ctx->qtv);
	Cmd_Printf(ctx, "Disconnected\n");
}

void Cmd_Halt(cmdctxt_t *ctx)
{
	if (ctx->qtv->errored == ERR_DISABLED || ctx->qtv->errored == ERR_PERMANENT)
	{
		Cmd_Printf(ctx, "Stream is already halted\n");
	}
	else
	{
		ctx->qtv->errored = ERR_PERMANENT;
		Cmd_Printf(ctx, "Stream will disconnect\n");
	}
}

void Cmd_Pause(cmdctxt_t *ctx)
{
	if (ctx->qtv->errored == ERR_PAUSED)
	{
		ctx->qtv->errored = ERR_NONE;
		Cmd_Printf(ctx, "Stream unpaused.\n");
	}
	else if (ctx->qtv->errored == ERR_NONE)
	{
		if (ctx->qtv->sourcetype == SRC_DEMO)
		{
			ctx->qtv->errored = ERR_PAUSED;
			Cmd_Printf(ctx, "Stream paused.\n");
		}
		else
			Cmd_Printf(ctx, "Sorry, only demos may be paused.\n");
	}
}

void Cmd_Resume(cmdctxt_t *ctx)
{
	if (ctx->qtv->errored == ERR_PAUSED)
	{
		ctx->qtv->errored = ERR_NONE;
		Cmd_Printf(ctx, "Stream unpaused.\n");
	}
	else
	{
		if (ctx->qtv->errored == ERR_NONE)
			Cmd_Printf(ctx, "Stream is already functional\n");

		ctx->qtv->errored = ERR_RECONNECT;
		Cmd_Printf(ctx, "Stream will attempt to reconnect\n");
	}
}

void Cmd_Record(cmdctxt_t *ctx)
{
	char *fname = Cmd_Argv(ctx, 1);
	if (!*fname)
		Cmd_Printf(ctx, "record requires a filename on the proxy's machine\n");

	if (!Cmd_IsLocal(ctx))
	{
		if (*fname == '\\' || *fname == '/' || strstr(fname, "..") || fname[1] == ':')
		{
			Cmd_Printf(ctx, "Absolute paths are prohibited.\n");
			return;
		}
	}

	if (Net_FileProxy(ctx->qtv, fname))
		Cmd_Printf(ctx, "Recording to disk\n");
	else
		Cmd_Printf(ctx, "Failed to open file\n");
}
void Cmd_Stop(cmdctxt_t *ctx)
{
	if (Net_StopFileProxy(ctx->qtv))
		Cmd_Printf(ctx, "stopped\n");
	else
		Cmd_Printf(ctx, "not recording to disk\n");
}

void Cmd_Reconnect(cmdctxt_t *ctx)
{
	if (ctx->qtv->autodisconnect == AD_REVERSECONNECT)
		Cmd_Printf(ctx, "Stream is a reverse connection (command rejected)\n");
//	else if (ctx->qtv->autodisconnect == AD_STATUSPOLL && !ctx->qtv->numviewers && !ctx->qtv->proxies)
//		Cmd_Printf(ctx, "Not reconnecting to idle server\n");
	else if (QTV_ConnectStream(ctx->qtv, ctx->qtv->server))
		Cmd_Printf(ctx, "Reconnected\n");
	else
		Cmd_Printf(ctx, "Failed to reconnect (will keep trying)\n");
}

void Cmd_MVDPort(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	int newp = atoi(val);

	if (!*val)
	{
		Cmd_Printf(ctx, "Listening for tcp connections on port %i\n", ctx->cluster->tcplistenportnum);
		return;
	}

	if (!newp)
	{
		if (ctx->cluster->tcpsocket[0] != INVALID_SOCKET && ctx->cluster->tcpsocket[1] != INVALID_SOCKET)
			Cmd_Printf(ctx, "Already closed\n");
	}

	Net_TCPListen(ctx->cluster, newp, true);
	Net_TCPListen(ctx->cluster, newp, false);
	ctx->cluster->tcplistenportnum = newp;
}

void Cmd_DemoList(cmdctxt_t *ctx)
{
	int i;
	int count;
	Cluster_BuildAvailableDemoList(ctx->cluster);

	count = ctx->cluster->availdemoscount;
	Cmd_Printf(ctx, "%i demos\n", count);
	for (i = 0; i < count; i++)
	{
		Cmd_Printf(ctx, " %7i %s\n", ctx->cluster->availdemos[i].size, ctx->cluster->availdemos[i].name);
	}
}

void Cmd_BaseDir(cmdctxt_t *ctx)
{
	char *val;
	val = Cmd_Argv(ctx, 1);
	if (!Cmd_IsLocal(ctx))
		Cmd_Printf(ctx, "Sorry, you may not use this command remotely\n");

	if (*val)
		chdir(val);
	else
	{
		char buffer[256];
		val = getcwd(buffer, sizeof(buffer));
		if (val)
			Cmd_Printf(ctx, "basedir is: %s\n", val);
		else
			Cmd_Printf(ctx, "system error getting basedir\n");
	}
}

void Cmd_DemoDir(cmdctxt_t *ctx)
{
	char *val;
	val = Cmd_Argv(ctx, 1);

	if (*val)
	{
		if (!Cmd_IsLocal(ctx))
		{
			Cmd_Printf(ctx, "Sorry, but I don't trust this code that well!\n");
			return;
		}
		while (*val > 0 &&*val <= ' ')
			val++;

		if (strchr(val, '.') || strchr(val, ':') || *val == '/')
			Cmd_Printf(ctx, "Rejecting path\n");
		else
		{
			strlcpy(ctx->cluster->demodir, val, sizeof(ctx->cluster->demodir));
			Cmd_Printf(ctx, "Changed demo dir to \"%s\"\n", ctx->cluster->demodir);
		}
	}
	else
	{
		Cmd_Printf(ctx, "Current demo directory is \"%s\"\n", ctx->cluster->demodir);
	}
}

void Cmd_DLDir(cmdctxt_t *ctx)
{
	char *val;
	val = Cmd_Argv(ctx, 1);

	if (!Cmd_IsLocal(ctx))
	{
		Cmd_Printf(ctx, "dldir may not be used remotely\n");
		return;
	}

	if (*val)
	{
		while (*val > 0 &&*val <= ' ')
			val++;

//		if (strchr(val, '.') || strchr(val, ':') || *val == '/')
//			Cmd_Printf(ctx, "Rejecting path\n");
//		else
		{
			strlcpy(ctx->cluster->downloaddir, val, sizeof(ctx->cluster->downloaddir));
			Cmd_Printf(ctx, "Changed download dir to \"%s\"\n", ctx->cluster->downloaddir);
		}
	}
	else
	{
		Cmd_Printf(ctx, "Current download directory is \"%s\"\n", ctx->cluster->downloaddir);
	}
}

void Cmd_PluginDataSource(cmdctxt_t *ctx)
{
	char *val;
	val = Cmd_Argv(ctx, 1);

	if (!Cmd_IsLocal(ctx))
	{
		Cmd_Printf(ctx, "plugindatasource may not be used remotely\n");
		return;
	}

	if (*val)
	{
		while (*val > 0 &&*val <= ' ')
			val++;

		strlcpy(ctx->cluster->plugindatasource, val, sizeof(ctx->cluster->plugindatasource));
		Cmd_Printf(ctx, "Changed plugindatasource to \"%s\"\n", ctx->cluster->plugindatasource);
	}
	else
	{
		Cmd_Printf(ctx, "Current plugindatasource is \"%s\"\n", ctx->cluster->plugindatasource);
	}
}

void Cmd_MapSource(cmdctxt_t *ctx)
{
	char *val;
	val = Cmd_Argv(ctx, 1);

	if (!Cmd_IsLocal(ctx))
	{
		Cmd_Printf(ctx, "mapsource may not be used remotely\n");
		return;
	}

	if (*val)
	{
		while (*val > 0 &&*val <= ' ')
			val++;

		strlcpy(ctx->cluster->mapsource, val, sizeof(ctx->cluster->mapsource));
		Cmd_Printf(ctx, "Changed mapsource url to \"%s\"\n", ctx->cluster->mapsource);
	}
	else
	{
		Cmd_Printf(ctx, "Current mapsource url is \"%s\"\n", ctx->cluster->mapsource);
	}
}

void Cmd_MuteStream(cmdctxt_t *ctx)
{
	char *val;
	val = Cmd_Argv(ctx, 1);
	if (*val)
	{
		ctx->qtv->silentstream = atoi(val);
		Cmd_Printf(ctx, "Stream is now %smuted\n", ctx->qtv->silentstream?"":"un");
	}
	else
		Cmd_Printf(ctx, "Stream is currently %smuted\n", ctx->qtv->silentstream?"":"un");
}

#ifdef VIEWER
void Cmd_Watch(cmdctxt_t *ctx)
{
	if (!localcommand)
	{
		Cmd_Printf(ctx, "watch is not permitted remotly\n");
	}

	if (cluster->viewserver == qtv)
	{
		cluster->viewserver = NULL;
		Cmd_Printf(ctx, "Stopped watching\n");
	}
	else
	{
		cluster->viewserver = qtv;
		Cmd_Printf(ctx, "Watching\n");
	}
}
#endif

#ifdef __linux__
#include <fcntl.h>
qboolean Sys_RandomBytes(unsigned char *out, int len)
{
	qboolean res;
	int fd = open("/dev/urandom", 0);
	res = (read(fd, out, len) == len);
	close(fd);
	return res;
}
#else
qboolean Sys_RandomBytes(unsigned char *out, int len)
{
	return false;
}
#endif

static void Cmd_Turn(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		if (ctx->cluster->turnenabled && ctx->cluster->turn_minport)
			Cmd_Printf(ctx, "turn is enabled, using ports %i-%i\n", ctx->cluster->turn_minport, ctx->cluster->turn_maxport);
		else if (ctx->cluster->turnenabled)
			Cmd_Printf(ctx, "turn is enabled, using ephemerial ports\n");
		else
			Cmd_Printf(ctx, "turn is disabled\n");
		return;
	}
	if (!Cmd_IsLocal(ctx))
	{
		Cmd_Printf(ctx, "turn support may not be configured remotely\n");
		return;
	}

	if (Cmd_Argc(ctx) >= 3)
	{	//two args - assume a two number range, so turn it on.
		ctx->cluster->turnenabled = true;
		ctx->cluster->turn_minport = atoi(Cmd_Argv(ctx, 1));
		ctx->cluster->turn_maxport = atoi(Cmd_Argv(ctx, 2));
	}
	else if ( atoi(Cmd_Argv(ctx, 1)))	//a boolean. turn it back on..
		ctx->cluster->turnenabled = true;	//switch it back on with whatever port range it previously had. probably 0-0 for ephemerial. probably bad for the relay's firewalls...
	else
		ctx->cluster->turnenabled = false;	//and off.

	if (!*ctx->cluster->chalkey && ctx->cluster->turnenabled)
	{
		unsigned char chalkey[12];
		if (!Sys_RandomBytes(chalkey, sizeof(chalkey)) ||
			!Sys_RandomBytes(ctx->cluster->turnkey, sizeof(ctx->cluster->turnkey)))
		{
			Cmd_Printf(ctx, "no random generator\n");
			ctx->cluster->turnenabled = false;
			return;
		}
		tobase64(ctx->cluster->chalkey,sizeof(ctx->cluster->chalkey), chalkey, sizeof(chalkey));
	}

	if (ctx->cluster->turnenabled && ctx->cluster->turn_minport)
		Cmd_Printf(ctx, "turn keys updated, using ports %i-%i\n", ctx->cluster->turn_minport, ctx->cluster->turn_maxport);
	else if (ctx->cluster->turnenabled)
		Cmd_Printf(ctx, "turn keys updated, using ephemerial ports\n");
	else
		Cmd_Printf(ctx, "turn disabled\n");
}
static void Cmd_Relay(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) >= 2)
	{
		if (Cmd_IsLocal(ctx))
		{
			Cmd_Printf(ctx, "relay support may not be configured remotely\n");
			return;
		}
		switch(atoi(Cmd_Argv(ctx, 1)))
		{
		case 0:
			ctx->cluster->relayenabled = ctx->cluster->pingtreeenabled = false;
			Cmd_Printf(ctx, "turn disabled\n");
			break;
		case 1:
			ctx->cluster->relayenabled = ctx->cluster->pingtreeenabled = true;
			break;
		default:
			ctx->cluster->relayenabled = true;
			ctx->cluster->pingtreeenabled = false;
			break;
		}
	}

	if (ctx->cluster->relayenabled && ctx->cluster->pingtreeenabled)
		Cmd_Printf(ctx, "relay is enabled (with pinging)\n");
	else if (ctx->cluster->relayenabled)
		Cmd_Printf(ctx, "relay is enabled, WITHOUT pinging\n");
	else
		Cmd_Printf(ctx, "relay is disabled\n");
}
static void Cmd_ProtocolName(cmdctxt_t *ctx)
{
	free(ctx->cluster->protocolname);
	ctx->cluster->protocolname = strdup(Cmd_Argv(ctx, 1));
	ctx->cluster->protocolver = atoi(Cmd_Argv(ctx, 2));
}

typedef struct rconcommands_s {
	char *name;
	qboolean serverspecific;	//works within a qtv context
	qboolean clusterspecific;	//works without a qtv context (ignores context)
	consolecommand_t func;
	char *description;
} rconcommands_t;

extern const rconcommands_t rconcommands[];

void Cmd_Commands(cmdctxt_t *ctx)
{
	const rconcommands_t *cmd;
	consolecommand_t lastfunc = NULL;

	Cmd_Printf(ctx, "Say Commands:\n");
	for (cmd = rconcommands; cmd->name; cmd++)
	{
		if (cmd->func == lastfunc)
			continue;	//no spamming alternative command names

		Cmd_Printf(ctx, "%s: %s\n", cmd->name, cmd->description?cmd->description:"no description available");
		lastfunc = cmd->func;
	}
}

const rconcommands_t rconcommands[] =
{
	{"exec",		1, 1, Cmd_Exec,		"executes a config file"},
	{"status",		1, 1, Cmd_Status,	"prints proxy/stream status" },
	{"say",			1, 1, Cmd_Say,		"says to a stream"},

	{"help",		0, 1, Cmd_Help,		"shows the brief intro help text"},
	{"commands",		0, 1, Cmd_Commands,	"prints the list of commands"},
	{"apropos",		0, 1, Cmd_Commands,	"prints all commands"},
	{"hostname",		0, 1, Cmd_Hostname,	"changes the hostname seen in server browsers"},
	{"master",		0, 1, Cmd_Master,	"specifies which master server to use"},
	{"udpport",		0, 1, Cmd_UDPPort,	"specifies to listen on a provided udp port for regular qw clients"},
	 {"port",		0, 1, Cmd_UDPPort},
	{"adminpassword",	0, 1, Cmd_AdminPassword,"specifies the password for qtv administrators"},
	 {"rconpassword",	0, 1, Cmd_AdminPassword},
	{"qtvlist",		0, 1, Cmd_QTVList,	"queries a seperate proxy for a list of available streams"},
	{"qtvdemolist",		0, 1, Cmd_QTVDemoList,	"queries a seperate proxy for a list of available demos"},
	{"qtv",			0, 1, Cmd_QTVConnect,	"adds a new tcp/qtv stream"},
	 {"addserver",		0, 1, Cmd_QTVConnect},
	 {"connect",		0, 1, Cmd_QTVConnect},
	{"qw",			0, 1, Cmd_QWConnect,	"adds a new udp/qw stream"},
	 {"observe",		0, 1, Cmd_QWConnect},
	{"demos",		0, 1, Cmd_DemoList,	"shows the list of demos available on this proxy"},
	{"demo",		0, 1, Cmd_MVDConnect,	"adds a demo as a new stream"},
	 {"playdemo",		0, 1, Cmd_MVDConnect},
	{"dir",			0, 1, Cmd_DirMVDConnect, "adds a directory of demos as a new stream"},
	 {"playdir",		0, 1, Cmd_DirMVDConnect},
	{"choke",		0, 1, Cmd_Choke,	"chokes packets to the data rate in the stream, disables proxy-side interpolation"},
	{"late",		0, 1, Cmd_Late,		"enforces a time delay on packets sent through this proxy"},
	{"talking",		0, 1, Cmd_Talking,	"permits viewers to talk to each other"},
	{"nobsp",		0, 1, Cmd_NoBSP,	"disables loading of bsp files"},
	{"userconnects",	0, 1, Cmd_UserConnects,	"prevents users from creating thier own streams"},
	{"maxviewers",		0, 1, Cmd_MaxViewers,	"sets a limit on udp/qw client connections"},
	{"maxproxies",		0, 1, Cmd_MaxProxies,	"sets a limit on tcp/qtv client connections"},
	{"demodir",		0, 1, Cmd_DemoDir,	"specifies where to get the demo list from"},
	{"basedir",		0, 1, Cmd_BaseDir,	"specifies where to get any files required by the game. this is prefixed to the server-specified game dir."},
	{"ping",		0, 1, Cmd_Ping,		"sends a udp ping to a qtv proxy or server"},
	{"reconnect",		0, 1, Cmd_Reconnect,	"forces a stream to reconnect to its server (restarts demos)"},
	{"echo",		0, 1, Cmd_Echo,		"a useless command that echos a string"},
	{"quit",		0, 1, Cmd_Quit,		"closes the qtv"},
	{"exit",		0, 1, Cmd_Quit},
	{"streams",		0, 1, Cmd_Streams,	"shows a list of active streams"},
	{"allownq",		0, 1, Cmd_AllowNQ,	"permits nq clients to connect. This can be disabled as this code is less tested than the rest"},
	{"initialdelay",0, 1, Cmd_InitialDelay, "Specifies the duration for which new connections will be buffered. Large values prevents players from spectating their enemies as a cheap wallhack."},
	{"slowdelay",	0, 1, Cmd_SlowDelay,	"If a server is not sending enough data, the proxy will delay parsing for this long."},

	{"turn",		0, 1, Cmd_Turn,			"Controls whether we accept turn requests."},
	{"relay",		0, 1, Cmd_Relay,		"Controls whether we accept qwfwd-style relay requests."},
	 {"qwfwd",		0, 1, Cmd_Relay},
	{"protocolname",0, 1, Cmd_ProtocolName,	"Protocol Name:Version used to register with master."},


	{"halt",		1, 0, Cmd_Halt,		"disables a stream, preventing it from reconnecting until someone tries watching it anew. Boots current spectators"},
	{"disable",		1, 0, Cmd_Halt},
	{"pause",		1, 0, Cmd_Pause,		"Pauses a demo stream."},
	{"resume",		1, 0, Cmd_Resume,	"reactivates a stream, allowing it to reconnect"},
	{"enable",		1, 0, Cmd_Resume},
	{"mute",		1, 0, Cmd_MuteStream,	"hides prints that come from the game server"},
	{"mutestream",		1, 0, Cmd_MuteStream},
	{"disconnect",		1, 0, Cmd_Disconnect,	"fully closes a stream"},
	{"record",		1, 0, Cmd_Record,	"records a stream to a demo"},
	{"stop",		1, 0, Cmd_Stop,		"stops recording of a demo"},
	{"demospeed",		1, 0, Cmd_DemoSpeed,	"changes the rate the demo is played at"},
	{"tcpport",		0, 1, Cmd_MVDPort,	"specifies which port to listen on for tcp/qtv connections"},
	 {"mvdport",		0, 1, Cmd_MVDPort},

	{"dldir",		0, 1, Cmd_DLDir,	"specifies the path to download stuff from (http://server/file/ maps to this native path)"},
	{"plugindatasource",0,1,Cmd_PluginDataSource, "Specifies the dataDownload property for plugins in the web server"},
	{"mapsource",	0, 1, Cmd_MapSource,"Public URL for where to download missing maps from"},

#ifdef VIEWER
	{"watch",		1, 0, Cmd_Watch,	"specifies to watch that stream in the built-in viewer"},
#endif

	{NULL}
};

void Cmd_ExecuteNow(cmdctxt_t *ctx, char *command)
{
#define TOKENIZE_PUNCTUATION ""

	int i;
	char arg[MAX_ARGS][ARG_LEN];
	char *sid;
	char *cmdname;

	for (sid = command; *sid; sid++)
	{
		if (*sid == ':')
			break;
		if (*sid < '0' || *sid > '9')
			break;
	}
	if (*sid == ':')
	{
		i = atoi(command);
		command = sid+1;

		ctx->streamid = i;

		for (ctx->qtv = ctx->cluster->servers; ctx->qtv; ctx->qtv = ctx->qtv->next)
			if (ctx->qtv->streamid == i)
				break;
	}
	else
		ctx->streamid = 0;

	ctx->argc = 0;
	for (i = 0; i < MAX_ARGS; i++)
	{
		command = COM_ParseToken(command, arg[i], ARG_LEN, TOKENIZE_PUNCTUATION);
		ctx->arg[i] = arg[i];
		if (command)
			ctx->argc++;
	}

	cmdname = Cmd_Argv(ctx, 0);

	//if there's only one stream, set that as the selected stream
	if (!ctx->qtv && ctx->cluster->numservers==1)
		ctx->qtv = ctx->cluster->servers;

	if (ctx->qtv)
	{	//if there is a specific connection targetted

		for (i = 0; rconcommands[i].name; i++)
		{
			if (rconcommands[i].serverspecific)
				if (!strcmp(rconcommands[i].name, cmdname))
				{
					rconcommands[i].func(ctx);
					return;
				}
		}
	}

	for (i = 0; rconcommands[i].name; i++)
	{
		if (!strcmp(rconcommands[i].name, cmdname))
		{
			if (rconcommands[i].clusterspecific)
			{
				rconcommands[i].func(ctx);
				return;
			}
			else if (rconcommands[i].serverspecific)
			{
				Cmd_Printf(ctx, "Command \"%s\" requires a targeted server.\n", cmdname);
				return;
			}
		}
	}


	Cmd_Printf(ctx, "Command \"%s\" not recognised.\n", cmdname);
}

void Rcon_PrintToBuffer(cmdctxt_t *ctx, char *msg)
{
	if (ctx->printcookiesize < 1)
		return;
	while (ctx->printcookiesize>2 && *msg)
	{
		ctx->printcookiesize--;
		*(char*)ctx->printcookie = *msg++;
		ctx->printcookie = ((char*)ctx->printcookie)+1;
	}

	ctx->printcookiesize--;
	*(char*)ctx->printcookie = 0;
}

char *Rcon_Command(cluster_t *cluster, sv_t *source, char *command, char *resultbuffer, int resultbuffersize, int islocalcommand)
{
	cmdctxt_t ctx;
	ctx.cluster = cluster;
	ctx.qtv = source;
	ctx.argc = 0;
	ctx.printfunc = Rcon_PrintToBuffer;
	ctx.printcookie = resultbuffer;
	ctx.printcookiesize = resultbuffersize;
	ctx.localcommand = islocalcommand;
	*(char*)ctx.printcookie = 0;
	Cmd_ExecuteNow(&ctx, command);

	return resultbuffer;
}
