#include "cl_master.h"

#define NET_GAMENAME_NQ		"QUAKE"

//rename to cl_master.c sometime

//the networking operates seperatly from the main code. This is so we can have full control over all parts of the server sending prints.
//when we send status to the server, it replys with a print command. The text to print contains the serverinfo.
//Q2's print command is a compleate 'print', while qw is just a 'p', thus we can distinguish the two easily.

//save favorites and allow addition of new ones from game?
//add filters some time

//remove dead servers.
//master was polled a minute ago and server was not on list - server on multiple masters would be awkward.


extern int msecstime;

//the number of servers should be limited only by memory.

vmcvar_t slist_cacheinfo = {"slist_cacheinfo", "0"};	//this proves dangerous, memory wise.
vmcvar_t slist_writeserverstxt = {"slist_writeservers", "0"};

void CL_MasterListParse(qbyte *buffer, qbyte *maxbuffer, qboolean isq2);
void CL_QueryServers(void);
int CL_ReadServerInfo(char *msg, int servertype, qboolean favorite, struct sockaddr_in net_from);

master_t *master;
player_t *mplayers;
serverinfo_t *firstserver;


#define POLLUDPSOCKETS 64	//it's big so we can have lots of messages when behind a firewall. Basically if a firewall only allows replys, and only remembers 3 servers per socket, we need this big cos it can take a while for a packet to find a fast optimised route and we might be waiting for a few secs for a reply the first time around.
SOCKET pollsocketsUDP[POLLUDPSOCKETS];
int lastpollsockUDP;

qboolean NET_StringToAdr(const char *s, struct sockaddr_in *a)
{
	char	copy[128];
	char *colon;
	struct hostent *he;

	strcpy(copy, s);
	colon = strchr(copy, ':');
	if (colon)
	{
		a->sin_port = htons((unsigned short)atoi(colon+1));
		*colon = '\0';
	}
	else
		a->sin_port = 0;

	*(unsigned int*)&a->sin_addr = inet_addr(copy);
	a->sin_family = AF_INET;

	if (*(unsigned int*)&a->sin_addr != INADDR_NONE)
		return true;

	he = gethostbyname(copy);
	if (he)
	{
		if (he->h_addrtype != AF_INET)
			return false;	//whoops...
			
		*(int*)&a->sin_addr = *(int*)he->h_addr;

		return true;
	}

	return false;
}
qboolean NET_CompareAdr(const struct sockaddr_in *a, const struct sockaddr_in *b)
{
	if (a->sin_family != b->sin_family)
		return false;
	if (*(int*)&a->sin_addr != *(int*)&b->sin_addr)
		return false;
	if (a->sin_port != b->sin_port)
		return false;
	return true;
}
char *NET_AdrToString(const struct sockaddr_in *a)
{
	static char buffer[24];
	const qbyte *ad = (const qbyte *)&a->sin_addr;
	snprintf(buffer, sizeof(buffer), "%i.%i.%i.%i:%i", ad[0], ad[1], ad[2], ad[3], ntohs(a->sin_port));
	return buffer;
}

void Master_AddMaster (char *address, int type, char *description)
{
	struct sockaddr_in adr;
	master_t *mast;

	if (!NET_StringToAdr(address, &adr))
	{
		Con_Printf("Failed to resolve address \"%s\"\n", address);
		return;
	}

	for (mast = master; mast; mast = mast->next)
	{
		if (NET_CompareAdr(&mast->adr, &adr))	//already exists.
			return;
	}
	mast = malloc(sizeof(master_t)+strlen(description));
	mast->adr = adr;
	mast->type = type;
	strcpy(mast->name, description);

	mast->next = master;
	master = mast;
}

//build a linked list of masters.	Doesn't duplicate addresses.
qboolean Master_LoadMasterList (char *filename, int defaulttype, int depth)
{

	return false;
/*
	extern char	com_basedir[MAX_OSPATH];
	FILE *f;
	char line[1024];
	char file[1024];
	char *name, *next;
	int servertype;

	if (depth <= 0)
		return false;
	depth--;

	f = fopen(va("%s/%s", com_basedir, filename), "rb");
	if (!f)
		return false;

	while(fgets(line, sizeof(line)-1, f))
	{
		if (*line == '#')	//comment
			continue;

		next = COM_Parse(line);
		if (!*com_token)
			continue;

		if (!strcmp(com_token, "file"))	//special case. Add a port if you have a server named 'file'... (unlikly)
		{
			next = COM_Parse(next);
			if (!next)
				continue;
			Q_strncpyz(file, com_token, sizeof(file));
		}
		else
			*file = '\0';

		*next = '\0';
		next++;
		name = COM_Parse(next);
		servertype = -1;

		if (!strcmp(com_token, "single:qw"))
			servertype = MT_SINGLEQW;
		else if (!strcmp(com_token, "single:q2"))
			servertype = MT_SINGLEQ2;
		else if (!strcmp(com_token, "single:nq") || !strcmp(com_token, "single:q1"))
			servertype = MT_SINGLENQ;
		else if (!strcmp(com_token, "single"))
			servertype = MT_SINGLEQW;

		else if (!strcmp(com_token, "master:qw"))
			servertype = MT_MASTERQW;
		else if (!strcmp(com_token, "master:q2"))
			servertype = MT_MASTERQ2;
		else if (!strcmp(com_token, "master"))	//any other sort of master, assume it's a qw master.
			servertype = MT_MASTERQW;

		else if (!strcmp(com_token, "bcast:qw"))
			servertype = MT_BCASTQW;
		else if (!strcmp(com_token, "bcast:q2"))
			servertype = MT_BCASTQ2;
		else if (!strcmp(com_token, "bcast:nq"))
			servertype = MT_BCASTNQ;
		else if (!strcmp(com_token, "bcast"))
			servertype = MT_BCASTQW;

		else if (!strcmp(com_token, "favorite:qw"))
			servertype = -MT_SINGLEQW;
		else if (!strcmp(com_token, "favorite:q2"))
			servertype = -MT_SINGLEQ2;
		else if (!strcmp(com_token, "favorite:nq"))
			servertype = -MT_SINGLENQ;
		else if (!strcmp(com_token, "favorite"))
			servertype = -MT_SINGLEQW;


		else
		{
			name = next;	//go back one token.
			servertype = defaulttype;
		}

		while(*name <= ' ' && *name != 0)	//skip whitespace
			name++;

		next = name + strlen(name)-1;
		while(*next <= ' ' && next > name)
		{
			*next = '\0';
			next--;
		}


		if (*file)
			Master_LoadMasterList(file, servertype, depth);
		else if (servertype < 0)
		{
			if (NET_StringToAdr(line, &net_from))
				CL_ReadServerInfo(va("\\hostname\\%s", name), -servertype, true);
			else
				Con_Printf("Failed to resolve address - \"%s\"\n", line);
		}
		else
			Master_AddMaster(line, servertype, name);
	}
	fclose(f);

	return true;
*/
}

int UDP_OpenSocket (int port, qboolean bcast)
{
	int newsocket;
	struct sockaddr_in address;
	unsigned long _true = true;
	int i;
int maxport = port + 100;

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		Sys_Errorf ("UDP_OpenSocket: socket: %s", strerror(qerrno));

	if (ioctlsocket (newsocket, FIONBIO, &_true) == -1)
		Sys_Errorf ("UDP_OpenSocket: ioctl FIONBIO: %s", strerror(qerrno));

	if (bcast)
	{
		_true = true;
		if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&_true, sizeof(_true)) == -1)
		{
			Con_Printf("Cannot create broadcast socket\n");
			return 0;
		}
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;

	for(;;)
	{
		if (port == PORT_ANY)
			address.sin_port = 0;
		else
			address.sin_port = htons((short)port);
	
		if( bind (newsocket, (void *)&address, sizeof(address)) == -1)
		{
			if (!port)
				Sys_Errorf ("UDP_OpenSocket: bind: %s", strerror(qerrno));
			port++;
			if (port > maxport)
				Sys_Errorf ("UDP_OpenSocket: bind: %s", strerror(qerrno));
		}
		else
			break;
	}

	return newsocket;
}

void NET_SendPollPacket(int len, void *data, struct sockaddr_in addr)
{
	int ret;

#ifdef USEIPX
	if (addr.sa_family == AF_IPX)
	{
		lastpollsockIPX++;
		if (lastpollsockIPX>=POLLIPXSOCKETS)
			lastpollsockIPX=0;
		if (!pollsocketsIPX[lastpollsockIPX])
			pollsocketsIPX[lastpollsockIPX] = IPX_OpenSocket(PORT_ANY, true);
		ret = sendto (pollsocketsIPX[lastpollsockIPX], data, len, 0, (struct sockaddr *)&addr, sizeof(addr) );
	}
	else
#endif
	{
		lastpollsockUDP++;
		if (lastpollsockUDP>=POLLUDPSOCKETS)
			lastpollsockUDP=0;
		if (!pollsocketsUDP[lastpollsockUDP])
			pollsocketsUDP[lastpollsockUDP] = UDP_OpenSocket(PORT_ANY, true);
		ret = sendto (pollsocketsUDP[lastpollsockUDP], data, len, 0, (struct sockaddr *)&addr, sizeof(addr) );
	}

	if (ret == -1)
	{
// wouldblock is silent
		if (qerrno == EWOULDBLOCK)
			return;

		if (qerrno == ECONNREFUSED)
			return;

		if (qerrno != EADDRNOTAVAIL)
			Con_Printf ("NET_SendPacket ERROR: %i\n", qerrno);
//		else
//			Con_DPrintf("NET_SendPacket Warning: %i\n", qerrno);
	}
}

int NET_CheckPollSockets(void)
{
#define	MAX_UDP_PACKET	8192	//hmm... while on windows, the max size is 1400 or so...
	qbyte		net_message_buffer[MAX_UDP_PACKET];
	qbyte		*readpoint;
	qbyte		*maxread;
	int sock;
	SOCKET usesocket;
	for (sock = 0; sock < POLLUDPSOCKETS; sock++)
	{
		int 	ret;
		struct sockaddr_in	from;
		int		fromlen;

		usesocket = pollsocketsUDP[sock];

		if (!usesocket)
			continue;
		fromlen = sizeof(from);
		ret = recvfrom (usesocket, (char *)net_message_buffer, sizeof(net_message_buffer), 0, (struct sockaddr *)&from, &fromlen);

		if (ret == -1)
		{
			if (qerrno == EWOULDBLOCK)
				continue;
			if (qerrno == EMSGSIZE)
			{
				Con_Printf ("Warning:  Oversize packet from %s\n",
					NET_AdrToString (&from));
				continue;
			}
			if (qerrno == ECONNABORTED || qerrno == ECONNRESET)
			{
//				Con_Printf ("Connection lost or aborted\n");
				continue;
			}


			Con_Printf ("NET_CheckPollSockets: %s", strerror(qerrno));
			continue;
		}

		if (ret >= sizeof(net_message_buffer) )
		{
			Con_Printf ("Oversize packet from %s\n", NET_AdrToString (&from));
			continue;
		}
		readpoint = net_message_buffer;
		maxread = readpoint+ret;

		if (*(int*)net_message_buffer == 0xffffffff)
		{
			int c;
#ifdef Q2CLIENT
			char *s;
#endif
			readpoint += 4;

#ifdef Q2CLIENT
			s = strchr('\n');
			if (!strncmp(readpoint, "print", 5))
			{
				CL_ReadServerInfo(s+1, MT_SINGLEQ2, false);
				continue;
			}
			else if (!strncmp(readpoint, "info", 4))	//parse a bit more...
			{
				CL_ReadServerInfo(s+1, MT_SINGLEQ2, false);
				continue;
			}
			else if (!strncmp(s, "servers", 6))	//parse a bit more...
			{
				readpoint = orp+7;
				CL_MasterListParse(readpoint, maxread, true);
				continue;
			}
			readpoint = orp;
#endif

			c = *readpoint++;

			if (c == A2C_PRINT)	//qw server reply.
			{
				CL_ReadServerInfo(readpoint, MT_SINGLEQW, false, from);
				continue;
			}

			if (c == M2C_MASTER_REPLY)	//qw master reply.
			{		
				CL_MasterListParse(readpoint, maxread, false);
				continue;
			}
		}
#ifdef NQPROT
		else
		{	//connected packet? Must be a NQ packet.
			char name[32];
			char map[16];
			int users, maxusers;

			int control;

			MSG_BeginReading ();
			control = BigLong(*((int *)net_message.data));
			MSG_ReadLong();
			if (control == -1)
				continue;
			if ((control & (~NETFLAG_LENGTH_MASK)) !=  NETFLAG_CTL)
				continue;
			if ((control & NETFLAG_LENGTH_MASK) != ret)
				continue;

			if (MSG_ReadByte() != CCREP_SERVER_INFO)
				continue;

			NET_StringToAdr(MSG_ReadString(), &net_from);

			Q_strncpyz(name, MSG_ReadString(), sizeof(name));
			Q_strncpyz(map, MSG_ReadString(), sizeof(map));
			users = MSG_ReadByte();
			maxusers = MSG_ReadByte();
			if (MSG_ReadByte() != NET_PROTOCOL_VERSION)
			{
//				Q_strcpy(name, "*");
//				Q_strcat(name, name);
			}

			CL_ReadServerInfo(va("\\hostname\\%s\\map\\%s\\maxclients\\%i", name, map, maxusers), MT_SINGLENQ, false);
		}
#endif
		continue;		
	}
	return 0;
}

void Master_RemoveKeepInfo(serverinfo_t *sv)
{
	sv->special &= ~SS_KEEPINFO;
	if (sv->moreinfo)
	{
		free(sv->moreinfo);
		sv->moreinfo = NULL;
	}
}

void SListOptionChanged(serverinfo_t *newserver)
{
	if (selectedserver.inuse)
	{
		char data[16];
		serverinfo_t *oldserver;

		selectedserver.detail = NULL;

		if (!slist_cacheinfo.value)	//we have to flush it. That's the rules.
		{
			for (oldserver = firstserver; oldserver; oldserver=oldserver->next)
			{
				if (NET_CompareAdr(&selectedserver.adr, &oldserver->adr))//*(int*)selectedserver.ipaddress == *(int*)server->ipaddress && selectedserver.port == server->port)
				{
					if (oldserver->moreinfo)
					{
						free(oldserver->moreinfo);
						oldserver->moreinfo = NULL;
					}
					break;
				}
			}
		}

		if (!newserver)
			return;

		selectedserver.adr = newserver->adr;

		if (newserver->moreinfo)	//we cached it.
		{
			selectedserver.detail = newserver->moreinfo;
			return;
		}
//we don't know all the info, so send a request for it.
		selectedserver.detail = newserver->moreinfo = malloc(sizeof(serverdetailedinfo_t));
		
		newserver->moreinfo->numplayers = newserver->players;
		snprintf(newserver->moreinfo->info, sizeof(newserver->moreinfo->info), "\\%s\\%s", "hostname", newserver->name);

		newserver->refreshtime = msecstime;
		snprintf(data, sizeof(data), "%c%c%c%cstatus\n", 255, 255, 255, 255);
		NET_SendPollPacket (strlen(data), data, newserver->adr);
	}
}


//don't try sending to servers we don't support
void MasterInfo_Request(master_t *mast)
{
	if (!mast)
		return;
	switch(mast->type)
	{
#ifdef Q2CLIENT
	case MT_BCASTQ2:
	case MT_SINGLEQ2:
#endif
	case MT_SINGLEQW:
	case MT_BCASTQW:
		NET_SendPollPacket (11, va("%c%c%c%cstatus\n", 255, 255, 255, 255), mast->adr);
		break;
#ifdef NQPROT
	case MT_BCASTNQ:
	case MT_SINGLENQ:
		SZ_Clear(&net_message);
		MSG_WriteLong(&net_message, 0);// save space for the header, filled in later
		MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
		MSG_WriteString(&net_message, NET_GAMENAME_NQ);	//look for either sort of server
		MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
		*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		NET_SendPollPacket(net_message.cursize, net_message.data, mast->adr);
		SZ_Clear(&net_message);
		break;
#endif
	case MT_MASTERQW:
		NET_SendPollPacket (3, "c\n", mast->adr);
		break;
#ifdef Q2CLIENT
	case MT_MASTERQ2:
		if (COM_FDepthFile("pics/colormap.pcx", true)!=0x7fffffff)	//only query this master if we expect to be able to load it's maps.
			NET_SendPollPacket (6, "query", mast->adr);
		break;
#endif
	}
}


void MasterInfo_WriteServers(void)
{
/*
	char *typename;
	master_t *mast;
	serverinfo_t *server;
	FILE *mf, *qws;
	
	mf = fopen("masters.txt", "wt");
	if (!mf)
	{
		Con_Printf("Couldn't write masters.txt");
		return;
	}
	
	for (mast = master; mast; mast=mast->next)
	{
		switch(mast->type)
		{
		case MT_MASTERQW:
			typename = "master:qw";
			break;
		case MT_MASTERQ2:
			typename = "master:q2";
			break;
		case MT_BCASTQW:
			typename = "bcast:qw";
			break;
		case MT_BCASTQ2:
			typename = "bcast:q2";
			break;
		case MT_BCASTNQ:
			typename = "bcast:nq";
			break;
		case MT_SINGLEQW:
			typename = "single:qw";
			break;
		case MT_SINGLEQ2:
			typename = "single:q2";
			break;
		case MT_SINGLENQ:
			typename = "single:nq";
			break;
		default:
			typename = "writeerror";
		}
		fprintf(mf, "%s\t%s\t%s\n", NET_AdrToString(mast->adr), typename, mast->name);
	}
	
	if (slist_writeserverstxt.value)
		qws = fopen("server.txt", "wt");
	else
		qws = NULL;
	if (qws)
		fprintf(mf, "\n%s\t%s\t%s\n\n", "file servers.txt", "favorite:qw", "personal server list");
		
	for (server = firstserver; server; server = server->next)
	{
		if (server->special & SS_FAVORITE)
		{
			if (server->special & SS_QUAKE2)
				fprintf(mf, "%s\t%s\t%s\n", NET_AdrToString(server->adr), "favorite:q2", server->name);
			else if (server->special & SS_NETQUAKE)
				fprintf(mf, "%s\t%s\t%s\n", NET_AdrToString(server->adr), "favorite:nq", server->name);
			else if (qws)	//servers.txt doesn't support the extra info.
				fprintf(qws, "%s\t%s\n", NET_AdrToString(server->adr), server->name);
			else	//read only? damn them!
				fprintf(mf, "%s\t%s\t%s\n", NET_AdrToString(server->adr), "favorite:qw", server->name);
		}
	}
	
	if (qws)
		fclose(qws);
	
	
	fclose(mf);
*/
}

//poll master servers for server lists.
void MasterInfo_Begin(void)
{
	master_t *mast;
	if (!Master_LoadMasterList("masters.txt",			MT_MASTERQW, 5))
	{
		Master_LoadMasterList("servers.txt",			MT_SINGLEQW, 1);
//		if (q1servers)
		{
			Master_AddMaster("255.255.255.255:26000",		MT_BCASTNQ, "Nearby Quake1 servers");
			Master_AddMaster("255.255.255.255:27500",		MT_BCASTQW, "Nearby QuakeWorld UDP servers.");
		}

//		if (q2servers)
		{
			Master_AddMaster("255.255.255.255:27910",		MT_BCASTQ2, "Nearby Quake2 UDP servers.");
//			Master_AddMaster("00000000:ffffffffffff:27910",	MT_BCASTQ2, "Nearby Quake2 IPX servers.");
		}


//		if (q1servers)
		{
			Master_AddMaster("192.246.40.37:27000",			MT_MASTERQW, "id Limbo");
			Master_AddMaster("192.246.40.37:27002",			MT_MASTERQW, "id CTF");
			Master_AddMaster("192.246.40.37:27003",			MT_MASTERQW, "id TeamFortress");
			Master_AddMaster("192.246.40.37:27004",			MT_MASTERQW, "id Miscilaneous");
			Master_AddMaster("192.246.40.37:27006",			MT_MASTERQW, "id Deathmatch Only");
			Master_AddMaster("150.254.66.120:27000",		MT_MASTERQW, "Poland's master server.");
			Master_AddMaster("62.112.145.129:27000",		MT_MASTERQW, "Ocrana master server.");
		}

//		if (q2servers)
		{
			Master_AddMaster("192.246.40.37:27900",			MT_MASTERQ2, "id q2 Master.");
		}

	}

	for (mast = master; mast; mast=mast->next)
	{
		MasterInfo_Request(mast);
	}
}

void Master_QueryServer(serverinfo_t *server)
{
	char	data[2048];
	server->sends++;
	server->refreshtime = msecstime;
	snprintf(data, sizeof(data), "%c%c%c%cstatus", 255, 255, 255, 255);
	NET_SendPollPacket (strlen(data), data, server->adr);
}
//send a packet to each server in sequence.
void CL_QueryServers(void)
{
	static int poll;
	int op;
	serverinfo_t *server;	
	op = poll;
	

	for (server = firstserver; op>0 && server; server=server->next, op--);

	if (!server)
	{
		poll = 0;
		return;
	}

	if (op == 0)
	{
		if (server->sends < 1)
		{
			Master_QueryServer(server);
		}
		poll++;
		return;
	}
	

	poll = 0;
}

int Master_TotalCount(void)
{
	int count=0;
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		count++;
	}
	return count;
}

//true if server is on a different master's list.
serverinfo_t *Master_InfoForServer (struct sockaddr_in addr)
{
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		if (NET_CompareAdr(&info->adr, &addr))
			return info;
	}
	return NULL;
}
serverinfo_t *Master_InfoForNum (int num)
{
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		if (num-- <=0)
			return info;
	}
	return NULL;
}

void MasterInfo_RemovePlayers(struct sockaddr_in adr)
{
	player_t *p, *prev;
	prev = NULL;
	for (p = mplayers; p; )
	{
		if (NET_CompareAdr(&p->adr, &adr))
		{
			if (prev)
				prev->next = p->next;
			else
				mplayers = p->next;
			free(p);
			p=prev;

			continue;
		}
		else
			prev = p;

		p = p->next;
	}
}

void MasterInfo_AddPlayer(struct sockaddr_in serveradr, char *name, int ping, int frags, int colours, char *skin)
{
	player_t *p;
	p = malloc(sizeof(player_t));
	p->next = mplayers;
	p->adr = serveradr;
	p->colour = colours;
	p->frags = frags;
	strncpy(p->name, name, sizeof(p->name));
	p->name[sizeof(p->name)-1] = '\0';
	strncpy(p->skin, skin, sizeof(p->skin));
	p->skin[sizeof(p->skin)-1] = '\0';
	mplayers = p;
}

//we got told about a server, parse it's info
int CL_ReadServerInfo(char *msg, int servertype, qboolean favorite, struct sockaddr_in net_from)
{
	serverdetailedinfo_t details;

	char *token;
	char *nl;
	char *name;	
	int ping;
	int len;
	serverinfo_t *info;

	info = Master_InfoForServer(net_from);

	if (!info)	//not found...
	{
		info = malloc(sizeof(serverinfo_t));
		memset(info, 0, sizeof(*info));

		info->adr = net_from;

		snprintf(info->name, sizeof(info->name), "%s", NET_AdrToString(&info->adr));

		info->next = firstserver;
		firstserver = info;

	}
	else
	{
		MasterInfo_RemovePlayers(info->adr);
	}

	nl = strchr(msg, '\n');
	if (nl)
	{
		*nl = '\0';
		nl++;
	}
	name = Info_ValueForKey(msg, "hostname");
	Q_strncpyz(info->name, name, sizeof(info->name));
	info->special = info->special & (SS_FAVORITE | SS_KEEPINFO);	//favorite is never cleared
	if (!strcmp("FTE", Info_ValueForKey(msg, "*distrib")))
		info->special |= SS_FTESERVER;
	else if (!strncmp("FTE", Info_ValueForKey(msg, "*version"), 3))
		info->special |= SS_FTESERVER;
		
		
	if (servertype == MT_SINGLEQ2)
		info->special |= SS_QUAKE2;
	else if (servertype == MT_SINGLENQ)
		info->special |= SS_NETQUAKE;
	if (favorite)	//was specifically named, not retrieved from a master.
		info->special |= SS_FAVORITE;

	ping = msecstime - info->refreshtime;
	if (ping > 0xffff)
		info->ping = 0xffff;
	else
		info->ping = ping;

	info->players = 0;
	info->maxplayers = atoi(Info_ValueForKey(msg, "maxclients"));

	info->tl = atoi(Info_ValueForKey(msg, "timelimit"));
	info->fl = atoi(Info_ValueForKey(msg, "fraglimit"));

	if (servertype == MT_SINGLEQ2)
	{
		Q_strncpyz(info->gamedir,	Info_ValueForKey(msg, "gamename"),	sizeof(info->gamedir));
		Q_strncpyz(info->map,		Info_ValueForKey(msg, "mapname"),	sizeof(info->map));
	}
	else
	{
		Q_strncpyz(info->gamedir,	Info_ValueForKey(msg, "*gamedir"),	sizeof(info->gamedir));
		Q_strncpyz(info->map,		Info_ValueForKey(msg, "map"),		sizeof(info->map));
	}

	{
		int clnum;
		strcpy(details.info, msg);
		msg = msg+strlen(msg)+1;

		info->players=details.numplayers = 0;
		for (clnum=0; clnum < MAX_CLIENTS; clnum++)
		{
			nl = strchr(msg, '\n');
			if (!nl)
				break;
			*nl = '\0';

			token = msg;
			if (!token)
				break;
			details.players[clnum].userid = atoi(token);
			token = strchr(token+1, ' ');
			if (!token)
				break;
			details.players[clnum].frags = atoi(token);
			token = strchr(token+1, ' ');
			if (!token)
				break;
			details.players[clnum].time = (float)atof(token);
			msg = token;
			token = strchr(msg+1, ' ');
			if (!token)	//probably q2 response
			{
				//see if this is actually a Quake2 server.
				token = strchr(msg+1, '\"');
				if (!token)	//it wasn't.
					break;

				details.players[clnum].ping = details.players[clnum].frags;
				details.players[clnum].frags = details.players[clnum].userid;

				msg = strchr(token+1, '\"');
				if (!msg)
					break;
				len = msg - token;
				if (len >= sizeof(details.players[clnum].name))
					len = sizeof(details.players[clnum].name);
				Q_strncpyz(details.players[clnum].name, token+1, len);

				details.players[clnum].skin[0] = '\0';

				details.players[clnum].topc = 0;
				details.players[clnum].botc = 0;
				details.players[clnum].time = 0;
			}
			else	//qw responce
			{
				details.players[clnum].time = (float)atof(token);
				msg = token;
				token = strchr(msg+1, ' ');
				if (!token)
					break;

				details.players[clnum].ping = atoi(token);

				token = strchr(token+1, '\"');
				if (!token)
					break;
				msg = strchr(token+1, '\"');
				if (!msg)
					break;
				len = msg - token;
				if (len >= sizeof(details.players[clnum].name))
					len = sizeof(details.players[clnum].name);
				Q_strncpyz(details.players[clnum].name, token+1, len);
				details.players[clnum].name[len] = '\0';

				token = strchr(msg+1, '\"');
				if (!token)
					break;
				msg = strchr(token+1, '\"');
				if (!msg)
					break;
				len = msg - token;
				if (len >= sizeof(details.players[clnum].skin))
					len = sizeof(details.players[clnum].skin);
				Q_strncpyz(details.players[clnum].skin, token+1, len);
				details.players[clnum].skin[len] = '\0';

				token = strchr(msg+1, ' ');
				if (!token)
					break;
				details.players[clnum].topc = atoi(token);
				token = strchr(token+1, ' ');
				if (!token)
					break;
				details.players[clnum].botc = atoi(token);
			}

			MasterInfo_AddPlayer(info->adr, details.players[clnum].name, details.players[clnum].ping, details.players[clnum].frags, details.players[clnum].topc*4 | details.players[clnum].botc, details.players[clnum].skin);

			info->players = ++details.numplayers;

			msg = nl;
			if (!msg)
				break;	//erm...
			msg++;
		}
	}
	if (!info->moreinfo && ((slist_cacheinfo.value == 2 || NET_CompareAdr(&info->adr, &selectedserver.adr)) || (info->special & SS_KEEPINFO)))
		info->moreinfo = malloc(sizeof(serverdetailedinfo_t));
	if (NET_CompareAdr(&info->adr, &selectedserver.adr))
		selectedserver.detail = info->moreinfo;

	if (info->moreinfo)
		memcpy(info->moreinfo, &details, sizeof(serverdetailedinfo_t));

	return true;
}

//rewrite to scan for existing server instead of wiping all.
void CL_MasterListParse(qbyte *buffer, qbyte *maxbuffer, qboolean isq2)
{
	serverinfo_t *info;
	serverinfo_t *last, *old;

	int p1, p2;
	buffer++;

	last = firstserver;

	while(buffer+1 <= maxbuffer)
	{
		info = malloc(sizeof(serverinfo_t));
		memset(info, 0, sizeof(*info));
		info->adr.sin_family = AF_INET;
		((qbyte *)&info->adr.sin_addr)[0] = *buffer++;
		((qbyte *)&info->adr.sin_addr)[1] = *buffer++;
		((qbyte *)&info->adr.sin_addr)[2] = *buffer++;
		((qbyte *)&info->adr.sin_addr)[3] = *buffer++;

		p1 = *buffer++;
		p2 = *buffer++;
		info->adr.sin_port = (short)(p1 + (p2<<8));
		if ((old = Master_InfoForServer(info->adr)))	//remove if the server already exists.
		{
			old->sends = 0;	//reset.
			free(info);
		}
		else
		{
			info->special = isq2?SS_QUAKE2:0;
			info->refreshtime = 0;

			snprintf(info->name, sizeof(info->name), "%s", NET_AdrToString(&info->adr));

			info->next = last;
			last = info;
		}		
	}

	firstserver = last;
}
