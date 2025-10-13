#include "quakedef.h"

#ifdef WEBSVONLY
#undef vsnprintf
#undef _vsnprintf
#ifdef _WIN32
#define vsnprintf _vsnprintf
#endif
#endif

#ifdef FTPSERVER

#include "iweb.h"

//hows this as a bug.
//TCP data can travel at different speeds.
//If the later bits of a data channel arrive after the message saying that a transfer was compleate,
//the later bits of the file may not arrive before the client closes the conenction.
//this is a major bug and can prevent the server from giving files at a high pl/ping

#include "netinc.h"

static iwboolean ftpserverinitied = false;
static SOCKET	ftpserversocket = INVALID_SOCKET;
static int	ftpserverport = 0;
qboolean ftpserverfailed;


typedef struct FTPclient_s{
	char peername[256];
	char name[64];
	char pwd[64];
	int auth;	//has it got auth?
	char path[256];

	char renamefrom[256];

	char commandbuffer[256];
	char messagebuffer[256];
	int cmdbuflen;
	int msgbuflen;

	int controlaf;
	SOCKET controlsock;
	SOCKET datasock;	//FTP only allows one transfer per connection.
	int dataislisten;
	int datadir;	//0 no data, 1 reading, 2 writing
	vfsfile_t *file;

	qboolean brieflist;	//incoming list command was an nlist

	qofs_t restartpos;

	unsigned long blocking;

#ifdef MULTITHREAD
	void *transferthread;
#endif

	struct FTPclient_s *next;
} FTPclient_t;

FTPclient_t *FTPclient;

SOCKET FTP_BeginListening(int aftype, int port)
{
	struct sockaddr_qstorage address;
	unsigned long _true = true;
	int i;
	SOCKET sock;

	int af;
	int prot;

	if (!port)
		port = IWebGetSafeListeningPort();

	switch(aftype)
	{
	case 0:
#ifdef IPPROTO_IPV6
	case 2:
		af = AF_INET6;
		prot = IPPROTO_TCP;
		break;
#endif
	case 1:
		af = AF_INET;
		prot = IPPROTO_TCP;
		break;
//	case 11:
//		af = AF_IPX;
//		prot = NSPROTO_SPX;
//		break;
	default:
		return INVALID_SOCKET;
	}

	if ((sock = socket (af, SOCK_STREAM, prot)) == -1)
	{
		IWebPrintf ("FTP_BeginListening: socket: %s\n", strerror(neterrno()));
		return INVALID_SOCKET;
	}

	if (ioctlsocket (sock, FIONBIO, &_true) == -1)
	{
		IWebPrintf ("FTP_BeginListening: ioctl FIONBIO: %s", strerror(neterrno()));
		return INVALID_SOCKET;
	}

#ifdef IPPROTO_IPV6
	if (aftype == 0 || aftype == 2)
	{
		//0=ipv4+ipv6
		//2=ipv6 only
		if (aftype == 0)
		{
			unsigned long _false = false;
			if (0 > setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&_false, sizeof(_false)))
			{
				//abort and do ipv4 only if hybrid sockets don't work.
				closesocket(sock);
				return FTP_BeginListening(1, port);
			}
		}
		else
			setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&_true, sizeof(_true));

		memset(&address, 0, sizeof(address));
		((struct sockaddr_in6*)&address)->sin6_family = AF_INET6;
		if (port == PORT_ANY)
			((struct sockaddr_in6*)&address)->sin6_port = 0;
		else
			((struct sockaddr_in6*)&address)->sin6_port = htons((short)port);
	}
	else
#endif
	{
		//1=ipv4 only
		((struct sockaddr_in*)&address)->sin_family = AF_INET;
	//ZOID -- check for interface binding option
		if ((i = COM_CheckParm("-ip")) != 0 && i < com_argc) {
			((struct sockaddr_in*)&address)->sin_addr.s_addr = inet_addr(com_argv[i+1]);
			Con_TPrintf("Binding to IP Interface Address of %s\n",
					inet_ntoa(((struct sockaddr_in*)&address)->sin_addr));
		} else
			((struct sockaddr_in*)&address)->sin_addr.s_addr = INADDR_ANY;

		if (port == PORT_ANY)
			((struct sockaddr_in*)&address)->sin_port = 0;
		else
			((struct sockaddr_in*)&address)->sin_port = htons((short)port);
	}

	if( bind (sock, (void *)&address, sizeof(address)) == -1)
	{
		IWebPrintf("FTP_BeginListening: failed to bind socket\n");
		closesocket(ftpserversocket);
		return INVALID_SOCKET;
	}

	listen(sock, 3);

	return sock;
}

void FTP_ServerShutdown(void)
{
	closesocket(ftpserversocket);
	ftpserversocket = INVALID_SOCKET;
	ftpserverinitied = false;
	IWebPrintf("FTP server is deactivated\n");
}

static iwboolean FTP_AllowUpLoad(const char *fname, FTPclient_t *cl)
{
	if (cl->auth & IWEBACC_FULL)
		return true;
	if (!(cl->auth & IWEBACC_WRITE))
		return false;

	return IWebAllowUpLoad(fname, cl->name);
}
static iwboolean FTP_AllowDownLoad(const char *fname, FTPclient_t *cl)
{
	if (cl->auth & IWEBACC_FULL)
		return true;
	if (!(cl->auth & IWEBACC_READ))
		return false;

	if (FTP_AllowUpLoad(fname, cl))
		return true;

	return SV_AllowDownload(fname);
}
static iwboolean FTP_AllowList(const char *fname, FTPclient_t *cl)
{
	return FTP_AllowDownLoad(fname, cl) || FTP_AllowUpLoad(fname, cl);
}

//we ought to filter this to remove duplicates.
static int QDECL SendFileNameTo(const char *rawname, qofs_t size, time_t mtime, void *param, searchpathfuncs_t *spath)
{
	FTPclient_t *cl = param;
	SOCKET socket = cl->datasock;
//	int i;
	char buffer[256+1];
	char *slash;
	char nondirname[MAX_QPATH];
	int isdir = rawname[strlen(rawname)-1] == '/';
	char *fname;

#ifndef WEBSVONLY	//copy protection of the like that QWSV normally has.
	if (!isdir)
		if (!FTP_AllowList(rawname, cl))
			return true;
#endif

	Q_strncpyz(nondirname, rawname, sizeof(nondirname));
	if (isdir)
		nondirname[strlen(nondirname)-1] = '\0';
	fname = nondirname;

	while((slash = strchr(fname, '/')))
		fname = slash+1;

	if (cl->brieflist)
		Q_snprintfz(buffer, sizeof(buffer), "%s\r\n", fname);
	else
	{
		char timestamp[32];
		if (1)
			strftime(timestamp, sizeof(timestamp), "%b %d  %Y", gmtime(&mtime));
		else
			strftime(timestamp, sizeof(timestamp), "%b %d %H:%M", gmtime(&mtime));

		Q_snprintfz(buffer, sizeof(buffer), "%c%c%c-------\t1\troot\troot\t%8"PRIuQOFS" %s %s\r\n",
			isdir?'d':'-',
			FTP_AllowDownLoad(rawname, cl)?'r':'-',
			FTP_AllowUpLoad(rawname, cl)?'w':'-',
			size, timestamp, fname);
	}

//	strcpy(buffer, fname);
//	for (i = strlen(buffer); i < 40; i+=8)
//		strcat(buffer, "\t");
	send(socket, buffer, strlen(buffer), 0);

	return true;
}

SOCKET FTP_SV_makelistensocket(unsigned long nblocking)
{
	char name[256];
	SOCKET sock;
	struct hostent *hent;

	struct sockaddr_in	address;
//	int fromlen;

	address.sin_family = AF_INET;
	if (gethostname(name, sizeof(name)) == -1)
		return INVALID_SOCKET;
	hent = gethostbyname(name);
	if (!hent)
		return INVALID_SOCKET;
	address.sin_addr.s_addr = *(int *)(hent->h_addr_list[0]);
	address.sin_port = 0;



	if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		Sys_Error ("FTP_TCP_OpenSocket: socket: %s", strerror(neterrno()));
	}

	if (ioctlsocket (sock, FIONBIO, &nblocking) == -1)
	{
		Sys_Error ("FTP_TCP_OpenSocket: ioctl FIONBIO: %s", strerror(neterrno()));
	}

	if( bind (sock, (void *)&address, sizeof(address)) == -1)
	{
		closesocket(sock);
		return INVALID_SOCKET;
	}

	listen(sock, 2);

	return sock;
}
int	FTP_SVGetSocketPort (SOCKET socket)
{
	struct sockaddr_qstorage addr;
	int adrlen = sizeof(addr);

	if (getsockname(socket, (struct sockaddr*)&addr, &adrlen) == -1)
		return false;

	if (((struct sockaddr_in*)&addr)->sin_family == AF_INET6)
		return ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
	else if (((struct sockaddr_in*)&addr)->sin_family == AF_INET)
		return ntohs(((struct sockaddr_in*)&addr)->sin_port);
	else
		return 0; //no idea
}
//only to be used for ipv4 sockets.
iwboolean	FTP_SVSocketToV4String (SOCKET socket, char *s)
{
	struct sockaddr_qstorage addr;
	qbyte *baddr;
	int adrlen = sizeof(addr);
	char name[256];
	unsigned short port;

	if (getsockname(socket, (struct sockaddr*)&addr, &adrlen) == -1)
		return false;
	if (((struct sockaddr_in*)&addr)->sin_family == AF_INET6)
	{
		port = ((struct sockaddr_in6*)&addr)->sin6_port;
		baddr = ((struct sockaddr_in6*)&addr)->sin6_addr.s6_addr;
		if (memcmp(baddr, "\0\0\0\0\0\0\0\0\0\0\xff\xff", 12))
			return false;	//must be ipv4-mapped for this ipv4 function
		baddr += 12;
	}
	else if (((struct sockaddr_in*)&addr)->sin_family == AF_INET)
	{
		port = ((struct sockaddr_in*)&addr)->sin_port;
		baddr = (qbyte*)&((struct sockaddr_in*)&addr)->sin_addr;
	}
	else
		return false;

	if (!*(int*)baddr)
	{
		//FIXME doesn't work on anything but windows.
		if (gethostname(name, sizeof(name)) != -1)
		{
			struct hostent *hent = gethostbyname(name);
			if (hent)
				baddr = hent->h_addr_list[0];
		}
	}

	if (!baddr)
		return false;
	sprintf(s, "%i,%i,%i,%i,%i,%i", baddr[0], baddr[1], baddr[2], baddr[3], ((qbyte *)&port)[0], ((qbyte *)&port)[1]);
	return true;
}
iwboolean	FTP_V4StringToAdr (const char *s, struct sockaddr_in *addr)
{
	((qbyte*)&addr->sin_addr)[0] = strtol(s, (char**)&s, 0);
	if (*s++ != ',') return false;
	((qbyte*)&addr->sin_addr)[1] = strtol(s, (char**)&s, 0);
	if (*s++ != ',') return false;
	((qbyte*)&addr->sin_addr)[2] = strtol(s, (char**)&s, 0);
	if (*s++ != ',') return false;
	((qbyte*)&addr->sin_addr)[3] = strtol(s, (char**)&s, 0);
	if (*s++ != ',') return false;
	((qbyte*)&addr->sin_port)[0] = strtol(s, (char**)&s, 0);
	if (*s++ != ',') return false;
	((qbyte*)&addr->sin_port)[1] = strtol(s, (char**)&s, 0);

	return true;
}

#if defined(_WIN32) && !defined(WEBSVONLY)
	int (WINAPI *pgetaddrinfo) (
	  const char* nodename,
	  const char* servname,
	  const struct addrinfo* hints,
	  struct addrinfo** res
	);
	void (WSAAPI *pfreeaddrinfo) (struct addrinfo*);
#else
#define pgetaddrinfo getaddrinfo
#define pfreeaddrinfo freeaddrinfo
#endif

iwboolean FTP_HostToSockaddr(int prot, char *host, int port, struct sockaddr_qstorage *addr, size_t *addrsize)
{
	iwboolean r = false;
	struct addrinfo *res, hint;
	char service[16];

	*addrsize = 0;

	memset(&hint, 0, sizeof(hint));
	hint.ai_flags = AI_NUMERICHOST;
	switch(prot)
	{
	case 1:
		hint.ai_family = AF_INET;
		break;
	case 2:
		hint.ai_family = AF_INET6;
		break;
	}
	Q_snprintfz(service, sizeof(service), "%i", port);
	if (pgetaddrinfo(host, service, &hint, &res))
		return false;
	if (res && res->ai_addr && res->ai_addrlen <= sizeof(*addr))
	{
		*addrsize = res->ai_addrlen;
		memcpy(addr, res->ai_addr, res->ai_addrlen);
		r = true;
	}
	pfreeaddrinfo(res);
	return r;
#if 0
	host = va("[%s]", host);
	return NET_StringToSockaddr(host, port, addr, NULL, NULL);
#endif
}

/*
 *	Responsable for sending all control server -> client messages.
 *	Queues the message if it cannot send now.
 *	Kicks if too big a queue.
*/
void QueueMessage(FTPclient_t *cl, char *msg)
{
	IWebDPrintf("FTP> %s", msg);
	if (send (cl->controlsock, msg, strlen(msg), 0) == -1)
	{	//wasn't sent
		if (strlen(msg) + strlen(cl->messagebuffer) >= sizeof(cl->messagebuffer)-1)
		{
			closesocket(cl->controlsock);	//but don't mark it as closed, so we get errors later (for this is how we shall tell).
			return;
		}
		strcat(cl->messagebuffer, msg);
	}
}

void VARGS QueueMessageva(FTPclient_t *cl, char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];

	va_start (argptr, fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	msg[sizeof(msg)-1] = 0;
	va_end (argptr);

	IWebDPrintf("FTP> %s", msg);
	if (send (cl->controlsock, msg, strlen(msg), 0) == -1)
	{	//wasn't sent
		if (strlen(msg) + strlen(cl->messagebuffer) >= sizeof(cl->messagebuffer)-1)
		{
			closesocket(cl->controlsock);
			cl->controlsock = INVALID_SOCKET;
		}
		strcat(cl->messagebuffer, msg);
	}
}

//if eg: "RETR Some File Name.txt \r\n", and the RETR was already parsed, we'll read out the Some File Name until the \r\n
//returns a directly-usable game path.
qboolean FTP_ReadToAbsFilename(FTPclient_t *cl, const char *msg, char *out, size_t outsize)
{
	size_t len = 0;
	const char *end;
	if (*msg == ' ')
		msg++;
	else
	{
		*out = 0;
		return false;
	}
	end = msg;
	while(*end)
	{
		if (*end == '\r' || *end == '\n')
			break;
		end++;
	}
	//trim stupid trailing space that windows insists on fucking shit up with
	if (end > msg && end[-1] == ' ')
		end--;

	//figure out the root
	if (*msg == '/')
		msg++;
	else
	{	//FIXME: nuke the +1s
		len = strlen(cl->path);
		if (len >= outsize)
		{	//too long...
			*out = 0;
			return false;
		}
		memcpy(out, cl->path, len);
	}

	while (msg < end)
	{
		if (*msg == '/')
			msg++;	//double slashes will be silently ignored. also simplifies ../ etc.
		else if (!strncmp(msg, "..", 2) && (msg+2 == end || msg[2] == '/'))
		{	// "/foo/../bar" should end up as just "bar"
			msg+=2;
			if (!len)
			{	//can't go above root...
				*out = 0;
				return false;
			}
			while (len > 0)
			{
				if (out[--len] == '/')
					break;
			}
		}
		else if (!strncmp(msg, ".", 1) && (msg+1 == end || msg[1] == '/'))
		{	//filenames relative to the working directory are stupid, but whatever
			msg+=1;
			while (len > 0)
			{
				if (out[--len] == '/')
					break;
			}
		}
		else
		{
			const char *s;
			for (s = msg; s < end; s++)
			{
				if (*s == '/')
					break;
			}
			if (s == msg)
			{	//error...
				*out = 0;
				return false;
			}
			if (len + s-msg + 2 > outsize)
				break;
			if (len)
				out[len++] = '/';
			memcpy(out+len, msg, s-msg);
			len += s-msg;
			msg = s;
		}
	}
	out[len] = 0;
	return true;
}

#ifdef MULTITHREAD
int FTP_TransferThread(void *vcl)
{
	char resource[8192];
	FTPclient_t *cl = vcl;
	u_long _false = false;
	ioctlsocket (cl->datasock, FIONBIO, &_false);

	if ((cl->datadir&~64) == 1)
	{
		while(1)
		{
			int ammount = VFS_READ(cl->file, resource, sizeof(resource));
			int chunk, sent;
			if (ammount <= 0)
				break;
			for (sent = 0; sent < ammount; sent += chunk)
			{
				chunk = send(cl->datasock, resource, ammount-sent, 0);
				if (chunk <= 0)
					break;
			}
			if (ammount != sent)
				break;
		}
	}
	else if ((cl->datadir&~64) == 2)
	{
		while(1)
		{
			int ammount = recv(cl->datasock, resource, sizeof(resource), 0);
			if (ammount <= 0)
				break;
			if (ammount != VFS_WRITE(cl->file, resource, ammount))
				break;
		}
	}
	cl->datadir &= ~64;

	return 0;
}
#endif

iwboolean FTP_ServerThinkForConnection(FTPclient_t *cl)
{
	int 	ret;
	struct sockaddr_qstorage	from;
	int		fromlen;
	char *msg, *line;

	char mode[64];
	char resource[8192];
	int _true = true;

#ifdef MULTITHREAD
	if (cl->datadir & 64)
	{
		if (*cl->messagebuffer)
		{	//fixme: gah!
			if (send (cl->controlsock, cl->messagebuffer, strlen(cl->messagebuffer), 0) != -1)
				*cl->messagebuffer = '\0';	//YAY! It went!
		}
		return false;
	}
	if (cl->transferthread)
	{
		Sys_WaitOnThread(cl->transferthread);
		cl->transferthread = NULL;
	}
#endif

	if (cl->datadir == 1)
	{
		int pos, sent;
		int ammount, wanted = sizeof(resource);

		pos = VFS_TELL(cl->file);
		ammount = VFS_READ(cl->file, resource, wanted);
		sent = send(cl->datasock, resource, ammount, 0);

		if (sent == -1)
		{
			VFS_SEEK(cl->file, pos);
			if (neterrno() != NET_EWOULDBLOCK)
			{
				closesocket(cl->datasock);
				cl->datasock = INVALID_SOCKET;
				VFS_CLOSE(cl->file);
				cl->file = NULL;

				QueueMessage (cl, "226 Transfer complete .\r\n");
				cl->datadir = 0;
			}
		}
		else
		{
			if (sent != ammount)
				VFS_SEEK(cl->file, pos + sent);

			if (ammount != wanted && sent == ammount)	//file is over
			{
				send(cl->datasock, resource, 0, 0);
				send(cl->datasock, resource, 0, 0);
				send(cl->datasock, resource, 0, 0);
				closesocket(cl->datasock);
				cl->datasock = INVALID_SOCKET;
				VFS_CLOSE(cl->file);
				cl->file = NULL;

				QueueMessage (cl, "226 Transfer complete .\r\n");
				cl->datadir = 0;
			}
		}

		pos = cl->datadir?1:!cl->blocking;
		if (ioctlsocket (cl->controlsock, FIONBIO, (u_long *)&pos) == -1)
		{
			IWebPrintf ("FTP_ServerRun: blocking error: %s\n", strerror(neterrno()));
			return 0;
		}
	}
	else if (cl->datadir == 2)
	{
		int len;
		while((len = recv(cl->datasock, resource, sizeof(resource), 0)) >0 )
		{
			VFS_WRITE(cl->file, resource, len);
		}
		if (len == -1)
		{
			if (neterrno() != NET_EWOULDBLOCK)
			{
				closesocket(cl->datasock);
				cl->datasock = INVALID_SOCKET;
				if (cl->file)
					VFS_CLOSE(cl->file);
				cl->file = NULL;

				QueueMessage (cl, "226 Transfer complete .\r\n");
				cl->datadir = 0;
			}
		}
		if (len == 0)
		{
			QueueMessage (cl, "226 Transfer complete .\r\n");
			VFS_CLOSE(cl->file);
			cl->file = NULL;
			cl->datadir = 0;
		}
	}

	ret = recv(cl->controlsock, cl->commandbuffer+cl->cmdbuflen, sizeof(cl->commandbuffer)-1 - cl->cmdbuflen, 0);
	if (ret == -1)
	{
		int e = neterrno();
		if (e == NET_EWOULDBLOCK)
			return false;	//remove

		if (e == NET_ECONNABORTED || e == NET_ECONNRESET)
			return true;

		IWebPrintf ("NET_GetPacket: %s\n", strerror(e));
		return true;
	}
	if (*cl->messagebuffer)
	{
		if (send (cl->controlsock, cl->messagebuffer, strlen(cl->messagebuffer), 0) != -1)
			*cl->messagebuffer = '\0';	//YAY! It went!

	}

	if (ret == 0)
		return false;
	cl->cmdbuflen += ret;
	cl->commandbuffer[cl->cmdbuflen] = 0;

	line = cl->commandbuffer;
	while (1)
	{
		msg = line;
		while (*line)
		{
			if (*line == '\r')
				*line = ' ';
			if (*line == '\n')
				break;
			line++;
		}
		if (!*line)	//broken client
		{
			memmove(cl->commandbuffer, line, strlen(line)+1);
			cl->cmdbuflen = strlen(line);
			break;
		}
		*line = '\0';
		line++;
		IWebDPrintf("FTP: %s\n", msg);

		msg = COM_ParseOut(msg, mode, sizeof(mode));
		if (!stricmp(mode, "SYST"))
		{
			QueueMessage (cl, "215 UNIX Type: L8.\r\n");	//some browsers can be wierd about things.
		}
		else if (!stricmp(mode, "FEAT"))
		{
			QueueMessage (cl, "211-Extensions supported:\r\n");
			QueueMessage (cl, " SIZE\r\n");
			QueueMessage (cl, " REST\r\n");
			QueueMessage (cl, " EPSV\r\n");
			QueueMessage (cl, " EPRT\r\n");
//			QueueMessage (cl, " MDTM\r\n");
			QueueMessage (cl, "211 End\r\n");
		}
		else if (!stricmp(mode, "user"))
		{
			msg = COM_ParseOut(msg, cl->name, sizeof(cl->name));
			cl->auth = 0;	//any access rights go away now, so they can't spoof read access with dodgy filenames.

			QueueMessage (cl, "331 User name received, will be checked with password.\r\n");
		}
		else if (!stricmp(mode, "pass"))
		{
			msg = COM_ParseOut(msg, cl->pwd, sizeof(cl->pwd));

			cl->auth = IWebAuthorize(cl->name, cl->pwd);

			if (cl->auth)
				QueueMessage (cl, "230 User logged in.\r\n");
			else
				QueueMessage (cl, "530 Username or Password was incorrect or otherwise invalid.\r\n");
		}
		else if (!stricmp(mode, "TYPE"))
		{
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			msg = COM_ParseOut(msg, resource, sizeof(resource));

			if (!stricmp(resource, "A"))	//ascii
			{
				QueueMessage (cl, "200 ascii selected.\r\n");
			}
			else if (!stricmp(resource, "I"))	//binary
			{
				QueueMessage (cl, "200 binary selected.\r\n");
			}
			else
			{
				QueueMessage (cl, "200 ascii selected.\r\n");
			}
		}
		else if (!stricmp(mode, "PWD"))
		{
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (*cl->path)
				QueueMessageva (cl, "257 \"/%s/\"\r\n", cl->path);
			else
				QueueMessageva (cl, "257 \"/\"\r\n");
		}
		else if (!stricmp(mode, "CWD"))
		{
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (!FTP_ReadToAbsFilename(cl, msg, resource, sizeof(resource)))
			{
				QueueMessage (cl, "550 invalid path.\r\n");
				continue;
			}

			Q_strncpyz(cl->path, resource, sizeof(cl->path));
			QueueMessage (cl, "200 directory changed.\r\n");
		}
		else if (!stricmp(mode, "MKD"))
		{
			//create directory
			FTP_ReadToAbsFilename(cl, msg, resource, sizeof(resource));
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (!*resource || !FTP_AllowUpLoad(resource, cl))
			{
				IWebPrintf("%s: Denied mkdir request for \"ftp://%s@%s/%s\"\n", cl->peername, cl->name, "", resource);
				QueueMessage (cl, "550 Access denied.\r\n");
				continue;
			}

			IWebPrintf("%s: Mkdir request for \"ftp://%s@%s/%s\"\n", cl->peername, cl->name, "", resource);

			Q_strncatz(resource, "/", sizeof(resource));
			FS_CreatePath(resource, FS_GAMEONLY);
			QueueMessage (cl, "250 Success.\r\n");
		}
		else if (!stricmp(mode, "EPSV"))
		{
			int aftype = 0;
			//one argument, "1"=ipv4, "2"=ipv6, "11"=ipx (by rfc 1700). if not present, use same as control connection
			//reply: "229 Entering Extended Passive Mode (|||$PORTNUM|)\r\n"

			while(*msg == ' ')
				msg++;
			if (!strncmp(msg, "ALL", 3))
				continue;	//rfc2428 'EPSV ALL' is a signal to client NATs that PORT/EPRT/PASV will not be used, and that they can just treat it as a regular TCP connection same as anything else. we 'must' also refuse any of those commands too, but we shouldn't receive them anyway.
			aftype = atoi(msg);
			if (!aftype)
				aftype = cl->controlaf;

			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (cl->datasock != INVALID_SOCKET)
			{
				closesocket(cl->datasock);
				cl->datasock = INVALID_SOCKET;
			}

			cl->datasock = FTP_BeginListening(aftype, 0);
			if (cl->datasock == INVALID_SOCKET)
				QueueMessage (cl, "425 server was unable to make a listen socket\r\n");
			else
			{
				QueueMessageva (cl, "229 Entering Extended Passive Mode (|||%i|).\r\n", FTP_SVGetSocketPort(cl->datasock));
			}
			cl->dataislisten = true;
		}
		else if (!stricmp(mode, "PASV"))
		{
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (cl->datasock != INVALID_SOCKET)
			{
				closesocket(cl->datasock);
				cl->datasock = INVALID_SOCKET;
			}

			cl->datasock = FTP_BeginListening(1, 0);
			if (cl->datasock == INVALID_SOCKET)
				QueueMessage (cl, "425 server was unable to make a listen socket\r\n");
			else
			{
				if (FTP_SVSocketToV4String(cl->datasock, resource))
					QueueMessageva (cl, "227 Entering Passive Mode (%s).\r\n", resource);
				else
					QueueMessageva (cl, "550 Unable to parse address.\r\n", resource);
			}
			cl->dataislisten = true;
		}
		else if (!stricmp(mode, "EPRT"))
		{
			//eg: one of:
			//EPRT |1|132.235.1.2|6275|
			//EPRT |2|1080::8:800:200C:417A|5282|

			//reply: 522 Network protocol not supported, use (1,2)

			char d;
			int prot;
			int port;
			char *eon, *host;
			struct sockaddr_qstorage peer;
			size_t peersize;
			while(*msg == ' ')
				msg++;
			d = *msg++;
			prot = strtol(msg, &msg, 0);
			host = ++msg;
			eon = strchr(msg, d);
			if (eon)
			{
				*eon++ = 0;
				msg = eon;
			}
			cl->dataislisten = false;
			if (cl->datasock != INVALID_SOCKET)
				closesocket(cl->datasock);
			cl->datasock = INVALID_SOCKET;

			port = strtol(msg, &msg, 0);
			if (*msg != d || !eon || !FTP_HostToSockaddr(prot, host, port, &peer, &peersize))
				QueueMessage (cl, "522 Network protocol not supported, use (1,2).\r\n");
			else
			{
				memset(&from, 0, sizeof(from));
				((struct sockaddr*)&from)->sa_family = ((struct sockaddr*)&peer)->sa_family;
				if ((cl->datasock = socket (((struct sockaddr*)&from)->sa_family, SOCK_STREAM, IPPROTO_TCP)) != -1)
				{
					if (ioctlsocket (cl->datasock, FIONBIO, (u_long *)&_true) != -1)
					if( bind (cl->datasock, (void *)&from, peersize) != -1)
					{
						connect(cl->datasock, (struct sockaddr *)&peer, peersize);
						QueueMessage (cl, "200 Opened data channel.\r\n");
						continue;
					}
					closesocket(cl->datasock);
					cl->datasock=INVALID_SOCKET;
				}
				QueueMessage (cl, "550 Command not fully implemented.\r\n");
			}
		}
		else if (!stricmp(mode, "PORT"))
		{
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (cl->datasock != INVALID_SOCKET)
			{
				closesocket(cl->datasock);
				cl->datasock = INVALID_SOCKET;
			}
			msg = COM_ParseOut(msg, resource, sizeof(resource));

			cl->dataislisten = false;

			memset(&from, 0, sizeof(from));
			((struct sockaddr_in*)&from)->sin_family = AF_INET;

			if ((cl->datasock = socket (((struct sockaddr*)&from)->sa_family, SOCK_STREAM, IPPROTO_TCP)) == -1)
			{
				Sys_Error ("FTP_ServerThinkForConnection: socket: %s", strerror(neterrno()));
			}

			if (ioctlsocket (cl->datasock, FIONBIO, (u_long *)&_true) == -1)
			{
				Sys_Error ("FTP_ServerThinkForConnection: ioctl FIONBIO: %s", strerror(neterrno()));
			}

			if( bind (cl->datasock, (void *)&from, sizeof(from)) == -1)
			{
				closesocket(cl->datasock);
				cl->datasock=INVALID_SOCKET;

				QueueMessage (cl, "425 server bind error.\r\n");
				continue;
			}


			fromlen = sizeof(from);
			if (FTP_V4StringToAdr(resource, (struct sockaddr_in *)&from))
			{
				connect(cl->datasock, (struct sockaddr *)&from, fromlen);

				QueueMessage (cl, "200 Opened data channel.\r\n");
			}
			else
			{
				closesocket(cl->datasock);
				cl->datasock=INVALID_SOCKET;

				QueueMessage (cl, "425 server resolve error.\r\n");
			}
		}
		else if (!stricmp(mode, "LIST") || !stricmp(mode, "NLST"))
		{
			char buffer[256];
			cl->brieflist = !stricmp(mode, "NLST");
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (cl->dataislisten)	//accept a connect.
			{
				int err;
				int _true = true;
				int temp;
				struct sockaddr_qstorage adr;
				int adrlen = sizeof(adr);
				temp = accept(cl->datasock, (struct sockaddr *)&adr, &adrlen);
				closesocket(cl->datasock);
				cl->datasock = temp;
				cl->dataislisten = false;

				if (cl->datasock == INVALID_SOCKET)
				{
					err = neterrno();
					QueueMessageva (cl, "425 Can't accept pasv data connection - %i.\r\n", err);
					continue;
				}
				else
					ioctlsocket(cl->datasock, FIONBIO, (u_long *)&_true);
			}
			if (cl->datasock == INVALID_SOCKET)
			{
				QueueMessage (cl, "503 Bad sequence of commands.\r\n");
				continue;
			}
			if (*cl->path == '/')
				strcpy(buffer, cl->path+1);
			else
				strcpy(buffer, cl->path);

			if (*buffer)	//last character should be a /
				if (buffer[strlen(buffer)-1] != '/')
					strcat(buffer, "/");

			strcat(buffer, "*");
			QueueMessage (cl, "125 Opening FAKE ASCII mode data connection for file.\r\n");

			COM_EnumerateFiles(buffer, SendFileNameTo, cl);

			QueueMessage (cl, "226 Transfer complete.\r\n");

			closesocket(cl->datasock);
			cl->datasock = INVALID_SOCKET;
		}
		/*else if (!stricmp(mode, "MDTM"))
		{
			char ospath[MAX_OSPATH];
			struct tm *t;
			FTP_ReadToAbsFilename(cl, msg, resource, sizeof(resource));
			if (FS_NativePath(resource, FS_GAME, ospath, sizeof(ospath)))
			{
				t = gmtime(Sys_FileTime(path));
				QueueMessageva (cl, "213 %04i%02i%02i%02i%02i%02i\r\n",
					1900+t->tm_year, 1+t->tm->mon, 1+t->tm->mday,
					t->tm->hour, t->tm->min, t->tm_sec );
			}
			else
				QueueMessageva (cl, "550 unavailable.\r\n", size);
		}*/
		else if (!stricmp(mode, "SIZE"))	//why IE can't use the list command to find file length, I've no idea.
		{
			//STRU, MODE, and TYPE may change the reported size...
			vfsfile_t *f;
			qofs_t size = qofs_ErrorValue();
			FTP_ReadToAbsFilename(cl, msg, resource, sizeof(resource));

			if (*resource && FTP_AllowList(resource, cl))
			{
				f = FS_OpenVFS(resource, "rb", FS_GAME);
				if (f)
				{
					size = VFS_GETLEN(f);
					VFS_CLOSE(f);
				}
			}

			if (qofs_Error(size))
				QueueMessageva (cl, "550 Couldn't read file.\r\n", size);
			else
				QueueMessageva (cl, "213 %"PRIuQOFS"\r\n", size);
		}
		else if (!stricmp(mode, "REST"))
		{
			COM_ParseOut(msg, resource, sizeof(resource));
			cl->restartpos = strtoull(resource, NULL, 0);
		}
		else if (!stricmp(mode, "RETR"))
		{
			qboolean waspassive = cl->dataislisten;
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (cl->dataislisten)	//accept a connect.
			{
				int _true = true;
				int temp;
				struct sockaddr_qstorage adr;
				int adrlen = sizeof(adr);
				temp = accept(cl->datasock, (struct sockaddr *)&adr, &adrlen);
				closesocket(cl->datasock);
				cl->datasock = temp;
				cl->dataislisten = false;

				if (cl->datasock == INVALID_SOCKET)
				{
					QueueMessageva (cl, "425 Can't accept pasv data connection - %i.\r\n", neterrno());
					continue;
				}
				else
					ioctlsocket(cl->datasock, FIONBIO, (u_long *)&_true);
			}
			if (cl->datasock == INVALID_SOCKET)
			{
				QueueMessage (cl, "503 Bad sequence of commands.\r\n");
				continue;
			}

			FTP_ReadToAbsFilename(cl, msg, resource, sizeof(resource));

			IWebPrintf("%s: Download request for \"ftp://%s@%s/%s\"\n", cl->peername, cl->name, "", resource);

			if (FTP_AllowDownLoad(resource, cl))
				cl->file = FS_OpenVFS(resource, "rb", FS_GAME);
			else
				cl->file = IWebGenerateFile(resource, NULL, 0);

			if (!cl->file)
			{
				QueueMessage (cl, "550 File not found.\r\n");
			}
			else
			{	//send data
				if (waspassive)
					QueueMessage (cl, "150 Opening BINARY mode data connection for file.\r\n");
				else
					QueueMessage (cl, "125 Opening BINARY mode data connection for file.\r\n");

				cl->datadir = 1;

				if (cl->restartpos)
					VFS_SEEK(cl->file, cl->restartpos);
			}
			cl->restartpos = 0;
		}
		else if (!stricmp(mode, "STOR") || !stricmp(mode, "APPE"))
		{
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}

			FTP_ReadToAbsFilename(cl, msg, resource, sizeof(resource));

			if (!*resource || !FTP_AllowUpLoad(resource, cl))
			{
				IWebPrintf("%s: Denied upload request for \"ftp://%s@%s/%s\"\n", cl->peername, cl->name, "", resource);
				QueueMessage (cl, "550 Permission denied.\r\n");
			}
			else
			{
				if (cl->dataislisten)	//accept a connect.
				{
					int _true = true;
					int temp;
					struct sockaddr_qstorage adr;
					int adrlen = sizeof(adr);
					temp = accept(cl->datasock, (struct sockaddr *)&adr, &adrlen);
					closesocket(cl->datasock);
					cl->datasock = temp;
					cl->dataislisten = false;

					if (cl->datasock == INVALID_SOCKET)
					{
						QueueMessageva (cl, "425 Can't accept pasv data connection - %i.\r\n", neterrno());
						continue;
					}
					else
						ioctlsocket(cl->datasock, FIONBIO, (u_long *)&_true);
				}
				if (cl->datasock == INVALID_SOCKET)
				{
					QueueMessage (cl, "502 Bad sequence of commands.\r\n");
					continue;
				}

				IWebPrintf("%s: Upload request for \"ftp://%s@%s/%s\"\n", cl->peername, cl->name, "", resource);

				if (cl->restartpos || !stricmp(mode, "APPE"))	//write without truncating.
					cl->file = FS_OpenVFS(resource, "w+b", FS_GAMEONLY);
				else
				{
					cl->file = FS_OpenVFS(resource, "rb", FS_GAMEONLY);
					if (cl->file)
					{
						VFS_CLOSE(cl->file);
						QueueMessage (cl, "550 File already exists.\r\n");
						continue;
					}
					cl->file = FS_OpenVFS(resource, "wb", FS_GAME);
				}

				if (!cl->file)
				{
					QueueMessage (cl, "550 Couldn't open output.\r\n");
				}
				else
				{	//send data
					QueueMessage (cl, "125 Opening BINARY mode data connection for input.\r\n");

					cl->datadir = 2;

					if (cl->restartpos)
						VFS_SEEK(cl->file, cl->restartpos);
					else if (!stricmp(mode, "APPE"))
						VFS_SEEK(cl->file, VFS_GETLEN(cl->file));
				}
				cl->restartpos = 0;
			}
		}
		else if (!stricmp(mode, "RNFR"))
		{
			FTP_ReadToAbsFilename(cl, msg, cl->renamefrom, sizeof(cl->renamefrom));

			QueueMessage (cl, "350 Success.\r\n");
		}
		else if (!stricmp(mode, "RNTO"))
		{
			FTP_ReadToAbsFilename(cl, msg, resource, sizeof(resource));

			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (!*cl->renamefrom)


			if (!FTP_AllowUpLoad(cl->renamefrom, cl) || !FTP_AllowUpLoad(resource, cl))
			{
				QueueMessage (cl, "550 Access denied.\r\n");
				continue;
			}

			IWebPrintf("%s: Rename request from \"ftp://%s@/%s\" to \"/%s\"\n", cl->peername, cl->name, cl->renamefrom, resource);

			if (FS_Rename(cl->renamefrom, resource, FS_GAMEONLY))
				QueueMessage (cl, "250 Success.\r\n");
			else
				QueueMessage (cl, "550 Requested action not taken.\r\n");

			FS_FlushFSHashRemoved(cl->renamefrom);
			FS_FlushFSHashWritten(resource);
			*cl->renamefrom = 0;
		}
		else if (!stricmp(mode, "DELE") || !stricmp(mode, "RMD"))
		{
			FTP_ReadToAbsFilename(cl, msg, resource, sizeof(resource));
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			if (!*resource || !FTP_AllowUpLoad(resource, cl))
			{
				IWebPrintf("%s: Denied delete request for \"ftp://%s@/%s\"\n", cl->peername, cl->name, resource);
				QueueMessage (cl, "550 Access denied.\r\n");
				continue;
			}
			IWebPrintf("%s: Delete request for \"ftp://%s@/%s\"\n", cl->peername, cl->name, resource);
			if (!stricmp(mode, "RMD"))
			{
				char path[MAX_OSPATH];
				if (FS_SystemPath(resource, FS_GAMEONLY, path, sizeof(path)) && Sys_rmdir(path))
					QueueMessage (cl, "250 Success.\r\n");
				else
					QueueMessage (cl, "550 Requested action not taken.\r\n");
			}
			else
			{
				if (FS_Remove(resource, FS_GAMEONLY))
					QueueMessage (cl, "250 Success.\r\n");
				else
					QueueMessage (cl, "550 Requested action not taken.\r\n");
			}

			FS_FlushFSHashRemoved(resource);
		}
		else if (!stricmp(mode, "STRU"))
		{
			if (!cl->auth)
			{
				QueueMessage (cl, "530 Not logged in.\r\n");
				continue;
			}
			msg = COM_ParseOut(msg, resource, sizeof(resource));
			if (!strcmp(resource, "F"))	//file
			{
				QueueMessage (cl, "200 recordless structure selected.\r\n");
			}
//			else if (!strcmp(resource, "R"))	//record
//			else if (!strcmp(resource, "P"))	//page
			else
			{
				QueueMessage (cl, "504 not implemented (it's a simple server).\r\n");
			}
		}
		else if (!stricmp(mode, "NOOP"))
		{
			QueueMessage (cl, "200 Do something then!\r\n");
		}
		else if (!stricmp(mode, "QUIT"))
		{
			QueueMessage (cl, "200 About to quit.\r\n");
			return true;
		}
		else
		{
			QueueMessage (cl, "502 Command not implemented.\r\n");
		}
	}

#ifdef MULTITHREAD
	if (cl->datadir && !cl->transferthread)
	{
		cl->datadir|=64;
		cl->transferthread = Sys_CreateThread("FTP RECV", FTP_TransferThread, cl, 0, 65536);
	}
#endif
	return false;
}

#if defined(WEBSVONLY) && defined(_WIN32)
DWORD WINAPI BlockingClient(void *ctx)
{
	FTPclient_t *cl = ctx;
	unsigned long _false = false;
	if (ioctlsocket (cl->controlsock, FIONBIO, &_false) == -1)
	{
		IWebPrintf ("FTP_ServerRun: blocking error: %s\n", strerror(neterrno()));
		return 0;
	}

	cl->blocking = true;

	while (!FTP_ServerThinkForConnection(cl))
	{
		Sleep(10);
	}

	if (cl->file)
		VFS_CLOSE(cl->file);
	closesocket(cl->controlsock);
	if (cl->datasock)
		closesocket(cl->datasock);

	IWebFree(cl);
	return 0;
}
#endif

iwboolean FTP_ServerRun(iwboolean ftpserverwanted, int port)
{
	FTPclient_t *cl, *prevcl;
	struct sockaddr_qstorage	from;
	int		fromlen;
	SOCKET clientsock;
unsigned long _true = true;

	if (!port)
		port = 21;
	if (ftpserverport != port)
	{
		ftpserverport = port;
		ftpserverwanted = false;	//forces it to restart if the port is changed.
	}

	if (!ftpserverinitied)
	{
		if (ftpserverwanted)
		{
			ftpserversocket = FTP_BeginListening(0, port);
			if (ftpserversocket == INVALID_SOCKET)
			{
				ftpserverfailed = true;
				IWebPrintf("Unable to establish listening FTP socket\n");
			}
			else
				IWebPrintf("FTP server is running\n");
			ftpserverinitied = true;
		}
		return false;
	}
	else if (!ftpserverwanted)
	{
		FTP_ServerShutdown();
		return false;
	}

	prevcl = NULL;
	for (cl = FTPclient; cl; cl = cl->next)
	{
		if (FTP_ServerThinkForConnection(cl))
		{
			if (cl->file)
				VFS_CLOSE(cl->file);
			closesocket(cl->controlsock);
			if (cl->datasock)
				closesocket(cl->datasock);

			if (prevcl)
			{
				prevcl->next = cl->next;
				IWebFree(cl);
				cl = prevcl;

				if (!cl)	//kills loop
					break;
			}
			else
			{
				FTPclient = cl->next;
				IWebFree(cl);
				cl = FTPclient;

				if (!cl)	//kills loop
					break;
			}
		}
		prevcl = cl;
	}

	fromlen = sizeof(from);
	if (ftpserversocket == INVALID_SOCKET)
		clientsock = INVALID_SOCKET;
	else
		clientsock = accept(ftpserversocket, (struct sockaddr *)&from, &fromlen);

	if (clientsock == INVALID_SOCKET)
	{
		int e = neterrno();
		if (e == NET_EWOULDBLOCK)
			return false;

		if (e == NET_ECONNABORTED || e == NET_ECONNRESET)
		{
			Con_TPrintf ("Connection lost or aborted\n");
			return false;
		}


		IWebPrintf ("NET_GetPacket: %s\n", strerror(e));
		return false;
	}

	if (ioctlsocket (clientsock, FIONBIO, &_true) == -1)
	{
		IWebPrintf ("FTP_ServerRun: blocking error: %s\n", strerror(neterrno()));
		return false;
	}
	cl = IWebMalloc(sizeof(FTPclient_t));
	if (!cl)	//iwebmalloc is allowed to fail.
	{
		char *msg = "421 Not enough memory is allocated.\r\n";	//don't be totally anti social
		send(clientsock, msg, strlen(msg), 0);
		closesocket(clientsock);	//try to forget this ever happend
		return true;
	}
	NET_SockadrToString(cl->peername, sizeof(cl->peername), &from, fromlen);
	IWebPrintf("%s: New FTP connection\n", cl->peername);
	//RFC1700
	if (((struct sockaddr *)&from)->sa_family == AF_INET)
		cl->controlaf = 1;
	else if (((struct sockaddr *)&from)->sa_family == AF_INET6)
		cl->controlaf = 2;
#ifdef USEIPX
	else if (((struct sockaddr *)&from)->sa_family == AF_IPX)
		cl->controlaf = 11;
#endif
	else
		cl->controlaf = 0;

	cl->controlsock = clientsock;
	cl->datasock = INVALID_SOCKET;
	cl->next = FTPclient;
	cl->blocking = false;
	strcpy(cl->path, "");

	QueueMessage(cl, "220-" FULLENGINENAME " FTP Server.\r\n220 Welcomes all new users.\r\n");

#if defined(WEBSVONLY) && defined(_WIN32)
	if (!CreateThread(NULL, 128, BlockingClient, cl, 0, NULL))
#endif
		FTPclient = cl;
	return true;
}

#endif
