#include "quakedef.h"

#if defined(WEBSERVER) || defined(FTPSERVER)

#include "iweb.h"
#include "netinc.h"

qboolean HTTP_ServerPoll(qboolean httpserverwanted, int portnum);

#ifdef WEBSVONLY	//we need some functions from quake

char *NET_SockadrToString(char *s, int slen, struct sockaddr_qstorage *addr, size_t sizeofaddr)
{
	switch(((struct sockaddr*)addr)->sa_family)
	{
	case AF_INET:
		Q_snprintfz(s, slen, "%s:%u", inet_ntoa(((struct sockaddr_in*)addr)->sin_addr), ntohs(((struct sockaddr_in*)addr)->sin_port));
		break;
	case AF_INET6:
		if (!memcmp(((struct sockaddr_in6*)addr)->sin6_addr.s6_addr, "\0\0\0\0\0\0\0\0\0\0\xff\xff", 12))
		{	//ipv4-mapped
			Q_snprintfz(s, slen, "[::ffff:%u.%u.%u.%u]:%u", 
				((struct sockaddr_in6*)addr)->sin6_addr.s6_addr[12],
				((struct sockaddr_in6*)addr)->sin6_addr.s6_addr[13],
				((struct sockaddr_in6*)addr)->sin6_addr.s6_addr[14],
				((struct sockaddr_in6*)addr)->sin6_addr.s6_addr[15],

				ntohs(((struct sockaddr_in6*)addr)->sin6_port));
		}
		else
		{
			Q_snprintfz(s, slen, "[%x:%x:%x:%x:%x:%x:%x:%x]:%u", 
				ntohs(((unsigned short*)((struct sockaddr_in6*)addr)->sin6_addr.s6_addr)[0]),
				ntohs(((unsigned short*)((struct sockaddr_in6*)addr)->sin6_addr.s6_addr)[1]),
				ntohs(((unsigned short*)((struct sockaddr_in6*)addr)->sin6_addr.s6_addr)[2]),
				ntohs(((unsigned short*)((struct sockaddr_in6*)addr)->sin6_addr.s6_addr)[3]),

				ntohs(((unsigned short*)((struct sockaddr_in6*)addr)->sin6_addr.s6_addr)[4]),
				ntohs(((unsigned short*)((struct sockaddr_in6*)addr)->sin6_addr.s6_addr)[5]),
				ntohs(((unsigned short*)((struct sockaddr_in6*)addr)->sin6_addr.s6_addr)[6]),
				ntohs(((unsigned short*)((struct sockaddr_in6*)addr)->sin6_addr.s6_addr)[7]),

				ntohs(((struct sockaddr_in6*)addr)->sin6_port));
		}
		break;
	default:
		*s = 0;
		break;
	}
	return s;
}

qboolean SV_AllowDownload (const char *name)
{
	if (strstr(name, ".."))
		return false;
	if (strchr(name, ':'))
		return false;
	if (*name == '/' || *name == '\\')
		return false;
	return true;
}
char		com_token[sizeof(com_token)];
com_tokentype_t com_tokentype;
int		com_argc;
const char	**com_argv;

vfsfile_t *IWebGenerateFile(const char *name, const char *content, int contentlength)
{
	return NULL;
}
vfsfile_t *VFSSTDIO_Open(const char *osname, const char *mode, qboolean *needsflush);
vfsfile_t *QDECL FS_OpenVFS(const char *filename, const char *mode, enum fs_relative relativeto)
{
	return VFSSTDIO_Open(filename, mode, NULL);
}

#include "fs.h"
searchpathfuncs_t *QDECL FSSTDIO_OpenPath(vfsfile_t *mustbenull, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
static searchpath_t filesystem;
int FS_FLocateFile(const char *filename, unsigned int lflags, flocation_t *loc)
{
	if (!filesystem.handle)
	{
		filesystem.handle = FSSTDIO_OpenPath(NULL, NULL, ".", ".", NULL);
		if (!filesystem.handle)
		{
			printf("Filesystem unavailable\n");
			return 0;
		}
	}

	if (filesystem.handle->FindFile(filesystem.handle, loc, filename, NULL))
	{
		loc->search = &filesystem;
		return 1;
	}
	return (lflags&FSLF_DEEPONFAILURE)?0x7fffffff:0;
}
qboolean FS_GetLocMTime(flocation_t *location, time_t *modtime)
{
	*modtime = 0;
	if (!location->search->handle->FileStat || !location->search->handle->FileStat(location->search->handle, location, modtime))
		return false;
	return true;
}
struct vfsfile_s *FS_OpenReadLocation(const char *fname, flocation_t *location)
{
	return location->search->handle->OpenVFS(location->search->handle, location, "rb");
}

void Q_strncpyz(char *d, const char *s, int n)
{
	int i;
	n--;
	if (n < 0)
		return;	//this could be an error

	for (i=0; *s; i++)
	{
		if (i == n)
			break;
		*d++ = *s++;
	}
	*d='\0';
}

qboolean VARGS Q_vsnprintfz (char *dest, size_t size, const char *fmt, va_list argptr)
{
	size_t ret;
#ifdef _WIN32
	//doesn't null terminate.
	//returns -1 on truncation
	ret = _vsnprintf (dest, size, fmt, argptr);
	dest[size-1] = 0;	//shitty paranoia
#else
	//always null terminates.
	//returns length regardless of truncation.
	ret = vsnprintf (dest, size, fmt, argptr);
#endif
#ifdef _DEBUG
	if (ret>=size)
		Sys_Error("Q_vsnprintfz: Truncation\n");
#endif
	//if ret is -1 (windows oversize, or general error) then it'll be treated as unsigned so really long. this makes the following check quite simple.
	return ret>=size;
}
qboolean VARGS Q_snprintfz (char *dest, size_t size, const char *fmt, ...)
{
	va_list		argptr;
	qboolean ret;
	va_start (argptr, fmt);
	ret = Q_vsnprintfz(dest, size, fmt, argptr);
	va_end (argptr);
	return ret;
}

/*char	*va(char *format, ...)
{
#define VA_BUFFERS 2 //power of two
	va_list		argptr;
	static char		string[VA_BUFFERS][1024];
	static int bufnum;

	bufnum++;
	bufnum &= (VA_BUFFERS-1);
	
	va_start (argptr, format);
	_vsnprintf (string[bufnum],sizeof(string[bufnum])-1, format,argptr);
	va_end (argptr);

	return string[bufnum];	
}*/

#undef _vsnprintf
void Sys_Error(const char *format, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, format);
#ifdef _WIN32
	_vsnprintf (string,sizeof(string)-1, format,argptr);
	string[sizeof(string)-1] = 0;
#else
	vsnprintf (string,sizeof(string), format,argptr);
#endif
	va_end (argptr);

	printf("%s", string);
	getchar();
	exit(1000);
}

int COM_CheckParm(const char *parm)
{
	return 0;
}

#ifndef _WIN32
#include <signal.h>
#endif

#ifdef HAVE_EPOLL
struct stun_ctx
{
	struct epollctx_s pub;
	int udpsock;
	int reportport;
	int id[3];
};
struct stunheader_s
{
	unsigned short msgtype;
	unsigned short msglen;
	unsigned int magiccookie;
	unsigned int transactid[3];
};
static void StunResponse(struct epollctx_s *pubctx, unsigned int ev)
{
	struct stun_ctx *ctx = (struct stun_ctx*)pubctx;
	unsigned char buf[8192];
	int respsize = recvfrom(ctx->udpsock, buf, sizeof(buf), 0, NULL, NULL);
	int offset;
	struct stunheader_s *h = (struct stunheader_s*)buf;
	if (h->transactid[0] != ctx->id[0] ||
		h->transactid[1] != ctx->id[1] ||
		h->transactid[2] != ctx->id[2])
		return;	//someone trying to spoof?

	if (((buf[0]<<8)|buf[1]) == 0x0101)
	{
		unsigned short attr;
		unsigned short sz;
		offset = sizeof(struct stunheader_s);
		while (offset+4 < respsize)
		{
			attr = (buf[offset+0]<<8)|buf[offset+1];
			sz   = (buf[offset+2]<<8)|buf[offset+3];
			offset+= 4;

			if (offset + sz > respsize)
				break;	//corrupt.
			if ((attr == 0x1 || attr == 0x20) && sz >= 4)
			{
				unsigned short type = (buf[offset+0]<<8)|buf[offset+1];
				unsigned short port = (buf[offset+2]<<8)|buf[offset+3];
				if (attr == 0x20)
					port ^= (buf[4]<<8)|buf[5];
				if (sz == 4+4 && type == 1)
				{
					printf("Address: %s%i.%i.%i.%i:%u\n",
						ctx->reportport?"http://":"",
						buf[offset+4]^((attr == 0x20)?buf[4]:0),
						buf[offset+5]^((attr == 0x20)?buf[5]:0),
						buf[offset+6]^((attr == 0x20)?buf[6]:0),
						buf[offset+7]^((attr == 0x20)?buf[7]:0),ctx->reportport?ctx->reportport:port
					);
				}
			}
			offset += sz;
		}
	}
}
void PrepareStun(int epfd, int reportport)
{
#if 0
	char *stunserver = "localhost";
	int stunport = 27500;
#else	//sorry about hardcoding a server, but probably few people are gonna care enough.
	char *stunserver = "master.frag-net.com";
	int stunport = 27950;
#endif

	SOCKET newsocket;
	struct sockaddr_in address;
	struct hostent *h;

	struct stunheader_s msg = {htons(1), 0, htonl(0x2112a442), {42,42,42}};
	if (epfd < 0)
		return;

	h = gethostbyname(stunserver);
	if (!h)
		return;

	if (h->h_addrtype != AF_INET)
		return;	//too many assumptions

	if ((newsocket = socket (h->h_addrtype, SOCK_CLOEXEC|SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
		return;

	address.sin_family = h->h_addrtype;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = 0;

	if (bind (newsocket, (void *)&address, sizeof(address)) == -1)
		return;

	//FIXME: not random enough to avoid hacks.
	srand(time(NULL)^*(int*)&msg);
	msg.transactid[0] = rand();
	msg.transactid[1] = rand();
	msg.transactid[2] = rand();
	if (epfd >= 0)
	{
		static struct stun_ctx ctx;
		struct epoll_event ev;
		ctx.udpsock = newsocket;
		ctx.pub.Polled = StunResponse;
		ctx.id[0] = msg.transactid[0];
		ctx.id[1] = msg.transactid[1];
		ctx.id[2] = msg.transactid[2];
		ctx.reportport = reportport;
		ev.events = EPOLLIN;
		ev.data.ptr = &ctx;
		epoll_ctl(epfd, EPOLL_CTL_ADD, newsocket, &ev);
	}

	msg.msglen = htons(sizeof(msg)-20);
	memcpy(&address.sin_addr, h->h_addr, h->h_length);
	address.sin_port = htons(stunport);
	sendto(newsocket, &msg, sizeof(msg), 0, (struct sockaddr*)&address, sizeof(address));
}
#endif

char *authedusername;
char *autheduserpassword;
int lport_min, lport_max;
int anonaccess = IWEBACC_READ;
iwboolean verbose;
int main(int argc, char **argv)
{
	int httpport = 80;
	int ftpport = 21;
	int arg = 1;
#ifdef _WIN32
	WSADATA pointlesscrap;
	WSAStartup(2, &pointlesscrap);
#else
#ifdef HAVE_EPOLL
	int ep = epoll_create1(0);
#endif
	signal(SIGPIPE, SIG_IGN);	//so we don't crash out if a peer closes the socket half way through.
#endif

	while (arg < argc)
	{
		char *a = argv[arg];
		if (!a)
			continue;
		if (*a != '-')
			break;	//other stuff
		while (*a == '-')
			a++;
		arg++;

		if (!strcmp(a, "help"))
		{
			printf("%s -http 80 -ftp 21 -user steve -pass swordfish -ports 5000 6000\n", argv[0]);
			printf("runs a simple http server\n");
			printf("	-http <num> specifies the port to listen on for http\n");
			printf("	-ftp <num> specifies the port to listen on for ftp\n");
			printf("	-ports <lowest> <highest> specifies a port range for incoming ftp connections, to work around firewall rules\n");
			printf("	-user <name> specifies the username that has full access. if not supplied noone can write.\n");
			printf("	-pass <pass> specifies the password to go with that username\n");
			printf("	-noanon will refuse to serve files to anyone but the authed user\n");
			return 0;
		}
		else if (!strcmp(a, "port") || !strcmp(a, "p"))
		{
			httpport = atoi(argv[arg++]);
			ftpport = 0;
		}
		else if (!strcmp(a, "http") || !strcmp(a, "h"))
			httpport = atoi(argv[arg++]);
		else if (!strcmp(a, "ftp") || !strcmp(a, "f"))
			ftpport = atoi(argv[arg++]);
		else if (!strcmp(a, "noanon"))
			anonaccess = 0;
		else if (!strcmp(a, "ports"))
		{
			lport_min = atoi(argv[arg++]);
			lport_max = atoi(argv[arg++]);
			if (lport_max < lport_min)
				lport_max = lport_min;
		}
		else if (!strcmp(a, "verbose") || !strcmp(a, "v"))
			verbose = true;
		else if (!strcmp(a, "user"))
			authedusername = argv[arg++];
		else if (!strcmp(a, "pass"))
			autheduserpassword = argv[arg++];
		else
			printf("Unknown argument: %s\n", a);
	}

	if (arg < argc && atoi(argv[arg]))
	{
		httpport = atoi(argv[arg++]);
		ftpport = 0;
	}
	if (arg < argc)
		authedusername = argv[arg++];
	if (arg < argc)
		autheduserpassword = argv[arg++];

	if (httpport)
		printf("http port %i\n", httpport);
	else
		printf("http not enabled\n");
	if (ftpport)
		printf("ftp port %i\n", ftpport);
	else
		printf("ftp not enabled\n");
	if (authedusername || autheduserpassword)
		printf("Username = \"%s\"\nPassword = \"%s\"\n", authedusername, autheduserpassword);
	else
		printf("Server is read only\n");

#ifndef HAVE_EPOLL
	while(1)
	{
		if (ftpport)
			FTP_ServerRun(1, ftpport);
		if (httpport)
			HTTP_ServerPoll(1, httpport);
#ifdef _WIN32
		Sleep(1);
#else
		usleep(10000);
#endif
	}
#else
	while (!HTTP_ServerInit(ep, httpport))
		sleep(5);
	PrepareStun(ep, httpport);
	for (;;)
	{
		struct epoll_event events[1];
		int e, me = epoll_wait(ep, events, countof(events), -1);
		for (e = 0; e < me; e++)
		{
			struct epollctx_s *ctx = events[e].data.ptr;
			ctx->Polled(ctx, events[e].events);
		}
	}
#endif
}

int IWebGetSafeListeningPort(void)
{
	static int sequence;
	return lport_min + (sequence++ % (lport_max+1-lport_min));
}
void VARGS IWebDPrintf(char *fmt, ...)
{
	va_list args;
	if (!verbose)
		return;
	va_start (args, fmt);
	vprintf (fmt, args);
	va_end (args);
}


#ifdef _WIN32
#ifdef _MSC_VER
#define ULL(x) x##ui64
#else
#define ULL(x) x##ull
#endif

static time_t Sys_FileTimeToTime(FILETIME ft)
{
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;
	return ull.QuadPart / ULL(10000000) - ULL(11644473600);
}
void COM_EnumerateFiles (const char *match, int (*func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *f), void *parm)
{
	HANDLE r;
	WIN32_FIND_DATAA fd;	
	char apath[MAX_OSPATH];
	char file[MAX_OSPATH+MAX_PATH];
	char *s;
	int go;
	strcpy(apath, match);
//	sprintf(apath, "%s%s", gpath, match);
	for (s = apath+strlen(apath)-1; s>= apath; s--)
	{
		if (*s == '/')			
			break;
	}
	s++;
	*s = '\0';	
	
	strcpy(file, match);
	r = FindFirstFileA(file, &fd);
	if (r==(HANDLE)-1)
		return;
	go = true;
	do
	{
		if (*fd.cFileName == '.');
		else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//is a directory
		{
			sprintf(file, "%s%s/", apath, fd.cFileName);
			go = func(file, fd.nFileSizeLow, Sys_FileTimeToTime(fd.ftLastWriteTime), parm, NULL);
		}
		else
		{
			sprintf(file, "%s%s", apath, fd.cFileName);
			go = func(file, fd.nFileSizeLow, Sys_FileTimeToTime(fd.ftLastWriteTime), parm, NULL);
		}
	}
	while(FindNextFileA(r, &fd) && go);
	FindClose(r);
}
#else
void COM_EnumerateFiles (const char *match, int (*func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *f), void *parm)
{
	//No implementation on unix etc
}
#endif

char *COM_ParseType (const char *data, char *out, size_t outlen, com_tokentype_t *toktype)
{
	int		c;
	int		len;
	
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
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		com_tokentype = TTP_STRING;
		data++;
		while (1)
		{
			if (len >= outlen-1)
				return (char*)data;

			c = *data++;
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return (char*)data;
			}
			out[len] = c;
			len++;
		}
	}

	com_tokentype = TTP_UNKNOWN;

// parse a regular word
	do
	{
		if (len >= outlen-1)
			return (char*)data;

		out[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);
	
	out[len] = 0;
	return (char*)data;
}

/*#undef COM_ParseToken
char *COM_ParseToken (const char *data, const char *punctuation)
{
	int		c;
	int		len;
	len = 0;
	com_token[0] = 0;
	
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
		com_tokentype = TTP_STRING;
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return (char*)data;
			}
			com_token[len] = c;
			len++;
		}
	}

	com_tokentype = TTP_UNKNOWN;

// parse single characters
	if (c==',' || c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':' || c==';' || c == '=' || c == '!' || c == '>' || c == '<' || c == '&' || c == '|' || c == '+')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return (char*)data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (c==',' || c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':' || c==';' || c == '=' || c == '!' || c == '>' || c == '<' || c == '&' || c == '|' || c == '+')
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return (char*)data;
}*/

/*
IWEBFILE *IWebFOpenRead(char *name)					//fread(name, "rb");
{
	FILE *f;
	char name2[512];
	if (strstr(name, ".."))
		return NULL;
	sprintf(name2, "%s/%s", com_gamedir, name);
	f = fopen(name2, "rb");
	if (f)
	{
		IWEBFILE *ret = IWebMalloc(sizeof(IWEBFILE));
		if (!ret)
		{
			fclose(f);
			return NULL;
		}
		ret->f = f;
		ret->start = 0;

		fseek(f, 0, SEEK_END);
		ret->end = ftell(f);//ret->start+ret->length;
		fseek(f, 0, SEEK_SET);

		ret->length = ret->end - ret->start;
		return ret;
	}
	return NULL;
}
*/


#else

#if defined(WEBSERVER) || defined(FTPSERVER)
static cvar_t sv_readlevel = CVAR("sv_readlevel", "0");	//default to allow anyone
static cvar_t sv_writelevel = CVARD("sv_writelevel", "35", "Specifies the required trust level at which user accounts may write to the user-specific subdir of /uploads/USERNAME/*. If blank, then no uploads are permitted");	//allowed to write to uploads/uname
static cvar_t sv_fulllevel = CVARD("sv_fulllevel", "51", "User accounts with an access level greater than this may write anywhere, including the gamedir. Note that setting this low is increadibly risky. An empty value will be understood to never give this permission.");	//allowed to write anywhere, replace any file...
#ifdef WEBSERVER
static cvar_t httpserver = CVAR("sv_http", "0");
static cvar_t httpserver_port = CVAR("sv_http_port", "80");
#endif
#ifdef FTPSERVER
static cvar_t ftpserver = CVAR("sv_ftp", "0");
static cvar_t ftpserver_port = CVAR("sv_ftp_port", "21");
static cvar_t sv_ftp_port_range = CVARD("sv_ftp_port_range", "0", "Specifies the port range for the server to create listening sockets for 'active' ftp connections, to work around NAT/firewall issues.\nMost FTP clients should use passive connections, but there's still some holdouts like windows.");

int IWebGetSafeListeningPort(void)
{
	char *e;
	int base, range;
	static int sequence;
	if (!sv_ftp_port_range.string || !*sv_ftp_port_range.string)
		return 0;	//lets the OS pick.
	base = strtol(sv_ftp_port_range.string, &e, 0);
	while(*e == ' ')
		e++;
	if (*e == '-')
		e++;
	while(*e == ' ')
		e++;
	range = strtol(e, NULL, 0);
	if (range < base)
		range = base;
	return base + (sequence++ % (range+1-base));
}
#endif
#endif

//this file contains functions called from each side.

void VARGS IWebWarnPrintf(char *fmt, ...)
{
	va_list		argptr;
	char		msg[4096];

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-10, fmt,argptr);	//catch any nasty bugs... (this is hopefully impossible)
	va_end (argptr);

	Con_Printf(CON_WARNING "%s", msg);
}

void IWebInit(void)
{
#ifdef WEBSERVER
	Cvar_Register(&sv_fulllevel, "Internet Server Access");
	Cvar_Register(&sv_writelevel, "Internet Server Access");
	Cvar_Register(&sv_readlevel, "Internet Server Access");

	Cvar_Register(&ftpserver, "Internet Server Access");
	Cvar_Register(&ftpserver_port, "Internet Server Access");
	Cvar_Register(&sv_ftp_port_range, "Internet Server Access");
	Cvar_Register(&httpserver, "Internet Server Access");
	Cvar_Register(&httpserver_port, "Internet Server Access");

	//don't allow these to be changed easily
	//this basically blocks these from rcon / stuffcmd
	ftpserver.restriction = RESTRICT_MAX;
	httpserver.restriction = RESTRICT_MAX;
	sv_fulllevel.restriction = RESTRICT_MAX;
	sv_writelevel.restriction = RESTRICT_MAX;
	sv_readlevel.restriction = RESTRICT_MAX;
#endif
}
void IWebRun(void)
{
#ifdef FTPSERVER
	{
		extern qboolean ftpserverfailed;
		FTP_ServerRun(ftpserver.ival!= 0, ftpserver_port.ival);
		if (ftpserverfailed)
		{
			Con_Printf("FTP Server failed to load, setting %s to 0\n", ftpserver.name);
			Cvar_SetValue(&ftpserver, 0);
			ftpserverfailed = false;
		}
	}
#endif

#ifdef WEBSERVER
	{
		extern qboolean httpserverfailed;
		HTTP_ServerPoll(httpserver.ival!=0, httpserver_port.ival);
		if (httpserverfailed)
		{
			Con_Printf("HTTP Server failed to load, setting %s to 0\n", httpserver.name);
			Cvar_SetValue(&httpserver, 0);
			httpserverfailed = false;
		}
	}
#endif
}
void IWebShutdown(void)
{
}
#endif

#ifdef WEBSVONLY
void *Sys_CreateThread(char *name, int (*func)(void *), void *args, int priority, int stacksize)
{
	return NULL;
}
void Sys_WaitOnThread(void *thread)
{
}
qboolean FS_Remove(const char *fname, enum fs_relative relativeto)
{
	return false;
}
qboolean FS_SystemPath(const char *fname, enum fs_relative relativeto, char *out, int outlen)
{
	Q_strncpyz(out, fname, outlen);
	if (*out == '/' || strstr(out, ".."))
	{
		*out = 0;
		return false;
	}
	return strlen(fname) == strlen(out);
}
void FS_FlushFSHashWritten(const char *fname) {}
void FS_FlushFSHashRemoved(const char *fname) {}
qboolean FS_Rename(const char *oldf, const char *newf, enum fs_relative relativeto)
{
	return rename(oldf, newf) != -1;
}
#ifdef _WIN32
#include <direct.h>
void FS_CreatePath(const char *pname, enum fs_relative relativeto)
{
	_mkdir(pname);
}
qboolean Sys_rmdir (const char *path)
{
	return _rmdir(path) != -1;
}
#else
#include <unistd.h>
#include <sys/stat.h>
void FS_CreatePath(const char *pname, enum fs_relative relativeto)
{
	mkdir(pname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}
qboolean Sys_rmdir (const char *path)
{
	return rmdir(path) != -1;
}
#endif

#endif




int IWebAuthorize(const char *name, const char *password)
{
#ifdef WEBSVONLY
	if (authedusername)
	{
		if (!strcmp(name, authedusername))
		{
			if (!strcmp(password, autheduserpassword))
				return IWEBACC_FULL;
		}
	}

	//if they tried giving some other username, don't give them any access (prevents them from reading actual user files).
	if (*name && stricmp(name, "anonymous"))
		return 0;
	return anonaccess;
#else
#ifndef CLIENTONLY
	int id = Rank_GetPlayerID(NULL, name, atoi(password), false, true);
	rankinfo_t info;
	if (!id)
	{
		if (!sv_readlevel.value && (!*name || !stricmp(name, "anonymous")))
			return IWEBACC_READ;	//read only anywhere
		return 0;
	}

	Rank_GetPlayerInfo(id, &info);

	if (*sv_fulllevel.string && info.s.trustlevel >= sv_fulllevel.value)
		return IWEBACC_READ	| IWEBACC_WRITE | IWEBACC_FULL;	//allowed to read and write anywhere to the quake filesystem
	if (*sv_writelevel.string && info.s.trustlevel >= sv_writelevel.value)
		return IWEBACC_READ	| IWEBACC_WRITE;	//allowed to read anywhere write to specific places
	if (info.s.trustlevel >= sv_readlevel.value)
		return IWEBACC_READ;	//read only anywhere
#endif
	return 0;
#endif
}

iwboolean IWebAllowUpLoad(const char *fname, const char *uname)	//called for partial write access
{
	if (strstr(fname, ".."))
		return false;
	if (!strncmp(fname, "uploads/", 8))
	{
		if (!strncmp(fname+8, uname, strlen(uname)))
			if (fname[8+strlen(uname)] == '/')
				return true;
	}
	return false;
}

char *Q_strcpyline(char *out, const char *in, int maxlen)
{
	char *w = out;
	while (*in && maxlen > 0)
	{
		if (*in == '\r' || *in == '\n')
			break;
		*w = *in;
		in++;
		w++;
		maxlen--;
	}
	*w = '\0';
	return out;
}

#endif
