#include "cl_master.h"




int	K_UPARROW,
	K_DOWNARROW,
	K_ENTER,
	K_DEL,
	K_BACKSPACE,
	K_ESCAPE,
	K_PGDN,
	K_PGUP,
	K_SPACE,
	K_LEFTARROW,
	K_RIGHTARROW;

#define RESTRICT_LOCAL 64

enum {
SLISTTYPE_SERVERS,
SLISTTYPE_FAVORITES,
SLISTTYPE_SOURCES,
SLISTTYPE_OPTIONS	//must be last
} slist_option;

int slist_numoptions;
int slist_firstoption;

int slist_type;

void M_DrawServers(void);
void M_SListKey(int key);

#define CVAR_ARCHIVE 0

#define cvargroup "Server Browser Vars"
//filtering
vmcvar_t	sb_hideempty		= {"sb_hideempty",		"0",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_hidenotempty		= {"sb_hidenotempty",	"0",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_hidefull			= {"sb_hidefull",		"0",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_hidedead			= {"sb_hidedead",		"1",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_hidequake2		= {"sb_hidequake2",		"1",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_hidenetquake		= {"sb_hidenetquake",	"1",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_hidequakeworld	= {"sb_hidequakeworld",	"0",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_maxping			= {"sb_maxping",		"0",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_gamedir			= {"sb_gamedir",		"",		cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_mapname			= {"sb_mapname",		"",		cvargroup, CVAR_ARCHIVE};

vmcvar_t	sb_showping			= {"sb_showping",		"1",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_showaddress		= {"sb_showaddress",	"0",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_showmap			= {"sb_showmap",		"0",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_showgamedir		= {"sb_showgamedir",	"0",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_showplayers		= {"sb_showplayers",	"1",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_showfraglimit	= {"sb_showfraglimit",	"0",	cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_showtimelimit	= {"sb_showtimelimit",	"0",	cvargroup, CVAR_ARCHIVE};

vmcvar_t	sb_filterkey		= {"sb_filterkey",		"hostname", cvargroup, CVAR_ARCHIVE};
vmcvar_t	sb_filtervalue		= {"sb_filtervalue",	"",		cvargroup, CVAR_ARCHIVE};
#undef cvargroup 

vmcvar_t	*cvarlist[] ={
	&sb_hideempty,
	&sb_hidenotempty,
	&sb_hidefull,
	&sb_hidedead,
	&sb_hidequake2,
	&sb_hidenetquake,
	&sb_hidequakeworld,
	&sb_maxping,
	&sb_gamedir,
	&sb_mapname,

	&sb_showping,
	&sb_showaddress,
	&sb_showmap,
	&sb_showgamedir,
	&sb_showplayers,
	&sb_showfraglimit,
	&sb_showtimelimit,

	&sb_filterkey,
	&sb_filtervalue
};

void M_Serverlist_InitCvars(void)
{
	vmcvar_t *v;
	int i;
	for (v = cvarlist[0],i=0; i < sizeof(cvarlist)/sizeof(cvarlist[0]); v++, i++)
		v->handle = Cvar_Register(v->name, v->string, v->flags, v->group);
}

int msecstime;
int Plug_Tick(int *args)
{
	msecstime = args[0];

	return true;
}

int Plug_MenuEvent(int *args)
{
	switch(args[0])
	{
	case 0:	//draw
		M_DrawServers();
		break;
	case 1:	//keydown
		M_SListKey(args[1]);
		break;
	case 2:	//keyup
		break;
	case 3:	//menu closed (this is called even if we change it).
		break;
	}

	return 0;
}

void Plug_StartBrowser(void)
{
	Menu_Control(1);
	if (BUILTINISVALID(LocalSound))
		LocalSound("misc/menu2.wav");

	MasterInfo_Begin();
}

int Plug_ExecuteCommand(int *args)
{
	char cmd[256];
	Cmd_Argv(0, cmd, sizeof(cmd));
	if (!strcmp("plug_browser", cmd))
	{
		Plug_StartBrowser();
		return 1;
	}
	return 0;
}

int Plug_Init(int *args)
{
	if (Plug_Export("Tick", Plug_Tick) &&
		Plug_Export("ExecuteCommand", Plug_ExecuteCommand) &&
		Plug_Export("MenuEvent", Plug_MenuEvent))
		Con_Print("IRC Client Plugin Loaded\n");
	else
	{
		Con_Print("IRC Client Plugin failed\n");
		return false;
	}

	K_UPARROW		= Key_GetKeyCode("uparrow");
	K_DOWNARROW		= Key_GetKeyCode("downarrow");
	K_ENTER			= Key_GetKeyCode("enter");
	K_DEL			= Key_GetKeyCode("del");
	K_BACKSPACE		= Key_GetKeyCode("backspace");
	K_ESCAPE		= Key_GetKeyCode("escape");
	K_PGDN			= Key_GetKeyCode("pgdn");
	K_PGUP			= Key_GetKeyCode("pgup");
	K_SPACE			= Key_GetKeyCode("space");
	K_LEFTARROW		= Key_GetKeyCode("leftarrow");
	K_RIGHTARROW	= Key_GetKeyCode("rightarrow");

	M_Serverlist_InitCvars();
	return true;
	
}

//doesn't use args or return value
int Plugin_CvarUpdate(int *args)
{
	vmcvar_t *v;
	int i;
	for (v = cvarlist[0],i=0; i < sizeof(cvarlist)/sizeof(cvarlist[0]); v++, i++)
		v->modificationcount = Cvar_Update(v->handle, v->modificationcount, v->string, &v->value);
	return 0;
}

static void NM_DrawCharacter (int cx, int line, unsigned int num)
{
	Draw_Character(cx, line, num);
}
static void NM_Print (int cx, int cy, qbyte *str)
{
	while (*str)
	{
		Draw_Character (cx, cy, (*str)|128);
		str++;
		cx += 8;
	}
}





qboolean M_IsFiltered(serverinfo_t *server)	//figure out if we should filter a server.
{
	if (slist_type == SLISTTYPE_FAVORITES)
		if (!(server->special & SS_FAVORITE))
			return true;
#ifdef Q2CLIENT
	if (sb_hidequake2.value)
#endif
		if (server->special & SS_QUAKE2)
			return true;
#ifdef NQPROT
	if (sb_hidenetquake.value)
#endif
		if (server->special & SS_NETQUAKE)
			return true;
	if (sb_hidequakeworld.value)
		if (!(server->special & (SS_QUAKE2|SS_NETQUAKE)))
			return true;
	if (sb_hideempty.value)
		if (!server->players)
			return true;
	if (sb_hidenotempty.value)
		if (server->players)
			return true;
	if (sb_hidefull.value)
		if (server->players == server->maxplayers)
			return true;
	if (sb_hidedead.value)
		if (server->maxplayers == 0)
			return true;
	if (sb_maxping.value)
		if (server->ping > sb_maxping.value)
			return true;
	if (*sb_gamedir.string)
		if (strcmp(server->gamedir, sb_gamedir.string))
			return true;
	if (*sb_mapname.string)
		if (!strstr(server->map, sb_mapname.string))
			return true;
	
	return false;
}

qboolean M_MasterIsFiltered(master_t *mast)
{
#ifndef Q2CLIENT
	if (mast->type == MT_BCASTQ2 || mast->type == MT_SINGLEQ2 || mast->type == MT_MASTERQ2)
		return true;
#endif
#ifndef NQPROT
	if (mast->type == MT_BCASTNQ || mast->type == MT_SINGLENQ)
		return true;
#endif
	return false;
}

static int	Sbar_ColorForMap (int m)
{
	m = (m < 0) ? 0 : ((m > 13) ? 13 : m);

	m *= 16;
	return m < 128 ? m + 8 : m + 8;
}

void M_DrawOneServer (int inity)
{
	char	key[512];
	char	value[512];
	char	*o;
	int		l, i;
	char *s;
	
	int miny=8*5;
	int y=8*(5-selectedserver.linenum);	

	miny += inity;
	y += inity;

	if (!selectedserver.detail)
	{
		NM_Print (0, y, "No details\n");
		return;
	}

	s = selectedserver.detail->info;

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
		if (y>=miny)
			NM_Print (0, y, va("%19s", key));

		if (!*s)
		{
			if (y>=miny)
				NM_Print (0, y, "MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		if (y>=miny)
			NM_Print (320/2, y, va("%s\n", value));

		y+=8;
	}

	for ( i = 0; i < selectedserver.detail->numplayers; i++)
	{
		if (y>=miny)
		{
			if (selectedserver.detail->players[i].frags>=-999)	//wow, too low, assume mvd spectators...
			{
				Draw_Colourp(Sbar_ColorForMap(selectedserver.detail->players[i].topc));
				Draw_Fill (12, y, 28, 4);
				Draw_Colourp(Sbar_ColorForMap(selectedserver.detail->players[i].botc));
				Draw_Fill (12, y+4, 28, 4);
				Draw_Colour3f(1,1,1);
				NM_Print (12, y, va("%3i", selectedserver.detail->players[i].frags));
			}
			NM_Print (12+8*4, y, selectedserver.detail->players[i].name);
		}
		y+=8;
	}

	if (y<=miny)	//whoops, there was a hole at the end, try scrolling up.
		selectedserver.linenum--;
}

char *Info_ValueForKey(char *buffer, char *key)
{
	char *s;
	char *e;
	static char ret[64];
	for (s = buffer; *s; s++)
	{
		if (*s == '\\')
		{	//key starts here
			for (e = s+1; *e; e++)
			{
				//find value start
				if (*e == '\\')
				{
					if (!strncmp(s+1, key, e - s-1))
					{
						//find the next \\ or \0
						s = e;
						for (e = s+1; *e && *e != '\\'; e++)
						{

						}
						if (e-s>=sizeof(ret))
							e = s + 63;
						strncpy(ret, s+1, e-s-1);
						ret[e-s-1] = '\0';
						return ret;
					}
					break;
				}
			}
			s = e;
		}
	}
	return "";
}

int M_AddColumn (int right, int y, char *text, int maxchars)
{
	int left;
	left = right - maxchars*8;
	if (left < 0)
		return right;

	right = left;
	while (*text && maxchars>0)
	{
		NM_DrawCharacter (right, y, *text);
		text++;
		right += 8;
		maxchars--;
	}
	return left;
}
void M_DrawServerList(void)
{
	serverinfo_t *server;
	int op=0, filtered=0;
	int snum=0;
	int blink = 0;

	int x;
	int y = 8*3;

	CL_QueryServers();
	
	slist_numoptions = 0;

	//find total servers.
	for (server = firstserver; server; server = server->next)
		if (M_IsFiltered(server))
			filtered++;
		else
			slist_numoptions++;

	if (!slist_numoptions)
	{
		char *text, *text2="", *text3="";
		if (filtered)
		{
			if (slist_type == SLISTTYPE_FAVORITES)
			{
				text	= "Highlight a server";
				text2	= "and press \'f\'";
				text3	= "to add it to this list";
			}
			else
				text = "All servers were filtered out";
		}
		else
			text = "No servers found";
		NM_Print((vid.width-strlen(text)*8)/2, 8*5, text);
		NM_Print((vid.width-strlen(text2)*8)/2, 8*5+8, text2);
		NM_Print((vid.width-strlen(text3)*8)/2, 8*5+16, text3);

		return;
	}

	
	if (slist_option >= slist_numoptions)
		slist_option = slist_numoptions-1;
	op = vid.height/2/8;
	op/=2;
	op=slist_option-op;
	snum = op;


	if (selectedserver.inuse == true)
	{
		M_DrawOneServer(8*5);
		return;
	}

	if (op < 0)
		op = 0;
	if (snum < 0)
		snum = 0;
	//find the server that we want
	for (server = firstserver; op>0; server=server->next)
	{
		if (M_IsFiltered(server))
			continue;
		op--;
	}

	y = 8*2;
	x = vid.width;
	if (sb_showtimelimit.value)
		x = M_AddColumn(x, y, "tl",			3);
	if (sb_showfraglimit.value)
		x = M_AddColumn(x, y, "fl",			3);
	if (sb_showplayers.value)
		x = M_AddColumn(x, y, "plyrs",		6);
	if (sb_showmap.value)
		x = M_AddColumn(x, y, "map",		9);
	if (sb_showgamedir.value)
		x = M_AddColumn(x, y, "gamedir",	9);
	if (sb_showping.value)
		x = M_AddColumn(x, y, "png",		4);
	if (sb_showaddress.value)
		x = M_AddColumn(x, y, "address",	21);
	x = M_AddColumn(x, y, "name",		x/8-1);

	y = 8*3;
	while(server)
	{
		if (M_IsFiltered(server))
		{
			server = server->next;
			continue;	//doesn't count
		}

		if (y > vid.height/2)
			break;

		if (slist_option == snum)
			blink = (msecstime/333)&1;
		if (*server->name)
		{
			if (blink)
				Draw_Colour3f(1,1,0);
			else if (server->special & SS_FAVORITE)
				Draw_Colour3f(0,1,0);
			else if (server->special & SS_FTESERVER)
				Draw_Colour3f(1,0,0);
			else if (server->special & SS_QUAKE2)
				Draw_Colour3f(0,0,1);
			else if (server->special & SS_NETQUAKE)
				Draw_Colour3f(1,0,1);
			else
				Draw_Colour3f(1,1,1);

			x = vid.width;

			if (sb_showtimelimit.value)
				x = M_AddColumn(x, y, va("%i", server->tl),			3);	//time limit
			if (sb_showfraglimit.value)
				x = M_AddColumn(x, y, va("%i", server->fl),			3);	//frag limit
			if (sb_showplayers.value)
				x = M_AddColumn(x, y, va("%i/%i", server->players, server->maxplayers),			6);
			if (sb_showmap.value)
				x = M_AddColumn(x, y, server->map,		9);
			if (sb_showgamedir.value)
				x = M_AddColumn(x, y, server->gamedir,	9);
			if (sb_showping.value)
				x = M_AddColumn(x, y, va("%i", server->ping),			4);	//frag limit
			if (sb_showaddress.value)
				x = M_AddColumn(x, y, NET_AdrToString(&server->adr),	21);
			x = M_AddColumn(x, y, server->name,		x/8-1);
		}

		blink = 0;
		if (*server->name)
			y+=8;

		server = server->next;

		snum++;
	}

	Draw_Colour3f(1,1,1);

	selectedserver.inuse=2;
	M_DrawOneServer(vid.height/2-4*8);
}

void M_DrawSources (void)
{
	int blink;
	int snum=0;
	int op;
	int y = 3*8;
	master_t *mast;

	slist_numoptions = 0;
	//find total sources.
	for (mast = master; mast; mast = mast->next)
		slist_numoptions++;

	if (!slist_numoptions)
	{
		char *text;
		if (0)//filtered)
			text = "All servers were filtered out\n";
		else
			text = "No sources were found\n";
		NM_Print((vid.width-strlen(text)*8)/2, 8*5, text);

		return;
	}

	if (slist_option >= slist_numoptions)
		slist_option = slist_numoptions-1;
	op=slist_option-vid.height/2/8;
	snum = op;

	if (op < 0)
		op = 0;
	if (snum < 0)
		snum = 0;
	//find the server that we want
	for (mast = master; op>0; mast=mast->next)
	{
		if (M_MasterIsFiltered(mast))
			continue;
		op--;
	}

	for (; mast; mast = mast->next)
	{
		if (M_MasterIsFiltered(mast))
			continue;

		if (slist_option == snum)
			blink = (msecstime/333)&1;
		else
			blink = 0;

		if (blink)
			Draw_Colour3f(0,1,1);
		else
			if (mast->type == MT_MASTERQW || mast->type == MT_MASTERQ2)
			Draw_Colour3f(1,1,1);
#ifdef NQPROT
		else if (mast->type == MT_SINGLENQ)
			Draw_Colour3f(0,1,0);
#endif
		else if (mast->type == MT_SINGLEQW || mast->type == MT_SINGLEQ2)
			Draw_Colour3f(0,0,1);
		else
			Draw_Colour3f(1,0,0);

		NM_Print(46, y, va("%s", mast->name));	//white.
		y+=8;
		snum++;
	}
}

#define NUMSLISTOPTIONS (7+7+3)
	struct {
		char *title;
		vmcvar_t *cvar;
		int type;
	} options[NUMSLISTOPTIONS] = {
		{"Hide Empty",		&sb_hideempty},
		{"Hide Not Empty",	&sb_hidenotempty},
		{"Hide Full",		&sb_hidefull},
		{"Hide Dead",		&sb_hidedead},
		{"Hide Quake 2",	&sb_hidequake2},
		{"Hide Quake 1",	&sb_hidenetquake},
		{"Hide QuakeWorld",	&sb_hidequakeworld},

		{"Show pings",		&sb_showping},
		{"Show Addresses",	&sb_showaddress},
		{"Show map",		&sb_showmap},
		{"Show Game Dir",	&sb_showgamedir},
		{"Show Players",	&sb_showplayers},
		{"Show Fraglimit",	&sb_showfraglimit},
		{"Show Timelimit",	&sb_showtimelimit},

		{"Max ping",		&sb_maxping,	1},
		{"GameDir",			&sb_gamedir,	2},
		{"Using map",		&sb_mapname,	2}
	};

void M_DrawSListOptions (void)
{
	int op;

	slist_numoptions = NUMSLISTOPTIONS;

	for (op = 0; op < NUMSLISTOPTIONS; op++)
	{
		if (slist_option == op && (msecstime/333)&1)
			Draw_Colour3f(0.5, 0.5, 1);
		else
		{
			if (options[op].cvar->value>0 || (*options[op].cvar->string && *options[op].cvar->string != '0'))
				Draw_Colour3f(1, 0, 0);
			else
				Draw_Colour3f(1, 1, 1);
		}
		switch(options[op].type)
		{
		default:
			NM_Print(46, op*8+8*3, options[op].title);
			break;
		case 1:
			if (!options[op].cvar->value)
			{
				NM_Print(46, op*8+8*3, va("%s ", options[op].title));
				break;
			}
		case 2:
			NM_Print(46, op*8+8*3, va("%s %s", options[op].title, options[op].cvar->string));
			break;
		}
	}
}

void M_SListOptions_Key (int key)
{
	if (key == K_UPARROW)
	{
		slist_option--;
		if (slist_option<0)
			slist_option=0;
	}
	else if (key == K_DOWNARROW)
	{
		slist_option++;
		if (slist_option >= slist_numoptions)
			slist_option = slist_numoptions-1;
	}
	
	switch(options[slist_option].type)
	{
	default:
		if (key == K_ENTER)
		{
			if (options[slist_option].cvar->value)
				VMCvar_SetString(options[slist_option].cvar, "0");
			else
				VMCvar_SetString(options[slist_option].cvar, "1");
		}
		break;
	case 1:
		if (key >= '0' && key <= '9')
			VMCvar_SetFloat(options[slist_option].cvar, options[slist_option].cvar->value*10+key-'0');
		else if (key == K_DEL)
			VMCvar_SetFloat(options[slist_option].cvar, 0);
		else if (key == K_BACKSPACE)
			VMCvar_SetFloat(options[slist_option].cvar, (int)options[slist_option].cvar->value/10);
		break;
	case 2:
		if ((key >= '0' && key <= '9') || (key >= 'a' && key <= 'z') || key == '_')
			VMCvar_SetString(options[slist_option].cvar, va("%s%c", options[slist_option].cvar->string, key));
		else if (key == K_DEL)
			VMCvar_SetString(options[slist_option].cvar, "");
		else if (key == K_BACKSPACE)	//FIXME
			VMCvar_SetString(options[slist_option].cvar, "");
		break;
	}
}


void M_DrawServers(void)
{
#define NUMSLISTHEADERS (SLISTTYPE_OPTIONS+1)
	char *titles[NUMSLISTHEADERS] = {
		"Servers",
		"Favorites",
		"Sources",
//		"Players",
		"Options"
	};
	int snum=0;

	int width, lofs;

	NET_CheckPollSockets();	//see if we were told something important.

	width = vid.width / NUMSLISTHEADERS;
	lofs = width/2 - 7*4;
	for (snum = 0; snum < NUMSLISTHEADERS; snum++)
	{
		NM_Print(width*snum+width/2 - strlen(titles[snum])*4, slist_type==snum, titles[snum]);
	}
	NM_Print(8, 8, "\35");
	for (snum = 16; snum < vid.width-16; snum+=8)
		NM_Print(snum, 8, "\36");
	NM_Print(snum, 8, "\37");

	switch(slist_type)
	{
	case SLISTTYPE_SERVERS:
	case SLISTTYPE_FAVORITES:
		M_DrawServerList();
		break;
	case SLISTTYPE_SOURCES:
		M_DrawSources ();
		break;
	case SLISTTYPE_OPTIONS:
		M_DrawSListOptions ();
		break;
	}
}

serverinfo_t *M_FindCurrentServer(void)
{
	serverinfo_t *server;
	int op = slist_option;
	for (server = firstserver; server; server = server->next)
	{
		if (M_IsFiltered(server))
			continue;	//doesn't count
		if (!op--)
			return server;
	}
	return NULL;
}

master_t *M_FindCurrentMaster(void)
{
	master_t *mast;
	int op = slist_option;
	for (mast = master; mast; mast = mast->next)
	{
		if (M_MasterIsFiltered(mast))
			continue;
		if (!op--)
			return mast;
	}
	return NULL;
}

void M_SListKey(int key)
{
	if (key == K_ESCAPE)
	{
//		if (selectedserver.inuse)
//			selectedserver.inuse = false;
//		else
		{
			Menu_Control(0);
//			Cmd_AddText("togglemenu\n", false);
		}
		return;
	}
	else if (key == K_LEFTARROW)
	{
		slist_type--;
		if (slist_type<0)
			slist_type=0;

		selectedserver.linenum--;
		if (selectedserver.linenum<0)
			selectedserver.linenum=0;

		slist_numoptions=0;
		return;
	}
	else if (key == K_RIGHTARROW)
	{
		slist_type++;
		if (slist_type>NUMSLISTHEADERS-1)
			slist_type=NUMSLISTHEADERS-1;

		selectedserver.linenum++;

		slist_numoptions = 0;
		return;
	}
	else if (key == 'q')
		selectedserver.linenum--;
	else if (key == 'a')
		selectedserver.linenum++;

	if (!slist_numoptions)
		return;

	if (slist_type == SLISTTYPE_OPTIONS)
	{
		M_SListOptions_Key(key);
		return;
	}
	
	if (key == K_UPARROW)
	{
		slist_option--;
		if (slist_option<0)
			slist_option=0;

		if (slist_type == SLISTTYPE_SERVERS)
			SListOptionChanged(M_FindCurrentServer());	//go for these early.
	}
	else if (key == K_DOWNARROW)
	{
		slist_option++;
		if (slist_option >= slist_numoptions)
			slist_option = slist_numoptions-1;

		if (slist_type == SLISTTYPE_SERVERS)
			SListOptionChanged(M_FindCurrentServer());	//go for these early.
	}
	else if (key == K_PGDN)
	{
		slist_option+=10;
		if (slist_option >= slist_numoptions)
			slist_option = slist_numoptions-1;

		if (slist_type == SLISTTYPE_SERVERS)
			SListOptionChanged(M_FindCurrentServer());	//go for these early.
	}
	else if (key == K_PGUP)
	{
		slist_option-=10;
		if (slist_option<0)
			slist_option=0;

		if (slist_type == SLISTTYPE_SERVERS)
			SListOptionChanged(M_FindCurrentServer());	//go for these early.
	}
	else if (key == 'r')
		MasterInfo_Begin();
	else if (key == K_SPACE)
	{
		if (slist_type == SLISTTYPE_SERVERS)
		{
			selectedserver.inuse = !selectedserver.inuse;
			if (selectedserver.inuse)
				SListOptionChanged(M_FindCurrentServer());
		}
	}
	else if (key == 'f')
	{
		serverinfo_t *server;
		if (slist_type == SLISTTYPE_SERVERS)	//add to favorites
		{
			server = M_FindCurrentServer();
			if (server)
			{
				server->special |= SS_FAVORITE;
				MasterInfo_WriteServers();
			}
		}
		if (slist_type == SLISTTYPE_FAVORITES)	//remove from favorites
		{
			server = M_FindCurrentServer();
			if (server)
			{
				server->special &= ~SS_FAVORITE;
				MasterInfo_WriteServers();
			}
		}
	}
	else if (key==K_ENTER)
	{
		serverinfo_t *server;
		if (slist_type == SLISTTYPE_SERVERS || slist_type == SLISTTYPE_FAVORITES)
		{
			if (!selectedserver.inuse)
			{
				selectedserver.inuse = true;
				SListOptionChanged(M_FindCurrentServer());
				return;
			}
			server = M_FindCurrentServer();
			if (!server)
				return;	//ah. off the end.

			if (server->special & SS_NETQUAKE)
				Cmd_AddText(va("nqconnect %s\n", NET_AdrToString(&server->adr)), false);
			else
				Cmd_AddText(va("connect %s\n", NET_AdrToString(&server->adr)), false);
		}
		else if (slist_type == SLISTTYPE_SOURCES)
		{
			MasterInfo_Request(M_FindCurrentMaster());
		}

		return;
	}	
}

