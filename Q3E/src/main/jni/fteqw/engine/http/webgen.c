#include "quakedef.h"

#if defined(WEBSERVER) || defined(FTPSERVER)

#include "iweb.h"

#if defined(CLIENTONLY) || !defined(WEBSERVER)
vfsfile_t *IWebGenerateFile(const char *name, const char *content, int contentlength)
{
	return NULL;
}
#else

static char lastrecordedmvd[MAX_QPATH];

static IWeb_FileGen_t *IWeb_GenerationBuffer;
static size_t IWeb_GenerationBufferTotal;
/*
static void IWeb_MoreGeneratedResize(size_t newsize)
{
	IWeb_FileGen_t *ob;

	if (IWeb_GenerationBuffer && IWeb_GenerationBufferTotal >= newsize)
		return;	//already big enough

	ob = IWeb_GenerationBuffer;
	IWeb_GenerationBuffer = BZ_Malloc(sizeof(IWeb_GenerationBuffer) + newsize);
	memset(IWeb_GenerationBuffer, 0, sizeof(*IWeb_GenerationBuffer));

	IWeb_GenerationBuffer->data = (char *)(IWeb_GenerationBuffer+1);
	if (ob)
	{
		memcpy(IWeb_GenerationBuffer->data, ob->data, ob->len);
		IWeb_GenerationBuffer->len = ob->len;
		IWebFree(ob);
	}

	IWeb_GenerationBufferTotal = newsize;
}
*/
static void IWeb_Generate(const char *buf)
{
	size_t count = strlen(buf);
	if (!IWeb_GenerationBuffer || IWeb_GenerationBuffer->len + count >= IWeb_GenerationBufferTotal)
	{
		IWeb_FileGen_t *ob;
		size_t newsize = IWeb_GenerationBufferTotal + count+(16*1024);

		if (newsize < IWeb_GenerationBufferTotal)
		{
			Sys_Error("Integer overflow\n");
			return;	//should probably crash or something.
		}

		ob = IWeb_GenerationBuffer;
		IWeb_GenerationBuffer = BZ_Malloc(sizeof(IWeb_GenerationBuffer) + newsize);
		memset(IWeb_GenerationBuffer, 0, sizeof(*IWeb_GenerationBuffer));

		IWeb_GenerationBuffer->data = (char *)(IWeb_GenerationBuffer+1);
		if (ob)
		{
			memcpy(IWeb_GenerationBuffer->data, ob->data, ob->len);
			IWeb_GenerationBuffer->len = ob->len;
			IWebFree(ob);
		}

		IWeb_GenerationBufferTotal = newsize;
	}

	memcpy(&IWeb_GenerationBuffer->data[IWeb_GenerationBuffer->len], buf, count);
	IWeb_GenerationBuffer->len+=count;
}

int Rank_Enumerate (unsigned int first, unsigned int last, void (*callback) (const rankinfo_t *ri));	//leader first.

/*
static void IWeb_ParseForm(char *info, int infolen, char *text)
{
	char *eq, *and;
	char *token, *out;
	*info = '\0';
	if (!text)
		return;
	while(*text)
	{
		eq = strchr(text, '=');
		if (!eq)
			break;
		*eq = '\0';
		and = strchr(eq+1, '&');
		if (and)
			*and = '\0';

		for (out = token = eq+1; *token;)
		{
			if (*token == '+')
			{
				*out++ = ' ';
				token++;
			}
			else if (*token == '%' && token[1] && token[2])
			{
				int c = 0;

				if (token[1] >= '0' && token[1] <= '9')
				{
					c += token[1] - '0';
				}
				else if (token[1] >= 'a' && token[1] <= 'f')
				{
					c += token[1] - 'a'+10;
				}
				else if (token[1] >= 'A' && token[1] <= 'F')
				{
					c += token[1] - 'A'+10;
				}
				c*=16;
				if (token[2] >= '0' && token[2] <= '9')
				{
					c += token[2] - '0';
				}
				else if (token[2] >= 'a' && token[2] <= 'f')
				{
					c += token[2] - 'a'+10;
				}
				else if (token[2] >= 'A' && token[2] <= 'F')
				{
					c += token[2] - 'A'+10;
				}

				*out++ = c;
				token+=3;
			}
			else
				*out++ = *token++;
		}
		*out = '\0';

		Info_SetValueForKey(info, text, eq+1, infolen);
		if (!and)
			return;
		text = and+1;
	}
}

static void IWeb_GenerateAdminFile(char *parms, char *content, int contentlength)
{
	extern char	outputbuf[];	//redirected buffer - always null termed.
	char info[16384];
	char *pwd;
	char *cmd;
	char *mark, *start;

	extern cvar_t	rcon_password;
	IWeb_Generate("<HTML><HEAD><TITLE>FTEQWSV - admin</TITLE></HEAD><BODY>");
	if (*rcon_password.string)
	{
		IWeb_ParseForm(info, sizeof(info), content);
		pwd = Info_ValueForKey(info, "pwd");
		cmd = Info_ValueForKey(info, "cmd");

		IWeb_Generate("<FORM action=\"admin.html\" method=\"post\">");
		IWeb_Generate("<CENTER>");
		IWeb_Generate("<input name=pwd value=\"");
		if (!strcmp(rcon_password.string, pwd))
			IWeb_Generate(rcon_password.string);
		IWeb_Generate("\">");
		IWeb_Generate("<BR>");
		IWeb_Generate("<input name=cmd maxsize=255 size=40 value=\"\">");
		IWeb_Generate("<BR>");
		IWeb_Generate("<input type=submit value=\"Submit\" name=btn>");
		IWeb_Generate("</CENTER>");
		IWeb_Generate("</FORM>");

		if (!strcmp(rcon_password.string, pwd))
		{
			Con_Printf("Web based rcon: %s\n", cmd);
			SV_BeginRedirect(RD_OBLIVION, host_client->language);
			Cmd_ExecuteString(cmd, RESTRICT_RCON);
			for (mark = start = outputbuf; *mark; mark++)
			{
				if (*mark == '\n')
				{
					*mark = '\0';
					IWeb_Generate(start);
					IWeb_Generate("<br>");
					start = mark+1;
				}

			}
			IWeb_Generate(start);
			SV_EndRedirect();
		}
		else if (*pwd)
			IWeb_Generate("Password is incorrect.");
	}
	else
		IWeb_Generate("<H1>Remote administration is not enabled.<H2>");
	IWeb_Generate("</BODY></HTML>");
}
*/

static void IWeb_GenerateRankingsFileCallback(const rankinfo_t *ri)
{
	IWeb_Generate("<TR><TD ALIGN = \"center\">");
	IWeb_Generate(ri->h.name);
	IWeb_Generate("</TD><TD ALIGN = \"center\">");
	IWeb_Generate(va("%i", ri->s.kills));
	IWeb_Generate("</TD><TD ALIGN = \"center\">");
	IWeb_Generate(va("%i", ri->s.deaths));
	IWeb_Generate("</TD>");
	IWeb_Generate("</TR>");
}

static void IWeb_GenerateRankingsFile (const char *parms, const char *content, int contentlength)
{
	IWeb_Generate("<HTML><HEAD></HEAD><BODY>");

	if (Rank_OpenRankings())
	{
		IWeb_Generate("<TABLE CELLSPACING=\"1\" CELLPADDING=\"1\">");
		IWeb_Generate("<CAPTION>Players</CAPTION>");
		if (Rank_Enumerate(atoi(parms), atoi(parms)+20, IWeb_GenerateRankingsFileCallback) == 20)
		{
			IWeb_Generate("</TABLE>");
			if (atoi(parms) >= 20)
				IWeb_Generate(va("<A HREF=allplayers.html?%i>Less</A>", atoi(parms)-20));
			else if (atoi(parms) > 0)
				IWeb_Generate(va("<A HREF=allplayers.html?%i>Less</A>", 0));
			IWeb_Generate(va("<A HREF=allplayers.html?%i>More</A>", atoi(parms)+20));
		}
		else
		{
			IWeb_Generate("</TABLE>");
			if (atoi(parms) >= 20)
				IWeb_Generate(va("<A HREF=allplayers.html?%i>Less</A>", atoi(parms)-20));
			else if (atoi(parms) > 0)
				IWeb_Generate(va("<A HREF=allplayers.html?%i>Less</A>", 0));
		}
	}
	else
		IWeb_Generate("<H1>Rankings are disabled. Sorry.<H2>");
	IWeb_Generate("</BODY></HTML>");
}

static void IWeb_GenerateIndexFile (const char *parms, const char *content, int contentlength)
{
	extern cvar_t	rcon_password;
	char *s, *o;
	char key[128], value[128];
	int l;
	qboolean added;
	client_t *cl;
	IWeb_Generate("<HTML><HEAD><TITLE>FTEQWSV</TITLE></HEAD><BODY>");
	IWeb_Generate("<H1>");
	IWeb_Generate(hostname.string);
	IWeb_Generate("</H1>");

	IWeb_Generate("<A HREF=\""ENGINEWEBSITE"\">Engine website</A><P>");

	if (Rank_OpenRankings())
		IWeb_Generate("<A HREF=\"allplayers.html\">Click here to see ranked players.</A><P>");
	if (*rcon_password.string)
		IWeb_Generate("<A HREF=\"admin.html\">Admin.</A><P>");

	s = svs.info;

	IWeb_Generate("<TABLE CELLSPACING=\"1\" CELLPADDING=\"1\">");
	IWeb_Generate("<CAPTION>Server Info</CAPTION>");
	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
//		if (l < 20)
//		{
//			memset (o, ' ', 20-l);
//			key[20] = 0;
//		}
//		else
			*o = 0;

		IWeb_Generate("<TR><TD ALIGN = \"center\">");
		IWeb_Generate(key);

		if (!*s)
		{
			IWeb_Generate("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;

		IWeb_Generate("</TD><TD ALIGN = \"center\">");
		IWeb_Generate(value);
		IWeb_Generate("</TD></TR>");
	}

	IWeb_Generate("</TABLE>");

	IWeb_Generate("<P><TABLE CELLSPACING=\"1\" CELLPADDING=\"1\">");
	IWeb_Generate("<CAPTION>Players</CAPTION>");
	added = false;
	for (l = 0, cl = svs.clients; l < sv.allocated_client_slots; l++, cl++)
	{
		if (cl->state <= cs_zombie)
			continue;

		IWeb_Generate("<TR><TD ALIGN = \"center\">");
		IWeb_Generate(cl->name);
		IWeb_Generate("</TD><TD ALIGN = \"center\">");
		IWeb_Generate(va("%i", cl->old_frags));
		IWeb_Generate("</TD></TR>");
		added = true;
	}
	if (!added)
	{
		IWeb_Generate("<TR><TD ALIGN = \"center\">");
		IWeb_Generate("No players on server");
		IWeb_Generate("</TD><TD ALIGN = \"center\">");
	}

	IWeb_Generate("</TABLE>");

	IWeb_Generate("</BODY></HTML>");
}

typedef struct {
	char *name;
	void (*GenerationFunction) (const char *parms, const char *content, int contentlength);
	float lastgenerationtime;
	float oldbysecs;
	IWeb_FileGen_t *buffer;
	int genid;
} IWebFile_t;

static IWebFile_t IWebFiles[] = {
	{"allplayers.html", IWeb_GenerateRankingsFile},
	{"index.html", IWeb_GenerateIndexFile},
//code is too flawed for this	{"admin.html", IWeb_GenerateAdminFile}
};

typedef struct {
	vfsfile_t funcs;

	IWeb_FileGen_t *buffer;
	int pos;
} vfsgen_t;

static int QDECL VFSGen_ReadBytes(vfsfile_t *f, void *buffer, int bytes)
{
	vfsgen_t *g = (vfsgen_t*)f;
	if (bytes + g->pos >= g->buffer->len)
	{
		bytes = g->buffer->len - g->pos;
		if (bytes <= 0)
			return 0;
	}

	memcpy(buffer, g->buffer->data+g->pos, bytes);
	g->pos += bytes;

	return bytes;
}

static int QDECL VFSGen_WriteBytes(vfsfile_t *f, const void *buffer, int bytes)
{
	Sys_Error("VFSGen_WriteBytes: Readonly\n");
	return 0;
}

static qboolean QDECL VFSGen_Seek(vfsfile_t *f, qofs_t newpos)
{
	vfsgen_t *g = (vfsgen_t*)f;
	if (newpos < 0 || newpos >= g->buffer->len)
		return false;

	g->pos = newpos;

	return true;
}

static qofs_t QDECL VFSGen_Tell(vfsfile_t *f)
{
	vfsgen_t *g = (vfsgen_t*)f;
	return g->pos;
}

static qofs_t QDECL VFSGen_GetLen(vfsfile_t *f)
{
	vfsgen_t *g = (vfsgen_t*)f;
	return g->buffer->len;
}

static qboolean QDECL VFSGen_Close(vfsfile_t *f)
{
	int fnum;
	vfsgen_t *g = (vfsgen_t*)f;
	g->buffer->references--;
	if (!g->buffer->references)
	{
		Z_Free(g->buffer->data);
		Z_Free(g->buffer);

		for (fnum = 0; fnum < sizeof(IWebFiles) / sizeof(IWebFile_t); fnum++)
			if (IWebFiles[fnum].buffer == g->buffer)
				IWebFiles[fnum].buffer = NULL;
	}
	Z_Free(g);
	return true;
}


static vfsfile_t *VFSGen_Create(IWeb_FileGen_t *gen)
{
	vfsgen_t *ret;
	ret = Z_Malloc(sizeof(vfsgen_t));

	ret->funcs.ReadBytes = VFSGen_ReadBytes;
	ret->funcs.WriteBytes = VFSGen_WriteBytes;
	ret->funcs.Seek = VFSGen_Seek;
	ret->funcs.Tell = VFSGen_Tell;
	ret->funcs.GetLen = VFSGen_GetLen;
	ret->funcs.Close = VFSGen_Close;

	ret->buffer = gen;
	gen->references++;

	return (vfsfile_t*)ret;
}

vfsfile_t *IWebGenerateFile(const char *name, const char *content, int contentlength)
{
	int fnum;
	const char *parms;
	int len;

	if (!sv.state)
		return NULL;

	if (*lastrecordedmvd && !strcmp(name, "lastdemo.mvd"))
		if (strcmp(name, "lastdemo.mvd"))	//no infinate loops please...
			return FS_OpenVFS(lastrecordedmvd, "rb", FS_GAME);

	parms = strchr(name, '?');
	if (!parms)
		parms = name + strlen(name);
	len = parms-name;
	if (*parms)
		parms++;

	if (!*name)
		return NULL;


	for (fnum = 0; fnum < sizeof(IWebFiles) / sizeof(IWebFile_t); fnum++)
	{
		if (!Q_strncasecmp(name, IWebFiles[fnum].name, len+1))
		{
			if (IWebFiles[fnum].buffer)
			{
				if (IWebFiles[fnum].lastgenerationtime < Sys_DoubleTime())
				{
					IWebFiles[fnum].buffer->references--;		//remove our reference and check free
					if (IWebFiles[fnum].buffer->references<=0)
					{
						BZ_Free(IWebFiles[fnum].buffer);
						IWebFiles[fnum].buffer = NULL;
					}
				}
			}
			if (!IWebFiles[fnum].buffer)
			{
				if (IWeb_GenerationBuffer!=NULL)
					Sys_Error("Recursive file generation\n");
				IWebFiles[fnum].GenerationFunction(parms, content, contentlength);
				IWebFiles[fnum].buffer = IWeb_GenerationBuffer;

				//so it can't be sent once and freed instantly.
				if (contentlength)
					IWebFiles[fnum].lastgenerationtime = Sys_DoubleTime()+10;
			}
			IWebFiles[fnum].buffer->references++;
			IWeb_GenerationBuffer = NULL;

			return VFSGen_Create(IWebFiles[fnum].buffer);
		}
	}
	return NULL;
}

#endif
#endif
