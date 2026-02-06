/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cmd.c -- Quake script command processing module

#include "quakedef.h"
#include "fs.h"
#include "cl_master.h"

cvar_t ruleset_allow_in		= CVAR("ruleset_allow_in", "1");
cvar_t rcon_level			= CVAR("rcon_level", "20");
cvar_t cmd_maxbuffersize	= CVAR("cmd_maxbuffersize", "65536");
#ifdef HAVE_LEGACY
cvar_t dpcompat_set         = CVAR("dpcompat_set", "0");
cvar_t dpcompat_console     = CVARD("dpcompat_console", "0", "Enables hacks to emulate DP's console.");
#else
static const cvar_t dpcompat_set = {0};
static const cvar_t dpcompat_console = {0};
#endif
int	Cmd_ExecLevel;
qboolean cmd_didwait;
qboolean cmd_blockwait;
static qboolean cmd_stuffedcmdline;

void Cmd_ForwardToServer (void);

#define	MAX_ALIAS_NAME	32

typedef struct cmdalias_s
{
	struct cmdalias_s	*next;
	char	*value;
	int flags;
	qbyte execlevel;
	qbyte restriction;
	char	name[1];
} cmdalias_t;

#define ALIAS_FROMSERVER	1

cmdalias_t	*cmd_alias;

cvar_t	cfg_save_all = CVARFD("cfg_save_all", "", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "If 1, cfg_save ALWAYS saves all cvars. If 0, cfg_save only ever saves archived cvars. If empty, cfg_save saves all cvars only when an explicit filename was given (ie: when not used internally via quit menu options).");
cvar_t	cfg_save_auto = CVARFD("cfg_save_auto", "0", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "If 1, the config will automatically be saved and without prompts. If 0, you'll have to save your config manually (possibly via prompts from the quit menu).");
cvar_t	cfg_save_infos = CVARFD("cfg_save_infos", "1", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "If 1, saves userinfo and serverinfo to configs.");
cvar_t	cfg_save_aliases = CVARFD("cfg_save_aliases", "1", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "If 1, saves aliases to configs. Note that aliases sent from servers are assumed to be server-specific and are never saved (and are forgotten on map changes, too).");
cvar_t	cfg_save_binds = CVARFD("cfg_save_binds", "1", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "If 1, saves all key bindings to configs.");
cvar_t	cfg_save_buttons = CVARFD("cfg_save_buttons", "0", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "If 1, saves the state of things such as +mlook or +forward to configs.");

cvar_t cl_warncmd			= CVARF("cl_warncmd", "1", CVAR_NOSAVE|CVAR_NORESET);
cvar_t cl_aliasoverlap		= CVARFD("cl_aliasoverlap", "1", CVAR_NOTFROMSERVER, "Rename new aliases if they would override cvar names.");

#ifdef HAVE_CLIENT
cvar_t tp_disputablemacros	= CVARF("tp_disputablemacros", "1", CVAR_SEMICHEAT);
#endif


//=============================================================================










#define MAX_MACROS 70

typedef struct {
	char name[32];
	char *(*func) (void);
	int disputableintentions;
} macro_command_t;

static macro_command_t macro_commands[MAX_MACROS];
static int macro_count = 0;

void Cmd_AddMacro(char *s, char *(*f)(void), int disputableintentions)
{
	int i;
	for (i = 0; i < macro_count; i++)
	{
		if (!strcmp(macro_commands[i].name, s))
			break;
	}

	if (i == MAX_MACROS)
		Sys_Error("Cmd_AddMacro: macro_count == MAX_MACROS");

	Q_strncpyz(macro_commands[i].name, s, sizeof(macro_commands[macro_count].name));
	macro_commands[i].func = f;
	macro_commands[i].disputableintentions = disputableintentions;

	if (i == macro_count)
		macro_count++;
}

static char *TP_MacroString (char *s, int *newaccesslevel, int *len)
{
	int i;
	macro_command_t	*macro;

	for (i = 0; i < macro_count; i++)
	{
		macro = &macro_commands[i];
		if (!Q_strcasecmp(s, macro->name))
		{
#ifdef HAVE_CLIENT
			if (macro->disputableintentions)
				if (!tp_disputablemacros.ival)
					*newaccesslevel = 0;
#endif
			if (len)
				*len = strlen(macro->name);
			return macro->func();
		}
	}
	return NULL;
}

static void Cmd_MacroList_f (void)
{
	int	i;

	if (!macro_count)
	{
		Con_Printf("No macros!");
		return;
	}

	for (i = 0; i < macro_count; i++)
		Con_Printf ("$%s\n", macro_commands[i].name);
}


static void Cmd_MacroCompletion_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	size_t i, len;
	const char *end = partial;
	if (*end++ != '$')
		return;
	if (*end == '{')
		end++;
	len = strlen(end);

	for (i = 0; i < macro_count; i++)
	{
		if (len <= strlen(macro_commands[i].name))
			if (!strncmp(end, macro_commands[i].name, len))
				ctx->cb(va("${%s}", macro_commands[i].name), NULL, NULL, ctx);
	}
}





/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

struct {
	sizebuf_t	buf;
	int noclear;
	double waitattime;
} cmd_text[RESTRICT_MAX+3+MAX_SPLITS];	//max is local.
							//RESTRICT_MAX+1 is the from sever buffer (max+2 is for second player...)

void Cbuf_Waited(void)
{
	//input packet was sent to server, its okay to continue executing stuff like -attack now
	cmd_text[RESTRICT_LOCAL].waitattime = 0;
}

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
static void Cmd_Wait_f (void)
{
	if (cmd_blockwait)
		return;

#ifdef HAVE_SERVER
	if (cmd_didwait && sv.state)
		Con_DPrintf("waits without server frames\n");
#endif
	cmd_didwait = true;
	cmd_text[Cmd_ExecLevel].waitattime = realtime;
}

/*
lame timers. :s
*/
typedef struct cmdtimer_s {
	struct cmdtimer_s *next;
	float timer;
	int iarg;
	void(*callback)(int iarg, void *data);
	char data[1];
} cmdtimer_t;
static cmdtimer_t *cmdtimers;
static void Cmd_ExecuteTimers(void)
{
	cmdtimer_t **link, *t;
	//FIXME: we should probably insert these in order instead, then early out.
	//really, it depends on just how many we end up with
	for(link = &cmdtimers; (t = *link); )
	{
		if (t->timer < realtime)
		{
			*link = t->next;
			t->callback(t->iarg, t->data);
			Z_Free(t);
		}
		else
			link = &t->next;
	}
}
void Cmd_AddTimer(float delay, void(*callback)(int iarg, void *data), int iarg, void *data, size_t datasize)
{
	cmdtimer_t *n = Z_Malloc(sizeof(*n) + datasize);
	n->iarg = iarg;
	n->callback = callback;
	memcpy(n->data, data, datasize);
	n->data[datasize] = 0;	//just in case.

	n->timer = realtime + delay;

	FTE_Atomic_Insert(cmdtimers, n, n->next);
}
static void Cmd_In_Callback(int iarg, void *data)
{
	Cbuf_AddText((char*)data, iarg);
	Cbuf_AddText("\n", iarg);
}
static void Cmd_In_f(void)
{
	float delay = atof(Cmd_Argv(1));
	char *cmd;
	if (Cmd_Argc() < 3)
	{
		Con_Printf("%s <seconds to wait> <command to execute>\n", Cmd_Argv(0));
		return;
	}
	Cmd_ShiftArgs(1, false);
	cmd = Cmd_Args();

	if (ruleset_allow_in.ival || !delay)
		Cmd_AddTimer(delay, Cmd_In_Callback, Cmd_ExecLevel, cmd, strlen(cmd));
}

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	int level;
	for (level = 0; level <= RESTRICT_MAX+1; level++)
		cmd_text[level].waitattime = -1;
}

static void Cbuf_WorkerAddText(void *ctx, void *data, size_t a, size_t b)
{
	Cbuf_AddText(data, a);
	Z_Free(data);
}
/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (const char *text, int level)
{
	int		l;

	if (!Sys_IsMainThread())
	{
		COM_AddWork(WG_MAIN, Cbuf_WorkerAddText, NULL, Z_StrDup(text), level, 0);
		return;
	}

	if (level > sizeof(cmd_text)/sizeof(cmd_text[0]) || level < 0)
	{
		Con_Printf("Bad execution level\n");
		return;	//reject.
	}

	l = Q_strlen (text);

	if (!cmd_text[level].buf.maxsize)
	{
		cmd_text[level].buf.data = (qbyte*)BZ_Malloc(8192);
		cmd_text[level].buf.maxsize = 8192;
	}
	if (cmd_text[level].buf.cursize + l >= cmd_text[level].buf.maxsize)
	{
		int newmax;

		newmax = cmd_text[level].buf.maxsize*2;

		if (newmax > cmd_maxbuffersize.ival && cmd_maxbuffersize.ival)
		{
			Con_TPrintf ("%s: overflow\n", "Cbuf_AddText");
			return;
		}
		while (newmax < cmd_text[level].buf.cursize + l)
			newmax*=2;
		cmd_text[level].buf.data = (qbyte*)BZ_Realloc(cmd_text[level].buf.data, newmax);
		cmd_text[level].buf.maxsize = newmax;
	}
	SZ_Write (&cmd_text[level].buf, text, Q_strlen (text));
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (const char *text, int level, qboolean addnl)
{
	char	*temp;
	int		templen;

	if (level > sizeof(cmd_text)/sizeof(cmd_text[0]) || level < 0)
	{
		Con_Printf("Bad execution level\n");
		return;	//reject.
	}

// copy off any commands still remaining in the exec buffer
	templen = cmd_text[level].buf.cursize;
	if (templen)
	{
		temp = (char*)Z_Malloc (templen+1);
		Q_memcpy (temp, cmd_text[level].buf.data, templen);
		SZ_Clear (&cmd_text[level].buf);
	}
	else
		temp = NULL;	// shut up compiler

// add the entire text of the file
	Cbuf_AddText (text, level);
	if (addnl)
		Cbuf_AddText ("\n", level);
// add the copied off data
	if (templen)
	{
		temp[templen] = '\0';
		Cbuf_AddText(temp, level);
//		SZ_Write (&cmd_text[level].buf, temp, templen);
		Z_Free (temp);
	}
}

char *Cbuf_GetNext(int level, qboolean ignoresemicolon)
{
	int		i;
	char	*text;
	int		quotes;
	static char	line[1024];

start:

	text = (char *)cmd_text[level].buf.data;

	quotes = 0;
	for (i=0 ; i< cmd_text[level].buf.cursize ; i++)
	{
		if (text[i] == '"')
			quotes++;
		if ( !(quotes&1) &&  text[i] == ';' && !ignoresemicolon)
			break;	// don't break if inside a quoted string
		if (text[i] == '\n')
			break;
	}

	if (i >= sizeof(line)-1)
	{
		Con_Printf("Statement too long\n");
		return "";
	}


	memcpy (line, text, i);
	line[i] = 0;

// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

	if (i == cmd_text[level].buf.cursize)
		cmd_text[level].buf.cursize = 0;
	else
	{
		i++;
		cmd_text[level].buf.cursize -= i;
		memmove (text, text+i, cmd_text[level].buf.cursize);
	}

//	Con_Printf("Found \"%s\"\n", line);
	text=line;
	while(*text == ' ' || *text == '\t')
		text++;

	if (!*text)
		if (cmd_text[level].buf.cursize)
			goto start;	//should be a while.

	return text;
}

static char *Cbuf_StripText(int level)	//remove all text in the command buffer and return it (so it can be readded later)
{
	char *buf;
	buf = (char*)Z_Malloc(cmd_text[level].buf.cursize+1);
	Q_memcpy (buf, cmd_text[level].buf.data, cmd_text[level].buf.cursize);
	cmd_text[level].buf.cursize = 0;
	return buf;
}

void Cbuf_ExecuteLevel (int level)
{
	int		i;
	char	*text;
	char	linebuf[65536], *line;
	qboolean	comment;
	int		quotes;

	while (cmd_text[level].buf.cursize)
	{
		if (cmd_text[level].waitattime == realtime)
		{	// skip out while text still remains in buffer, leaving it
			// for next frame
			break;
		}

// find a \n or ; line break
		text = (char *)cmd_text[level].buf.data;

		quotes = false;
		comment = false;
		for (i=0 ; i< cmd_text[level].buf.cursize ; i++)
		{
			if (text[i] == '\n')
				break;

			if (quotes)
			{
				if (text[i] == '"')
				{
					quotes=false;
				}
				if (text[i] == '\\' && quotes==2)
				{
					//skip over both chars if its something embedded.
					if (text[i+1] == '\"' || text[i+1] == '\\')
					{
						i++;
						continue;
					}
				}
				continue;
			}
			else if (text[i] == '"')
			{	//simple quoted string
				quotes = true;
				continue;
			}
			else if (text[i] == '\\' && text[i+1] == '\"')
			{	//escaped quoted string.
				quotes = 2;
				i++;
				continue;
			}

			if (comment)
				continue;

			if (text[i] == '/' && i+1 < cmd_text[level].buf.cursize && text[i+1] == '/')
				comment = true;
			else if (text[i] == ';')
				break;	// don't break if inside a quoted string
		}

		if (i >= sizeof(linebuf))
			line = malloc(i+1);	//might leak if the command longjmps. :(
		else
			line = linebuf;
		memcpy (line, text, i);
		line[i] = 0;

// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

		if (i == cmd_text[level].buf.cursize)
			cmd_text[level].buf.cursize = 0;
		else
		{
			i++;
			cmd_text[level].buf.cursize -= i;
			memmove (text, text+i, cmd_text[level].buf.cursize);
		}

// execute the command line
		Cmd_ExecuteString (line, level);
		if (line != linebuf)
			free(line);
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int level;

#ifdef HAVE_CLIENT
	if (cmd_text[RESTRICT_LOCAL].waitattime && cls.state == ca_active)
	{
		//keep binds blocked until after the next input frame was sent to the server (at which point it will be cleared
		//this ensures that wait and +attack etc works synchronously, as though your client never even supported network independance! yay... I guess.
		cmd_text[RESTRICT_LOCAL].waitattime = realtime;
	}
#endif
	Cmd_ExecuteTimers();

	for (level = 0; level < sizeof(cmd_text)/sizeof(cmd_text[0]); level++)
		if (cmd_text[level].buf.cursize)
			Cbuf_ExecuteLevel(level);
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
void Cmd_StuffCmds (void)
{
	int		i;
	int		s;
	char	*text;
	const char	*arg;
	int		pluscmd = false;

	if (cmd_stuffedcmdline)
		return;	//don't do it multiple times

// build the combined string to parse from
	s = 2;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		s += Q_strlen (com_argv[i])*2 + 4;
	}
	if (!s)
		return;

	text = (char*)Z_Malloc (s+2);
	text[0] = 0;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		arg = com_argv[i];
		if ((*arg == '+' || *arg == '-') && !(arg[1] >= '0' && arg[1] <= '9'))	//we only really want the '+foo arg' commands, split by +/- prefixes. if its a -1 or +1 then that's a numerical parameter, not a separate command!
		{
			if (*text)
			{
				Q_strcat (text, "\n");
				Cbuf_AddText (text, RESTRICT_LOCAL);
				*text = 0;
			}
			pluscmd = *arg++ == '+';
		}

		if (pluscmd)
		{
			if (*text)
				Q_strcat (text, " ");
			if (strchr(arg, ' ') || strchr(arg, '\t') || strchr(arg, '@') || strchr(arg, '/') || strchr(arg, '\\'))
				COM_QuotedString(arg, text+strlen(text),s-strlen(text), false);
			else
				Q_strcat (text,arg);
		}
	}

	if (*text)
	{
		Q_strcat (text, "\n");
		Cbuf_AddText (text, RESTRICT_LOCAL);
	}

	Z_Free (text);

	cmd_stuffedcmdline = true;	//don't add multiple times.

	//and exec anything added.
	Cbuf_Execute ();
}

#if defined(HAVE_LEGACY) && defined(HAVE_CLIENT)
static const char *replacementq1binds =
	"unbindall\n"

	"bind		`			toggleconsole\n"
	"bind		w			+forward\n"
	"bind		s			+back\n"
	"bind		a			+moveleft\n"
	"bind		d			+moveright\n"
	"bind		MOUSE1		+attack\n"
	"bind		MOUSE2		+jump\n"
	"bind		SPACE		+jump\n"
	"bind		v			+voip\n"
	"bind		t			messagemode2\n"
	"bind		y			messagemode\n"
	"bind		TAB			+showscores\n"

	"bind		e			+moveup\n"
	"bind		c			+movedown\n"

	"bind		MWHEELUP	impulse 12\n"
	"bind		MWHEELDOWN	impulse 10\n"

	"bind		UPARROW		+forward\n"
	"bind		DOWNARROW	+back\n"
	"bind		LEFTARROW	+left\n"
	"bind		RIGHTARROW	+right\n"

	"bind		LCTRL		+attack\n"
	"bind		RCTRL		+attack\n"
	"bind		LALT		+strafe\n"
	"bind		RALT		+strafe\n"
	"bind		LSHIFT		+speed\n"
	"bind		RSHIFT		+speed\n"

	"bind		=			sizeup\n"
	"bind		-			sizedown\n"

	"bind		1			impulse 1\n"
	"bind		2			impulse 2\n"
	"bind		3			impulse 3\n"
	"bind		4			impulse 4\n"
	"bind		5			impulse 5\n"
	"bind		6			impulse 6\n"
	"bind		7			impulse 7\n"
	"bind		8			impulse 8\n"
//	"bind		9			impulse 9\n"
//	"bind		0			impulse 10\n"

//	"bind		F1	help\n"
	"bind		F2	menu_save\n"
	"bind		F3	menu_load\n"
	"bind		F4	menu_options\n"
	"bind		F5	menu_multiplayer\n"
	"bind		F6	save quick\n"
//	"bind		F7
//	"bind		F8
	"bind		F9	load quick\n"
	"bind		F10	menu_quit\n"
//	"bind		F11	+zoom\n"
	"bind		F12	screenshot\n"

	"bind		volup		\"if $volume < 0.9 then inc volume 0.1 else if $volume < 1.0 then set volume 1\"\n"
	"bind		voldown		\"inc volume -0.1; if $volume < 0 then set volume 0\"\n"
	;
static const char *defaulttouchcfg =
	"showpic_removeall\n"
//	"sv_aim 0.90\n" //quake style, avoid needing to pitch too much
	"showpic touch_moveforward.tga	fwd		-128	-112 bm	32	32	+forward	5\n"
	"showpic touch_moveback.tga		back	-128	-80	bm	32	32	+back		5\n"
	"showpic touch_moveleft.tga		left	-160	-88	bm	32	32	+moveleft	5\n"
	"showpic touch_moveright.tga	rght	-96		-88	bm	32	32	+moveright	5\n"

	"showpic touch_attack.tga		fire	-160	-160 bm	32	32	+attack		5\n"
	"showpic touch_jump.tga			jump	128		-80	bm	32	32	+jump		5\n"

	"showpic touch_weapons.tga		weap	80		-80	bm	32	32	+weaponwheel	5\n"
	"showpic touch_menu.tga			menu	-32		0	tr	32	32	togglemenu 10\n"
	;
#endif

/*
===============
Cmd_Exec_f
===============
*/
static void Cmd_Exec_f (void)
{
	char	*f, *s;
	char	name[256];
	char	buf[512];
	flocation_t loc;
	qboolean untrusted;
	vfsfile_t *file;
	size_t l;
	unsigned int level;

	if (Cmd_Argc () != 2)
	{
		Con_TPrintf ("exec <filename> : execute a script file\n");
		return;
	}


	if (!strcmp(Cmd_Argv(0), "cfg_load"))
	{
		f = Cmd_Argv(1);
		if (!*f)
			f = fs_manifest->mainconfig;
		if (!*f)
			f = "config";
		Q_snprintfz(name, sizeof(name)-5, "configs/%s", f);
		COM_DefaultExtension(name, ".cfg", sizeof(name));
	}
	else
	{
		Q_strncpyz(name, Cmd_Argv(1), sizeof(name));
		if (*fs_manifest->mainconfig && Q_strcasecmp("config.cfg", fs_manifest->mainconfig) && Q_strcasecmp("q3config.cfg", fs_manifest->mainconfig))
		{
			//fte writes to a different config file from that specified by the quake.rc, to avoid conflicts.
			//so make sure that fte's settings override those from whatever other engine that wrote the legacy config.cfg file.
			if (!strcmp(name, "config.cfg") || !strcmp(name, "q3config.cfg"))
			{
				int cfgdepth = COM_FDepthFile(name, true);
				int defdepth = COM_FDepthFile("default.cfg", true);
				Cbuf_InsertText(va("exec %s", fs_manifest->mainconfig), Cmd_ExecLevel, true);
				if (defdepth < cfgdepth && cfgdepth != FDEPTH_MISSING)
				{
					if (cl_warncmd.ival)
					{
						char fulldefault[MAX_OSPATH];
						char fullconfig[MAX_OSPATH];
						*fulldefault = *fullconfig = 0;
						FS_DisplayPath("default.cfg", FS_GAME, fulldefault, sizeof(fulldefault));
						FS_DisplayPath(name, FS_GAME, fullconfig, sizeof(fullconfig));
						Con_Printf("Refusing to execute \"%s\", superceded by %s\n", fullconfig, fulldefault);
					}
					return;
				}
			}
			else if (!strcmp(name, fs_manifest->mainconfig))
			{
				int cfgdepth = COM_FDepthFile(name, true);
				int defdepth = COM_FDepthFile("default.cfg", true);
				if (defdepth < cfgdepth && cfgdepth != FDEPTH_MISSING)
				{
					if (cl_warncmd.ival)
					{
						char fulldefault[MAX_OSPATH];
						char fullconfig[MAX_OSPATH];
						*fulldefault = *fullconfig = 0;
						FS_DisplayPath("default.cfg", FS_GAME, fulldefault, sizeof(fulldefault));
						FS_DisplayPath(name, FS_GAME, fullconfig, sizeof(fullconfig));
						Con_Printf("Refusing to execute \"%s\", superceded by %s\n", fullconfig, fulldefault);
					}
					return;
				}
			}
		}
	}

	if (!strncmp(name, "../", 3) || !strncmp(name, "..\\", 3) || !strncmp(name, "./", 2) || !strncmp(name, ".\\", 2))
	{	//filesystem will correctly block this (and more), but it does look dodgy when servers try doing this dodgy shit anyway.
		if (Cmd_IsInsecure())
			Con_TPrintf ("%s: %s is an invalid path (from server)\n", Cmd_Argv(0), name);
		else
			Con_TPrintf ("%s: %s is an invalid path\n", Cmd_Argv(0), name);
		return;
	}

	if (FS_FLocateFile(name, FSLF_IFFOUND|FSLF_IGNOREPURE, &loc) || FS_FLocateFile(va("%s.cfg", name), FSLF_IFFOUND, &loc))
	{
		file = FS_OpenReadLocation(name, &loc);
		if (!file)
		{
			Con_TPrintf ("couldn't %s %s. check permissions.\n", Cmd_Argv(0), name);
			return;
		}

		l = VFS_GETLEN(file);
		f = BZ_Malloc(l+1);
		f[l] = 0;
		VFS_READ(file, f, l);
		VFS_CLOSE(file);

		untrusted = !!(loc.search->flags&SPF_UNTRUSTED);
	}
#if defined(HAVE_LEGACY) && defined(HAVE_CLIENT)
	else if (!strcmp(name, "default.cfg"))	//the q1 rerelease lacks a default.cfg (which I suppose is kinda handy, but oh well)
	{
		f = Z_StrDup(replacementq1binds);
		untrusted = false;
		l = 0;
	}
	else if (!strcmp(name, "touch.cfg"))	//auto-execed if they touch a touchscreen.
	{
		f = Z_StrDup(defaulttouchcfg);
		untrusted = false;
		l = 0;
	}
#endif
	else
	{
		Con_TPrintf ("couldn't exec %s\n", name);
		return;
	}
	level = ((Cmd_FromGamecode() || untrusted) ? RESTRICT_INSECURE : Cmd_ExecLevel);

	if (cl_warncmd.ival || developer.ival || cvar_watched || dpcompat_console.ival)
	{
		if (loc.search)
			Con_TPrintf ("execing ^[^7%s\\tip\\from %s/%s^]\n", name, loc.search->logicalpath, name);
		else
			Con_TPrintf ("execing %s\n", name);
	}

	s = f;
	if (s[0] == '\xef' && s[1] == '\xbb' && s[2] == '\xbf')
	{
		Con_DPrintf("Ignoring UTF-8 BOM\n");
		s+=3;
	}

	if (!strcmp(name, "config.cfg") || !strcmp(name, "q3config.cfg") || (*fs_manifest->mainconfig && !strcmp(name, fs_manifest->mainconfig)))
	{
		char *restart;
		//if the config is from id1 and the default.cfg was from some mod, make sure the default.cfg overrides the config.
		//we won't just exec the default instead, because we can at least retain things which are not specified (ie: a few binds)
		int cfgdepth = COM_FDepthFile(name, true);
		int defdepth = COM_FDepthFile("default.cfg", true);
		if (defdepth < cfgdepth)
			Cbuf_InsertText("exec default.cfg\n", level, false);

		//hack to work around the more insideous hacks of other engines.
		//namely: vid_restart at the end of config.cfg is evil, and NOT desired in FTE as it generally means any saved video settings are wrong.
		restart = strstr(f, "\nvid_restart");
		if (restart && (restart[12] == '\r' || restart[12]=='\n'))
		{	//convert it to a comment so we don't get fucked over by bad configs.
			restart[1] = restart[2] = '/';
			Con_Printf(CON_WARNING "WARNING: %s came from a different engine\n", loc.rawname);
		}
	}

	if (*loc.rawname)
		COM_QuotedString(loc.rawname, buf, sizeof(buf), false);
	else if (loc.search)
		COM_QuotedString(va("%s/%s", loc.search->logicalpath, name), buf, sizeof(buf), false);
	else
		COM_QuotedString(name, buf, sizeof(buf), false);

	if (cvar_watched)
		Cbuf_InsertText (va("echo END %s", buf), level, true);
	// don't execute anything if it was from server (either the stuffcmd/localcmd, or the file)
	if (!strcmp(name, "default.cfg"))
	{
		if (!(Cmd_FromGamecode() || untrusted))
			Cbuf_InsertText ("\ncvar_lockdefaults 1\n", level, false);
		if (fs_manifest->defaultoverrides)
			Cbuf_InsertText (fs_manifest->defaultoverrides, level, false);

#if defined(HAVE_LEGACY) && defined(HAVE_CLIENT)
		if (l == 1914 && CalcHashInt(&hash_md4, f, l) == 0x2d7b72b9)
			s = (char*)replacementq1binds;
#ifdef HEXEN2
		else if (l == 1875 && CalcHashInt(&hash_md4, f, l) == 0x27b4d813)
		{	//hexen2 has weird stuff in there. just give it wasd.
			s = va(
				"%s\n"
				"bind w +forward\n"
				"bind a +moveleft\n"
				"bind s +back\n"
				"bind d +moveright\n"

				"bind mouse2 +jump\n"
				"bind mouse3 +forward\n" //mneh

				"bind x +lookup\n"	//moved to x instead of a
				"cl_forwardspeed 400\n" //hexen2's autorun state.
			, s);
		}
#endif
#endif
	}
#ifndef QUAKETC
	//hack to try to work around nquake's b0rkedness
	if (!strncmp(s, "// This is nQuake's Frogbot config", 33))
	{
		s = va("echo \""CON_ERROR"Refusing to exec nQuake's %s\"", buf);	//otherwise many people with nquake installed will be fucked over whenever they try playing singleplayer
		Cbuf_InsertText (s, level, true);
	}
	else
	{
		int foundone = 0;
		while (!strncmp(s, "//", 2))
		{
			char *eol = strstr(s, "\n");
			if (eol)
			{
				*eol++ = 0;
				if (strstr(s, "nQuake") || strstr(s, "N Q U A K E"))
				{	//this is evil, but if we're running quake then com_parseutf8 will be 0 and we can just convert to quake chars (less text).
					char *out = s = eol;
					const char *in = s;
					while (*in)
					{
						if (*in == '\n' && !strncmp(in,"\nexec configs/config.cfg", 24))
						{	//ezquake writes its configs elsewhere, and nquake stomps on everything in its autoexec.cfg, so we need to try to work around its breakages
							memmove(out, in, 6);out+=6;in+=6;
							in += 8;
							foundone++;
							continue;
						}
						if (*in == '^')
						{
							*out++ = 0x80|*++in;
							foundone++;
						}
						else
							*out++ = *in;
						in++;
					}
					*out = 0;
					break;
				}
				s = eol;
				continue;
			}
			break;
		}

		#ifdef HAVE_CLIENT
		if (!cl_warncmd.ival && foundone && (!strcmp(name, "quake.rc") || !strcmp(name, "default.cfg") || !strcmp(name, "autoexec.cfg")))
		{
#if defined(HAVE_LEGACY)
			if (!strcmp(name, "default.cfg"))
			{
				s = (char*)replacementq1binds;
				foundone = 0;
			}
			else
#endif
			{
				Menu_Prompt(NULL, NULL, va(localtext("WARNING: nquake %s file detected. The file has been ignored."), name), NULL, NULL, "Argh", false);
				*s = 0;
				foundone = 0;
			}
		}
		#endif

		Cbuf_InsertText (s, level, true);
		if (foundone)
			Cbuf_InsertText(va("\necho \""CON_ERROR"fixups for nquake config %s: %i replacements\"\n", buf, foundone), level, false);
	}
#else
	Cbuf_InsertText (s, level, true);
#endif
	if (cvar_watched)
		Cbuf_InsertText (va("echo BEGIN %s", buf), level, true);
	BZ_Free(f);

	if (level != Cmd_ExecLevel)
		Cbuf_ExecuteLevel(level);
}

static int QDECL CompleteExecList (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	ctx->cb(name, NULL, NULL, ctx);
	return true;
}
static void Cmd_Exec_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	if (argn == 1)
	{
		COM_EnumerateFiles(va("configs/%s*.cfg", partial), CompleteExecList, ctx);
		COM_EnumerateFiles(va("%s*.cfg", partial), CompleteExecList, ctx);
		COM_EnumerateFiles(va("%s*.rc", partial), CompleteExecList, ctx);
	}
}

/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
static void Cmd_Echo_f (void)
{
	char text[4096];
	char extext[4096];
	const char *t;
	int level = Cmd_ExecLevel;
	int		i;
	*text = 0;

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		if (i >= 2)
			Q_strncatz(text, " ", sizeof(text));
		Q_strncatz(text, Cmd_Argv(i), sizeof(text));
	}
	Q_strncatz(text, "\n", sizeof(text));

	//echo text is often quoted, so expand the text again now that we're no longer in quotes.
	t = Cmd_ExpandString(text, extext, sizeof(extext), &level, false, !Cmd_IsInsecure()?true:false, true);

#ifndef HAVE_CLIENT
	Con_Printf ("%s", t);
#else
	t = TP_ParseFunChars(t);
#ifdef HAVE_LEGACY
	Con_PrintFlags (t, ((ezcompat_markup.ival>=2)?PFS_EZQUAKEMARKUP:0), 0);
#else
	Con_PrintFlags (t, 0, 0);
#endif
#endif
}

static void Key_Alias_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	cmdalias_t	*a;
	size_t len = strlen(partial);
	if (argn != 1)
		return;
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!Q_strncasecmp(partial,a->name, len))
			ctx->cb(a->name, a->value, NULL, ctx);
	}
}
static void Cmd_ShowAlias_f (void)
{
	cmdalias_t	*a;
	char *s;

	s = Cmd_Argv(1);

	//find it, print it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name))
		{
			Con_Printf ("alias %s %s\n", a->name, a->value);
			return;
		}
	}

	Con_Printf("Alias doesn't exist\n");
}

//returns a zoned string.
char *Cmd_ParseMultiline(qboolean checkheader)
{
	char *result;
	char *end;
	int in = checkheader?0:1;
	char *s;
	result = NULL;
	for(;;)
	{
		s = Cbuf_GetNext(Cmd_ExecLevel, false);
		if (!*s)
		{
			if (in)
				Con_Printf(CON_WARNING "WARNING: Multiline alias was not terminated\n");
			break;
		}
		while (*s <= ' ' && *s)
			s++;
		for (end = s + strlen(s)-1; end >= s && *end <= ' '; end--)
			*end = '\0';
		if (!strcmp(s, "{"))
		{
			in++;
			if (in == 1)
				continue;	//don't embed the first one in the string, because that would be weird.
		}
		else if (!strcmp(s, "}"))
		{
			in--;
			if (!in)
				break;	//phew
		}
		if (result)
		{
			char *newv = (char*)Z_Malloc(strlen(result) + strlen(s) + 2);
			sprintf(newv, "%s;%s", result, s);
			Z_Free(result);
			result = newv;
		}
		else
			result = Z_StrDup(s);
		if (!in)
			break;
	}
	return result;
}
/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/

static void Cmd_Alias_f (void)
{
	cmdalias_t	*a, *b;
	char		cmd[65536];
	int			i, c;
	char		*s;
	qboolean multiline;

	if (Cmd_Argc() == 1)	//list em all.
	{
		if (Cmd_FromGamecode())
		{
			if (Cmd_ExecLevel==RESTRICT_SERVER)
			{
				Con_TPrintf ("Current alias commands:\n");
				for (a = cmd_alias ; a ; a=a->next)
				{
					if (a->flags & ALIAS_FROMSERVER)
						Con_Printf ("%s : %s\n", a->name, a->value);
				}
			}
		}
		else
		{
			Con_TPrintf ("Current alias commands:\n");
			for (a = cmd_alias ; a ; a=a->next)
			{
	/*			extern int con_linewidth;
				if (strlen(a->value)+strlen(a->name)+3 > con_linewidth)
					Con_Printf ("%s ...\n", a->name);
				else*/
					Con_Printf ("%s : %s\n", a->name, a->value);
			}
		}
		return;
	}

	s = Cmd_Argv(1);
	if (!strcmp(s, "say"))	//reject aliasing the say command. We use it as an easy way to warn that our player is cheating.
	{
		Con_TPrintf ("Refusing to create an alias with the name '%s'\n", s);
		return;
	}

	// check for overlap with a command
	if (Cmd_Exists (s))
	{	//commands always take precedence over aliases (so mods can't clobber 'quit' etc), so creating an alias with one of these names is stupid. always try to rename them.
		if (Cmd_IsInsecure() && Q_snprintfz(cmd, sizeof(cmd), "%s_a", s) < sizeof(cmd))
		{
			if (Cmd_Exists (cmd))
			{
				Con_Printf (S_COLOR_RED"Can't register alias, %s is a command\n", s);
				return;
			}
			Con_Printf (S_COLOR_RED"alias %s: renamed to %s due to command conflict\n", s, cmd);
			s = cmd;
		}
		else
		{
			Con_Printf (S_COLOR_RED"Can't register alias, %s is a command\n", s);
			return;
		}
	}
	if (!cl_aliasoverlap.value)
	{	//aliases take precedence over cvars (while cvars can be set via 'set'), so user's choice.
		if (Cvar_FindVar (s))
		{
			if (Cmd_IsInsecure() && Q_snprintfz(cmd, sizeof(cmd), "%s_a", s) < sizeof(cmd))
			{
				Con_Printf (S_COLOR_RED"alias %s: renamed to %s due to cvar conflict\n", s, cmd);
				s = cmd;
			}
			else
			{
				Con_Printf (S_COLOR_RED"alias %s: can't register alias - matches existing cvar\n", s);
				return;
			}
		}
	}

	// if the alias already exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name))
		{
			if ((a->restriction?a->restriction:rcon_level.ival) > Cmd_ExecLevel)
			{
				Con_TPrintf ("Alias is already bound with a higher restriction\n");
				return;
			}

			if (!stricmp(Cmd_Argv(0), "newalias"))
				return;	//newalias command only registers the alias if it is new, and does not change it if it already exists

			Z_Free (a->value);
			break;
		}
	}

	if (!a)
	{
		cmdalias_t **link;

		a = (cmdalias_t*)Z_Malloc (sizeof(cmdalias_t) + strlen(s));
		strcpy (a->name, s);
		for (link = &cmd_alias; ; link = &(*link)->next)
		{
			if (!*link || strcmp((*link)->name, s) >= 0)
			{
				a->next = *link;
				*link = a;
				break;
			}
		}
	}
	if (Cmd_FromGamecode())
		a->flags |= ALIAS_FROMSERVER;
	else
		a->flags &= ~ALIAS_FROMSERVER;

	multiline = false;
	if (Cmd_Argc() == 2)	//check the next statement for being '{'
	{
		char *line, *end;
		line = Cbuf_GetNext(Cmd_ExecLevel, false);

		while(*line <= ' ' && *line)	//skip leading whitespace.
			line++;

		for (end = line + strlen(line)-1; end >= line && *end <= ' '; end--)	//skip trailing
			*end = '\0';
		if (!strcmp(line, "{"))
			multiline = true;
		else
			Cbuf_InsertText(line, Cmd_ExecLevel, true);	//whoops. Stick the trimmed string back in to the cbuf.
	}
	else if (!strcmp(Cmd_Argv(2), "{"))
		multiline = true;

	if (multiline)
	{	//fun! MULTILINE ALIASES!!!!
		a->value = Cmd_ParseMultiline(false);
	}
	else
	{
// copy the rest of the command line
		cmd[0] = 0;		// start out with a null string
		c = Cmd_Argc();
		for (i=2 ; i< c ; i++)
		{
			strcat (cmd, Cmd_Argv(i));
			if (i != c-1)
				strcat (cmd, " ");
		}

		if (!*cmd && !dpcompat_console.ival)	//someone wants to wipe it. let them
		{
			if (a == cmd_alias)
			{
				cmd_alias = a->next;
				Z_Free(a);
				return;
			}
			else
			{
				for (b = cmd_alias ; b ; b=b->next)
				{
					if (b->next == a)
					{
						b->next = a->next;
						Z_Free(a);
						return;
					}
				}
			}
		}
		a->value = Z_StrDup (cmd);
	}
	if (Cmd_FromGamecode())
	{
		a->execlevel = RESTRICT_SERVER;	//server-set aliases MUST run at the server's level.
		a->restriction = 1;				//and be runnable at the user's level
	}
	else
	{
		a->execlevel = 0;	//run at users exec level
		a->restriction = 1;	//this is possibly a security risk if the admin also changes execlevel
	}
}

#ifdef HAVE_CLIENT
static void Cmd_AliasEdit_f (void)
{
	char *alias = Cmd_AliasExist(Cmd_Argv(1), RESTRICT_LOCAL);
	char quotedalias[2048];
	if (alias)
	{
		COM_QuotedString(alias, quotedalias, sizeof(quotedalias), false);
		Key_ConsoleReplace(va("alias %s %s", Cmd_Argv(1), quotedalias));
	}
	else
		Con_Printf("Not an alias\n");
}
#endif

/*static void Cmd_DeleteAlias(const char *name)
{
	cmdalias_t	*a, **link;
	for (link = &cmd_alias; (a = *link); link = &(*link)->next)
	{
		if (!strcmp(a->name, name))
		{
			*link = a->next;
			Z_Free(a->value);
			Z_Free(a);
			return;
		}
	}
}*/

char *Cmd_AliasExist(const char *name, int restrictionlevel)
{
	cmdalias_t	*a;
	// if the alias already exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(name, a->name))
		{
			if ((a->restriction?a->restriction:rcon_level.ival) > restrictionlevel)
			{
				return NULL;	//not at this level...
			}
			return a->value;
		}
	}
	return NULL;
}

static void Cmd_AliasLevel_f (void)
{
	cmdalias_t	*a;
	char *s = Cmd_Argv(1);
	int level;
	if (Cmd_Argc() < 2 || Cmd_Argc() > 3)
	{
		Con_TPrintf("aliaslevel <var> [execlevel]\n");
		return;
	}

	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name))
		{
			break;
		}
	}
	if (!a)
	{
		Con_TPrintf("Alias not found\n");
		return;
	}

	if (Cmd_Argc() == 3)
	{
		level = atoi(Cmd_Argv(2));
		if (level > RESTRICT_MAX)
		{
			level = RESTRICT_MAX;
		}
		else if (level < RESTRICT_MIN)
			level = RESTRICT_MIN;

		if (level > Cmd_ExecLevel || (a->restriction?a->restriction:rcon_level.ival) > Cmd_ExecLevel)
		{
			Con_TPrintf("You arn't allowed to raise a command above your own level\n");
			return;
		}

		a->execlevel = level;

		if (a->restriction == 1)
			Con_TPrintf("WARNING: %s is available to all clients, any client will be able to use it at the new level.\n", a->name);
	}
	else
		Con_TPrintf("alias %s is set to run at the user level of %i\n", s, a->execlevel);
}

//lists commands, also prints restriction level
static void Cmd_AliasList_f (void)
{
	cmdalias_t	*cmd;
	int num=0;
	int flags;

	if (!strcmp(Cmd_Argv(1), "server"))
		flags = ALIAS_FROMSERVER;
	else
		flags = 0;

	for (cmd=cmd_alias ; cmd ; cmd=cmd->next)
	{
		if ((cmd->restriction?cmd->restriction:rcon_level.ival) > Cmd_ExecLevel)
			continue;
		if (flags && !(cmd->flags & flags))
			continue;
		if (!num)
			Con_TPrintf("Alias list:\n");
		if (cmd->execlevel)
			Con_Printf(S_COLOR_TRANS"(%2i)(%2i) "S_COLOR_WHITE"^[%s\\type\\/%s^]\n", (int)(cmd->restriction?cmd->restriction:rcon_level.ival), cmd->execlevel, cmd->name, cmd->name);
		else
			Con_Printf(S_COLOR_TRANS "(%2i)     "S_COLOR_WHITE"^[%s\\type\\/%s^]\n", (int)(cmd->restriction?cmd->restriction:rcon_level.ival), cmd->name, cmd->name);
		num++;
	}
	if (num)
		Con_Printf("\n");
}

static void Alias_WriteAliases (vfsfile_t *f)
{
	const char *s;
	cmdalias_t	*cmd;
	int num=0;
	char buf[65536];
	for (cmd=cmd_alias ; cmd ; cmd=cmd->next)
	{
//		if ((cmd->restriction?cmd->restriction:rcon_level.ival) > Cmd_ExecLevel)
//			continue;
		if (cmd->flags & ALIAS_FROMSERVER)
			continue;
		if (!num)
		{
			s = va("\n//////////////////\n//Aliases\n");
			VFS_WRITE(f, s, strlen(s));
		}
		s = va("alias %s ", cmd->name);
		VFS_WRITE(f, s, strlen(s));
		s = COM_QuotedString(cmd->value, buf, sizeof(buf), false);
		VFS_WRITE(f, s, strlen(s));
		VFS_WRITE(f, "\n", 1);
		if (cmd->restriction != 1)	//1 is default
		{
			s = va("restrict %s %i\n", cmd->name, cmd->restriction);
			VFS_WRITE(f, s, strlen(s));
		}
		if (cmd->execlevel != 0)	//0 is default (runs at user's level)
		{
			s = va("aliaslevel %s %i\n", cmd->name, cmd->execlevel);
			VFS_WRITE(f, s, strlen(s));
		}
		num++;
	}
}

void Alias_WipeStuffedAliases(void)
{
	cmdalias_t	**link, *cmd;
	for (link=&cmd_alias ; (cmd=*link) ; )
	{
		if (cmd->flags & ALIAS_FROMSERVER)
		{
			*link = cmd->next;
			Z_Free(cmd->value);
			Z_Free(cmd);
		}
		else
			link=&(*link)->next;
	}
}

void Cvar_List_f (void);
void Cvar_Reset_f (void);
void Cvar_ResetAll_f(void);
void Cvar_LockDefaults_f(void);
void Cvar_PurgeDefaults_f(void);

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s	*next;
	const char				*name;
	const char				*description;
	xcommand_t				function;
	xcommandargcompletion_t	argcompletion;

	qbyte	restriction;	//restriction of admin level
} cmd_function_t;


#define	MAX_ARGS		80

static	int			cmd_argc;
static	char		*cmd_argv[MAX_ARGS];
static	char		*cmd_null_string = "";
static	char		*cmd_args = NULL, *cmd_args_buf;



static	cmd_function_t	*cmd_functions;		// possible commands to execute

/*
============
Cmd_Argc
============
*/
int		VARGS Cmd_Argc (void)
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char	*VARGS Cmd_Argv (int arg)
{
	if ( arg >= cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/

char *VARGS Cmd_Args (void)
{
	if (!cmd_args)
		return "\0\0";	//fucking hell gcc, I shouldn't need this shit.
	return cmd_args;
}

void Cmd_Args_Set(const char *newargs, size_t len)
{
	if (cmd_args_buf)
		Z_Free(cmd_args_buf);

	if (newargs)
	{
		cmd_args_buf = (char*)Z_Malloc (len+1);
		memcpy(cmd_args_buf, newargs, len);
		cmd_args_buf[len] = 0;
		cmd_args = cmd_args_buf;
	}
	else
	{
		cmd_args = NULL;
		cmd_args_buf = NULL;
	}
}

/*
============
Cmd_ShiftArgs

Shifts Cmd_Argv results down one (killing first param)
============
*/
void Cmd_ShiftArgs (int ammount, qboolean expandstring)
{
	int arg;
	while (ammount>0 && cmd_argc)
	{
		arg=0;
		cmd_argc--;
		Z_Free(cmd_argv[0]);
		while ( arg < cmd_argc )
		{
			cmd_argv[arg] = cmd_argv[arg+1];
			arg++;
		}
		cmd_argv[arg]=NULL;

		ammount--;

		if (cmd_args)
		{
			cmd_args = COM_StringParse(cmd_args, com_token, sizeof(com_token), expandstring, false);
			if (cmd_args)
				while(*cmd_args == ' ' || *cmd_args == '\t')
					cmd_args++;
		}
	}
}

static const char *Cmd_ExpandCvar(char *cvarterm, int maxaccesslevel, int *newaccesslevel, qboolean enclosed, int *len)
{
	const char *ret = NULL;
	char *fixup = NULL, fixval=0, *t;
	cvar_t	*var;
	static char temp[12];
	static char quoted[256];
	unsigned int	result;
	int termlen, pl;

	int quotetype = 0;
	const char *cvarname;

	//set foo ba"r; ${foo q} -> ba\"r
	//set foo bar; ${foo asis} -> ba"r
	//${bar q} -> <EMPTY>
	//${bar ?} -> ""
	//${bar !} -> <ERROR>
	fixup = cvarterm+strlen(cvarterm);
	fixval = 0;
	termlen = fixup - cvarterm;
	if (fixup-cvarterm > 2 && !strncmp(fixup-2, " ?", 2))
	{	//force expansion, even if not defined.
		pl = 2;
		quotetype = 2;
	}
	else if (fixup-cvarterm > 2 && !strncmp(fixup-2, " !", 2))
	{	//abort if not defined
		pl = 2;
		quotetype = 3;
	}
	else if (fixup-cvarterm > 2 && !strncmp(fixup-2, " q", 2))
	{	//escaping it if not empty, otherwise empty.
		pl = 2;
		quotetype = 1;
	}
	else if (fixup-cvarterm > 5 && !strncmp(fixup-5, " asis", 5))
	{	//no escaping...
		pl = 5;
		quotetype = 0;
	}
	else
	{
		pl = 0;
		quotetype = enclosed && dpcompat_console.ival;	//default to escaping.
	}
	if (pl)
	{
		fixup -= pl;
		fixval = *fixup;
		*fixup = 0;
	}
	else
		fixup = NULL;
	if (*cvarterm == '$')
		cvarname = Cmd_ExpandCvar(cvarterm+1, maxaccesslevel, newaccesslevel, false, &pl);
	else
		cvarname = cvarterm;

	if (!cvarname)
		;
	else if ((result = strtoul(cvarname, &t, 10)), (dpcompat_console.ival||fixval) && (*t == 0 || (*t == '-' && t[1] == 0))) //only expand $0 if its actually ${0} - this avoids conflicting with the $0 macro
	{
		if (*t == '-')	//pure number with a trailing minus means
		{				//args starting after that.
			ret = Cmd_Args();
			while (ret && result-- > 1)
				ret = COM_StringParse(ret, com_token, sizeof(com_token), false, false);
			while(ret && (*ret == ' ' || *ret == '\t'))
				ret++;
		}
		else	//purely numerical
			ret = Cmd_Argv(result);
	}
	else if (!strcmp(cvarname, "*") || !stricmp(cvarname, "cmd_args"))
	{
		ret = Cmd_Args();
	}
	else if (!strnicmp(cvarname, "cmd_argv", 8))
	{
		ret = Cmd_Argv(atoi(cvarname+8));
	}
	else if (!strcmp(cvarname, "#") || !stricmp(cvarname, "cmd_argc"))
	{
		Q_snprintfz(temp, sizeof(temp), "%u", Cmd_Argc());
		ret = temp;
	}
	else if ( (var = Cvar_FindVar(cvarname)) != NULL )
	{
		if (var->restriction <= maxaccesslevel && !((var->flags & CVAR_NOUNSAFEEXPAND) && Cmd_IsInsecure()))
		{
			ret = var->string;

			if (var->flags & CVAR_TEAMPLAYTAINT)	//if we're only allowed to expand this for teamplay, then switch access levels
				*newaccesslevel = 0;
		}
	}

	if (fixup)
		*fixup = fixval;

	if (quotetype == 3)
	{
		if (ret)
			quotetype = 1;
		else
			return NULL;
	}
	else if (quotetype == 2)
	{
		quotetype = 1;
		if (!ret)
			ret = "";
	}
	if (ret)
		*len = termlen;

	if (quotetype)
		ret = COM_QuotedString(ret?ret:"", quoted, sizeof(quoted), true);
	return ret;
}

/*
================
Cmd_ExpandString

Expands all $cvar expressions to cvar values
If not SERVERONLY, also expands $macro expressions
Note: dest must point to a 1024 byte buffer
================
*/
char *Cmd_ExpandString (const char *data, char *dest, int destlen, int *accesslevel, qboolean expandargs, qboolean expandcvars, qboolean expandmacros)
{
	unsigned int	c;
	char	buf[255];
	int		i, len;
	int		quotes = 0;
	const char	*str;
	const char	*bestvar;
	int		name_length, var_length, best_length;
	qboolean striptrailing;
	int		maxaccesslevel = *accesslevel;

	len = 0;

	while ( (c = *data) != 0)
	{
		if (c == '"')
			quotes++;

		if (c == '%' && !(quotes&1) && !dpcompat_console.ival && expandargs)
		{	//QW262/ezquake does this. kinda annoying.
			char *end;
			if (data[1] == '%')
			{
				str = "%";
				data+=2;
			}
			else if (data[1] == '#')
			{
				str = va("\"%s\"", Cmd_Args());
				data+=2;
			}
			else if (data[1] == '*')
			{
				str = Cmd_Args();
				data+=2;
			}
			else if ((i=strtol(data+1, &end, 10)) || (end!=data+1&&(!*end||*end==' '||*end=='\t')))
			{
				data = end;
				str = Cmd_Argv(i);
			}
			else
			{
				data++;
				str = "%";	//some kind of encoding error
			}

			// check buffer size
			if (len + strlen(str) >= destlen-1)
				break;
			strcpy(&dest[len], str);
			len += strlen(str);
		}
		else if (c == '$' && (!(quotes&1) || dpcompat_console.ival))
		{
			data++;
			if (*data == '$')
			{	//double-dollar expands to a single dollar.
				data++;
				str = "$";
				striptrailing = false;
				name_length = 0;
				buf[0] = 0;
				buf[1] = 0;
			}
			else if (*data == '{')
			{	//${foo} can do some especially weird expansions.
				data++;
				i = 0;
				buf[i++] = '{';
				striptrailing = (*data == '-')?true:false;
				while (*data && *data != '}')
				{
					if (i < sizeof(buf)-2)
						buf[i++] = *data;
					data++;
				}
				buf[i] = 0;
				bestvar = NULL;
				if ((str = Cmd_ExpandCvar(buf+1+striptrailing, expandcvars?maxaccesslevel:-999, accesslevel, true, &var_length)))
					bestvar = str;
				if (expandmacros && (str = TP_MacroString (buf+1+striptrailing, accesslevel, &var_length)))
					bestvar = str;
				str = bestvar;
				if (*data == '}')
				{
					data++;
					buf[i++] = '}';
					buf[i] = 0;
				}
				name_length = i;
			}
			else
			{
				striptrailing = (*data == '-')?true:false;

				// Copy the text after '$' to a temp buffer
				i = 0;
				buf[0] = 0;
				buf[1] = 0;
				bestvar = NULL;
				var_length = best_length = 0;
				while((c = *data))
				{
					if (c < ' ' || c == '$')
						break;
					if (c == ' ' && buf[0] != '{')
						break;
					data++;
					buf[i++] = c;
					buf[i] = 0;
					if ((str = Cmd_ExpandCvar(buf+striptrailing, expandcvars?maxaccesslevel:-999, accesslevel, false, &var_length)))
						bestvar = str, best_length=var_length;
					if (expandmacros && (str = TP_MacroString (buf+striptrailing, accesslevel, &var_length)))
						bestvar = str, best_length=var_length;
				}

				if (bestvar)
				{
					str = bestvar;
					name_length = best_length;
				}
				else
				{
					str = NULL;
					name_length = 0;
				}
			}

			if (str)
			{
				// check buffer size
				if (len + strlen(str) >= destlen-1)
					break;

				strcpy(&dest[len], str);
				len += strlen(str);
				i = name_length;
				while (buf[i])
					dest[len++] = buf[i++];

				if (striptrailing && !*str)
					while(*data <= ' ' && *data)
						data++;
			}
			else
			{
				// no matching cvar or macro
				dest[len++] = '$';
				if (len + strlen(buf) >= destlen-1)
					break;
				strcpy (&dest[len], buf);
				len += strlen(buf);
			}
		}
		else
		{
			dest[len] = c;
			data++;
			len++;
			if (len >= destlen-1)
				break;
		}
	}

	dest[len] = 0;

	if (len && dest[len-1] == '\r')	//with dos line endings, don't add some pointless \r char on the end.
		dest[len-1] = 0;

	return dest;
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens, stopping at the \n
============
*/
const char *Cmd_TokenizeString (const char *text, qboolean expandmacros, qboolean qctokenize)
{
	int		i;
	const char *args = NULL;

// clear the args from the last string
	for (i=0 ; i<cmd_argc ; i++)
		Z_Free (cmd_argv[i]);

	cmd_argc = 0;

	while (1)
	{
// skip whitespace up to a \n
		while (*text && (unsigned)*text <= ' ' && *text != '\n')
		{
			text++;
		}

		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			break;

		if (cmd_argc == 1)
			args = text;

		text = COM_StringParse (text, com_token, sizeof(com_token), expandmacros, qctokenize);
		if (!text)
			break;
		if (!strcmp(com_token, "\n"))
			break;

		if (cmd_argc < MAX_ARGS)
		{
			cmd_argv[cmd_argc] = (char*)Z_Malloc (Q_strlen(com_token)+1);
			Q_strcpy (cmd_argv[cmd_argc], com_token);
			cmd_argc++;
		}
	}

	if (args)
	{
		const char *argsend = text?text:args+strlen(args);
		while (argsend > args && (argsend[-1] == '\n' || argsend[-1] == '\r'))
			argsend--;
		Cmd_Args_Set(args, argsend-args);
	}
	else
		Cmd_Args_Set(NULL, 0);
	return text;
}

void Cmd_TokenizePunctation (char *text, char *punctuation)
{
	int		i;
	char *args = NULL;

// clear the args from the last string
	for (i=0 ; i<cmd_argc ; i++)
		Z_Free (cmd_argv[i]);

	cmd_argc = 0;
	Cmd_Args_Set(NULL, 0);

	while (1)
	{
// skip whitespace up to a \n
		while (*text && (unsigned)*text <= ' ' && *text != '\n')
		{
			text++;
		}

		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			break;

		if (cmd_argc == 1)
			args = text;

		text = COM_ParseToken (text, punctuation);
		if (!text)
			break;

		if (cmd_argc < MAX_ARGS)
		{
			cmd_argv[cmd_argc] = (char*)Z_Malloc (Q_strlen(com_token)+1);
			Q_strcpy (cmd_argv[cmd_argc], com_token);
			cmd_argc++;
		}
	}

	if (args)
	{
		const char *argsend = text?text:args+strlen(args);
		while (argsend > args && (argsend[-1] == '\n' || argsend[-1] == '\r'))
			argsend--;
		Cmd_Args_Set(args, argsend-args);
	}
}


/*
============
Cmd_AddCommand
============
*/

qboolean Cmd_AddCommandAD (const char *cmd_name, xcommand_t function, xcommandargcompletion_t argcompletion, const char *desc)
{
	cmd_function_t	*cmd;

// fail if the command is a variable name
	cvar_t *var = Cvar_FindVar (cmd_name);
	if (var && function)
	{
		Con_Printf ("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return false;
	}

// fail if the command already exists
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcmp (cmd_name, cmd->name))
		{
			if (cmd->function == function)	//happens a lot with q3
				Con_DPrintf ("Cmd_AddCommand: %s already defined\n", cmd_name);
			else
			{
				Con_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
				break;
			}
			return false;
		}
	}

	cmd = (cmd_function_t*)Z_Malloc (sizeof(cmd_function_t)+strlen(cmd_name)+1);
	cmd->name = (char*)(cmd+1);
	strcpy((char*)(cmd+1), cmd_name);
	cmd->argcompletion = argcompletion;
	cmd->description = desc;
	cmd->function = function;
	cmd->next = cmd_functions;
	cmd->restriction = 0;
	cmd_functions = cmd;

	return true;
}

qboolean Cmd_AddCommandD (const char *cmd_name, xcommand_t function, const char *desc)
{
	return Cmd_AddCommandAD(cmd_name, function, NULL, desc);
}
qboolean Cmd_AddCommand (const char *cmd_name, xcommand_t function)
{
	return Cmd_AddCommandAD(cmd_name, function, NULL, NULL);
}

void	Cmd_RemoveCommand (const char *cmd_name)
{
	cmd_function_t	*cmd, **back;

	back = &cmd_functions;
	while (1)
	{
		cmd = *back;
		if (!cmd)
		{
//			Con_Printf ("Cmd_RemoveCommand: %s not added\n", cmd_name);
			return;
		}
		if (!strcmp (cmd_name, cmd->name))
		{
			*back = cmd->next;
			Z_Free (cmd);
			return;
		}
		back = &cmd->next;
	}
}
void	Cmd_RemoveCommands (xcommand_t function)
{
	cmd_function_t	*cmd, **back;

	for (back = &cmd_functions; (cmd = *back); )
	{
		if (cmd->function == function)
		{
			*back = cmd->next;
			Z_Free (cmd);
			continue;
		}
		back = &cmd->next;
	}
}

static void Cmd_RestrictCommand_f (void)
{
	cmdalias_t *a;
	cvar_t *v;
	cmd_function_t	*cmd;
	char *cmd_name = Cmd_Argv(1);
	int level;

	if (Cmd_Argc() != 3 && Cmd_Argc() != 2)
	{
		Con_Printf("restrict <commandname> [level]\n");
		return;
	}

	if (Cmd_Argc() > 2)
	{
		level = atoi(Cmd_Argv(2));
		if (level > RESTRICT_MAX)
		{
			level = RESTRICT_MAX;
			if (level > Cmd_ExecLevel)
			{
				Con_TPrintf("You arn't allowed to raise a command above your own level\n");
				return;
			}
		}
		else if (level < RESTRICT_MIN)
			level = RESTRICT_MIN;
	}
	else level = 0;
//commands
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcmp (cmd_name, cmd->name))
		{
			if (Cmd_Argc() == 2)
			{
				if (cmd->restriction)
					Con_TPrintf ("%s is restricted to %i\n", cmd_name, (int)cmd->restriction);
				else
					Con_TPrintf ("%s is restricted to rcon_level (%i)\n", cmd_name, rcon_level.ival);
			}
			else if ((cmd->restriction?cmd->restriction:rcon_level.ival) > Cmd_ExecLevel)
				Con_TPrintf("You arn't allowed to alter a level above your own\n");
			else
				cmd->restriction = level;
			return;
		}
	}

//cvars
	v = Cvar_FindVar(cmd_name);
	if (v)
	{
		if (Cmd_Argc() == 2)
		{
			if (v->restriction)
				Con_TPrintf ("%s is restricted to %i\n", cmd_name, (int)v->restriction);
			else
				Con_TPrintf ("%s is restricted to rcon_level (%i)\n", cmd_name, rcon_level.ival);
		}
		else if ((v->restriction?v->restriction:rcon_level.ival) > Cmd_ExecLevel)
			Con_TPrintf("You arn't allowed to alter a level above your own\n");
		else
			v->restriction = level;

		return;
	}

	// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!Q_strcasecmp (cmd_name, a->name))
		{
			if (Cmd_Argc() == 2)
			{
				if (a->restriction)
					Con_TPrintf ("%s is restricted to %i\n", cmd_name, (int)a->restriction);
				else
					Con_TPrintf ("%s is restricted to rcon_level (%i)\n", cmd_name, rcon_level.ival);
			}
			else if ((a->restriction?a->restriction:rcon_level.ival) > Cmd_ExecLevel)
				Con_TPrintf("You arn't allowed to alter a level above your own\n");
			else
				a->restriction = level;
			return;
		}
	}

	Con_TPrintf ("restrict: %s not defined\n", cmd_name);
	return;
}

void Cmd_EnumerateLevel(int level, char *buf, size_t bufsize)
{
	cmdalias_t *a;
	cmd_function_t *cmds;
	int cmdlevel;
	*buf = 0;
	for (cmds = cmd_functions; cmds; cmds=cmds->next)
	{
		cmdlevel = cmds->restriction?cmds->restriction:rcon_level.ival;

		if (level == cmdlevel)
		{
			if (*buf)
				Q_strncatz(buf, "\t", bufsize);
			Q_strncatz(buf, cmds->name, bufsize);
		}
	}
	for (a=cmd_alias ; a ; a=a->next)
	{
		cmdlevel = a->restriction?a->restriction:rcon_level.ival;

		if (level == cmdlevel)
		{
			if (*buf)
				Q_strncatz(buf, "\t", bufsize);
			Q_strncatz(buf, a->name, bufsize);
		}
	}
}

int Cmd_Level(const char *name)
{
	cmdalias_t *a;
	cmd_function_t *cmds;
	for (cmds = cmd_functions; cmds; cmds=cmds->next)
	{
		if (!strcmp(cmds->name, name))
		{
			return cmds->restriction?cmds->restriction:rcon_level.ival;
		}
	}
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(a->name, name))
		{
			return a->restriction?a->restriction:rcon_level.ival;
		}
	}
	return -1;
}

/*
============
Cmd_Exists
============
*/
qboolean	Cmd_Exists (const char *cmd_name)
{
	cmd_function_t	*cmd;

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcmp (cmd_name,cmd->name))
			return true;
	}

	return false;
}

/*
============
Cmd_Exists
============
*/
const char *Cmd_Describe (const char *cmd_name)
{
	cmd_function_t	*cmd;

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcmp (cmd_name,cmd->name))
			return cmd->description;
	}

	return NULL;
}


/*
============
Cmd_CompleteCommand
============
*/


struct cmdargcompletion_ctx_s
{
	struct xcommandargcompletioncb_s cb;
	cmd_function_t *cmd;
	const char *prefix;
	size_t prefixlen;
	qboolean quoted;
	cmd_completion_t *res;
	const char *desc;
};
fte_inlinestatic int Q_tolower(char c)
{
	if (c >= 'a' && c <= 'z')
		c -= ('a' - 'A');
	return c;
}
static void Cmd_Complete_CheckArg(const char *value, const char *desc, const char *repl, struct xcommandargcompletioncb_s *vctx)	//compare cumulative strings and join the result
{
	struct cmdargcompletion_ctx_s *ctx = (struct cmdargcompletion_ctx_s*)vctx;
	cmd_completion_t *res = ctx->res;
	char *text;

	const char *c;
	char *p;
	char quoted[8192];

	if (!desc && ctx->desc)	//if no arg desc, use the command's.
		desc = localtext(ctx->desc);

	if (strchr(value, ' ') || strchr(value, '\t') || strchr(value, '\"') || strchr(value, '\r') || strchr(value, '\n'))
	{
		if (/*ctx->prefix[ctx->prefixlen] &&*/ !ctx->quoted)	//FIXME... Figure out some way to insert quotes earlier in the completion without it bugging out
			return;
	}

	if (ctx->quoted)
	{
		value = COM_QuotedString(value, quoted, sizeof(quoted), false);
		value++;
	}

	if (!res->guessed)
	{
		text = BZ_Malloc(ctx->prefixlen + strlen(value) + 1);
		memcpy(text, ctx->prefix, ctx->prefixlen);
		strcpy(text+ctx->prefixlen, value);
		res->guessed = text;
	}
	else
	{
		for (p = res->guessed, c = ctx->prefix; *p && c < ctx->prefix+ctx->prefixlen; p++, c++)
		{
			if (Q_tolower(*p) != Q_tolower(*c))
			{
				*p = 0;
				break;
			}
		}
		if (c == ctx->prefix+ctx->prefixlen)
			for (c = value; *p; p++, c++)
			{
				if (Q_tolower(*p) != Q_tolower(*c))
				{
					*p = 0;
					break;
				}
				if (!*c)
					break;
			}
	}
	

	if (res->num == countof(res->completions))
	{
		res->extra++;
		return;	//no more space for more options
	}

	text = BZ_Malloc(ctx->prefixlen + strlen(value) + 1);
	memcpy(text, ctx->prefix, ctx->prefixlen);
	strcpy(text+ctx->prefixlen, value);

	if (repl)
	{
		p = BZ_Malloc(ctx->prefixlen + strlen(repl) + 2);
		memcpy(p, ctx->prefix, ctx->prefixlen);
		strcpy(p+ctx->prefixlen, repl);
		if (value == quoted+1)
			Q_strcat(p, "\"");
		repl = p;
	}

	res->completions[res->num].text_alloced = true;
	res->completions[res->num].text = text;
	res->completions[res->num].desc_alloced = true;
	res->completions[res->num].desc = desc?Z_StrDup(desc):NULL;
	res->completions[res->num].repl = repl;
	res->num++;
}

static void Cmd_Complete_Check(const char *check, cmd_completion_t *res, const char *desc)	//compare cumulative strings and join the result
{
	const char *c;
	char *p;
	if (!res->guessed)
		res->guessed = Z_StrDup(check);
	else for (p = res->guessed, c = check; *p; p++, c++)
	{	//we need to do this stuff here because we're not always tracking all of them.
		if (Q_tolower(*p) != Q_tolower(*c))
		{
			*p = 0;
			break;
		}
		
		if (!*c)
			break;
	}

	if (res->num == countof(res->completions))
	{
		res->extra++;
		return;	//no more space for more options
	}

	res->completions[res->num].text_alloced = false;
	res->completions[res->num].text = check;
	res->completions[res->num].desc_alloced = false;
	res->completions[res->num].desc = desc;
	res->completions[res->num].repl = NULL;
	res->num++;
}
static void Cmd_Complete_End(cmd_completion_t *c)
{
	size_t u;
	for (u = 0; u < c->num; u++)
	{
		if (c->completions[u].text_alloced)
			Z_Free((char*)c->completions[u].text);
		c->completions[u].text_alloced = false;
		c->completions[u].text = NULL;

		if (c->completions[u].desc_alloced)
			Z_Free((char*)c->completions[u].desc);
		c->completions[u].desc_alloced = false;
		c->completions[u].desc = NULL;

		c->completions[u].repl = NULL;
	}
	c->num = 0;
	c->extra = 0;
	Z_Free(c->guessed);
	c->guessed = NULL;
	Z_Free(c->partial);
	c->partial = NULL;
}
static int QDECL Cmd_Complete_Sort(const void *a, const void *b)
{	//FIXME: its possible that they're equal (eg: filesystem searches). we should strip one in that case, but gah.
	const struct cmd_completion_opt_s *c1 = a, *c2 = b;
	return Q_strcasecmp(c1->text, c2->text);
}
cmd_completion_t *Cmd_Complete(const char *partial, qboolean caseinsens)
{
	extern cvar_group_t *cvar_groups;
	cmd_function_t	*cmd;
	int				len;
	cmdalias_t		*a;

	cvar_group_t	*grp;
	cvar_t		*cvar;
	const char *sp, *e;
	qboolean quoted = false;
	int arg = 0;

	static cmd_completion_t c;

	if (!partial)
	{
		Cmd_Complete_End(&c);
		return NULL;
	}

	if ((c.partial && !strcmp(partial, c.partial)) && c.caseinsens == caseinsens)
		return &c;	//still valid.
	Cmd_Complete_End(&c);
	c.partial = Z_StrDup(partial);
	c.caseinsens = caseinsens;

	len = 0;
	for(e = partial;;)
	{
		sp = e;	//the start of where we're trying to complete...
		while (*sp == ' ' || *sp == '\t')
			sp++;	//leading spaces are annoying...
		e = COM_Parse(sp);
		if (!arg && e)
			len = e - partial;
		if (e && (*e == ' ' || *e == '\t'))
		{	//there seems to be whitespace after it.
			arg++;
			while (*sp == ' ' || *sp == '\t')
				sp++;
			//try to handle quotes
			if (*sp == '\\' && sp[1] == '\"')
			{
				sp+=2;
				quoted = true;
			}
			else if (*sp == '\"')
			{
				sp++;
				quoted = true;
			}
			else
				quoted = false;
		}
		else
			break;
	}

//	if (len)
	{
		if (caseinsens)
		{
			for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
				if (!Q_strncasecmp (partial,cmd->name, len) && (!partial[len] || strlen(cmd->name) == len))
				{
					if (arg)
					{
						struct cmdargcompletion_ctx_s ctx;
						ctx.cb.cb = Cmd_Complete_CheckArg;
						ctx.cmd = cmd;
						ctx.prefix = partial;
						ctx.prefixlen = sp-partial;
						ctx.res = &c;
						ctx.desc = cmd->description;
						ctx.quoted = quoted;

						Cmd_MacroCompletion_c(arg, sp, &ctx.cb);
						if (cmd->argcompletion)
							cmd->argcompletion(arg, sp, &ctx.cb);
					}
					else
						Cmd_Complete_Check(cmd->name, &c, cmd->description);
				}
			for (a=cmd_alias ; a ; a=a->next)
				if (!Q_strncasecmp (partial, a->name, len) && (!partial[len] || strlen(a->name) == len))
					Cmd_Complete_Check(a->name, &c, a->value);
			for (grp=cvar_groups ; grp ; grp=grp->next)
			for (cvar=grp->cvars ; cvar ; cvar=cvar->next)
			{
				if (!Q_strncasecmp (partial,cvar->name, len) && (!partial[len] || strlen(cvar->name) == len))
					Cmd_Complete_Check(cvar->name, &c, cvar->description);
				if (cvar->name2 && !Q_strncasecmp (partial,cvar->name2, len) && (!partial[len] || strlen(cvar->name2) == len))
					Cmd_Complete_Check(cvar->name2, &c, cvar->description);
			}

		}
		else
		{
			for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
				if (!Q_strncmp (partial,cmd->name, len) && (!partial[len] || strlen(cmd->name) == len))
					Cmd_Complete_Check(cmd->name, &c, cmd->description);
			for (a=cmd_alias ; a ; a=a->next)
				if (!Q_strncmp (partial, a->name, len) && (!partial[len] || strlen(a->name) == len))
					Cmd_Complete_Check(a->name, &c, "");
			for (grp=cvar_groups ; grp ; grp=grp->next)
			for (cvar=grp->cvars ; cvar ; cvar=cvar->next)
			{
				if (!Q_strncmp (partial,cvar->name, len) && (!partial[len] || strlen(cvar->name) == len))
					Cmd_Complete_Check(cvar->name, &c, cvar->description);
				if (cvar->name2 && !Q_strncmp (partial,cvar->name2, len) && (!partial[len] || strlen(cvar->name2) == len))
					Cmd_Complete_Check(cvar->name2, &c, cvar->description);
			}
		}
	}

	//quickly sort the completions. this is primarily so that the first item is the shortest, but whatever.
	qsort(c.completions, c.num, sizeof(c.completions[0]), Cmd_Complete_Sort);
	return &c;
}

char *Cmd_CompleteCommand (const char *partial, qboolean fullonly, qboolean caseinsens, int matchnum, const char **descptr)
{
	cmd_completion_t *c = Cmd_Complete(partial, caseinsens);
	const char *text = NULL, *desc = NULL;

	if (matchnum < 0)
	{	//completes only if there's an EXACT match.
		for (matchnum = 0; matchnum < c->num; matchnum++)
		{
			if (!strcmp(partial, c->completions[matchnum].text))
			{
				text = c->completions[matchnum].text;
				desc = c->completions[matchnum].desc;
				break;
			}
		}
	}
	else if (!matchnum)
	{	//returns the longest-common-denominator of all the matches
		text = c->guessed;
		desc = ((c->num==1)?c->completions[matchnum].desc:NULL);
	}
	else
	{
		matchnum--;
		if (matchnum < c->num)
		{
			text = c->completions[matchnum].text;
			desc = c->completions[matchnum].desc;
		}
	}

	if (descptr)
		*descptr = desc;
	return (char*)text;
}


//lists commands, also prints restriction level
static void Cmd_List_f (void)
{
	cmd_function_t	*cmd;
	int num=0;
	const char *search = (Cmd_Argc()>1)?Cmd_Argv(1):NULL;
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if ((cmd->restriction?cmd->restriction:rcon_level.ival) > Cmd_ExecLevel)
			continue;

		if (search)
			if (!wildcmp(search, cmd->name))
				continue;	//nope, no match

		if (!num)
			Con_TPrintf("Command list:\n");
		Con_Printf("(%2i) %s\n", (int)(cmd->restriction?cmd->restriction:rcon_level.ival), cmd->name);
		num++;
	}
	if (num)
		Con_Printf("\n");
}

//I'm not personally keen on this name, but its somewhat standard in both DP and suse (which lh uses, hence why DP uses that name). oh well.
static void Cmd_Apropos_f (void)
{
	extern cvar_group_t *cvar_groups;
	cmd_function_t	*cmd;
	cvar_group_t	*grp;
	cvar_t	*var;
	char *name;
	char escapedvalue[1024];
	char latchedvalue[1024];
	char *query = Cmd_Argv(1);
	const char *d;

	for (grp=cvar_groups ; grp ; grp=grp->next)
	for (var=grp->cvars ; var ; var=var->next)
	{
		d = var->description?localtext(var->description):NULL;
		if (var->name && Q_strcasestr(var->name, query))
			name = var->name;
		else if (var->name2 && Q_strcasestr(var->name2, query))
			name = var->name2;
		else if (d && Q_strcasestr(d, query))
			name = var->name;
		else
			continue;
		
		COM_QuotedString(var->string, escapedvalue, sizeof(escapedvalue), false);

		if (var->latched_string)
		{
			COM_QuotedString(var->latched_string, latchedvalue, sizeof(latchedvalue), false);
			if (d)
				Con_TPrintf("cvar ^[^2%s\\type\\%s^]: %s (effective %s): ^3%s\n", name,name, latchedvalue, escapedvalue, d);
			else
				Con_TPrintf("cvar ^[^2%s\\type\\%s^]: %s (effective %s): ^3no description\n", name,name, latchedvalue, escapedvalue);
		}
		else
		{
			if (d)
				Con_TPrintf("cvar ^[^2%s\\type\\%s^]: %s : ^3%s\n", name,name, escapedvalue, d);
			else
				Con_TPrintf("cvar ^[^2%s\\type\\%s^]: %s : ^3no description\n", name,name, escapedvalue);
		}
	}

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		d = cmd->description?localtext(cmd->description):NULL;
		if (cmd->name && Q_strcasestr(cmd->name, query))
			;
		else if (d && strstr(d, query))
			;
		else
			continue;

		if (d)
			Con_TPrintf("command ^[^2%s\\type\\%s^]: ^3%s\n", cmd->name,cmd->name, d);
		else
			Con_TPrintf("command ^[^2%s\\type\\%s^]: ^3no description\n", cmd->name,cmd->name);
	}
	//FIXME: add aliases.
}

#ifdef HAVE_CLIENT		// FIXME
/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer (void)
{
	int sp;
	if (cls.state == ca_disconnected)
	{
		if (cl_warncmd.ival)
			Con_TPrintf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	if (cls.demoplayback)
		return;		// not really connected

#ifdef Q3CLIENT
	if (cls.protocol == CP_QUAKE3)
	{
		q3->cl.SendClientCommand("%s %s", Cmd_Argv(0), Cmd_Args());
		return;
	}
#endif

	sp = CL_TargettedSplit(false);
	if (Cmd_Argc() > 1)
		CL_SendSeatClientCommand(true, sp, "%s %s", Cmd_Argv(0), Cmd_Args());
	else
		CL_SendSeatClientCommand(true, sp, "%s", Cmd_Argv(0));
}

// don't forward the first argument
static void Cmd_ForwardToServer_f (void)
{
	if (cls.state == ca_disconnected)
	{
		Con_TPrintf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

#ifdef IMAGEFMT_PCX
	if (Q_strcasecmp(Cmd_Argv(1), "snap") == 0 && cls.protocol == CP_QUAKEWORLD)
	{
		if (SCR_RSShot())
			return;
	}
#endif
#ifdef NQPROT
	if (Q_strcasecmp(Cmd_Argv(1), "protocols") == 0 && cls.protocol == CP_NETQUAKE)
	{
		CL_SendClientCommand(true, "protocols %#x %#x %#x %#x %#x", PROTOCOL_VERSION_RMQ, PROTOCOL_VERSION_FITZ, PROTOCOL_VERSION_BJP3, PROTOCOL_VERSION_BJP2, PROTOCOL_VERSION_DP7);
		return;
	}
#endif
	if (Q_strcasecmp(Cmd_Argv(1), "pext") == 0 && (cls.protocol != CP_NETQUAKE || cls.fteprotocolextensions2 || cls.protocol_nq != CPNQ_ID || cls.proquake_angles_hack || cls.netchan.remote_address.type != NA_LOOPBACK))
	{	//don't send any extension flags this if we're using cl_loopbackprotocol nqid, purely for a compat test.
		//if you want to record compat-demos, disable extensions instead.
		unsigned int	fp1 = Net_PextMask(PROTOCOL_VERSION_FTE1, cls.protocol == CP_NETQUAKE),
						fp2 = Net_PextMask(PROTOCOL_VERSION_FTE2, cls.protocol == CP_NETQUAKE),
						ez1 = Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, cls.protocol == CP_NETQUAKE) & EZPEXT1_CLIENTADVERTISE;
		extern cvar_t cl_nopext;
		char line[256];
		if (cl_nopext.ival)
		{
			fp1 = 0;
			fp2 = 0;
		}
		Q_strncpyz(line, "pext", sizeof(line));
		if (fp1)
			Q_strncatz(line, va(" %#x %#x", PROTOCOL_VERSION_FTE1, fp1), sizeof(line));
		if (fp2)
			Q_strncatz(line, va(" %#x %#x", PROTOCOL_VERSION_FTE2, fp2), sizeof(line));
		if (ez1)
			Q_strncatz(line, va(" %#x %#x", PROTOCOL_VERSION_EZQUAKE1, ez1), sizeof(line));
		CL_SendClientCommand(true, "%s", line);
		return;
	}
	if (Q_strcasecmp(Cmd_Argv(1), "ptrack") == 0)
	{
		playerview_t *pv = &cl.playerview[CL_TargettedSplit(false)];
		if (!*Cmd_Argv(2))
			Cam_Unlock(pv);
		else
			Cam_Lock(pv, atoi(Cmd_Argv(2)));
		return;
	}

	if (Cmd_Argc() > 1)
	{
		int split = CL_TargettedSplit(false);
		CL_SendSeatClientCommand(true, split, "%s", Cmd_Args());
	}
}
#else
void Cmd_ForwardToServer (void)
{
}
#endif



static void	Cmd_FindForExecution (const char *name, int level, cmd_function_t **foundcmd, cmdalias_t **foundalias, cvar_t **foundcvar)
{
	//WARNING: PF_checkcommand should match the order.
	cmd_function_t	*cmd;
	cmdalias_t		*a;

	*foundcmd = NULL;
	*foundalias = NULL;
	*foundcvar = NULL;

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcasecmp (name, cmd->name))
		{
			*foundcmd = cmd;
			if (!strcmp (name, cmd->name))
				break;	//don't keep looking for others when we get an exact match.
		}
	}
	cmd = *foundcmd;
	if (!cmd)
		;
	else if (level == RESTRICT_TEAMPLAY)
	{	//extra weirdness so that teamplay macros can only execute certain known commands
		static char *tpcmds[] =
		{
			"if", "wait",						/*would be nice to include alias in here*/
			"say", "say_team", "echo",			/*display stuff, because it would be useless otherwise*/
			"set_tp", "set", "set_calc", "inc",	/*because scripting variables is fun. not.*/
			"tp_point", "tp_pickup", "tp_took"	/*updates what the $took etc macros are allowed to generate*/
		};
		size_t i;
		for (i = 0; i < countof(tpcmds); i++)
			if (!strcmp(cmd->name, tpcmds[i]))
				break;
		if (i == countof(tpcmds))
			*foundcmd = NULL;
		else if (cmd->restriction && cmd->restriction > 0)
		{
			//warning, these commands would normally be considered to be run at restrict_local, but they're running at a much lower level
			//which means that if there's ANY restriction on them then they'll fail.
			//this means we have to ignore the default restriction levels and just do it anyway.
			Con_TPrintf("'%s' was restricted.\n", cmd_argv[0]);
			*foundcmd = NULL;
		}
	}
	else if ((cmd->restriction?cmd->restriction:rcon_level.ival) > level)
	{
		Con_TPrintf("cmd '%s' was restricted.\n", name);
		*foundcmd = NULL;
	}

// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!Q_strcasecmp (cmd_argv[0], a->name))
		{
			//teamplay restrictions block any execlevel elevations, so the contents are what matter
			//(there's no reason to restrict aliases other than for exec level promotion)
			if (level!=RESTRICT_TEAMPLAY)
			if ((a->restriction?a->restriction:rcon_level.ival) > level)
			{
				Con_TPrintf("alias '%s' was restricted.\n", cmd_argv[0]);
				return;
			}

			*foundalias = a;
			if (!strcmp (name, a->name))
				break;
		}
	}

// check cvars
	*foundcvar = Cvar_FindVar(name);
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void Cmd_ExecuteString (const char *text, int level)
{
	//WARNING: PF_checkcommand should match the order.
	cmd_function_t	*cmd;
	cmdalias_t		*a;
	cvar_t			*var;
	int olev = Cmd_ExecLevel;

	char dest[65536];

	while (*text == ' ' || *text == '\n')
		text++;
	if (dpcompat_console.ival && !strncmp(text, "alias", 5) && (text[5] == ' ' || text[5] == '\t'))
		;	//certain commands don't get pre-expanded in dp. evil hack. quote them to pre-expand anyway. double evil.
	else
		text = Cmd_ExpandString(text, dest, sizeof(dest), &level, false, true/*!Cmd_IsInsecure()?true:false*/, true);
	Cmd_TokenizeString (text, (level == RESTRICT_LOCAL&&!dpcompat_console.ival)?true:false, false);

// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

	Cmd_FindForExecution (cmd_argv[0], level, &cmd, &a, &var);

//check (explicit) functions
	if (cmd && cmd->function)
	{
		Cmd_ExecLevel = level;
		cmd->function();
		Cmd_ExecLevel = olev;
		return;
	}

	//priority is cmd>alias>cvar
	//but this means that user aliases can override cvars
	//which means users can use aliases to block cvar access, aka cheat.
	//so favour the cvar when its a server command (unless the alias was also created by the server)
	if (a && (!var || Cmd_ExecLevel<RESTRICT_SERVER || a->flags&ALIAS_FROMSERVER))
	{
		int execlevel;

#ifdef HAVE_CLIENT	//an emergency escape mechansim, to avoid infinatly recursing aliases.
		extern unsigned int con_splitmodifier;

		if (keydown[K_SHIFT] && (keydown[K_LCTRL]||keydown[K_RCTRL]) && (keydown[K_LALT]||keydown[K_RALT]) && !isDedicated)
			return;
#endif

		Cmd_ExecLevel = level;

		if (level == RESTRICT_TEAMPLAY)
			execlevel = level;	//teamplay aliases can't let the user's settings promote them out of their restrictions.
		else
		{
			if (a->execlevel)
				execlevel = a->execlevel;
			else
				execlevel = level;
		}

		// if the alias value is a command or cvar and
		// the alias is called with parameters, add them
		//unless we're mimicing dp, or the alias has explicit expansions (or macros) in which case it can do its own damn args
		if (dpcompat_console.ival)
		{	//defective double escaping. the following line should sum it up nicely...
			//set foo 3; alias test "set foo 2; echo $foo==1"; set foo 1; test
			char *ignoringquoteswasstupid;
			Cmd_ExpandString(a->value, dest, sizeof(dest), &execlevel, true, !Cmd_IsInsecure()?true:false, true);
			for (ignoringquoteswasstupid = dest; *ignoringquoteswasstupid; )
			{	//double up dollars, to prevent expansion when its actually execed.
				if (*ignoringquoteswasstupid == '$')
				{
					memmove(ignoringquoteswasstupid+1, ignoringquoteswasstupid, strlen(ignoringquoteswasstupid)+1);
					ignoringquoteswasstupid++;
				}
				ignoringquoteswasstupid++;
			}
		}
		else
		{	//more sane (and ezquake-like)
			//set foo 3; alias test "set foo 2; echo $foo==2"; set foo 1; test
			//alias test "echo Args were $qt${* q}$qt"; set foo 1; test Test Args Here
			Cmd_ExpandString(a->value, dest, sizeof(dest), &execlevel, true, false, false);	//expand args, but not other stuff.
		}
		if ((a->restriction?a->restriction:rcon_level.ival) > execlevel)
			return;	//we expanded something it wasn't meant to see.

		Cbuf_InsertText ("\n", execlevel, false);
		if (!dpcompat_console.ival)
		{
			if (Cmd_Argc() > 1 && (!strncmp(a->value, "cmd ", 4) || (!strchr(a->value, ' ') && !strchr(a->value, '\t')	&&
				(Cvar_FindVar(a->value) || (Cmd_Exists(a->value) && a->value[0] != '+' && a->value[0] != '-'))))
				)
			{
				Cbuf_InsertText (Cmd_Args(), execlevel, false);
				Cbuf_InsertText (" ", execlevel, false);
			}
		}
		Cbuf_InsertText (dest, execlevel, false);

#ifdef HAVE_CLIENT
		if (con_splitmodifier > 0)
		{	//if the alias was execed via p1/p2 etc, make sure that propagates properly (at least for simple aliases like impulses)
			//fixme: should probably prefix each line. that may have different issues however.
			//don't need to care about + etc
			Cbuf_InsertText (va("p %i ", con_splitmodifier), execlevel, false);
		}
#endif

		Con_DPrintf("Execing alias %s ^3%s:\n^1%s\n^2%s\n", a->name, Cmd_Args(), a->value, dest);
		Cmd_ExecLevel = olev;
		return;
	}

// check cvars
	Cmd_ExecLevel = level;
	if (!cmd && Cvar_Command (var, level))
		;
#if defined(CSQC_DAT) && defined(HAVE_CLIENT)
	else if (CSQC_ConsoleCommand(-1, text))
		;
#endif
#if defined(MENU_DAT) && defined(HAVE_CLIENT)
	else if (MP_ConsoleCommand(text))
		;	//let the csqc handle it if it wants.
#endif
#if defined(MENU_NATIVECODE) && defined(HAVE_CLIENT)
	if (mn_entry && mn_entry->ConsoleCommand(text, cmd_argc, (char const*const*)cmd_argv))
		;
#endif

#ifdef HAVE_SERVER
	else if (sv.state && PR_ConsoleCmd(text))
		;
#endif

#if defined(VM_CG) && defined(HAVE_CLIENT)
	else if (q3 && q3->cg.ConsoleCommand())
		;
#endif
#if defined(Q3SERVER) && defined(HAVE_SERVER)
	else if (q3 && q3->sv.ConsoleCommand())
		;
#endif
#if defined(VM_UI) && defined(HAVE_CLIENT)
	else if (q3 && q3->ui.ConsoleCommand())
		;
#endif
	else if (cmd
#if defined(Q2CLIENT) && defined(HAVE_CLIENT)
		|| (cls.state!=ca_disconnected && (cls.protocol == CP_QUAKE2 || cls.protocol == CP_QUAKE3))
#endif
		)
	{	//q2 servers convert unknown commands to text.
		Cmd_ForwardToServer();
	}
	else if ((cl_warncmd.value && level <= RESTRICT_LOCAL) || developer.value)
		Con_TPrintf ("Unknown command \"%s\"\n", Cmd_Argv(0));
	Cmd_ExecLevel = olev;
}


/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/
int Cmd_CheckParm (const char *parm)
{
	int i;

	if (!parm)
		Sys_Error ("Cmd_CheckParm: NULL");

	for (i = 1; i < Cmd_Argc (); i++)
		if (! Q_strcasecmp (parm, Cmd_Argv (i)))
			return i;

	return 0;
}
















typedef struct tempstack_s{
	struct tempstack_s *next;
	char str[1];
} tempstack_t;
static tempstack_t *ifstack;

static void If_Token_Clear (tempstack_t *mark)
{
	tempstack_t *ois;
	while(ifstack)
	{
		if (ifstack == mark)
			break;
		ois = ifstack;
		ifstack = ifstack->next;
		Z_Free(ois);
	}
}

static tempstack_t *If_Token_GetMark (void)
{
	return ifstack;
}


static const char *retstring(const char *s)
{
	tempstack_t *ret;
	ret = (tempstack_t*)Z_Malloc(sizeof(tempstack_t)+strlen(s));
	ret->next = ifstack;
	ifstack=ret;
	strcpy(ret->str, s);
	return ret->str;
}
static const char *retint(int f)
{
	char s[1024];
	tempstack_t *ret;
	if (!f)
		return "";
	sprintf(s, "%d", f);
	ret = (tempstack_t*)Z_Malloc(sizeof(tempstack_t)+strlen(s));
	ret->next = ifstack;
	ifstack=ret;
	strcpy(ret->str, s);
	return ret->str;
}
static const char *retbool(qboolean b)
{
	if (b)
		return "1";
	return "";
}
static const char *retfloat(float f)
{
	char s[1024];
	tempstack_t *ret;
	if (!f)
		return "";
	sprintf(s, "%g", f);
	ret = (tempstack_t*)Z_Malloc(sizeof(tempstack_t)+strlen(s));
	ret->next = ifstack;
	ifstack=ret;
	strcpy(ret->str, s);
	return ret->str;
}
static qboolean is_numeric (const char *c)
{
	return (*c >= '0' && *c <= '9') ||
		((*c == '-' || *c == '+') && (c[1] == '.' || (c[1]>='0' && c[1]<='9'))) ||
		(*c == '.' && (c[1]>='0' && c[1]<='9'))?true:false;
}
static qboolean is_true (const char *c)
{
	if (is_numeric(c))
		return !!atof(c);
	if (!Q_strcasecmp(c, "true") || !Q_strcasecmp(c, "yes"))
		return true;
	if (!Q_strcasecmp(c, "false") || !Q_strcasecmp(c, "no") || !Q_strcasecmp(c, "null") || !Q_strcasecmp(c, "nil"))
		return false;
	return !!*c;
}
#define IF_PRI_MAX 12
#define IFPUNCT "(,{})~\':;=!><&|+*/-"
static const char *If_Token(const char *func, const char **end, int pri);
static const char *If_Token_Term(const char *func, const char **end)
{
	const char *s, *s2;
	cvar_t *var;
	int level;
	while(*func <= ' ' && *func)
		func++;

	if (*func == '\'')
	{
		char *o = com_token;
		func++;
		while (*func)
		{
			if (*func == '\'')
			{
				func++;
				break;
			}

			if (o < com_token + sizeof(com_token)-1)
				*o++ = *func;
			func++;
		}
		*o = 0;
		s = func;
	}
	else
		s = COM_ParseToken(func, IFPUNCT);

	if (*com_token == '(')
	{
		s2 = s;
		level=1;
		while (*s2)
		{
			if (*s2 == ')')
			{
				level--;
				if (!level)
				{
					s2++;
					break;
				}
			}
			else if (*s2 == '(')
				level++;
			s2++;
		}
		if (!level)
		{
			char *t = malloc(s2-s+1);
			memcpy(t, s, s2-s);
			t[s2-s-((s2==s)?0:1)] = 0;
			func = If_Token(t, end, IF_PRI_MAX);
			free(t);
		}
		else
			func = If_Token(s, end, IF_PRI_MAX);
		*end = s2;
		s = *end;
		s2 = func;
	}
	else if (*com_token == '!')
	{
		func = If_Token(s, &s, 0);
		s2 = retbool(!is_true(func));
	}
	else if (*com_token == '~')
	{
		func = If_Token(s, &s, 0);
		s2 = retbool(~atoi(func));
	}
	else if (*com_token == '-')
	{
		func = If_Token(s, &s, 0);
		s2 = retfloat(-atof(func));
	}
	else if (!strcmp(com_token, "int"))
	{
		func = If_Token(s, &s, 0);
		s2 = retint(atoi(func));
	}
	else if (!strcmp(com_token, "strlen"))
	{
		func = If_Token(s, &s, 0);
		s2 = retfloat(strlen(func));
	}
	else if (!strcmp(com_token, "eval"))
	{
		//read the stuff to the right
		func = If_Token(s, &s, IF_PRI_MAX);
		//and evaluate it
		s2 = If_Token(func, &func, IF_PRI_MAX);
	}
	else if (!strcmp(com_token, "defined"))	//functions
	{
		s = COM_ParseToken(s, IFPUNCT);
		var = Cvar_FindVar(com_token);
		*end = s;
		s2 = retbool(var != NULL);
	}
	else if (!strcmp(com_token, "random"))
	{
		s2 = retfloat((rand()&0x7fff) / (float)0x7fff);
	}
	else if (!strcmp(com_token, "vid"))	//mostly for use with the menu system.
	{
		s = COM_ParseToken(s, IFPUNCT);
#ifdef HAVE_CLIENT
		if (qrenderer == QR_NONE)
			s2 = "";
		else if (!strcmp(com_token, "width"))
			s2 = retint(vid.width);
		else if (!strcmp(com_token, "height"))
			s2 = retint(vid.height);
		else
#endif
			s2 = "";
	}
	else
	{
		if (*com_token == '$')
			var = Cvar_FindVar(com_token+1);
		else
			var = Cvar_FindVar(com_token);	//for consistancy.
		if (var)
		{
			if ((var->restriction?var->restriction:rcon_level.ival) > Cmd_ExecLevel)
			{
				Con_Printf("Console script attempted to read restricted cvar %s\n", var->name);
				s2 = "RESTRICTED";
			}
			else
				s2 = var->string;
		}
		else
			s2 = retstring(com_token);
	}

	*end = s;

	return s2;
}
enum
{
	IFOP_CAT,
	IFOP_MUL,
	IFOP_DIV,
	IFOP_MOD,
	IFOP_ADD,
	IFOP_SUB,
	IFOP_SHL,
	IFOP_SHR,
	IFOP_ISIN,
	IFOP_ISNOTIN,
	IFOP_LT,
	IFOP_LE,
	IFOP_GT,
	IFOP_GE,
	IFOP_EQ,
	IFOP_NE,
	IFOP_BA,
	IFOP_XOR,
	IFOP_BO,
	IFOP_LA,
	IFOP_LO
};
static const struct
{
	int opnamelen;
	const char *opname;
	int pri;
	int op;
} ifops[] =
{
	{3,	"cat",	3,	IFOP_CAT},
	{1, "*",	3,	IFOP_MUL},
	{3, "mul", 3,	IFOP_MUL},
	{1, "/",	3,	IFOP_DIV},
	{3,	"div",	3,	IFOP_DIV},
	{1,	"%",	3,	IFOP_MOD},
	{3,	"mod",	3,	IFOP_MOD},
	{1,	"+",	4,	IFOP_ADD},
	{3,	"add",	4,	IFOP_ADD},
	{1,	"-",	4,	IFOP_SUB},
	{3,	"sub",	4,	IFOP_SUB},
	{2,	"<<",	5,	IFOP_SHL},
	{2,	">>",	5,	IFOP_SHR},
	{4,	"isin",5,	IFOP_ISIN},		//fuhquake
	{5,	"!isin",5,	IFOP_ISNOTIN},	//fuhquake
	{2,	"<=",	6,	IFOP_LE},
	{1,	"<",	6,	IFOP_LT},
	{2,	">=",	6,	IFOP_GE},
	{1,	">",	6,	IFOP_GT},
	{2,	"==",	7,	IFOP_EQ},
	{1,	"=",	7,	IFOP_EQ},
	{5,	"equal",7,	IFOP_EQ},	//qw262
	{2,	"!=",	7,	IFOP_NE},
	{2,	"&&",	11,	IFOP_LA},
	{1,	"&",	8,	IFOP_BA},
	{3,	"and",	8,	IFOP_BA},	//qw262
	{1,	"^",	9,	IFOP_XOR},
	{3,	"xor",	8,	IFOP_XOR},	//qw262
	{2,	"||",	12,	IFOP_LO},
	{1,	"|",	10,	IFOP_BO},
	{2,	"or",	10,	IFOP_BO}	//qw262
};
static const char *If_Operator(int op, const char *left, const char *right)
{
	int r;
	switch(op)
	{
	case IFOP_CAT:
		return retstring(va("%s%s", left, right));
	case IFOP_MUL:
		return retfloat(atof(left)*atof(right));
	case IFOP_DIV:
		return retfloat(atof(left)/atof(right));
	case IFOP_MOD:
		r = atoi(right);
		if (r)
			return retfloat(atoi(left)%r);
		else
			return retfloat(0);
	case IFOP_ADD:
		return retfloat(atof(left)+atof(right));
	case IFOP_SUB:
		return retfloat(atof(left)-atof(right));
	case IFOP_SHL:
		return retfloat(atoi(left)<<atoi(right));
	case IFOP_SHR:
		return retfloat(atoi(left)>>atoi(right));
	case IFOP_ISIN:
		return retfloat(!!strstr(right, left));
	case IFOP_ISNOTIN:
		return retfloat(!strstr(right, left));
	case IFOP_LT:
		return retfloat(atof(left)<atof(right));
	case IFOP_LE:
		return retfloat(atof(left)<=atof(right));
	case IFOP_GT:
		return retfloat(atof(left)>atof(right));
	case IFOP_GE:
		return retfloat(atof(left)>=atof(right));
	case IFOP_EQ:
		if (is_numeric(left) && is_numeric(right))
			return retfloat(atof(left) == atof(right));
		else
			return retfloat(!strcmp(left, right));
	case IFOP_NE:
		if (is_numeric(left) && is_numeric(right))
			return retfloat(atof(left) != atof(right));
		else
			return retfloat(!!strcmp(left, right));
	case IFOP_BA:
		return retfloat(atoi(left)&atoi(right));
	case IFOP_XOR:
		return retfloat(atoi(left)^atoi(right));
	case IFOP_BO:
		return retfloat(atoi(left)|atoi(right));
	case IFOP_LA:
		return retfloat(is_true(left)&&is_true(right));
	case IFOP_LO:
		return retfloat(is_true(left)||is_true(right));
	default:
		return retfloat(0);
	}
}
static const char *If_Token(const char *func, const char **end, int pri)
{
	const char *s, *s2;
	int i;

	if (pri > 0)
		s2 = If_Token(func, &s, pri-1);
	else
		s2 = If_Token_Term(func, &s);
	*end = s;

	if (s)
	{
		while (*s == ' ' || *s == '\t')
			s++;

		for (i = 0; i < countof(ifops); i++)
		{
			if (!strncmp(s, ifops[i].opname, ifops[i].opnamelen))
			{
				if (pri == ifops[i].pri)
				{
					s = If_Token(s + ifops[i].opnamelen, end, pri);
					s2 = If_Operator(ifops[i].op, s2, s);
				}
				break;
			}
		}
	}
	return s2;
}

qboolean If_EvaluateBoolean(const char *text, int restriction)
{
	qboolean ret;
	const char *end;
	tempstack_t *ts = If_Token_GetMark();
	int restore = Cmd_ExecLevel;
	Cmd_ExecLevel = restriction;
	text = If_Token(text, &end, IF_PRI_MAX);
	ret = is_true(text);
	If_Token_Clear(ts);
	Cmd_ExecLevel = restore;
	return ret;
}

static void Cbuf_ExecBlock(int level)
{
	char *remainingcbuf;
	char *exectext = NULL;
	char *line, *end;
	line = Cbuf_GetNext(level, false);

	while(*line <= ' ' && *line)	//skip leading whitespace.
		line++;

	for (end = line + strlen(line)-1; end >= line && *end <= ' '; end--)	//skip trailing
		*end = '\0';

	if (!strcmp(line, "{"))	//multiline block
	{
		int indent = 1;

		for(;;)
		{
			line = Cbuf_GetNext(level, false);

			while(*line <= ' ' && *line)	//skip leading whitespace.
				line++;

			for (end = line + strlen(line)-1; end >= line && *end <= ' '; end--)	//skip trailing
				*end = '\0';

			if (!strcmp(line, "{"))
				indent++;
			else if (!strcmp(line, "}"))
			{
				indent--;
				if (!indent)
					break;
			}
			else if (!*line)
			{
				Con_Printf("Unterminated block\n");
				break;
			}

			if (exectext)
			{
				char *newv;
				newv = (char*)Z_Malloc(strlen(exectext) + strlen(line) + 2);
				sprintf(newv, "%s;%s", exectext, line);
				Z_Free(exectext);
				exectext = newv;
			}
			else
				exectext = Z_StrDup(line);
//			Con_Printf("Exec \"%s\"\n", line);
		}
	}
	else
	{
		exectext = Z_StrDup(line);
//		Con_Printf("Exec \"%s\"\n", line);
	}
	remainingcbuf = Cbuf_StripText(level);	//this craziness is to prevent an if } from breaking the entire con text
	Cbuf_AddText(exectext, level);
	Z_Free(exectext);
	Cbuf_ExecuteLevel(level);
	Cbuf_AddText(remainingcbuf, level);
	Z_Free(remainingcbuf);
}

static void Cbuf_SkipBlock(int level)
{
	char *line, *end;
	line = Cbuf_GetNext(level, false);

	while(*line <= ' ' && *line)	//skip leading whitespace.
		line++;

	for (end = line + strlen(line)-1; end >= line && *end <= ' '; end--)	//skip trailing
		*end = '\0';

	if (!strcmp(line, "{"))	//multiline block
	{
		int indent = 1;

		for(;;)
		{
			line = Cbuf_GetNext(level, false);

			while(*line <= ' ' && *line)	//skip leading whitespace.
				line++;

			for (end = line + strlen(line)-1; end >= line && *end <= ' '; end--)	//skip trailing
				*end = '\0';

			if (!strcmp(line, "{"))
				indent++;
			else if (!strcmp(line, "}"))
			{
				indent--;
				if (!indent)
					break;
			}
			else if (!*line)
			{
				Con_Printf("Unterminated block\n");
				break;
			}
//			Con_Printf("Skip \"%s\"\n", line);
		}
	}
//	else
//		Con_Printf("Skip \"%s\"\n", line);
}

void Cmd_if_f(void)
{
	char *text = Cmd_Args();
	const char *ret;
	char *end;
	char *ws;
	int level;
	qboolean trueblock=false;
	tempstack_t *ts;

	if (Cmd_Argc()==1)
	{
		Con_TPrintf("if <condition> <statement> [elseif <condition> <statement>] [...] [else <statement>]\n");
		return;
	}

	ts = If_Token_GetMark();
	level = Cmd_ExecLevel;

elseif:
//	Con_Printf("if %s\n", text);
	ret = If_Token(text, (const char **)&end, IF_PRI_MAX);
	if (!end)
	{
		Con_TPrintf("Not terminated\n");
		If_Token_Clear(ts);
		return;
	}

skipws:
	while(*end == ' ' || *end == '\t')	//skip leading whitespace.
		end++;

	for (ws = end + strlen(end)-1; ws >= end && *ws <= ' '; ws--)	//skip trailing
		*ws = '\0';

	if (!strncmp(end, "then", 4))	//sigh... trying to make fuhquake's ifs work.
	{
		end+=4;
		goto skipws;
	}

	while (*end == ' ' || *end == '\t')
		end++;

	if (!*end)
	{
		if (is_true(ret))	//equation was true.
		{
			trueblock = true;
			Cbuf_ExecBlock(level);
		}
		else	//equation was false.
		{
skipblock:
			Cbuf_SkipBlock(level);
		}
		end = Cbuf_GetNext(level, false);
		while(*end <= ' ' && *end)
			end++;
		if (!strncmp(end, "else", 4))
		{
			end+=4;
			while(*end <= ' ' && *end)	//skip leading whitespace.
				end++;

			if (!strncmp(end, "if", 2))
			{
				text = end + 2;
				if (trueblock)
					goto skipblock;	//we've had our true, all others are assumed to be false.
				else
					goto elseif;	//and have another go.
			}
			else
			{	//we got an else. This is the last block. Don't go through the normal way, cos that would let us follow up with a second else.
				if (trueblock)
					Cbuf_SkipBlock(level);
				else
					Cbuf_ExecBlock(level);

				If_Token_Clear(ts);
				return;
			}
		}
		//whoops. Too far.
		Cbuf_InsertText(end, level, true);
		If_Token_Clear(ts);
		return;
	}

	text = strstr(end, "else");
	if (ret && *ret)
	{
		if (text)	//don't bother execing the else bit...
			*text = '\0';
		Cbuf_InsertText(end, level, true);
	}
	else
	{
		if (text)
			Cbuf_InsertText(text+4, level, true);	//ironically, this will do elseif...
	}

	If_Token_Clear(ts);
}

static void Cmd_Vstr_f( void )
{
	char	*v;

	if (Cmd_Argc () != 2)
	{
		Con_Printf ("vstr <variablename> : execute a variable command\n");
		return;
	}

	v = Cvar_VariableString(Cmd_Argv(1));
	Cbuf_InsertText(v, Cmd_ExecLevel, true);
}

static void Cmd_toggle_f(void)
{
	cvar_t *v;
	if (Cmd_Argc()<2)
	{
		Con_Printf("missing cvar name\n");
		return;
	}
	v = Cvar_Get(Cmd_Argv(1), "0", 0, "Custom variables");
	if (!v)
		return;

	if (Cmd_Argc() >= 3)
	{
		const char *newval = Cmd_Argv(2);
		const char *defval = (Cmd_Argc()>3)?Cmd_Argv(3):v->defaultstr;
		if (!strcmp(newval, v->string))
			Cvar_Set(v, defval);
		else
			Cvar_Set(v, newval);
	}
	else
	{
		if (v->value)
			Cvar_Set(v, "0");
		else
			Cvar_Set(v, "1");
	}
}

static void Cmd_Set_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	extern cvar_group_t *cvar_groups;
	size_t len = strlen(partial);
	cvar_group_t	*grp;
	cvar_t	*var;
	if (argn != 1)
		return;
	for (grp=cvar_groups ; grp ; grp=grp->next)
		for (var=grp->cvars ; var ; var=var->next)
		{
			if (!Q_strncasecmp (partial, var->name, len))
				ctx->cb(var->name, var->description, NULL, ctx);
			else if (var->name2 && !Q_strncasecmp (partial, var->name2, len))
				ctx->cb(var->name2, var->description, NULL, ctx);
		}
}

static void Cmd_set_f(void)
{
	void *mark;
	cvar_t *var;
	const char *end;
	const char *text;
	int forceflags = 0;
	qboolean docalc;
	char name[256];
	const char *desc = NULL;

	if (!strcmp(Cmd_Argv(0), "set_calc") || !strcmp(Cmd_Argv(0), "seta_calc"))
		docalc = true;
	else
		docalc = false;

	if (Cmd_Argc()<3)
	{
		if (docalc)
			Con_TPrintf("%s %s <equation>\n", Cmd_Argv(0), *Cmd_Argv(1)?Cmd_Argv(1):"<var>");
		else if (!strcmp(Cmd_Argv(0), "setfl"))
			Con_TPrintf("%s %s <value> <a|u|s>\n", Cmd_Argv(0), *Cmd_Argv(1)?Cmd_Argv(1):"<var>");
		else
			Con_TPrintf("%s %s <value>\n", Cmd_Argv(0), *Cmd_Argv(1)?Cmd_Argv(1):"<var>");
		return;
	}

	if (!strncmp(Cmd_Argv(0), "seta", 4) && !Cmd_FromGamecode())
		forceflags |= CVAR_ARCHIVE;

	Q_strncpyz(name, Cmd_Argv(1), sizeof(name));

	if (!strcmp(Cmd_Argv(0), "setfl") || Cmd_FromGamecode())	//AARGHHHH!!! Q2 set command is different
	{
		text = Cmd_Argv(3);
		while(*text)
		{
			switch(*text++)
			{
			case 'u':
				forceflags |= CVAR_USERINFO;
				break;
			case 's':
				forceflags |= CVAR_SERVERINFO;
				break;
			case 'a':
				forceflags |= CVAR_ARCHIVE;
				break;
			default:
				return;
			}
		}
		text = Cmd_Argv(2);

		if (Cmd_Argc()>=5)
			desc = Cmd_Argv(4);
	}
	else if (dpcompat_set.ival && !docalc)
	{
		text = Cmd_Argv(2);
		if (Cmd_Argc()>=4)
			desc = Cmd_Argv(3);
	}
	else
	{
		Cmd_ShiftArgs(1, false);
		text = Cmd_Args();
		if (!docalc && Cmd_Argc()==2 && (*text == '\"' || (*text == '\\' && text[1] == '\"')))	//if it's already quoted, dequote it, and ignore trailing stuff, for q2/q3 compatability
		{
			desc = COM_StringParse (text, com_token, sizeof(com_token), false, false);
			while (*desc == ' ' || *desc == '\t')
				desc++;
			if (desc[0] == '/' && desc[1] == '/')
			{
				desc+=2;
				while (*desc == ' ' || *desc == '\t')
					desc++;
				end = desc + strlen(desc);
				while (end > desc)
				{
					end--;
					if (*end == ' ' || *end == '\t' || *end == '\r')
						*(char*)end = 0;
					else
						break;
				}
			}
			else
				desc = NULL;
			text = Cmd_Argv(1);
		}
		else
		{
			desc = strstr(text, "//");
			if (desc)
				end = desc;
			else
				end = text+strlen(text);
			end--;
			while (end >= text)
			{
				if (*end == ' ' || *end == '\t' || *end == '\r')
					end--;
				else
					break;
			}
			end++;
			*(char*)end = 0;
			if (desc)
			{
				desc+=2;
				while(*desc == ' ' || *desc == '\t')
					desc++;
				end = desc + strlen(desc);
				while (end > desc)
				{
					end--;
					if (*end == ' ' || *end == '\t' || *end == '\r')
						*(char*)end = 0;
					else
						break;
				}
			}
		}
		//fixme: should peek onto the next line to see if that's an indented // too, or something.
		forceflags |= 0;
	}

	var = Cvar_Get2 (name, text, CVAR_TEAMPLAYTAINT|forceflags, desc, "Custom variables");

	mark = If_Token_GetMark();

	if (var)
	{
		if (var->flags & CVAR_NOTFROMSERVER && Cmd_IsInsecure())
		{
			Con_Printf ("Server tried setting %s cvar\n", var->name);
			return;
		}
		if (var->flags & CVAR_NOSET)
		{
			Con_Printf ("variable %s is readonly\n", var->name);
			return;
		}
		if (var->flags & CVAR_NOUNSAFEEXPAND)
			forceflags &= ~(CVAR_USERINFO|CVAR_SERVERINFO);

		if (Cmd_FromGamecode())
		{
			if (forceflags)
			{
				var->flags &=~(CVAR_USERINFO|CVAR_SERVERINFO);
				var->flags |= forceflags;
			}
			Cvar_LockFromServer(var, text);
		}
		else
		{
			if (docalc)
				text = If_Token(text, &end, IF_PRI_MAX);
			var->flags |= CVAR_USERCREATED | forceflags;
			Cvar_Set(var, text);

			if (Cmd_ExecLevel == RESTRICT_TEAMPLAY)
				var->flags |= CVAR_TEAMPLAYTAINT;
		}
	}
	else
	{
		if (docalc)
			text = If_Token(text, &end, IF_PRI_MAX);
		if (Cmd_FromGamecode())
		{
			var = Cvar_Get(Cmd_Argv(1), "", 0, "Game variables");
			if (var)
				Cvar_LockFromServer(var, text);
		}
		else
			var = Cvar_Get(Cmd_Argv(1), text, CVAR_USERCREATED, "User variables");

		if (var)
			var->flags |= forceflags;
	}

	If_Token_Clear(mark);
}

static void Cvar_Inc_f (void)
{
	int c;
	cvar_t *var;
	float delta;

	c = Cmd_Argc();
	if (c != 2 && c != 3)
	{
		Con_Printf ("inc <cvar> [value]\n");
		return;
	}

	var = Cvar_FindVar (Cmd_Argv(1));
	if (!var)
	{
		Con_Printf ("Unknown variable \"%s\"\n", Cmd_Argv(1));
		return;
	}
	if (var->flags & CVAR_NOTFROMSERVER && Cmd_IsInsecure())
	{
		Con_Printf ("Server tried setting %s cvar\n", var->name);
		return;
	}


	delta = (c == 3) ? atof (Cmd_Argv(2)) : 1;

	if (Cmd_ExecLevel == RESTRICT_TEAMPLAY || (var->flags & CVAR_TEAMPLAYTAINT))
	{
		Cvar_SetValue (var, var->value + delta);
		var->flags |= CVAR_TEAMPLAYTAINT;
	}
	else
		Cvar_SetValue (var, var->value + delta);
}

void Cvar_ParseWatches(void)
{
	const char *cvarname;
	int i;
	cvar_t *var;
	for (i=1 ; i<com_argc-1 ; i++)
	{
		if (!com_argv[i] || strcmp(com_argv[i], "-watch"))
			continue;		// NEXTSTEP sometimes clears appkit vars.
		cvarname = com_argv[i+1];
		if (!cvarname)
			continue;

		var = Cvar_FindVar (cvarname);
		if (!var)
		{
			Con_Printf ("cvar \"%s\" is not defined yet\n", cvarname);
			continue;
		}
		var->flags |= CVAR_WATCHED;
		cvar_watched = true;

		i++;
	}
}

static void Cvar_Watch_f(void)
{
	char *cvarname = Cmd_Argv(1);
	cvar_t *var;
	cvar_group_t *grp;
	extern cvar_group_t *cvar_groups;

	if (!strcmp(cvarname, ""))
	{
		for (grp=cvar_groups ; grp ; grp=grp->next)
		for (var=grp->cvars ; var ; var=var->next)
		{
			if (var->flags & CVAR_WATCHED)
				Con_Printf("Watching %s\n", var->name);
		}
		return;
	}
	else if (!strcmp(cvarname, "off"))
	{
		cvar_watched = false;
		Con_Printf("Disabling all cvar watches\n");
		for (grp=cvar_groups ; grp ; grp=grp->next)
		for (var=grp->cvars ; var ; var=var->next)
			var->flags &= ~CVAR_WATCHED;
		return;
	}
	else if (!strcmp(cvarname, "all"))
	{
		cvar_watched = 2;
		Con_Printf("Notifying for ALL cvar changes\n");
		return;
	}
	else
	{
		var = Cvar_FindVar (cvarname);
		if (!var)
		{
			Con_Printf ("cvar \"%s\" is not defined yet\n", cvarname);
			return;
		}
		var->flags |= CVAR_WATCHED;
		cvar_watched |= true;
	}
}

static void Cmd_WriteConfig_f(void)
{
	char curtime[256];
	vfsfile_t *f;
	char *filename;
	char fname[MAX_QPATH];
	char displayname[MAX_OSPATH];
	qboolean all = true;
	qboolean nohidden = false;

	//special variation that only saves if an archived cvar was actually modified.
	if (!Q_strcasecmp(Cmd_Argv(0), "cfg_save_ifmodified"))
		if (!Cvar_UnsavedArchive())
			return;

	filename = Cmd_Argv(1);
	if (!*filename)
	{
		if (*fs_manifest->mainconfig)
			Q_strncpyz(fname, fs_manifest->mainconfig, sizeof(fname));
		else
			Q_strncpyz(fname, "config.cfg", sizeof(fname));	//write SOMETHING.

#if defined(CL_MASTER) && defined(HAVE_CLIENT)
		MasterInfo_WriteServers();
#endif

		f = FS_OpenWithFriends(fname, displayname, sizeof(displayname), 4, "quake.rc", "hexen.rc", "*.cfg", "configs/*.cfg", "dlcache/*.pk3*");

		all = cfg_save_all.ival;
	}
	else if (!Q_strcasecmp(Cmd_Argv(0), "saveconfig"))
	{
		//dpcompat: this variation allows writing to any path. at least force the extension.
		Q_snprintfz(fname, sizeof(fname), "%s", filename);
		COM_RequireExtension(fname, ".cfg", sizeof(fname));

		if (!strncmp(fname, "data/", 5))
			nohidden = true;	//we're writing to the data/ dir, which mods may potentially read. don't write any settings they're not allowed to see.
		else if (Cmd_IsInsecure())
		{
			Con_Printf ("%s %s: not allowed\n", Cmd_Argv(0), Cmd_Args());
			return;
		}

		FS_DisplayPath(fname, FS_BASEGAMEONLY, displayname, sizeof(displayname));
		FS_CreatePath(fname, FS_BASEGAMEONLY);
		f = FS_OpenVFS(fname, "wbp", FS_BASEGAMEONLY);

		all = cfg_save_all.ival;// || !*cfg_save_all.string;
	}
	else
	{
		if (Cmd_IsInsecure())
		{
			Con_Printf ("%s %s: not allowed\n", Cmd_Argv(0), Cmd_Args());
			return;
		}

		if (strstr(filename, ".."))
		{
			Con_Printf (CON_ERROR "Couldn't write config %s\n",filename);
			return;
		}
		Q_snprintfz(fname, sizeof(fname), "configs/%s", filename);
		COM_DefaultExtension(fname, ".cfg", sizeof(fname));

		FS_DisplayPath(fname, FS_BASEGAMEONLY, displayname, sizeof(displayname));
		FS_CreatePath(fname, FS_BASEGAMEONLY);
		f = FS_OpenVFS(fname, "wbp", FS_BASEGAMEONLY);

		all = cfg_save_all.ival || !*cfg_save_all.string;
	}
	if (!f)
	{
		Con_Printf (CON_ERROR "Couldn't write config %s\n", displayname);
		return;
	}

	{
		time_t t = time(NULL);
		strftime(curtime, sizeof(curtime), "%Y-%m-%d %H:%M", localtime(&t));
	}
	VFS_PRINTF(f, "// %s config file (%s)\n\n", *fs_gamename.string?fs_gamename.string:FULLENGINENAME, curtime);
#ifdef HAVE_CLIENT
	if (cfg_save_binds.ival)
		Key_WriteBindings (f);
	if (cfg_save_buttons.ival)
		IN_WriteButtons(f, all);
	
	#ifdef HAVE_CDPLAYER
//		if (cfg_save_cdtracks.ival)
			Media_SaveTracks(f);
	#endif
	if (cfg_save_infos.ival)
		CL_SaveInfo(f);
#else
	VFS_WRITE(f, "// Dedicated Server config\n\n", 28);
#endif
#ifdef HAVE_SERVER
	if (cfg_save_infos.ival)
		SV_SaveInfos(f);
#else
	VFS_WRITE(f, "// no local/server infos\n\n", 26);
#endif
	if (cfg_save_aliases.ival)
		Alias_WriteAliases (f);
	Cvar_WriteVariables (f, all, nohidden);
	VFS_CLOSE(f);

	Cvar_Saved();

	Con_Printf ("Wrote %s\n",displayname);
}

static void Cmd_Reset_f(void)
{
	//FIXME: this should reset to _engine_ defaults (with fmf overrides)
	Cbuf_InsertText("cvarreset *\n", RESTRICT_LOCAL, false);
	//FIXME: wipe aliases
	//FIXME: reset + commands
	//FIXME: destroy custom cvars (unless there's a live pointer still... which is not tracked properly, bah)
//	Cbuf_InsertText("exec default.cfg\n", RESTRICT_LOCAL, false);
}

#ifdef HAVE_CLIENT
// dumps current console contents to a text file
static void Cmd_Condump_f(void)
{
	console_t *c = Con_GetMain();
	vfsfile_t *f;
	char *filename;
	char line[8192];

	if (!c)
	{
		Con_Printf ("No console to dump.\n");
		return;
	}

	if (Cmd_IsInsecure()) // don't allow insecure level execute this
		return;

	filename = Cmd_Argv(1);
	if (!*filename)
		filename = "condump";

	filename = va("%s", filename);
	COM_DefaultExtension(filename, ".txt", MAX_QPATH);

	f = FS_OpenVFS (filename, "wb", FS_GAME);
	if (!f)
	{
		Con_Printf (CON_ERROR "Couldn't write console dump %s\n",filename);
		return;
	}

	// print out current contents of console
	// stripping out starting blank lines and blank spaces
	{
		conline_t *l;
		conchar_t *t;
		for (l = c->oldest; l; l = l->newer)
		{
			t = (conchar_t*)(l+1);
			COM_DeFunString(t, t + l->length, line, sizeof(line), true, !!(c->parseflags & PFS_FORCEUTF8));
			VFS_WRITE(f, line, strlen(line));
			VFS_WRITE(f, "\n", 1);
		}
	}

	VFS_CLOSE(f);

	Con_Printf ("Dumped console to %s\n",filename);
}
#endif

void Cmd_Shutdown(void)
{
	cmd_function_t *c;
	cmdalias_t *a;
	int i;

	//make sure we get no other execution
	int level;
	for (level = 0; level < sizeof(cmd_text)/sizeof(cmd_text[0]); level++)
	{
		SZ_Clear (&cmd_text[level].buf);

		if (cmd_text[level].buf.data)
		{
			BZ_Free(cmd_text[level].buf.data);
			cmd_text[level].buf.data = NULL;
			cmd_text[level].buf.maxsize = 0;
		}
	}

	Cmd_Complete(NULL, false);	//NULL frees any cached results without generating new ones.

	while(cmd_functions)
	{
		c = cmd_functions;
		cmd_functions = c->next;
		Z_Free(c);
	}
	while(cmd_alias)
	{
		a = cmd_alias;
		cmd_alias = a->next;
		Z_Free(a->value);
		Z_Free(a);
	}

	for (i=0 ; i<cmd_argc ; i++)
		Z_Free (cmd_argv[i]);
	Z_Free(cmd_args_buf);
	cmd_argc = 0;
	cmd_args_buf = NULL;
}


static char	macro_buf[256] = "";
static char *Macro_Time (void)
{
	time_t		t;
	struct tm	*ptm;

	time (&t);
	ptm = localtime (&t);
	if (!ptm)
		return "#bad date#";
	strftime (macro_buf, sizeof(macro_buf)-1, "%H:%M", ptm);
	return macro_buf;
}
static char *Macro_UKDate (void)	//and much but not all of EU
{
	time_t		t;
	struct tm	*ptm;

	time (&t);
	ptm = localtime (&t);
	if (!ptm)
		return "#bad date#";
	strftime (macro_buf, sizeof(macro_buf)-1, "%d.%m.%Y", ptm);
	return macro_buf;
}
static char *Macro_USDate (void)	//and much but not all of EU
{
	time_t		t;
	struct tm	*ptm;

	time (&t);
	ptm = localtime (&t);
	if (!ptm)
		return "#bad date#";
	strftime (macro_buf, sizeof(macro_buf)-1, "%m.%d.%Y", ptm);
	return macro_buf;
}
static char *Macro_ProperDate (void)	//americans get it wrong. besides, this is more easily sortable for filenames etc
{
	time_t		t;
	struct tm	*ptm;

	time (&t);
	ptm = localtime (&t);
	if (!ptm)
		return "#bad date#";
	strftime (macro_buf, sizeof(macro_buf)-1, "%Y-%m-%d", ptm);
	return macro_buf;
}
static char *Macro_Version (void)
{
	/*	you probably don't need date, but it's included as this is likly to be used by
		q2 servers checking for cheats. */
	return va("%.2f %s", 2.57, version_string());
}
static char *Macro_Dedicated (void)
{
	if (isDedicated)
		return "1";
	else
		return "0";
}
static char *Macro_Quote (void)
{
	return "\"";
}

static char *Macro_Random(void)
{
	Q_snprintfz(macro_buf, sizeof(macro_buf), "%u", rand());
	return macro_buf;
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
	cmd_stuffedcmdline = false;
	macro_count = 0;	
//
// register our commands
//
	Cmd_AddCommandAD ("cfg_save",Cmd_WriteConfig_f, Cmd_Exec_c, NULL);
	Cmd_AddCommandAD ("cfg_save_ifmodified",Cmd_WriteConfig_f, Cmd_Exec_c, NULL);
	Cmd_AddCommandAD ("saveconfig",Cmd_WriteConfig_f, Cmd_Exec_c, NULL);	//for dpcompat

	Cmd_AddCommandAD ("cfg_load",Cmd_Exec_f, Cmd_Exec_c, NULL);
	Cmd_AddCommand ("cfg_reset",Cmd_Reset_f);	//ezquake
	Cmd_AddCommand ("resetcfg",Cmd_Reset_f);	//QS

	Cmd_AddCommandAD ("exec",Cmd_Exec_f, Cmd_Exec_c, NULL);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("alias",Cmd_Alias_f);
	Cmd_AddCommand ("newalias",Cmd_Alias_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
#ifdef HAVE_CLIENT
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer_f);
	Cmd_AddCommand ("condump", Cmd_Condump_f);
	Cmd_AddCommandAD ("aliasedit", Cmd_AliasEdit_f, Key_Alias_c, NULL);
#endif
	Cmd_AddCommand ("restrict", Cmd_RestrictCommand_f);
	Cmd_AddCommandAD ("aliaslevel", Cmd_AliasLevel_f, Key_Alias_c, NULL);

	Cmd_AddCommandAD ("showalias", Cmd_ShowAlias_f, Key_Alias_c, NULL);

	Cmd_AddCommandAD ("toggle", Cmd_toggle_f, Cmd_Set_c, "Toggles a cvar between two values\ntoggle CVARNAME [newval [altval]]");
	Cmd_AddCommandAD ("set", Cmd_set_f, Cmd_Set_c, "Changes the current value of the named cvar, creating it if it doesn't yet exist.");
	Cmd_AddCommandAD ("setfl", Cmd_set_f, Cmd_Set_c, "Changes the current value of the named cvar, creating it if it doesn't yet exist. The third arg allows setting cvar flags and should be u, s, or a. This command should normally be used only inside default.cfg.");
	Cmd_AddCommandAD ("set_calc", Cmd_set_f, Cmd_Set_c, "Sets the named cvar to the result of a (complex) expression.");
#ifndef QUAKETC
	Cmd_AddCommandAD ("set_tp", Cmd_set_f, Cmd_Set_c, "Identical to set. Included for compatibility with ezquake where ruleset restrictions on macros are on commands rather than cvars.");
#endif
	Cmd_AddCommandAD ("seta", Cmd_set_f, Cmd_Set_c, "Changes the current value of the named cvar, creating it if it doesn't yet exist. Also forces the archive flag so that the cvar will always be written into any saved configs.");
	Cmd_AddCommandAD ("seta_calc", Cmd_set_f, Cmd_Set_c, "Sets the named cvar to the result of a (complex) expression. Also forces the archive flag so that the cvar will always be written into any saved configs.");
	Cmd_AddCommandD ("vstr", Cmd_Vstr_f, "Executes the string value of the cvar, much like if it were an alias. For compatibility with q3.");
	Cmd_AddCommandAD ("inc", Cvar_Inc_f, Cmd_Set_c, "Adds a value to the named cvar. Use a negative value if you wish to decrease the cvar's value.");
	Cmd_AddCommandD ("if", Cmd_if_f, "For conditionally executing console commands.");

	Cmd_AddCommand ("cmdlist", Cmd_List_f);
	Cmd_AddCommand ("aliaslist", Cmd_AliasList_f);
	Cmd_AddCommandD ("macrolist", Cmd_MacroList_f, "Lists all available $macro expansions.");
	Cmd_AddCommandD ("cvarlist", Cvar_List_f, "Lists all cvars. eg, 'cvarlist -cvd *' can be used to list all cvars with a value other than the mod's default.");
	Cmd_AddCommandD ("cvarreset", Cvar_Reset_f, "Resets the named cvar to its default value.");
	Cmd_AddCommandD ("cvarwatch", Cvar_Watch_f, "Prints a notification when the named cvar is changed. Also displays the start/end of configs. Alternatively, use '-watch foo' on the commandline.");
	Cmd_AddCommand ("cvar_lockdefaults", Cvar_LockDefaults_f);
	Cmd_AddCommandD ("cvar_purgedefaults", Cvar_PurgeDefaults_f, "Resets all cvar defaults to back to the engine's default. Does not change their active value.");
	Cmd_AddCommandD ("cvar_resettodefaults_nosaveonly", Cvar_ResetAll_f, "Resets all unsaved cvars to the mod's default value.");
	Cmd_AddCommandD ("cvar_resettodefaults_saveonly", Cvar_ResetAll_f, "Resets all saved cvars to the mod's default value (without changing unsaved cvars).");
	Cmd_AddCommandD ("cvar_resettodefaults_all", Cvar_ResetAll_f, "Resets all cvars to the mod's default value.");

	Cmd_AddCommandD ("apropos", Cmd_Apropos_f, "Lists all cvars or commands with the specified substring somewhere in their name or descrition.");
	Cmd_AddCommandD ("find", Cmd_Apropos_f, "Lists all cvars or commands with the specified substring somewhere in their name or descrition.");

	Cmd_AddMacro("random", Macro_Random, true);
	Cmd_AddMacro("time", Macro_Time, true);
	Cmd_AddMacro("ukdate", Macro_UKDate, false);
	Cmd_AddMacro("usdate", Macro_USDate, false);
	Cmd_AddMacro("date", Macro_ProperDate, false);
	Cmd_AddMacro("version", Macro_Version, false);
	Cmd_AddMacro("qt", Macro_Quote, false);
	Cmd_AddMacro("dedicated", Macro_Dedicated, false);

#ifdef HAVE_CLIENT
	Cvar_Register(&tp_disputablemacros, "Teamplay");
#endif

	Cvar_Register(&ruleset_allow_in, "Console");
	Cmd_AddCommandD ("in", Cmd_In_f, "Issues the given command after a time delay. Disabled if ruleset_allow_in is 0.");

#ifdef HAVE_LEGACY
	Cmd_AddCommandD ("defer", Cmd_In_f, "Issues the given command after a time delay. Disabled if ruleset_allow_in is 0.");
	Cvar_Register(&dpcompat_set, "Darkplaces compatibility");
	Cvar_Register(&dpcompat_console, "Darkplaces compatibility");
#endif
	Cvar_Register (&cl_warncmd, "Warnings");
	Cvar_Register (&cfg_save_all, "client operation options");
	Cvar_Register (&cfg_save_auto, "client operation options");
	Cvar_Register (&cfg_save_infos, "client operation options");
	Cvar_Register (&cfg_save_aliases, "client operation options");
	Cvar_Register (&cfg_save_binds, "client operation options");
	Cvar_Register (&cfg_save_buttons, "client operation options");

#ifdef HAVE_CLIENT
	rcon_level.ival = atof(rcon_level.enginevalue);	//client is restricted to not be allowed to change restrictions.
#else
	Cvar_Register(&rcon_level, "Access controls");		//server gains versatility.
#endif
	rcon_level.restriction = RESTRICT_MAX;	//default. Don't let anyone change this too easily.
	cmd_maxbuffersize.restriction = RESTRICT_MAX;	//filling this causes a loop for quite some time.

	Cvar_Register(&cl_aliasoverlap, "Console");
	//FIXME: go through quake.rc and parameters looking for sets and setas and setting them now.
}

