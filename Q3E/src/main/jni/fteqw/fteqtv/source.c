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


//connection notes
//The connection is like http.
//The stream starts with a small header.
//The header is a list of 'key: value' pairs, separated by new lines.
//The header ends with a totally blank line.
//to record an mvd from telnet or somesuch, you would use:
//"QTV\nRAW: 1\n\n"

//VERSION: a list of the different qtv protocols supported. Multiple versions can be specified. The first is assumed to be the preferred version.
//RAW: if non-zero, send only a raw mvd with no additional markup anywhere (for telnet use). Doesn't work with challenge-based auth, so will only be accepted when proxy passwords are not required.
//AUTH: specifies an auth method, the exact specs varies based on the method
//		PLAIN: the password is sent as a PASSWORD line
//		MD4: the server responds with an "AUTH: MD4\n" line as well as a "CHALLENGE: somerandomchallengestring\n" line, the client sends a new 'initial' request with CHALLENGE: MD4\nRESPONSE: hexbasedmd4checksumhere\n"
//		etc: same idea as md4
//		CCITT: same as md4, but using the CRC stuff common to all quake engines. should not be used.
//		if the supported/allowed auth methods don't match, the connection is silently dropped.
//SOURCE: which stream to play from, DEFAULT is special. Without qualifiers, it's assumed to be a tcp address.
//COMPRESSION: Suggests a compression method (multiple are allowed). You'll get a COMPRESSION response, and compression will begin with the binary data.
//SOURCELIST: Asks for a list of active sources from the proxy.
//DEMOLIST:	Asks for a list of available mvd demos.

//Response:
//if using RAW, there will be no header or anything
//Otherwise you'll get a QTVSV %f response (%f being the protocol version being used)
//same structure, terminated by a \n
//AUTH: Server requires auth before proceeding. If you don't support the method the server says, then, urm, the server shouldn't have suggested it.
//CHALLENGE: used with auth
//COMPRESSION: Method of compression used. Compression begins with the raw data after the connection process.
//ASOURCE: names a source
//ADEMO: gives a demo file name




#include "qtv.h"
#include <string.h>

#include "bsd_string.h"

#ifndef _WIN32
#include <dirent.h>
#include <signal.h>
#endif

#include <stddef.h>
#ifdef UNIXSOCKETS
#include <sys/un.h>
#endif

#define RECONNECT_TIME (1000*30)
#define RECONNECT_TIME_DEMO (1000*5)
#define UDPRECONNECT_TIME (1000)
#define STATUSPOLL_TIME 1000*30
#define PINGSINTERVAL_TIME (1000*5)
#define UDPTIMEOUT_LENGTH (1000*20)
#define UDPPACKETINTERVAL (1000/72)

void Net_SendConnectionMVD(sv_t *qtv, oproxy_t *prox);
void Net_QueueUpstream(sv_t *qtv, int size, char *buffer);


qboolean	NET_StringToAddr (char *s, netadr_t *sadr, int defaultport)
{
	struct hostent	*h;
	char	*colon;
	char	copy[128];

	memset (sadr, 0, sizeof(netadr_t));

#ifdef USEIPX
	if ((strlen(s) >= 23) && (s[8] == ':') && (s[21] == ':'))	// check for an IPX address
	{
		unsigned int val;

		((struct sockaddr_ipx *)sadr)->sa_family = AF_IPX;
		copy[2] = 0;
		DO(0, sa_netnum[0]);
		DO(2, sa_netnum[1]);
		DO(4, sa_netnum[2]);
		DO(6, sa_netnum[3]);
		DO(9, sa_nodenum[0]);
		DO(11, sa_nodenum[1]);
		DO(13, sa_nodenum[2]);
		DO(15, sa_nodenum[3]);
		DO(17, sa_nodenum[4]);
		DO(19, sa_nodenum[5]);
		sscanf (&s[22], "%u", &val);
		((struct sockaddr_ipx *)sadr)->sa_socket = htons((unsigned short)val);
	}
	else
#endif
#ifndef _WIN32
		if (1)
	{//ipv6 method (can return ipv4 addresses too)
		struct addrinfo *addrinfo, *pos;
		struct addrinfo udp6hint;
		int error;
		char *port;
		char dupbase[256];
		int len;

		memset(&udp6hint, 0, sizeof(udp6hint));
		udp6hint.ai_family = 0;//Any... we check for AF_INET6 or 4
		udp6hint.ai_socktype = SOCK_DGRAM;
		udp6hint.ai_protocol = IPPROTO_UDP;

		if (*s == '[')
		{
			s++;
			colon = strchr(s, ']');
			if (!colon || colon-s >= sizeof(copy))
				return false;	//too long to handle.
			memcpy(copy, s, colon-s);
			copy[colon-s] = 0;
			colon++;
			if (*colon == ':')
				port = colon;
			else
				port = NULL;
			s = copy;
		}
		else
		{
			port = s + strlen(s);
			while(port >= s)
			{
				if (*port == ':')
				break;
				port--;
			}
		}

		if (port == s)
			port = NULL;
		if (port)
		{
			len = port - s;
			if (len > sizeof(dupbase)-1)
				len = sizeof(dupbase)-1;
			memcpy(dupbase, s, len);
			dupbase[len] = 0;
			error = getaddrinfo(dupbase, port+1, &udp6hint, &addrinfo);
		}
		else
			error = EAI_NONAME, addrinfo=NULL;
		if (error)	//failed, try string with no port.
			error = getaddrinfo(s, NULL, &udp6hint, &addrinfo);	//remember, this func will return any address family that could be using the udp protocol... (ip4 or ip6)
		if (error)
		{
			return false;
		}
		((struct sockaddr*)sadr->sockaddr)->sa_family = 0;
		for (pos = addrinfo; pos; pos = pos->ai_next)
		{
			switch(pos->ai_family)
			{
			case AF_INET6:
				if (((struct sockaddr_in *)sadr->sockaddr)->sin_family == AF_INET6)
					break;	//first one should be best...
				//fallthrough
			case AF_INET:
				memcpy(sadr->sockaddr, addrinfo->ai_addr, addrinfo->ai_addrlen);
				if (pos->ai_family == AF_INET)
					goto dblbreak;	//don't try finding any more, this is quake, they probably prefer ip4...
				break;
			}
		}
dblbreak:
		pfreeaddrinfo (addrinfo);
		if (!((struct sockaddr*)sadr->sockaddr)->sa_family)	//none suitablefound
			return false;
	}
	else
#endif
	{ //old fashioned method
		struct sockaddr_in *sin = (struct sockaddr_in *)sadr->sockaddr;
		sin->sin_family = AF_INET;

		sin->sin_port = htons(defaultport);

		strcpy (copy, s);
		// strip off a trailing :port if present
		for (colon = copy ; *colon ; colon++)
			if (*colon == ':')
			{
				*colon = 0;
				sin->sin_port = htons((short)atoi(colon+1));
			}

		if (copy[0] >= '0' && copy[0] <= '9')	//this is the wrong way to test. a server name may start with a number.
		{
			*(int *)&sin->sin_addr = inet_addr(copy);
		}
		else
		{
			if (! (h = gethostbyname(copy)) )
				return 0;
			if (h->h_addrtype != AF_INET)
				return 0;
			*(int *)&sin->sin_addr = *(int *)h->h_addr_list[0];
		}
	}

	return true;
}

qboolean Net_CompareAddress(netadr_t *s1, netadr_t *s2, int qp1, int qp2)
{
	struct sockaddr *g1=(void*)s1->sockaddr, *g2=(void*)s2->sockaddr;
	if (g1->sa_family != g2->sa_family)
	{	//urgh...
		if (g1->sa_family == AF_INET6 && g2->sa_family == AF_INET && (
			((unsigned int*)&((struct sockaddr_in6 *)g1)->sin6_addr)[0] == 0 &&
			((unsigned int*)&((struct sockaddr_in6 *)g1)->sin6_addr)[1] == 0 &&
			((unsigned short*)&((struct sockaddr_in6 *)g1)->sin6_addr)[4] == 0 &&
			((unsigned short*)&((struct sockaddr_in6 *)g1)->sin6_addr)[5] == 0xffff))
		{
			struct sockaddr_in6 *i1=(void*)s1->sockaddr;
			struct sockaddr_in *i2=(void*)s2->sockaddr;
			if (((unsigned int*)&i1->sin6_addr)[3] != *(unsigned int*)&i2->sin_addr)
				return false;
			if (i1->sin6_port != i2->sin_port && qp1 != qp2)	//allow qports to match instead of ports, if required.
				return false;
			return true;
		}
		if (g1->sa_family == AF_INET && g2->sa_family == AF_INET6 && (
			((unsigned int*)&((struct sockaddr_in6 *)g2)->sin6_addr)[0] == 0 &&
			((unsigned int*)&((struct sockaddr_in6 *)g2)->sin6_addr)[1] == 0 &&
			((unsigned short*)&((struct sockaddr_in6 *)g2)->sin6_addr)[4] == 0 &&
			((unsigned short*)&((struct sockaddr_in6 *)g2)->sin6_addr)[5] == 0xffff))
		{
			struct sockaddr_in6 *i1=(void*)s2->sockaddr;
			struct sockaddr_in *i2=(void*)s1->sockaddr;
			if (((unsigned int*)&i1->sin6_addr)[3] != *(unsigned int*)&i2->sin_addr)
				return false;
			if (i1->sin6_port != i2->sin_port && qp1 != qp2)	//allow qports to match instead of ports, if required.
				return false;
			return true;
		}
		return false;
	}
	switch(g1->sa_family)
	{
	default:
		return true;
	case AF_INET:
		{
			struct sockaddr_in *i1=(void*)s1->sockaddr, *i2=(void*)s2->sockaddr;
			if (*(unsigned int*)&i1->sin_addr != *(unsigned int*)&i2->sin_addr)
				return false;
			if (i1->sin_port != i2->sin_port && qp1 != qp2)	//allow qports to match instead of ports, if required.
				return false;
			return true;
		}

	case AF_INET6:
		{
			struct sockaddr_in6 *i1=(void*)s1->sockaddr, *i2=(void*)s2->sockaddr;
			if (((unsigned int*)&i1->sin6_addr)[0] != ((unsigned int*)&i2->sin6_addr)[0])
				return false;
			if (((unsigned int*)&i1->sin6_addr)[1] != ((unsigned int*)&i2->sin6_addr)[1])
				return false;
			if (((unsigned int*)&i1->sin6_addr)[2] != ((unsigned int*)&i2->sin6_addr)[2])
				return false;
			if (((unsigned int*)&i1->sin6_addr)[3] != ((unsigned int*)&i2->sin6_addr)[3])
				return false;
			if (i1->sin6_port != i2->sin6_port && qp1 != qp2)	//allow qports to match instead of ports, if required.
				return false;
			return true;
		}
	}
	return false;
}

void Net_TCPListen(cluster_t *cluster, int port, int socketid)
{
	SOCKET sock;

	union
	{
		struct sockaddr		s;
		struct sockaddr_in	ipv4;
		struct sockaddr_in6	ipv6;
#ifdef UNIXSOCKETS
		struct sockaddr_un	un;
#endif
	} address;
	int prot;
	int addrsize;
	int _true = true;
//	int fromlen;

	unsigned long nonblocking = true;
	unsigned long v6only = false;
	const char *famname;

	switch(socketid)
	{
#ifdef UNIXSOCKETS
	case SG_UNIX:
		prot = AF_UNIX;
		memset(&address.un, 0, sizeof(address.un));
		address.un.sun_family = prot;
		memcpy(address.un.sun_path, "\0qtv", 4);
		addrsize = offsetof(struct sockaddr_un, sun_path[4]);
		famname = "unix";
		break;
#endif
	case SG_IPV6:
		prot = AF_INET6;
		memset(&address.ipv6, 0, sizeof(address.ipv6));
		address.ipv6.sin6_family = prot;
		address.ipv6.sin6_port = htons((u_short)port);
		addrsize = sizeof(struct sockaddr_in6);
		if (v6only)
			famname = "tcp6";
		else
			famname = "tcp";
		break;
	case SG_IPV4:
		prot = AF_INET;
		address.ipv4.sin_family = prot;
		address.ipv4.sin_addr.s_addr = INADDR_ANY;
		address.ipv4.sin_port = htons((u_short)port);
		addrsize = sizeof(struct sockaddr_in);
		famname = "tcp4";
		break;
	default:
		return;	//some kind of error. avoid unintialised warnings.
	}

	if (socketid==SG_IPV4 && !v6only && cluster->tcpsocket[SG_IPV6] != INVALID_SOCKET)
	{	//if we already have a hybrid ipv6 socket, don't bother with ipv4 too
		int sz = sizeof(v6only);
		if (getsockopt(cluster->tcpsocket[1], IPPROTO_IPV6, IPV6_V6ONLY, (char *)&v6only, &sz) == 0 && !v6only)
			port = 0;
	}

	if (cluster->tcpsocket[socketid] != INVALID_SOCKET)
	{
		closesocket(cluster->tcpsocket[socketid]);
		cluster->tcpsocket[socketid] = INVALID_SOCKET;

		Sys_Printf(cluster, "closed %s port\n", famname);
	}
	if (!port)
		return;

	if ((sock = socket (prot, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		cluster->tcpsocket[socketid] = INVALID_SOCKET;
		return;
	}

	if (ioctlsocket (sock, FIONBIO, &nonblocking) == -1)
	{
		cluster->tcpsocket[socketid] = INVALID_SOCKET;
		closesocket(sock);
		return;
	}

#ifdef _WIN32 
	//win32 is so fucked up
//	setsockopt(newsocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *)&_true, sizeof(_true));
#endif
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&_true, sizeof(_true));


	if (socketid == SG_IPV6)
	{
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&v6only, sizeof(v6only)) == -1)
			v6only = true;
		if (v6only)
			famname = "tcp6";
		else
			famname = "tcp";
	}

	if (bind (sock, &address.s, addrsize) == -1)
	{
		printf("socket bind error %i (%s)\n", qerrno, strerror(qerrno));
		closesocket(sock);
		return;
	}

	listen(sock, 2);	//don't listen for too many clients at once.

	Sys_Printf(cluster, "opened %s port %i\n", famname, port);

	cluster->tcpsocket[socketid] = sock;
}

char *strchrrev(char *str, char chr)
{
	char *firstchar = str;
	for (str = str + strlen(str)-1; str>=firstchar; str--)
		if (*str == chr)
			return str;

	return NULL;
}

void Net_SendQTVConnectionRequest(sv_t *qtv, char *authmethod, char *challenge)
{
	char *at;
	char *str;
	char hash[512];

	//due to mvdsv sucking and stuff, we try using raw connections where possibleso that we don't end up expecting a header.
	//at some point, this will be forced to 1
	qtv->parsingqtvheader = true;//!!*qtv->connectpassword;
	qtv->buffersize = 0;
	qtv->forwardpoint = 0;

	str =	"QTV\n";			Net_QueueUpstream(qtv, strlen(str), str);
	str =	"VERSION: 1\n";		Net_QueueUpstream(qtv, strlen(str), str);

	if (qtv->serverquery)
	{
		if (qtv->serverquery == 2)
		{
			str =	"DEMOLIST\n";		Net_QueueUpstream(qtv, strlen(str), str);
		}
		else
		{
			str =	"SOURCELIST\n";		Net_QueueUpstream(qtv, strlen(str), str);
		}
	}
	else
	{

		at = strchrrev(qtv->server, '@');
		if (at)
		{
			*at = '\0';
			str =	"SOURCE: ";		Net_QueueUpstream(qtv, strlen(str), str);

			if (strncmp(qtv->server, "tcp:", 4))
			{
				str = qtv->server;
				Net_QueueUpstream(qtv, strlen(str), str);
			}
			else
			{
				str = strchr(qtv->server, ':');
				if (str)
				{
					str++;
					Net_QueueUpstream(qtv, strlen(str), str);
				}
			}

			str =	"\n";			Net_QueueUpstream(qtv, strlen(str), str);
			*at = '@';
		}
		else
		{
			str =	"RECEIVE\n";	Net_QueueUpstream(qtv, strlen(str), str);
		}

		if (!qtv->parsingqtvheader)
		{
			str =	"RAW: 1\n";		Net_QueueUpstream(qtv, strlen(str), str);
		}
		else
		{
			if (authmethod)
			{
				if (!strcmp(authmethod, "PLAIN"))
				{
					str = "AUTH: PLAIN\n";	Net_QueueUpstream(qtv, strlen(str), str);
					str = "PASSWORD: \"";	Net_QueueUpstream(qtv, strlen(str), str);
					str = qtv->connectpassword;	Net_QueueUpstream(qtv, strlen(str), str);
					str = "\"\n";			Net_QueueUpstream(qtv, strlen(str), str);
				}
				/*else if (challenge && strlen(challenge)>=32 && !strcmp(authmethod, "CCITT"))
				{
					unsigned short crcvalue;
					str = "AUTH: CCITT\n";	Net_QueueUpstream(qtv, strlen(str), str);
					str = "PASSWORD: \"";	Net_QueueUpstream(qtv, strlen(str), str);

					snprintf(hash, sizeof(hash), "%s%s", challenge, qtv->connectpassword);
					crcvalue = QCRC_Block(hash, strlen(hash));
					sprintf(hash, "0x%X", (unsigned int)QCRC_Value(crcvalue));

					str = hash;				Net_QueueUpstream(qtv, strlen(str), str);
					str = "\"\n";			Net_QueueUpstream(qtv, strlen(str), str);
				}*/
				else if (challenge && strlen(challenge)>=8 && !strcmp(authmethod, "MD4"))
				{
					unsigned int md4sum[4];
					str = "AUTH: MD4\n";	Net_QueueUpstream(qtv, strlen(str), str);
					str = "PASSWORD: \"";	Net_QueueUpstream(qtv, strlen(str), str);

					snprintf(hash, sizeof(hash), "%s%s", challenge, qtv->connectpassword);
					Com_BlockFullChecksum (hash, strlen(hash), (unsigned char*)md4sum);
					sprintf(hash, "%X%X%X%X", md4sum[0], md4sum[1], md4sum[2], md4sum[3]);	//FIXME: bad formatting!

					str = hash;				Net_QueueUpstream(qtv, strlen(str), str);
					str = "\"\n";			Net_QueueUpstream(qtv, strlen(str), str);
				}
				else if (challenge && strlen(challenge)>=8 && !strcmp(authmethod, "SHA1"))
				{
					unsigned char digest[20];
					str = "AUTH: SHA1\n";	Net_QueueUpstream(qtv, strlen(str), str);
					str = "PASSWORD: \"";	Net_QueueUpstream(qtv, strlen(str), str);

					snprintf(hash, sizeof(hash), "%s%s", challenge, qtv->connectpassword);
					CalcHash(&hash_sha1, (unsigned char*)digest, sizeof(digest), hash, strlen(hash));
					tobase64(hash, sizeof(hash), digest, hash_sha1.digestsize);

					str = hash;				Net_QueueUpstream(qtv, strlen(str), str);
					str = "\"\n";			Net_QueueUpstream(qtv, strlen(str), str);
				}
				else if (!strcmp(authmethod, "NONE"))
				{
					str = "AUTH: NONE\n";	Net_QueueUpstream(qtv, strlen(str), str);
					str = "PASSWORD: \n";	Net_QueueUpstream(qtv, strlen(str), str);
				}
				else
				{
					qtv->errored = ERR_PERMANENT;
					qtv->upstreambuffersize = 0;
					Sys_Printf(qtv->cluster, "Auth method %s was not usable\n", authmethod);
					return;
				}
			}
			else
			{
				str = "AUTH: SHA1\n";		Net_QueueUpstream(qtv, strlen(str), str);
				str = "AUTH: MD4\n";		Net_QueueUpstream(qtv, strlen(str), str);
//				str = "AUTH: CCITT\n";		Net_QueueUpstream(qtv, strlen(str), str);
				str = "AUTH: PLAIN\n";		Net_QueueUpstream(qtv, strlen(str), str);
				str = "AUTH: NONE\n";		Net_QueueUpstream(qtv, strlen(str), str);
			}
		}
	}
	str =	"\n";		Net_QueueUpstream(qtv, strlen(str), str);
}

qboolean Net_ConnectToTCPServer(sv_t *qtv, char *ip)
{
	int err;
	netadr_t from;
	unsigned long nonblocking = true;
	int afam;
	int pfam;
	int asz;

	if (!NET_StringToAddr(ip, &qtv->serveraddress, 27500))
	{
		Sys_Printf(qtv->cluster, "Stream %i: Unable to resolve %s\n", qtv->streamid, ip);
		strcpy(qtv->status, "Unable to resolve server\n");
		return false;
	}
	afam = ((struct sockaddr*)&qtv->serveraddress.sockaddr)->sa_family;
	pfam = ((afam==AF_INET6)?PF_INET6:PF_INET);
	asz = ((afam==AF_INET6)?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in));
	qtv->sourcesock = socket(pfam, SOCK_STREAM, IPPROTO_TCP);
	if (qtv->sourcesock == INVALID_SOCKET)
	{
		strcpy(qtv->status, "Network error\n");
		return false;
	}

	if (afam == AF_INET6)
	{
		qboolean v6only = true;
		setsockopt(qtv->sourcesock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&v6only, sizeof(v6only));
	}

	memset(&from, 0, sizeof(from));
	((struct sockaddr*)&from)->sa_family = afam;
	if (bind(qtv->sourcesock, (struct sockaddr *)&from, sizeof(from)) == -1)
	{
		closesocket(qtv->sourcesock);
		qtv->sourcesock = INVALID_SOCKET;
		strcpy(qtv->status, "Network error\n");
		return false;
	}

	if (ioctlsocket (qtv->sourcesock, FIONBIO, &nonblocking) == -1)
	{
		closesocket(qtv->sourcesock);
		qtv->sourcesock = INVALID_SOCKET;
		strcpy(qtv->status, "Network error\n");
		return false;
	}

	if (connect(qtv->sourcesock, (struct sockaddr *)&qtv->serveraddress.sockaddr, asz) == INVALID_SOCKET)
	{
		err = qerrno;
		if (err != NET_EINPROGRESS && err != NET_EAGAIN && err != NET_EWOULDBLOCK)	//bsd sockets are meant to return EINPROGRESS, but some winsock drivers use EWOULDBLOCK instead. *sigh*...
		{
			closesocket(qtv->sourcesock);
			qtv->sourcesock = INVALID_SOCKET;
			strcpy(qtv->status, "Connection failed\n");
			return false;
		}
	}

	//make sure the buffers are empty. we could have disconnected prematurly
	qtv->upstreambuffersize = 0;
	qtv->buffersize = 0;
	qtv->forwardpoint = 0;

	//read the notes at the start of this file for what these text strings mean
	Net_SendQTVConnectionRequest(qtv, NULL, NULL);
	return true;
}
qboolean Net_ConnectToUDPServer(sv_t *qtv, char *ip)
{
	netadr_t from;
	unsigned long nonblocking = true;
	int afam, pfam, asz;

	if (!NET_StringToAddr(ip, &qtv->serveraddress, 27500))
	{
		Sys_Printf(qtv->cluster, "Stream %i: Unable to resolve %s\n", qtv->streamid, ip);
		return false;
	}
	afam = ((struct sockaddr*)&qtv->serveraddress.sockaddr)->sa_family;
	pfam = ((afam==AF_INET6)?PF_INET6:PF_INET);
	asz = ((afam==AF_INET6)?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in));
	qtv->sourcesock = socket(pfam, SOCK_DGRAM, IPPROTO_UDP);
	if (qtv->sourcesock == INVALID_SOCKET)
		return false;

	if (afam == AF_INET6)
	{
		qboolean v6only = true;
		setsockopt(qtv->sourcesock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&v6only, sizeof(v6only));
	}

	memset(&from, 0, sizeof(from));
	((struct sockaddr*)&from)->sa_family = afam;
	if (bind(qtv->sourcesock, (struct sockaddr *)&from, asz) == -1)
	{
		closesocket(qtv->sourcesock);
		qtv->sourcesock = INVALID_SOCKET;
		return false;
	}

	if (ioctlsocket (qtv->sourcesock, FIONBIO, &nonblocking) == -1)
	{
		closesocket(qtv->sourcesock);
		qtv->sourcesock = INVALID_SOCKET;
		return false;
	}

	qtv->qport = Sys_Milliseconds()*1000+Sys_Milliseconds();

	return true;
}

qboolean DemoFilenameIsOkay(sv_t* qtv, char *fname, char *dir)
{
	int len;

	if (dir)
	{
		if (strchr(dir, '/'))
			return false;	//unix path seperator
		if (strchr(dir, '\\'))
			return false;	//windows path seperator
		if (strchr(dir, ':'))
			return false;	//mac path seperator
		if (*(dir) == '.')
			return false;
	}

	if (strchr(fname, '/'))
		return false;	//unix path seperator
	if (strchr(fname, '\\'))
		return false;	//windows path seperator
	if (strchr(fname, ':'))
		return false;	//mac path seperator
	if (*(fname) == '.')
		return false;

	//now make certain that the last four characters are '.mvd' and not something like '.cfg' perhaps
	len = strlen(fname);
	if (len < 5)
		return false;
	if (strcmp(fname+len-4, ".mvd"))
		return false;

	return true;

/*
	if (strchr(fname, '\\'))
	{
		char *s;
		Con_Printf("Warning: \\ characters in filename %s\n", fname);
		while((s = strchr(fname, '\\')))
			*s = '/';
	}

	if (strstr(fname, ".."))
	{
		Con_Printf("Error: '..' characters in filename %s\n", fname);
	}
	else if (fname[0] == '/')
	{
		Con_Printf("Error: absolute path in filename %s\n", fname);
	}
	else if (strstr(fname, ":")) //win32 drive seperator (or mac path seperator, but / works there and they're used to it)
	{
		Con_Printf("Error: absolute path in filename %s\n", fname);
	}
	else
		return false;
	return true;
*/
}

qboolean Net_ConnectToDemoServer(sv_t* qtv, char* ip, char* dir)
{
	char fullname[512];
	qtv->sourcesock = INVALID_SOCKET;
	if (DemoFilenameIsOkay(qtv, ip, dir))
	{
		if (!dir)
			snprintf(fullname, sizeof(fullname), "%s%s", qtv->cluster->demodir, ip);
		else
			snprintf(fullname, sizeof(fullname), "%s%s/%s", qtv->cluster->demodir, dir, ip);
		qtv->sourcefile = fopen(fullname, "rb");
	}
	else
		qtv->sourcefile = NULL;

	if (qtv->sourcefile)
	{
		char smallbuffer[17];
		fseek(qtv->sourcefile, 0, SEEK_END);
		qtv->filelength = ftell(qtv->sourcefile);

		//attempt to detect the end of the file
		fseek(qtv->sourcefile, 0-(long)sizeof(smallbuffer), SEEK_CUR);
		fread(smallbuffer, 1, 17, qtv->sourcefile);
		//0 is the time
		if (smallbuffer[1] == dem_all || smallbuffer[1] == dem_read) //mvdsv changed it to read...
		{
			//2,3,4,5 are the length
			if (smallbuffer[6] == svc_disconnect)
			{
				if (!strcmp(smallbuffer+7, "EndOfDemo"))
				{
					qtv->filelength -= 17;
				}
			}
		}

		fseek(qtv->sourcefile, 0, SEEK_SET);
		return true;
	}

	if (!dir)
		Sys_Printf(qtv->cluster, "Stream %i: Unable to open file %s\n", qtv->streamid, ip);
	else
		Sys_Printf(qtv->cluster, "Stream %i: Unable to open file %s in directory %s\n", qtv->streamid, ip, dir);
	return false;
}

qboolean Net_ConnectToDemoDirServer(sv_t* qtv, char *ip)
{
	char fullname[512];
	qtv->sourcesock = INVALID_SOCKET;
	snprintf(fullname, sizeof(fullname), "%s%s", qtv->cluster->demodir, ip);

	#ifdef _WIN32
	// TODO: code for Windows directories goes here
	// TODO: possible to do this without copy-pasting the entire GNU/Linux code below?
	// TODO: also, what about MAC OS X?
	Sys_Printf(qtv->cluster, "Windows support coming soon!\n");
	return false;
	#else
	{
		DIR *dir;
		struct dirent* ent;

		dir = opendir(fullname);
		if (dir)
		{
			char demoname[512];
			int current_demo = 0;
			int file_count = 0;
			int random_number = 1; // always this value if the directory contains one file

			// count the files, important for determining a random demo file
			while ((ent = readdir(dir)) != NULL)
			{
				int len;

				// only count files neding in .mvd
				len = strlen(ent->d_name);
				if (len < 5)
				{
					continue;
				}
				if (strcmp(ent->d_name+len-4, ".mvd"))
				{
					continue;
				}

				if (ent->d_type == DT_REG && *(ent->d_name) != '.')
					file_count++;	// only add non-hidden and regular files
			}

			if (file_count == 0)
			{
				// empty directory
				Sys_Printf(qtv->cluster, "Stream %i: Error: Directory has no demos.\n", qtv->streamid);
				closedir(dir);
				return false;
			}

			closedir(dir);
			dir = opendir(fullname);

			// FIXME: not sure if srand should only be called once somewhere?
			// FIXME: this is not really shuffling the demos, but does introduce some variety
			if (file_count > 1)
			{
				//srand(time(NULL));
				while ((random_number = rand()%file_count + 1) == qtv->last_random_number);
				qtv->last_random_number = random_number;
			}

			while (1) {
				int len;

				ent = readdir(dir);
				if (!ent)
				{
					// reached the end of the directory, shouldn't happen
					Sys_Printf(qtv->cluster, "Stream %i: Error: Reached end of directory (%s%s)\n", qtv->streamid, qtv->cluster->demodir, ip);
					closedir(dir);
					return false;
				}

				if (ent->d_type != DT_REG || *(ent->d_name) == '.')
				{
					continue;	// ignore hidden and non-regular files
				}

				//now make certain that the last four characters are '.mvd' and not something like '.cfg' perhaps
				len = strlen(ent->d_name);
				if (len < 5)
				{
					continue;
				}
				if (strcmp((ent->d_name)+len-4, ".mvd"))
				{
					continue;
				}

				if (++current_demo != random_number)
					continue;

				snprintf(demoname, sizeof(demoname), "%s/%s", ip, ent->d_name);
				qtv->sourcefile = fopen(demoname, "rb");
				closedir(dir);
				if (Net_ConnectToDemoServer(qtv, ent->d_name, ip) == true)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			closedir(dir);
		}
		else
		{
			Sys_Printf(qtv->cluster, "Stream %i: Unable to open directory %s\n", qtv->streamid, qtv->cluster->demodir);
			return false;
		}
	}
	#endif

	return false;
}

/*figures out the ip to connect to, and decides the protocol for it*/
char *Net_DiagnoseProtocol(sv_t *qtv)
{
	char *at;
	sourcetype_t type = SRC_BAD;
	char *ip = qtv->server;

	if (!strncmp(ip, "udp:", 4))
	{
		type = SRC_UDP;
		ip += 4;
	}
	else if (!strncmp(ip, "tcp:", 4))
	{
		type = SRC_TCP;
		ip += 4;
	}
	else if (!strncmp(ip, "demo:", 5))
	{
		type = SRC_DEMO;
		ip += 5;
	}
	else if (!strncmp(ip, "file:", 5))
	{
		type = SRC_DEMO;
		ip += 5;
	}
	else if (!strncmp(ip, "dir:", 4))
	{
		type = SRC_DEMODIR;
		ip += 4;
	}

	at = strchrrev(ip, '@');
	if (at && (type == SRC_DEMO || type == SRC_DEMODIR || type == SRC_TCP))
	{
		if (type == SRC_DEMO || type == SRC_DEMODIR)
			type = SRC_TCP;
		ip = at+1;
	}

	qtv->sourcetype = type;
	return ip;
}

qboolean Net_ConnectToServer(sv_t *qtv)
{
	char *ip = Net_DiagnoseProtocol(qtv);

	qtv->usequakeworldprotocols = false;
	qtv->pext1 = 0;
	qtv->pext2 = 0;

	if (qtv->sourcetype == SRC_DEMO || qtv->sourcetype == SRC_DEMODIR)
	{
		qtv->nextconnectattempt = qtv->curtime + RECONNECT_TIME_DEMO;	//wait half a minuite before trying to reconnect
	}
	else
		qtv->nextconnectattempt = qtv->curtime + RECONNECT_TIME;	//wait half a minuite before trying to reconnect

	switch(	qtv->sourcetype)
	{
	case SRC_DEMO:
		return Net_ConnectToDemoServer(qtv, ip, NULL);


	case SRC_DEMODIR:
		return Net_ConnectToDemoDirServer(qtv, ip);


	case SRC_UDP:
		qtv->usequakeworldprotocols = true;
		return Net_ConnectToUDPServer(qtv, ip);

	case SRC_TCP:
		return Net_ConnectToTCPServer(qtv, ip);

	default:
		Sys_Printf(qtv->cluster, "Unknown source type %s\n", ip);
		return false;
	}
}

void Net_QueueUpstream(sv_t *qtv, int size, char *buffer)
{
	if (qtv->usequakeworldprotocols)
		return;

	if (qtv->upstreambuffersize + size > sizeof(qtv->upstreambuffer))
	{
		Sys_Printf(qtv->cluster, "Stream %i: Upstream queue overflowed for %s\n", qtv->streamid, qtv->server);
		strcpy(qtv->status, "Upstream overflow");
		qtv->errored = ERR_RECONNECT;
		return;
	}
	memcpy(qtv->upstreambuffer + qtv->upstreambuffersize, buffer, size);
	qtv->upstreambuffersize += size;
}

qboolean Net_WriteUpstream(sv_t *qtv)
{
	int len;

	if (qtv->upstreambuffersize && qtv->sourcesock != INVALID_SOCKET)
	{
		len = send(qtv->sourcesock, qtv->upstreambuffer, qtv->upstreambuffersize, 0);
		if (len == 0)
			return false;
		if (len < 0)
		{
			int err = qerrno;
			if (err != NET_EWOULDBLOCK && err != NET_EAGAIN && err != NET_ENOTCONN)
			{
				if (err)
				{
					Sys_Printf(qtv->cluster, "Stream %i: Error: source socket error %i (%s)\n", qtv->streamid, err, strerror(err));
					strcpy(qtv->status, "Network error\n");
				}
				else
				{
					Sys_Printf(qtv->cluster, "Stream %i: Error: server %s disconnected\n", qtv->streamid, qtv->server);
					strcpy(qtv->status, "Server disconnected");
				}
				qtv->errored = ERR_RECONNECT;	//if the server is down, we'll detect it on reconnect
			}
			return false;
		}
		qtv->upstreambuffersize -= len;
		memmove(qtv->upstreambuffer, qtv->upstreambuffer + len, qtv->upstreambuffersize);
	}
	return true;
}

void SV_SendUpstream(sv_t *qtv, netmsg_t *nm)
{
	char size[2];
	int ffs = nm->cursize+2;	//qq fucked up and made his upstream inconsistent with downstream. ffs.

	size[0] = (ffs&0x00ff)>>0;
	size[1] = (ffs&0xff00)>>8;
	Net_QueueUpstream(qtv, 2, size);
	Net_QueueUpstream(qtv, nm->cursize, nm->data);
	Net_WriteUpstream(qtv);	//try and flush it
}

int SV_SayToUpstream(sv_t *qtv, char *message)
{
	char buffer[1024];
	netmsg_t nm;

	if (!qtv->upstreamacceptschat)
	{
#ifndef _MSC_VER
#warning This is incomplete!
#endif
		//Sys_Printf(qtv->cluster, "not forwarding say\n");
		return 0;
	}

	InitNetMsg(&nm, buffer, sizeof(buffer));

	WriteByte(&nm, qtv_clc_stringcmd);
	WriteString2(&nm, "say ");
	WriteString(&nm, message);
	SV_SendUpstream(qtv, &nm);

	return 1;
}

void SV_SayToViewers(sv_t *qtv, char *message)
{
	Fwd_SayToDownstream(qtv, message);
#ifndef _MSC_VER
	#warning Send to viewers here too
#endif
}

//This function 1: parses the 'don't delay' packets in the stream
//              2: returns the length of continuous data (that is, whole-packet bytes that have not been truncated by the networking layer)
//                 this means we know that the client proxies have valid data, at least from our side.
int SV_EarlyParse(sv_t *qtv, unsigned char *buffer, int remaining)
{
	int lengthofs;
	int length;
	int available = 0;
	while(1)
	{
		if (remaining < 2)
			return available;

		//buffer[0] is time

		switch (buffer[1]&dem_mask)
		{
		case dem_set:
			lengthofs = 0;	//to silence gcc, nothing more
			break;
		case dem_multiple:
			lengthofs = 6;
			break;
		default:
			lengthofs = 2;
			break;
		}

		if (lengthofs > 0)
		{
			if (lengthofs+4 > remaining)
				return available;

			length = (buffer[lengthofs]<<0) + (buffer[lengthofs+1]<<8) + (buffer[lengthofs+2]<<16) + (buffer[lengthofs+3]<<24);

			length += lengthofs+4;
			if (length > MAX_MSGLEN)
				printf("Probably corrupt mvd (length %i)\n", length);
		}
		else
			length = 10;

		if (remaining < length)
			return available;

		if ((buffer[1]&dem_mask) == dem_all && (buffer[1] & ~dem_mask) && qtv->sourcetype != SRC_DEMO)	//dem_qtvdata
		{
			ParseMessage(qtv, buffer+lengthofs+4, length - (lengthofs+4), buffer[1], 0xffffffff);
		}

		remaining -= length;
		available += length;
		buffer += length;
	}
}

qboolean Net_ReadStream(sv_t *qtv)
{
	int maxreadable;
	int read;
	void *buffer;
	int err;

	maxreadable = MAX_PROXY_BUFFER - qtv->buffersize;
	if (!maxreadable)
		return true;	//this is bad!
	buffer = qtv->buffer + qtv->buffersize;

	if (qtv->sourcefile)
	{
		if (maxreadable > PREFERRED_PROXY_BUFFER-qtv->buffersize)
			maxreadable = PREFERRED_PROXY_BUFFER-qtv->buffersize;
		if (maxreadable<=0)
			return true;

		//reuse read a little...
		read = ftell(qtv->sourcefile);
		if (read+maxreadable > qtv->filelength)
			maxreadable = qtv->filelength-read;	//clamp to the end of the file
								//even if that 'end' is before the svc_disconnect

		read = fread(buffer, 1, maxreadable, qtv->sourcefile);
	}
	else
	{
		unsigned int errsize;
		errsize = sizeof(err);
		err = 0;
		getsockopt(qtv->sourcesock, SOL_SOCKET, SO_ERROR, (char*)&err, &errsize);
		if (err == NET_ECONNREFUSED)
		{
			Sys_Printf(qtv->cluster, "Stream %i: Error: server %s refused connection\n", qtv->streamid, qtv->server);
			closesocket(qtv->sourcesock);
			qtv->sourcesock = INVALID_SOCKET;
			qtv->upstreambuffersize = 0;	//probably contains initial connection request info
			return false;
		}

		read = recv(qtv->sourcesock, buffer, maxreadable, 0);
	}
	if (read > 0)
	{
		qtv->buffersize += read;
		if (!qtv->cluster->lateforward && !qtv->parsingqtvheader)	//qtv header being the auth part of the connection rather than the stream
		{
			int forwardable;
			//this has the effect of not only parsing early packets, but also saying how much complete data there is.
			forwardable = SV_EarlyParse(qtv, qtv->buffer+qtv->forwardpoint, qtv->buffersize - qtv->forwardpoint);
			if (forwardable > 0)
			{
				SV_ForwardStream(qtv, qtv->buffer+qtv->forwardpoint, forwardable);
				qtv->forwardpoint += forwardable;
			}
		}
	}
	else
	{
		if (read == 0)
			err = 0;
		else
			err = qerrno;
		if (read == 0 || (err != NET_EWOULDBLOCK && err != NET_EAGAIN && err != NET_ENOTCONN))	//ENOTCONN can be returned whilst waiting for a connect to finish.
		{
			if (qtv->sourcefile)
				Sys_Printf(qtv->cluster, "Stream %i: Error: End of file\n", qtv->streamid);
			else if (read)
				Sys_Printf(qtv->cluster, "Stream %i: Error: source socket error %i (%s)\n", qtv->streamid, qerrno, strerror(qerrno));
			else
				Sys_Printf(qtv->cluster, "Stream %i: Error: server %s disconnected\n", qtv->streamid, qtv->server);
			if (qtv->sourcesock != INVALID_SOCKET)
			{
				closesocket(qtv->sourcesock);
				qtv->sourcesock = INVALID_SOCKET;
			}
			if (qtv->sourcefile)
			{
				fclose(qtv->sourcefile);
				qtv->sourcefile = NULL;
			}
			return false;
		}
	}
	return true;
}

unsigned int Sys_Milliseconds(void)
{
#ifdef _WIN32
	#ifdef _MSC_VER
		#pragma comment(lib, "winmm.lib")
	#endif

#if 0
	static firsttime = 1;
	static starttime;
	if (firsttime)
	{
		starttime = timeGetTime() + 1000*20;
		firsttime = 0;
	}
	return timeGetTime() - starttime;
#endif



	return timeGetTime();
#else
	//assume every other system follows standards.
	unsigned int t;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	t = ((unsigned int)tv.tv_sec)*1000 + (((unsigned int)tv.tv_usec)/1000);
	return t;
#endif
}
/*
void NetSleep(sv_t *tv)
{
	int m;
	struct timeval timeout;
	fd_set socketset;

	FD_ZERO(&socketset);
	m = 0;
	if (tv->sourcesock != INVALID_SOCKET)
	{
		FD_SET(tv->sourcesock, &socketset);
		if (tv->sourcesock >= m)
			m = tv->sourcesock+1;
	}
	if (tv->qwdsocket != INVALID_SOCKET)
	{
		FD_SET(tv->qwdsocket, &socketset);
		if (tv->sourcesock >= m)
			m = tv->sourcesock+1;
	}

#ifndef _WIN32
	#ifndef STDIN
		#define STDIN 0
	#endif
	FD_SET(STDIN, &socketset);
	if (STDIN >= m)
		m = STDIN+1;
#endif

	timeout.tv_sec = 100/1000;
	timeout.tv_usec = (100%1000)*1000;

	select(m, &socketset, NULL, NULL, &timeout);

#ifdef _WIN32
	for (;;)
	{
		char buffer[8192];
		char *result;
		char c;

		if (!kbhit())
			break;
		c = getch();

		if (c == '\n' || c == '\r')
		{
			printf("\n");
			if (tv->inputlength)
			{
				tv->commandinput[tv->inputlength] = '\0';
				result = Rcon_Command(tv->cluster, tv, tv->commandinput, buffer, sizeof(buffer), true);
				printf("%s", result);
				tv->inputlength = 0;
				tv->commandinput[0] = '\0';
			}
		}
		else if (c == '\b')
		{
			if (tv->inputlength > 0)
			{
				tv->inputlength--;
				tv->commandinput[tv->inputlength] = '\0';
			}
		}
		else
		{
			if (tv->inputlength < sizeof(tv->commandinput)-1)
			{
				tv->commandinput[tv->inputlength++] = c;
				tv->commandinput[tv->inputlength] = '\0';
			}
		}if (FD_ISSET(STDIN, &socketset))
		printf("\r%s \b", tv->commandinput);
	}
#else
	if (FD_ISSET(STDIN, &socketset))
	{
		char buffer[8192];
		char *result;
		tv->inputlength = read (0, tv->commandinput, sizeof(tv->commandinput));
		if (tv->inputlength >= 1)
		{
			tv->commandinput[tv->inputlength-1] = 0;        // rip off the /n and terminate

			if (tv->inputlength)
			{
				tv->commandinput[tv->inputlength] = '\0';
				result = Rcon_Command(tv, tv->commandinput, buffer, sizeof(buffer), true);
				printf("%s", result);
				tv->inputlength = 0;
				tv->commandinput[0] = '\0';
			}
		}
	}
#endif
}
*/

void Trim(char *s)
{
	char *f;
	f = s;
	while(*f <= ' ' && *f > '\0')
		f++;
	while(*f > ' ')
		*s++ = *f++;
	*s = '\0';
}

qboolean QTV_ConnectStream(sv_t *qtv, char *serverurl)
{
	if (qtv->sourcesock != INVALID_SOCKET)
	{
		closesocket(qtv->sourcesock);
		qtv->sourcesock = INVALID_SOCKET;
	}

	if (qtv->sourcefile)
	{
		fclose(qtv->sourcefile);
		qtv->sourcefile = NULL;
	}

	memcpy(qtv->server, serverurl, sizeof(qtv->server)-1);
	if (qtv->autodisconnect == AD_STATUSPOLL && !qtv->numviewers && !qtv->proxies)
	{
		char *ip;
		ip = Net_DiagnoseProtocol(qtv);
		if (!NET_StringToAddr(ip, &qtv->serveraddress, 27500))
		{
			Sys_Printf(qtv->cluster, "Stream %i: Unable to resolve %s\n", qtv->streamid, ip);
			return false;
		}
		return true;
	}

	*qtv->map.serverinfo = '\0';
	Info_SetValueForStarKey(qtv->map.serverinfo, "*version",	"FTEQTV",	sizeof(qtv->map.serverinfo));
	Info_SetValueForStarKey(qtv->map.serverinfo, "*qtv",		QTV_VERSION_STRING,	sizeof(qtv->map.serverinfo));
	Info_SetValueForStarKey(qtv->map.serverinfo, "hostname",	qtv->cluster->hostname,	sizeof(qtv->map.serverinfo));
	Info_SetValueForStarKey(qtv->map.serverinfo, "maxclients",	"99",	sizeof(qtv->map.serverinfo));
	if (!strncmp(qtv->server, "file:", 5))
		Info_SetValueForStarKey(qtv->map.serverinfo, "server",		"file",	sizeof(qtv->map.serverinfo));
	else
		Info_SetValueForStarKey(qtv->map.serverinfo, "server",		qtv->server,	sizeof(qtv->map.serverinfo));

	if (qtv->autodisconnect == AD_REVERSECONNECT)
	{	//added because of paranoia rather than need. Should never occur.
		printf("bug: autoclose==2\n");
		strcpy(qtv->status, "Network error\n");
		return false;
	}
	else if (!Net_ConnectToServer(qtv))
	{
		Sys_Printf(qtv->cluster, "Stream %i: Couldn't connect (%s)\n", qtv->streamid, qtv->server);
		return false;
	}

	if (qtv->sourcesock == INVALID_SOCKET)
	{
		qtv->parsetime = Sys_Milliseconds();
//		Sys_Printf(qtv->cluster, "Stream %i: Playing from file\n", qtv->streamid);
	}
	else
	{
		qtv->parsetime = Sys_Milliseconds() + qtv->cluster->anticheattime;
	}
	return true;
}

void QTV_CleanupMap(sv_t *qtv)
{
	int i;

	//free the bsp
	BSP_Free(qtv->map.bsp);
	qtv->map.bsp = NULL;

	//clean up entity state
	for (i = 0; i < ENTITY_FRAMES; i++)
	{
		if (qtv->map.frame[i].ents)
		{
			free(qtv->map.frame[i].ents);
			qtv->map.frame[i].ents = NULL;
		}
		if (qtv->map.frame[i].entnums)
		{
			free(qtv->map.frame[i].entnums);
			qtv->map.frame[i].entnums = NULL;
		}
	}
	memset(&qtv->map, 0, sizeof(qtv->map));
}

void QTV_DisconnectFromSource(sv_t *qtv)
{
		// close the source handle
	if (qtv->sourcesock != INVALID_SOCKET)
	{
		if (qtv->usequakeworldprotocols)
		{
			char dying[] = {clc_stringcmd, 'd', 'r', 'o', 'p', '\0'};
			Netchan_Transmit (qtv->cluster, &qtv->netchan, sizeof(dying), dying);
			Netchan_Transmit (qtv->cluster, &qtv->netchan, sizeof(dying), dying);
			Netchan_Transmit (qtv->cluster, &qtv->netchan, sizeof(dying), dying);
		}
		closesocket(qtv->sourcesock);
		qtv->sourcesock = INVALID_SOCKET;
	}
	if (qtv->sourcefile)
	{
		fclose(qtv->sourcefile);
		qtv->sourcefile = NULL;
	}

	//cancel downloads
	if (qtv->downloadfile)
	{
		fclose(qtv->downloadfile);
		qtv->downloadfile = NULL;
		unlink(qtv->downloadname);
		*qtv->downloadname = '\0';
	}
}

void QTV_Cleanup(sv_t *qtv, qboolean leaveadmins)
{	//disconnects the stream
	viewer_t *v;
	cluster_t *cluster;
	oproxy_t *prox;
	oproxy_t *old;

	cluster = qtv->cluster;

	//set connected viewers to a different stream
	if (cluster->viewserver == qtv)
		cluster->viewserver = NULL;
	for (v = cluster->viewers; v; v = v->next)
	{
	//warning fixme: honour leaveadmins
		if (v->server == qtv)
		{	//they were watching this one
			QW_SetViewersServer(qtv->cluster, v, NULL);
			QW_SetMenu(v, MENU_NONE);
			QTV_SayCommand(cluster, v->server, v, "menu");
			QW_PrintfToViewer(v, "Stream %s is closing\n", qtv->server);
		}
	}

	QTV_DisconnectFromSource(qtv);

	QTV_CleanupMap(qtv);

	//boot connected downstream proxies
	for (prox = qtv->proxies; prox; )
	{
		if (prox->file)
			fclose(prox->file);
		if (prox->sock != INVALID_SOCKET)
			closesocket(prox->sock);
		old = prox;
		prox = prox->next;
		free(old);
		cluster->numproxies--;
	}
}

void QTV_ShutdownStream(sv_t *qtv)
{
	sv_t *peer;
	cluster_t *cluster;
	Sys_Printf(qtv->cluster, "Stream %i: Closing source %s\n", qtv->streamid, qtv->server);

	QTV_Cleanup(qtv, false);

	//unlink it
	cluster = qtv->cluster;
	if (cluster->servers == qtv)
		cluster->servers = qtv->next;
	else
	{
		for (peer = cluster->servers; peer->next; peer = peer->next)
		{
			if (peer->next == qtv)
			{
				peer->next = qtv->next;
				break;
			}
		}
	}

	free(qtv);
	cluster->numservers--;
}









void SendClientCommand(sv_t *qtv, char *fmt, ...)
{
	va_list		argptr;
	char buf[1024];

	va_start (argptr, fmt);
	vsnprintf (buf, sizeof(buf), fmt, argptr);
	va_end (argptr);

	WriteByte(&qtv->netchan.message, clc_stringcmd);
	WriteString(&qtv->netchan.message, buf);
}






void ChooseFavoriteTrack(sv_t *tv)
{
	int frags, best, pnum;
	char buffer[64];

	frags = -10000;
	best = -1;
	if (tv->controller || tv->proxyplayer)
		best = tv->map.trackplayer;
	else
	{
		for (pnum = 0; pnum < MAX_CLIENTS; pnum++)
		{
			if (*tv->map.players[pnum].userinfo && !atoi(Info_ValueForKey(tv->map.players[pnum].userinfo, "*spectator", buffer, sizeof(buffer))))
			{
				if (tv->map.thisplayer == pnum)
					continue;
				if (frags < tv->map.players[pnum].frags)
				{
					best = pnum;
					frags = tv->map.players[pnum].frags;
				}
			}
		}
	}
	if (best != tv->map.trackplayer)
	{
		SendClientCommand (tv, "ptrack %i\n", best);
		tv->map.trackplayer = best;

		if (tv->usequakeworldprotocols)
			QW_StreamStuffcmd(tv->cluster, tv, "track %i\n", best);
	}
}






static const unsigned char chktbl[1024] = {
0x78,0xd2,0x94,0xe3,0x41,0xec,0xd6,0xd5,0xcb,0xfc,0xdb,0x8a,0x4b,0xcc,0x85,0x01,
0x23,0xd2,0xe5,0xf2,0x29,0xa7,0x45,0x94,0x4a,0x62,0xe3,0xa5,0x6f,0x3f,0xe1,0x7a,
0x64,0xed,0x5c,0x99,0x29,0x87,0xa8,0x78,0x59,0x0d,0xaa,0x0f,0x25,0x0a,0x5c,0x58,
0xfb,0x00,0xa7,0xa8,0x8a,0x1d,0x86,0x80,0xc5,0x1f,0xd2,0x28,0x69,0x71,0x58,0xc3,
0x51,0x90,0xe1,0xf8,0x6a,0xf3,0x8f,0xb0,0x68,0xdf,0x95,0x40,0x5c,0xe4,0x24,0x6b,
0x29,0x19,0x71,0x3f,0x42,0x63,0x6c,0x48,0xe7,0xad,0xa8,0x4b,0x91,0x8f,0x42,0x36,
0x34,0xe7,0x32,0x55,0x59,0x2d,0x36,0x38,0x38,0x59,0x9b,0x08,0x16,0x4d,0x8d,0xf8,
0x0a,0xa4,0x52,0x01,0xbb,0x52,0xa9,0xfd,0x40,0x18,0x97,0x37,0xff,0xc9,0x82,0x27,
0xb2,0x64,0x60,0xce,0x00,0xd9,0x04,0xf0,0x9e,0x99,0xbd,0xce,0x8f,0x90,0x4a,0xdd,
0xe1,0xec,0x19,0x14,0xb1,0xfb,0xca,0x1e,0x98,0x0f,0xd4,0xcb,0x80,0xd6,0x05,0x63,
0xfd,0xa0,0x74,0xa6,0x86,0xf6,0x19,0x98,0x76,0x27,0x68,0xf7,0xe9,0x09,0x9a,0xf2,
0x2e,0x42,0xe1,0xbe,0x64,0x48,0x2a,0x74,0x30,0xbb,0x07,0xcc,0x1f,0xd4,0x91,0x9d,
0xac,0x55,0x53,0x25,0xb9,0x64,0xf7,0x58,0x4c,0x34,0x16,0xbc,0xf6,0x12,0x2b,0x65,
0x68,0x25,0x2e,0x29,0x1f,0xbb,0xb9,0xee,0x6d,0x0c,0x8e,0xbb,0xd2,0x5f,0x1d,0x8f,
0xc1,0x39,0xf9,0x8d,0xc0,0x39,0x75,0xcf,0x25,0x17,0xbe,0x96,0xaf,0x98,0x9f,0x5f,
0x65,0x15,0xc4,0x62,0xf8,0x55,0xfc,0xab,0x54,0xcf,0xdc,0x14,0x06,0xc8,0xfc,0x42,
0xd3,0xf0,0xad,0x10,0x08,0xcd,0xd4,0x11,0xbb,0xca,0x67,0xc6,0x48,0x5f,0x9d,0x59,
0xe3,0xe8,0x53,0x67,0x27,0x2d,0x34,0x9e,0x9e,0x24,0x29,0xdb,0x69,0x99,0x86,0xf9,
0x20,0xb5,0xbb,0x5b,0xb0,0xf9,0xc3,0x67,0xad,0x1c,0x9c,0xf7,0xcc,0xef,0xce,0x69,
0xe0,0x26,0x8f,0x79,0xbd,0xca,0x10,0x17,0xda,0xa9,0x88,0x57,0x9b,0x15,0x24,0xba,
0x84,0xd0,0xeb,0x4d,0x14,0xf5,0xfc,0xe6,0x51,0x6c,0x6f,0x64,0x6b,0x73,0xec,0x85,
0xf1,0x6f,0xe1,0x67,0x25,0x10,0x77,0x32,0x9e,0x85,0x6e,0x69,0xb1,0x83,0x00,0xe4,
0x13,0xa4,0x45,0x34,0x3b,0x40,0xff,0x41,0x82,0x89,0x79,0x57,0xfd,0xd2,0x8e,0xe8,
0xfc,0x1d,0x19,0x21,0x12,0x00,0xd7,0x66,0xe5,0xc7,0x10,0x1d,0xcb,0x75,0xe8,0xfa,
0xb6,0xee,0x7b,0x2f,0x1a,0x25,0x24,0xb9,0x9f,0x1d,0x78,0xfb,0x84,0xd0,0x17,0x05,
0x71,0xb3,0xc8,0x18,0xff,0x62,0xee,0xed,0x53,0xab,0x78,0xd3,0x65,0x2d,0xbb,0xc7,
0xc1,0xe7,0x70,0xa2,0x43,0x2c,0x7c,0xc7,0x16,0x04,0xd2,0x45,0xd5,0x6b,0x6c,0x7a,
0x5e,0xa1,0x50,0x2e,0x31,0x5b,0xcc,0xe8,0x65,0x8b,0x16,0x85,0xbf,0x82,0x83,0xfb,
0xde,0x9f,0x36,0x48,0x32,0x79,0xd6,0x9b,0xfb,0x52,0x45,0xbf,0x43,0xf7,0x0b,0x0b,
0x19,0x19,0x31,0xc3,0x85,0xec,0x1d,0x8c,0x20,0xf0,0x3a,0xfa,0x80,0x4d,0x2c,0x7d,
0xac,0x60,0x09,0xc0,0x40,0xee,0xb9,0xeb,0x13,0x5b,0xe8,0x2b,0xb1,0x20,0xf0,0xce,
0x4c,0xbd,0xc6,0x04,0x86,0x70,0xc6,0x33,0xc3,0x15,0x0f,0x65,0x19,0xfd,0xc2,0xd3
};


unsigned char	COM_BlockSequenceCRCByte (void *base, int length, int sequence)
{
	unsigned short crc;
	const unsigned char	*p;
	unsigned char chkb[60 + 4];

	p = chktbl + (sequence % (sizeof(chktbl) - 4));

	if (length > 60)
		length = 60;
	memcpy (chkb, base, length);

	chkb[length] = (sequence & 0xff) ^ p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = ((sequence>>8) & 0xff) ^ p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = QCRC_Block(chkb, length);

	crc &= 0xff;

	return crc;
}
void SetMoveCRC(sv_t *qtv, netmsg_t *msg)
{
	char *outbyte;
	outbyte = (char*)msg->data + msg->startpos+1;

	*outbyte = COM_BlockSequenceCRCByte(
				outbyte+1, msg->cursize - (msg->startpos+2),
				qtv->netchan.outgoing_sequence);
}





void QTV_ParseQWStream(sv_t *qtv)
{
	char buffer[1500];
	netadr_t from;
	unsigned int fromlen;
	int readlen;
	netmsg_t msg;
	fromlen = sizeof(from.sockaddr);	//bug: this won't work on (free)bsd

	for (;;)
	{
		from.tcpcon = NULL;
		readlen = recvfrom(qtv->sourcesock, buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&from.sockaddr, &fromlen);
		if (readlen < 0)
		{
			//FIXME: Check for error
			break;
		}
		if (readlen > sizeof(buffer)-1)
			break;	//oversized!

		buffer[readlen] = 0;
		if (*(int*)buffer == -1)
		{
			if (buffer[4] == 'c')
			{	//got a challenge
				strcpy(qtv->status, "Attemping connection\n");
				qtv->challenge = atoi(buffer+5);
				if (qtv->controller)
					sprintf(buffer, "connect %i %i %i \"%s\\*qtv\\1\\Qizmo\\2.9 notimer\"", 28, qtv->qport, qtv->challenge, qtv->controller->userinfo);
				else if (qtv->proxyplayer)
					sprintf(buffer, "connect %i %i %i \"%s\\name\\%s\"", 28, qtv->qport, qtv->challenge, "\\*ver\\fteqtv\\spectator\\0\\rate\\10000", qtv->cluster->hostname);
				else
					sprintf(buffer, "connect %i %i %i \"%s\\name\\%s\"", 28, qtv->qport, qtv->challenge, "\\*ver\\fteqtv\\spectator\\1\\rate\\10000", qtv->cluster->hostname);
				Netchan_OutOfBandSocket(qtv->cluster, qtv->sourcesock, &qtv->serveraddress, strlen(buffer), buffer);
				continue;
			}
			if (buffer[4] == 'n')
			{
				strlcpy(qtv->status, buffer+5, sizeof(qtv->status));
				Sys_Printf(qtv->cluster, "%s: %s", qtv->server, buffer+5);
				continue;
			}
			if (buffer[4] == 'j')
			{
				strcpy(qtv->status, "Waiting for gamestate\n");
				Netchan_Setup(qtv->sourcesock, &qtv->netchan, qtv->serveraddress, qtv->qport, true);

				qtv->map.trackplayer = -1;

				qtv->isconnected = true;
				qtv->timeout = qtv->curtime + UDPTIMEOUT_LENGTH;
				SendClientCommand(qtv, "new");
				Sys_Printf(qtv->cluster, "Stream %i: Connected!\n", qtv->streamid);
				continue;
			}
			Sys_Printf(qtv->cluster, "Stream %i: %s: unrecognized connectionless packet:\n%s\n", qtv->streamid, qtv->server, buffer+4);
			continue;
		}
		memset(&msg, 0, sizeof(msg));
		msg.cursize = readlen;
		msg.data = buffer;
		msg.maxsize = readlen;
		qtv->timeout = qtv->curtime + UDPTIMEOUT_LENGTH;
		if (!Netchan_Process(&qtv->netchan, &msg))
			continue;
		ParseMessage(qtv, (char*)msg.data + msg.readpos, msg.cursize - msg.readpos, dem_all, -1);

		qtv->oldpackettime = qtv->nextpackettime;
		qtv->nextpackettime = qtv->parsetime;
		qtv->parsetime = qtv->curtime;

		if (qtv->simtime < qtv->oldpackettime)
			qtv->simtime = qtv->oldpackettime;	//too old

		if (qtv->controller)
		{
			qtv->controller->maysend = true;
//if (qtv->controller->netchan.outgoing_sequence != qtv->controller->netchan.incoming_sequence)
//printf("bug is here\n");
		}
	}
}

#ifdef COMMENTARY
#include <speex/speex.h>
#endif

void QTV_CollectCommentry(sv_t *qtv)
{
#define usespeex 0
#ifdef COMMENTARY
	int samps;
	unsigned char buffer[8192+6];
	unsigned char *uchar;
	signed char *schar;
	int bytesleft;
	if (!qtv->comentrycapture)
	{
		if (0)
		{
//			if (usespeex)
//				qtv->comentrycapture = SND_InitCapture(11025, 16);
//			else
				qtv->comentrycapture = SND_InitCapture(11025, 8);
		}
		return;
	}

	while(1)
	{
		//the protocol WILL be different. Don't add compatibility for this code.
		buffer[0] = 0;
		buffer[1] = dem_audio;
		buffer[2] = 255;
		buffer[3] = 255;
		buffer[4] = 8;
		buffer[5] = 11*5;

	/*	if (usespeex)
		{

			SpeexBits bits;
			void *enc_state;

			int frame_size;

			spx_int16_t pcmdata[8192/2];

			samps=qtv->comentrycapture->update(qtv->comentrycapture, 2048, (char*)pcmdata);


			speex_bits_init(&bits);

			enc_state = speex_encoder_init(&speex_nb_mode);


			speex_encoder_ctl(enc_state,SPEEX_GET_FRAME_SIZE,&frame_size);


			speex_bits_reset(&bits);

			speex_encode_int(enc_state, (spx_int16_t*)pcmdata, &bits);

			samps = speex_bits_write(&bits, buffer+6, sizeof(buffer)-6);


			speex_bits_destroy(&bits);

			speex_encoder_destroy(enc_state);

		}
		else*/
		{
			samps=qtv->comentrycapture->update(qtv->comentrycapture, 2048, buffer+6);

			bytesleft = samps;
			schar = buffer+6;
			uchar = buffer+6;
			while(bytesleft-->0)
			{
				*schar++ = *uchar++ - 128;
			}
		}

		buffer[2] = samps&255;
		buffer[3] = samps>>8;

		if (samps)
			SV_ForwardStream(qtv, buffer, 6 + samps);

		if (samps < 64)
			break;
	}
#endif
}

void QTV_Run(sv_t *qtv)
{
	int from;
	int to;
	int lengthofs;
	unsigned int length;
	unsigned char *buffer;
	int oldcurtime;
	int packettime;

	if (qtv->numviewers == 0 && qtv->proxies == NULL)
	{
		if (qtv->autodisconnect == AD_WHENEMPTY)
		{
			Sys_Printf(qtv->cluster, "Stream %i: %s became inactive\n", qtv->streamid, qtv->server);
			qtv->errored = ERR_DROP;
		}
		else if (qtv->autodisconnect == AD_STATUSPOLL && qtv->isconnected)
		{
			/*switch to status polling instead of packet spamming*/
			qtv->errored = ERR_RECONNECT;
		}
	}
	if (qtv->errored)
	{
		if (qtv->errored == ERR_DISABLED)
		{
			//this keeps any connected proxies ticking over.
			//probably we should drop them instead - the connection will only be revived if one of them reconnects.
			SV_ForwardStream(qtv, NULL, 0);
			return;
		}
		else if (qtv->errored == ERR_PERMANENT)
		{
			QTV_Cleanup(qtv, false);	//frees various pieces of context
			qtv->errored = ERR_DISABLED;
			return;
		}
		else if (qtv->errored == ERR_DROP)
		{
			QTV_ShutdownStream(qtv);	//destroys the stream
			return;
		}
	}



//we will read out as many packets as we can until we're up to date
//note: this can cause real issues when we're overloaded for any length of time
//each new packet comes with a leading msec byte (msecs from last packet)
//then a type, an optional destination mask, and a 4byte size.
//the 4 byte size is probably excessive, a short would do.
//some of the types have thier destination mask encoded inside the type byte, yielding 8 types, and 32 max players.


//if we've no got enough data to read a new packet, we print a message and wait an extra two seconds. this will add a pause, connected clients will get the same pause, and we'll just try to buffer more of the game before playing.
//we'll stay 2 secs behind when the tcp stream catches up, however. This could be bad especially with long up-time.
//All timings are in msecs, which is in keeping with the mvd times, but means we might have issues after 72 or so days.
//the following if statement will reset the parse timer. It might cause the game to play too soon, the buffersize checks in the rest of the function will hopefully put it back to something sensible.

	oldcurtime = qtv->curtime;
	qtv->curtime = Sys_Milliseconds();
	if (oldcurtime > qtv->curtime)
	{
		Sys_Printf(qtv->cluster, "Time wrapped\n");
		qtv->parsetime = qtv->curtime;
	}


	if (qtv->errored == ERR_PAUSED)
	{
		if (!qtv->parsingconnectiondata)
			qtv->parsetime = qtv->curtime;
	}

	if (qtv->errored == ERR_RECONNECT)
	{
		qtv->buffersize = 0;
		qtv->forwardpoint = 0;
		QTV_DisconnectFromSource(qtv);
		qtv->isconnected = 0;
		qtv->errored = ERR_NONE;
		qtv->nextconnectattempt = qtv->curtime;	//make the reconnect happen _now_
	}


	if (qtv->sourcetype == SRC_UDP)
	{
		qtv->simtime += qtv->curtime - oldcurtime;

		if (qtv->simtime > qtv->nextpackettime)
			qtv->simtime = qtv->nextpackettime;	//too old

		if (!qtv->isconnected && (qtv->curtime >= qtv->nextconnectattempt || qtv->curtime < qtv->nextconnectattempt - (UDPRECONNECT_TIME+STATUSPOLL_TIME)))
		{
			if (qtv->errored == ERR_DISABLED)
			{
				strcpy(qtv->status, "Given up connecting\n");
			}
			else if (qtv->autodisconnect == AD_STATUSPOLL)
			{
				QTV_DisconnectFromSource(qtv);
				Netchan_OutOfBand(qtv->cluster, qtv->serveraddress, 13, "status\n");
				qtv->nextconnectattempt = qtv->curtime + STATUSPOLL_TIME;
				return;
			}
			else
			{
				strcpy(qtv->status, "Attemping challenge\n");
				if (qtv->sourcesock == INVALID_SOCKET && !qtv->sourcefile)
				{
					if (!QTV_ConnectStream(qtv, qtv->server))	//reconnect it
					{
						qtv->errored = ERR_PERMANENT;
					}
				}
				if (qtv->errored == ERR_NONE)
					Netchan_OutOfBandSocket(qtv->cluster, qtv->sourcesock, &qtv->serveraddress, 13, "getchallenge\n");
			}
			qtv->nextconnectattempt = qtv->curtime + UDPRECONNECT_TIME;
		}
		if (qtv->sourcesock == INVALID_SOCKET && !qtv->sourcefile)
			return;

		QTV_ParseQWStream(qtv);

		if (qtv->isconnected)
		{
			char buffer[128];
			netmsg_t msg;
			memset(&msg, 0, sizeof(msg));
			msg.data = buffer;
			msg.maxsize = sizeof(buffer);

			if (qtv->curtime >= qtv->timeout || qtv->curtime < qtv->timeout - UDPTIMEOUT_LENGTH*2)
			{
				Sys_Printf(qtv->cluster, "Stream %i: Timeout\n", qtv->streamid);
				qtv->isconnected = false;
				return;
			}

			if (qtv->controller && !qtv->controller->netchan.isnqprotocol)
			{
				qtv->netchan.outgoing_sequence = qtv->controller->netchan.incoming_sequence;
				if (qtv->maysend)
				{
					qtv->maysend = false;
					qtv->packetratelimiter = qtv->curtime;
				}
				else
					qtv->packetratelimiter = qtv->curtime + 1;
			}
			else
			{
				if (qtv->curtime < qtv->packetratelimiter - UDPPACKETINTERVAL*2)
					qtv->packetratelimiter = qtv->curtime;
			}
			if (qtv->curtime >= qtv->packetratelimiter)
			{
				if (qtv->curtime >= qtv->nextsendpings || qtv->curtime < qtv->nextsendpings - PINGSINTERVAL_TIME*2)
				{
					qtv->nextsendpings = qtv->curtime + PINGSINTERVAL_TIME;
					SendClientCommand(qtv, "pings\n");

				}
				ChooseFavoriteTrack(qtv);

				//if we froze somehow, don't speedcheat by a burst of 10000+ packets while we were frozen in a debugger or disk spinup or whatever
				if (qtv->packetratelimiter < qtv->curtime - UDPPACKETINTERVAL*2)
					qtv->packetratelimiter = qtv->curtime;

				if (qtv->map.trackplayer >= 0)
				{
					qtv->packetratelimiter += UDPPACKETINTERVAL;

					WriteByte(&msg, clc_tmove);
					WriteShort(&msg, qtv->map.players[qtv->map.trackplayer].current.origin[0]);
					WriteShort(&msg, qtv->map.players[qtv->map.trackplayer].current.origin[1]);
					WriteShort(&msg, qtv->map.players[qtv->map.trackplayer].current.origin[2]);
				}
				else if (qtv->controller)
				{
					qtv->packetratelimiter += UDPPACKETINTERVAL;

					if (qtv->controller->netchan.isnqprotocol)
					{
						memcpy(&qtv->controller->ucmds[0], &qtv->controller->ucmds[1], sizeof(qtv->controller->ucmds[0]));
						memcpy(&qtv->controller->ucmds[1], &qtv->controller->ucmds[2], sizeof(qtv->controller->ucmds[0]));
						qtv->controller->ucmds[2].msec = UDPPACKETINTERVAL;
					}

					WriteByte(&msg, clc_tmove);
					WriteShort(&msg, qtv->controller->origin[0]);
					WriteShort(&msg, qtv->controller->origin[1]);
					WriteShort(&msg, qtv->controller->origin[2]);

/*					qtv->controller->ucmds[0].angles[1] = qtv->curtime*120;
					qtv->controller->ucmds[1].angles[1] = qtv->curtime*120;
					qtv->controller->ucmds[2].angles[1] = qtv->curtime*120;
*/
					msg.startpos = msg.cursize;
					WriteByte(&msg, clc_move);
					WriteByte(&msg, 0);
					WriteByte(&msg, 0);
					WriteDeltaUsercmd(&msg, &nullcmd, &qtv->controller->ucmds[0]);
					WriteDeltaUsercmd(&msg, &qtv->controller->ucmds[0], &qtv->controller->ucmds[1]);
					WriteDeltaUsercmd(&msg, &qtv->controller->ucmds[1], &qtv->controller->ucmds[2]);

					SetMoveCRC(qtv, &msg);
				}
				else if (qtv->proxyplayer || qtv->map.trackplayer < 0)
				{
					usercmd_t *cmd[3];
					cmd[0] = &qtv->proxyplayerucmds[(qtv->proxyplayerucmdnum-2)%3];
					cmd[1] = &qtv->proxyplayerucmds[(qtv->proxyplayerucmdnum-1)%3];
					cmd[2] = &qtv->proxyplayerucmds[(qtv->proxyplayerucmdnum-0)%3];

					cmd[2]->angles[0] = (qtv->proxyplayerangles[0]/360)*0x10000;
					cmd[2]->angles[1] = (qtv->proxyplayerangles[1]/360)*0x10000;
					cmd[2]->angles[2] = (qtv->proxyplayerangles[2]/360)*0x10000;
					cmd[2]->buttons = qtv->proxyplayerbuttons & 255;
					cmd[2]->forwardmove = (qtv->proxyplayerbuttons & (1<<8))?800:0 + (qtv->proxyplayerbuttons & (1<<9))?-800:0;
					cmd[2]->sidemove = (qtv->proxyplayerbuttons & (1<<11))?800:0 + (qtv->proxyplayerbuttons & (1<<10))?-800:0;
					cmd[2]->msec = qtv->curtime - qtv->packetratelimiter;
					cmd[2]->impulse = qtv->proxyplayerimpulse;
					if (cmd[2]->msec < 13)
						cmd[2]->msec = 13;
					qtv->packetratelimiter += cmd[2]->msec;
					qtv->proxyplayerimpulse = 0;


					msg.startpos = msg.cursize;
					WriteByte(&msg, clc_move);
					WriteByte(&msg, 0);
					WriteByte(&msg, 0);
					WriteDeltaUsercmd(&msg, &nullcmd, cmd[0]);
					WriteDeltaUsercmd(&msg, cmd[0], cmd[1]);
					WriteDeltaUsercmd(&msg, cmd[1], cmd[2]);
					qtv->proxyplayerucmdnum++;

					SetMoveCRC(qtv, &msg);
				}

				to = qtv->netchan.outgoing_sequence & (ENTITY_FRAMES-1);
				from = qtv->netchan.incoming_sequence & (ENTITY_FRAMES-1);
				if (qtv->map.frame[from].numents)
				{
					//remember which one we came from
					qtv->map.frame[to].oldframe = from;

					WriteByte(&msg, clc_delta);
					WriteByte(&msg, qtv->map.frame[to].oldframe);	//let the server know
				}
				else
					qtv->map.frame[to].oldframe = -1;

				qtv->map.frame[to].numents = 0;

				Netchan_Transmit(qtv->cluster, &qtv->netchan, msg.cursize, msg.data);
			}
		}
		return;
	}
	else
		qtv->simtime = qtv->curtime;


	if (qtv->sourcesock == INVALID_SOCKET && !qtv->sourcefile)
	{
		if (qtv->errored == ERR_DISABLED)
			return;

		if (qtv->sourcetype == SRC_DEMODIR || qtv->curtime >= qtv->nextconnectattempt || qtv->curtime < qtv->nextconnectattempt - RECONNECT_TIME*2)
		{
			if (qtv->autodisconnect == AD_REVERSECONNECT)	//2 means a reverse connection
			{
				qtv->errored = ERR_DROP;
				return;
			}
			if (!QTV_ConnectStream(qtv, qtv->server))	//reconnect it
			{
				qtv->errored = ERR_PERMANENT;
				return;
			}
		}
	}


//	SV_FindProxies(qtv->tcpsocket, qtv->cluster, qtv);	//look for any other proxies wanting to muscle in on the action.

	if (qtv->sourcefile || qtv->sourcesock != INVALID_SOCKET)
	{
		if (!Net_ReadStream(qtv))
		{	//if we have an error reading it
			//if it's valid, give up
			//what should we do here?
			//obviously, we need to keep reading the stream to keep things smooth
		}

		Net_WriteUpstream(qtv);
	}


	if (qtv->parsingqtvheader)
	{
		float svversion;
		int length;
		char *start;
		char *nl;
		char *colon;
		char *end;
		char value[128];
		char challenge[128];
		char authmethod[128];

//		qtv->buffer[qtv->buffersize] = 0;
//		Sys_Printf(qtv->cluster, "msg: ---%s---\n", qtv->buffer);

		*authmethod = 0;

		qtv->parsetime = qtv->curtime;
		length = qtv->buffersize;
		if (length > 6)
			length = 6;
		if (ustrncmp(qtv->buffer, "QTVSV ", length))
		{
			Sys_Printf(qtv->cluster, "Stream %i: Server is not a QTV server (or is incompatible)\n", qtv->streamid);
//printf("%i, %s\n", qtv->buffersize, qtv->buffer);
			qtv->errored = ERR_PERMANENT;
			return;
		}
		if (length < 6)
			return;	//not ready yet
		end = (char*)qtv->buffer + qtv->buffersize - 1;
		for (nl = (char*)qtv->buffer; nl < end; nl++)
		{
			if (nl[0] == '\n' && nl[1] == '\n')
				break;
		}
		if (nl == end)
			return;	//we need more header still

		//we now have a complete packet.

		svversion = atof((char*)qtv->buffer + 6);
		if ((int)svversion != 1)
		{
			Sys_Printf(qtv->cluster, "Stream %i: QTV server doesn't support a compatible protocol version (returned %i)\n", qtv->streamid, atoi((char*)qtv->buffer + 6));
			qtv->errored = ERR_PERMANENT;
			return;
		}


		qtv->upstreamacceptschat = svversion>=1.1;
		qtv->upstreamacceptsdownload = svversion>=1.1;

		length = (nl - (char*)qtv->buffer) + 2;
		end = nl;
		nl[1] = '\0';
		start = strchr((char*)qtv->buffer, '\n')+1;

		while((nl = strchr(start, '\n')))
		{
			*nl = '\0';
			colon = strchr(start, ':');
			if (colon)
			{
				*colon = '\0';
				colon++;
				while (*colon == ' ')
					colon++;
				COM_ParseToken(colon, value, sizeof(value), NULL);
			}
			else
			{
				colon = "";
				*value = '\0';
			}


			//read the notes at the top of this file for which messages to expect
			if (!strcmp(start, "AUTH"))
				strcpy(authmethod, value);
			else if (!strcmp(start, "CHALLENGE"))
				strcpy(challenge, colon);
			else if (!strcmp(start, "COMPRESSION"))
			{	//we don't support compression, we didn't ask for it.
				Sys_Printf(qtv->cluster, "Stream %i: QTV server wrongly used compression\n", qtv->streamid);
				qtv->errored = ERR_PERMANENT;
				return;
			}
			else if (!strcmp(start, "PERROR"))
			{
				Sys_Printf(qtv->cluster, "\nStream %i: Server PERROR from %s: %s\n\n", qtv->streamid, qtv->server, colon);
				qtv->errored = ERR_PERMANENT;
				qtv->buffersize = 0;
				qtv->forwardpoint = 0;
				return;
			}
			else if (!strcmp(start, "TERROR") || !strcmp(start, "ERROR"))
			{	//we don't support compression, we didn't ask for it.
				Sys_Printf(qtv->cluster, "\nStream %i: Server TERROR from %s: %s\n\n", qtv->streamid, qtv->server, colon);
				qtv->buffersize = 0;
				qtv->forwardpoint = 0;

				if (qtv->autodisconnect == AD_WHENEMPTY || qtv->autodisconnect == AD_REVERSECONNECT)
					qtv->errored = ERR_DROP;	//if its a user registered stream, drop it immediatly
				else
				{	//otherwise close the socket (this will result in a timeout and reconnect)
					if (qtv->sourcesock != INVALID_SOCKET)
					{
						closesocket(qtv->sourcesock);
						qtv->sourcesock = INVALID_SOCKET;
					}
				}
				return;
			}
			else if (!strcmp(start, "ASOURCE"))
			{
				Sys_Printf(qtv->cluster, "SRC: %s\n", colon);
			}
			else if (!strcmp(start, "ADEMO"))
			{
				int size;
				size = atoi(colon);
				colon = strchr(colon, ':');
				if (!colon)
					colon = "";
				else
					colon = colon+1;
				while(*colon == ' ')
					colon++;
				if (size > 1024*1024)
					Sys_Printf(qtv->cluster, "DEMO: (%3imb) %s\n", size/(1024*1024), colon);
				else
					Sys_Printf(qtv->cluster, "DEMO: (%3ikb) %s\n", size/1024, colon);
			}
			else if (!strcmp(start, "PRINT"))
			{
				Sys_Printf(qtv->cluster, "Stream %i: QTV server: %s\n", qtv->streamid, colon);
			}
			else if (!strcmp(start, "BEGIN"))
			{
				qtv->parsingqtvheader = false;
			}
			else
			{
				Sys_Printf(qtv->cluster, "DBG: QTV server responded with a %s key\n", start);
			}

			start = nl+1;
		}

		qtv->buffersize -= length;
		memmove(qtv->buffer, qtv->buffer + length, qtv->buffersize);

		if (qtv->serverquery)
		{
			Sys_Printf(qtv->cluster, "End of list\n");
			qtv->errored = ERR_DROP;
			qtv->buffersize = 0;
			qtv->forwardpoint = 0;
			return;
		}
		else if (*authmethod)
		{	//we need to send a challenge response now.
			Net_SendQTVConnectionRequest(qtv, authmethod, challenge);
			return;
		}
		else if (qtv->parsingqtvheader)
		{
			Sys_Printf(qtv->cluster, "Stream %i: QTV server sent no begin command - assuming incompatible\n\n", qtv->streamid);
			qtv->errored = ERR_PERMANENT;
			qtv->buffersize = 0;
			qtv->forwardpoint = 0;
			return;
		}

		qtv->parsetime = Sys_Milliseconds() + qtv->cluster->anticheattime;
		if (!qtv->usequakeworldprotocols)
			Sys_Printf(qtv->cluster, "Stream %i: Connection established, buffering for %g seconds\n", qtv->streamid, qtv->cluster->anticheattime/1000.0f);

		SV_ForwardStream(qtv, qtv->buffer, qtv->forwardpoint);
	}

	QTV_CollectCommentry(qtv);

	while (qtv->curtime >= qtv->parsetime)
	{
		if (qtv->buffersize < 2)
		{	//not enough stuff to play.
			if (qtv->parsetime < qtv->curtime)
			{
				qtv->parsetime = qtv->curtime + qtv->cluster->tooslowdelay;
//				if (qtv->sourcefile || qtv->sourcesock != INVALID_SOCKET)
//					QTV_Printf(qtv, "Stream %i: Not enough buffered\n", qtv->streamid);
			}
			break;
		}

		buffer = qtv->buffer;

		switch (qtv->buffer[1]&dem_mask)
		{
		case dem_set:
			length = 10;
			if (qtv->buffersize < length)
			{	//not enough stuff to play.
				qtv->parsetime = qtv->curtime + qtv->cluster->tooslowdelay;
//				if (qtv->sourcefile || qtv->sourcesock != INVALID_SOCKET)
//					QTV_Printf(qtv, "Stream %i: Not enough buffered\n", qtv->streamid);
				continue;
			}
			qtv->parsetime += buffer[0];	//well this was pointless

			if (qtv->forwardpoint < length)	//we're about to destroy this data, so it had better be forwarded by now!
			{
				SV_ForwardStream(qtv, qtv->buffer, length);
				qtv->forwardpoint += length;
			}

			memmove(qtv->buffer, qtv->buffer+10, qtv->buffersize-(length));
			qtv->buffersize -= length;
			qtv->forwardpoint -= length;
			continue;
		case dem_multiple:
			lengthofs = 6;
			break;
		default:
			lengthofs = 2;
			break;
		}

		if (qtv->buffersize < lengthofs+4)
		{	//the size parameter doesn't fit.
//			if (qtv->sourcefile || qtv->sourcesock != INVALID_SOCKET)
//				QTV_Printf(qtv, "Stream %i: Not enough buffered\n", qtv->streamid);
			qtv->parsetime = qtv->curtime + qtv->cluster->tooslowdelay;
			break;
		}


		length = (buffer[lengthofs]<<0) + (buffer[lengthofs+1]<<8) + (buffer[lengthofs+2]<<16) + (buffer[lengthofs+3]<<24);
		if (length > MAX_MSGLEN)
		{	//THIS SHOULDN'T HAPPEN!
			//Blame the upstream proxy!
			Sys_Printf(qtv->cluster, "Stream %i: Warning: corrupt input packet (%i bytes) too big! Flushing and reconnecting!\n", qtv->streamid, length);
			if (qtv->sourcefile)
			{
				fclose(qtv->sourcefile);
				qtv->sourcefile = NULL;
			}
			else
			{
				closesocket(qtv->sourcesock);
				qtv->sourcesock = INVALID_SOCKET;
			}
			qtv->buffersize = 0;
			qtv->forwardpoint = 0;
			break;
		}

		if (length+lengthofs+4 > qtv->buffersize)
		{
//			if (qtv->sourcefile || qtv->sourcesock != INVALID_SOCKET)
//				QTV_Printf(qtv, "Stream %i: Not enough buffered\n", qtv->streamid);
			qtv->parsetime = qtv->curtime + qtv->cluster->tooslowdelay;	//add two seconds
			break;	//can't parse it yet.
		}

//		if (qtv->sourcesock != INVALID_SOCKET)
//		{
//			QTV_Printf(qtv, "Forcing demo speed to play at 100% speed\n");
//			qtv->parsespeed = 1000;	//no speeding up/slowing down routed demos
//		}

		packettime = buffer[0];
		if (qtv->parsespeed>0)
			packettime = ((1000*packettime) / qtv->parsespeed);
		qtv->nextpackettime = qtv->parsetime + packettime;

		if (qtv->nextpackettime < qtv->curtime)
		{
			switch(qtv->buffer[1]&dem_mask)
			{
			case dem_multiple:
				if ((qtv->pexte&PEXTE_HIDDENMESSAGES) &&
					0 == (buffer[lengthofs-4]<<0) + (buffer[lengthofs-3]<<8) + (buffer[lengthofs-2]<<16) + (buffer[lengthofs-1]<<24))
					;	//fucked hidden message crap. don't trip up on it.
				else
					ParseMessage(qtv, buffer+lengthofs+4, length, qtv->buffer[1]&dem_mask, (buffer[lengthofs-4]<<0) + (buffer[lengthofs-3]<<8) + (buffer[lengthofs-2]<<16) + (buffer[lengthofs-1]<<24));
				break;
			case dem_single:
			case dem_stats:
				ParseMessage(qtv, buffer+lengthofs+4, length, qtv->buffer[1]&dem_mask, 1<<(qtv->buffer[1]>>3));
				break;
			case dem_all:
				if (qtv->buffer[1] & ~dem_mask)	//dem_qtvdata
					if (qtv->sourcetype != SRC_DEMO)
						break;
				//fallthrough
			case dem_read:
				ParseMessage(qtv, buffer+lengthofs+4, length, qtv->buffer[1], 0xffffffff);
				break;
			default:
				Sys_Printf(qtv->cluster, "Message type %i\n", qtv->buffer[1]&dem_mask);
				break;
			}

			length = lengthofs+4+length;	//make length be the length of the entire packet

			qtv->oldpackettime = qtv->curtime;

			if (qtv->buffersize)
			{	//svc_disconnect can flush our input buffer (to prevent the EndOfDemo part from interfering)

				if (qtv->forwardpoint < length)	//we're about to destroy this data, so it had better be forwarded by now!
				{
					SV_ForwardStream(qtv, qtv->buffer, length);
					qtv->forwardpoint += length;
				}

				memmove(qtv->buffer, qtv->buffer+length, qtv->buffersize-(length));
				qtv->buffersize -= length;
				qtv->forwardpoint -= length;
			}

			if (qtv->sourcefile)
			{
				Net_ReadStream(qtv);
				qtv->nextconnectattempt = qtv->curtime + RECONNECT_TIME_DEMO;
			}
			else
				qtv->nextconnectattempt = qtv->curtime + RECONNECT_TIME;

			qtv->parsetime += packettime;
		}
		else
			break;
	}
}

sv_t *QTV_NewServerConnection(cluster_t *cluster, int newstreamid, char *server, char *password, qboolean force, enum autodisconnect_e autoclose, qboolean noduplicates, qboolean query)
{
	sv_t *qtv;

	if (noduplicates)
	{
		for (qtv = cluster->servers; qtv; qtv = qtv->next)
		{
			if (!strcmp(qtv->server, server))
			{	//if the stream detected some permanent/config error, try reconnecting again (of course this only happens when someone tries using the stream)
//warning review this logic
				if (qtv->errored == ERR_DISABLED)
				{
					if (!(!QTV_ConnectStream(qtv, server) && !force))	//try and wake it up
						qtv->errored = ERR_NONE;
				}
				return qtv;
			}
		}
	}
	if (!newstreamid)	//no fixed id? generate a default id
		newstreamid = 100;
	//make sure it doesn't conflict
	for(;;newstreamid++)
	{
		for (qtv = cluster->servers; qtv; qtv = qtv->next)
		{
			if (qtv->streamid == newstreamid)
				break;
		}
		if (!qtv)
			break;
	}

	if (autoclose)
		if (cluster->nouserconnects)
			return NULL;

	qtv = malloc(sizeof(sv_t));
	if (!qtv)
		return NULL;

	memset(qtv, 0, sizeof(*qtv));
	//set up a default config
//	qtv->tcplistenportnum = PROX_DEFAULTLISTENPORT;
	strcpy(qtv->server, PROX_DEFAULTSERVER);

	memcpy(qtv->connectpassword, password, sizeof(qtv->connectpassword)-1);

//	qtv->tcpsocket = INVALID_SOCKET;
	qtv->sourcesock = INVALID_SOCKET;
	qtv->autodisconnect = autoclose;
	qtv->parsingconnectiondata = true;
	qtv->serverquery = query;
	qtv->silentstream = true;
	qtv->parsespeed = 1000;

	qtv->streamid = newstreamid;

	qtv->cluster = cluster;
	qtv->next = cluster->servers;

	if (autoclose != AD_REVERSECONNECT)	//2 means reverse connection (don't ever try reconnecting)
	{
		if (!QTV_ConnectStream(qtv, server) && !force)
		{
			QTV_Cleanup(qtv, false);
			free(qtv);
			return NULL;
		}
	}
	cluster->servers = qtv;
	cluster->numservers++;

	return qtv;
}
