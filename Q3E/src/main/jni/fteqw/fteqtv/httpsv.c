#include "qtv.h"

#define WEBPORTMANIFEST	"https://triptohell.info/demo.fmf"

//#define WEBPORTMANIFEST	"http://localhost:8080/demo.fmf"

//main reason to use connection close is because we're lazy and don't want to give sizes in advance (yes, we could use chunks..)

size_t SHA1(unsigned char *digest, size_t maxdigestsize, const unsigned char *string, size_t stringlen);

void tobase64(unsigned char *out, int outlen, unsigned char *in, int inlen)
{
	static unsigned char tab[64] =
	{
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
	};
	unsigned int usedbits = 0;
	unsigned int val = 0;
	outlen--;
	while(inlen)
	{
		while(usedbits < 24 && inlen)
		{
			val <<= 8;
			val |= (*in++);
			inlen--;
			usedbits += 8;
		}
		if (outlen < 4)
			return;
		val <<= 24 - usedbits;

		*out++ = (usedbits > 0)?tab[(val>>18)&0x3f]:'=';
		*out++ = (usedbits > 6)?tab[(val>>12)&0x3f]:'=';
		*out++ = (usedbits > 12)?tab[(val>>6)&0x3f]:'=';
		*out++ = (usedbits > 18)?tab[(val>>0)&0x3f]:'=';
		val=0;
		usedbits = 0;
	}
	*out = 0;
}

static const char qfont_table[256] = {
	'\0', '#', '#', '#', '#', '.', '#', '#',
	'#', 9, 10, '#', ' ', 13, '.', '.',
	'[', ']', '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', '.', '<', '=', '>',
	' ', '!', '"', '#', '$', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', '{', '|', '}', '~', '<',
 
	'<', '=', '>', '#', '#', '.', '#', '#',
	'#', '#', ' ', '#', ' ', '>', '.', '.',
	'[', ']', '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', '.', '<', '=', '>',
	' ', '!', '"', '#', '$', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', '{', '|', '}', '~', '<'
};

static void HTMLprintf(char *outb, int outl, char *fmt, ...)
{
	va_list val;
	char qfmt[8192*4];
	char *inb = qfmt;
	unsigned char inchar;

	va_start(val, fmt);
	vsnprintf(qfmt, sizeof(qfmt), fmt, val);
	va_end(val);
	qfmt[sizeof(qfmt)-1] = 0;

	outl--;
	outl -= 5;
	while (outl > 0 && *inb)
	{
		inchar = qfont_table[*(unsigned char*)inb];
		if (inchar == '<')
		{
			*outb++ = '&';
			*outb++ = 'l';
			*outb++ = 't';
			*outb++ = ';';
			outl -= 4;
		}
		else if (inchar == '>')
		{
			*outb++ = '&';
			*outb++ = 'g';
			*outb++ = 't';
			*outb++ = ';';
			outl -= 4;
		}
		else if (inchar == '\n')
		{
			*outb++ = '<';
			*outb++ = 'b';
			*outb++ = 'r';
			*outb++ = '/';
			*outb++ = '>';
			outl -= 5;
		}
		else if (inchar == '&')
		{
			*outb++ = '&';
			*outb++ = 'a';
			*outb++ = 'm';
			*outb++ = 'p';
			*outb++ = ';';
			outl -= 5;
		}
		else
		{
			*outb++ = inchar;
			outl -= 1;
		}
		inb++;
	}
	*outb++ = 0;
}

static void HTTPSV_SendHTTPHeader(cluster_t *cluster, oproxy_t *dest, char *error_code, char *content_type, int csize, qboolean nocache)
{
	char *s;
	char buffer[2048];

	if (nocache)
	{
		if (csize >= 0)
		{
			s =	"HTTP/1.1 %s OK\r\n"
				"Content-Type: %s\r\n"
				"Content-Length: %i\r\n"
				"Cache-Control: no-cache, must-revalidate\r\n"
				"Expires: Mon, 26 Jul 1997 05:00:00 GMT\r\n"
				"\r\n";
		}
		else
		{
			s =	"HTTP/1.1 %s OK\r\n"
				"Content-Type: %s\r\n"
				"Cache-Control: no-cache, must-revalidate\r\n"
				"Expires: Mon, 26 Jul 1997 05:00:00 GMT\r\n"
				"Connection: close\r\n"
				"\r\n";
		}
	}
	else
	{
		if (csize >= 0)
		{
			s =	"HTTP/1.1 %s OK\r\n"
				"Cache-Control: public, max-age=3600\r\n"
				"Content-Type: %s\r\n"
				"Content-Length: %i\r\n"
				"\r\n";
		}
		else
		{
			s =	"HTTP/1.1 %s OK\r\n"
				"Cache-Control: public, max-age=3600\r\n"
				"Content-Type: %s\r\n"
				"Connection: close\r\n"
				"\r\n";
		}
	}

	snprintf(buffer, sizeof(buffer), s, error_code, content_type, csize);

	Net_ProxySend(cluster, dest, buffer, strlen(buffer));
}

static void HTTPSV_SendHTMLHeader(cluster_t *cluster, oproxy_t *dest, char *title, char *args)
{
	char *s;
	char buffer[2048];

	qboolean plugin = false;
	while (*args && *args != ' ')
	{
		if (*args == 'p')
			 plugin = true;
		args++;
	}

	s =	"<!DOCTYPE HTML>\n"
		"<html>\n"
		"<head>\n"
		"  <meta http-equiv=\"content-type\" content=\"text/html; charset=iso-8859-1\">\n"
		"  <title>%s</title>\n"
		"  <link rel=\"StyleSheet\" href=\"/style.css\" type=\"text/css\" />\n"
		"</head>\n"
		"<body><div id=\"navigation\"><ul>"
		"<li><a href=\"/nowplaying.html%s\">Live</a></li>"
		"<li><a href=\"/demos.html%s\">Demos</a></li>"
		"%s"
		/*"%s"*/
		"</ul></div>";

	snprintf(buffer, sizeof(buffer), s, title, 

		plugin?"?p":"", 
		plugin?"?p":"", 
		(!*cluster->adminpassword)?"":(plugin?"<li><a href=\"/admin.html?p\">Admin</a></li>":"<li><a href=\"/admin.html\">Admin</a></li>")/*,
		plugin?"<li><a target=\"_top\" href=\"/nowplaying.html\">Basic</a></li>":"<li><a href=\"/plugin.html\">Plugin</a></li>"*/
		
		);

	Net_ProxySend(cluster, dest, buffer, strlen(buffer));
}

static void HTTPSV_SendHTMLFooter(cluster_t *cluster, oproxy_t *dest)
{
	char *s;
	char buffer[2048];

	/*Proxy version*/
	snprintf(buffer, sizeof(buffer), "<br/>Server Version: "QTV_VERSION_STRING" <a href=\""PROXYWEBSITE"\" target=\"_blank\">"PROXYWEBSITE"</a>");
	Net_ProxySend(cluster, dest, buffer, strlen(buffer));

	/*terminate html page*/
	s = "</body>\n"
		"</html>\n";
	Net_ProxySend(cluster, dest, s, strlen(s));
}

#define HTMLPRINT(str) Net_ProxySend(cluster, dest, str "\n", strlen(str "\n"))
#define HTTPPRINT(str) Net_ProxySend(cluster, dest, str, strlen(str))

static void HTTPSV_GenerateNowPlaying(cluster_t *cluster, oproxy_t *dest, char *args)
{
	int player;
	char *s;
	char buffer[1024];
	char plname[64];
	sv_t *streams;
	qboolean plugin = false;
	qboolean activeonly = false;

	HTTPSV_SendHTTPHeader(cluster, dest, "200", "text/html", -1, true);
	HTTPSV_SendHTMLHeader(cluster, dest, "QuakeTV: Now Playing", args);

	while (*args && *args != ' ')
	{
		if (*args == 'p')
			 plugin = true;
		else if (*args == 'a')
			 activeonly = true;
		args++;
	}

	if (!strcmp(cluster->hostname, DEFAULT_HOSTNAME))
		snprintf(buffer, sizeof(buffer), "<h1>QuakeTV: Now Playing</h1>");	//don't show the hostname if its set to the default
	else
		snprintf(buffer, sizeof(buffer), "<h1>%s: Now Playing</h1>", cluster->hostname);
	Net_ProxySend(cluster, dest, buffer, strlen(buffer));

	HTMLPRINT("<dl class=\"nowplaying\">");
	for (streams = cluster->servers; streams; streams = streams->next)
	{
		if (activeonly)
		{
			for (player = 0; player < MAX_CLIENTS; player++)
			{
				if (streams->isconnected && streams->map.thisplayer == player)
					continue;
				if (*streams->map.players[player].userinfo)
				{
					break;
				}
			}
			if (player == MAX_CLIENTS)
				continue;
		}
		HTMLPRINT("<dt>");
		HTMLprintf(buffer, sizeof(buffer), "%s (%s: %s)", streams->server, streams->map.gamedir, streams->map.mapname);
		Net_ProxySend(cluster, dest, buffer, strlen(buffer));
		if (plugin && !strncmp(streams->server, "tcp:", 4))
		{
			snprintf(buffer, sizeof(buffer), "<span class=\"qtvfile\"> [ <a onclick=\"parent.joinstream('%s')\">Join</a> ]</span>", streams->server);
			Net_ProxySend(cluster, dest, buffer, strlen(buffer));
		}
		if (plugin && !strncmp(streams->server, "udp:", 4))
			snprintf(buffer, sizeof(buffer), "<span class=\"qtvfile\"> [ <a href=\"javascript:parent.joinserver('%s')\">Join</a> ]</span>", streams->server+4);
		else
			snprintf(buffer, sizeof(buffer), "<span class=\"qtvfile\"> [ <a href=\"/watch.qtv?sid=%i\">Watch Now</a>]</span>", streams->streamid);
		Net_ProxySend(cluster, dest, buffer, strlen(buffer));
		HTMLPRINT("</dt><dd><ul class=\"playerslist\">");

		for (player = 0; player < MAX_CLIENTS; player++)
		{
			if (streams->isconnected && streams->map.thisplayer == player)
				continue;
			if (*streams->map.players[player].userinfo)
			{
				Info_ValueForKey(streams->map.players[player].userinfo, "name", plname, sizeof(plname));

				if (streams->map.players[player].frags < -90)
				{
					HTMLPRINT("<li class=\"spectator\">");
				}
				else
				{
					HTMLPRINT("<li class=\"player\">");
				}

				HTMLprintf(buffer, sizeof(buffer), "%s", plname);
				Net_ProxySend(cluster, dest, buffer, strlen(buffer));
				HTMLPRINT("</li>");
			}
		}
		HTMLPRINT("</ul></dd>");
	}
	HTMLPRINT("</dl>");
	if (!cluster->servers)
	{
		s = "No streams are currently being played<br />";
		Net_ProxySend(cluster, dest, s, strlen(s));
	}

	HTTPSV_SendHTMLFooter(cluster, dest);
}

static void HTTPSV_GenerateCSSFile(cluster_t *cluster, oproxy_t *dest)
{
    HTTPSV_SendHTTPHeader(cluster, dest, "200", "text/css", -1, false);

    HTMLPRINT("* { font-family: Verdana, Helvetica, sans-serif; }");
    HTMLPRINT("body { color: #000; background-color: #fff; padding: 0 0px; }");
    HTMLPRINT("a { color: #00f; }");
    HTMLPRINT("div.topdiv { display:flex; align-items: stretch; position: absolute; top: 0; right: 0; bottom: 0; left: 0; }");
    HTMLPRINT("div.left { resize: horizontal; overflow: auto; flex 0 0 25%;}");
    HTMLPRINT("div.right { padding:0; margin: 0; flex: auto; }");
    HTMLPRINT("a.qtvfile { font-weight: bold; }");
    HTMLPRINT("a:visited { color: #00f; }");
    HTMLPRINT("a:hover { background-color: black; color: yellow; }");
    HTMLPRINT("li.spectator { color: #666; font-size: 0.9ex; }");
    HTMLPRINT("dl.nowplaying dd { margin: 0 0 2em 0; }");
    HTMLPRINT("dl.nowplaying dt { margin: 1em 0 0 0; font-size: 1.1em; font-weight: bold; }");
    HTMLPRINT("dl.nowplaying li { list-style: none; margin: 0 0 0 1em; padding: 0; }");
    HTMLPRINT("dl.nowplaying ul { margin: 0 0 0 1em; padding: 0; }");
    HTMLPRINT("canvas.emscripten { border: 0px none; padding:0; margin: 0; width: 100%; height: 100%;}");
    HTMLPRINT("#navigation { background-color: #eef; }");
    HTMLPRINT("#navigation li { display: inline; list-style: none; margin: 0 3em; }");
}

static qboolean HTTPSV_GetHeaderField(char *s, char *field, char *buffer, int buffersize)
{
	char *e;
	char *colon;
	int fieldnamelen = strlen(field);

	buffer[0] = 0;

	e = s;
	while(*e)
	{
		if (*e == '\n')
		{
			*e = '\0';
			colon = strchr(s, ':');
			*e = '\n';
			if (!colon)
			{
				if (!strncmp(field, s, fieldnamelen))
				{
					if (s[fieldnamelen] <= ' ')
					{
						return true;
					}
				}
			}
			else
			{
				if (fieldnamelen == colon - s)
				{
					if (!strncmp(field, s, colon-s))
					{
						colon++;
						while (*colon == ' ')
							colon++;
						while (buffersize > 2)
						{
							if (!*colon || *colon == '\r' || *colon == '\n')
								break;
							*buffer++ = *colon++;
							buffersize--;
						}
						*buffer = 0;
						return true;
					}
				}

			}
			s = e+1;
		}

		e++;
	}
	return false;
}
static qboolean HTTPSV_GetHeaderCommaField(char *s, char **ctx, char *field, char *buffer, int buffersize)
{	//some http header fields are a,b,c,d\ne,f type lists. this function will read one token despite them being split between multiple headers.
	char *e;	//end of current line...
	char *colon;
	int fieldnamelen = strlen(field);

	buffer[0] = 0;

	if (*ctx)
		e = *ctx;
	else
		e = s;
	if (*e == ',')
	{
		colon = e;
		goto foundfield;
	}
	while(*e)
	{
		if (*e == '\n')
		{
			*e = '\0';
			colon = strchr(s, ':');
			*e = '\n';
			if (colon)
			{
				if (fieldnamelen == colon - s)
				{
					if (!strncmp(field, s, colon-s))
					{
foundfield:
						colon++;
						while (*colon == ' ')
							colon++;
						while (buffersize > 2)
						{
							if (!*colon || *colon == '\r' || *colon == '\n' || *colon == ',')
								break;
							*buffer++ = *colon++;
							buffersize--;
						}
						*buffer = 0;
						*ctx = colon;
						return true;
					}
				}

			}
			s = e+1;
		}

		e++;
	}
	return false;
}

static void HTTPSV_GenerateQTVStub(cluster_t *cluster, oproxy_t *dest, char *streamtype, char *streamid)
{
	char *s;
	char hostname[128];
	char buffer[1024];


	char fname[256];
	s = fname;
	while (*streamid > ' ')
	{
		if (s > fname + sizeof(fname)-4)	//4 cos I'm too lazy to work out what the actual number should be
			break;
		if (*streamid == '%')
		{
			*s = 0;
			streamid++;
			if (*streamid >= 'a' && *streamid <= 'f')
				*s += 10 + *streamid-'a';
			else if (*streamid >= 'A' && *streamid <= 'F')
				*s += 10 + *streamid-'A';
			else if (*streamid >= '0' && *streamid <= '9')
				*s += *streamid-'0';
			else
				break;

			*s <<= 4;

			streamid++;
			if (*streamid >= 'a' && *streamid <= 'f')
				*s += 10 + *streamid-'a';
			else if (*streamid >= 'A' && *streamid <= 'F')
				*s += 10 + *streamid-'A';
			else if (*streamid >= '0' && *streamid <= '9')
				*s += *streamid-'0';
			else
				break;

			//don't let hackers try adding extra commands to it.
			if (*s == '$' || *s == ';' || *s == '\r' || *s == '\n')
				continue;

			streamid++;
			s++;
		}
		else if (*streamid == '$' || *streamid == ';' || *streamid == '\r' || *streamid == '\n')
		{
			//don't let hackers try adding extra commands to it.
			streamid++;
		}
		else
			*s++ = *streamid++;
	}
	*s = 0;
	streamid = fname;


	if (!HTTPSV_GetHeaderField((char*)dest->inbuffer, "Host", hostname, sizeof(hostname)))
	{
		HTTPSV_SendHTTPHeader(cluster, dest, "400", "text/html", -1, true);
		HTTPSV_SendHTMLHeader(cluster, dest, "QuakeTV: Error", "");

		s = "Your client did not send a Host field, which is required in HTTP/1.1\n<BR />"
			"Please try a different browser.\n"
			"</BODY>"
			"</HTML>";

		Net_ProxySend(cluster, dest, s, strlen(s));
		return;
	}
	/*if there's a port number on there, strip it*/
	if (strchr(hostname, ':'))
		*strchr(hostname, ':') = 0;

	snprintf(buffer, sizeof(buffer), "[QTV]\r\n"
					"Stream: %s%s@%s:%i\r\n"
					"", 
					//5, 		256, 		64.	snprintf is not required, but paranoia is a wonderful thing.
					streamtype, streamid, hostname, cluster->tcplistenportnum);

	HTTPSV_SendHTTPHeader(cluster, dest, "200", "text/x-quaketvident", strlen(buffer), false);
	Net_ProxySend(cluster, dest, buffer, strlen(buffer));
}

static void HTTPSV_GenerateQWSVStub(cluster_t *cluster, oproxy_t *dest, char *method, char *streamid)
{
	char *s;
	char buffer[1024];


	char fname[256];
	s = fname;
	while (*streamid > ' ')
	{
		if (s > fname + sizeof(fname)-4)	//4 cos I'm too lazy to work out what the actual number should be
			break;
		if (*streamid == '%')
		{
			*s = 0;
			streamid++;
			if (*streamid >= 'a' && *streamid <= 'f')
				*s += 10 + *streamid-'a';
			else if (*streamid >= 'A' && *streamid <= 'F')
				*s += 10 + *streamid-'A';
			else if (*streamid >= '0' && *streamid <= '9')
				*s += *streamid-'0';
			else
				break;

			*s <<= 4;

			streamid++;
			if (*streamid <= ' ')
				break;
			else if (*streamid >= 'a' && *streamid <= 'f')
				*s += 10 + *streamid-'a';
			else if (*streamid >= 'A' && *streamid <= 'F')
				*s += 10 + *streamid-'A';
			else if (*streamid >= '0' && *streamid <= '9')
				*s += *streamid-'0';
			else
				break;

			streamid++;
			s++;
		}
		else
			*s++ = *streamid++;
	}
	*s = 0;
	streamid = fname;

	snprintf(buffer, sizeof(buffer), "[QTV]\r\n"
					"%s: %s\r\n"
					"", 
					method, streamid);

	HTTPSV_SendHTTPHeader(cluster, dest, "200", "text/x-quaketvident", strlen(buffer), false);
	Net_ProxySend(cluster, dest, buffer, strlen(buffer));
}

static char *HTTPSV_ParsePOST(char *post, char *buffer, int buffersize)
{
	while(*post && *post != '&')
	{
		if (--buffersize>0)
		{
			if (*post == '+')
				*buffer++ = ' ';
			else if  (*post == '%')
			{
				*buffer = 0;
				post++;
				if (*post == '\0' || *post == '&')
					break;
				else if (*post >= 'a' && *post <= 'f')
					*buffer += 10 + *post-'a';
				else if (*post >= 'A' && *post <= 'F')
					*buffer += 10 + *post-'A';
				else if (*post >= '0' && *post <= '9')
					*buffer += *post-'0';

				*buffer <<= 4;

				post++;
				if (*post == '\0' || *post == '&')
					break;
				else if (*post >= 'a' && *post <= 'f')
					*buffer += 10 + *post-'a';
				else if (*post >= 'A' && *post <= 'F')
					*buffer += 10 + *post-'A';
				else if (*post >= '0' && *post <= '9')
					*buffer += *post-'0';

				buffer++;
			}
			else
				*buffer++ = *post;
		}
		post++;
	}
	*buffer = 0;

	return post;
}
static void HTTPSV_GenerateAdmin(cluster_t *cluster, oproxy_t *dest, int streamid, char *postbody, char *args)
{
	char pwd[64];
	char cmd[256];
	char result[8192];
	char *s;
	char *o;
	int passwordokay = false;

	if (!*cluster->adminpassword)
	{
		HTTPSV_SendHTTPHeader(cluster, dest, "403", "text/html", -1, true);
		HTTPSV_SendHTMLHeader(cluster, dest, "QuakeTV: Admin Error", args);

		s = "The admin password is disabled. You may not log in remotely.</body></html>\n";
		Net_ProxySend(cluster, dest, s, strlen(s));
		return;
	}
		

	pwd[0] = 0;
	cmd[0] = 0;
	if (postbody)
	while (*postbody)
	{
		if (!strncmp(postbody, "pwd=", 4))
		{
			postbody = HTTPSV_ParsePOST(postbody+4, pwd, sizeof(pwd));
		}
		else if (!strncmp(postbody, "cmd=", 4))
		{
			postbody = HTTPSV_ParsePOST(postbody+4, cmd, sizeof(cmd));
		}
		else
		{
			while(*postbody && *postbody != '&')
			{
				postbody++;
			}
			if (*postbody == '&')
				postbody++;
		}
	}

	if (!*pwd)
	{
		if (postbody)
			o = "No Password";
		else
			o = "";
	}
	else if (!strcmp(pwd, cluster->adminpassword))
	{
		passwordokay = true;
		//small hack (as http connections are considered non-connected proxies)
		cluster->numproxies--;
		if (*cmd)
			o = Rcon_Command(cluster, NULL, cmd, result, sizeof(result), false);
		else
			o = "";
		cluster->numproxies++;
	}
	else
	{
		o = "Bad Password";
	}
	if (o != result)
	{
		strlcpy(result, o, sizeof(result));
		o = result;
	}

	HTTPSV_SendHTTPHeader(cluster, dest, "200", "text/html", -1, true);
	HTTPSV_SendHTMLHeader(cluster, dest, "QuakeTV: Admin", args);

	s = "<H1>QuakeTV Admin: ";
	Net_ProxySend(cluster, dest, s, strlen(s));
	s = cluster->hostname;
	Net_ProxySend(cluster, dest, s, strlen(s));
	s = "</H1>";
	Net_ProxySend(cluster, dest, s, strlen(s));


	s =	"<FORM action=\"admin.html\" method=\"post\" name=f>"
		"<CENTER>"
		"Password <input name=pwd value=\"";

	Net_ProxySend(cluster, dest, s, strlen(s));
	if (passwordokay)
		Net_ProxySend(cluster, dest, pwd, strlen(pwd));

			
	s =	"\">"
		"<BR />"
		"Command <input name=cmd maxsize=255 size=40 value=\"\">"
		"<input type=submit value=\"Submit\" name=btn>"
		"</CENTER>"
		"</FORM>";
	Net_ProxySend(cluster, dest, s, strlen(s));

	if (passwordokay)
		HTMLPRINT("<script>document.forms[0].elements[1].focus();</script>");
	else
		HTMLPRINT("<script>document.forms[0].elements[0].focus();</script>");

	while(*o)
	{
		s = strchr(o, '\n');
		if (s)
			*s = 0;
		HTMLprintf(cmd, sizeof(cmd), "%s", o);
		Net_ProxySend(cluster, dest, cmd, strlen(cmd));
		Net_ProxySend(cluster, dest, "<BR />", 6);
		if (!s)
			break;
		o = s+1;
	}

	HTTPSV_SendHTMLFooter(cluster, dest);
}

static void HTTPSV_GenerateDemoListing(cluster_t *cluster, oproxy_t *dest, char *args)
{
	int i;
	char link[512];
	char *s;
	qboolean plugframe = false;
	for (s=args; *s && *s != ' ';)
	{
		if (*s == 'p')
			 plugframe = true;
		s++;
	}

	HTTPSV_SendHTTPHeader(cluster, dest, "200", "text/html", -1, true);
	HTTPSV_SendHTMLHeader(cluster, dest, "QuakeTV: Demos", args);

	s = "<h1>QuakeTV: Demo Listing</h1>";
	Net_ProxySend(cluster, dest, s, strlen(s));

	Cluster_BuildAvailableDemoList(cluster);
	for (i = 0; i < cluster->availdemoscount; i++)
	{
		if (plugframe)
		{
			snprintf(link, sizeof(link), "<A HREF=\"javascript:parent.playdemo('%s')\">%s</A> (%ikb)<br/>", cluster->availdemos[i].name, cluster->availdemos[i].name, cluster->availdemos[i].size/1024);
		}
		else
		{
			snprintf(link, sizeof(link), "<A HREF=\"/watch.qtv?demo=%s\">%s</A> (%ikb)<br/>", cluster->availdemos[i].name, cluster->availdemos[i].name, cluster->availdemos[i].size/1024);
		}
		Net_ProxySend(cluster, dest, link, strlen(link));
	}

	snprintf(link, sizeof(link), "<P>Total: %i demos</P>", cluster->availdemoscount);
	Net_ProxySend(cluster, dest, link, strlen(link));

	HTTPSV_SendHTMLFooter(cluster, dest);
}

static void HTTPSV_GeneratePlugin(cluster_t *cluster, oproxy_t *dest)
{
	char hostname[1024];
	char *html;
	if (!HTTPSV_GetHeaderField((char*)dest->inbuffer, "Host", hostname, sizeof(hostname)))
	{
		HTTPSV_SendHTTPHeader(cluster, dest, "400", "text/html", -1, true);
		HTTPSV_SendHTMLHeader(cluster, dest, "QuakeTV: Error", "p");

		html = "Your client did not send a Host field, which is required in HTTP/1.1\n<BR />"
			"Please try a different browser.\n"
			"</BODY>"
			"</HTML>";

		Net_ProxySend(cluster, dest, html, strlen(html));
		return;
	}

	html = "<!DOCTYPE HTML>\n"
		"<HTML><HEAD><TITLE>QuakeTV With Plugin</TITLE>"
				"  <link rel=\"StyleSheet\" href=\"/style.css\" type=\"text/css\" />\n"
				"<meta charset='utf-8' />"
				"</HEAD><body>"
	"<div class='topdiv'>"
		"<div class='left' id=optdiv>"
					"<iframe frameborder=0 src=\"nowplaying.html?p\" width=\"100%\" height=\"100%\">"
					"oh dear. your browser doesn't support this site"
					"</iframe>"
		"</div>"
		"<div class='right' id=plugdiv>"
			"<canvas class='emscripten' id='canvas' oncontextmenu='event.preventDefault()'>Canvas not supported</canvas>"
		"</div>"
	"</div>"
	"<script type='text/javascript'>"
		"var Module = {"
			"canvas: document.getElementById('canvas'),"
			"print: function(msg)"
			"{"
				"console.log(msg);"
			"},"
			"printErr: function(text)"
			"{"
				"if (text.substr(0, 28) == 'Cannot enlarge memory arrays')"
					"alert('Memory full/fragmented. Please reload the page.');"
				"else "
					"console.log(text);"
			"},"
			"arguments: ["
				"'-nohome',"
				"'-manifest',"
				"'"WEBPORTMANIFEST"',"
				"'+connect',"
				"(window.location.protocol=='https:'?'wss://':'ws://')+window.location.host"
			"]"
		"};"

		//create the script object
		"var s = document.createElement('script');"
		// set it up
		"s.setAttribute('src','/ftewebgl.js');"
		"s.setAttribute('type','text/javascript');"
		"s.setAttribute('charset','utf-8');"
		"s.addEventListener('error', function() {alert('Error loading game script.');}, false);"
		// add to DOM
		"document.head.appendChild(s);"

		//set up some functions that the embedded stuff can use
		"parent.joinserver = function(d)"
		"{"
			"d = '\\nconnect '+d+'\\n';"
			"FTEC.cbufadd(d);"
		"}\n"
		"parent.joinstream = function(d)"
		"{"
			"joinserver((window.location.protocol=='https:'?'wss://':'ws://')+window.location.host+'/'+d);"
		"}\n"
		"parent.playdemo = function(d)"
		"{"
			"alert('playdemo'+d);"
		"}\n"

	"</script>"
	"<noscript>"
		"It looks like you have javascript disabled.<br/>"
		"If you want to run a javascript port of a game engine, it helps to have javascript enabled. Just saying.<br/>"
		"<br/>"
	"</noscript>"

	"</body></HTML>";

	HTTPSV_SendHTTPHeader(cluster, dest, "200", "text/html", strlen(html), true);
	Net_ProxySend(cluster, dest, html, strlen(html));
}

static void HTTPSV_GenerateDownload(cluster_t *cluster, oproxy_t *dest, char *filename, char *svroot, char *redir)
{
	char fname[256];
	char link[512];
	char *s, *suppliedname;
	int len;

	char *mime = "application/x-forcedownload";
	if (!strcmp(filename, "/ftewebgl.wasm"))
		mime = "application/wasm";
	else if (!strcmp(filename, "/ftewebgl.js"))
		mime = "text/javascript";

	if (!svroot || !*svroot)
	{
		if (redir)
		{
			HTTPPRINT("HTTP/1.0 302 Found\nLocation: ");
			HTTPPRINT(redir);
			HTTPPRINT("\n\n");
			return;
		}
		HTTPSV_SendHTTPHeader(cluster, dest, "403", "text/html", -1, true);
		HTTPSV_SendHTMLHeader(cluster, dest, "Permission denied", "");
		HTMLPRINT("<h1>403: Forbidden</h1>");
		HTMLPRINT("File downloads from this proxy are currently not permitted.");
		HTTPSV_SendHTMLFooter(cluster, dest);
		return;
	}

	suppliedname = s = fname + strlcpy(fname, svroot, sizeof(fname));
	while (*filename > ' ')
	{
		if (s > fname + sizeof(fname)-4)	//4 cos I'm too lazy to work out what the actual number should be
			break;
		if (*filename == '%')
		{
			*s = 0;
			filename++;
			if (*filename <= ' ')
				break;
			else if (*filename >= 'a' && *filename <= 'f')
				*s += 10 + *filename-'a';
			else if (*filename >= 'A' && *filename <= 'F')
				*s += 10 + *filename-'A';
			else if (*filename >= '0' && *filename <= '9')
				*s += *filename-'0';

			*s <<= 4;

			filename++;
			if (*filename <= ' ')
				break;
			else if (*filename >= 'a' && *filename <= 'f')
				*s += 10 + *filename-'a';
			else if (*filename >= 'A' && *filename <= 'F')
				*s += 10 + *filename-'A';
			else if (*filename >= '0' && *filename <= '9')
				*s += *filename-'0';

			s++;
		}
		else
			*s++ = *filename++;
	}
	*s = 0;

	if (!redir && (*suppliedname == '\\' || *suppliedname == '/' || strstr(suppliedname, "..") || strstr(suppliedname, "//") || strstr(suppliedname, "\\\\") || strchr(suppliedname, ':')))
	{
		HTTPSV_SendHTTPHeader(cluster, dest, "403", "text/html", -1, true);
		HTTPSV_SendHTMLHeader(cluster, dest, "Permission denied", "");
		HTMLPRINT("<h1>403: Forbidden</h1>");

		HTMLPRINT("<p>");
		HTMLprintf(link, sizeof(link), "The filename '%s' names an absolute path.", suppliedname);
		Net_ProxySend(cluster, dest, link, strlen(link));
		HTMLPRINT("</p>");
		return;
	}
	len = strlen(fname);
	if (len > 4)
	{
		/*protect id's content (prevent downloading of bsps from pak files - we don't do pak files so just prevent the entire pak)*/
		if (!stricmp(link+len-4, ".pak"))
		{
			HTTPSV_SendHTTPHeader(cluster, dest, "403", "text/html", -1, true);
			HTTPSV_SendHTMLHeader(cluster, dest, "Permission denied", "");
			HTMLPRINT("<h1>403: Forbidden</h1>");
		
			HTMLPRINT("<p>");
			HTMLprintf(link, sizeof(link), "Pak files may not be downloaded.", suppliedname);
			Net_ProxySend(cluster, dest, link, strlen(link));
			HTMLPRINT("</p>");
			return;
		}		
	}

	printf("Download request for %s\n", fname);
	dest->srcfile = fopen(fname, "rb");

	if (dest->srcfile)
	{
		long size;
		fseek(dest->srcfile, 0, SEEK_END);
		size = ftell(dest->srcfile);
		fseek(dest->srcfile, 0, SEEK_SET);
		HTTPSV_SendHTTPHeader(cluster, dest, "200", mime, size, false);
	}
	else if (redir)
	{
		HTTPPRINT("HTTP/1.0 302 Found\nLocation: ");
		HTTPPRINT(redir);
		HTTPPRINT("\n\n");
	}
	else
	{
		HTTPSV_SendHTTPHeader(cluster, dest, "404", "text/html", -1, true);
		HTTPSV_SendHTMLHeader(cluster, dest, "File not found", "");
		HTMLPRINT("<h1>404: File not found</h1>");

		HTMLPRINT("<p>");
		HTMLprintf(link, sizeof(link), "The file '%s' could not be found on this server", suppliedname);
		Net_ProxySend(cluster, dest, link, strlen(link));
		HTMLPRINT("</p>");

		HTTPSV_SendHTMLFooter(cluster, dest);
	}
}




static qboolean urimatch(char *uri, char *match, int urilen)
{
	int mlen = strlen(match);
	if (urilen < mlen)
		return false;
	if (strncmp(uri, match, mlen))
		return false;
	if (urilen == mlen)
		return true;
	if (uri[mlen] == '?')
		return true;
	return false;
}
static qboolean uriargmatch(char *uri, char *match, int urilen, char **args)
{
	int mlen = strlen(match);
	if (urilen < mlen)
		return false;
	if (strncmp(uri, match, mlen))
		return false;
	if (urilen == mlen)
	{
		*args = "";
		return true;
	}
	if (uri[mlen] == '?')
	{
		*args = uri+mlen+1;
		return true;
	}
	return false;
}


void HTTPSV_PostMethod(cluster_t *cluster, oproxy_t *pend, char *postdata)
{
	char tempbuf[512];
	char *s;
	int len;

	char *uri, *uriend;
	char *args;
	int urilen;
	uri = pend->inbuffer+4;
	while (*uri == ' ')
		uri++;
	uriend = strchr(uri, '\n');
	s = strchr(uri, ' ');
	if (s && s < uriend)
		uriend = s;
	urilen = uriend - uri;

	if (!HTTPSV_GetHeaderField((char*)pend->inbuffer, "Content-Length", tempbuf, sizeof(tempbuf)))
	{
		s = "HTTP/1.1 411 OK\n"
			"Content-Type: text/html\n"
			"Connection: close\n"
			"\n"
			"<html><HEAD><TITLE>QuakeTV</TITLE></HEAD><BODY>No Content-Length was provided.</BODY>\n";
		Net_ProxySend(cluster, pend, s, strlen(s));
		pend->flushing = true;
		return;
	}
	len = atoi(tempbuf);
	if (pend->inbuffersize + len >= sizeof(pend->inbuffer)-20)
	{	//too much data
		pend->flushing = true;
		return;
	}
	len = postdata - (char*)pend->inbuffer + len;
	if (len > pend->inbuffersize)
		return;	//still need the body

//	if (len <= pend->inbuffersize)
	{
		if (uriargmatch(uri, "/admin.html", urilen, &args))
		{
			HTTPSV_GenerateAdmin(cluster, pend, 0, postdata, args);
		}
		else
		{
			s = "HTTP/1.1 404 OK\n"
				"Content-Type: text/html\n"
				"Connection: close\n"
				"\n"
				"<html><HEAD><TITLE>QuakeTV</TITLE></HEAD><BODY>That HTTP method is not supported for that URL.</BODY></html>\n";
			Net_ProxySend(cluster, pend, s, strlen(s));
	
		}
		pend->flushing = true;
		return;
	}
}

#define REDIRECTIF(uri_,url_)	\
	if (urimatch(uri, uri_, urilen))	\
	{	\
		s = "HTTP/1.0 302 Found\n"	\
			"Location: " url_ "\n"	\
			"\n";	\
		Net_ProxySend(cluster, pend, s, strlen(s));	\
	}
#define REDIRECTIFNEEDED(uri_,url_)	\
	if (urimatch(uri, uri_, urilen))	\
	{	\
		HTTPSV_GenerateDownload(cluster, pend, uri_, cluster->downloaddir, url_);\
	}

char *HTTPSV_GetMethod(cluster_t *cluster, oproxy_t *pend)
{
	char connection[64];
	char upgrade[64];

	char *s;
	char *uri, *uriend;
	char *args;
	int urilen;
	uri = pend->inbuffer+4;
	while (*uri == ' ')
		uri++;
	uriend = strchr(uri, '\n');
	s = strchr(uri, ' ');
	if (s && s < uriend)
		uriend = s;
	urilen = uriend - uri;

	if (!pend->websocket.websocket && uri[0] == '/' && HTTPSV_GetHeaderField((char*)pend->inbuffer, "Connection", connection, sizeof(connection)) && strstr(connection, "Upgrade"))
	{
		if (HTTPSV_GetHeaderField((char*)pend->inbuffer, "Upgrade", upgrade, sizeof(upgrade)) && !stricmp(upgrade, "websocket"))
		{
			char wsprot[64];
			char ver[64];
			char key[64];
			char *ctx;
			HTTPSV_GetHeaderField((char*)pend->inbuffer, "Sec-WebSocket-Key", key, sizeof(key));

			for (ctx = NULL; HTTPSV_GetHeaderCommaField((char*)pend->inbuffer, &ctx, "Sec-WebSocket-Protocol", wsprot, sizeof(wsprot)); )
			{
				//if (!strcmp(wsprot, "quake"))	//webquake. we don't support this! (no OOB and missing header flags and some screwy sequence numbers)
				if (!strcmp(wsprot, "fteqw") ||	//as a client
					(!strcmp(wsprot, "faketcp") && urilen==1&&!strncmp(uri,"/",1)))	//as a qtv proxy (eztv style, but websocked). we are NOT proxying tcp. require a qtv handshake over the resulting websocket connection.
					break;	//break out on the first one we know. this is the recommended way...
			}

			if (!*wsprot)
			{
		#define dest pend
				HTTPSV_SendHTTPHeader(cluster, pend, "404", "text/html", -1, true);
				HTTPSV_SendHTMLHeader(cluster, pend, "Websocket SubProtocol not recognised", "");
				HTMLPRINT("<h1>Websocket SubProtocol not recognised</h1>");
				HTTPSV_SendHTMLFooter(cluster, pend);
			}
			else if (HTTPSV_GetHeaderField((char*)pend->inbuffer, "Sec-WebSocket-Version", ver, sizeof(ver)) && atoi(ver) != 13)
			{
				s =	"HTTP/1.1 426 Upgrade Required\r\n"
					"Sec-WebSocket-Version: 13\r\n"
					"Access-Control-Allow-Origin: *\r\n"	//allow cross-origin requests. this means you can use any domain to play on any public server.
					"\r\n";
				Net_ProxySend(cluster, pend, s, strlen(s));
				return NULL;
			}
			else
			{
				char acceptkey[20*2];
				unsigned char sha1digest[20];
				char padkey[512];
				snprintf(padkey, sizeof(padkey), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);
				tobase64(acceptkey, sizeof(acceptkey), sha1digest, CalcHash(&hash_sha1, sha1digest, sizeof(sha1digest), padkey, strlen(padkey)));

				snprintf(padkey, sizeof(padkey), 
							"HTTP/1.1 101 Switching Protocols\r\n"
							"Upgrade: websocket\r\n"
							"Connection: Upgrade\r\n"
							"Access-Control-Allow-Origin: *\r\n"	//allow cross-origin requests. this means you can use any domain to play on any public server.
							"Sec-WebSocket-Accept: %s\r\n"
							"Sec-WebSocket-Protocol: %s\r\n"
							"\r\n", acceptkey, wsprot);
				//send the websocket handshake response.
				Net_ProxySend(cluster, pend, padkey, strlen(padkey));
				pend->websocket.websocket = true;

				pend->droptime = cluster->curtime + 20*1000;

				printf("websocket upgrade\n");
				pend->drop = false;

				uri[urilen] = 0;	//make sure its null terminated
				if (!strcmp(wsprot, "faketcp"))
					return NULL;	//we're using websockets, don't treat it as a client
				else
					return strdup(uri+1);	//its a client, track the url as the initial stream.
			}
			return NULL;
		}
	}

	if (urimatch(uri, "/plugin.html", urilen) || urimatch(uri, "/", urilen))
	{
		HTTPSV_GeneratePlugin(cluster, pend);
	}
	else if (uriargmatch(uri, "/nowplaying.html", urilen, &args))
	{
		HTTPSV_GenerateNowPlaying(cluster, pend, args);
	}
	else if (!strncmp(uri, "/watch.qtv?sid=", 15))
	{
		HTTPSV_GenerateQTVStub(cluster, pend, "", (char*)pend->inbuffer+19);
	}
	else if (!strncmp(uri, "/watch.qtv?demo=", 16))
	{
		HTTPSV_GenerateQTVStub(cluster, pend, "file:", (char*)pend->inbuffer+20);
	}
	else if (!strncmp(uri, "/watch.qtv?join=", 16))
	{
		HTTPSV_GenerateQWSVStub(cluster, pend, "Join", (char*)pend->inbuffer+16);
	}
	else if (!strncmp(uri, "/watch.qtv?obsv=", 16))
	{
		HTTPSV_GenerateQWSVStub(cluster, pend, "Observe", (char*)pend->inbuffer+16);
	}
//	else if (!strncmp((char*)pend->inbuffer+4, "/demo/", 6))
//	{	//fixme: make this send the demo as an http download
//		HTTPSV_GenerateQTVStub(cluster, pend, "file:", (char*)pend->inbuffer+10);
//	}
	else if (uriargmatch(uri, "/admin.html", urilen, &args))
	{
		HTTPSV_GenerateAdmin(cluster, pend, 0, NULL, args);
	}
#if defined(_DEBUG) || defined(DEBUG)
	else REDIRECTIF("/", "/plugin.html")
#endif
	else REDIRECTIF("/", "/nowplaying.html")
	else REDIRECTIFNEEDED("/about.html", PROXYWEBSITE)
	else REDIRECTIFNEEDED("/favicon.ico",	"https://fte.triptohell.info/favicon.ico")
	else REDIRECTIFNEEDED("/ftewebgl.js",	"https://fte.triptohell.info/ftewebgl.js")
	else REDIRECTIFNEEDED("/ftewebgl.wasm",	"https://fte.triptohell.info/ftewebgl.wasm")
	else if (uriargmatch(uri, "/demos.html", urilen, &args))
	{
		HTTPSV_GenerateDemoListing(cluster, pend, args);
	}
	else if (!strncmp((char*)pend->inbuffer+4, "/file/", 6))
	{
		HTTPSV_GenerateDownload(cluster, pend, (char*)pend->inbuffer+10, cluster->downloaddir, NULL);
	}
	else if (urimatch(uri, "/style.css", urilen))
	{
		HTTPSV_GenerateCSSFile(cluster, pend);
	}
	else
	{
#define dest pend
		HTTPSV_SendHTTPHeader(cluster, dest, "404", "text/html", -1, true);
		HTTPSV_SendHTMLHeader(cluster, dest, "Address not recognised", "");
		HTMLPRINT("<h1>Address not recognised</h1>");
		HTTPSV_SendHTMLFooter(cluster, dest);
	}
	return NULL;
}
