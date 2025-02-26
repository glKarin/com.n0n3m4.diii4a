//Released under the terms of the gpl as this file uses a bit of quake derived code. All sections of the like are marked as such
// changes name to while in channel
// mode command
// Spike can you implement nick tab completion. ~moodles
// need option for whois on receiving PM
// bug: setting channel to private, crashes fte when trying to join it.
// http://www.mirc.net/raws/
// http://www.ircle.com/reference/commands.shtml


#include "../plugin.h"
static plugsubconsolefuncs_t *confuncs;
static plugfsfuncs_t *filefuncs;
static plugnetfuncs_t *netfuncs;
static plug2dfuncs_t *drawfuncs;
#include <time.h>
#include <ctype.h>
#include "../../engine/common/netinc.h"

#define handleisvalid(h) ((h)>=0)
#define invalid_handle -1

enum tlsmode_e
{
	TLS_OFF,
	TLS_INITIAL,	//tls only
	TLS_START,		//tls upgrade

	TLS_STARTING	//don't send any nick/user/pass info while this is set
};

static cvar_t	*irc_debug;// = {"irc_debug", "0", irccvars, 0};
static cvar_t	*irc_motd;// = {"irc_motd", "0", irccvars, 0};
static cvar_t	*irc_nick;// = {"irc_nick", "", irccvars, 0};
static cvar_t	*irc_altnick;// = {"irc_altnick", "", irccvars, 0};
static cvar_t	*irc_realname;// = {"irc_realname", "FTE IRC-Plugin", irccvars, 0};
static cvar_t	*irc_hostname;// = {"irc_hostname", "localhost", irccvars, 0};
static cvar_t	*irc_username;// = {"irc_username", "FTE", irccvars, 0};
static cvar_t	*irc_timestamp;// = {"irc_timestamp", "0", irccvars, 0};
static cvar_t	*irc_quitmessage;// = {"irc_quitmessage", "", irccvars, 0};
static cvar_t	*irc_config;// = {"irc_config", "1", irccvars, 0};

static icefuncs_t *piceapi;
static int next_window_x;
static int next_window_y;
static qboolean reloadconfig;
//static char commandname[64]; // belongs to magic tokenizer
static char subvar[9][1000]; // etghack
static char casevar[9][1000]; //numbered_command
//static char servername[64]; // store server name
#define CURRENTCONSOLE "" // need to make this the current console
#define DEFAULTCONSOLE ""
#define COMMANDNAME "irc"

#if defined(SVNREVISION)
	#define RELEASE STRINGIFY(SVNREVISION)
#else
	#define RELEASE __DATE__
#endif

static struct
{
	int width;
	int height;
} pvid;
static void QDECL IRC_UpdateVideo(int width, int height, qboolean restarted)
{
	pvid.width = width;
	pvid.height = height;
}

static qboolean (*Con_TrySubPrint)(const char *subname, const char *text);
static qboolean Con_FakeSubPrint(const char *subname, const char *text)
{
	plugfuncs->Print(text);
	return true;
}

//porting zone:


	#define COLOURGREEN	"^2"
	#define COLORWHITE "^7"
	#define COLOURWHITE "^7" // word
	#define COLOURRED "^1"
	#define COLOURYELLOW "^3"
	#define COLOURPURPLE "^5"
	#define COLOURBLUE "^4"
	#define COLOURINDIGO "^6"


	#define IRC_Malloc	malloc
	#define IRC_Free		free
#undef COM_Parse
	static char *COM_Parse (char *data, char *token_out, int token_maxlen)	//this is taken out of quake
	{
		int		c;
		int		len;

		len = 0;
		token_out[0] = 0;

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
			data++;
			while (1)
			{
				if (len >= token_maxlen-1)
					return data;

				c = *data++;
				if (c=='\"' || !c)
				{
					token_out[len] = 0;
					return data;
				}
				token_out[len] = c;
				len++;
			}
		}

	// parse a regular word
		do
		{
			if (len >= token_maxlen-1)
				return data;

			token_out[len] = c;
			data++;
			len++;
			c = *data;
		} while (c>32);

		token_out[len] = 0;
		return data;
	}










//\r\n is used to end a line.
//meaning \0s are valid.
//but never used cos it breaks strings



#define IRC_MAXNICKLEN 32	//9 and a null term
#define IRC_MAXMSGLEN 512
#define IRC_MAXOUTBUFFER (IRC_MAXMSGLEN*16)


typedef struct ircclient_s {
	struct ircclient_s *next;

	char id[64];	//used for console prints, so we can match up consoles and clients.

	char server[64];
	int port;

	qhandle_t socket;

	enum tlsmode_e tlsmode;
	qboolean quitting;
	qboolean connecting;
	char nick[IRC_MAXNICKLEN];	//nick that we're actually using
	size_t nicktries;			//so we can cycle nicks till we get one that works.

	qboolean persist;			//server connection is persistent across restarts
	char primarynick[IRC_MAXNICKLEN];	//primary nick the connection was configured with
	char pwd[128];				//server password
	char realname[128];			//this is your descriptive OS user account... supposedly.
	char username[128];			//this is your unique OS user name... supposedly.
	char hostname[128];			//this is your OS hostname... supposedly.
	char autochannels[256];		//"#chan,pwd #foo,bar #fred #splodge" for four channnels, two with a password

	char defaultdest[IRC_MAXNICKLEN];//channel or nick

	char bufferedinmessage[IRC_MAXMSGLEN+1];	//there is a max size for protocol. (conveinient eh?) (and it's text format)
	int bufferedinammount;


	char bufferedoutmessage[IRC_MAXOUTBUFFER+1];	//there is a max size for protocol. (conveinient eh?) (and it's text format)
	int bufferedoutammount;

	struct ircice_s
	{
		struct ircice_s *next;

		enum iceproto_e type;
		char peer[IRC_MAXNICKLEN];
		qboolean host;		//the host is the person that initiated the call/etc. they're the ones responsible for resolving deadlocks.
		qboolean allowed;	//user allowed it. woot.
		qboolean accepted;	//peer accepted it. connection is active.

		struct icestate_s *ice;
	} *ice;
} ircclient_t;
static ircclient_t *ircclients;


static void IRC_SetFooter(ircclient_t *irc, const char *subname, const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];
	char lwr[128];
	int i;
	const char *channame = subname;

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format,argptr);
	va_end (argptr);

	if (irc)
	{
		Q_strlcpy(lwr, irc->id, sizeof(lwr));
		for (i = strlen(lwr); *subname && i < sizeof(lwr)-2; i++, subname++)
		{
			if (*subname >= 'A' && *subname <= 'Z')
				lwr[i] = *subname - 'A' + 'a';
			else
				lwr[i] = *subname;
		}
	
		lwr[i] = '\0';

		if (confuncs && confuncs->GetConsoleFloat(lwr, "iswindow") < true)
		{
			confuncs->SetConsoleString(lwr, "title", *channame?channame:irc->server);
			confuncs->SetConsoleString(lwr, "prompt", va("[^1%s^7]: ", irc->nick));
			confuncs->SetConsoleFloat(lwr, "iswindow", 2);
			confuncs->SetConsoleFloat(lwr, "forceutf8", true);
			confuncs->SetConsoleFloat(lwr, "wnd_w", 256);
			confuncs->SetConsoleFloat(lwr, "wnd_h", 320);

			//lame, but whatever.
			if (next_window_x + 256 > pvid.width)
			{
				next_window_x = 0;
				next_window_y += 320;
				if (next_window_y + 320 > pvid.height)
					next_window_y = 0;
			}
			confuncs->SetConsoleFloat(lwr, "wnd_x", next_window_x);
			confuncs->SetConsoleFloat(lwr, "wnd_y", next_window_y);
			next_window_x += 256;
		}

		if (confuncs)
			confuncs->SetConsoleString(lwr, "footer", string);
	}
}
static qboolean IRC_WindowShown(ircclient_t *irc, const char *subname)
{
	char lwr[128];
	int i;
	if (irc)
	{
		Q_strlcpy(lwr, irc->id, sizeof(lwr));
		for (i = strlen(lwr); *subname && i < sizeof(lwr)-2; i++, subname++)
		{
			if (*subname >= 'A' && *subname <= 'Z')
				lwr[i] = *subname - 'A' + 'a';
			else
				lwr[i] = *subname;
		}
	
		lwr[i] = '\0';

		if (confuncs && confuncs->GetConsoleFloat(lwr, "iswindow") < true)
			return false;
	}
	return true;
}
static void IRC_Printf(ircclient_t *irc, const char *subname, const char *format, ...) LIKEPRINTF(3);
static void IRC_Printf(ircclient_t *irc, const char *subname, const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];
	char lwr[128];
	int i;
	const char *channame = subname;

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format,argptr);
	va_end (argptr);

	if (!irc)
		plugfuncs->Print(string);
	else
	{
		Q_strlcpy(lwr, irc->id, sizeof(lwr));
		for (i = strlen(lwr); *subname && i < sizeof(lwr)-2; i++, subname++)
		{
			if (*subname >= 'A' && *subname <= 'Z')
				lwr[i] = *subname - 'A' + 'a';
			else
				lwr[i] = *subname;
		}
	
		lwr[i] = '\0';

		if (confuncs && confuncs->GetConsoleFloat(lwr, "iswindow") < true)
		{
			confuncs->SetConsoleString(lwr, "title", *channame?channame:irc->server);
			confuncs->SetConsoleString(lwr, "prompt", va("[^1%s^7]: ", irc->nick));
			confuncs->SetConsoleFloat(lwr, "iswindow", 2);
			confuncs->SetConsoleFloat(lwr, "forceutf8", true);
			confuncs->SetConsoleFloat(lwr, "wnd_w", 256);
			confuncs->SetConsoleFloat(lwr, "wnd_h", 320);

			//lame, but whatever.
			if (next_window_x + 256 > pvid.width)
			{
				next_window_x = 0;
				next_window_y += 320;
				if (next_window_y + 320 > pvid.height)
					next_window_y = 0;
			}
			confuncs->SetConsoleFloat(lwr, "wnd_x", next_window_x);
			confuncs->SetConsoleFloat(lwr, "wnd_y", next_window_y);
			next_window_x += 256;
		}
		if (!*string)
			confuncs->SetActive(lwr);

		Con_TrySubPrint(lwr, string);
	}
}




static void IRC_InitCvars(void)
{
	const char *cvargroup = "IRC Console Variables";
	irc_debug		= cvarfuncs->GetNVFDG("irc_debug", "0", 0, NULL, cvargroup);
	irc_motd		= cvarfuncs->GetNVFDG("irc_motd", "0", 0, NULL, cvargroup);
	irc_nick		= cvarfuncs->GetNVFDG("irc_nick", "", 0, NULL, cvargroup);
	irc_altnick		= cvarfuncs->GetNVFDG("irc_altnick", "", 0, NULL, cvargroup);
	irc_realname	= cvarfuncs->GetNVFDG("irc_realname", "FTE IRC-Plugin", 0, NULL, cvargroup);
	irc_hostname	= cvarfuncs->GetNVFDG("irc_hostname", "localhost", 0, NULL, cvargroup);
	irc_username	= cvarfuncs->GetNVFDG("irc_username", "FTE", 0, NULL, cvargroup);
	irc_timestamp	= cvarfuncs->GetNVFDG("irc_timestamp", "0", 0, NULL, cvargroup);
	irc_quitmessage	= cvarfuncs->GetNVFDG("irc_quitmessage", "", 0, NULL, cvargroup);
	irc_config		= cvarfuncs->GetNVFDG("irc_config", "1", 0, NULL, cvargroup);
}

void IRC_Command(ircclient_t *ircclient, char *dest, char *args);
void IRC_ExecuteCommand_f(void);
int IRC_ConExecuteCommand(qboolean isinsecure);
void IRC_Frame(double realtime, double gametime);
qboolean IRC_ConsoleLink(void);

qboolean Plug_Init(void)
{
	confuncs = plugfuncs->GetEngineInterface(plugsubconsolefuncs_name, sizeof(*confuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	netfuncs = plugfuncs->GetEngineInterface(plugnetfuncs_name, sizeof(*netfuncs));
	drawfuncs = plugfuncs->GetEngineInterface(plug2dfuncs_name, sizeof(*drawfuncs));
	piceapi = plugfuncs->GetEngineInterface(ICE_API_CURRENT, sizeof(*piceapi));
	plugfuncs->ExportFunction("UpdateVideo", IRC_UpdateVideo);

	if (netfuncs &&
		filefuncs &&
		plugfuncs->ExportFunction("Tick", IRC_Frame))
	{
		cmdfuncs->AddCommand(COMMANDNAME, IRC_ExecuteCommand_f, "Internet Relay Chat client, use ^[/"COMMANDNAME" /help^] for help");

		plugfuncs->ExportFunction("ConsoleLink", IRC_ConsoleLink);
		if (!confuncs || !plugfuncs->ExportFunction("ConExecuteCommand", IRC_ConExecuteCommand))
			Con_TrySubPrint = Con_FakeSubPrint;
		else
			Con_TrySubPrint = confuncs->SubPrint;

		reloadconfig = true;
		IRC_InitCvars();

		return true;
	}
	else
	{
		plugfuncs->Print("IRC Client Plugin failed\n");
	}
	return false;
}



void IRC_ExecuteCommand_f(void)
{
	ircclient_t *ircclient = ircclients;
	char imsg[8192];
	cmdfuncs->Args(imsg, sizeof(imsg));
	//FIXME: select an irc network more inteligently
	IRC_Command(ircclient, ircclient?ircclient->defaultdest:"", imsg);
}
int IRC_ConExecuteCommand(qboolean isinsecure)
{
	char buffer[256];
	char imsg[8192];
	ircclient_t *ircclient;
	//FIXME: select the right network

	cmdfuncs->Argv(0, buffer, sizeof(buffer));
	cmdfuncs->Args(imsg, sizeof(imsg));

	//buffer is something like: irc53:#foo
	for (ircclient = ircclients; ircclient; ircclient = ircclient->next)
	{
		if (!strncmp(ircclient->id, buffer, strlen(ircclient->id)))
			break;
	}

	if (!ircclient)
	{
		if (*buffer == '/')
			IRC_Command(NULL, "", imsg);
		else
			Con_TrySubPrint(buffer, "You were disconnected\n");
		return true;
	}

	IRC_Command(ircclient, buffer+strlen(ircclient->id), imsg);
	return true;
}

static void IRC_AddClientMessage(ircclient_t *irc, char *msg)
{
	char output[4096];
	int len;

	Q_strlcpy(output, msg, sizeof(output));
	Q_strlcat(output, "\n", sizeof(output));
	len = strlen(output);

	if (irc->bufferedoutammount + len > sizeof(irc->bufferedoutmessage))
		return;
	memcpy(irc->bufferedoutmessage + irc->bufferedoutammount, output, len);
	irc->bufferedoutammount += len;

	if (irc_debug->value == 1) { IRC_Printf(irc, DEFAULTCONSOLE,COLOURYELLOW "<< %s \n",msg); }
}

static ircclient_t *IRC_FindAccount(const char *server)
{
	ircclient_t *irc;
	for (irc = ircclients; irc; irc = irc->next)
	{
		if (!strcmp(irc->server, server))
			return irc;
	}
	return NULL;	//no match
}

static ircclient_t *IRC_Create(const char *server, const char *nick, const char *realname, const char *hostname, const char *username, const char *password, const char *channels)
{
	ircclient_t *irc;

	//FIXME: accept server:+port for starttls
	//FIXME: accept server:*port for initial tls

	irc = IRC_Malloc(sizeof(ircclient_t));
	if (!irc)
		return NULL;

	memset(irc, 0, sizeof(ircclient_t));
	Q_snprintf(irc->id, sizeof(irc->id), "IRC%x%x:", rand(),rand());
	irc->connecting = true;
	irc->tlsmode = TLS_OFF;
	irc->quitting = false;
	irc->socket = invalid_handle;

	Q_strlcpy(irc->server, server, sizeof(irc->server));

	Q_strlcpy(irc->primarynick,	nick,		sizeof(irc->primarynick));
	Q_strlcpy(irc->nick,		nick,		sizeof(irc->nick));
	Q_strlcpy(irc->realname,	realname,	sizeof(irc->realname));
	Q_strlcpy(irc->hostname,	hostname,	sizeof(irc->hostname));
	Q_strlcpy(irc->username,	username,	sizeof(irc->username));
	Q_strlcpy(irc->pwd,			password,	sizeof(irc->pwd));

	Q_strlcpy(irc->autochannels,channels,	sizeof(irc->autochannels));

//	gethostname(irc->hostname, sizeof(irc->hostname));
//	irc->hostname[sizeof(irc->hostname)-1] = 0;

	irc->next = ircclients;
	ircclients = irc;

	return irc;
}

static void IRC_SetPass(ircclient_t *irc, char *pass)
{
	if (irc->pwd != pass)
		Q_strlcpy(irc->pwd, pass, sizeof(irc->pwd));
	if (*pass && irc->tlsmode != TLS_STARTING)
		IRC_AddClientMessage(irc, va("PASS %s", pass));
}
static void IRC_SetNick(ircclient_t *irc, char *nick)
{
	if (irc->nick != nick)
		Q_strlcpy(irc->nick, nick, sizeof(irc->nick));
	if (irc->tlsmode != TLS_STARTING)
		IRC_AddClientMessage(irc, va("NICK %s", irc->nick));
}
static void IRC_SetUser(ircclient_t *irc, char *user)
{
	if (irc->tlsmode != TLS_STARTING)
	{
		const char *username = irc->username;
		const char *realname = irc->realname;
		if (!*username)
			username = getenv("USER");
		if (!username)
			username = "FTE";	//we need something.

		if (!*realname)
			realname = username;
		//servers will usually ignore the server arg, as they usually know their own dns name already...
		//servers SHOULD ignore the hostname arg too (using a reverse dns). or they'll just replace it with an ip address (note: could use this instead of a STUN server).
		//the username+realname are used, and need to be persistent for auto-op type mechanisms.
		IRC_AddClientMessage(irc, va("USER %s %s %s :%s", username, irc->hostname, irc->server, realname));
	}
}

static qboolean IRC_Establish(ircclient_t *irc)
{
	if (!irc)
		return false;

	if (handleisvalid(irc->socket))	//don't need to do anything.
		return true;

	//clear up any stale state
	irc->bufferedoutammount = 0;
	irc->bufferedinammount = 0;
	irc->quitting = false;

	irc->socket = netfuncs->TCPConnect(irc->server, 6667);	//port is only used if the url doesn't contain one. It's a default.

	//not yet blocking. So no frequent attempts please...
	//non blocking prevents connect from returning worthwhile sensible value.
	if (!handleisvalid(irc->socket))
	{
		Con_Printf("IRC_OpenSocket: couldn't connect\n");
		return false;
	}

	if (irc->tlsmode == TLS_INITIAL)
	{
		if (netfuncs->SetTLSClient(irc->socket, irc->server) < 0)
		{
			netfuncs->Close(irc->socket);
			irc->socket = invalid_handle;
			return false;
		}
	}
	else if (irc->tlsmode != TLS_OFF)
	{
		IRC_AddClientMessage(irc, "STARTTLS");
		irc->tlsmode = TLS_STARTING;
	}
	else
	{
		irc->nicktries = 0;
		IRC_SetPass(irc, irc->pwd);
		IRC_SetNick(irc, irc->nick);
		IRC_SetUser(irc, irc_username->string);
	}

	return true;
}

static void IRC_ParseConfig(void)
{
	qhandle_t config;
	int len = filefuncs->Open("**plugconfig", &config, 1);
	if (len >= 0)
	{
		char *buf = malloc(len+1);
		char *msg = buf;
		buf[len] = 0;
		filefuncs->Read(config, buf, len);
		filefuncs->Close(config);

		while (msg && *msg)
		{
			ircclient_t *irc;
			char server[256];
			char channels[1024];
			char nick[256];
			char password[256];
			char realname[256];
			char hostname[256];
			char username[256];

			msg = COM_Parse(msg, server, sizeof(server));
			msg = COM_Parse(msg, channels, sizeof(channels));
			msg = COM_Parse(msg, nick, sizeof(nick));
			msg = COM_Parse(msg, password, sizeof(password));
			msg = COM_Parse(msg, realname, sizeof(realname));
			msg = COM_Parse(msg, hostname, sizeof(hostname));
			msg = COM_Parse(msg, username, sizeof(username));
			if (*server)
			{
				irc = IRC_Create(server, nick, realname, hostname, username, password, channels);
				if (irc)
				{
					irc->persist = true;
					if (IRC_Establish(irc))
					{
						if (!*irc->autochannels)
							IRC_Printf(irc, DEFAULTCONSOLE, "Trying to connect to %s\n", irc->server);
					}
					else
						IRC_Printf(irc, DEFAULTCONSOLE, "Unable to connect to %s\n", irc->server);
				}
			}
		}

		free(buf);
	}
}
static void IRC_WriteConfig(void)
{
	qhandle_t config;

	if (irc_config->value == 0)
		return;

	filefuncs->Open("**plugconfig", &config, 2);
	if (config >= 0)
	{
		ircclient_t *irc;
		for(irc = ircclients; irc; irc = irc->next)
		{
			char *s = va("\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\n", irc->server, irc->autochannels, irc->primarynick, irc->pwd, irc->realname, irc->hostname, irc->username);
			if (irc->quitting || !irc->persist)
				continue;
			filefuncs->Write(config, s, strlen(s));
		}

		filefuncs->Close(config);
	}
}
static void IRC_MakeDefault(ircclient_t *irc)
{	//unlinks the client, then links it at the head, so that its the first found (thus the default)
	ircclient_t **link;

	for (link = &ircclients; *link; link = &(*link)->next)
	{
		if (*link == irc)
		{
			*link = irc->next;
			break;
		}
	}

	irc->next = ircclients;
	ircclients = irc;
}

static void IRC_PartChannelInternal(ircclient_t *irc, char *channelname)
{
	char ac[countof(irc->autochannels)];
	char *chan;
	strcpy(ac, irc->autochannels);
	chan = strtok(ac, " ");
	*irc->autochannels = 0;
	while(chan)
	{
		if (*chan)
		{
			char *pwd = strchr(chan, ',');
			if (pwd)
				*pwd++ = 0;

			if (strcmp(chan, channelname))
			{
				if (*irc->autochannels)
					Q_strncatz(irc->autochannels, " ", sizeof(irc->autochannels));
				if (pwd)
					Q_strncatz(irc->autochannels, va("%s,%s", chan, pwd), sizeof(irc->autochannels));
				else
					Q_strncatz(irc->autochannels, va("%s", chan), sizeof(irc->autochannels));
			}
		}
		chan = strtok(NULL, " ");
	}
}

static void IRC_PartChannel(ircclient_t *irc, char *channelname)
{
	IRC_PartChannelInternal(irc, channelname);
	IRC_AddClientMessage(irc, va("PART %s", channelname));
}

static void IRC_JoinChannel(ircclient_t *irc, char *channel, char *key) // i screwed up, its actually: <channel>{,<channel>} [<key>{,<key>}]
{
	IRC_PartChannelInternal(irc, channel);

	if (*irc->autochannels)
		Q_strncatz(irc->autochannels, " ", sizeof(irc->autochannels));
	Q_strncatz(irc->autochannels, va("%s,%s", channel, key), sizeof(irc->autochannels));

	if (key)
	{
		/*if (*channel != '#')
			IRC_AddClientMessage(irc, va("JOIN #%s %s", channel,key));
		else*/
			IRC_AddClientMessage(irc, va("JOIN %s %s", channel,key));
	}
	else
	{
		/*if (*channel != '#')
			IRC_AddClientMessage(irc, va("JOIN #%s", channel));
		else*/
		IRC_AddClientMessage(irc, va("JOIN %s", channel));
	}
}

static void IRC_JoinChannels(ircclient_t *irc, char *channelstring)
{
	char *chan = strtok(channelstring, " ");
	while(chan)
	{
		if (*chan)
		{
			char *line = va("JOIN %s", chan);
			char *comma = strchr(line, ',');
			if (comma)
				*comma = ' ';
			IRC_AddClientMessage(irc, line);
		}
		chan = strtok(NULL, " ");
	}
}


/*

ATTN: Spike

# (just for reference) == Ctrl+K in mirc to put the color code symbol in

now to have a background color, you must specify a forground color first (#0,15)

, denotes end of forground color, and start of background color

irc colors work in many strange ways:

#0-#15 for forground color // the code currently converts to this one, which is not the "proper" irc way, read the next one to understand. Still need to support it, just not output as it.

#00-#15 for forground color (note #010 to #015 is not valid) --- this is the "proper" irc way, because I could say "#11+1=2" (which means I want 1+1=2 to appear black (1), but instead it will come out as indigo (11) and look like this: +1=2)

background examples: (note

#0,15 (white forground, light gray background)

#00,15 (white forground, light gray background) // proper way

#15,0 (white forground, light gray background)

#15,00 (white forground, light gray background) // proper way

I hope this makes sense to you, to be able to edit the IRC_FilterMircColours function ~ Moodles

*/
static void IRC_FilterMircColours(char *msg)
{
	int i;
	int chars;
	while(*msg)
	{
		if (*msg == 3)
		{
			chars = 2;
			if (msg[1] >= '0' && msg[1] <= '9')
			{
				i = msg[1]- '0';
				if (msg[2] >= '0' && msg[2] <= '9')
				{
					i = i*10 + (msg[2]-'0');
					chars = 3;
				}
			}
			else
				i = msg[1];
			switch(i)
			{
			case 0:
				msg[1] = '7';	//white
				break;
			case 1:
				msg[1] = '0';	//black
				break;
			case 2:
				msg[1] = '4';	//darkblue
				break;
			case 3:
				msg[1] = '2';	//darkgreen
				break;
			case 4:
				msg[1] = '1';	//red
				break;
			case 5:
				msg[1] = '1';	//brown
				break;
			case 6:
				msg[1] = '5';	//purple
				break;
			case 7:
				msg[1] = '3';	//orange
				break;
			case 8:
				msg[1] = '3';	//yellow
				break;
			case 9:
				msg[1] = '2';	//lightgreen
				break;
			case 10:
				msg[1] = '6';	//darkcyan
				break;
			case 11:
				msg[1] = '6';	//lightcyan
				break;
			case 12:
				msg[1] = '4';	//lightblue
				break;
			case 13:
				msg[1] = '5';	//pink
				break;
			case 14:
				msg[1] = '7';	//grey
				break;
			case 15:
				msg[1] = '7';	//lightgrey
				break;
			default:
				msg++;
				continue;
			}
			*msg = '^';
			msg+=2;
			if (chars==3)
				memmove(msg, msg+1, strlen(msg));
			continue;
		}
		msg++;
	}
}

#define IRC_DONE 0
#define IRC_CONTINUE 1
#define IRC_KILL 2

static void magic_tokenizer(int word,char *thestring)
{
	char *temp;
	int i = 1;

	strcpy(casevar[1],thestring);

	temp = strchr(casevar[1], ' ');

	while (i < 8)
	{
		i++;

		if (temp != NULL)
		{
			strcpy(casevar[i],temp+1);
		}
		else
		{
			strcpy(casevar[i], "");
		}

		temp=strchr(casevar[i], ' ');

	}

}

static void magic_etghack(char *thestring)
{
	char *temp;
	int i = 1;

	strcpy(subvar[1],thestring);

	temp = strchr(subvar[1], ' ');

	while (i < 8)
	{
		i++;

		if (temp != NULL)
		{
			strcpy(subvar[i],temp+1);
		}
		else
		{
			strcpy(subvar[i], "");
		}

		temp=strchr(subvar[i], ' ');

	}

}

static void IRC_TryNewNick(ircclient_t *irc, char *nickname)
{
	char *seedednick;

	if (irc->tlsmode == TLS_STARTING)
	{
		//don't submit any of this info here.
		return;
	}
	if (irc->connecting)
	{
		if (irc->nicktries == 0)
		{
			irc->nicktries++;
			if (*irc->primarynick && strcmp(nickname, irc->primarynick))
			{
				IRC_SetNick(irc, irc->primarynick);
				return;
			}
		}
		if (irc->nicktries == 1)
		{
			irc->nicktries++;
			if (*irc_nick->string && strcmp(nickname, irc_nick->string))
			{
				IRC_SetNick(irc, irc_nick->string);
				return;
			}
		}
		if (irc->nicktries == 2)
		{
			irc->nicktries++;
			if (*irc_altnick->string && strcmp(nickname, irc_altnick->string))
			{
				IRC_SetNick(irc, irc_altnick->string);
				return;
			}
		}

		if (++irc->nicktries == 10)
		{
			IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "ERROR: Unable to obtain usable nickname\n");
			return;
		}

		//panic and pick something at random
		//IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "ERROR: primary nickname in use. Attempting random nickname.\n");
		if (*irc->primarynick && irc->nicktries < 7)
			seedednick = va("%.6s%i", irc->primarynick, rand());
		else if (*irc_nick->string && irc->nicktries < 8)
			seedednick = va("%.6s%i", irc_nick->string, rand());
		else if (*irc_altnick->string && irc->nicktries < 9)
			seedednick = va("%.6s%i", irc_altnick->string, rand());
		else
			seedednick = va("%.6s%i", "FTE", rand());
		seedednick[9] = 0; //'Each client is distinguished from other clients by a unique nickname having a maximum length of nine (9) characters'

		IRC_SetNick(irc, seedednick);
	}
}

//==================================================

static void numbered_command(int comm, char *msg, ircclient_t *irc) // move vars up 1 more than debug says
{
	magic_tokenizer(0,msg);

	switch (comm)
	{
	case   1:	/* RPL_WELCOME */
	case   2:	/* RPL_YOURHOST */
	case   3:	/* RPL_CREATED */
	case   4:	/* RPL_MYINFO */
	case   5:	/* RPL_ISUPPORT */
	{
		if (irc->tlsmode != TLS_STARTING)
			irc->connecting = 0; // ok we are connected

		if (irc_motd->value)
			IRC_Printf(irc, DEFAULTCONSOLE, COLOURYELLOW "SERVER STATS: %s\n",casevar[3]);
		return;
	}
//	case   5:	/* RPL_BOUNCE */
//	{
//		IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "RPL_BOUNCE: %s\n",casevar[3]);
//		return;
//	}
	case 20:	/* RPL_HELLO */
	{
		if (irc_motd->value)
			IRC_Printf(irc, DEFAULTCONSOLE, COLOURYELLOW "%s\n",casevar[3]);
		return;
	}
	case 42:	/*RPL_YOURID */
	{
		if (irc_motd->value)
			IRC_Printf(irc, DEFAULTCONSOLE, COLOURYELLOW "%s\n",casevar[3]);
		return;
	}
	case 250:
	case 251:	/* RPL_LUSERCLIENT */
	case 252:	/* RPL_LUSEROP */
	case 253:	/* RPL_LUSERUNKNOWN */
	case 254:	/* RPL_LUSERCHANNELS */
	case 255:	/* RPL_LUSERME */
	case 256:	/* RPL_ADMINME */
	case 257:	/* RPL_ADMINLOC1 */
	case 258:	/* RPL_ADMINLOC2 */
	case 259:	/* RPL_ADMINEMAIL */
	case 265:
	case 266:
	{
		if (irc_motd->value)
			IRC_Printf(irc, DEFAULTCONSOLE, COLOURYELLOW "SERVER STATS: %s\n",casevar[3]);
		return;
	}
	case 301: /* #define RPL_AWAY             301 */
	{
		char *username = strtok(casevar[3], " ");
		char *awaymessage = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE,"WHOIS: <%s> (Away Message: %s)\n",username,awaymessage);
		return;
	}
	case 305: /* RPL_UNAWAY */
	case 306: /* RPL_NOWAWAY */
	{
		char *away = casevar[3]+1;

		IRC_Printf(irc, CURRENTCONSOLE,"%s\n",away);
		return;
	}
	case 311: /* #define RPL_WHOISUSER        311 */
	{
		char *username = strtok(casevar[3], " ");
		char *ident = strtok(casevar[4], " ");
		char *address = strtok(casevar[5], " ");
		char *realname = casevar[7]+1;

		IRC_Printf(irc, DEFAULTCONSOLE,"WHOIS: <%s> (Ident: %s) (Address: %s) (Realname: %s) \n", username, ident, address, realname);
		return;
	}
	case 312: /* #define RPL_WHOISSERVER      312 */ //seems to be /whowas also
	{
		char *username = strtok(casevar[3], " ");
		char *serverhostname = strtok(casevar[4], " ");
		char *servername = casevar[5]+1;

		IRC_Printf(irc, DEFAULTCONSOLE,"WHOIS: <%s> (Server: %s) (Server Name: %s) \n", username, serverhostname, servername);
		return;
	}
	case 313: /* RPL_WHOISOPERATOR */
	{
		char *username = strtok(casevar[3], " ");
		char *isoperator = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE,"WHOIS: <%s> (%s)\n", username,isoperator);

		return;
	}
	case 317: /* #define RPL_WHOISIDLE        317 */
	{
		char *username = strtok(casevar[3], " ");
		char *secondsidle = strtok(casevar[4], " ");
		char *signontime = strtok(casevar[5], " ");
		time_t t;
		const struct tm *tm;
		char buffer[100];

		t=strtoul(signontime, 0, 0);
		tm=localtime(&t);

		strftime (buffer, 100, "%a %b %d %H:%M:%S", tm);

		IRC_Printf(irc, DEFAULTCONSOLE,"WHOIS: <%s> (Idle Time: %s seconds) (Signon Time: %s) \n", username, secondsidle, buffer);
		return;
	}
	case 318: /* #define RPL_ENDOFWHOIS       318 */
	{
		char *endofwhois = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE,"WHOIS: %s\n", endofwhois);

		return;
	}
	case 319: /* #define RPL_WHOISCHANNELS    319 */
	{
		char *username = strtok(casevar[3], " ");
		char *channels = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE,"WHOIS: <%s> (Channels: %s)\n",username,channels); // need to remove the space from the end of channels
		return;
	}
	case 321:
	{
//		IRC_Printf(irc, "list", "Start /LIST\n");

		return;
	}
	case 322: /* #define RPL_LIST             322 */
	{
		char *channel = strtok(casevar[3], " ");
		char *users = strtok(casevar[4], " ");
		char *topic = casevar[5]+1;

		IRC_Printf(irc, "list", "^1Channel:^7 %s ^1Users:^7 %s ^1Topic:^7 %s\n\n", channel,users,topic);
		return;
	}
	case 323:	/* RPL_LISTEND*/
	{
		//char *endoflist = casevar[3]+1;

//		IRC_Printf(irc, "list", "%s\n",endoflist);

		return;
	}
	case 333:	/* RPL_TOPICWHOTIME channel user timestamp*/
		return;
	case 366:	/* RPL_ENDOFNAMES */
	{
		char *channel = strtok(casevar[3], " ");
		char *endofnameslist = casevar[4]+1;

		IRC_Printf(irc, channel,"%s\n",endofnameslist);
		return;
	}
	case 372:	/* RPL_MOTD */
	case 375:	/* RPL_MOTDSTART */
	case 376:	/* RPL_ENDOFMOTD */
	{
		char *motdmessage = casevar[3]+1;

		if (irc_motd->value == 2)
			IRC_Printf(irc, DEFAULTCONSOLE, "MOTD: %s\n", motdmessage);
		else if (irc_motd->value)
			IRC_Printf(irc, DEFAULTCONSOLE, "%s\n", motdmessage);

		if (*irc->autochannels)
			IRC_JoinChannels(irc, irc->autochannels);

		return;
	}
	case 378:
	{
		IRC_Printf(irc, DEFAULTCONSOLE, "%s\n", msg);
		return;
	}
	case 401:	/* ERR_NOSUCHNICK */
	case 403:	/* ERR_NOSUCHCHANNEL */
	case 404:	/* ERR_CANNOTSENDTOCHAN */
	case 405:	/* ERR_TOOMANYCHANNELS */
	case 442:	/* ERR_NOTONCHANNEL */
	{
		char *username = strtok(casevar[3], " ");
		char *error = casevar[4]+1;

		IRC_Printf(irc, username, COLOURRED "ERROR <%s>: %s\n",username,error);
		return;
	}
	case 432: /* #define ERR_ERRONEUSNICKNAME 432 */
	{
		IRC_Printf(irc, DEFAULTCONSOLE, "Erroneous/invalid nickname given\n");
		IRC_TryNewNick(irc, "FTEUser");
		return;
	}
	case 433: /* #define ERR_NICKNAMEINUSE    433 */
	case 438:
	case 453:
	{
		char *nickname = strtok(casevar[4], " ");
		char *badnickname = ":Nickname";

		if ( !strcasecmp(nickname,badnickname) ) // bug with ircd, the nickname actually shifts position.
		{
			nickname = strtok(casevar[3], " ");
		}

//		IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "ERROR: <%s> is already in use.\n",nickname);

		IRC_TryNewNick(irc, nickname);
		return;
	}
	case 471: /* ERR_CHANNELISFULL */
	{
		char *channel = strtok(casevar[3], " ");
		char *error = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "ERROR: <%s>: %s (Channel is full and has reached user limit)\n",channel,error);
		return;
	}
	case 472: /* ERR_UNKNOWNMODE */
	{
		char *mode = strtok(casevar[3], " ");
		char *error = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "ERROR: <%s>: %s (Unknown mode)\n",mode,error);
		return;
	}
	case 473: /* ERR_INVITEONLYCHAN */
	{
		char *channel = strtok(casevar[3], " ");
		char *error = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "ERROR: <%s>: %s (Invite only)\n",channel,error);
		return;
	}
	case 474: /* ERR_BANNEDFROMCHAN */
	{
		char *channel = strtok(casevar[3], " ");
		char *error = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "ERROR: <%s>: %s (You are banned)\n",channel,error);
		return;
	}
	case 475: /* ERR_BADCHANNELKEY */
	{
		char *channel = strtok(casevar[3], " ");
		char *error = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "ERROR: <%s>: %s (Need the correct channel key. Example: /join %s bananas)\n",channel,error,channel);
		return;
	}
	case 482: /* ERR_CHANOPRIVSNEEDED */
	{
		char *channel = strtok(casevar[3], " ");
		char *error = casevar[4]+1;

		IRC_Printf(irc, DEFAULTCONSOLE, COLOURRED "ERROR: <%s>: %s (Need +o or @ status)\n",channel,error);
		return;
	}
	case 670: /* RPL_STARTTLS */
	{
		netfuncs->SetTLSClient(irc->socket, irc->server);
		irc->tlsmode = TLS_START;
		irc->nicktries = 0;
		IRC_SetPass(irc, irc->pwd);
		IRC_SetNick(irc, irc->nick);
		IRC_SetUser(irc, irc_username->string);
		return;
	}
	case 691: /* ERR_STARTTLS */
	{
		IRC_Printf(irc, DEFAULTCONSOLE, COLOURYELLOW "STARTTLS Failed: %s\n", casevar[3]);
		netfuncs->Close(irc->socket);
		irc->socket = invalid_handle;
		return;
	}
	}

	IRC_Printf(irc, DEFAULTCONSOLE, "%s\n", msg); // if no raw number exists, print the thing
}

static struct ircice_s *IRC_ICE_Find(ircclient_t *irc, const char *sender, enum iceproto_e type)
{
	struct ircice_s *ice;
	for (ice = irc->ice; ice; ice = ice->next)
	{
		if (ice->type == type && !strcmp(ice->peer, sender))
			return ice;
	}
	return NULL;
}
static struct ircice_s *IRC_ICE_Create(ircclient_t *irc, const char *sender, enum iceproto_e type, qboolean creator)
{
	struct icestate_s *ice;
	struct ircice_s *ircice;
	char *s, token[MAX_OSPATH];
	if (!piceapi)
		return NULL;

	if (!creator && type == ICEP_QWSERVER)
		ice = piceapi->Create(NULL, NULL, sender, ICEM_ICE, ICEP_QWCLIENT, creator);
	else if (!creator && type == ICEP_QWCLIENT)
		ice = piceapi->Create(NULL, NULL, sender, ICEM_ICE, ICEP_QWSERVER, creator);
	else
		ice = piceapi->Create(NULL, NULL, sender, ICEM_ICE, type, creator);

	if (!ice)
		return NULL;

	piceapi->Set(ice, "controller", creator?"1":"0");
	if (creator && type == ICEP_VOICE)
	{
		//note: the engine will ignore codecs it does not support.
		piceapi->Set(ice, "codec96", "opus@48000");
		piceapi->Set(ice, "codec97", "speex@16000");	//wide
		piceapi->Set(ice, "codec98", "speex@8000");		//narrow
		piceapi->Set(ice, "codec99", "speex@32000");	//ultrawide
		piceapi->Set(ice, "codec8", "pcma@8000");
		piceapi->Set(ice, "codec0", "pcmu@8000");
	}

	//query dns to see if there's a stunserver hosted by the same domain
	//nslookup -querytype=SRV _stun._udp.example.com
//	Q_snprintf(stunhost, sizeof(stunhost), "_stun._udp.%s", ice->server);
//	if (NET_DNSLookup_SRV(stunhost, stunhost, sizeof(stunhost)))
//		piceapi->Set(ice, "server", va("stun:%s" + stunhost));
//	else
	{
		char *stun = cvarfuncs->GetNVFDG("net_ice_broker", "", 0, NULL, NULL)->string;
		s = strstr(stun, "://");
		if (s) stun = s+3;
		piceapi->Set(ice, "server", va("stun:%s", stun));
	}

	//sadly we need to add the other ice servers ourselves despite there being a cvar to list them.
	s = cvarfuncs->GetNVFDG("net_ice_servers", "", 0, NULL, NULL)->string;
	while((s=cmdfuncs->ParseToken(s, token, sizeof(token), NULL)))
		piceapi->Set(ice, "server", token);

	ircice = malloc(sizeof(*ircice));
	memset(ircice, 0, sizeof(*ircice));
	ircice->next = irc->ice;
	irc->ice = ircice;

	ircice->type = type;
	Q_strlcpy(ircice->peer, sender, sizeof(ircice->peer));
	ircice->host = creator;
	ircice->accepted = false;
	ircice->allowed = creator;
	ircice->ice = ice;

	return ircice;
}
static void IRC_ICE_Update(ircclient_t *irc, struct ircice_s *ice, char updatetype)
{
	//'+'	propose  ('hey, can I call you please?')
	//'='	offer    ('these are my details')
	//'*'	finalise ('this is what I'm going to use')
	//'-'	reject   ('get lost, I don't want to talk to you any more')
	//'%'	candiate ('try this address')
	//I was originally using colons to separate terms, but switched to slashes to avoid smilies for irc clients that print unknown CTCP messages.
	char message[1024];
	struct icecandinfo_s *c;
	char *icetype;

	if (!ice->allowed && updatetype != '-')
		return;

	*message = 0;

	switch(ice->type)
	{
	default:
	case ICEP_VOICE:
		icetype = "voice";
		break;
	case ICEP_QWSERVER: 
		icetype = "qwserver";
		break;
	case ICEP_QWCLIENT:
		icetype = "qwclient";
		break;
	}

	if (updatetype == '=' || updatetype == '*')
	{
		char ufrag[256];
		char pwd[256];

		piceapi->Get(ice->ice, "lufrag",  ufrag, sizeof(ufrag));
		piceapi->Get(ice->ice, "lpwd",	 pwd, sizeof(pwd));

		Q_snprintf(message, sizeof(message), " ufrag/%s pwd/%s", ufrag, pwd);
	}

	if (updatetype == '+' || updatetype == '=')
	{
		unsigned int i;
		for (i = 0; i <= 127; i++)
		{
			char codec[256];
			char codecname[64];
			char argn[64];
			Q_snprintf(argn, sizeof(argn), "codec%i", i);
			if (!piceapi->Get(ice->ice, argn,  codecname, sizeof(codecname)))
				continue;

			if (!strcmp(codecname, "speex@8000"))		//speex narrowband
				Q_snprintf(codec, sizeof(codec), "codec/%i/speex/8000", i);
			else if (!strcmp(codecname, "speex@16000"))	//speex wideband
				Q_snprintf(codec, sizeof(codec), "codec/%i/speex/16000", i);
			else if (!strcmp(codecname, "speex@32000"))	//speex ultrawideband
				Q_snprintf(codec, sizeof(codec), "codec/%i/speex/32000", i);
			else if (!strcmp(codecname, "pcma@8000"))	//speex wideband
				Q_snprintf(codec, sizeof(codec), "codec/%i/pcma/8000", i);
			else if (!strcmp(codecname, "pcmu@8000"))	//speex ultrawideband
				Q_snprintf(codec, sizeof(codec), "codec/%i/pcmu/8000", i);
			else if (!strcmp(codecname, "opus@48000"))		//opus codec.
				Q_snprintf(codec, sizeof(codec), "codec/%i/opus/48000", i);
			else
				continue;

			if (strlen(message) + strlen(codec) + 2 > 256)
			{
				IRC_AddClientMessage(irc, va("NOTICE %s :\001FTEICE %c%s%s\001", ice->peer, updatetype, icetype, message));
				updatetype = '%';
				*message = 0;
			}

			Q_strlcat(message, " ", sizeof(message));
			Q_strlcat(message, codec, sizeof(message));
		}
	}

/*	if (*message)
	{
		IRC_AddClientMessage(irc, va("NOTICE %s :\001FTEICE %c%s%s\001", ice->peer, updatetype, icetype, message));
		*message = 0;
	}
*/
	if (updatetype != '+' && updatetype != '-')
	{
		while ((c = piceapi->GetLCandidateInfo(ice->ice)))
		{
			char type[] = "hspr";
			char cand[256];
			Q_snprintf(cand, sizeof(cand), "cand/"
				"%c%c/%i/%i/"
				"%i/%i/%i/"
				"%i/%s/%s", 
				type[c->type], 'u', c->priority, c->port, 
				c->network, c->generation, c->foundation,
				c->component, c->candidateid, c->addr);

			if (strlen(message) + strlen(cand) + 2 > 256)
			{
				IRC_AddClientMessage(irc, va("NOTICE %s :\001FTEICE %c%s%s\001", ice->peer, updatetype, icetype, message));
				updatetype = '%';
				*message = 0;
			}

			Q_strlcat(message, " ", sizeof(message));
			Q_strlcat(message, cand, sizeof(message));
		}
	}
	if (*message || updatetype != '%')
		IRC_AddClientMessage(irc, va("NOTICE %s :\001FTEICE %c%s%s\001", ice->peer, updatetype, icetype, message));
}

static void IRC_ICE_ParseCandidate(struct icestate_s *ice, char *cand)
{
	char *addr;
	struct icecandinfo_s info;
	if (strlen(cand) < 12)
		return;
	switch(cand[5])
	{
	case 'h':	info.type = ICE_HOST;	break;
	case 's':	info.type = ICE_SRFLX;	break;
	case 'p':	info.type = ICE_PRFLX;	break;
	default:
	case 'r':	info.type = ICE_RELAY;	break;
	}
	info.transport	= (cand[6] == 't')?1:0;
	info.priority	= strtol(cand+8, &cand, 0); if (*cand != '/')return;
	info.port		= strtol(cand+1, &cand, 0); if (*cand != '/')return;
	info.network	= strtol(cand+1, &cand, 0); if (*cand != '/')return;
	info.generation	= strtol(cand+1, &cand, 0); if (*cand != '/')return;
	info.foundation	= strtol(cand+1, &cand, 0); if (*cand != '/')return;
	info.component	= strtol(cand+1, &cand, 0); if (*cand != '/')return;
	addr = strchr(cand+1, '/');
	if (!addr)
		return;
	*addr++ = 0;
	Q_strlcpy(info.candidateid, cand+1, sizeof(info.candidateid));
	Q_strlcpy(info.addr, addr, sizeof(info.candidateid));
	
	piceapi->AddRCandidateInfo(ice, &info);
}

static void IRC_ICE_ParseCodec(struct icestate_s *ice, char *codec)
{
	char *start;
	unsigned int num;
	char name[64];
	unsigned int rate;
	num		= strtoul(codec+6, &codec, 0); if (*codec != '/')return;
	start = codec+1; codec = strchr(codec, '/'); if (!codec)return;*codec = 0; Q_strlcpy(name, start, sizeof(name));
	rate = strtoul(codec+1, &codec, 0); 

	Q_strlcat(name, va("@%u", rate), sizeof(name));

	piceapi->Set(ice, va("codec%i", num), name);
}

static void IRC_ICE_Parse(ircclient_t *irc, const char *sender, char *message)
{
	struct ircice_s *ice;
	char token[256];
	enum iceproto_e type = ICEP_INVALID;
	message = COM_Parse(message, token, sizeof(token));
	if (*token == '+' || *token == '=' || *token == '*' || *token == '%')
	{	//+ is offer or accept for a new content type
		//= is an ack from the receiver
		//* is the final handshake that includes offerer's full details
		//% is extra updates.
		char icetype = *token;
		if (!strcmp(token+1, "voice"))
			type = ICEP_VOICE;
		else if (!strcmp(token+1, "qwserver"))
			type = ICEP_QWSERVER;
		else if (!strcmp(token+1, "qwclient"))
			type = ICEP_QWCLIENT;
		else
		{
			IRC_Printf(irc, sender, "ICE session type %s is not recognised\n", token);
			return;
		}

		ice = IRC_ICE_Find(irc, sender, type);
		if (!ice && (icetype == '+' || icetype == '='))
			ice = IRC_ICE_Create(irc, sender, type, false);//icetype=='=');

		if (ice)
		{
			while(message)
			{
				message = COM_Parse(message, token, sizeof(token));
				if (!strncmp(token, "cand/", 5))
					IRC_ICE_ParseCandidate(ice->ice, token);
				else if (!strncmp(token, "codec/", 6))
					IRC_ICE_ParseCodec(ice->ice, token);
				else if (!strncmp(token, "ufrag/", 6))
					piceapi->Set(ice->ice, "rufrag", token+6);
				else if (!strncmp(token, "pwd/", 4))
					piceapi->Set(ice->ice, "rpwd", token+4);
				else if (*token)
					IRC_Printf(irc, sender, "unknown ice token %s\n", token);
			}

			if ((icetype == '=' || icetype == '*') && !ice->accepted && ice->allowed)
			{
				piceapi->Set(ice->ice, "state", STRINGIFY(ICE_CONNECTING));
				ice->accepted = true;
			}

			switch(icetype)
			{
			case '+':
				//needs user 
				if (!ice->allowed)
				{
					switch(type)
					{
					case ICEP_VOICE:	IRC_Printf(irc, sender, "%s is trying to call you. ^[[Click to Converse]\\act\\iceaccept_v\\who\\%s^] ^[[Click to Decline]\\act\\icedecline_v\\who\\%s^] \n", sender, sender, sender); break;
					case ICEP_QWSERVER: IRC_Printf(irc, sender, "%s wants you to join their game. ^[[Click to Join]\\act\\iceaccept_s\\who\\%s^] ^[[Click to Decline]\\act\\icedecline_s\\who\\%s^] \n", sender, sender, sender); break;
					case ICEP_QWCLIENT: IRC_Printf(irc, sender, "%s is trying to gatecrash your game. ^[[Click to Allow]\\act\\iceaccept_c\\who\\%s^] ^[[Click to Decline]\\act\\icedecline_c\\who\\%s^] \n", sender, sender, sender); break;
					case ICEP_INVALID:	break;
					case ICEP_VIDEO:	break;
					}
				}
				else
				{
					switch(type)
					{
					case ICEP_VOICE:	IRC_Printf(irc, sender, "Accepting voice call\n"); break;
					case ICEP_QWSERVER: IRC_Printf(irc, sender, "Accepting game invite\n"); break;
					case ICEP_QWCLIENT: IRC_Printf(irc, sender, "Accepting gatecrash\n"); break;
					case ICEP_INVALID:	break;
					case ICEP_VIDEO:	break;
					}
					IRC_ICE_Update(irc, ice, '=');
				}
				break;
			case '=':
				switch(type)
				{
				case ICEP_VOICE:	IRC_Printf(irc, sender, "Establishing voice call\n"); break;
				case ICEP_QWSERVER: IRC_Printf(irc, sender, "Establishing game invite\n"); break;
				case ICEP_QWCLIENT: IRC_Printf(irc, sender, "Establishing gatecrash\n"); break;
				case ICEP_INVALID:	break;
				case ICEP_VIDEO:	break;
				}
				IRC_ICE_Update(irc, ice, '*');
				break;
			default:
				switch(type)
				{
				case ICEP_VOICE:	IRC_Printf(irc, sender, "Updating voice call\n"); break;
				case ICEP_QWSERVER: IRC_Printf(irc, sender, "Updating game invite\n"); break;
				case ICEP_QWCLIENT: IRC_Printf(irc, sender, "Updating gatecrash\n"); break;
				case ICEP_INVALID:	break;
				case ICEP_VIDEO:	break;
				}
				IRC_ICE_Update(irc, ice, '%');
				break;
			}
		}
	}
	else if (*token == '-')
	{
		IRC_Printf(irc, sender, "dropping connections is not supported yet: %s\n", token);
	}
	else
		IRC_Printf(irc, sender, "ICE command type not supported: %s\n", token);
}

static void IRC_ICE_Frame(ircclient_t *irc)
{
	char bah[8];
	struct ircice_s *ice;
	for (ice = irc->ice; ice; ice = ice->next)
	{
		if (!ice->accepted || !ice->allowed)
			continue;
		//ice needs some maintainence. if things change then we need to be prepared to send updated candidate info
		piceapi->Get(ice->ice, "newlc", bah, sizeof(bah));
		if (atoi(bah))
		{
			IRC_Printf(irc, ice->peer, "Sending updated peer info\n");
			IRC_ICE_Update(irc, ice, '%');
		}

		//FIXME: detect when the ice connection goes idle.
	}
}

static void IRC_ICE_Authorise(ircclient_t *irc, const char *with, enum iceproto_e type, qboolean authorize, char *announce)
{
	struct ircice_s *ice, **link;
	for (link = &irc->ice; *link; link = &(*link)->next)
	{
		ice = *link;
		if (ice->type == type)
			if (!strcmp(ice->peer, with))
			{
				if (authorize)
				{
					if (ice->allowed)
					{
						IRC_Printf(irc, announce, "Connection is already authorised\n");
						return;	//nothing to do
					}

					//yay! its good to go!
					ice->allowed = true;
					switch(type)
					{
					case ICEP_VOICE:	IRC_Printf(irc, announce, "Accepting voice call\n"); break;
					case ICEP_QWSERVER: IRC_Printf(irc, announce, "Accepting game invite\n"); break;
					case ICEP_QWCLIENT: IRC_Printf(irc, announce, "Accepting gatecrash\n"); break;
					case ICEP_VIDEO:	break;
					case ICEP_INVALID:	break;
					}
					IRC_ICE_Update(irc, ice, '=');
				}
				else
				{
					IRC_ICE_Update(irc, ice, '-');
					*link = ice->next;
					if (ice->ice)
						piceapi->Close(ice->ice, true);
					IRC_Free(ice);

					IRC_Printf(irc, announce, "Connection terminated\n");
				}
				return;
			}
	}

	IRC_Printf(irc, announce, "Connection is already terminated\n");
}

qboolean IRC_ConsoleLink(void)
{
	ircclient_t *irc;
	char link[256];
	char *who = NULL;
	char *channel = NULL;
	char what[256];
	char whobuf[256];
	char which[512];
	enum iceproto_e type;
//	cmdfuncs->Argv(0, text, sizeof(text));
	cmdfuncs->Argv(1, link, sizeof(link));
	cmdfuncs->Argv(2, which, sizeof(which));

	Plug_Info_ValueForKey(link, "act", what, sizeof(what));
	who = Plug_Info_ValueForKey(link, "who", whobuf, sizeof(whobuf));

	for (irc = ircclients; irc; irc = irc->next)
	{
		if (!strncmp(irc->id, which, strlen(irc->id)))
		{
			channel = which + strlen(irc->id);
			if (!*who)
				who = channel;
			break;
		}
	}
	if (!irc || !who || !*what)
		return false;

	if (!strcmp(what, "reconnect"))
	{
		if (handleisvalid(irc->socket))
			IRC_Printf(irc, channel, "Already %s.\n", irc->connecting?"reconnecting":"connected");
		else if (IRC_Establish(irc))
			IRC_Printf(irc, channel, "Reconnecting...\n");
		else
			IRC_Printf(irc, channel, "Unable to connect\n");
		return true;
	}
	if (!*who)
		return false;	//that seems wrong. probably nothing to do with irc.

	if (irc->tlsmode == TLS_STARTING || irc->connecting)
	{
		IRC_SetFooter(irc, channel, "Still connecting. Please wait.\n");
		return true;
	}

	if (!strcmp(what, "iceaccept_v") || !strcmp(what, "iceaccept_s") || !strcmp(what, "iceaccept_c"))
	{
		switch(what[10])
		{default:
		case 'v': type = ICEP_VOICE; break;
		case 's': type = ICEP_QWSERVER; break;
		case 'c': type = ICEP_QWCLIENT; break;
		}
		IRC_Printf(irc, channel, "Accepting foo from %s\n", who);
		IRC_ICE_Authorise(irc, who, type, true, channel);
		return true;
	}
	else if (!strcmp(what, "icedecline_v") || !strcmp(what, "icedecline_s") || !strcmp(what, "icedecline_c"))
	{
		switch(what[11])
		{default:
		case 'v': type = ICEP_VOICE; break;
		case 's': type = ICEP_QWSERVER; break;
		case 'c': type = ICEP_QWCLIENT; break;
		}
		IRC_ICE_Authorise(irc, who, type, false, channel);
		return true;
	}
	else if (!strcmp(what, "user"))
	{
		char links[2048];
		char link[512];
		Q_snprintf(links, sizeof(links), "%s:", who);
		if (1)
		{
			Q_snprintf(link, sizeof(link), " ^[[Message]\\act\\msg\\who\\%s^]", who);
			Q_strlcat(links, link, sizeof(links));
		}
		if (IRC_ICE_Find(irc, who, ICEP_VOICE))
		{
			Q_snprintf(link, sizeof(link), " ^[[Hang up]\\act\\icedecline_v\\who\\%s^]", who);
			Q_strlcat(links, link, sizeof(links));
		}
		else
		{
			Q_snprintf(link, sizeof(link), " ^[[Call]\\act\\icestart_v\\who\\%s^]", who);
			Q_strlcat(links, link, sizeof(links));
		}
		if (IRC_ICE_Find(irc, who, ICEP_QWSERVER))
		{
			Q_snprintf(link, sizeof(link), " ^[[Disconnect]\\act\\icedecline_s\\who\\%s^]", who);
			Q_strlcat(links, link, sizeof(links));
		}
		else
		{
			Q_snprintf(link, sizeof(link), " ^[[Invite]\\act\\icestart_s\\who\\%s^]", who);
			Q_strlcat(links, link, sizeof(links));
		}
		if (IRC_ICE_Find(irc, who, ICEP_QWCLIENT))
		{
			Q_snprintf(link, sizeof(link), " ^[[Disconnect]\\act\\icedecline_c\\who\\%s^]", who);
			Q_strlcat(links, link, sizeof(links));
		}
		else
		{
			Q_snprintf(link, sizeof(link), " ^[[Gatecrash]\\act\\icestart_c\\who\\%s^]", who);
			Q_strlcat(links, link, sizeof(links));
		}
		IRC_SetFooter(irc, channel, links);
		return true;
	}
	else if (!strcmp(what, "msg"))
	{
		IRC_Printf(irc, who, "");
		return true;
	}
	else if (!strcmp(what, "icestart_v") || !strcmp(what, "icestart_s") || !strcmp(what, "icestart_c"))
	{
		struct ircice_s *ice;
		char *text;
		char link[512];
		switch(what[9])
		{default:
		case 'v': type = ICEP_VOICE;	text = "Calling";		break;
		case 's': type = ICEP_QWSERVER; text = "Inviting";		break;
		case 'c': type = ICEP_QWCLIENT; text = "Gatecrashing";	break;
		}
		ice = IRC_ICE_Create(irc, who, type, true);
		if (ice)
		{
			Q_snprintf(link, sizeof(link), "^["COLOURGREEN"%s\\act\\user^]", ice->peer);
			IRC_ICE_Update(irc, ice, '+');
			IRC_Printf(irc, ice->peer, "<%s %s ^[[Abort]\\act\\icedecline_%c\\who\\%s^]>\n", text, link, what[9], ice->peer);
		}

		return true;
	}
	return false;
}

//==================================================

static int IRC_ClientFrame(ircclient_t *irc)
{
	char prefix[256];
	int ret;
	char *nextmsg, *msg;
	char *temp;
	char token[1024];
	char var[9][1000];

	int i = 1;

	ret = netfuncs->Recv(irc->socket, irc->bufferedinmessage+irc->bufferedinammount, sizeof(irc->bufferedinmessage)-1 - irc->bufferedinammount);
	if (ret == 0)
	{
		if (!irc->bufferedinammount)	//if we are half way through a message, read any possible conjunctions.
			return IRC_DONE;	//remove
	}
	if (ret < 0)
		return IRC_KILL;

	if (ret>0)
		irc->bufferedinammount+=ret;
	irc->bufferedinmessage[irc->bufferedinammount] = '\0';
	nextmsg = strstr(irc->bufferedinmessage, "\r\n");
	if (!nextmsg)
		return IRC_DONE;

	*nextmsg = '\0';
	nextmsg+=2;

	msg = irc->bufferedinmessage;

	strcpy(var[1],msg);

	temp = strchr(var[1], ' ');

	while (i < 8)
	{
		i++;

		if (temp != NULL)
		{
			strcpy(var[i],temp+1);
		}
		else
		{
			strcpy(var[i], "");
		}

		temp=strchr(var[i], ' ');

	}

//	if (irc_debug->value == 1) { IRC_Printf(irc, DEFAULTCONSOLE,COLOURRED "!!!!! ^11: %s ^22: %s ^33: %s ^44: %s ^55: %s ^66: %s ^77: %s ^88: %s\n",var[1],var[2],var[3],var[4],var[5],var[6],var[7],var[8]); }
	if (irc_debug->value == 1) { IRC_Printf(irc, DEFAULTCONSOLE,COLOURRED "%s\n",var[1]); }

	if (*msg == ':')	//we need to strip off the prefix
	{
		char *sp = strchr(msg, ' ');
		if (!sp)
		{
			IRC_Printf(irc, DEFAULTCONSOLE, "Ignoring bad message\n%s\n", msg);
			memmove(irc->bufferedinmessage, nextmsg, irc->bufferedinammount - (msg-irc->bufferedinmessage));
			irc->bufferedinammount-=nextmsg-irc->bufferedinmessage;
			return IRC_CONTINUE;
		}

		if (sp-msg >= sizeof(prefix))
			Q_strlcpy(prefix, msg+1, sizeof(prefix));
		else
			Q_strlcpy(prefix, msg+1, sp-msg);

		msg = sp;
		while(*msg == ' ')
			msg++;
	}
	else
		strcpy(prefix, irc->server);

	if (!strncmp(var[1], "NOTICE AUTH ", 12))
	{
		IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN "SERVER NOTICE: %s\n", var[3]+1);
	}
	else if (!strncmp(var[1], "PING ", 5))
	{
		IRC_AddClientMessage(irc, va("PONG %s", var[2]));
	}
	else if (!strncmp(var[2], "NOTICE ", 6))
	{
		char *exc = strchr(prefix, '!');
		char *col = strchr(msg+6, ':');
		char *end;
		char *to = msg + 7;
		char *etghack;

		if (!strncmp(var[4]+1, "\1", 1))
		{
			char delimiters[] = "!";
			char *username = strtok(var[1]+1, delimiters);
			char *ctcpreplytype = strtok(var[4]+2, " ");
			char *ctcpreply = var[5];

			if (!strcmp(ctcpreplytype, "FTEICE"))
			{
				IRC_Printf(irc, username, "ICE from %s\n", username);	//from client
				IRC_ICE_Parse(irc, username, ctcpreply);
			}
			else
				IRC_Printf(irc, DEFAULTCONSOLE,"<CTCP Reply> %s FROM %s: %s\n",ctcpreplytype,username,ctcpreply); // need to remove the last char on the end of ctcpreply
		}
		else if (exc && col)
		{
			*col = '\0';
			col++;

			while(*to <= ' ' && *to)
				to++;
			for (end = to + strlen(to)-1; end >= to && *end <= ' '; end--)
				*end = '\0';
			if (!strcmp(to, irc_nick->string))
				to = prefix;	//This was directed straight at us.
								//So change the 'to', to the 'from'.

			for (end = to; *end; end++)
			{
				if (*end >= 'A' && *end <= 'Z')
					*end = *end + 'a' - 'A';
			}

			*exc = '\0';
			if (!strncmp(col, "\001", 1))
			{
				end = strchr(col+1, '\001');
				if (end)
					*end = '\0';
				if (!strncmp(col+1, "ACTION ", 7))
				{
					IRC_FilterMircColours(col+8);
					IRC_Printf(irc, to, COLOURGREEN "***%s "COLORWHITE"%s\n", prefix, col+8);	//from client
				}
			}
			else
			{
				IRC_FilterMircColours(col);
				IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN "NOTICE: -%s- %s\n", prefix, col);	//from client
			}
		}
		else
		{
			etghack = strtok(var[1],"\n");

			if (!irc->connecting || IRC_WindowShown(irc, DEFAULTCONSOLE))
				IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN "SERVER NOTICE: <%s> %s\n", prefix, etghack);

//			strcpy(servername,prefix);

			while (1)
			{
				etghack = strtok(NULL, "\n");

				if (etghack == NULL)
					break;

				magic_etghack(etghack);

				if (atoi(subvar[2]) != 0)
					numbered_command(atoi(subvar[2]), etghack, irc);
				else
					IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN "SERVER NOTICE: <%s> %s\n", prefix, subvar[4]);

			}
		}

	}
	else if (!strncmp(var[2], "PRIVMSG ", 7))	//no autoresponses to notice please, and any autoresponses should be in the form of a notice
	{
		char *exc = strchr(prefix, '!');
		char *col = strchr(msg+6, ':');
		char *end;
		char *to = msg + 7;

		//message takes the form :FROM PRIVMSG TO :MESSAGE

		if (drawfuncs)
			drawfuncs->LocalSound ("misc/talk.wav", 256, 1);

		if ((!strcasecmp(var[4]+1, "\1VERSION\1")) && (!strncmp(var[2], "PRIVMSG ", 7)))
		{
			char *username;
			char delimiters[] = "!";

			username = strtok(var[1]+1, delimiters);

			IRC_AddClientMessage(irc, va("NOTICE %s :\1VERSION FTEQW-IRC-Plugin Release: %s", username, RELEASE));
		}
		else if ((!strcasecmp(var[4]+1, "\1TIME\1")) && (!strncmp(var[2], "PRIVMSG ", 7)))
		{
			char delimiters[] = "!";
			char *username = strtok(var[1], delimiters);
			time_t t;
			const struct tm *tm;
			char buffer[100];

			time(&t);
			tm=localtime(&t);

			strftime (buffer, 100, "%a %b %d %H:%M:%S", tm);

			IRC_AddClientMessage(irc, va("NOTICE %s :\1TIME %s\1", username, buffer));
		}
		else if (exc && col)
		{
			char link[256];
			*col = '\0';
			col++;

			while(*to <= ' ' && *to)
				to++;
			for (end = to + strlen(to)-1; end >= to && *end <= ' '; end--)
				*end = '\0';
			if (!strcmp(to, irc->nick))
				to = prefix;	//This was directed straight at us.
								//So change the 'to', to the 'from'.

			for (end = to; *end; end++)
			{
				if (*end >= 'A' && *end <= 'Z')
					*end = *end + 'a' - 'A';
			}

			*exc = '\0';

			//a link to interact with the sender
			if (Q_snprintfz(link, sizeof(link), "^["COLOURGREEN"%s\\act\\user\\who\\%s^]", prefix, prefix))
				Q_snprintf(link, sizeof(link), "%s", prefix);

			if (!strncmp(col, "\001", 1))
			{
				end = strchr(col+1, '\001');
				if (end)
					*end = '\0';
				if (!strncmp(col+1, "ACTION ", 7))
				{
					IRC_FilterMircColours(col+8);
					IRC_Printf(irc, to, "***%s %s\n", link, col+8);	//from client
				}
				else if (!strncmp(col+1, "PING ", 5))
				{
					time_t currentseconds;

					currentseconds = time (NULL);

					IRC_Printf(irc, to, "CTCP Ping from %s\n", link);	//from client
					IRC_AddClientMessage(irc, va("NOTICE %s :\001PING %u\001\r\n", prefix, (unsigned int)currentseconds));
				}
				else if (!strncmp(col+1, "VERSION ", 8))
				{
					IRC_Printf(irc, to, "CTCP Version from %s\n", link);	//from client
					IRC_AddClientMessage(irc, va("NOTICE %s :\001VERSION "FULLENGINENAME" "RELEASE" \001\r\n", prefix));
				}
				else if (!strncmp(col+1, "FTEICE ", 7))
				{
					IRC_Printf(irc, to, "ICE from %s\n", link);	//from client
					IRC_ICE_Parse(irc, to, col+8);
				}
				else
				{
					if (end)//put it back on. might as well.
						*end = '\001';
					IRC_Printf(irc, to, "%s: %s\n", link, col);	//from client
				}
			}
			else
			{
				IRC_FilterMircColours(col);
				IRC_Printf(irc, to, "%s: %s\n", link, col);	//from client
			}
		}
		else IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN "SERVER: <%s> %s\n", prefix, msg);	//direct server message
	}
	else if (!strncmp(var[2], "MODE ", 5))
	{
		char *username = strtok(var[1]+1, "! ");
		char *mode = strtok(var[4], " ");
		char *target = strtok(var[5], " ");
		char channel[100];

		if (!strncmp(var[3], "#", 1))
		{
			strcpy(channel,strtok(var[3], " "));
		}
		else
		{
			strcpy(channel,DEFAULTCONSOLE);
		}

		if ((!strncmp(mode+1,"o", 1)) || (!strncmp(mode+1,"v",1))) // ops or voice
		{
			IRC_Printf(irc, channel,COLOURGREEN "%s sets mode %s on %s\n",username,mode,target);
		}
		else
		{
			if (IRC_WindowShown(irc, channel))
				IRC_Printf(irc, channel, COLOURGREEN "%s sets mode %s\n",username,mode);
		}

	}
	else if (!strncmp(var[2], "KICK ", 5))
	{
		char *username = strtok(var[1]+1, "!");
		char *channel = strtok(var[3], " ");
		char *target = strtok(var[4], " ");
		char *reason = var[5]+1;

		IRC_Printf(irc, channel,COLOURGREEN "%s was kicked from %s Reason: '%s' by %s\n",target,channel,reason,username);
	}
	else if (!strncmp(msg, "NICK ", 5))
	{
		char *exc = strchr(prefix, '!');
		char *col = strchr(msg+5, ':');
		if (exc && col)
		{
			*exc = '\0';
			//fixme: print this in all channels as appropriate.
			IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN "%s changes name to %s\n", prefix, col+1);
			if (confuncs)
			{
				char oldname[256];
				char newname[256];
				Q_snprintf(oldname, sizeof(oldname), irc->id, prefix);
				Q_snprintf(newname, sizeof(newname), irc->id, col+1);
				confuncs->RenameSub(oldname, newname);	//if we were pming to them, rename accordingly.
			}
		}
		else IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN ":%s%s\n", prefix, msg+6);
	}
	else if (!strncmp(msg, "QUIT ", 5))
	{
		/*char *exc = strchr(prefix, '!');
		char *col = strchr(msg+5, ':');
		if (exc && col)
		{
			*exc = '\0';
			IRC_Printf(irc, col+1, COLOURGREEN "%s joins channel %s\n", prefix, col+1);
		}
		else IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN ":%s QUIT %s\n", prefix, msg+5);*/
	}
	else if (!strncmp(msg, "PART ", 5))
	{
		char *exc = strchr(prefix, '!');
		COM_Parse(msg+5, token, sizeof(token));
		if (exc)
		{
			*exc = '\0';
			IRC_Printf(irc, token, "%s leaves channel %s\n", prefix, token);
		}
		else IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN ":%sPART %s\n", prefix, msg+5);
	}
	else if (!strncmp(msg, "JOIN ", 5))
	{
		char *exc = strchr(prefix, '!');
		char *col = strchr(msg+5, ':');
		if (exc && col)
		{
			*exc = '\0';
			IRC_Printf(irc, col+1, COLOURGREEN "%s joins channel %s\n", prefix, col+1);
		}
		else IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN ":%s JOIN %s\n", prefix, msg+5);
	}
	else if (!strncmp(msg, "372 ", 4))
	{
		char *text = strstr(msg, ":-");
		if (!*irc->autochannels || irc_motd->value)
		{
			if (text)
				IRC_Printf(irc, DEFAULTCONSOLE, "%s\n", text+2);
			else
				IRC_Printf(irc, DEFAULTCONSOLE, "%s\n", msg);
		}
	}
	else if (!strncmp(msg, "TOPIC ", 5))
	{
		char *topic = COM_Parse(msg+5, token, sizeof(token));
		while (*topic == ' ')
			topic++;
		if (*topic++ == ':')
		{
			char *exc = strchr(prefix, '!');
			if (exc)
				*exc = 0;
			IRC_Printf(irc, token, COLOURGREEN "%s changes topic to %s\n", prefix, topic);
		}
		else IRC_Printf(irc, DEFAULTCONSOLE, COLOURGREEN ":%s TOPIC %s\n", prefix, msg+5);
	}
	else if (!strncmp(msg, "331 ", 4) ||//no topic
			 !strncmp(msg, "332 ", 4))	//the topic
	{
		char *topic;
		char *chan;
		topic = COM_Parse(msg, token, sizeof(token));
		topic = COM_Parse(topic, token, sizeof(token));
		topic = COM_Parse(topic, token, sizeof(token));
		while(*topic == ' ')
			topic++;
		if (*topic == ':')
		{
			topic++;
			chan = token;
		}
		else
		{
			topic = "No topic";
			chan = DEFAULTCONSOLE;
		}

		IRC_Printf(irc, chan, "Topic on channel %s is: "COLOURGREEN"%s\n", chan, topic);
	}
	else if (!strncmp(msg, "353 ", 4))	//the names of people on a channel
	{
		char *eq = strstr(msg, "="); // BAD SPIKE!! = is normal channel :(
		char *eq2 = strstr(msg, "@"); // @ means the channel is +s (secret)
		char *eq3 = strstr(msg, "*"); // * means the channel is +p (private) rather redundant...
		char *channeltype = strtok(var[4], " ");
		char *channel = strtok(var[5], " ");
		char *str;


		int secret = 0;
		int privatechan = 0;
		if ( !strcmp(channeltype,"=") )
		{
			char *end;
			eq++;
			str = strstr(eq, ":");
			while(*eq == ' ')
				eq++;
			for (end = eq; *end>' '&&*end !=':'; end++)
				;
			*end = '\0';
			str++;
		}
		//else if (eq2)
		else if ( !strcmp(channeltype,"@") )
		{
			char *end;

			secret = 1;

			eq2++;
			str = strstr(eq2, ":");
			while(*eq2 == ' ')
				eq2++;
			for (end = eq2; *end>' '&&*end !=':'; end++)
				;
			*end = '\0';
			str++;
		}
		else if ( !strcmp(channeltype,"*") )
		{
			char *end;

			privatechan = 1;

			eq3++;
			str = strstr(eq3, ":");
			while(*eq3 == ' ')
				eq3++;
			for (end = eq3; *end>' '&&*end !=':'; end++)
				;
			*end = '\0';
			str++;
		}
		else
		{
			eq = "Corrupted_Message";
			str = NULL;
		}
		IRC_Printf(irc, channel, "Users on channel %s:\n", channel);
		while (str)
		{
			str = COM_Parse(str, token, sizeof(token));
			if (*token == '@')	//they're an operator
				IRC_Printf(irc, channel, "^[@"COLOURGREEN"%s\\act\\user\\who\\%s\\tip\\Channel Operator^]\n", token+1, token+1);
			else if (*token == '%')	//they've got half-op
				IRC_Printf(irc, channel, "^[%%"COLOURGREEN"%s\\act\\user\\who\\%s\\tip\\Channel Half-Operator^]\n", token+1, token+1);
			else if (*token == '+')	//they've got voice
				IRC_Printf(irc, channel, "^[+"COLOURGREEN"%s\\act\\user\\who\\%s\\tip\\Voice^]\n", token+1, token+1);
			else
				IRC_Printf(irc, channel, " ^["COLOURGREEN"%s\\act\\user\\who\\%s^]\n", token, token);
		}
		if (secret == 1)
		{
			IRC_Printf(irc, channel, "%s is secret (+s)\n",channel);
		}
		else if (privatechan == 1)
		{
			IRC_Printf(irc, channel, "%s is private (+p)\n",channel);
		}

	}
	// would be great to convert the above to work better
	else if (atoi(var[2]) != 0)
	{
//		char *rawparameter = strtok(var[4], " ");
//		char *rawmessage = var[5];
//		char *wholerawmessage = var[4];

		numbered_command(atoi(var[2]), msg, irc);

		if (irc_debug->value == 1) { IRC_Printf(irc, DEFAULTCONSOLE, "%s\n", msg); }
	}
	else
		IRC_Printf(irc, DEFAULTCONSOLE, "%s\n", msg);

	memmove(irc->bufferedinmessage, nextmsg, irc->bufferedinammount - (msg-irc->bufferedinmessage));
	irc->bufferedinammount-=nextmsg-irc->bufferedinmessage;
	return IRC_CONTINUE;
}

//functions above this line allow connections to multiple servers.
//it is just the control functions that only allow one server.

void IRC_Frame(double realtime, double gametime)
{
	ircclient_t *ircclient;
	if (reloadconfig)
	{
		reloadconfig = false;
		IRC_ParseConfig();
	}
	for (ircclient = ircclients; ircclient; ircclient = ircclient->next)
	{
		int stat = IRC_CONTINUE;
		if (!handleisvalid(ircclient->socket))
			continue;	//this connection isn't enabled.
		while(stat == IRC_CONTINUE)
		{
			stat = IRC_ClientFrame(ircclient);
			if (ircclient->bufferedoutammount)
			{
				int flushed = netfuncs->Send(ircclient->socket, ircclient->bufferedoutmessage, ircclient->bufferedoutammount);	//FIXME: This needs rewriting to cope with errors+throttle.
				if (flushed > 0)
				{
					memmove(ircclient->bufferedoutmessage, ircclient->bufferedoutmessage+flushed, ircclient->bufferedoutammount - flushed);
					ircclient->bufferedoutammount -= flushed;
				}
			}
		}
		if (ircclient->quitting && !ircclient->bufferedoutammount)
			stat = IRC_KILL;
		if (stat == IRC_KILL)
		{
			netfuncs->Close(ircclient->socket);
			ircclient->socket = invalid_handle;
			IRC_Printf(ircclient, DEFAULTCONSOLE, "Disconnected from irc\n^[[Reconnect]\\act\\reconnect^]\n");
			break;	//lazy
		}
		else
			IRC_ICE_Frame(ircclient);
	}
}

void IRC_Command(ircclient_t *ircclient, char *dest, char *args)
{
	char token[1024];
	char *msg;

	msg = COM_Parse(args, token, sizeof(token));

	if (*token == '/')
	{
		if (!strcmp(token+1, "server"))
		{	//selects the default server without connecting anywhere, for main console to use.
			msg = COM_Parse(msg, token, sizeof(token));

			ircclient = IRC_FindAccount(token);
			if (!ircclient)
				IRC_Printf(ircclient, dest, "No such connection\n");
			else if (ircclients == ircclient)
				IRC_Printf(ircclient, dest, "Connection is already the default.\n");
			else
			{
				IRC_MakeDefault(ircclient);
				IRC_Printf(ircclient, dest, "Connection is now default.\n");
			}
		}
		else if (!strcmp(token+1, "open") || !strcmp(token+1, "connect"))
		{
			char server[256];
			char channels[1024];
			char nick[256];
			char password[256];

			msg = COM_Parse(msg, server, sizeof(server));
			msg = COM_Parse(msg, channels, sizeof(channels));
			msg = COM_Parse(msg, nick, sizeof(nick));
			msg = COM_Parse(msg, password, sizeof(password));

			//set up some defaults
			if (!*nick)
				Q_strlcpy(nick, irc_nick->string, sizeof(nick));
			if (!*nick)
				cvarfuncs->GetString("name", nick, sizeof(nick));

			ircclient = IRC_FindAccount(server);
			if (ircclient)
			{
				if (handleisvalid(ircclient->socket))	//don't need to do anything.
				{
					IRC_Printf(ircclient, dest, "IRC connection to %s already registered\n", server);
					return;	//silently ignore it if the account already exists
				}
			}
			else
				ircclient = IRC_Create(server, nick, irc_realname->string, irc_hostname->string, irc_username->string, password, channels);
			if (ircclient)
			{
				IRC_MakeDefault(ircclient);
				ircclient->persist |= !strcmp(token+1, "connect");
				if (IRC_Establish(ircclient))
					IRC_Printf(ircclient, dest, "Trying to connect\n");
				else
					IRC_Printf(ircclient, dest, "Unable to connect\n");
			}
			else
				IRC_Printf(ircclient, dest, "Unable to open account\n");

			IRC_WriteConfig();
		}
		else if (!strcmp(token+1, "help"))
		{
			IRC_Printf(ircclient, dest, "to connect to a server: /connect SERVER \"#chan #chan2,chan2password\" NICK SERVERPASSWORD\n");
			IRC_Printf(ircclient, dest, "to disconnect from a server: /quit\n");
			IRC_Printf(ircclient, dest, "to join a channel: /join\n");
			IRC_Printf(ircclient, dest, "to leave a channel: /part\n");
			IRC_Printf(ircclient, dest, "note that servers and channels will be remembered\n");
		}
		else if (!strcmp(token+1, "nick"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			if (!ircclient)	//not yet connected.
				cvarfuncs->SetString(irc_nick->name, token);
			else
			{
				if (!handleisvalid(ircclient->socket))
					Q_strlcpy(ircclient->primarynick, token, sizeof(ircclient->primarynick));
				ircclient->nicktries = 0;
				IRC_SetNick(ircclient, token);
			}

			IRC_WriteConfig();
		}
		else if (!strcmp(token+1, "user"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			cvarfuncs->SetString(irc_username->name, token);
			if (ircclient)
				IRC_SetUser(ircclient, token);

			IRC_WriteConfig();
		}
		else if (!strcmp(token+1, "info") || !strcmp(token+1, "status"))
		{
			ircclient_t *e;
			struct ircice_s *ice;
			for (e = ircclients; e; e = e->next)
			{
				IRC_Printf(ircclient, dest, "SERVER: ^[%s\\type\\irc /server \"%s\"^]\n", e->server, e->server);
				if (e->connecting && handleisvalid(e->socket))
					IRC_Printf(ircclient, dest, "<CONNECTING>\n");
				else if (handleisvalid(e->socket))
					IRC_Printf(ircclient, dest, "<CONNECTED>\n");
				else
					IRC_Printf(ircclient, dest, "<DISCONNECTED>\n");
				if (e->quitting)
					IRC_Printf(ircclient, dest, "<QUITTING>\n");
				switch(e->tlsmode)
				{
				default:
				case TLS_OFF:
					IRC_Printf(ircclient, dest, "TLS: insecure\n");
					break;
				case TLS_INITIAL:
					IRC_Printf(ircclient, dest, "TLS: initial\n");
					break;
				case TLS_START:
				case TLS_STARTING:
					IRC_Printf(ircclient, dest, "TLS: upgrade\n");
					break;
				}
				IRC_Printf(ircclient, dest, "nick: %s\n", e->nick);
				IRC_Printf(ircclient, dest, "realname: %s\n", e->realname);
				IRC_Printf(ircclient, dest, "hostname: %s\n", e->hostname);
				IRC_Printf(ircclient, dest, "rejoin: %s\n", *e->autochannels?e->autochannels:"<no channels>");
				IRC_Printf(ircclient, dest, "default dest: %s\n", e->defaultdest);
				for (ice = e->ice; ice; ice = ice->next)
				{
					char *allowed=ice->allowed?" allowed":" not-allowed";
					char *accepted=ice->accepted?" accepted":" not-accepted";
					switch(ice->type)
					{
					default:
					case ICEP_INVALID:
						IRC_Printf(ircclient, dest, " <INVALID ICE>\n");
						break;
					case ICEP_QWSERVER:
						IRC_Printf(ircclient, dest, " server: \"%s\"%s%s\n", ice->peer, allowed, accepted);
						break;
					case ICEP_QWCLIENT:
						IRC_Printf(ircclient, dest, " client: \"%s\"%s%s\n", ice->peer, allowed, accepted);
						break;
					case ICEP_VOICE:
						IRC_Printf(ircclient, dest, " voice: \"%s\"%s%s\n", ice->peer, allowed, accepted);
						break;
					case ICEP_VIDEO:
						IRC_Printf(ircclient, dest, " voice: \"%s\"%s%s\n", ice->peer, allowed, accepted);
						break;
					}
				}
			}
		}
		else if (!ircclient)
		{
			IRC_Printf(ircclient, dest, "Not connected, please connect to an irc server first.\n");
		}

		//ALL other commands require you to be connected.
		else if (!strcmp(token+1, "list"))
		{
			IRC_AddClientMessage(ircclient, "LIST");
		}
		else if ( !strcmp(token+1, "join") || !strcmp(token+1, "j") )
		{
			char chan[256];
			char pwd[256];

			if (ircclient->tlsmode == TLS_STARTING || ircclient->connecting)
			{
				IRC_Printf(ircclient, dest, "Still connecting. Please wait.\n");
				return;
			}

			msg = COM_Parse(msg, chan, sizeof(chan));
			msg = COM_Parse(msg, pwd, sizeof(pwd));

			IRC_JoinChannel(ircclient,chan,pwd);
			IRC_WriteConfig();
		}
		else if (!strcmp(token+1, "part") || !strcmp(token+1, "leave")) // need to implement leave reason
		{
			msg = COM_Parse(msg, token, sizeof(token));
			IRC_PartChannel(ircclient, *token?token:dest);
			IRC_WriteConfig();
		}
		else if (!strcmp(token+1, "call"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			if (ircclient->tlsmode == TLS_STARTING || ircclient->connecting)
			{
				IRC_Printf(ircclient, dest, "Still connecting. Please wait.\n");
				return;
			}
			{
				struct ircice_s *ice = IRC_ICE_Create(ircclient, *token?token:dest, ICEP_VOICE, true);
				if (ice)
				{
					IRC_ICE_Update(ircclient, ice, '+');
					IRC_Printf(ircclient, ice->peer, "<Calling %s>\n", ice->peer);
				}
			}
		}
		else if (!strcmp(token+1, "ginvite"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			if (ircclient->tlsmode == TLS_STARTING || ircclient->connecting)
			{
				IRC_Printf(ircclient, dest, "Still connecting. Please wait.\n");
				return;
			}
			{
				struct ircice_s *ice = IRC_ICE_Create(ircclient, *token?token:dest, ICEP_QWSERVER, true);
				if (ice)
				{
					IRC_ICE_Update(ircclient, ice, '+');
					IRC_Printf(ircclient, ice->peer, "<inviting %s>\n", ice->peer);
				}
			}
		}
		else if (!strcmp(token+1, "gatecrash"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			if (ircclient->tlsmode == TLS_STARTING || ircclient->connecting)
			{
				IRC_Printf(ircclient, dest, "Still connecting. Please wait.\n");
				return;
			}
			{
				struct ircice_s *ice = IRC_ICE_Create(ircclient, *token?token:dest, ICEP_QWCLIENT, true);
				if (ice)
				{
					IRC_ICE_Update(ircclient, ice, '+');
					IRC_Printf(ircclient, ice->peer, "<gatecrashing %s>\n", ice->peer);
				}
			}
		}
		else if (!strcmp(token+1, "msg"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			if (!msg)
				return;
			if (ircclient->tlsmode == TLS_STARTING || ircclient->connecting)
			{
				IRC_Printf(ircclient, dest, "Still connecting. Please wait.\n");
				return;
			}
			IRC_AddClientMessage(ircclient, va("PRIVMSG %s :%s", token, msg+1));
			IRC_Printf(ircclient, token, "%s: %s\n", ircclient->nick, msg);
		}
		else if (!strcmp(token+1, "quote") || !strcmp(token+1, "raw"))
		{
			IRC_AddClientMessage(ircclient, va("%s", msg));
		}
		else if (!strcmp(token+1, "reconnect"))
		{
			if (IRC_Establish(ircclient))
				IRC_Printf(ircclient, dest, "Trying to connect\n");
			else
				IRC_Printf(ircclient, dest, "Unable to connect\n");
		}
		else if (!strcmp(token+1, "quit") || !strcmp(token+1, "disconnect"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			if (*token)
				IRC_AddClientMessage(ircclient, va("QUIT :%s", token));
			else
				IRC_AddClientMessage(ircclient, va("QUIT :%s", irc_quitmessage->string));
			ircclient->quitting = true;

			IRC_WriteConfig();
		}
		else if (!strcmp(token+1, "whois"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			IRC_AddClientMessage(ircclient, va("WHOIS :%s",token));
		}
		else if (!strcmp(token+1, "away"))
		{
			if ( strlen(msg) > 1 )
				IRC_AddClientMessage(ircclient, va("AWAY :%s",msg+1));
			else
				IRC_AddClientMessage(ircclient, va("AWAY :"));
		}
		else if (!strcmp(token+1, "motd"))
		{
			IRC_AddClientMessage(ircclient, "MOTD");
		}
		else if (!strcmp(token+1, "ctcp"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			IRC_AddClientMessage(ircclient, va("PRIVMSG %s :\1%s\1",token,msg+1));
		}
		else if (!strcmp(token+1, "dest"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			Q_strlcpy(ircclient->defaultdest, token, sizeof(ircclient->defaultdest));
		}
		else if (!strcmp(token+1, "ping"))
		{
			if (!*dest)
				IRC_Printf(ircclient, DEFAULTCONSOLE, "No channel joined. Try /join #<channel>\n");
			else
				IRC_AddClientMessage(ircclient, va("PRIVMSG %s :\001PING%s\001", dest, msg));
		}
		else if (!strcmp(token+1, "notice"))
		{
			msg = COM_Parse(msg, token, sizeof(token));
			IRC_AddClientMessage(ircclient, va("NOTICE %s :%s",token, msg+1));
		}
		else if (!strcmp(token+1, "me"))
		{
			if (!*dest)
				IRC_Printf(ircclient, DEFAULTCONSOLE, "No channel joined. Try /join #<channel>\n");
			else
			{
				if(*msg <= ' ' && *msg)
					msg++;
				IRC_AddClientMessage(ircclient, va("PRIVMSG %s :\001ACTION %s\001", dest, msg));
				IRC_Printf(ircclient, dest, "***^3%s^7 %s\n", ircclient->nick, msg);
			}
		}
		else if (!strcmp(token+1, "topic"))
		{
			if (!*dest)
				IRC_Printf(ircclient, DEFAULTCONSOLE, "No channel joined. Try /join #<channel>\n");
			else
			{
				if(*msg <= ' ' && *msg)
					msg++;
				IRC_AddClientMessage(ircclient, va("TOPIC %s :%s", dest, msg));
			}
		}
		else
			IRC_Printf(ircclient, dest, "Command not recognised\n");
	}
	else
	{
		if (ircclient)
		{
			if (!handleisvalid(ircclient->socket))
				IRC_Printf(ircclient, dest, "Connection was closed. use /reconnect\n");
			else if (ircclient->tlsmode == TLS_STARTING || ircclient->connecting)
				IRC_Printf(ircclient, dest, "Still connecting. Please wait.\n");
			else if (!*dest)
			{
				IRC_Printf(ircclient, dest, "No channel joined. Try /join #<channel>\n");
			}
			else
			{
				msg = args;
				while (*msg == ' ')
					msg++;
				if (!*msg)
					return;	//this is apparently an error. certainly wasteful.
				IRC_AddClientMessage(ircclient, va("PRIVMSG %s :%s", dest, msg));
				IRC_Printf(ircclient, dest, "^3%s^7: %s\n", ircclient->nick, msg);
			}
		}
		else
			IRC_Printf(ircclient, "Not connected\ntype \"%s /open IRCSERVER [#channel1[,#channel2[,...]]] [nick]\" to connect\n", COMMANDNAME);
	}
}
