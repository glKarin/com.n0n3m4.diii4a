#include "quakedef.h"

#include "netinc.h"
#include "fs.h"

#if defined(WEBCLIENT)
static struct dl_download *activedownloads;

#if defined(FTE_TARGET_WEB)

vfsfile_t *FSWEB_OpenTempHandle(int f);

static void DL_Cancel(struct dl_download *dl)
{
	//FIXME: clear out the callbacks somehow
	dl->ctx = NULL;
}
static void DL_OnLoad(void *c, int buf)
{
	//also fires from 404s.
	struct dl_download *dl = c;
	vfsfile_t *tempfile = FSWEB_OpenTempHandle(buf);
	//make sure the file is 'open'.
	if (!dl->file)
	{
		if (*dl->localname)
		{
			FS_CreatePath(dl->localname, dl->fsroot);
			dl->file = FS_OpenVFS(dl->localname, "w+b", dl->fsroot);
		}
		else
		{
			dl->file = tempfile;
			tempfile = NULL;
		}
	}

	if (dl->file)
	{
		dl->status = DL_FINISHED;
		if (tempfile)
		{
			qofs_t datasize = VFS_GETLEN(tempfile);
			char *data = malloc(datasize);	//grab a temp buffer so we can do the entire file at once...
			if (!data)
				dl->status = DL_FAILED;
			else
			{
				VFS_READ(tempfile, data, datasize);

				VFS_WRITE(dl->file, data, datasize);
				if (dl->file->seekstyle < SS_PIPE)
					VFS_SEEK(dl->file, 0);

				free(data);
			}
		}
	}
	else
		dl->status = DL_FAILED;

	if (tempfile)
		VFS_CLOSE(tempfile);

	dl->replycode = 200;
}
static void DL_OnError(void *c, int ecode)
{
	struct dl_download *dl = c;
	//fires from cross-domain blocks, tls errors, etc.
	//anything which doesn't yield an http response (404 is NOT an error as far as js is aware).

	dl->replycode = ecode;
	Con_Printf(CON_WARNING"dl error(%i): %s\n", ecode, dl->url);
	dl->status = DL_FAILED;
}
static void DL_OnProgress(void *c, int position, int totalsize)
{
	struct dl_download *dl = c;

	dl->completed = position;
	dl->totalsize = totalsize;
}

//this becomes a poll function. the main thread will call this once a frame or so.
qboolean DL_Decide(struct dl_download *dl)
{
	const char *url = dl->redir;
	if (!*url)
		url = dl->url;
	if (dl->postdata)
	{	//not supported...
		DL_Cancel(dl);
		return false;	//safe to destroy it now
	}

	if (dl->ctx)
	{
		if (dl->status == DL_FINISHED)
			return false;
		if (dl->status == DL_FAILED)
		{
			DL_Cancel(dl);
			return false;
		}
	}
	else if (dl->status == DL_PENDING)
	{
		dl->status = DL_ACTIVE;
		dl->abort = DL_Cancel;
		dl->ctx = dl;

		emscriptenfte_async_wget_data2(url, dl, DL_OnLoad, DL_OnError, DL_OnProgress);
	}
	else if (dl->status == DL_ACTIVE)
	{	//canceled?
		dl->status = DL_FAILED;
		return false;
	}

	return true;
}
#else
qboolean DL_Decide(struct dl_download *dl);

/*
This file does one thing. Connects to servers and grabs the specified file. It doesn't do any uploading whatsoever. Live with it.
It doesn't use persistant connections.

*/

#define COOKIECOOKIECOOKIE
#ifdef COOKIECOOKIECOOKIE
typedef struct cookie_s
{
	struct cookie_s *next;
	char *domain;
	int secure;
	char *name;
	char *value;
} cookie_t;
static cookie_t *cookies;


//set a specific cookie.
void Cookie_Feed(char *domain, int secure, char *name, char *value)
{
	cookie_t **link, *c;
	Sys_LockMutex(com_resourcemutex);
	for(link = &cookies; (c=*link)!=NULL; link = &(*link)->next)
	{
		if (!strcmp(c->domain, domain) && c->secure == secure && !strcmp(c->name, name))
			break;
	}
	//delete it, if it exists, so we can create it anew.
	if (c)
	{
		*link = c->next;
		Z_Free(c);
	}
	if (value && *value)
	{
//		Con_Printf("Setting cookie http%s://%s/ %s=%s\n", secure?"s":"", domain, name, value);
		c = Z_Malloc(sizeof(*c) + strlen(domain) + strlen(name) + strlen(value) + 3);
		c->domain = (char*)(c+1);
		strcpy(c->domain, domain);
		c->secure = secure;
		c->name = c->domain+strlen(c->domain)+1;
		strcpy(c->name, name);
		c->value = c->name+strlen(c->name)+1;
		strcpy(c->value, value);
		c->next = cookies;
		cookies = c;
	}
	else
	{
//		Con_Printf("Deleted cookie http%s://%s/ %s\n", secure?"s":"", domain, name);
	}
	Sys_UnlockMutex(com_resourcemutex);
}

//just removes all the cookies it can.
void Cookie_Monster(void)
{
	cookie_t *c;
	while (cookies)
	{
		c = cookies;
		cookies = c->next;
		Z_Free(c);
	}
}

//parses Set-Cookie: THISPARTONLY\r\n
//we don't support:
//domain) we don't have a list of public suffixes, like .co.uk, and thus cannot safely block dangerous super-cookies. thus we require the exact same host each time
//path) I'm going to call this an optimisation feature and not bother with it... hopefully there won't be too many sites that have sub-paths or third-party stuff... gah.
//httponly) irrelevant until we support javascript... which we don't.
//secure) assumed to be true. https:// vs http:// are thus completely independant. sorry.
//expires) gah, parsing time values sucks! plus we don't have persistent storage. All cookies are session cookies.
void Cookie_Parse(char *domain, int secure, char *line, char *end)
{
	char *e;
	while (*line == ' ' && line < end)
		line++;
	for (e = line; e < end; e++)
		if (*e == ';')
			end = e;

	for (e = line; e < end; e++)
		if (*e == '=')
			break;

	*e = 0;
	*end = 0;
	Cookie_Feed(domain, secure, line, e+1);
}
//outputs a complete http line: Cookie: a=v1; b=v2\r\n
void Cookie_Regurgitate(char *domain, int secure, char *buffer, size_t buffersize)
{
	qboolean hascookies = false;
	cookie_t *c;
//	char *l = buffer;
	buffersize -= 3;	//\r\n\0
	*buffer = 0;
	Sys_LockMutex(com_resourcemutex);
	for (c = cookies; c; c = c->next)
	{
		if (!strcmp(c->domain, domain) && c->secure == secure)
		{
			int nlen,vlen;
			if (!hascookies)
			{
				if (buffersize < 8)
					break;
				strcpy(buffer, "Cookie: ");
				buffersize -= 8;
				hascookies=true;
			}
			else
			{
				if (buffersize < 2)
					break;
				strcpy(buffer, "; ");
				buffersize -= 2;
			}
			buffer += strlen(buffer);

			nlen = strlen(c->name);
			vlen = strlen(c->value);
			
			if (buffersize < nlen+1+vlen)
				break;
			memcpy(buffer, c->name, nlen);
			buffer += nlen;
			*buffer++ = '=';
			memcpy(buffer, c->value, vlen);
			buffer += vlen;
		}
	}
	Sys_UnlockMutex(com_resourcemutex);

	if (hascookies)
		strcpy(buffer, "\r\n");
	else
		*buffer = 0;

//	if (*l)
//		Con_Printf("Sending cookie(s) to http%s://%s/ %s\n", secure?"s":"", domain, l);
}
#endif

struct http_dl_ctx_s {
//	struct dl_download *dlctx;

	vfsfile_t *stream;
	SOCKET sock;	//so we can wait on it when multithreaded.

	char *buffer;

	char server[128];
	qboolean secure;

	size_t bufferused;
	size_t bufferlen;

	size_t totalreceived;	//useful when we're just dumping to a file.

	struct vfsfile_s *file;	//if gzipping, this is a temporary file. we'll write to the real file from this after the transfer is complete.
	qboolean gzip;
	qboolean chunking;
	size_t chunksize;
	size_t chunked;

	enum {HC_REQUESTING, HC_GETTINGHEADER, HC_GETTING} state;

	size_t contentlength;
};

void HTTP_Cleanup(struct dl_download *dl)
{
	struct http_dl_ctx_s *con = dl->ctx;
	dl->ctx = NULL;

	if (con->stream)
		VFS_CLOSE(con->stream);
	con->stream = NULL;
	free(con->buffer);
	free(con);

	dl->abort = NULL;
	dl->status = DL_PENDING;
	dl->completed = 0;
	dl->totalsize = 0;
}

static void ExpandBuffer(struct http_dl_ctx_s *con, int quant)
{
	int newlen;
	newlen = con->bufferlen + quant;
	con->buffer = realloc(con->buffer, newlen);
	con->bufferlen = newlen;
}

static int VFSError_To_HTTP(int vfserr)
{
	switch(vfserr)
	{
	case VFS_ERROR_TRYLATER:
		return 0;
	default:
	case VFS_ERROR_UNSPECIFIED:
		return 0;	//don't know, no reason given.
	case VFS_ERROR_REFUSED:
		return HTTP_REFUSED;
	case VFS_ERROR_EOF:
		return HTTP_EOF;
	case VFS_ERROR_DNSFAILURE:
		return HTTP_DNSFAILURE;
	case VFS_ERROR_WRONGCERT:
		return HTTP_MITM;
	case VFS_ERROR_UNTRUSTED:
		return HTTP_UNTRUSTED;
	}
}

#ifdef HAVE_EPOLL
#include <poll.h>
#endif
static qboolean HTTP_DL_Work(struct dl_download *dl)
{
	struct http_dl_ctx_s *con = dl->ctx;
	char buffer[256];
	char Location[4096];
	char mimetype[256];
	char *nl;
	char *msg;
	int ammount;
	qboolean transfercomplete = false;

#ifdef MULTITHREAD
	//if we're running in a thread, wait for some actual activity instead of busylooping like an moron.
	if (dl->threadctx)
	{
#ifdef HAVE_EPOLL
		struct pollfd fd;
		fd.fd = con->sock;
		fd.events = POLLIN;
		fd.revents = 0;
		if (con->state == HC_REQUESTING)
			fd.events |= POLLOUT;
		poll(&fd, 1, 0.1*1000);	//wake up when we can read OR write
		//note that https should wake up more often, but we don't want to wake up because we *can* write when we're reading without any need to write.
#else
		struct timeval timeout;
		fd_set	rdset, wrset;
		FD_ZERO(&wrset);
		FD_ZERO(&rdset);
		FD_SET(con->sock, &wrset); // network socket
		FD_SET(con->sock, &rdset); // network socket
		timeout.tv_sec = 0;
		timeout.tv_usec = 0.1*1000*1000;
		if (con->state == HC_REQUESTING)
			select(con->sock+1, &rdset, &wrset, NULL, &timeout);	//wake up when we can read OR write
		else
			select(con->sock+1, &rdset, NULL, NULL, &timeout);	//wake when we can read.
		//note that https should wake up more often, but we don't want to wake up because we *can* write when we're reading without any need to write.
#endif
	}
#endif

	switch(con->state)
	{
	case HC_REQUESTING:

		ammount = VFS_WRITE(con->stream, con->buffer, con->bufferused);
		if (!ammount)
			return true;
		if (ammount < 0)
		{
			dl->status = DL_FAILED;
			dl->replycode = VFSError_To_HTTP(ammount);
			return false;
		}

		con->bufferused -= ammount;
		memmove(con->buffer, con->buffer+ammount, con->bufferused);
		if (!con->bufferused)	//that's it, all sent.
			con->state = HC_GETTINGHEADER;
		break;

	case HC_GETTINGHEADER:
		if (con->bufferlen - con->bufferused < 1530)
			ExpandBuffer(con, 1530);

		ammount = VFS_READ(con->stream, con->buffer+con->bufferused, con->bufferlen-con->bufferused-15);
		if (!ammount)
			return true;
		if (ammount < 0)
		{
			dl->status = DL_FAILED;
			dl->replycode = VFSError_To_HTTP(ammount);
			return false;
		}

		con->bufferused+=ammount;
		con->buffer[con->bufferused] = '\0';
		//have we got the entire thing yet?

		msg = con->buffer;
		con->chunking = false;
		con->contentlength = -1;	//means unspecified
		con->gzip = false;
		*mimetype = 0;
		*Location = 0;
		if (strnicmp(msg, "HTTP/", 5))
		{	//pre version 1 (lame servers). no response headers at all.
			con->state = HC_GETTING;
			dl->status = DL_ACTIVE;
			dl->replycode = 200;
		}
		else
		{
			//check if the headers are complete or not
			qboolean hcomplete = false;
			while(*msg)
			{
				if (*msg == '\n')
				{
					if (msg[1] == '\n')
					{	//tut tut, not '\r'? that's not really allowed...
						msg+=2;
						hcomplete = true;
						break;
					}
					if (msg[1] == '\r' && msg[2] == '\n')
					{
						msg+=3;
						hcomplete = true;
						break;
					}
					msg++;
				}
				while (*msg == ' ' || *msg == '\t')
					msg++;

				nl = strchr(msg, '\n');
				if (!nl)
					break;//not complete, don't bother trying to parse it.
				msg = nl;
			}

			if (!hcomplete)
				break;//headers not complete. break out of switch

			//okay, they're complete, woot. we need to actually go through the headers now
			msg = con->buffer;
			while(*msg)
			{
				if (*msg == '\n')
				{
					if (msg[1] == '\n')
					{	//tut tut, not '\r'? that's not really allowed...
						msg+=2;
						break;
					}
					if (msg[1] == '\r' && msg[2] == '\n')
					{
						msg+=3;
						break;
					}
					msg++;
				}
				while (*msg == ' ' || *msg == '\t')
					msg++;

				nl = strchr(msg, '\n');
				if (!nl)
					break;//not complete, don't bother trying to parse it.

				if (!strnicmp(msg, "Content-Length: ", 16))
					con->contentlength = atoi(msg+16);
				else if (!strnicmp(msg, "Content-Type:", 13))
				{
					*nl = '\0';
					//don't worry too much about truncation. its not like we can really do much with fancy mime types anyway.
					COM_TrimString(msg+13, mimetype, sizeof(mimetype));
					*nl = '\n';
				}
				else if (!strnicmp(msg, "Location: ", 10))
				{
					*nl = '\0';
					if (!COM_TrimString(msg+10, Location, sizeof(Location)))
					{
						Con_Printf("HTTP Redirect: location too long\n");
						dl->status = DL_FAILED;
						return false;
					}
					*nl = '\n';
				}
				else if (!strnicmp(msg, "Content-Encoding: ", 18))
				{
					char *chunk = strstr(msg, "gzip");
					if (chunk < nl)
						con->gzip = true;
				}
				else if (!strnicmp(msg, "Transfer-Encoding: ", 19))
				{
					char *chunk = strstr(msg, "chunked");
					if (chunk < nl)
						con->chunking = true;
				}
#ifdef COOKIECOOKIECOOKIE
				else if (!strnicmp(msg, "Set-Cookie: ", 12))
				{
					Cookie_Parse(con->server, con->secure, msg+12, nl);
				}
#endif
				msg = nl;
			}
			if (!hcomplete)
				break;//headers not complete. break out of switch

			ammount = msg - con->buffer;

			msg = COM_ParseOut(con->buffer, buffer, sizeof(buffer));
			msg = COM_ParseOut(msg, buffer, sizeof(buffer));

			dl->replycode = atoi(buffer);

			if (!stricmp(buffer, "100"))
			{	//http/1.1 servers can give this. We ignore it.
				con->bufferused -= ammount;
				memmove(con->buffer, con->buffer+ammount, con->bufferused);
				return true;
			}

			if (!stricmp(buffer, "301") || !stricmp(buffer, "302") || !stricmp(buffer, "303"))
			{
				char trimmed[256];
				nl = strchr(msg, '\n');
				if (nl)
					*nl = '\0';
				COM_TrimString(msg, trimmed, sizeof(trimmed));
				Con_Printf(S_COLOR_GRAY "%s: %s %s (%s)\n", dl->url, buffer, trimmed, Location);
				if (!*Location)
				{
					Con_Printf("Server redirected to null location\n");
					return false;
				}
				else
				{
					if (dl->redircount++ > 10)
					{
						Con_Printf("HTTP: Recursive redirects\n");
						return false;
					}
					HTTP_Cleanup(dl);
					if (*Location == '/')
					{
						char *cur = *dl->redir?dl->redir:dl->url;
						char *curserver = cur;
						char *curpath;
						/*same server+protocol*/
						if (!strncmp(curserver, "http://", 7))
							curserver += 7;
						curpath = strchr(curserver, '/');
						if (!curpath)
							curpath = curserver + strlen(curserver);
						if (cur == dl->redir)
							*curpath = 0;
						else
							Q_strncpyz(dl->redir, cur, (curpath-cur) + 1);
						Q_strncatz(dl->redir, Location, sizeof(dl->redir));
					}
					else
						Q_strncpyz(dl->redir, Location, sizeof(dl->redir));
					dl->poll = DL_Decide;
					dl->status = DL_PENDING;
				}
				return true;
			}

			if (stricmp(buffer, "200") && stricmp(buffer, "201") && stricmp(buffer, "202"))
			{
				nl = strchr(msg, '\n');
				if (!nl)
					return false;	//eh?
				if (nl>msg&&nl[-1] == '\r')
					nl--;
				*nl = '\0';
				Con_Printf("%s: %s%s\n", dl->url, buffer, msg);
				return false;	//something went wrong.
			}

			if (con->contentlength != -1 && con->contentlength > dl->sizelimit)
			{
				char s1[32],s2[32];
				dl->replycode = 413;	//413 Payload Too Large 
				Con_Printf(CON_WARNING"Request exceeds size limit - %s (%s vs %s)\n", dl->url, FS_AbbreviateSize(s1,sizeof(s1),con->contentlength), FS_AbbreviateSize(s2,sizeof(s2),dl->sizelimit));
				return false;	//something went wrong.
			}

			con->bufferused -= ammount;

			dl->totalsize = con->contentlength;

			memmove(con->buffer, con->buffer+ammount, con->bufferused);
		}

		if (dl->notifystarted)
		{
			if (!dl->notifystarted(dl, *mimetype?mimetype:NULL))
			{
				dl->notifycomplete = NULL;
				dl->status = DL_FAILED;
				return false;
			}
		}


		if (!dl->file)
		{
			if (*dl->localname)
			{
				FS_CreatePath(dl->localname, dl->fsroot);
				dl->file = FS_OpenVFS(dl->localname, "w+b", dl->fsroot);
			}
			else
				dl->file = FS_OpenTemp();

			if (!dl->file)
			{
				if (*dl->localname)
					Con_Printf("HTTP: Couldn't open file \"%s\"\n", dl->localname);
				else
					Con_Printf("HTTP: Couldn't open temporary file\n");
				dl->status = DL_FAILED;
				return false;
			}
		}

		if (con->gzip)
		{
#ifdef AVAIL_GZDEC
			con->file = FS_GZ_WriteFilter(dl->file, false, false);
#else
			Con_Printf("HTTP: no support for gzipped files \"%s\"\n", dl->localname);
			dl->status = DL_FAILED;
			return false;
#endif
		}
		else
			con->file = dl->file;
		con->state = HC_GETTING;
		dl->status = DL_ACTIVE;
		goto firstread;	//oh noes! an evil goto! this is the easiest way to avoid the 'return' here when no data follows a 0-byte download
	case HC_GETTING:
		if (con->bufferlen - con->bufferused < 1530)
			ExpandBuffer(con, 1530);

		ammount = VFS_READ(con->stream, con->buffer+con->bufferused, con->bufferlen-con->bufferused-1);
		if (ammount == 0)
			return true;	//no data yet
		else if (ammount < 0)
			ammount = 0;	//error (EOF?)

		con->bufferused+=ammount;

firstread:
		if (con->chunking)
		{
			//9\r\n
			//chunkdata\r\n
			//(etc)
			int trim;
			char *nl;
			con->buffer[con->bufferused] = '\0';
			for(;;)
			{	//work out as we go.
				if (con->chunksize)//we are trying to parse a chunk.
				{
					trim = con->bufferused - con->chunked;
					if (trim > con->chunksize)
						trim = con->chunksize;	//don't go into the next size field.

					if (con->chunksize == trim)
					{	//we need to find the next \n and trim it.
						nl = strchr(con->buffer+con->chunked+trim, '\n');
						if (!nl)
							break;
						nl++;
						con->chunksize = 0;
						con->chunked += trim;

						//chop out the \r\n from the stream
						trim = nl - (con->buffer+con->chunked);
						memmove(con->buffer + con->chunked, nl, con->buffer+con->bufferused-nl+1);
						con->bufferused -= trim;
					}
					else
					{
						con->chunksize -= trim;
						con->chunked += trim;
					}

					if (!(con->bufferused - con->chunked))
						break;
				}
				else
				{
					size_t nextsize;
					nl = strchr(con->buffer+con->chunked, '\n');
					if (!nl)
						break;
					nextsize = strtoul(con->buffer+con->chunked, NULL, 16);	//it's hex.
					nl++;
					if (!nextsize) //eof. make sure we skip its \n too
					{
						nl = strchr(nl, '\n');
						if (!nl)
							break;
						transfercomplete = true;
					}
					con->chunksize = nextsize;
					trim = nl - (con->buffer+con->chunked);
					memmove(con->buffer + con->chunked, nl, con->buffer+con->bufferused-nl+1);
					con->bufferused -= trim;
				}
			}

			con->totalreceived+=con->chunked;
			if (con->totalreceived > dl->sizelimit)
			{
				char s1[32],s2[32];
				dl->replycode = 413;	//413 Payload Too Large 
				Con_Printf(CON_WARNING"Request exceeds size limit - %s (%s vs %s)\n", dl->url, FS_AbbreviateSize(s1,sizeof(s1),con->totalreceived), FS_AbbreviateSize(s2,sizeof(s2),dl->sizelimit));
				return false;	//something went wrong.
			}
			if (con->file && con->chunked)	//we've got a chunk in the buffer
			{	//write it
				if (VFS_WRITE(con->file, con->buffer, con->chunked) != con->chunked)
				{
					Con_Printf("Write error whilst downloading %s\nDisk full?\n", dl->localname);
					return false;
				}

				//and move the unparsed chunk to the front.
				con->bufferused -= con->chunked;
				memmove(con->buffer, con->buffer+con->chunked, con->bufferused);
				con->chunked = 0;
			}
		}
		else
		{
			int chunk = con->bufferused;
			if (con->contentlength != -1 && chunk > con->contentlength-con->totalreceived)
				chunk = con->contentlength-con->totalreceived;

			con->totalreceived+=chunk;
			if (con->totalreceived > dl->sizelimit)
			{
				char s1[32], s2[32];
				dl->replycode = 413;	//413 Payload Too Large 
				Con_Printf(CON_WARNING"Request exceeds size limit - %s (%s vs %s)\n", dl->url, FS_AbbreviateSize(s1,sizeof(s1),con->totalreceived), FS_AbbreviateSize(s2,sizeof(s2),dl->sizelimit));
				return false;	//something went wrong.
			}
			if (con->file)	//we've got a chunk in the buffer
			{	//write it
				if (VFS_WRITE(con->file, con->buffer, chunk) != chunk)
				{
					Con_Printf("Write error whilst downloading %s\nDisk full?\n", dl->localname);
					return false;
				}

				//and move the unparsed chunk to the front.
				memmove(con->buffer, con->buffer+con->bufferused, con->bufferused-chunk);
				con->bufferused -= chunk;
			}
			if (con->totalreceived == con->contentlength)
				transfercomplete = true;
		}

		if (!ammount || transfercomplete)
		{	//server closed off the connection (or signalled eof/sent enough data).
			//if (ammount) then we can save off the connection for reuse.
			if (con->chunksize)
				dl->status = DL_FAILED;
			else
			{
#ifdef AVAIL_GZDEC
				if (con->gzip && con->file)
				{
					VFS_CLOSE(con->file);
					con->file = NULL;
				}
#endif
				if (con->contentlength != -1 && con->totalreceived != con->contentlength)
					dl->status = DL_FAILED;	//file was truncated
				else
					dl->status = (dl->replycode == 200)?DL_FINISHED:DL_FAILED; 
			}
			return false;
		}
		dl->completed = con->totalreceived;

		break;
	}

	return true;
}

void HTTPDL_Establish(struct dl_download *dl)
{
	struct http_dl_ctx_s *con;
	qboolean https = false;

#ifdef COOKIECOOKIECOOKIE
	char cookies[8192];
#else
	char *cookies = "";
#endif
	char uri[MAX_OSPATH];
	char *slash;
	const char *url = dl->redir;
	if (!*url)
		url = dl->url;

	if (!strnicmp(url, "https://", 8))
	{
		url+=8;
		https = true;
	}
	else if (!strnicmp(url, "http://", 7))
		url+=7;

	con = malloc(sizeof(*con));
	memset(con, 0, sizeof(*con));

	slash = strchr(url, '/');
	if (!slash)
	{
		Q_strncpyz(con->server, url, sizeof(con->server));
		Q_strncpyz(uri, "/", sizeof(uri));
	}
	else
	{
		Q_strncpyz(uri, slash, sizeof(uri));
		Q_strncpyz(con->server, url, sizeof(con->server));
		con->server[slash-url] = '\0';
	}

	dl->ctx = con;
	dl->abort = HTTP_Cleanup;

	dl->status = DL_RESOLVING;

	con->sock = INVALID_SOCKET;
	con->stream = NULL;
	con->secure = false;
#ifndef HAVE_SSL
	if (!https)
#endif
	{
		netadr_t adr = {0};
		//fixme: support more than one address possibility?
		//https uses a different default port
		if (NET_StringToAdr2(con->server, https?443:80, &adr, 1, NULL))
			con->sock = TCP_OpenStream(&adr, *dl->redir?dl->redir:dl->url);
		con->stream = FS_WrapTCPSocket(con->sock, true, con->server);
		if (!con->stream)
		{
			dl->status = DL_FAILED;
			return;
		}
	}
#ifdef HAVE_SSL
	if (https)
	{
		//https has an extra ssl/tls layer between tcp and http.
		con->stream = FS_OpenSSL(con->server, con->stream, false);
		con->secure = true;
	}
#endif
	if (!con->stream)
	{
		dl->status = DL_FAILED;
		return;
	}
#ifdef COOKIECOOKIECOOKIE
	Cookie_Regurgitate(con->server, con->secure, cookies, sizeof(cookies));
#endif
	if (dl->postdata)
	{
		ExpandBuffer(con, 1024 + strlen(uri) + strlen(con->server) + strlen(cookies) + strlen(dl->postmimetype) + dl->postlen);
		Q_snprintfz(con->buffer, con->bufferlen,
			"POST %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			/*Cookie:*/ "%s"
			"Content-Length: %u\r\n"
			"Content-Type: %s\r\n"
			"Connection: close\r\n"
#ifdef AVAIL_GZDEC
			"Accept-Encoding: gzip\r\n"
#endif
			"User-Agent: "FULLENGINENAME"\r\n"
			"\r\n", uri, con->server, cookies, (unsigned int)dl->postlen, dl->postmimetype);
		con->bufferused = strlen(con->buffer);
		memcpy(con->buffer + con->bufferused, dl->postdata, dl->postlen);
		con->bufferused += dl->postlen;
	}
	else
	{
		ExpandBuffer(con, 512*1024);
		Q_snprintfz(con->buffer, con->bufferlen,
			"GET %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			/*Cookie:*/ "%s"
			"Connection: close\r\n"			//theoretically, this is not needed. but as our code will basically do it anyway, it might as well be here FIXME: implement connection reuse.
#ifdef AVAIL_GZDEC
			"Accept-Encoding: gzip\r\n"
#endif
			"User-Agent: "FULLENGINENAME"\r\n"
			"\r\n", uri, con->server, cookies);
		con->bufferused = strlen(con->buffer);
	}
	con->contentlength = -1;
}

qboolean HTTPDL_Poll(struct dl_download *dl)
{
	/*failed previously*/
	if (dl->status == DL_FAILED)
		return false;

	if (!dl->ctx)
	{
		HTTPDL_Establish(dl);
		if (dl->status == DL_FAILED)
		{
			HTTP_Cleanup(dl);
			dl->status = DL_FAILED;
			return false;
		}
	}

	if (dl->ctx)
	{
		if (!HTTP_DL_Work(dl))
			if (dl->status != DL_FINISHED)
				dl->status = DL_FAILED;
		if (dl->status == DL_FAILED)
		{
			HTTP_Cleanup(dl);
			dl->status = DL_FAILED;
			return false;
		}
		if (dl->status == DL_FINISHED)
			return false;
	}

	return true;
}

/*
//decode a base64 byte to a 0-63 value. Cannot cope with =.
static unsigned int Base64_DecodeByte(char byt)
{
    if (byt >= 'A' && byt <= 'Z')
        return (byt-'A') + 0;
    if (byt >= 'a' && byt <= 'z')
        return (byt-'a') + 26;
    if (byt >= '0' && byt <= '9')
        return (byt-'0') + 52;
    if (byt == '+')
        return 62;
    if (byt == '/')
        return 63;
    return -1;
}
//FIXME: we should be able to skip whitespace.
static int Base64_Decode(char *out, int outlen, char **srcout, int *srclenout)
{
	int len = 0;
	unsigned int result;

	char *src = *srcout;
	int srclen = *srclenout;

	//4 input chars give 3 output chars
	while(srclen >= 4)
	{
		if (len+3 > outlen)
		{
			//ran out of space in the output buffer
			*srcout = src;
			*srclenout = srclen;
			return len;
		}
		result = Base64_DecodeByte(src[0])<<18;
		result |= Base64_DecodeByte(src[1])<<12;
		out[len++] = (result>>16)&0xff;
		if (src[2] != '=')
		{
			result |= Base64_DecodeByte(src[2])<<6;
			out[len++] = (result>>8)&0xff;
			if (src[3] != '=')
			{
				result |= Base64_DecodeByte(src[3])<<0;
				out[len++] = (result>>0)&0xff;
			}
		}
		if (result & 0xff000000)
			return 0;	//some kind of invalid char

		src += 4;
		srclen -= 4;
	}

	//end of string
	*srcout = src;
	*srclenout = srclen;

	//some kind of error
	if (srclen)
	{
		if (srclen != 1 || *src)
			return 0;
	}
	
	return len;
}

qboolean DataScheme_Decode(struct dl_download *dl)
{
	char block[8192];
	int remaining, blocksize;
	char mimetype[256];
	char baseval[256];
	char charsetval[256];
	char *url;
	char *data;
	char *charset;
	char *base;
	//failed previously
	if (dl->status == DL_FAILED)
		return false;

	//data:[<MIME-type>][;charset=<encoding>][;base64],<data>

	*mimetype = 0;
	*baseval = 0;
	*charsetval = 0;

	url = dl->url;
	if (!strncmp(url, "data:", 5))
		url+=5;	//should always match
	data = strchr(url, ',');
	if (!data)
		return false;
	charset = memchr(url, ';', data-url);
	if (charset)
	{
		base = memchr(charset+1, ';', data-charset);
		if (base)
		{
			if (data-(base+1) >= sizeof(baseval))
				return false;
			memcpy(baseval, base+1, data-(base+1));
			baseval[data-(base+1)] = 0;
		}
		else
			base = data;
		if (base-(charset+1) >= sizeof(charsetval))
			return false;
		memcpy(charsetval, charset+1, base-(charset+1));
		charsetval[base-(charset+1)] = 0;

		if (!strchr(charsetval, '='))
		{
			strcpy(baseval, charsetval);
			*charsetval = 0;
		}
	}
	else
		charset = data;
	if (charset-(url) >= sizeof(charsetval))
		return false;
	memcpy(mimetype, url, charset-(url));
	mimetype[charset-(url)] = 0;

	if (!*mimetype)
		Q_strncpyz(mimetype, "text/plain", sizeof(mimetype));
	if (!*charsetval)
		Q_strncpyz(charsetval, "charset=US-ASCII", sizeof(charsetval));

	if (dl->notifystarted)
		dl->notifystarted(dl, *mimetype?mimetype:NULL);

	if (!dl->file)
	{
		if (*dl->localname)
		{
			FS_CreatePath(dl->localname, dl->fsroot);
			dl->file = FS_OpenVFS(dl->localname, "w+b", dl->fsroot);
		}
		else
			dl->file = FS_OpenTemp();
		if (!dl->file)
		{
			if (*dl->localname)
				Con_Printf("HTTP: Couldn't open file \"%s\"\n", dl->localname);
			else
				Con_Printf("HTTP: Couldn't open temporary file\n");
			dl->status = DL_FAILED;
			return false;
		}
	}

	data++;
	remaining = strlen(data);
	while(remaining > 0)
	{
		blocksize = Base64_Decode(block, sizeof(block), &data, &remaining);
		if (!blocksize)
		{
			dl->status = DL_FAILED;
			return false;
		}
		VFS_WRITE(dl->file, block, blocksize);
	}

	dl->status = DL_FINISHED;
	return false;
}
*/

qboolean DL_Decide(struct dl_download *dl)
{
	const char *url = dl->redir;
	if (!*url)
		url = dl->url;

	/*if (!strnicmp(url, "data:", 5))
		dl->poll = DataScheme_Decode;
	else*/
	if (!strnicmp(url, "http://", 7))
		dl->poll = HTTPDL_Poll;
	else if (!strnicmp(url, "https://", 7))
		dl->poll = HTTPDL_Poll;
	else
	{
		dl->status = DL_FAILED;
		return false;
	}
	return true;
}
#endif	/*!defined(FTE_TARGET_WEB)*/

#ifdef MULTITHREAD
static unsigned int dlthreads = 0;
#define MAXDOWNLOADTHREADS 4
#if defined(LOADERTHREAD) 
static void HTTP_Wake_Think(void *ctx, void *data, size_t a, size_t b)
{
	dlthreads--;
	HTTP_CL_Think(NULL, NULL);
}
#endif
static int DL_Thread_Work(void *arg)
{
	struct dl_download *dl = arg;

	while (dl->threadenable)
	{
		if (!dl->poll(dl))
		{
			if (dl->status != DL_FAILED && dl->status != DL_FINISHED)
				dl->status = DL_FAILED;
			break;
		}
	}
	dl->threadenable = false;

#if defined(LOADERTHREAD) 
	COM_AddWork(WG_MAIN, HTTP_Wake_Think, NULL, NULL, 0, 0);
#endif
	return 0;
}

/*create a thread to perform the given download
to use: call DL_Create (not HTTP_CL_Get!) to get a context, then call this.
note that you need to call DL_Close from another thread, NOT IN THE NOTIFY FUNC.
the file handle must be safe to write to in threads.
*/
qboolean DL_CreateThread(struct dl_download *dl, vfsfile_t *file, void (*NotifyFunction)(struct dl_download *dl))
{
	if (!dl)
		return false;

	if (file)
		dl->file = file;
	if (NotifyFunction)
		dl->notifycomplete = NotifyFunction;

	dl->threadenable = true;
#if defined(LOADERTHREAD) 
	if (dlthreads < 4)
#endif
	{
		dl->threadctx = Sys_CreateThread("download", DL_Thread_Work, dl, THREADP_NORMAL, 0);
		if (!dl->threadctx)
			return false;
		dlthreads++;
	}

	return true;
}
#else
qboolean DL_CreateThread(struct dl_download *dl, vfsfile_t *file, void (*NotifyFunction)(struct dl_download *dl))
{
	if (!dl)
		return false;

	if (file)
		dl->file = file;
	if (NotifyFunction)
		dl->notifycomplete = NotifyFunction;

	return false;
}
#endif

/*create a standalone download context*/
struct dl_download *DL_Create(const char *url)
{
	struct dl_download *newdl;
	newdl = malloc(sizeof(*newdl) + strlen(url)+1);
	if (!newdl)
		return NULL;
	memset(newdl, 0, sizeof(*newdl));
	newdl->url = (char*)(newdl+1);
	strcpy(newdl->url, url);
	newdl->poll = DL_Decide;
	newdl->fsroot = FS_GAMEONLY;
	newdl->sizelimit = 0x80000000u;	//some sanity limit.
#if !defined(SERVERONLY)
	newdl->qdownload.method = DL_HTTP;
#endif

	if (!newdl->poll(newdl))
	{
		free(newdl);
		newdl = NULL;
	}

	return newdl;
}

/*destroys an entire download context*/
void DL_Close(struct dl_download *dl)
{
	struct dl_download **link = NULL;

	for (link = &activedownloads; *link; link = &(*link)->next)
	{
		if (*link == dl)
		{
			*link = dl->next;
			break;
		}
	}

#if !defined(SERVERONLY)
	if (cls.download == &dl->qdownload)
		cls.download = NULL;
#endif

#ifdef MULTITHREAD
	dl->threadenable = false;
	if (dl->threadctx)
	{
		Sys_WaitOnThread(dl->threadctx);
		dl->threadctx = NULL;
	}
#endif
	if (dl->file && dl->file->seekstyle < SS_PIPE)
		VFS_SEEK(dl->file, 0);
	if (dl->notifycomplete)
		dl->notifycomplete(dl);
	if (dl->abort)
		dl->abort(dl);
	if (dl->file)
		VFS_CLOSE(dl->file);
	if (dl->postdata)
		BZ_Free(dl->postdata);
	free(dl);
}

void DL_DeThread(void)
{
#ifdef MULTITHREAD
	//if we're about to fork, ensure that any downloads are properly parked so that there's no workers in an unknown state.
	struct dl_download *dl;
	for (dl = activedownloads; dl; dl = dl->next)
	{
		dl->threadenable = false;
		if (dl->threadctx)
		{
			Sys_WaitOnThread(dl->threadctx);
			dl->threadctx = NULL;
		}
	}
#endif
}

/*updates pending downloads*/
unsigned int HTTP_CL_GetActiveDownloads(void)
{
	struct dl_download *dl;
	unsigned int count = 0;

	for (dl = activedownloads; dl; dl = dl->next)
		count++;
	return count;
}

/*create a download context and add it to the list, for lazy people. not threaded*/
struct dl_download *HTTP_CL_Get(const char *url, const char *localfile, void (*NotifyFunction)(struct dl_download *dl))
{
	struct dl_download *newdl = DL_Create(url);
	if (!newdl)
		return newdl;

	newdl->notifycomplete = NotifyFunction;
	if (localfile)
		Q_strncpyz(newdl->localname, localfile, sizeof(newdl->localname));

	newdl->next = activedownloads;
	activedownloads = newdl;


#ifndef SERVERONLY
	if (!cls.download && localfile && !newdl->isquery)
	{
		cls.download = &newdl->qdownload;
		if (*newdl->localname)
			Q_strncpyz(newdl->qdownload.localname, newdl->localname, sizeof(newdl->qdownload.localname));
		else
			Q_strncpyz(newdl->qdownload.localname, newdl->url, sizeof(newdl->qdownload.localname));
		Q_strncpyz(newdl->qdownload.remotename, newdl->url, sizeof(newdl->qdownload.remotename));
		newdl->qdownload.starttime = Sys_DoubleTime();
	}
#endif

	return newdl;
}

struct dl_download *HTTP_CL_Put(const char *url, const char *mime, const char *data, size_t datalen, void (*NotifyFunction)(struct dl_download *dl))
{
	struct dl_download *dl;
	if (!*mime)
		return NULL;

	dl = HTTP_CL_Get(url, NULL, NotifyFunction);
	if (dl)
	{
		Q_strncpyz(dl->postmimetype, mime, sizeof(dl->postmimetype));
		dl->postdata = BZ_Malloc(datalen);
		memcpy(dl->postdata, data, datalen);
		dl->postlen = datalen;
	}
	return dl;
}

void HTTP_CL_Think(const char **curname, float *curpercent)
{
	struct dl_download *dl = activedownloads;
	struct dl_download **link = NULL;
#ifndef SERVERONLY
	float currenttime;
	if (!activedownloads)
		return;
	currenttime = Sys_DoubleTime();
#endif

	link = &activedownloads;
	while (*link)
	{
		dl = *link;
#ifdef MULTITHREAD
		if (dl->threadctx)
		{
			if (dl->status == DL_FINISHED || dl->status == DL_FAILED)
			{
				Sys_WaitOnThread(dl->threadctx);
				dl->threadctx = NULL;
				continue;
			}
		}
		else if (dl->threadenable)
		{
			if (dlthreads < MAXDOWNLOADTHREADS)
			{
				dl->threadctx = Sys_CreateThread("download", DL_Thread_Work, dl, THREADP_NORMAL, 0);
				if (dl->threadctx)
					dlthreads++;
				else
					dl->threadenable = false;
			}
		}
		else
#endif
		{
			if (!dl->poll(dl))
			{
				*link = dl->next;
				DL_Close(dl);
				continue;
			}
		}
		link = &dl->next;

		if (curname && curpercent)
		{
			if (*dl->localname)
				*curname = (const char*)dl->localname;
			else
				*curname = (const char*)dl->url;

			if (dl->status == DL_FINISHED)
				*curpercent = 100;
			else if (dl->status != DL_ACTIVE)
				*curpercent = 0;
			else if (dl->totalsize <= 0)
				*curpercent = -1;
			else
				*curpercent = dl->completed*100.0f/dl->totalsize;
		}

#ifndef SERVERONLY
		if (!cls.download && !dl->isquery)
#ifdef MULTITHREAD
		if (!dl->threadenable || dl->threadctx)	//don't show pending downloads in preference to active ones.
#endif
		{
			cls.download = &dl->qdownload;
			dl->qdownload.method = DL_HTTP;
			if (*dl->localname)
				Q_strncpyz(dl->qdownload.localname, dl->localname, sizeof(dl->qdownload.localname));
			else
				Q_strncpyz(dl->qdownload.localname, dl->url, sizeof(dl->qdownload.localname));
			Q_strncpyz(dl->qdownload.remotename, dl->url, sizeof(dl->qdownload.remotename));
			dl->qdownload.starttime = Sys_DoubleTime();
		}

		if (dl->status == DL_FINISHED)
			dl->qdownload.percent = 100;
		else if (dl->status != DL_ACTIVE)
			dl->qdownload.percent = 0;
		else if (dl->totalsize <= 0)
		{
			dl->qdownload.sizeunknown = true;
			dl->qdownload.percent = 50;
		}
		else
			dl->qdownload.percent = dl->completed*100.0f/dl->totalsize;
		dl->qdownload.completedbytes = dl->completed;

		if (dl->qdownload.ratetime < currenttime)
		{
			dl->qdownload.ratetime = currenttime+1;
			dl->qdownload.rate = (dl->qdownload.completedbytes - dl->qdownload.ratebytes) / 1;
			dl->qdownload.ratebytes = dl->qdownload.completedbytes;
		}
#endif
	}
}

void HTTP_CL_Terminate(void)
{
	struct dl_download *dl = activedownloads;
	struct dl_download *next = NULL;
	next = activedownloads;
	activedownloads = NULL;
	while (next)
	{
		dl = next;
		next = dl->next;
		DL_Close(dl);
	}
	HTTP_CL_Think(NULL, NULL);

#ifdef COOKIECOOKIECOOKIE
	Cookie_Monster();
#endif
}
#endif	/*WEBCLIENT*/


typedef struct 
{
	vfsfile_t funcs;

	char *data;
	size_t maxlen;
	size_t writepos;
	size_t readpos;
	void *mutex;
	int refs;
	qboolean terminate;	//one end has closed, make the other report failures now that its no longer needed.

	void *ctx;
	void (*callback) (void *ctx, vfsfile_t *pipe);
} vfspipe_t;

static qboolean QDECL VFSPIPE_Close(vfsfile_t *f)
{
	int r;
	vfspipe_t *p = (vfspipe_t*)f;
	Sys_LockMutex(p->mutex);
	r = --p->refs;
	p->terminate = true;
	Sys_UnlockMutex(p->mutex);
	if (!r)
	{
		free(p->data);
		Sys_DestroyMutex(p->mutex);
		free(p);
	}
	else if (p->callback)
		p->callback(p->ctx, f);
	return true;
}
static qofs_t QDECL VFSPIPE_GetLen(vfsfile_t *f)
{
	vfspipe_t *p = (vfspipe_t*)f;
	return p->writepos - p->readpos;
}
static qofs_t QDECL VFSPIPE_Tell(vfsfile_t *f)
{
	vfspipe_t *p = (vfspipe_t*)f;
	return p->readpos;
}
static qboolean QDECL VFSPIPE_Seek(vfsfile_t *f, qofs_t offset)
{
	vfspipe_t *p = (vfspipe_t*)f;
	p->readpos = offset;
	return true;
}
static int QDECL VFSPIPE_ReadBytes(vfsfile_t *f, void *buffer, int len)
{
	vfspipe_t *p = (vfspipe_t*)f;
	Sys_LockMutex(p->mutex);
	if (p->readpos > p->writepos)
		len = 0;
	if (len > p->writepos - p->readpos)
	{
		len = p->writepos - p->readpos;
		if (!len && p->terminate)
		{	//if we reached eof, we started with two refs and are down to 1 and we're reading, then its the writer side that disconnected.
			//eof is now fatal rather than 'try again later'.
			Sys_UnlockMutex(p->mutex);
			return -1;
		}
	}
	memcpy(buffer, p->data+p->readpos, len);
	p->readpos += len;
	Sys_UnlockMutex(p->mutex);
	return len;
}
static int QDECL VFSPIPE_WriteBytes(vfsfile_t *f, const void *buffer, int len)
{
	vfspipe_t *p = (vfspipe_t*)f;
	if (len < 0)
		return -1;
	Sys_LockMutex(p->mutex);
	if (p->terminate)
	{	//if we started with 2 refs, and we're down to one, and we're writing, then its the reader that closed. that means writing is redundant and we should signal an error.
		Sys_UnlockMutex(p->mutex);
		return -1;
	}
	if (p->readpos > 8192 && !p->funcs.Seek)
	{	//don't grow infinitely if we're reading+writing at the same time
		//if we're seekable, then we have to buffer the ENTIRE file.
		memmove(p->data, p->data+p->readpos, p->writepos-p->readpos);
		p->writepos -= p->readpos;
		p->readpos = 0;
	}
	if (p->writepos + len > p->maxlen)
	{
		p->maxlen = p->writepos + len;
		if (p->maxlen < (p->writepos-p->readpos)*2)	//over-allocate a little
			p->maxlen = (p->writepos-p->readpos)*2;
		if (p->maxlen > 0x8000000 && p->data)
		{
			p->maxlen = max(p->writepos,0x8000000);
			if (p->maxlen <= p->writepos)
			{
				Sys_UnlockMutex(p->mutex);
				return -1;	//try and get the caller to stop
			}
			len = min(len, p->maxlen - p->writepos);
		}
		p->data = realloc(p->data, p->maxlen);
	}
	memcpy(p->data+p->writepos, buffer, len);
	p->writepos += len;
	Sys_UnlockMutex(p->mutex);
	return len;
}

vfsfile_t *VFS_OpenPipeInternal(int refs, qboolean seekable, void (*callback)(void *ctx, vfsfile_t *file), void *ctx)
{
	vfspipe_t *newf;
	newf = malloc(sizeof(*newf));
	newf->refs = refs;
	newf->terminate = false;
	newf->mutex = Sys_CreateMutex();
	newf->data = NULL;
	newf->maxlen = 0;
	newf->readpos = 0;
	newf->writepos = 0;
	newf->funcs.Close = VFSPIPE_Close;
	newf->funcs.Flush = NULL;
	newf->funcs.GetLen = VFSPIPE_GetLen;
	newf->funcs.ReadBytes = VFSPIPE_ReadBytes;
	newf->funcs.WriteBytes = VFSPIPE_WriteBytes;

	newf->ctx = ctx;
	newf->callback = callback;

	if (seekable)
	{	//if this is set, then we allow changing the readpos at the expense of buffering the ENTIRE file. no more fifo.
		newf->funcs.Seek = VFSPIPE_Seek;
		newf->funcs.Tell = VFSPIPE_Tell;
		newf->funcs.seekstyle = SS_PIPE;
	}
	else
	{	//periodically reclaim read data to avoid memory wastage. this means that seeking can't work.
		newf->funcs.Seek = NULL;
		newf->funcs.Tell = NULL;
		newf->funcs.seekstyle = SS_UNSEEKABLE;
	}

	return &newf->funcs;
}
vfsfile_t *VFS_OpenPipeCallback(void (*callback)(void*ctx, vfsfile_t *file), void *ctx)
{
	return VFS_OpenPipeInternal(2, false, callback, ctx);
}

vfsfile_t *VFSPIPE_Open(int refs, qboolean seekable)
{
	return VFS_OpenPipeInternal(refs, seekable, NULL, NULL);
}

static int QDECL VFSPIPE_WriteBytes_Pair(vfsfile_t *f, const void *buffer, int len)
{
	f = ((vfspipe_t*)f)->ctx;	//swap to its partner
	if (!f)	//it got closed...
		return VFS_ERROR_EOF;
	return VFSPIPE_WriteBytes(f, buffer, len);	//read it directly.
}
static int QDECL VFSPIPE_ReadBytes_Pair(vfsfile_t *f, void *buffer, int len)
{
	f = ((vfspipe_t*)f)->ctx;	//swap to its partner
	if (!f)	//it got closed...
		return VFS_ERROR_EOF;
	return VFSPIPE_WriteBytes(f, buffer, len);	//read it directly.
}
static qboolean QDECL VFSPIPE_Close_Pair(vfsfile_t *f)
{
	vfspipe_t *p = ((vfspipe_t*)f)->ctx;
	if (p)
		p->ctx = NULL;	//don't allow writes to a dead pipe...
	return VFSPIPE_Close(f);
}
void VFSPIPE_OpenPair(vfsfile_t *pair[2])
{
	pair[0] = VFS_OpenPipeInternal(1, false, NULL, NULL);
	pair[1] = VFS_OpenPipeInternal(1, false, NULL, NULL);

	//cross-link them
	((vfspipe_t*)pair[0])->ctx = pair[1];
	((vfspipe_t*)pair[1])->ctx = pair[0];
	pair[0]->Close = VFSPIPE_Close_Pair;
	pair[1]->Close = VFSPIPE_Close_Pair;

	pair[0]->WriteBytes = VFSPIPE_WriteBytes_Pair;
	pair[1]->WriteBytes = VFSPIPE_WriteBytes_Pair;


	pair[0]->ReadBytes = VFSPIPE_ReadBytes_Pair;
	pair[1]->ReadBytes = VFSPIPE_ReadBytes_Pair;
}
