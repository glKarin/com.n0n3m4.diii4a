/*
This file is intended as a set of exports for an NQ-based engine.
This is supported _purely_ for clients, and will not work for servers.

documentation can be found inside net_qtv.h
*/



#include "qtv.h"
int build_number(void);

static cluster_t *cluster;

//note that a qsockaddr is only 16 bytes.
//this is not enough for ipv6 etc.
struct qsockaddr
{
	int ipid;
};
static char resolvedadrstring[128];
static int lastadrid;

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#endif

#include "net_qtv.h"


PUBEXPORT void PUBLIC QTV_Command (char *command, char *results, int resultlen)
{
	if (!cluster)
		return;
	Rcon_Command(cluster, NULL, command, results, resultlen, true);
}
PUBEXPORT void PUBLIC QTV_RunFrame (void)
{
	if (!cluster)
		return;
	Cluster_Run(cluster, false);
}


EXPORT int PUBLIC QTV_Init (void)
{
	if (cluster)
		return 0;

	cluster = malloc(sizeof(*cluster));
	if (cluster)
	{
		memset(cluster, 0, sizeof(*cluster));

		cluster->qwdsocket[0] = INVALID_SOCKET;
		cluster->qwdsocket[1] = INVALID_SOCKET;
		cluster->tcpsocket[0] = INVALID_SOCKET;
		cluster->tcpsocket[1] = INVALID_SOCKET;
		cluster->anticheattime = 1*1000;
		cluster->tooslowdelay = 100;
		cluster->qwlistenportnum = 0;
		cluster->allownqclients = true;
		strcpy(cluster->hostname, DEFAULT_HOSTNAME);
		cluster->buildnumber = build_number();
		cluster->maxproxies = -1;

		strcpy(cluster->demodir, "qw/demos/");
		return 0;
	}

	return -1;
}
EXPORT void PUBLIC QTV_Shutdown (void)
{
}
EXPORT void PUBLIC QTV_Listen (qboolean state)
{
}
EXPORT int PUBLIC QTV_OpenSocket (int port)
{
	return 0;
}
EXPORT int PUBLIC QTV_CloseSocket (int socket)
{
	//give it a chance to close any server connections from us disconnecting (should have already send disconnect message, but won't have run the server so not noticed the lack of viewers)
	Cluster_Run(cluster, false);
	return 0;
}
EXPORT int PUBLIC QTV_Connect (int socket, struct qsockaddr *addr)
{
	if (addr->ipid == lastadrid)
	{
		strlcpy(cluster->autojoinadr, resolvedadrstring, sizeof(cluster->autojoinadr));
		return 0;
	}
	else
	{
		cluster->autojoinadr[0] = 0;
		return -1;
	}
	return 0;
}
EXPORT int PUBLIC QTV_CheckNewConnections (void)
{
	return -1;
}

static byte pendingbuf[8][1032];
static int pendinglen[8];
static unsigned int pendingin, pendingout;
void QTV_DoReceive(void *data, int length)
{
	int idx;
	if (length > sizeof(pendingbuf[0]))
		return;
	idx = pendingout++;
	idx &= 7;
	memcpy(pendingbuf[idx], data, length);
	pendinglen[idx] = length;
}
EXPORT int PUBLIC QTV_Read (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	if (pendingout == pendingin)
	{
		Cluster_Run(cluster, false);
		Cluster_Run(cluster, false);
	}

	while (pendingin != pendingout)
	{
		int idx = pendingin++;
		idx &= 7;
		if (pendinglen[idx] > len)
			continue;	//error
		memcpy(buf, pendingbuf[idx], pendinglen[idx]);
		return pendinglen[idx];
	}
	return 0;
}
EXPORT int PUBLIC QTV_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	netmsg_t m;
	netadr_t from;
	from.tcpcon = NULL;
	((struct sockaddr*)from.sockaddr)->sa_family = AF_UNSPEC;

	m.cursize = len;
	m.data = buf;
	m.readpos = 0;

	QW_ProcessUDPPacket(cluster, &m, from);

	if (pendingout == pendingin)
		Cluster_Run(cluster, false);

	return 0;
}
EXPORT int PUBLIC QTV_Broadcast (int socket, byte *buf, int len)
{
	netmsg_t m;
	netadr_t from;
	from.tcpcon = NULL;
	((struct sockaddr*)from.sockaddr)->sa_family = AF_UNSPEC;

	m.cursize = len;
	m.data = buf;
	m.readpos = 0;

	QW_ProcessUDPPacket(cluster, &m, from);

	return 0;
}
EXPORT char *PUBLIC QTV_AddrToString (struct qsockaddr *addr)
{
	return 0;
}
EXPORT int PUBLIC QTV_StringPortToAddr (char *string, int port, struct qsockaddr *addr)
{
	if (!strncmp(string, "file:", 5) || !strncmp(string, "demo:", 5))
	{
		snprintf(resolvedadrstring, sizeof(resolvedadrstring), "%s", string);
		addr->ipid = ++lastadrid;
		return 0;
	}
	if (!strncmp(string, "qw:", 3) || !strncmp(string, "udp:", 4) || !strncmp(string, "tcp:", 4))
	{
		if (port)
			snprintf(resolvedadrstring, sizeof(resolvedadrstring), "%s:%i", string, port);
		else
			snprintf(resolvedadrstring, sizeof(resolvedadrstring), "%s", string);
		addr->ipid = ++lastadrid;
		return 0;
	}
	return -1;
}
EXPORT int PUBLIC QTV_StringToAddr (char *string, struct qsockaddr *addr)
{
	return QTV_StringPortToAddr(string, 0, addr);
}
EXPORT int PUBLIC QTV_GetSocketAddr (int socket, struct qsockaddr *addr)
{
	return 0;
}
EXPORT int PUBLIC QTV_GetNameFromAddr (struct qsockaddr *addr, char *name)
{
	return 0;
}
EXPORT int PUBLIC QTV_GetAddrFromName (char *name, struct qsockaddr *addr)
{
	return QTV_StringToAddr(name, addr);
}
EXPORT int PUBLIC QTV_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2)
{
	return 0;
}
EXPORT int PUBLIC QTV_GetSocketPort (struct qsockaddr *addr)
{
	return 0;
}
EXPORT int PUBLIC QTV_SetSocketPort (struct qsockaddr *addr, int port)
{
	return 0;
}
