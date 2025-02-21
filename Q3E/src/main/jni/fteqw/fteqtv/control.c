/*
Contains the control routines that handle both incoming and outgoing stuff
*/

#include "qtv.h"
#include <signal.h>
#include "bsd_string.h"

#ifndef _WIN32
#include <sys/stat.h>
#include <dirent.h>
#else
#include <direct.h>
#endif

typedef struct {
	char name[56];
	int offset;
	int length;
} pakfile;
// PACK, offset, lengthofpakfiles
FILE *FindInPaks(char *gamedir, char *filename, int *size)
{
	FILE *f;
	char fname[1024];
	int i, j;
	int numfiles;
	unsigned int header[3];

	pakfile pf;

	for (i = 0; ; i++)
	{
		sprintf(fname, "%s/pak%i.pak", gamedir, i);
		f = fopen(fname, "rb");
		if (!f)
			return NULL;	//ran out of possible pak files.

		fread(header, 1, sizeof(header), f);
		if (header[0] != *(unsigned int*)"PACK")
		{	//err... hmm.
			fclose(f);
			continue;
		}
		numfiles = LittleLong(header[2])/sizeof(pakfile);
		fseek(f, LittleLong(header[1]), SEEK_SET);
		for (j = 0; j < numfiles; j++)
		{
			fread(&pf, 1, sizeof(pf), f);
			if (!strcmp(pf.name, filename))
			{
				fseek(f, LittleLong(pf.offset), 0);
				if (size)
					*size = LittleLong(pf.length);
				return f;
			}
		}
		fclose(f);
		//not found
	}
	return NULL;
}

unsigned char *FS_ReadFile2(char *gamedir, char *filename, unsigned int *sizep)
{
	int size;
	unsigned char *data;

	FILE *f;
	char fname[1024];

	if (!*filename)
		return NULL;

	//try and read it straight out of the file system
	sprintf(fname, "%s/%s", gamedir, filename);
	f = fopen(fname, "rb");
	if (!f)
		f = fopen(filename, "rb");	//see if we're being run from inside the gamedir
	if (!f)
	{
		f = FindInPaks(gamedir, filename, &size);
		if (!f)
			f = FindInPaks("id1", filename, &size);
		if (!f)
		{
			return NULL;
		}
	}
	else
	{
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
	}
	data = malloc(size);
	if (data)
		fread(data, 1, size, f);
	fclose(f);

	if (sizep)
		*sizep = size;
	return data;
}

unsigned char *FS_ReadFile(char *gamedir, char *filename, unsigned int *size)
{
	unsigned char *data;
	if (!gamedir || !*gamedir || !strcmp(gamedir, "qw"))
		data = NULL;
	else
		data = FS_ReadFile2(gamedir, filename, size);
	if (!data)
	{
		data = FS_ReadFile2("qw", filename, size);
		if (!data)
		{
			data = FS_ReadFile2("id1", filename, size);
			if (!data)
			{
				return NULL;
			}
		}
	}
	return data;
}

#ifndef _WIN32
#define _cdecl
#endif
int _cdecl SortFilesByDate(const void *a, const void *b)
{
	if (((const availdemo_t*)a)->time < ((const availdemo_t*)b)->time)
		return 1;
	if (((const availdemo_t*)a)->time > ((const availdemo_t*)b)->time)
		return -1;

	if (((const availdemo_t*)a)->smalltime < ((const availdemo_t*)b)->smalltime)
		return 1;
	if (((const availdemo_t*)a)->smalltime > ((const availdemo_t*)b)->smalltime)
		return -1;
	return 0;
}

void Cluster_BuildAvailableDemoList(cluster_t *cluster)
{
	cluster->availdemoscount = 0;

#ifdef _WIN32
	{
		WIN32_FIND_DATA ffd;
		HANDLE h;
		char path[512];
		snprintf(path, sizeof(path), "%s*.mvd", cluster->demodir);
		h = FindFirstFile(path, &ffd);
		if (h != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (cluster->availdemoscount == sizeof(cluster->availdemos)/sizeof(cluster->availdemos[0]))
					break;
				strlcpy(cluster->availdemos[cluster->availdemoscount].name, ffd.cFileName, sizeof(cluster->availdemos[0].name));
				cluster->availdemos[cluster->availdemoscount].size = ffd.nFileSizeLow;
				cluster->availdemos[cluster->availdemoscount].time = ffd.ftLastWriteTime.dwHighDateTime;
				cluster->availdemos[cluster->availdemoscount].smalltime = ffd.ftLastWriteTime.dwLowDateTime;
				cluster->availdemoscount++;
			} while(FindNextFile(h, &ffd));
			FindClose(h);
		}
	}
#else
	{
		DIR *dir;
		struct dirent *ent;
		struct stat sb;
		char fullname[512];
		dir = opendir(cluster->demodir);	//yeek!
		if (dir)
		{
			for(;;)
			{
				if (cluster->availdemoscount == sizeof(cluster->availdemos)/sizeof(cluster->availdemos[0]))
					break;
				ent = readdir(dir);
				if (!ent)
					break;
				if (*ent->d_name == '.')
					continue;	//ignore 'hidden' files
				snprintf(fullname, sizeof(fullname), "%s%s", cluster->demodir, ent->d_name);
				if (stat(fullname, &sb))
					continue;	//some kind of error
				strlcpy(cluster->availdemos[cluster->availdemoscount].name, ent->d_name, sizeof(cluster->availdemos[0].name));
				cluster->availdemos[cluster->availdemoscount].size = sb.st_size;
				cluster->availdemos[cluster->availdemoscount].time = sb.st_mtime;
				cluster->availdemoscount++;
			}
			closedir(dir);
		}
		else
			Sys_Printf(cluster, "Couldn't open dir %s for demo listings\n", cluster->demodir);
	}
#endif

	qsort(cluster->availdemos, cluster->availdemoscount, sizeof(cluster->availdemos[0]), SortFilesByDate);
}

void Cluster_Run(cluster_t *cluster, qboolean dowait)
{
	oproxy_t *pend, *pend2, *pend3;
	sv_t *sv, *old;
	tcpconnect_t *tc;

	int m;
	struct timeval timeout;
	fd_set socketset;
	fd_set socketset_wr;

	if (dowait)
	{
		//FIXME: use poll or epoll to work around FD_SETSIZE limits, though we're mostly only doing this for the sleeping.

		FD_ZERO(&socketset);
		FD_ZERO(&socketset_wr);
		m = 0;
		if (cluster->qwdsocket[0] != INVALID_SOCKET)
		{
			if (cluster->qwdsocket[0] < FD_SETSIZE)
			{
				FD_SET(cluster->qwdsocket[0], &socketset);
				if (cluster->qwdsocket[0] >= m)
					m = cluster->qwdsocket[0]+1;
			}
		}
		if (cluster->qwdsocket[1] != INVALID_SOCKET)
		{
			if (cluster->qwdsocket[1] < FD_SETSIZE)
			{
				FD_SET(cluster->qwdsocket[1], &socketset);
				if (cluster->qwdsocket[1] >= m)
					m = cluster->qwdsocket[1]+1;
			}
		}

		for (sv = cluster->servers; sv; sv = sv->next)
		{
			if (sv->usequakeworldprotocols && sv->sourcesock != INVALID_SOCKET)
			{
				if (sv->sourcesock >= FD_SETSIZE)
					continue;	//panic...
				FD_SET(sv->sourcesock, &socketset);
				if (sv->sourcesock >= m)
					m = sv->sourcesock+1;
			}
		}

		for (tc = cluster->tcpconnects; tc; tc = tc->next)
		{
			if (tc->sock != INVALID_SOCKET && tc->sock < FD_SETSIZE)
			{
				FD_SET(tc->sock, &socketset);
				if (tc->sock >= m)
					m = tc->sock+1;
			}
		}

		TURN_AddFDs(cluster, &socketset, &m);

		for (pend = cluster->pendingproxies; pend; pend = pend->next)
		{
			if (pend->sock != INVALID_SOCKET && pend->sock < FD_SETSIZE)
			{
				FD_SET(pend->sock, &socketset);
				if (pend->file)	//also wake up if we're doing some (large) file transfer and we can give them a bit more.
					FD_SET(pend->sock, &socketset_wr);
				if (pend->sock >= m)
					m = pend->sock+1;
			}
		}

	#ifndef _WIN32
		#ifndef STDIN
			#define STDIN 0
		#endif
		FD_SET(STDIN, &socketset);
		if (STDIN >= m)
			m = STDIN+1;
	#endif

		if (cluster->viewserver)
		{
			timeout.tv_sec = 0;
			timeout.tv_usec = 1000;
		}
		else
		{
			timeout.tv_sec = 10/1000;
			timeout.tv_usec = (100%1000)*1000;
		}

		m = select(m, &socketset, &socketset_wr, NULL, &timeout);

#ifdef _WIN32
		for (;;)
		{
			char buffer[8192];
			char *result;
			char c;

			if (!_kbhit())
				break;
			c = _getch();

			if (c == '\n' || c == '\r')
			{
				Sys_Printf(cluster, "\n");
				if (cluster->inputlength)
				{
					cluster->commandinput[cluster->inputlength] = '\0';
					result = Rcon_Command(cluster, NULL, cluster->commandinput, buffer, sizeof(buffer), true);
					Sys_Printf(cluster, "%s", result);
					cluster->inputlength = 0;
					cluster->commandinput[0] = '\0';
				}
			}
			else if (c == '\b')
			{
				if (cluster->inputlength > 0)
				{
					Sys_Printf(cluster, "%c", c);
					Sys_Printf(cluster, " ");
					Sys_Printf(cluster, "%c", c);

					cluster->inputlength--;
					cluster->commandinput[cluster->inputlength] = '\0';
				}
			}
			else
			{
				Sys_Printf(cluster, "%c", c);
				if (cluster->inputlength < sizeof(cluster->commandinput)-1)
				{
					cluster->commandinput[cluster->inputlength++] = c;
					cluster->commandinput[cluster->inputlength] = '\0';
				}
			}
		}
#else
		if (FD_ISSET(STDIN, &socketset))
		{
			char buffer[8192];
			char *result;
			cluster->inputlength = read (STDIN, cluster->commandinput, sizeof(cluster->commandinput));
			if (cluster->inputlength >= 1)
			{
				cluster->commandinput[cluster->inputlength-1] = 0;        // rip off the /n and terminate
				cluster->inputlength--;

				if (cluster->inputlength)
				{
					cluster->commandinput[cluster->inputlength] = '\0';
					result = Rcon_Command(cluster, NULL, cluster->commandinput, buffer, sizeof(buffer), true);
					printf("%s", result);
					cluster->inputlength = 0;
					cluster->commandinput[0] = '\0';
				}
			}
		}
#endif
	}



	cluster->curtime = Sys_Milliseconds();

	for (sv = cluster->servers; sv; )
	{
		old = sv;
		sv = sv->next;
		QTV_Run(old);
	}

	TURN_CheckFDs(cluster);

	SV_FindProxies(cluster->tcpsocket[0], cluster, NULL);	//look for any other proxies wanting to muscle in on the action.
	SV_FindProxies(cluster->tcpsocket[1], cluster, NULL);	//look for any other proxies wanting to muscle in on the action.

	QW_UpdateUDPStuff(cluster);

	while(cluster->pendingproxies)
	{
		pend2 = cluster->pendingproxies->next;
		if (SV_ReadPendingProxy(cluster, cluster->pendingproxies))
			cluster->pendingproxies = pend2;
		else
			break;
	}
	if (cluster->pendingproxies)
	{
		for(pend = cluster->pendingproxies; pend && pend->next; )
		{
			pend2 = pend->next;
			pend3 = pend2->next;
			if (SV_ReadPendingProxy(cluster, pend2))
			{
				pend->next = pend3;
				pend = pend3;
			}
			else
			{
				pend = pend2;
			}
		}
	}
}





void DoCommandLine(cluster_t *cluster, int argc, char **argv)
{
	int i;
	char commandline[8192];
	char *result;
	char *arg;
	char buffer[8192];

//exec the - commands
	commandline[0] = '\0';
	for (i = 1; i <= argc; i++)
	{
		if (i == argc)
			arg = "";
		else
		{
			arg = argv[i];
			if (!arg)	//NeXT can do this supposedly
				arg = "";
		}
		if(i == argc || *arg == '+' || *arg == '-')
		{
			if (commandline[0] == '-')
			{
				result = Rcon_Command(cluster, NULL, commandline+1, buffer, sizeof(buffer), true);
				Sys_Printf(cluster, "%s", result);
			}

			commandline[0] = '\0';
		}
		strcat(commandline, arg);
		strcat(commandline, " ");
	}

//exec the configs
	result = Rcon_Command(cluster, NULL, "exec qtv.cfg", buffer, sizeof(buffer), true);
	Sys_Printf(cluster, "%s", result);


//exec the + commands
	commandline[0] = '\0';
	for (i = 1; i <= argc; i++)
	{
		if (i == argc)
			arg = "";
		else
		{
			arg = argv[i];
			if (!arg)	//NeXT can do this supposedly
				arg = "";
		}
		if(i == argc || *arg == '+' || *arg == '-')
		{
			if (commandline[0] == '+')
			{
				result = Rcon_Command(cluster, NULL, commandline+1, buffer, sizeof(buffer), true);
				Sys_Printf(cluster, "%s", result);
			}

			commandline[0] = '\0';
		}
		strcat(commandline, arg);
		strcat(commandline, " ");
	}
}

#ifndef LIBQTV
int main(int argc, char **argv)
{
	cluster_t *cluster;

//	soundtest();

#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif

#ifdef _WIN32
	{
		WSADATA discard;
		WSAStartup(MAKEWORD(1,1), &discard);
	}
#endif

	cluster = malloc(sizeof(*cluster));
	if (cluster)
	{
		int j;
		memset(cluster, 0, sizeof(*cluster));

		for (j = 0; j < SOCKETGROUPS; j++)
		{
			cluster->qwdsocket[j] = INVALID_SOCKET;
			cluster->tcpsocket[j] = INVALID_SOCKET;
		}
		cluster->anticheattime = 1*1000;
		cluster->tooslowdelay = 100;
		cluster->qwlistenportnum = 0;
		cluster->allownqclients = true;
		strcpy(cluster->hostname, DEFAULT_HOSTNAME);
		cluster->maxproxies = -1;

		//master protocol setup
		cluster->protocolname = strdup("FTE-Quake");
		cluster->protocolver = 3;
		strlcpy(cluster->master, "master.frag-net.com:27950", sizeof(cluster->master));	//default to eukara's master server.
		cluster->mastersendtime = cluster->curtime;

		cluster->relayenabled = true;		//allow qtv
		cluster->pingtreeenabled = false;	//spammy.
		cluster->turnenabled = false;		//leave turn off by default. we need to know a usable inbound port range, we can't depend on just outgoing ephemerial ones. misconfigured relays will result in failures so don't default this to on.

#ifdef HAVE_EPOLL
		cluster->epfd = epoll_create1(0);
#endif

		strcpy(cluster->demodir, "qw/demos/");

		Sys_Printf(cluster, "QTV "QTV_VERSION_STRING"\n");

		DoCommandLine(cluster, argc, argv);

		if (!cluster->numservers)
		{	//probably running on a home user's computer
			if (cluster->qwdsocket[SG_IPV4] == INVALID_SOCKET && cluster->qwdsocket[SG_IPV6] == INVALID_SOCKET && !cluster->qwlistenportnum)
			{
				cluster->qwlistenportnum = 27599;
				NET_InitUDPSocket(cluster, cluster->qwlistenportnum, SG_IPV6);
				NET_InitUDPSocket(cluster, cluster->qwlistenportnum, SG_IPV4);
			}
			if (cluster->tcpsocket[SG_IPV4] == INVALID_SOCKET && cluster->tcpsocket[SG_IPV6] == INVALID_SOCKET && !cluster->tcplistenportnum)
			{
				cluster->tcplistenportnum = 27599;
				Net_TCPListen(cluster, cluster->tcplistenportnum, SG_IPV6);
				Net_TCPListen(cluster, cluster->tcplistenportnum, SG_IPV4);
			}
			Net_TCPListen(cluster, 1, SG_UNIX);

			Sys_Printf(cluster, "\n"
				"Welcome to QTV\n"
				"Please type\n"
				"qtv server:port\n"
				" to connect to a tcp server.\n"
				"qw server:port\n"
				" to connect to a regular qw server.\n"
				"demo qw/example.mvd\n"
				" to play a demo from an mvd.\n"
				"\n");
		}

//		Cluster_BuildAvailableDemoList(cluster);

		while (!cluster->wanttoexit)
		{
			Cluster_Run(cluster, true);
#ifdef VIEWER
			DemoViewer_Update(cluster->viewserver);
#endif
		}

		free(cluster);
	}

	return 0;
}
#endif

void QTV_Printf(sv_t *qtv, char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];

	va_start (argptr, fmt);
	vsnprintf (string, sizeof(string)-1, fmt,argptr);
	string[sizeof(string)-1] = 0;
	va_end (argptr);

	if (qtv->silentstream)
		return;

	Sys_Printf(qtv->cluster, "%s", string);
}

//#ifdef LIBQTV
//#ifndef _WIN32
//#define _cdecl
//#endif
//void _cdecl Con_Printf(char *fmt, ...);
//#endif

void Sys_Printf(cluster_t *cluster, char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	unsigned char *t;

	va_start (argptr, fmt);
	vsnprintf (string, sizeof(string)-1, fmt,argptr);
	string[sizeof(string)-1] = 0;
	va_end (argptr);

//#ifdef LIBQTV
//	Con_Printf("QTV: %s", string);
//#endif

	for (t = (unsigned char*)string; *t; t++)
	{
		if (*t >= 146 && *t < 156)
			*t = *t - 146 + '0';
		if (*t == 143)
			*t = '.';
		if (*t == 157 || *t == 158 || *t == 159)
			*t = '-';
		if (*t >= 128)
			*t -= 128;
		if (*t == 16)
			*t = '[';
		if (*t == 17)
			*t = ']';
		if (*t == 29)
			*t = '-';
		if (*t == 30)
			*t = '-';
		if (*t == 31)
			*t = '-';
		if (*t == '\a')	//doh. :D
			*t = ' ';
	}

	printf("%s", string);
}























//FIXME: move this to an appropriate place
#ifdef _WIN32
void Sys_mkdir(char *name)
{
	_mkdir(name);
}
#elif defined(__linux__)
void Sys_mkdir(char *name)
{
	mkdir(name, 0777);
}
#else
#warning no Sys_mkdir function defined, hope the default works for you
void Sys_mkdir(char *name)
{
	mkdir(name, 0777);
}
#endif

void QTV_mkdir(char *path)
{
	char	*ofs;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_mkdir (path);
			*ofs = '/';
		}
	}
}



/*


unsigned char *FS_ReadFile2(char *gamedir, char *filename, unsigned int *sizep)
{
	int size;
	unsigned char *data;

	FILE *f;
	char fname[1024];

	if (!*filename)
		return NULL;

	//try and read it straight out of the file system
	sprintf(fname, "%s/%s", gamedir, filename);
	f = fopen(fname, "rb");
	if (!f)
		f = fopen(filename, "rb");	//see if we're being run from inside the gamedir
	if (!f)
	{
		f = FindInPaks(gamedir, filename, &size);
		if (!f)
			f = FindInPaks("id1", filename, &size);
		if (!f)
		{
			return NULL;
		}
	}
	else
	{
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
	}
	data = malloc(size);
	if (data)
		fread(data, 1, size, f);
	fclose(f);

	if (sizep)
		*sizep = size;
	return data;
}

unsigned char *FS_ReadFile(char *gamedir, char *filename, unsigned int *size)
{
	char *data;
	if (!gamedir || !*gamedir || !strcmp(gamedir, "qw"))
		data = NULL;
	else
		data = FS_ReadFile2(gamedir, filename, size);
	if (!data)
	{
		data = FS_ReadFile2("qw", filename, size);
		if (!data)
		{
			data = FS_ReadFile2("id1", filename, size);
			if (!data)
			{
				return NULL;
			}
		}
	}
	return data;
}

void Cluster_Run(cluster_t *cluster, qboolean dowait)
{
	oproxy_t *pend, *pend2, *pend3;
	sv_t *sv, *old;

	int m;
	struct timeval timeout;
	fd_set socketset;

	if (dowait)
	{

		FD_ZERO(&socketset);
		m = 0;
		if (cluster->qwdsocket != INVALID_SOCKET)
		{
			FD_SET(cluster->qwdsocket, &socketset);
			if (cluster->qwdsocket >= m)
				m = cluster->qwdsocket+1;
		}

		for (sv = cluster->servers; sv; sv = sv->next)
		{
			if (sv->usequkeworldprotocols && sv->sourcesock != INVALID_SOCKET)
			{
				FD_SET(sv->sourcesock, &socketset);
				if (sv->sourcesock >= m)
					m = sv->sourcesock+1;
			}
		}

	#ifndef _WIN32
		#ifndef STDIN
			#define STDIN 0
		#endif
		FD_SET(STDIN, &socketset);
		if (STDIN >= m)
			m = STDIN+1;
	#endif

		if (cluster->viewserver)
		{
			timeout.tv_sec = 0;
			timeout.tv_usec = 1000;
		}
		else
		{
			timeout.tv_sec = 100/1000;
			timeout.tv_usec = (100%1000)*1000;
		}

		m = select(m, &socketset, NULL, NULL, &timeout);

#ifdef _WIN32
		for (;;)
		{
			char buffer[8192];
			char *result;
			char c;

			if (!_kbhit())
				break;
			c = _getch();

			if (c == '\n' || c == '\r')
			{
				Sys_Printf(cluster, "\n");
				if (cluster->inputlength)
				{
					cluster->commandinput[cluster->inputlength] = '\0';
					result = Rcon_Command(cluster, NULL, cluster->commandinput, buffer, sizeof(buffer), true);
					Sys_Printf(cluster, "%s", result);
					cluster->inputlength = 0;
					cluster->commandinput[0] = '\0';
				}
			}
			else if (c == '\b')
			{
				if (cluster->inputlength > 0)
				{
					Sys_Printf(cluster, "%c", c);
					Sys_Printf(cluster, " ", c);
					Sys_Printf(cluster, "%c", c);

					cluster->inputlength--;
					cluster->commandinput[cluster->inputlength] = '\0';
				}
			}
			else
			{
				Sys_Printf(cluster, "%c", c);
				if (cluster->inputlength < sizeof(cluster->commandinput)-1)
				{
					cluster->commandinput[cluster->inputlength++] = c;
					cluster->commandinput[cluster->inputlength] = '\0';
				}
			}
		}
#else
		if (FD_ISSET(STDIN, &socketset))
		{
			char buffer[8192];
			char *result;
			cluster->inputlength = read (0, cluster->commandinput, sizeof(cluster->commandinput));
			if (cluster->inputlength >= 1)
			{
				cluster->commandinput[cluster->inputlength-1] = 0;        // rip off the /n and terminate
				cluster->inputlength--;

				if (cluster->inputlength)
				{
					cluster->commandinput[cluster->inputlength] = '\0';
					result = Rcon_Command(cluster, NULL, cluster->commandinput, buffer, sizeof(buffer), true);
					printf("%s", result);
					cluster->inputlength = 0;
					cluster->commandinput[0] = '\0';
				}
			}
		}
#endif
	}



	cluster->curtime = Sys_Milliseconds();

	for (sv = cluster->servers; sv; )
	{
		old = sv;
		sv = sv->next;
		QTV_Run(old);
	}

	SV_FindProxies(cluster->tcpsocket, cluster, NULL);	//look for any other proxies wanting to muscle in on the action.

	QW_UpdateUDPStuff(cluster);

	while(cluster->pendingproxies)
	{
		pend2 = cluster->pendingproxies->next;
		if (SV_ReadPendingProxy(cluster, cluster->pendingproxies))
			cluster->pendingproxies = pend2;
		else
			break;
	}
	if (cluster->pendingproxies)
	{
		for(pend = cluster->pendingproxies; pend && pend->next; )
		{
			pend2 = pend->next;
			pend3 = pend2->next;
			if (SV_ReadPendingProxy(cluster, pend2))
			{
				pend->next = pend3;
				pend = pend3;
			}
			else
			{
				pend = pend2;
			}
		}
	}
}





*/