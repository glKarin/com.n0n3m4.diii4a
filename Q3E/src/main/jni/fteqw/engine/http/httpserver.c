#include "quakedef.h"

#ifdef WEBSERVER

#include "iweb.h"

#include "netinc.h"

//FIXME: Before any admins use this for any serious usage, make the server send bits of file slowly.

static qboolean httpserverinitied = false;
qboolean httpserverfailed = false;
static int	httpserversocket;
static int	httpserverport;

#if 0//def WEBSVONLY
static int natpmpsocket = INVALID_SOCKET;
static int natpmptime;
#ifdef _WIN32
#include <mmsystem.h>
#endif
static void sendnatpmp(int port)
{
	struct sockaddr_in router;
	struct
	{
		qbyte ver;
		qbyte op;
		short reserved1;
		short privport; short pubport;
		int mapping_expectancy;
	} pmpreqmsg;

	int curtime = timeGetTime();
	if (natpmpsocket == INVALID_SOCKET)
	{
		unsigned long _true = true;
		natpmpsocket = socket(AF_INET, SOCK_CLOEXEC|SOCK_DGRAM, 0);
		if (natpmpsocket == INVALID_SOCKET)
			return;
		ioctlsocket (natpmpsocket, FIONBIO, &_true);
	}
	else if (curtime - natpmptime < 0)
		return;
	natpmptime = curtime+60*1000;

	memset(&router, 0, sizeof(router));
	router.sin_family = AF_INET;
	router.sin_port = htons(5351);
	router.sin_addr.S_un.S_un_b.s_b1 = 192;
	router.sin_addr.S_un.S_un_b.s_b2 = 168;
	router.sin_addr.S_un.S_un_b.s_b3 = 0;
	router.sin_addr.S_un.S_un_b.s_b4 = 1;

	pmpreqmsg.ver = 0;
	pmpreqmsg.op = 0;
	pmpreqmsg.reserved1 = htons(0);
	pmpreqmsg.privport = htons(port);
	pmpreqmsg.pubport = htons(port);
	pmpreqmsg.mapping_expectancy = htons(60*5);

	sendto(natpmpsocket, (void*)&pmpreqmsg, 2, 0, (struct sockaddr*)&router, sizeof(router));

	pmpreqmsg.op = 2;
	sendto(natpmpsocket, (void*)&pmpreqmsg, sizeof(pmpreqmsg), 0, (struct sockaddr*)&router, sizeof(router));
}
void checknatpmp(int port)
{
	struct
	{
		qbyte ver; qbyte op; short resultcode;
		int age;
		union
		{
			struct
			{
				short privport; short pubport;
				int mapping_expectancy;
			};
			qbyte ipv4[4];
		};
	} pmpreqrep;
	struct sockaddr_in from;
	int fromlen = sizeof(from);
	int len;
	static int oldip=-1;
	static short oldport;
	memset(&pmpreqrep, 0, sizeof(pmpreqrep));
	sendnatpmp(port);
	if (natpmpsocket == INVALID_SOCKET)
		len = -1;
	else
		len = recvfrom(natpmpsocket, (void*)&pmpreqrep, sizeof(pmpreqrep), 0, (struct sockaddr*)&from, &fromlen);
	if (len == 12 && pmpreqrep.op == 128)
	{
		if (oldip != *(int*)pmpreqrep.ipv4)
		{
			oldip = *(int*)pmpreqrep.ipv4;
			oldport = 0;
			IWebPrintf("Public ip is %i.%i.%i.%i\n", pmpreqrep.ipv4[0], pmpreqrep.ipv4[1], pmpreqrep.ipv4[2], pmpreqrep.ipv4[3]);
		}
	}
	else if (len == 16 && pmpreqrep.op == 129)
	{
		if (oldport != pmpreqrep.pubport)
		{
			oldport = pmpreqrep.pubport;
			IWebPrintf("Public udp port %i (local %i)\n", ntohs(pmpreqrep.pubport), ntohs(pmpreqrep.privport));
		}
	}
	else if (len == 16 && pmpreqrep.op == 130)
	{
		if (oldport != pmpreqrep.pubport)
		{
			oldport = pmpreqrep.pubport;
			IWebPrintf("Public tcp port %i (local %i)\n", ntohs(pmpreqrep.pubport), ntohs(pmpreqrep.privport));
		}
	}
}
#else
void checknatpmp(int port)
{
}
#endif

typedef enum {HTTP_WAITINGFORREQUEST,HTTP_SENDING} http_mode_t;


#ifndef _WIN32
static qboolean HTTP_DoAccepts(int epfd);
struct http_epoll_ctx
{
	struct epollctx_s pub;
	int epollfd;
};
static void HTTP_ServerEPolled(struct epollctx_s *pubctx, unsigned int ev)
{
	struct http_epoll_ctx *ctx = (struct http_epoll_ctx*)pubctx;
	if (!HTTP_DoAccepts(ctx->epollfd))
	{
		int e = neterrno();
		switch(e)
		{
		/*case NET_EWOULDBLOCK:
			break;
		case NET_ECONNABORTED:
		case NET_ECONNRESET:
			Con_TPrintf ("Connection lost or aborted\n");
			break;*/
		default:
			IWebPrintf ("NET_GetPacket: %s\n", strerror(e));
			break;
		}
	}
}
#endif

qboolean HTTP_ServerInit(int epfd, int port)
{
	struct sockaddr_qstorage address;
	unsigned long _true = true;
	int i;

	memset(&address, 0, sizeof(address));
	//check for interface binding option. this also forces ipv4, oh well.
	if ((i = COM_CheckParm("-ip")) != 0 && i < com_argc)
	{
		((struct sockaddr_in*)&address)->sin_addr.s_addr = inet_addr(com_argv[i+1]);
			Con_TPrintf("Binding to IP Interface Address of %s\n",
					inet_ntoa(((struct sockaddr_in*)&address)->sin_addr));


		((struct sockaddr_in*)&address)->sin_family = AF_INET;
		if (port != PORT_ANY)
			((struct sockaddr_in*)&address)->sin_port = htons((short)port);
	}
	else
	{	//otherwise just use ipv6
		((struct sockaddr_in6*)&address)->sin6_family = AF_INET6;
		if (port != PORT_ANY)
			((struct sockaddr_in6*)&address)->sin6_port = htons((short)port);
	}

	if ((httpserversocket = socket (((struct sockaddr*)&address)->sa_family, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		IWebPrintf ("HTTP_ServerInit: socket: %s\n", strerror(neterrno()));
		httpserverfailed = true;
		return false;
	}

	if (((struct sockaddr_in6*)&address)->sin6_family == AF_INET6 && !memcmp(((struct sockaddr_in6*)&address)->sin6_addr.s6_addr, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16))
	{	//in6addr_any, allow ipv4 too, if we can do hybrid sockets.
		unsigned long v6only = false;
		setsockopt(httpserversocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&v6only, sizeof(v6only));
	}

#ifdef SO_REUSEADDR
	{	//bypass TIME_WAIT (this is supposed to still fail if another process still has it bound)
		int reuse = true;
		setsockopt(httpserversocket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
	}
#endif

	if (ioctlsocket (httpserversocket, FIONBIO, &_true) == -1)
	{
		IWebPrintf ("HTTP_ServerInit: ioctl FIONBIO: %s\n", strerror(neterrno()));
		httpserverfailed = true;
		return false;
	}

	if (bind (httpserversocket, (void *)&address, sizeof(address)) == -1)
	{
		closesocket(httpserversocket);
		IWebPrintf("HTTP_ServerInit: failed to bind to socket\n");
		httpserverfailed = true;
		return false;
	}

	listen(httpserversocket, 3);

	httpserverinitied = true;
	httpserverfailed = false;
	httpserverport = port;

#ifndef _WIN32
	if (epfd >= 0)
	{
		static struct http_epoll_ctx ctx;
		struct epoll_event ev;
		ctx.epollfd = epfd;
		ctx.pub.Polled = HTTP_ServerEPolled;
		ev.events = EPOLLIN;
		ev.data.ptr = &ctx;
		epoll_ctl(epfd, EPOLL_CTL_ADD, httpserversocket, &ev);
	}
#endif

	IWebPrintf("HTTP server is running\n");
	return true;
}

void HTTP_ServerShutdown(void)
{
	closesocket(httpserversocket);
	IWebPrintf("HTTP server closed\n");

	httpserverinitied = false;
}

typedef struct HTTP_active_connections_s {
#ifndef _WIN32
	struct epollctx_s pub;
	int epfd;
#endif
	SOCKET datasock;
	char peername[256];
	vfsfile_t *file;
	struct HTTP_active_connections_s *next;

	http_mode_t mode;
	qboolean modeswitched;
	qboolean closeaftertransaction;
	qboolean acceptgzip;

	char *inbuffer;
	unsigned int inbuffersize;
	unsigned int inbufferused;

	char *outbuffer;
	unsigned int outbuffersize;
	unsigned int outbufferused;
} HTTP_active_connections_t;
static HTTP_active_connections_t *HTTP_ServerConnections;
static int httpconnectioncount;

static void ExpandInBuffer(HTTP_active_connections_t *cl, unsigned int quant, qboolean fixedsize)
{
	unsigned int newsize;
	if (fixedsize)
		newsize = quant;
	else
		newsize = cl->inbuffersize+quant;
	if (newsize <= cl->inbuffersize)
		return;

	cl->inbuffer = IWebRealloc(cl->inbuffer, newsize);
	cl->inbuffersize = newsize;
}
static void ExpandOutBuffer(HTTP_active_connections_t *cl, unsigned int quant, qboolean fixedsize)
{
	unsigned int newsize;
	if (fixedsize)
		newsize = quant;
	else
		newsize = cl->outbuffersize+quant;
	if (newsize <= cl->outbuffersize)
		return;

	cl->outbuffer = IWebRealloc(cl->outbuffer, newsize);
	cl->outbuffersize = newsize;
}

const char *HTTP_RunClient (HTTP_active_connections_t *cl)
{
	char *content;
	char *msg, *nl;
	char buf2[2560];	//short lived temp buffer.
	char resource[2560], *args;
	char host[256];
	char mode[80];
	qboolean hostspecified;
	unsigned int contentlen;

	int HTTPmarkup;	//version
	int localerrno;

	int ammount, wanted;
	int matchetag;

	switch(cl->mode)
	{
	case HTTP_WAITINGFORREQUEST:
		if (cl->outbufferused)
			Sys_Error("Persistant connection was waiting for input with unsent output");
		ammount = cl->inbuffersize-1 - cl->inbufferused;
		if (ammount < 128)
		{
			if (cl->inbuffersize>128*1024)
				return "headers larger than 128kb";	//that's just taking the piss.

			ExpandInBuffer(cl, 1500, false);
			ammount = cl->inbuffersize-1 - cl->inbufferused;
		}
		if (cl->modeswitched)
		{
			ammount = 0;
		}
		else
		{
			//we can't try and recv 0 bytes as we use an expanding buffer
			ammount = recv(cl->datasock, cl->inbuffer+cl->inbufferused, ammount, 0);
			if (ammount < 0)
			{
				int e = neterrno();
				if (e == NET_EWOULDBLOCK)	//they closed on us. Assume end.
					return NULL;
				return "recv error";
			}
			if (ammount == 0)
				return "peer closed connection";
		}
		cl->modeswitched = false;

		cl->inbufferused += ammount;
		cl->inbuffer[cl->inbufferused] = '\0';

		content = NULL;
		msg = cl->inbuffer;
		nl = strchr(msg, '\n');
		if (!nl)
			return NULL;	//we need more... MORE!!! MORE I TELL YOU!!!!

		//FIXME: COM_ParseOut recognises // comments, which is not correct.
		msg = COM_ParseOut(msg, mode, sizeof(mode));

		msg = COM_ParseOut(msg, resource, sizeof(resource));

		if (!*resource)
			return "not http";	//even if they forgot to specify a resource, we didn't find an HTTP so we have no option but to close.

		host[0] = '?';
		host[1] = 0;

		hostspecified = false;
		if (!strnicmp(resource, "http://", 7))
		{	//groan... 1.1 compliance requires parsing this correctly, without the client ever specifiying it.
			char *slash;	//we don't do multiple hosts.
			hostspecified=true;
			slash = strchr(resource+7, '/');
			if (!slash)
				strcpy(resource, "/");
			else
			{
				int hlen = slash-(resource+7);
				if (hlen > sizeof(host)-1)
					hlen = sizeof(host)-1;
				memcpy(host, resource+7, hlen);
				host[hlen] = 0;
				memmove(resource, slash, strlen(slash+1));	//just get rid of the http:// stuff.
			}
		}

		args = strchr(resource, '?');
		if (args)
			*args++=0;

		if (!strcmp(resource, "/"))
			strcpy(resource, "/index.html");

		cl->acceptgzip = false;

		msg = COM_ParseOut(msg, buf2, sizeof(buf2));
		contentlen = 0;
		matchetag = 0;
		if (!strnicmp(buf2, "HTTP/", 5))
		{
			if (!strncmp(buf2, "HTTP/1.1", 8))
			{
				HTTPmarkup = 3;
				cl->closeaftertransaction = false;
			}
			else if (!strncmp(buf2, "HTTP/1", 6))
			{
				HTTPmarkup = 2;
				cl->closeaftertransaction = true;
			}
			else
			{
				HTTPmarkup = 1;	//0.9... lamer.
				cl->closeaftertransaction = true;
			}

			//expect X lines containing options.
			//then a blank line. Don't continue till we have that.

			msg = nl+1;
			while (1)
			{
				if (*msg == '\r')
					msg++;
				if (*msg == '\n')
				{
					msg++;
					break;	//that was our blank line.
				}

				while(*msg == ' ')
					msg++;

				if (!strnicmp(msg, "Host: ", 6))	//parse needed header fields
				{
					int l = 0;
					msg += 6;
					while (*msg == ' ' || *msg == '\t')
						msg++;
					while (*msg != '\r' && *msg != '\n' && l < sizeof(host)-1)
						host[l++] = *msg++;
					host[l] = 0;
					hostspecified = true;
				}
				else if (!strnicmp(msg, "Content-Length: ", 16))	//parse needed header fields
					contentlen = strtoul(msg+16, NULL, 0);
				else if (!strnicmp(msg, "Accept-Encoding:", 16))	//parse needed header fields
				{
					char *e;
					msg += 16;
					while(*msg)
					{
						if (*msg == '\n')
							break;
						while (*msg == ' ' || *msg == '\t')
							msg++;
						if (*msg == ',')
						{
							msg++;
							continue;
						}
						e = msg;
						while(*e)
						{
							if (*e == ',' || *e == '\r' || *e == '\n')
								break;
							e++;
						}
						while(e > msg && (e[-1] == ' ' || e[-1] == '\t'))
							e--;
						if (e-msg == 4 && !strncmp(msg, "gzip", 4))
							cl->acceptgzip = true;
						while(*msg && *msg != '\n' && *msg != ',')
							msg++;
					}
				}
				else if (!strnicmp(msg, "If-None-Match:", 14))
				{
					msg += 14;
					while(*msg)
					{
						if (*msg == ' ' || *msg == ',')
							msg++;
						else if (*msg == '\"')
						{
							msg++;
							matchetag = strtoul(msg, &msg, 16);
							if (*msg == '\"')
								msg++;
							else
							{
								matchetag=0;	//something went wrong.
								break;
							}
						}
						else
							break;
					}
				}
				else if (!strnicmp(msg, "Transfer-Encoding: ", 18))	//parse needed header fields
				{
					cl->closeaftertransaction = true;
					goto notimplemented;
				}
				else if (!strnicmp(msg, "Connection: close", 17))
					cl->closeaftertransaction = true;
				else if (!strnicmp(msg, "Connection: Keep-Alive", 22))
					cl->closeaftertransaction = false;

				while(*msg != '\n')
				{
					if (!*msg)
					{
						if (msg == cl->inbuffer+cl->inbufferused)
							return NULL;
						else
							return "Unexpected null byte";
					}
					msg++;
				}
				msg++;
			}
		}
		else
		{
			HTTPmarkup = 0;	//strimmed... totally...
			cl->closeaftertransaction = true;
			//don't bother running to nl.
		}

		if (cl->inbufferused-(msg-cl->inbuffer) < contentlen)
			return NULL;

		cl->modeswitched = true;

		if (contentlen)
		{
			content = IWebMalloc(contentlen+1);
			memcpy(content, msg, contentlen+1);
		}

		memmove(cl->inbuffer, cl->inbuffer+(msg-cl->inbuffer+contentlen), cl->inbufferused-(msg-cl->inbuffer+contentlen));
		cl->inbufferused -= msg-cl->inbuffer+contentlen;


		if (HTTPmarkup == 3 && !hostspecified)	//1.1 requires the host to be specified... we ca,just ignore it as we're not routing or imitating two servers. (for complience we need to encourage the client to send - does nothing for compatability or anything, just compliance to spec. not always the same thing)
		{
			msg = "HTTP/1.1 400 Bad Request\r\n"	/*"Content-Type: application/octet-stream\r\n"*/		"Content-Length: 69\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n"	"400 Bad Request\r\nYour client failed to provide the host header line";

			IWebPrintf("%s: no host specified\n", cl->peername);

			ammount = strlen(msg);
			ExpandOutBuffer(cl, ammount, true);
			memcpy(cl->outbuffer, msg, ammount);
			cl->outbufferused = ammount;
			cl->mode = HTTP_SENDING;
		}
		else if (!stricmp(mode, "GET") || !stricmp(mode, "HEAD") || !stricmp(mode, "POST"))
		{
			time_t timestamp = 0;
			qboolean gzipped = false;
			if (*resource != '/')
			{
				resource[0] = '/';
				resource[1] = 0;	//I'm lazy, they need to comply
			}
			IWebPrintf("%s: Download request for \"http://%s/%s\"\n", cl->peername, host, resource+1);

			if (!strnicmp(mode, "P", 1))	//when stuff is posted, data is provided. Give an error message if we couldn't do anything with that data.
				cl->file = IWebGenerateFile(resource+1, content, contentlen);
			else
			{
				char filename[MAX_OSPATH], *q;
				cl->file = NULL;
				Q_strncpyz(filename, resource+1, sizeof(filename));
				q = strchr(filename, '?');
				if (q) *q = 0;

				if (SV_AllowDownload(filename))
				{
					char nbuf[MAX_OSPATH];
					flocation_t loc = {NULL};

					if (cl->acceptgzip && strlen(filename) < sizeof(nbuf)-4)
					{
						Q_strncpyz(nbuf, filename, sizeof(nbuf));
						Q_strncatz(nbuf, ".gz", sizeof(nbuf));
						gzipped = !!FS_FLocateFile(nbuf, FSLF_IFFOUND|FSLF_DONTREFERENCE, &loc);
					}
					else
						gzipped = false;
					if (gzipped || FS_FLocateFile(filename, FSLF_IFFOUND|FSLF_DONTREFERENCE, &loc))
					{
						FS_GetLocMTime(&loc, &timestamp);
						cl->file = FS_OpenReadLocation(NULL, &loc);
					}
					else
						cl->file = NULL;
				}

				if (!cl->file)
				{
					cl->file = IWebGenerateFile(resource+1, content, contentlen);
				}
			}
			if (!cl->file)
			{
				IWebPrintf("%s: 404 - not found\n", cl->peername);

				if (HTTPmarkup >= 3)
					msg = "HTTP/1.1 404 Not Found\r\n"	"Content-Type: text/plain\r\n"		"Content-Length: 15\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n"	"404 Bad address";
				else if (HTTPmarkup == 2)
					msg = "HTTP/1.0 404 Not Found\r\n"	"Content-Type: text/plain\r\n"		"Content-Length: 15\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n"	"404 Bad address";
				else if (HTTPmarkup)
					msg = "HTTP/0.9 404 Not Found\r\n"																						"\r\n"	"404 Bad address";
				else
					msg = "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY>404 Not Found<BR>The specified file could not be found on the server</HEAD></HTML>";

				ammount = strlen(msg);
				ExpandOutBuffer(cl, ammount, true);
				memcpy(cl->outbuffer, msg, ammount);
				cl->outbufferused = ammount;
				cl->mode = HTTP_SENDING;
			}
			else
			{
				const char *mimeline;
				char modifiedline[128];
				if (strstr(resource, ".htm"))
					mimeline = "Content-Type: text/html\r\n";
				else if (strstr(resource, ".wasm"))
					mimeline = "Content-Type: application/wasm\r\n";
				else if (strstr(resource, ".js"))
					mimeline = "Content-Type: text/javascript\r\n";
//				else if (strstr(resource, ".mvd"))
//					mimeline = "Content-Type: application/x-multiviewdemo\r\nAccess-Control-Allow-Origin: *\r\n";
/*
				else if (strstr(resource, ".fmf"))
					mimeline = "Content-Type: application/x-multiviewdemo\r\nAccess-Control-Allow-Origin: *\r\n";
				else if (strstr(resource, ".pak"))
					mimeline = "Content-Type: application/x-multiviewdemo\r\nAccess-Control-Allow-Origin: *\r\n";
				else if (strstr(resource, ".pk3"))
					mimeline = "Content-Type: application/x-multiviewdemo\r\nAccess-Control-Allow-Origin: *\r\n";*/
				else
					mimeline = "";

				if (timestamp)
				{
					strftime(modifiedline, sizeof(modifiedline), "Last-Modified: %a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&timestamp));
					Q_snprintfz(modifiedline+strlen(modifiedline), sizeof(modifiedline)-strlen(modifiedline), "ETag: \"%x\"\r\n", (unsigned int)timestamp);
				}
				else if (gzipped)
					Q_snprintfz(modifiedline+strlen(modifiedline), sizeof(modifiedline)-strlen(modifiedline), "Cache-Control: public, max-age=86400\r\n");
				else
					*modifiedline = 0;

				//fixme: add connection: keep-alive or whatever so that ie3 is happy...
				if (HTTPmarkup >= 3 && matchetag && matchetag==(unsigned int)timestamp)
				{
					sprintf(resource, "HTTP/1.1 304 Not Modified\r\n"	"%s%s%s"		"Connection: %s\r\n"	/*"Content-Length: %i\r\n"*/	"Server: "FULLENGINENAME"/0\r\n"	"\r\n", modifiedline, mimeline, gzipped?"Content-Encoding: gzip\r\n":"", cl->closeaftertransaction?"close":"keep-alive"/*, (int)VFS_GETLEN(cl->file)*/);
					if (cl->file)
					{	//don't send any actual data...
						VFS_CLOSE(cl->file);
						cl->file = NULL;
					}
					IWebPrintf("%s:   Not Modified\n", cl->peername);
				}
				else if (HTTPmarkup>=3)
					sprintf(resource, "HTTP/1.1 200 OK\r\n"				"%s%s%s"		"Connection: %s\r\n"	"Content-Length: %i\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n", modifiedline, mimeline, gzipped?"Content-Encoding: gzip\r\n":"", cl->closeaftertransaction?"close":"keep-alive", (int)VFS_GETLEN(cl->file));
				else if (HTTPmarkup==2)
					sprintf(resource, "HTTP/1.0 200 OK\r\n"				"%s%s%s"		"Connection: %s\r\n"	"Content-Length: %i\r\n"	"Server: "FULLENGINENAME"/0\r\n"	"\r\n", modifiedline, mimeline, gzipped?"Content-Encoding: gzip\r\n":"", cl->closeaftertransaction?"close":"keep-alive", (int)VFS_GETLEN(cl->file));
				else if (HTTPmarkup)
					sprintf(resource, "HTTP/0.9 200 OK\r\n\r\n");
				else
					strcpy(resource, "");
				msg = resource;

				if ((*mode == 'H' || *mode == 'h') && cl->file)
				{	//'head'
					VFS_CLOSE(cl->file);
					cl->file = NULL;
				}

				ammount = strlen(msg);
				ExpandOutBuffer(cl, ammount, true);
				memcpy(cl->outbuffer, msg, ammount);
				cl->outbufferused = ammount;
				cl->mode = HTTP_SENDING;
			}
		}
		//PUT/POST must support chunked transfer encoding for 1.1 compliance.
/*			else if (!stricmp(mode, "PUT"))	//put is replacement of a resource. (file uploads)
		{
		}
*/
		else
		{
notimplemented:
			if (HTTPmarkup >= 3)
				msg = "HTTP/1.1 501 Not Implemented\r\n\r\n";
			else if (HTTPmarkup == 2)
				msg = "HTTP/1.0 501 Not Implemented\r\n\r\n";
			else if (HTTPmarkup)
				msg = "HTTP/0.9 501 Not Implemented\r\n\r\n";
			else
			{
				msg = NULL;
				return "unsupported http version";
			}
			IWebPrintf("%s: 501 - not implemented\n", cl->peername);

			if (msg)
			{
				ammount = strlen(msg);
				ExpandOutBuffer(cl, ammount, true);
				memcpy(cl->outbuffer, msg, ammount);
				cl->outbufferused = ammount;
				cl->mode = HTTP_SENDING;
			}
		}

		if (content)
			IWebFree(content);
		break;

	case HTTP_SENDING:
		if (cl->outbufferused < 8192)
		{
			if (cl->file)
			{
				ExpandOutBuffer(cl, 32768, true);
				wanted = cl->outbuffersize - cl->outbufferused;
				ammount = VFS_READ(cl->file, cl->outbuffer+cl->outbufferused, wanted);

				if (!ammount)
				{
					VFS_CLOSE(cl->file);
					cl->file = NULL;

					IWebPrintf("%s: Download complete\n", cl->peername);
				}
				else
					cl->outbufferused+=ammount;
			}
		}

		ammount = send(cl->datasock, cl->outbuffer, cl->outbufferused, 0);

		if (ammount == -1)
		{
			localerrno = neterrno();
			if (localerrno != NET_EWOULDBLOCK)
				return "some error when sending";
		}
		else if (ammount||!cl->outbufferused)
		{
			memcpy(cl->outbuffer, cl->outbuffer+ammount, cl->outbufferused-ammount);
			cl->outbufferused -= ammount;
			if (!cl->outbufferused && !cl->file)
			{
				cl->modeswitched = true;
				cl->mode = HTTP_WAITINGFORREQUEST;
				if (cl->closeaftertransaction)
					return "file sent";
			}
		}
		else
			return "peer prematurely closed connection";
		break;

/*	case HTTP_RECEIVING:
		sent = recv(cl->datasock, resource, ammount, 0);
		if (sent == -1)
		{
			if (qerrno != EWOULDBLOCK)	//they closed on us. Assume end.
			{
				VFS_CLOSE(cl->file);
				cl->file = NULL;
				cl->close = true;
				continue;
			}
		}
		if (sent != 0)
			IWebFWrite(resource, 1, sent, cl->file);
		break;*/
	default:
		return "Unknown state";
	}
	return NULL;
}

void HTTP_RunExisting (void)
{
	const char *err;
	HTTP_active_connections_t **link, *cl;

	link = &HTTP_ServerConnections;
	for (link = &HTTP_ServerConnections; *link;)
	{
		cl = *link;

		err = HTTP_RunClient(cl);
		if (err)
		{
			IWebPrintf("%s: Closing connection: %s\n", cl->peername, err);

			*link = cl->next;
			closesocket(cl->datasock);
			cl->datasock = INVALID_SOCKET;
			if (cl->inbuffer)
				IWebFree(cl->inbuffer);
			if (cl->outbuffer)
				IWebFree(cl->outbuffer);
			if (cl->file)
				VFS_CLOSE(cl->file);
			IWebFree(cl);
			httpconnectioncount--;
			continue;
		}
		link = &(*link)->next;
	}
}

#ifndef _WIN32
static void HTTP_ClientEPolled(struct epollctx_s *pubctx, unsigned int ev)
{
	HTTP_active_connections_t *cl = (HTTP_active_connections_t *)pubctx;
	const char *err = HTTP_RunClient(cl);

	if (err)
	{
		IWebPrintf("%s: Closing connection: %s\n", cl->peername, err);

		//should be automatic thanks to the closesocket, but do it just in case.
		epoll_ctl(cl->epfd, EPOLL_CTL_DEL, cl->datasock, NULL);

		closesocket(cl->datasock);
		cl->datasock = INVALID_SOCKET;
		if (cl->inbuffer)
			IWebFree(cl->inbuffer);
		if (cl->outbuffer)
			IWebFree(cl->outbuffer);
		if (cl->file)
			VFS_CLOSE(cl->file);
		IWebFree(cl);
		httpconnectioncount--;
	}
	else
	{
		struct epoll_event ev;
		switch(cl->mode)
		{
		case HTTP_WAITINGFORREQUEST:
			ev.events = EPOLLIN;
			break;
		case HTTP_SENDING:
			ev.events = EPOLLOUT;
			break;
		default:	//don't know, spam it.
			ev.events = EPOLLIN|EPOLLOUT;
			break;
		}
		ev.data.ptr = &cl->pub;
		epoll_ctl(cl->epfd, EPOLL_CTL_MOD, cl->datasock, &ev);
	}
}
#endif

static qboolean HTTP_DoAccepts(int epfd)
{
	struct sockaddr_qstorage	from;
	int fromlen = sizeof(from);
	HTTP_active_connections_t *cl;
	int _true = true;
	int clientsock = accept(httpserversocket, (struct sockaddr *)&from, &fromlen);

	if (clientsock < 0)
		return false;

	if (ioctlsocket (clientsock, FIONBIO, (u_long *)&_true) == -1)
	{
		IWebPrintf ("HTTP_ServerInit: ioctl FIONBIO: %s\n", strerror(neterrno()));
		closesocket(clientsock);
		return false;
	}

	cl = IWebMalloc(sizeof(HTTP_active_connections_t));
	NET_SockadrToString(cl->peername, sizeof(cl->peername), &from, sizeof(from));
	IWebPrintf("%s: New http connection\n", cl->peername);

	cl->datasock = clientsock;

#ifndef _WIN32
	if (epfd >= 0)
	{
		struct epoll_event ev;
		cl->pub.Polled = HTTP_ClientEPolled;
		cl->epfd = epfd;
		ev.events = EPOLLIN|EPOLLOUT;
		ev.data.ptr = &cl->pub;
		epoll_ctl(epfd, EPOLL_CTL_ADD, clientsock, &ev);
	}
	else
#endif
	{
		cl->next = HTTP_ServerConnections;
		HTTP_ServerConnections = cl;
	}
	httpconnectioncount++;
	return true;
}

qboolean HTTP_ServerPoll(qboolean httpserverwanted, int portnum)	//loop while true
{
	if (httpserverport != portnum && httpserverinitied)
		HTTP_ServerShutdown();
	if (!httpserverinitied)
	{
		if (httpserverwanted)
			return HTTP_ServerInit(-1, portnum);
		return false;
	}
	else if (!httpserverwanted)
	{
		HTTP_ServerShutdown();
		return false;
	}

	checknatpmp(httpserverport);

	if (httpconnectioncount>32)
		return false;

	if (!HTTP_DoAccepts(-1))
	{
		int e = neterrno();
		if (e == NET_EWOULDBLOCK)
		{
			HTTP_RunExisting();
			return false;
		}

		if (e == NET_ECONNABORTED || e == NET_ECONNRESET)
		{
			Con_TPrintf ("Connection lost or aborted\n");
			return false;
		}


		IWebPrintf ("NET_GetPacket: %s\n", strerror(e));
		return false;
	}
	return true;
}

#endif
