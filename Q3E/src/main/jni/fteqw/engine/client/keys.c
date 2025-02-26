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
#include "quakedef.h"
#ifdef _WIN32
#include "winquake.h"
#endif
#include "shader.h"
/*

key up events are sent even if in console mode

*/
qboolean Editor_Key(int key, int unicode);
void Key_ConsoleInsert(const char *instext);
void Key_ClearTyping (void);

unsigned char	*key_lines[CON_EDIT_LINES_MASK+1];
int		key_linepos;
int		shift_down=false;

int		edit_line=0;
int		history_line=0;

unsigned int key_dest_mask;
qboolean key_dest_console;
unsigned int key_dest_absolutemouse;

struct key_cursor_s key_customcursor[kc_max];

int		key_bindmaps[2];
char	*keybindings[K_MAX][KEY_MODIFIERSTATES];
qbyte	bindcmdlevel[K_MAX][KEY_MODIFIERSTATES];	//should be a struct, but not due to 7 bytes wasted per on 64bit machines
qboolean	consolekeys[K_MAX];	// if true, can't be rebound while in console
int		keyshift[K_MAX];		// key to map to if shift held down in console
unsigned int	keydown[K_MAX];	//	bitmask, for each device (to block autorepeat binds per-seat).

#define MAX_INDEVS 8

char *releasecommand[K_MAX][MAX_INDEVS];	//this is the console command to be invoked when the key is released. should free it.
qbyte releasecommandlevel[K_MAX][MAX_INDEVS];	//and this is the cbuf level it is to be run at.

extern cvar_t con_displaypossibilities;
cvar_t con_echochat = CVAR("con_echochat", "0");
extern cvar_t cl_chatmode;

static int KeyModifier (unsigned int shift, unsigned int alt, unsigned int ctrl, unsigned int devbit)
{
	int stateset = 0;
	if (shift&devbit)
		stateset |= KEY_MODIFIER_SHIFT;
	if (alt&devbit)
		stateset |= KEY_MODIFIER_ALT;
	if (ctrl&devbit)
		stateset |= KEY_MODIFIER_CTRL;

	return stateset;
}

void Key_GetBindMap(int *bindmaps)
{
	int i;
	for (i = 0; i < countof(key_bindmaps); i++)
	{
		if (key_bindmaps[i])
			bindmaps[i] = (key_bindmaps[i]&~KEY_MODIFIER_ALTBINDMAP) + 1;
		else
			bindmaps[i] = 0;
	}
}

void Key_SetBindMap(int *bindmaps)
{
	int i;
	for (i = 0; i < countof(key_bindmaps); i++)
	{
		if (bindmaps[i] > 0 && bindmaps[i] <= KEY_MODIFIER_ALTBINDMAP)
			key_bindmaps[i] = (bindmaps[i]-1)|KEY_MODIFIER_ALTBINDMAP;
		else
			key_bindmaps[i] = 0;
	}
}

typedef struct
{
	char	*name;
	int		keynum;
} keyname_t;

keyname_t keynames[] =
{
	{"TAB",			K_TAB},
	{"ENTER",		K_ENTER},
	{"RETURN",		K_ENTER},
	{"ESCAPE",		K_ESCAPE},
	{"SPACE",		K_SPACE},
	{"BACKSPACE",	K_BACKSPACE},
	{"UPARROW",		K_UPARROW},
	{"DOWNARROW",	K_DOWNARROW},
	{"LEFTARROW",	K_LEFTARROW},
	{"RIGHTARROW",	K_RIGHTARROW},

	{"LALT",	K_LALT},
	{"RALT",	K_RALT},
	{"LCTRL",	K_LCTRL},
	{"RCTRL",	K_RCTRL},
	{"LSHIFT",	K_LSHIFT},
	{"RSHIFT",	K_RSHIFT},
	{"ALT",		K_ALT},		//depricated name
	{"CTRL",	K_CTRL},	//depricated name
	{"SHIFT",	K_SHIFT},	//depricated name
	
	{"F1",		K_F1},
	{"F2",		K_F2},
	{"F3",		K_F3},
	{"F4",		K_F4},
	{"F5",		K_F5},
	{"F6",		K_F6},
	{"F7",		K_F7},
	{"F8",		K_F8},
	{"F9",		K_F9},
	{"F10",		K_F10},
	{"F11",		K_F11},
	{"F12",		K_F12},

	{"INS",		K_INS},
	{"DEL",		K_DEL},
	{"PGDN",	K_PGDN},
	{"PGUP",	K_PGUP},
	{"HOME",	K_HOME},
	{"END",		K_END},

	
	{"KP_HOME",		K_KP_HOME},
	{"KP_UPARROW",	K_KP_UPARROW},
	{"KP_PGUP",		K_KP_PGUP},
	{"KP_LEFTARROW", K_KP_LEFTARROW},
	{"KP_5",		K_KP_5},
	{"KP_RIGHTARROW", K_KP_RIGHTARROW},
	{"KP_END",		K_KP_END},
	{"KP_DOWNARROW",	K_KP_DOWNARROW},
	{"KP_PGDN",		K_KP_PGDN},
	{"KP_ENTER",	K_KP_ENTER},
	{"KP_INS",		K_KP_INS},
	{"KP_DEL",		K_KP_DEL},
	{"KP_SLASH",	K_KP_SLASH},
	{"KP_MINUS",	K_KP_MINUS},
	{"KP_PLUS",		K_KP_PLUS},
	{"KP_NUMLOCK",	K_KP_NUMLOCK},
	{"KP_STAR",		K_KP_STAR},
	{"KP_MULTIPLY",	K_KP_STAR},
	{"KP_EQUALS",	K_KP_EQUALS},

	//fuhquake compatible.
	{"KP_0",		K_KP_INS},
	{"KP_1",		K_KP_END},
	{"KP_2",		K_KP_DOWNARROW},
	{"KP_3",		K_KP_PGDN},
	{"KP_4",		K_KP_LEFTARROW},
	{"KP_6",		K_KP_RIGHTARROW},
	{"KP_7",		K_KP_HOME},
	{"KP_8",		K_KP_UPARROW},
	{"KP_9",		K_KP_PGUP},
	//dp compat
	{"KP_PERIOD",	K_KP_DEL},
	{"KP_DIVIDE",	K_KP_SLASH},
	{"NUMLOCK",		K_KP_NUMLOCK},



	{"MOUSE1",		K_MOUSE1},
	{"MOUSE2",		K_MOUSE2},
	{"MOUSE3",		K_MOUSE3},
	{"MOUSE4",		K_MOUSE4},
	{"MOUSE5",		K_MOUSE5},
	{"MOUSE6",		K_MOUSE6},
	{"MOUSE7",		K_MOUSE7},
	{"MOUSE8",		K_MOUSE8},
	{"MOUSE9",		K_MOUSE9},
	{"MOUSE10",		K_MOUSE10},
	{"MWHEELUP",	K_MWHEELUP},
	{"MWHEELDOWN",	K_MWHEELDOWN},
	{"MWHEELLEFT",	K_MWHEELLEFT},
	{"MWHEELRIGHT",	K_MWHEELRIGHT},
	{"TOUCH",		K_TOUCH},
	{"TOUCHSLIDE",	K_TOUCHSLIDE},
	{"TOUCHTAP",	K_TOUCHTAP},
	{"TOUCHLONG",	K_TOUCHLONG},

	{"LWIN",	K_LWIN},	//windows name
	{"RWIN",	K_RWIN},	//windows name
	{"WIN",		K_WIN},		//depricated
	{"RCOMMAND",K_RWIN},	//mac name
	{"LCOMMAND",K_LWIN},	//mac name
	{"COMMAND",	K_WIN},		//quakespasm(mac) compat
	{"LMETA",	K_LWIN},	//linux name
	{"RMETA",	K_RWIN},	//linux name
	{"APP",		K_APP},
	{"MENU",	K_APP},
	{"SEARCH",	K_SEARCH},
	{"POWER",	K_POWER},
	{"VOLUP",	K_VOLUP},
	{"VOLDOWN",	K_VOLDOWN},

	{"JOY1",	K_JOY1},
	{"JOY2",	K_JOY2},
	{"JOY3",	K_JOY3},
	{"JOY4",	K_JOY4},
	{"JOY5",	K_JOY5},
	{"JOY6",	K_JOY6},
	{"JOY7",	K_JOY7},
	{"JOY8",	K_JOY8},
	{"JOY9",	K_JOY9},
	{"JOY10",	K_JOY10},
	{"JOY11",	K_JOY11},
	{"JOY12",	K_JOY12},
	{"JOY13",	K_JOY13},
	{"JOY14",	K_JOY14},
	{"JOY15",	K_JOY15},
	{"JOY16",	K_JOY16},
	{"JOY17",	K_JOY17},
	{"JOY18",	K_JOY18},
	{"JOY19",	K_JOY19},
	{"JOY20",	K_JOY20},
	{"JOY21",	K_JOY21},
	{"JOY22",	K_JOY22},
	{"JOY23",	K_JOY23},
	{"JOY24",	K_JOY24},
	{"JOY25",	K_JOY25},
	{"JOY26",	K_JOY26},
	{"JOY27",	K_JOY27},
	{"JOY28",	K_JOY28},
	{"JOY29",	K_JOY29},
	{"JOY30",	K_JOY30},
	{"JOY31",	K_JOY31},
	{"JOY32",	K_JOY32},

	{"AUX1",	K_AUX1},
	{"AUX2",	K_AUX2},
	{"AUX3",	K_AUX3},
	{"AUX4",	K_AUX4},
	{"AUX5",	K_AUX5},
	{"AUX6",	K_AUX6},
	{"AUX7",	K_AUX7},
	{"AUX8",	K_AUX8},
	{"AUX9",	K_AUX9},
	{"AUX10",	K_AUX10},
	{"AUX11",	K_AUX11},
	{"AUX12",	K_AUX12},
	{"AUX13",	K_AUX13},
	{"AUX14",	K_AUX14},
	{"AUX15",	K_AUX15},
	{"AUX16",	K_AUX16},

	{"PAUSE",		K_PAUSE},

	{"PRINTSCREEN",	K_PRINTSCREEN},
	{"CAPSLOCK",	K_CAPSLOCK},
	{"SCROLLLOCK",	K_SCRLCK},

	{"SEMICOLON",	';'},	// because a raw semicolon seperates commands
	{"PLUS",		'+'},	// because "shift++" is inferior to shift+plus
	{"MINUS",		'-'},	// because "shift+-" is inferior to shift+minus

	{"APOSTROPHE",	'\''},	//can mess up string parsing, unfortunately
	{"QUOTES",		'\"'},	//can mess up string parsing, unfortunately
	{"TILDE",		'~'},
	{"BACKQUOTE",	'`'},
	{"BACKSLASH",	'\\'},

	{"GP_A",			K_GP_A},	//note: xbox arrangement, not nintendo arrangement.
	{"GP_B",			K_GP_B},
	{"GP_X",			K_GP_X},
	{"GP_Y",			K_GP_Y},
	{"GP_LSHOULDER",	K_GP_LEFT_SHOULDER},
	{"GP_RSHOULDER",	K_GP_RIGHT_SHOULDER},
	{"GP_LTRIGGER",		K_GP_LEFT_TRIGGER},
	{"GP_RTRIGGER",		K_GP_RIGHT_TRIGGER},
	{"GP_VIEW",			K_GP_VIEW},
	{"GP_MENU",			K_GP_MENU},
	{"GP_LTHUMB",		K_GP_LEFT_STICK},
	{"GP_RTHUMB",		K_GP_RIGHT_STICK},
	{"GP_DPAD_UP",		K_GP_DPAD_UP},
	{"GP_DPAD_DOWN",	K_GP_DPAD_DOWN},
	{"GP_DPAD_LEFT",	K_GP_DPAD_LEFT},
	{"GP_DPAD_RIGHT",	K_GP_DPAD_RIGHT},
	{"GP_GUIDE",		K_GP_GUIDE},
	{"GP_SHARE",		K_GP_MISC1},
	{"GP_PADDLE1",		K_GP_PADDLE1},
	{"GP_PADDLE2",		K_GP_PADDLE2},
	{"GP_PADDLE3",		K_GP_PADDLE3},
	{"GP_PADDLE4",		K_GP_PADDLE4},
	{"GP_TOUCHPAD",		K_GP_TOUCHPAD},
	{"GP_UNKNOWN",		K_GP_UNKNOWN},

	//older xbox names
	{"GP_BACK",			K_GP_BACK},
	{"GP_START",		K_GP_START},
	//names for playstation controllers
	{"GP_CROSS",		K_GP_PS_CROSS},
	{"GP_CIRCLE",		K_GP_PS_CIRCLE},
	{"GP_SQUARE",		K_GP_PS_SQUARE},
	{"GP_TRIANGLE",		K_GP_PS_TRIANGLE},
	{"GP_MIC",			K_GP_MISC1},
	{"GP_SELECT",		K_GP_VIEW},
	{"GP_SHARE",		K_GP_VIEW},
	{"GP_OPTIONS",		K_GP_START},

	//axis->button emulation
	{"GP_LTHUMB_UP",	K_GP_LEFT_THUMB_UP},
	{"GP_LTHUMB_DOWN",	K_GP_LEFT_THUMB_DOWN},
	{"GP_LTHUMB_LEFT",	K_GP_LEFT_THUMB_LEFT},
	{"GP_LTHUMB_RIGHT",	K_GP_LEFT_THUMB_RIGHT},
	{"GP_RTHUMB_UP",	K_GP_RIGHT_THUMB_UP},
	{"GP_RTHUMB_DOWN",	K_GP_RIGHT_THUMB_DOWN},
	{"GP_RTHUMB_LEFT",	K_GP_RIGHT_THUMB_LEFT},
	{"GP_RTHUMB_RIGHT",	K_GP_RIGHT_THUMB_RIGHT},

#ifdef Q2BSPS
	//kingpin compat
	{"ESC",				K_ESCAPE},
	{"B_SPACE",			K_BACKSPACE},
	{"U_ARROW",			K_UPARROW},
	{"D_ARROW",			K_DOWNARROW},
	{"L_ARROW",			K_LEFTARROW},
	{"R_ARROW",			K_RIGHTARROW},
#endif

#ifndef QUAKETC
	//dp compat
	{"X360_DPAD_UP",			K_GP_DPAD_UP},
	{"X360_DPAD_DOWN",			K_GP_DPAD_DOWN},
	{"X360_DPAD_LEFT",			K_GP_DPAD_LEFT},
	{"X360_DPAD_RIGHT",			K_GP_DPAD_RIGHT},
	{"X360_START",				K_GP_START},
	{"X360_BACK",				K_GP_BACK},
	{"X360_LEFT_THUMB",			K_GP_LEFT_STICK},
	{"X360_RIGHT_THUMB",		K_GP_RIGHT_STICK},
	{"X360_LEFT_SHOULDER",		K_GP_LEFT_SHOULDER},
	{"X360_RIGHT_SHOULDER",		K_GP_RIGHT_SHOULDER},
	{"X360_A",					K_GP_A},
	{"X360_B",					K_GP_B},
	{"X360_X",					K_GP_X},
	{"X360_Y",					K_GP_Y},
	{"X360_LEFT_TRIGGER",		K_GP_LEFT_TRIGGER},
	{"X360_RIGHT_TRIGGER",		K_GP_RIGHT_TRIGGER},
	{"X360_LEFT_THUMB_UP",		K_GP_LEFT_THUMB_UP},
	{"X360_LEFT_THUMB_DOWN",	K_GP_LEFT_THUMB_DOWN},
	{"X360_LEFT_THUMB_LEFT",	K_GP_LEFT_THUMB_LEFT},
	{"X360_LEFT_THUMB_RIGHT",	K_GP_LEFT_THUMB_RIGHT},
	{"X360_RIGHT_THUMB_UP",		K_GP_RIGHT_THUMB_UP},
	{"X360_RIGHT_THUMB_DOWN",	K_GP_RIGHT_THUMB_DOWN},
	{"X360_RIGHT_THUMB_LEFT",	K_GP_RIGHT_THUMB_LEFT},
	{"X360_RIGHT_THUMB_RIGHT",	K_GP_RIGHT_THUMB_RIGHT},

	//quakespasm compat
	{"LTHUMB",					K_GP_LEFT_STICK},
	{"RTHUMB",					K_GP_RIGHT_STICK},
	{"LSHOULDER",				K_GP_LEFT_SHOULDER},
	{"RSHOULDER",				K_GP_RIGHT_SHOULDER},
	{"ABUTTON",					K_GP_A},
	{"BBUTTON",					K_GP_B},
	{"XBUTTON",					K_GP_X},
	{"YBUTTON",					K_GP_Y},
	{"LTRIGGER",				K_GP_LEFT_TRIGGER},
	{"RTRIGGER",				K_GP_RIGHT_TRIGGER},
#endif

	/* Steam Controller */
	{"SC_LPADDLE",				K_JOY16},
	{"SC_RPADDLE",				K_JOY17},

	{NULL,			0}
};

#if defined(CSQC_DAT) || defined(MENU_DAT)
int MP_TranslateFTEtoQCCodes(keynum_t code);
void Key_PrintQCDefines(vfsfile_t *f, qboolean defines)
{
	int i, j;
	for (i = 0; keynames[i].name; i++)
	{
		for (j = 0; j < i; j++)
			if (keynames[j].keynum == keynames[i].keynum)
				break;
		if (j == i)
		{
			if (defines)
				VFS_PRINTF(f, "#define K_%s\t%i\n", keynames[i].name, MP_TranslateFTEtoQCCodes(keynames[j].keynum));
			else
				VFS_PRINTF(f, "const float K_%s = %i;\n", keynames[i].name, MP_TranslateFTEtoQCCodes(keynames[j].keynum));
		}
	}
}
#endif

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

qboolean Cmd_IsCommand (const char *line)
{
	char	command[128];
	const char	*cmd, *s;
	int		i;

	s = line;

	for (i=0 ; i<127 ; i++)
		if (s[i] <= ' ' || s[i] == ';')
			break;
		else
			command[i] = s[i];
	command[i] = 0;

	cmd = Cmd_CompleteCommand (command, true, false, -1, NULL);
	if (!cmd  || strcmp (cmd, command) )
		return false;		// just a chat message
	return true;
}

#define COLUMNWIDTH 20
#define MINCOLUMNWIDTH 18

int PaddedPrint (char *s, int x)
{
	Con_Printf ("^4%s\t", s);
	x+=strlen(s);

	return x;
}

int con_commandmatch;
void Key_UpdateCompletionDesc(void)
{
	const char *desc;
	cmd_completion_t *c;
	const char *s = key_lines[edit_line];
	if (*s == ' ' || *s == '\t')
		s++;
	if (*s == '\\' || *s == '/')
		s++;
	if (*s == ' ' || *s == '\t')
		s++;

	c = Cmd_Complete(s, true);
	if (!con_commandmatch || con_commandmatch > c->num)
		con_commandmatch = 1;

	if (con_commandmatch <= c->num)
	{
		const char *cmd;
		cvar_t *var;
		if (c->completions[con_commandmatch-1].repl)
			cmd = c->completions[con_commandmatch-1].repl;
		else
			cmd = c->completions[con_commandmatch-1].text;
		desc = c->completions[con_commandmatch-1].desc;
		var = Cvar_FindVar(cmd);
		if (var)
		{
			if (desc)
				Con_Footerf(NULL, false, "%s %s\n%s", cmd, var->string, localtext(desc));
			else
				Con_Footerf(NULL, false, "%s %s", cmd, var->string);
		}
		else
		{
			if (desc)
				Con_Footerf(NULL, false, "%s: %s", cmd, localtext(desc));
			else
				Con_Footerf(NULL, false, "");
		}
	}
	else
	{
		Con_Footerf(NULL, false, "");
		if (con_commandmatch)
			con_commandmatch = 1;
	}
}

void CompleteCommand (qboolean force, int direction)
{
	const char	*cmd, *s;
	const char *desc;
	cmd_completion_t *c;

	s = key_lines[edit_line];
	if (!*s)
		return;
	if (*s == ' ' || *s == '\t')
		s++;
	if (*s == '\\' || *s == '/')
		s++;
	if (*s == ' ' || *s == '\t')
		s++;

	//check for singular matches and complete if found
	c = Cmd_Complete(s, true);
	if (c->num == 1 || force)
	{
		desc = NULL;
		if (!force && c->num != 1)
			cmd = c->guessed;
		else if (c->num)
		{
			int idx = max(0, con_commandmatch-1);
			if (c->completions[idx].repl)
				cmd = c->completions[idx].repl;
			else
				cmd = c->completions[idx].text;
		}
		else
			cmd = NULL;
		if (cmd)
		{
			if (strlen(cmd) < strlen(s))
				return;

			//complete to that (maybe partial) cmd.
			Key_ClearTyping();
			if (cl_chatmode.ival)
				Key_ConsoleInsert("/");
			Key_ConsoleInsert(cmd);
			s = key_lines[edit_line];
			if (*s == '/')
				s++;

			//if its the only match, add a space ready for arguments.
			c = Cmd_Complete(s, true);
			cmd = ((c->num >= 1)?(c->completions[0].repl?c->completions[0].repl:c->completions[0].text):NULL);
			desc = ((c->num >= 1)?c->completions[0].desc:NULL);
			if (c->num == 1)
				Key_ConsoleInsert(" ");

			if (!con_commandmatch)
				con_commandmatch = 1;

			if (desc)
				Con_Footerf(con_current, false, "%s: %s", cmd, desc);
			else
				Con_Footerf(con_current, false, "");
			return;
		}
	}
	//complete to a partial match.
	cmd = c->guessed;
	if (cmd)
	{
		int i = key_lines[edit_line][0] == '/'?1:0;
		if (i != 1 || strcmp(key_lines[edit_line]+i, cmd))
		{	//if successful, use that instead.
			Key_ClearTyping();
			if (cl_chatmode.ival)
				Key_ConsoleInsert("/");
			Key_ConsoleInsert(cmd);

			s = key_lines[edit_line];	//readjust to cope with the insertion of a /
			if (*s == '\\' || *s == '/')
				s++;
		}
	}

	con_commandmatch += direction;
	if (con_commandmatch <= 0)
		con_commandmatch += c->num;
	Key_UpdateCompletionDesc();
}

int Con_Navigate(console_t *con, const char *line)
{
	if (con->backshader)
	{
#ifdef HAVE_MEDIA_DECODER
		cin_t *cin = R_ShaderGetCinematic(con->backshader);
		if (cin)
		{
			Media_Send_Command(cin, line);
		}
#endif
	}
	con->linebuffered = NULL;
	return 2;
}

//lines typed at the main console enter here
int Con_ExecuteLine(console_t *con, const char *line)
{
	qboolean waschat = false;
	char *deutf8 = NULL;
	if (com_parseutf8.ival <= 0)
	{
		unsigned int unicode;
		int err;
		int len = 0;
		int maxlen = strlen(line)*6+1;
		deutf8 = malloc(maxlen);
		while(*line)
		{
			unicode = utf8_decode(&err, line, &line);
			len += unicode_encode(deutf8+len, unicode, maxlen-1 - len, true);
		}
		deutf8[len] = 0;
		line = deutf8;
	}

	if (con_commandmatch)
		con_commandmatch=1;
	Con_Footerf(con, false, "");

	if (cls.state >= ca_connected && cl_chatmode.value == 2)
	{
		waschat = true;
		if (keydown[K_CTRL])
			Cbuf_AddText ("say_team ", RESTRICT_LOCAL);
		else if (keydown[K_SHIFT] || *line == ' ')
			Cbuf_AddText ("say ", RESTRICT_LOCAL);
		else
			waschat = false;
	}
	while (*line == ' ')
		line++;
	if (waschat)
		Cbuf_AddText (line, RESTRICT_LOCAL);
	else
	{
		const char *exec = NULL;
		if (line[0] == '\\' || line[0] == '/')
			exec = line+1;	// skip the slash
		else if (cl_chatmode.value == 2 && Cmd_IsCommand(line))
			exec = line;	// valid command
	#ifdef Q2CLIENT
		else if (cls.protocol == CP_QUAKE2)
			exec = line;	// send the command to the server via console, and let the server convert to chat
	#endif
		else if (*line)
		{	// convert to a chat message
			if ((cl_chatmode.value == 1 || ((cls.state >= ca_connected && cl_chatmode.value == 2) && (strncmp(line, "say ", 4)))))
			{
				if (keydown[K_CTRL])
					Cbuf_AddText ("say_team ", RESTRICT_LOCAL);
				else
					Cbuf_AddText ("say ", RESTRICT_LOCAL);
				waschat = true;
				Cbuf_AddText (line, RESTRICT_LOCAL);
			}
			else
				exec = line;	//exec it anyway. let the cbuf give the error message in case its 'INVALID;VALID'
		}

		if (exec)
		{
#ifdef TEXTEDITOR
			if (editormodal)
			{
				char cvarname[128];
				COM_ParseOut(exec, cvarname, sizeof(cvarname));
				if (Cvar_FindVar(cvarname) && !strchr(line, ';') && !strchr(line, '\n'))
				{
					Con_Printf ("]%s\n",line);
					Cmd_ExecuteString(exec, RESTRICT_SERVER);
					free(deutf8);
					return true;
				}

				Con_Footerf(con, false, "Commands cannot be execed while debugging QC");
			}
#endif
			Cbuf_AddText (exec, RESTRICT_LOCAL);
		}
	}

	Cbuf_AddText ("\n", RESTRICT_LOCAL);
	if (!waschat || con_echochat.value)
	{
		Con_Printf ("%s", con->prompt);
		Con_Printf ("%s\n",line);
	}

//	if (cls.state == ca_disconnected)
//		SCR_UpdateScreen ();	// force an update, because the command
//									// may take some time

	free(deutf8);
	return true;
}

qboolean Key_GetConsoleSelectionBox(console_t *con, int *sx, int *sy, int *ex, int *ey)
{
	*sx = *sy = *ex = *ey = 0;

	if (con->buttonsdown == CB_SCROLL || con->buttonsdown == CB_SCROLL_R)
	{
		float lineheight = Font_CharVHeight(font_console);
		//left-mouse.
		//scroll the console with the mouse. trigger links on release.
		con->displayscroll += (con->mousecursor[1] - con->mousedown[1])/lineheight;
		con->mousedown[1] = con->mousecursor[1];
		while (con->displayscroll > con->display->numlines)
		{
			if (con->display->older)
			{
				con->displayscroll -= con->display->numlines;
				con->display = con->display->older;
			}
			else
			{
				con->displayscroll = con->display->numlines;
				break;
			}
		}
		while (con->displayscroll <= 0)
		{
			if (con->display->newer)
			{
				con->display = con->display->newer;
				con->displayscroll += con->display->numlines;
			}
			else
			{
				con->displayscroll = 0;
				break;
			}
		}
		/*
		while (con->mousecursor[1] - con->mousedown[1] > 8 && con->display->older)
		{
			con->mousedown[1] += 8;
			con->display = con->display->older;
		}
		while (con->mousecursor[1] - con->mousedown[1] < -8 && con->display->newer)
		{
			con->mousedown[1] -= 8;
			con->display = con->display->newer;
		}
*/
		*sx = con->mousecursor[0];
		*sy = con->mousecursor[1];
		*ex = con->mousecursor[0];
		*ey = con->mousecursor[1];
		return true;
	}
	else if (con->buttonsdown == CB_SELECT || con->buttonsdown == CB_SELECTED || con->buttonsdown == CB_TAPPED)
	{
		//right-mouse
		//select. copy-to-clipboard on release.
		*sx = con->mousedown[0];
		*sy = con->mousedown[1];
		*ex = con->mousecursor[0];
		*ey = con->mousecursor[1];
		return true;
	}
	else
	{
		if (con_curwindow == con && con->buttonsdown)
		{
			if (con->buttonsdown == CB_MOVE)
			{	//move window to track the cursor
				con->wnd_x += con->mousecursor[0] - con->mousedown[0];
		//		con->mousedown[0] = con->mousecursor[0];
				con->wnd_y += con->mousecursor[1] - con->mousedown[1];
		//		con->mousedown[1] = con->mousecursor[1];
			}
			if (con->buttonsdown & CB_SIZELEFT)
			{
				if (con->wnd_w - (con->mousecursor[0] - con->mousedown[0]) >= 64)
				{
					con->wnd_w -= con->mousecursor[0] - con->mousedown[0];
					con->wnd_x += con->mousecursor[0] - con->mousedown[0];
				}
			}
			if (con->buttonsdown & CB_SIZERIGHT)
			{
				if (con->wnd_w + (con->mousecursor[0] - con->mousedown[0]) >= 64)
				{
					con->wnd_w += con->mousecursor[0] - con->mousedown[0];
					con->mousedown[0] = con->mousecursor[0];
				}
			}
			if (con->buttonsdown & CB_SIZEBOTTOM)
			{
				if (con->wnd_h + (con->mousecursor[1] - con->mousedown[1]) >= 64)
				{
					con->wnd_h += con->mousecursor[1] - con->mousedown[1];
					con->mousedown[1] = con->mousecursor[1];
				}
			}
		}
		else
			con->buttonsdown = CB_NONE;

		*sx = con->mousecursor[0];
		*sy = con->mousecursor[1];
		*ex = con->mousecursor[0];
		*ey = con->mousecursor[1];
		return false;
	}
}

/*insert the given text at the console input line at the current cursor pos*/
void Key_ConsoleInsert(const char *instext)
{
	int i;
	int len, olen;
	char *old;
	if (!*instext)
		return;

	old = key_lines[edit_line];
	len = strlen(instext);
	olen = strlen(old);
	key_lines[edit_line] = BZ_Malloc(olen + len + 1);
	memcpy(key_lines[edit_line], old, key_linepos);
	memcpy(key_lines[edit_line]+key_linepos, instext, len);
	memcpy(key_lines[edit_line]+key_linepos+len, old+key_linepos, olen - key_linepos+1);
	Z_Free(old);
	for (i = key_linepos; i < key_linepos+len; i++)
	{
		if (key_lines[edit_line][i] == '\r')
			key_lines[edit_line][i] = ' ';
		else if (key_lines[edit_line][i] == '\n')
			key_lines[edit_line][i] = ';';
	}
	key_linepos += len;
}
void Key_ConsoleReplace(const char *instext)
{
	if (!*instext)
		return;

	key_linepos = 0;
	key_lines[edit_line][key_linepos] = 0;
	Key_ConsoleInsert(instext);
}

void Key_DefaultLinkClicked(console_t *con, char *text, char *info)
{
	char *c;
	/*the engine supports specific default links*/
	/*we don't support everything. a: there's no point. b: unbindall links are evil.*/
	c = Info_ValueForKey(info, "player");
	if (*c)
	{
		unsigned int player = atoi(c);
		int i;
		if (player >= cl.allocated_client_slots || !*cl.players[player].name)
			return;

		c = Info_ValueForKey(info, "action");
		if (*c)
		{
			if (!strcmp(c, "mute"))
			{
				if (!cl.players[player].vignored)
				{
					cl.players[player].vignored = true;
					Con_Printf("^[%s\\player\\%i^] muted\n", cl.players[player].name, player);
				}
				else
				{
					cl.players[player].vignored = false;
					Con_Printf("^[%s\\player\\%i^] unmuted\n", cl.players[player].name, player);
				}
			}
			else if (!strcmp(c, "ignore"))
			{
				if (!cl.players[player].ignored)
				{
					cl.players[player].ignored = true;
					cl.players[player].vignored = true;
					Con_Printf("^[%s\\player\\%i^] ignored\n", cl.players[player].name, player);
				}
				else
				{
					cl.players[player].ignored = false;
					cl.players[player].vignored = false;
					Con_Printf("^[%s\\player\\%i^] unignored\n", cl.players[player].name, player);
				}
			}
			else if (!strcmp(c, "spec"))
			{
				Cam_TrackPlayer(0, "spectate", cl.players[player].name);
			}
			else if (!strcmp(c, "kick"))
			{
#ifndef CLIENTONLY
				if (sv.active)
				{
					//use the q3 command, because we can.
					Cbuf_AddText(va("\nclientkick %i\n", player), RESTRICT_LOCAL);
				}
				else
#endif
					Cbuf_AddText(va("\nrcon kick %s\n", cl.players[player].name), RESTRICT_LOCAL);
			}
			else if (!strcmp(c, "ban"))
			{
#ifndef CLIENTONLY
				if (sv.active)
				{
					//use the q3 command, because we can.
					Cbuf_AddText(va("\nbanname %s QuickBan\n", cl.players[player].name), RESTRICT_LOCAL);
				}
				else
#endif
					Cbuf_AddText(va("\nrcon banname %s QuickBan\n", cl.players[player].name), RESTRICT_LOCAL);
			}
			return;
		}
		if (!con)
			return;	//can't do footers

		Con_Footerf(con, false, "^m#^m ^[%s\\player\\%i^]: %if %ims", cl.players[player].name, player, cl.players[player].frags, cl.players[player].ping);

		for (i = 0; i < cl.splitclients; i++)
		{
			if (cl.playerview[i].playernum == player)
				break;
		}
		if (i == cl.splitclients)
		{
			extern cvar_t rcon_password;
			if (*cl.players[player].ip)
				Con_Footerf(con, true, "\n%s", cl.players[player].ip);

			if (cl.playerview[0].spectator || cls.demoplayback==DPB_MVD)
			{
				//we're spectating, or an mvd
				Con_Footerf(con, true, " ^[Spectate\\player\\%i\\action\\spec^]", player);
			}
			else
			{
				//we're playing.
				if (cls.protocol == CP_QUAKEWORLD && strcmp(cl.players[cl.playerview[0].playernum].team, cl.players[player].team))
					Con_Footerf(con, true, " ^[[Join Team %s]\\cmd\\setinfo team %s^]", cl.players[player].team, cl.players[player].team);
			}
			Con_Footerf(con, true, " ^[%sgnore\\player\\%i\\action\\ignore^]", cl.players[player].ignored?"Uni":"I", player);
	//		if (cl_voip_play.ival)
				Con_Footerf(con, true, " ^[%sute\\player\\%i\\action\\mute^]", cl.players[player].vignored?"Unm":"M",  player);

			if (!cls.demoplayback && (*rcon_password.string
#ifndef CLIENTONLY
				|| (sv.state && svs.clients[player].netchan.remote_address.type != NA_LOOPBACK)
#endif
				))
			{
				Con_Footerf(con, true, " ^[Kick\\player\\%i\\action\\kick^]", player);
				Con_Footerf(con, true, " ^[Ban\\player\\%i\\action\\ban^]", player);
			}
		}
		else
		{
			char cmdprefix[6];
//			if (i == 0)
				*cmdprefix = 0;
//			else
//				snprintf(cmdprefix, sizeof(cmdprefix), "%i ", i+1);

			//hey look! its you!

			if (i || cl.playerview[i].spectator || cls.demoplayback)
			{
				//need join option here or something
			}
			else
			{
				Con_Footerf(con, true, " ^[Suicide\\cmd\\%skill^]", cmdprefix);
	#ifndef CLIENTONLY
				if (!sv.state)
					Con_Footerf(con, true, " ^[Disconnect\\cmd\\disconnect^]");
				if (cls.allow_cheats || (sv.state && sv.allocated_client_slots == 1))
	#else
				Con_Footerf(con, true, " ^[Disconnect\\cmd\\disconnect^]");
				if (cls.allow_cheats)
	#endif
				{
					Con_Footerf(con, true, " ^[Noclip\\cmd\\%snoclip^]", cmdprefix);
					Con_Footerf(con, true, " ^[Fly\\cmd\\%sfly^]", cmdprefix);
					Con_Footerf(con, true, " ^[God\\cmd\\%sgod^]", cmdprefix);
					Con_Footerf(con, true, " ^[Give\\impulse\\9^]");
				}
			}
		}
		return;
	}
	c = Info_ValueForKey(info, "menu");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nmenu_cmd conlink %s\n", c), RESTRICT_LOCAL);
		return;
	}
	c = Info_ValueForKey(info, "connect");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nconnect \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
	c = Info_ValueForKey(info, "join");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\njoin \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
	/*c = Info_ValueForKey(info, "url");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nplayfilm %s\n", c), RESTRICT_LOCAL);
		return;
	}*/
	c = Info_ValueForKey(info, "observe");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nobserve \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
	c = Info_ValueForKey(info, "qtv");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nqtvplay \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
	c = Info_ValueForKey(info, "demo");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nplaydemo \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
#ifndef CLIENTONLY
	c = Info_ValueForKey(info, "map");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nmap \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
#endif
#ifndef NOBUILTINMENUS
	c = Info_ValueForKey(info, "modelviewer");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nmodelviewer \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
#endif
	c = Info_ValueForKey(info, "type");
	if (*c)
	{
		Key_ConsoleReplace(c);
		return;
	}
	c = Info_ValueForKey(info, "cmd");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\ncmd %s\n", c), RESTRICT_LOCAL);
		return;
	}
	c = Info_ValueForKey(info, "say");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nsay %s\n", c), RESTRICT_LOCAL);
		return;
	}
	c = Info_ValueForKey(info, "echo");
	if (*c)
	{
		Con_Printf("%s\n", c);
		return;
	}
	c = Info_ValueForKey(info, "dir");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\necho Contents of %s:\ndir \"%s\"\n", c, c), RESTRICT_LOCAL);
		return;
	}
#ifdef TEXTEDITOR
	c = Info_ValueForKey(info, "edit");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nedit \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
#endif
#ifdef SUBSERVERS
	c = Info_ValueForKey(info, "ssv");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nssv \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
#endif
#ifdef SUPPORT_ICE
	c = Info_ValueForKey(info, "ice");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nnet_ice_show \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
#endif
	c = Info_ValueForKey(info, "impulse");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nimpulse %s\n", c), RESTRICT_LOCAL);
		return;
	}
#ifdef HAVE_MEDIA_DECODER
	c = Info_ValueForKey(info, "film");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		Cbuf_AddText(va("\nplayfilm \"%s\"\n", c), RESTRICT_LOCAL);
		return;
	}
#endif
	c = Info_ValueForKey(info, "playaudio");
	if (*c && !strchr(c, ';') && !strchr(c, '\n'))
	{
		S_StartSound(0, 0x7ffffffe, S_PrecacheSound(c), NULL, NULL, 1, ATTN_NONE, 0, 0, CF_NOSPACIALISE);
		return;
	}
	c = Info_ValueForKey(info, "desc");
	if (*c)
	{
		if (con)
			Con_Footerf(con, false, "%s", c);
		return;
	}

	//if there's no info and the text starts with a leading / then insert it as a suggested/completed console command
	//skip any leading colour code.
	if (text[0] == '^' && text[1] >= '0' && text[1] <= '9')
		text+=2;
	if (*text == '/')
	{
		int tlen;
		if (!cl_chatmode.ival)
			text++;	//NQ should generally not bother with the / in commands.
		tlen = info - text;
		Z_Free(key_lines[edit_line]);
		key_lines[edit_line] = BZ_Malloc(tlen + 1);
		memcpy(key_lines[edit_line], text, tlen);
		key_lines[edit_line][tlen] = 0;
		key_linepos = strlen(key_lines[edit_line]);

		Key_UpdateCompletionDesc();
		return;
	}
}

void Key_HandleConsoleLink(console_t *con, char *buffer)
{
	if (!buffer)
		return;
	if (buffer[0] == '^' && buffer[1] == '[')
	{
		//looks like it might be a link!
		char *end = NULL;
		char *info;
		for (info = buffer + 2; *info; )
		{
			if (info[0] == '^' && info[1] == ']')
				break; //end of tag, with no actual info, apparently
			if (*info == '\\')
				break;
			else if (info[0] == '^' && info[1] == '^')
				info+=2;
			else
				info++;
		}
		for(end = info; *end; )
		{
			if (end[0] == '^' && end[1] == ']')
			{
				//okay, its a valid link that they clicked
				*end = 0;
#ifdef PLUGINS	//plugins can use these window things like popup menus.
				if (con && !Plug_ConsoleLink(buffer+2, info, con->name))
#endif
#ifdef CSQC_DAT
				if (!CSQC_ConsoleLink(buffer+2, info))
#endif
				{
					Key_DefaultLinkClicked(con, buffer+2, info);
				}

				break;
			}
			if (end[0] == '^' && end[1] == '^')
				end+=2;
			else
				end++;
		}
	}
}

void Key_ConsoleRelease(console_t *con, int key, unsigned int unicode)
{
	char *buffer;	
	if (key < 0)
		key = 0;

	if ((key == K_TOUCHTAP || key == K_MOUSE1) && con->buttonsdown == CB_SELECT)
	{
		if (fabs(con->mousedown[0] - con->mousecursor[0]) < 5 && fabs(con->mousedown[1] - con->mousecursor[1]) < 5 && realtime < con->mousedowntime + 0.4)
			con->buttonsdown = CB_TAPPED;	//don't leave it selected.
		else
			con->buttonsdown = CB_SELECTED;
		return;
	}
	if ((key == K_TOUCHSLIDE || key == K_MOUSE1) && con->buttonsdown == CB_SCROLL)// || (key == K_MOUSE2 && con->buttonsdown == CB_SCROLL_R))
	{
		con->buttonsdown = CB_NONE;
		if (fabs(con->mousedown[0] - con->mousecursor[0]) < 5 && fabs(con->mousedown[1] - con->mousecursor[1]) < 5)
		{
			buffer = Con_CopyConsole(con, false, false, false);
			Con_Footerf(con, false, "");
			if (!buffer)
				return;
			if (keydown[K_SHIFT])
			{
				int len;
				len = strlen(buffer);
				//strip any trailing dots/elipsis
				while (len > 1 && !strcmp(buffer+len-1, "."))
				{
					len-=1;
					buffer[len] = 0;
				}
				//strip any enclosing quotes
				while (*buffer == '\"' && len > 2 && !strcmp(buffer+len-1, "\""))
				{
					len-=2;
					memmove(buffer, buffer+1, len);
					buffer[len] = 0;
				}
				Key_ConsoleInsert(buffer);
			}
			else
				Key_HandleConsoleLink(con, buffer);
			Z_Free(buffer);
		}
		else
			Con_Footerf(con, false, "");
	}
	/*if ((key == K_TOUCHLONG || key == K_MOUSE2) && con->buttonsdown == CB_COPY)
	{
		con->buttonsdown = CB_NONE;
		buffer = Con_CopyConsole(con, true, false, true);	//don't keep markup if we're copying to the clipboard
		if (!buffer)
			return;
		Sys_SaveClipboard(CBT_CLIPBOARD,  buffer);
		Z_Free(buffer);
	}*/
	if (con->buttonsdown == CB_CLOSE)
	{	//window X (close)
		if (con->mousecursor[0] > con->wnd_w-16 && con->mousecursor[1] < 8)
		{
			if (con->close && !con->close(con, false))
				return;
			Con_Destroy (con);
			return;
		}
	}
	if (con->buttonsdown == CB_SELECTED || con->buttonsdown == CB_TAPPED)
		;	//will time out in the drawing code.
	else
//	if (con->buttonsdown == CB_MOVE)	//window title(move)
		con->buttonsdown = CB_NONE;

#ifdef HAVE_MEDIA_DECODER
	if (con->backshader)
	{
		cin_t *cin = R_ShaderGetCinematic(con->backshader);
		if (cin)
			Media_Send_KeyEvent(cin, key, unicode, 1);
	}
#endif
}

static qbyte *emojidata = NULL;
static const qbyte *builtinemojidata =
//		"\x02\x03" ":)"				"\xE2\x98\xBA"

#ifdef QUAKEHUD
	"\x04\x03" ":sg:"			"\xEE\x84\x82"
	"\x05\x03" ":ssg:"			"\xEE\x84\x83"
	"\x04\x03" ":ng:"			"\xEE\x84\x84"
	"\x05\x03" ":sng:"			"\xEE\x84\x85"
	"\x04\x03" ":gl:"			"\xEE\x84\x86"
	"\x04\x03" ":rl:"			"\xEE\x84\x87"
	"\x04\x03" ":lg:"			"\xEE\x84\x88"

	"\x05\x03" ":sg2:"			"\xEE\x84\x92"
	"\x06\x03" ":ssg2:"			"\xEE\x84\x93"
	"\x05\x03" ":ng2:"			"\xEE\x84\x94"
	"\x06\x03" ":sng2:"			"\xEE\x84\x95"
	"\x05\x03" ":gl2:"			"\xEE\x84\x96"
	"\x05\x03" ":rl2:"			"\xEE\x84\x97"
	"\x05\x03" ":lg2:"			"\xEE\x84\x98"

	"\x08\x03" ":shells:"		"\xEE\x84\xA0"
	"\x07\x03" ":nails:"		"\xEE\x84\xA1"
	"\x08\x03" ":rocket:"		"\xEE\x84\xA2"
	"\x07\x03" ":cells:"		"\xEE\x84\xA3"
	"\x04\x03" ":ga:"			"\xEE\x84\xA4"
	"\x04\x03" ":ya:"			"\xEE\x84\xA5"
	"\x04\x03" ":ra:"			"\xEE\x84\xA6"

	"\x06\x03" ":key1:"			"\xEE\x84\xB0"
	"\x06\x03" ":key2:"			"\xEE\x84\xB1"
	"\x06\x03" ":ring:"			"\xEE\x84\xB2"
	"\x06\x03" ":pent:"			"\xEE\x84\xB3"
	"\x06\x03" ":suit:"			"\xEE\x84\xB4"
	"\x06\x03" ":quad:"			"\xEE\x84\xB5"
	"\x08\x03" ":sigil1:"		"\xEE\x84\xB6"
	"\x08\x03" ":sigil2:"		"\xEE\x84\xB7"
	"\x08\x03" ":sigil3:"		"\xEE\x84\xB8"
	"\x08\x03" ":sigil4:"		"\xEE\x84\xB9"

	"\x07\x03" ":face1:"		"\xEE\x85\x80"
	"\x09\x03" ":face_p1:"		"\xEE\x85\x81"
	"\x07\x03" ":face2:"		"\xEE\x85\x82"
	"\x09\x03" ":face_p2:"		"\xEE\x85\x83"
	"\x07\x03" ":face3:"		"\xEE\x85\x84"
	"\x09\x03" ":face_p3:"		"\xEE\x85\x85"
	"\x07\x03" ":face4:"		"\xEE\x85\x86"
	"\x09\x03" ":face_p4:"		"\xEE\x85\x87"
	"\x07\x03" ":face5:"		"\xEE\x85\x88"
	"\x09\x03" ":face_p5:"		"\xEE\x85\x89"
	"\x0c\x03" ":face_invis:"	"\xEE\x85\x8A"
	"\x0d\x03" ":face_invul2:"	"\xEE\x85\x8B"
	"\x0b\x03" ":face_inv2:"	"\xEE\x85\x8C"
	"\x0b\x03" ":face_quad:"	"\xEE\x85\x8D"
#endif
	"";
static void Key_LoadEmojiList(void)
{
	qbyte line[1024];
	vfsfile_t *f;
	char *json = FS_MallocFile("data-by-emoji.json", FS_GAME, NULL);	//https://unpkg.com/unicode-emoji-json/data-by-emoji.json

	emojidata = Z_StrDup(builtinemojidata);
	if (json)
	{
		//eg: {	"utf8":{"slug":"text_for_emoji"}, ... } (there's a few other keys*/
		json_t *root = JSON_Parse(json);
		json_t *def;
		char nam[64];
		for (def = (root?root->child:NULL); def; def = def->sibling)
		{
			int e;
			const char *o;
			utf8_decode(&e, def->name, &o);
			if (*o)
				continue;	//we can only cope with single codepoints.
			if (JSON_GetString(def, "slug", nam+1, sizeof(nam)-2, NULL))
			{
				nam[0] = ':';
				Q_strncatz(nam, ":", sizeof(nam));
				line[0] = strlen(nam);
				line[1] = strlen(def->name);
				strcpy(line+2, nam);
				strcpy(line+2+line[0], def->name);
				Z_StrCat((char**)&emojidata, line);
			}
		}
		JSON_Destroy(root);
		FS_FreeFile(json);
	}

	f = FS_OpenVFS("emoji.lst", "rb", FS_GAME);
	if (f)
	{
		qbyte line[1024];
		char nam[64];
		char rep[64];
		while (VFS_GETS(f, line, sizeof(line)))
		{
			COM_ParseTokenOut(COM_ParseTokenOut(line, NULL, nam, sizeof(nam), NULL), NULL, rep, sizeof(rep), NULL);
			if (!*nam || !*rep)
				continue;	//next line then, I guess.
			line[0] = strlen(nam);
			line[1] = strlen(rep);
			strcpy(line+2, nam);
			strcpy(line+2+line[0], rep);
			Z_StrCat((char**)&emojidata, line);
		}
		VFS_CLOSE(f);
	}
}
void Key_EmojiCompletion_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	char guess[256];
	char repl[256];
	size_t ofs, len;
	if (*partial != ':')
		return;	//don't show annoying completion crap.
	if (!emojidata)
		Key_LoadEmojiList();
	len = strlen(partial);
	for (ofs = 0; emojidata[ofs]; )
	{
		if (len <= emojidata[ofs+0])
			if (!strncmp(partial, &emojidata[ofs+2], len))
			{
				memcpy(guess, &emojidata[ofs+2], emojidata[ofs+0]);
				guess[emojidata[ofs+0]] = 0;

				memcpy(repl, &emojidata[ofs+2]+emojidata[ofs+0], emojidata[ofs+1]);
				repl[emojidata[ofs+1]] = 0;
				ctx->cb(guess, NULL, NULL, ctx);
			}
		ofs += 2+emojidata[ofs+0]+emojidata[ofs+1];
	}
}

const char *Key_Demoji(char *buffer, size_t buffersize, const char *in)
{
	char *estart = strchr(in, ':');
	size_t ofs;
	char *out = buffer, *outend = buffer+buffersize-1;
	if (!estart)
		return in;

	if (!emojidata)
		Key_LoadEmojiList();

	for(; estart; )
	{
		if (out + (estart-in) >= outend)
			break; //not enough space
		memcpy(out, in, estart-in);
		out += estart-in;
		in = estart;

		for (ofs = 0; emojidata[ofs]; )
		{
			if (!strncmp(in, &emojidata[ofs+2], emojidata[ofs+0]))
				break;	//its this one!
			ofs += 2+emojidata[ofs+0]+emojidata[ofs+1];
		}
		if (emojidata[ofs])
		{
			if (out + emojidata[ofs+1] >= outend)
			{
				in = "";	//no half-emoji
				break;
			}
			in += emojidata[ofs+0];
			memcpy(out, &emojidata[ofs+2]+emojidata[ofs+0], emojidata[ofs+1]);
			out += emojidata[ofs+1];
			estart = strchr(in, ':');
		}
		else
		{
			estart = strchr(in+1, ':');
		}
	}
	while (*in && out < outend)
		*out++ = *in++;
	*out = 0;
	return buffer;
}

//if the referenced (trailing) chevron is doubled up, then it doesn't act as part of any markup and should be ignored for such things.
static qboolean utf_specialchevron(unsigned char *start, unsigned char *chev)
{
	int count = 0;
	while (chev >= start)
	{
		if (*chev-- == '^')
			count++;
		else
			break;
	}
	return count&1;
}
//move the cursor one char to the left. cursor must be within the 'start' string.
static unsigned char *utf_left(unsigned char *start, unsigned char *cursor, qboolean skiplink)
{
	if (cursor == start)
		return cursor;
	if (1)//com_parseutf8.ival>0)
	{
		cursor--;
		while ((*cursor & 0xc0) == 0x80 && cursor > start)
			cursor--;
	}
	else
		cursor--;

	//FIXME: should verify that the ^ isn't doubled.
	if (*cursor == ']' && cursor > start && skiplink && utf_specialchevron(start, cursor-1))
	{
		//just stepped onto a link
		unsigned char *linkstart;
		linkstart = cursor-1;
		while(linkstart >= start)
		{
			//FIXME: should verify that the ^ isn't doubled.
			if (utf_specialchevron(start, linkstart) && linkstart[1] == '[')
				return linkstart;
			linkstart--;
		}
	}

	return cursor;
}

//move the cursor one char to the right.
static unsigned char *utf_right(unsigned char *start, unsigned char *cursor, qboolean skiplink)
{
	//FIXME: should make sure this is not doubled.
	if (utf_specialchevron(start, cursor) && cursor[1] == '[' && skiplink)
	{
		//just stepped over a link
		char *linkend;
		linkend = cursor+2;
		while(*linkend)
		{
			if (utf_specialchevron(start, linkend) && linkend[1] == ']')
				return linkend+2;
			else
				linkend++;
		}
		return linkend;
	}

	if (1)//com_parseutf8.ival>0)
	{
		int skip = 1;
		//figure out the length of the char
		if ((*cursor & 0xc0) == 0x80)
			skip = 1;	//error
		else if ((*cursor & 0xe0) == 0xc0)
			skip = 2;
		else if ((*cursor & 0xf0) == 0xe0)
			skip = 3;
		else if ((*cursor & 0xf1) == 0xf0)
			skip = 4;
		else if ((*cursor & 0xf3) == 0xf1)
			skip = 5;
		else if ((*cursor & 0xf7) == 0xf3)
			skip = 6;
		else if ((*cursor & 0xff) == 0xf7)
			skip = 7;
		else skip = 1;

		while (*cursor && skip)
		{
			cursor++;
			skip--;
		}
	}
	else if (*cursor)
		cursor++;

	return cursor;
}

void Key_EntryInsert(unsigned char **line, int *linepos, const char *instext)
{
	int i;
	int len, olen;
	char *old;

	if (!*instext)
		return;

	old = (*line);
	len = strlen(instext);
	olen = strlen(old);
	*line = BZ_Malloc(olen + len + 1);
	memcpy(*line, old, *linepos);
	memcpy(*line+*linepos, instext, len);
	memcpy(*line+*linepos+len, old+*linepos, olen - *linepos+1);
	Z_Free(old);
	for (i = *linepos; i < *linepos+len; i++)
	{
		if ((*line)[i] == '\r')
			(*line)[i] = ' ';
		else if ((*line)[i] == '\n')
			(*line)[i] = ';';
	}
	*linepos += len;
}

static void Key_ConsolePaste(void *ctx, const char *utf8)
{
	unsigned char **line = ctx;
	int *linepos = ((line == &chat_buffer)?&chat_bufferpos:&key_linepos);
	if (utf8)
		Key_EntryInsert(line, linepos, utf8);
}
qboolean Key_EntryLine(console_t *con, unsigned char **line, int lineoffset, int *linepos, int key, unsigned int unicode)
{
	qboolean alt = keydown[K_LALT] || keydown[K_RALT];
	qboolean ctrl = keydown[K_LCTRL] || keydown[K_RCTRL];
	qboolean shift = keydown[K_LSHIFT] || keydown[K_RSHIFT];
	char utf8[8];

	if (key == K_LEFTARROW || key == K_KP_LEFTARROW || key == K_GP_DPAD_LEFT)
	{
		if (ctrl)
		{
			//ignore whitespace if we're at the end of the word
			while (*linepos > 0 && (*line)[*linepos-1] == ' ')
				*linepos = utf_left((*line)+lineoffset, (*line) + *linepos, !alt) - (*line);
			//keep skipping until we find the start of that word
			while (ctrl && *linepos > lineoffset && (*line)[*linepos-1] != ' ')
				*linepos = utf_left((*line)+lineoffset, (*line) + *linepos, !alt) - (*line);
		}
		else
			*linepos = utf_left((*line)+lineoffset, (*line) + *linepos, !alt) - (*line);
		return true;
	}
	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW || key == K_GP_DPAD_RIGHT)
	{
		if ((*line)[*linepos])
		{
			*linepos = utf_right((*line)+lineoffset, (*line) + *linepos, !alt) - (*line);
			if (ctrl)
			{
				//skip over the word
				while ((*line)[*linepos] && (*line)[*linepos] != ' ')
					*linepos = utf_right((*line)+lineoffset, (*line) + *linepos, !alt) - (*line);
				//as well as any trailing whitespace
				while ((*line)[*linepos] == ' ')
					*linepos = utf_right((*line)+lineoffset, (*line) + *linepos, !alt) - (*line);
			}
			return true;
		}
		else
			unicode = ' ';
	}

	if (key == K_DEL || key == K_KP_DEL)
	{
		if ((*line)[*linepos])
		{
			int charlen = utf_right((*line)+lineoffset, (*line) + *linepos, !alt) - ((*line) + *linepos);
			memmove((*line)+*linepos, (*line)+*linepos+charlen, strlen((*line)+*linepos+charlen)+1);
			return true;
		}
		else
			key = K_BACKSPACE;
	}

	if (key == K_BACKSPACE)
	{
		if (*linepos > lineoffset)
		{
			int charlen = ((*line)+*linepos) - utf_left((*line)+lineoffset, (*line) + *linepos, !alt);
			memmove((*line)+*linepos-charlen, (*line)+*linepos, strlen((*line)+*linepos)+1);
			*linepos -= charlen;
		}

		if (con_commandmatch)
		{
			Con_Footerf(NULL, false, "");
			con_commandmatch = (**line?1:0);
		}
		return true;
	}



	if (key == K_HOME || key == K_KP_HOME)
	{
		*linepos = lineoffset;
		return true;
	}

	if (key == K_END || key == K_KP_END)
	{
		*linepos = strlen(*line);
		return true;
	}

	//beware that windows translates ctrl+c and ctrl+v to a control char
	if (((unicode=='C' || unicode=='c' || unicode==3) && ctrl) || (ctrl && key == K_INS))
	{
		if (con && (con->flags & CONF_KEEPSELECTION))
		{	//copy selected text to the system clipboard
			char *buffer = Con_CopyConsole(con, true, false, true);
			if (buffer)
			{
				Sys_SaveClipboard(CBT_CLIPBOARD,  buffer);
				Z_Free(buffer);
			}
			return true;
		}
		//copy the entire input line if there's nothing selected.
		Sys_SaveClipboard(CBT_CLIPBOARD, *line);
		return true;
	}

	if (key == K_MOUSE3)
	{	//middle-click to paste from the unixy primary buffer.
		Sys_Clipboard_PasteText(CBT_SELECTION, Key_ConsolePaste, line);
		return true;
	}
	if (((unicode=='V' || unicode=='v' || unicode==22/*sync*/) && ctrl) || (shift && key == K_INS))
	{	//ctrl+v to paste from the windows-style clipboard.
		Sys_Clipboard_PasteText(CBT_CLIPBOARD, Key_ConsolePaste, line);
		return true;
	}

	if ((unicode=='X' || unicode=='x' || unicode==24/*cancel*/) && ctrl)
	{	//cut - copy-to-clipboard-and-delete
		Sys_SaveClipboard(CBT_CLIPBOARD, *line);
		(*line)[lineoffset] = 0;
		*linepos = strlen(*line);
		return true;
	}
	if ((unicode=='U' || unicode=='u' || unicode==21/*nak*/) && ctrl)
	{	//clear line
		(*line)[lineoffset] = 0;
		*linepos = strlen(*line);
		return true;
	}

	if (unicode < ' ')
	{
		//if the user is entering control codes, then the ctrl+foo mechanism is probably unsupported by the unicode input stuff, so give best-effort replacements.
		switch(unicode)
		{
		case 27/*'['*/: unicode = 0xe010; break;
		case 29/*']'*/: unicode = 0xe011; break;
		case 7/*'g'*/: unicode = 0xe086; break;
		case 18/*'r'*/: unicode = 0xe087; break;
		case 25/*'y'*/: unicode = 0xe088; break;
		case 2/*'b'*/: unicode = 0xe089; break;
		case 19/*'s'*/: unicode = 0xe080; break;
		case 4/*'d'*/: unicode = 0xe081; break;
		case 6/*'f'*/: unicode = 0xe082; break;
		case 1/*'a'*/: unicode = 0xe083; break;
		case 21/*'u'*/: unicode = 0xe01d; break;
		case 9/*'i'*/: unicode = 0xe01e; break;
		case 15/*'o'*/: unicode = 0xe01f; break;
		case 10/*'j'*/: unicode = 0xe01c; break;
		case 16/*'p'*/: unicode = 0xe09c; break;
		case 13/*'m'*/: unicode = 0xe08b; break;
		case 11/*'k'*/: unicode = 0xe08d; break;
		case 14/*'n'*/: unicode = '\r'; break;
		default:
//			if (unicode)
//				Con_Printf("escape code %i\n", unicode);

			//even if we don't print these, we still need to cancel them in the caller.
			if (key == K_LALT || key == K_RALT ||
				key == K_LCTRL || key == K_RCTRL ||
				key == K_LSHIFT || key == K_RSHIFT)
				return true;
			return false;
		}
	}
#ifndef FTE_TARGET_WEB	//browser port gets keys stuck down when task switching, especially alt+tab. don't confuse users.
	else if (com_parseutf8.ival >= 0)	//don't do this for iso8859-1. the major user of that is hexen2 which doesn't have these chars.
	{
		if (ctrl && !keydown[K_RALT])
		{
			if (unicode >= '0' && unicode <= '9')
				unicode = unicode - '0' + 0xe012;	// yellow number
			else switch (unicode)
			{
				case '[': unicode = 0xe010; break;
				case ']': unicode = 0xe011; break;
				case 'g': unicode = 0xe086; break;
				case 'r': unicode = 0xe087; break;
				case 'y': unicode = 0xe088; break;
				case 'b': unicode = 0xe089; break;
				case '(': unicode = 0xe080; break;
				case '=': unicode = 0xe081; break;
				case ')': unicode = 0xe082; break;
				case 'a': unicode = 0xe083; break;
				case '<': unicode = 0xe01d; break;
				case '-': unicode = 0xe01e; break;
				case '>': unicode = 0xe01f; break;
				case ',': unicode = 0xe01c; break;
				case '.': unicode = 0xe09c; break;
				case 'B': unicode = 0xe08b; break;
				case 'C': unicode = 0xe08d; break;
				case 'n': unicode = '\r'; break;
			}
		}

		if (keydown[K_LALT] && unicode > 32 && unicode < 128)
			unicode |= 0xe080;		// red char
	}
#endif

	unicode = utf8_encode(utf8, unicode, sizeof(utf8)-1);
	if (unicode)
	{
		utf8[unicode] = 0;
		Key_EntryInsert(line, linepos, utf8);
		return true;
	}

	return false;
}

/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
qboolean Key_Console (console_t *con, int key, unsigned int unicode)
{
	qboolean ctrl = keydown[K_LCTRL] || keydown[K_RCTRL];
	qboolean shift = keydown[K_LSHIFT] || keydown[K_RSHIFT];
	int rkey = key;
	char *buffer;

	//weirdness for the keypad.
	if ((unicode >= '0' && unicode <= '9') || unicode == '.' || key < 0)
		key = 0;

	if (key == K_TAB && !(con->flags & CONF_ISWINDOW) && ctrl&&shift)
	{	// cycle consoles with ctrl+shift+tab.
		// (ctrl+tab forces tab completion,
		//	shift+tab controls completion cycle,
		//  so it has to be both.)
		Con_CycleConsole();
		return true;
	}
	if (con->redirect)
	{
		if (key == K_TOUCHTAP || key == K_TOUCHSLIDE || key == K_TOUCHLONG || key == K_MOUSE1 || key == K_MOUSE2)
			;
		else if (con->redirect(con, unicode, key))
			return true;
	}

	if (key == K_GP_VIEW || key == K_GP_MENU)
	{
		Key_Dest_Remove(kdm_console);
		Key_Dest_Remove(kdm_cwindows);
		if (!cls.state && !Key_Dest_Has(~kdm_game))
			M_ToggleMenu_f ();
		return true;
	}

	if (key == K_TOUCHTAP || key == K_TOUCHSLIDE || key == K_TOUCHLONG || key == K_MOUSE1 || key == K_MOUSE2)
	{
		int olddown[2] = {con->mousedown[0],con->mousedown[1]};
		if (con->flags & CONF_ISWINDOW)
			if (con->mousecursor[0] < -8 || con->mousecursor[1] < 0 || con->mousecursor[0] > con->wnd_w || con->mousecursor[1] > con->wnd_h)
				return true;
		if (con == con_mouseover)
		{
			con->buttonsdown = CB_NONE;

			if ((con->flags & CONF_ISWINDOW) && !keydown[K_SHIFT])
				Con_SetActive(con);
		}
		con->mousedown[0] = con->mousecursor[0];
		con->mousedown[1] = con->mousecursor[1];
		if (con_mouseover && con->mousedown[1] < 8)//(8.0*vid.height)/vid.pixelheight)
		{
			if ((key == K_MOUSE2||key==K_TOUCHLONG) && !(con->flags & CONF_ISWINDOW))
			{
				if (con->close && !con->close(con, false))
					return true;
				Con_Destroy (con);
			}
			else
			{
				Con_SetActive(con);
				if ((con->flags & CONF_ISWINDOW))
					con->buttonsdown = (con->mousedown[0] > con->wnd_w-16)?CB_CLOSE:CB_MOVE;
			}
		}
		else if (con_mouseover && con->mousedown[1] < 16)
			con->buttonsdown = CB_ACTIONBAR;
		else if (key == K_MOUSE2)
		{
			if (con->redirect && con->redirect(con, unicode, key))
				return true;

			con->flags &= ~CONF_KEEPSELECTION;
			con->buttonsdown = CB_SCROLL_R;
		}
		else
		{
			con->buttonsdown = CB_NONE;
			if ((con->flags & CONF_ISWINDOW) && con->mousedown[0] < 0)
				con->buttonsdown |= CB_SIZELEFT;
			if ((con->flags & CONF_ISWINDOW) && con->mousedown[0] > con->wnd_w-16)
				con->buttonsdown |= CB_SIZERIGHT;
			if ((con->flags & CONF_ISWINDOW) && con->mousedown[1] > con->wnd_h-8)
				con->buttonsdown |= CB_SIZEBOTTOM;
			if (con->buttonsdown == CB_NONE)
			{
				if (con->redirect && con->redirect(con, unicode, key))
					return true;
#ifdef HAVE_MEDIA_DECODER
				if (con->backshader && R_ShaderGetCinematic(con->backshader))
				{
					Media_Send_KeyEvent(R_ShaderGetCinematic(con->backshader), rkey, unicode, 0);
					return true;
				}
#endif
				if (key == K_TOUCHSLIDE || con->mousecursor[0] > ((con->flags & CONF_ISWINDOW)?con->wnd_w-16:vid.width)-8)
				{	//just scroll the console up/down
					con->buttonsdown = CB_SCROLL;
				}
				else
				{	//selecting text. woo.
					if (realtime < con->mousedowntime + 0.4
						&& con->mousecursor[0] >= olddown[0]-3 && con->mousecursor[0] <= olddown[0]+3
						&& con->mousecursor[1] >= olddown[1]-3 && con->mousecursor[1] <= olddown[1]+3
						)
					{	//this was a double-click... expand the selection to the entire word
						//FIXME: detect tripple-clicks to select the entire line
						Con_ExpandConsoleSelection(con);
						con->flags |= CONF_KEEPSELECTION;

						buffer = Con_CopyConsole(con, true, false, true);	//don't keep markup if we're copying to the clipboard
						if (buffer)
						{
							Sys_SaveClipboard(CBT_SELECTION,  buffer);
							Z_Free(buffer);
						}
						return true;
					}
					con->mousedowntime = realtime;

					con->buttonsdown = CB_SELECT;
					con->flags &= ~CONF_KEEPSELECTION;

					if (shift)
					{
						con->mousedown[0] = olddown[0];
						con->mousedown[1] = olddown[1];
					}
				}
			}
		}

		if (con->buttonsdown == CB_SCROLL && !con->linecount && (!con->linebuffered || con->linebuffered == Con_Navigate))
			con->buttonsdown = CB_NONE;
		else
			return true;
	}
	if (key == K_TOUCH)
		return true;	//eat it, so we don't get any kind of mouse emu junk

	if (key == K_PGUP || key == K_KP_PGUP || key==K_MWHEELUP || key == K_GP_LEFT_THUMB_UP)
	{
		conline_t *l;
		int i = 2;
		if (ctrl)
			i = 8;
		if (!con->display)
			return true;
//		if (con->display == con->current)
//			i+=2;	//skip over the blank input line, and extra so we actually move despite the addition of the ^^^^^ line
		if (con->display->older != NULL)
		{
			con->displayscroll += i;
			while (con->displayscroll >= con->display->numlines)
			{
				if (con->display->older == NULL)
					break;
				con->displayscroll -= con->display->numlines;
				con->display = con->display->older;
				con->display->time = realtime;
			}
			for (l = con->display; l; l = l->older)
				l->time = realtime;
			return true;
		}
	}
	if (key == K_PGDN || key == K_KP_PGDN || key==K_MWHEELDOWN || key == K_GP_LEFT_THUMB_DOWN)
	{
		int i = 2;
		if (ctrl)
			i = 8;
		if (!con->display)
			return true;
		if (con->display->newer != NULL)
		{
			con->displayscroll -= i;
			while (con->displayscroll < 0)
			{
				if (con->display->newer == NULL)
					break;
				con->display = con->display->newer;
				con->display->time = realtime;
				con->displayscroll += con->display->numlines;
			}
			if (con->display->newer && con->display->newer == con->current)
			{
				con->display = con->current;
				con->displayscroll = 0;
			}
			return true;
		}
	}

	if ((key == K_HOME || key == K_KP_HOME) && ctrl)
	{
		if (con->display != con->oldest)
		{
			con->displayscroll = 0;
			con->display = con->oldest;
			return true;
		}
	}

	if ((key == K_END || key == K_KP_END) && ctrl)
	{
		if (con->display != con->current)
		{
			con->displayscroll = 0;
			con->display = con->current;
			return true;
		}
	}

#ifdef TEXTEDITOR
	if (editormodal)
	{
		if (Editor_Key(key, unicode))
			return true;
	}
#endif

	if (key == K_MOUSE3)	//mousewheel click/middle button
	{
	}

	//console does not have any way to accept input, so don't try giving it any.
	if (!con->linebuffered)
	{
#ifdef HAVE_MEDIA_DECODER
		if (con->backshader)
		{
			cin_t *cin = R_ShaderGetCinematic(con->backshader);
			if (cin)
			{
				Media_Send_KeyEvent(cin, rkey, unicode, 0);
				return true;
			}
		}
#endif
		return false;
	}
	
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM)
	{	// backslash text are commands, else chat
		char demoji[8192];
		const char *txt = Key_Demoji(demoji, sizeof(demoji), key_lines[edit_line]);

#ifndef FTE_TARGET_WEB
		if (keydown[K_LALT] || keydown[K_RALT])
			Cbuf_AddText("\nvid_toggle\n", RESTRICT_LOCAL);
#endif

		if ((con_commandmatch && !strchr(txt, ' ')) || shift)
		{	//if that isn't actually a command, and we can actually complete it to something, then lets try to complete it.
			if (*txt == '/')
				txt++;

			if ((shift||!Cmd_IsCommand(txt)) && Cmd_CompleteCommand(txt, true, true, con_commandmatch, NULL))
			{
				CompleteCommand (true, 1);
				return true;
			}
			Con_Footerf(con, false, "");
			con_commandmatch = 0;
		}


		if (con->linebuffered)
		{
			if (con->linebuffered(con, txt) != 2)
			{
				edit_line = (edit_line + 1) & (CON_EDIT_LINES_MASK);
				history_line = edit_line;
			}
		}

		Z_Free(key_lines[edit_line]);
		key_lines[edit_line] = BZ_Malloc(1);
		key_lines[edit_line][0] = '\0';
		key_linepos = 0;
		con_commandmatch = 0;
		return true;
	}

	if (key == K_SPACE && ctrl && con->commandcompletion)
	{
		char *txt = key_lines[edit_line];
		if (*txt == '/')
			txt++;
		if (Cmd_CompleteCommand(txt, true, true, con->commandcompletion, NULL))
		{
			CompleteCommand (true, 1);
			return true;
		}
	}

	if (key == K_TAB)
	{	// command completion
		if (con->commandcompletion)
			CompleteCommand (ctrl, shift?-1:1);
		return true;
	}
	
	if (key == K_UPARROW || key == K_KP_UPARROW || key == K_GP_DPAD_UP)
	{
		do
		{
			history_line = (history_line - 1) & CON_EDIT_LINES_MASK;
		} while (history_line != edit_line
				&& !key_lines[history_line][0]);
		if (history_line == edit_line)
			history_line = (edit_line+1)&CON_EDIT_LINES_MASK;
		key_lines[edit_line] = BZ_Realloc(key_lines[edit_line], strlen(key_lines[history_line])+1);
		Q_strcpy(key_lines[edit_line], key_lines[history_line]);
		key_linepos = Q_strlen(key_lines[edit_line]);

		con_commandmatch = 0;
		return true;
	}

	if (key == K_DOWNARROW || key == K_KP_DOWNARROW || key == K_GP_DPAD_DOWN)
	{
		if (history_line == edit_line)
		{
			key_lines[edit_line][0] = '\0';
			key_linepos=0;
			con_commandmatch = 0;
			return true;
		}
		do
		{
			history_line = (history_line + 1) & CON_EDIT_LINES_MASK;
		}
		while (history_line != edit_line
			&& !key_lines[history_line][1]);
		if (history_line == edit_line)
		{
			key_lines[edit_line][0] = '\0';
			key_linepos = 0;
		}
		else
		{
			key_lines[edit_line] = BZ_Realloc(key_lines[edit_line], strlen(key_lines[history_line])+1);
			Q_strcpy(key_lines[edit_line], key_lines[history_line]);
			key_linepos = Q_strlen(key_lines[edit_line]);
		}
		return true;
	}

	if (rkey && !consolekeys[rkey])
	{
		if (rkey != '`' || key_linepos==0)
			return false;
	}

	if (con_commandmatch)
	{	//if they're typing, try to retain the current completion guess.
		cmd_completion_t *c;
		char *txt = key_lines[edit_line];
		char *guess = Cmd_CompleteCommand(*txt=='/'?txt+1:txt, true, true, con_commandmatch, NULL);
		Key_EntryLine(con, &key_lines[edit_line], 0, &key_linepos, key, unicode);
		if (!key_linepos)
			con_commandmatch = 0;
		else if (guess)
		{
			guess = Z_StrDup(guess);
			txt = key_lines[edit_line];
			c = Cmd_Complete(*txt=='/'?txt+1:txt, true);
			for (con_commandmatch = c->num; con_commandmatch > 1; con_commandmatch--) 
			{
				if (!strcmp(guess, c->completions[con_commandmatch-1].text))
					break;
			}
			Z_Free(guess);
		}
	}
	else
		Key_EntryLine(con, &key_lines[edit_line], 0, &key_linepos, key, unicode);

	return true;
}

//============================================================================

qboolean	chat_team;
unsigned char		*chat_buffer;
int			chat_bufferpos;

void Key_Message (int key, int unicode)
{
	if (!chat_buffer)
	{
		chat_buffer = BZ_Malloc(1);
		chat_buffer[0] = 0;
		chat_bufferpos = 0;
	}

	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM)
	{
		if (chat_buffer && chat_buffer[0])
		{	//send it straight into the command.
			const char *line = chat_buffer;
			char deutf8[8192];
			char demoji[8192];
			line = Key_Demoji(demoji, sizeof(demoji), line);

			if (com_parseutf8.ival <= 0)
			{
				unsigned int unicode;
				int err;
				int len = 0;
				while(*line)
				{
					unicode = utf8_decode(&err, line, &line);
					len += unicode_encode(deutf8+len, unicode, sizeof(deutf8)-1 - len, true);
				}
				deutf8[len] = 0;
				line = deutf8;
			}

			Cmd_TokenizeString(va("%s %s", chat_team?"say_team":"say", line), true, false);
			CL_Say(chat_team, "");
		}

		Key_Dest_Remove(kdm_message);
		chat_bufferpos = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key == K_ESCAPE || key == K_TOUCHLONG || key == K_GP_DIAMOND_CANCEL)
	{
		Key_Dest_Remove(kdm_message);
		chat_bufferpos = 0;
		chat_buffer[0] = 0;
		return;
	}

	Key_EntryLine(NULL, &chat_buffer, 0, &chat_bufferpos, key, unicode);
}

//============================================================================

//for qc
const char *Key_GetBinding(int keynum, int bindmap, int modifier)
{
	char *key = NULL;
	if (keynum < 0 || keynum >= K_MAX)
		;
	else if (bindmap < 0)
	{
		key = NULL;
		if (!key)
			key = keybindings[keynum][key_bindmaps[0]];
		if (!key)
			key = keybindings[keynum][key_bindmaps[1]];
	}
	else
	{
		if (bindmap)
			modifier = (bindmap-1) + KEY_MODIFIER_ALTBINDMAP;
		if (modifier >= 0 && modifier < KEY_MODIFIERSTATES)
			key = keybindings[keynum][modifier];
	}
	return key;
}

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (const char *str, int *modifier)
{
	int k;
	keyname_t	*kn;

	if (!strnicmp(str, "std_", 4) || !strnicmp(str, "std+", 4))
		*modifier = 0;
	else
	{
		struct
		{
			char *prefix;
			int len;
			int mod;
		} mods[] =
		{
			{"shift",	5, KEY_MODIFIER_SHIFT},
			{"ctrl",	4, KEY_MODIFIER_CTRL},
			{"alt",		3, KEY_MODIFIER_ALT},
		};
		int i;
		*modifier = 0;
		for (i = 0; i < countof(mods); )
		{
			if (!Q_strncasecmp(mods[i].prefix, str, mods[i].len))
				if (/*str[mods[i].len] == '_' ||*/ str[mods[i].len] == '+' || str[mods[i].len] == ' ')
				if (str[mods[i].len+1])
				{
					*modifier |= mods[i].mod;
					str += mods[i].len+1;
					i = 0;
					continue;
				}
			i++;
		}
		if (!*modifier)
			*modifier = ~0;
	}
	
	if (!str || !str[0])
		return -1;
	if (!str[1])	//single char.
	{
#if 0//def _WIN32
		return VkKeyScan(str[0]);
#else
		k = str[0];
//		return str[0];
#endif
	}
	else
	{
		if (!strncmp(str, "K_", 2))
			str+=2;

		for (kn=keynames ; kn->name ; kn++)
		{
			if (!Q_strcasecmp(str,kn->name))
				return kn->keynum;
		}
		k = atoi(str);
	}
	if (k)	//assume ascii code. (prepend with a 0 if needed)
	{
		if (k >= 'A' && k <= 'Z')
			k += 'a'-'A';
		return k;
	}
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
static char *Key_KeynumToStringRaw (int keynum)
{
	keyname_t	*kn;	
	static	char	tinystr[2];
	
	if (keynum < 0)
		return "<KEY NOT FOUND>";
	if (keynum > 32 && keynum < 127 && keynum != '\'' && keynum != '\"')
	{	// printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}
	
	for (kn=keynames ; kn->name ; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	{
		if (keynum < 10)	//don't let it be a single character
			return va("0%i", keynum);
		return va("%i", keynum);
	}

	return "<UNKNOWN KEYNUM>";
}

const char *Key_KeynumToString (int keynum, int modifier)
{
	const char *r = Key_KeynumToStringRaw(keynum);
	if (r[0] == '<' && r[1])
		modifier = 0;	//would be too weird.
	switch(modifier)
	{
	case KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT:
		return va("Ctrl+Alt+Shift+%s", r);
	case KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT:
		return va("Alt+Shift+%s", r);
	case KEY_MODIFIER_CTRL|KEY_MODIFIER_SHIFT:
		return va("Ctrl+Shift+%s", r);
	case KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT:
		return va("Ctrl+Alt+%s", r);
	case KEY_MODIFIER_CTRL:
		return va("Ctrl+%s", r);
	case KEY_MODIFIER_ALT:
		return va("Alt+%s", r);
	case KEY_MODIFIER_SHIFT:
		return va("Shift+%s", r);
	default:
		return r;	//no modifier or a bindmap
	}
}

const char *Key_KeynumToLocalString (int keynum, int modifier)
{
	const char *r;
#if defined(_WIN32)	|| (defined(__linux__)&&!defined(NO_X11)) //not defined in all targets yet...
	static char tmp[64];
	if (INS_KeyToLocalName(keynum, tmp, sizeof(tmp)))
		r = tmp;
	else
#endif
		r = Key_KeynumToStringRaw(keynum);
	if (r[0] == '<' && r[1])
		modifier = 0;	//would be too weird.
	switch(modifier)
	{
	case KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT:
		return va("Ctrl+Alt+Shift+%s", r);
	case KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT:
		return va("Alt+Shift+%s", r);
	case KEY_MODIFIER_CTRL|KEY_MODIFIER_SHIFT:
		return va("Ctrl+Shift+%s", r);
	case KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT:
		return va("Ctrl+Alt+%s", r);
	case KEY_MODIFIER_CTRL:
		return va("Ctrl+%s", r);
	case KEY_MODIFIER_ALT:
		return va("Alt+%s", r);
	case KEY_MODIFIER_SHIFT:
		return va("Shift+%s", r);
	default:
		return r;	//no modifier or a bindmap
	}
}

/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keynum, int modifier, const char *binding, int level)
{
	char	*newc;
	int		l;

	if (modifier == ~0)	//all of the possibilities.
	{
		if (binding)
		{	//bindmaps are meant to be independant of each other.
			for (l = 0; l < KEY_MODIFIER_ALTBINDMAP; l++)
				Key_SetBinding(keynum, l, binding, level);
		}
		else
		{	//when unbinding, unbind all bindmaps.
			for (l = 0; l < KEY_MODIFIERSTATES; l++)
				Key_SetBinding(keynum, l, binding, level);
		}
		return;
	}

	if (keynum < 0 || keynum >= K_MAX)
		return;

	//just so the quit menu realises it needs to show something.
	Cvar_ConfigChanged();

// free old bindings
	if (keybindings[keynum][modifier])
	{
		Z_Free (keybindings[keynum][modifier]);
		keybindings[keynum][modifier] = NULL;
	}


	if (!binding)
	{
		keybindings[keynum][modifier] = NULL;
		return;
	}
// allocate memory for new binding
	l = Q_strlen (binding);	
	newc = Z_Malloc (l+1);
	Q_strcpy (newc, binding);
	newc[l] = 0;
	keybindings[keynum][modifier] = newc;
	bindcmdlevel[keynum][modifier] = level;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
	int		b, modifier;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv(1), &modifier);
	if (b==-1)
	{
		if (cl_warncmd.ival)
			Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding (b, modifier, NULL, Cmd_ExecLevel);
}

void Key_Unbindall_f (void)
{
	int		i;
	
	for (i=0 ; i<K_MAX ; i++)
		Key_SetBinding (i, ~0, NULL, Cmd_ExecLevel);
}

void Key_Bind_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	int key, mf;
	keyname_t *kn;
	const char *n, *m;
	size_t len = strlen(partial), l;
	struct
	{
		const char *prefix;
		int mods;
	} mtab[] = 
	{
		{"",0},

		{"CTRL+ALT+SHIFT+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"ALT+CTRL+SHIFT+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"SHIFT+CTRL+ALT+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"SHIFT+ALT+CTRL+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"CTRL+SHIFT+ALT+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"ALT+SHIFT+CTRL+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},

		{"CTRL+ALT+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"ALT+CTRL+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"SHIFT+ALT+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"ALT+SHIFT+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"CTRL+SHIFT+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},
		{"SHIFT+CTRL+",KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT},

		{"CTRL+",KEY_MODIFIER_CTRL},
		{"ALT+",KEY_MODIFIER_ALT},
		{"SHIFT+",KEY_MODIFIER_SHIFT},
	};
	for (l = 0; l < countof(mtab); l++)
	{
		m = mtab[l].prefix;
		mf = strlen(m);
		mf = min(mf, len);
		if (Q_strncasecmp(partial, m, mf))
			continue;
		mf = mtab[l].mods;

		for (kn=keynames ; kn->name ; kn++)
		{
			//don't suggest shift+shift, because that would be weird.
			if ((mf & KEY_MODIFIER_CTRL) && (kn->keynum == K_LCTRL || kn->keynum == K_RCTRL))
				continue;
			if ((mf & KEY_MODIFIER_ALT) && (kn->keynum == K_LALT || kn->keynum == K_RALT))
				continue;
			if ((mf & KEY_MODIFIER_SHIFT) && (kn->keynum == K_LSHIFT || kn->keynum == K_RSHIFT))
				continue;

			n = va("%s%s", m, kn->name);
			if (!Q_strncasecmp(partial,n, len))
				ctx->cb(n, NULL, NULL, ctx);
		}
		for (key = 32+1; key < 127; key++)
		{
			if (key >= 'a' && key <= 'z')
				continue;
			n = va("%s%c", m, key);
			if (!Q_strncasecmp(partial,n, len))
				ctx->cb(n, NULL, NULL, ctx);
		}
	}
}
/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
	int			i, c, b, modifier;
	char		cmd[1024];
	int bindmap = 0;
	int level = Cmd_ExecLevel;
	qboolean isbindlevel = !strcmp("bindlevel", Cmd_Argv(0));
	if (!strcmp("in_bind", Cmd_Argv(0)))
	{
		bindmap = atoi(Cmd_Argv(1));
		Cmd_ShiftArgs(1, level==RESTRICT_LOCAL);
	}
	
	c = Cmd_Argc();

	if (c < 2+isbindlevel)
	{
		Con_Printf ("%s <key> %s[command] : attach a command to a key\n", Cmd_Argv(0), isbindlevel?"<level> ":"");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1), &modifier);
	if (b==-1)
	{
		if (cl_warncmd.ival)
			Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}
	if (isbindlevel)
	{
		level = atoi(Cmd_Argv(2));
		if (Cmd_IsInsecure())
			level = Cmd_ExecLevel;
		else
		{
			if (level > RESTRICT_MAX)
				level = RESTRICT_INSECURE;
			else
			{
				if (level < RESTRICT_MIN)
					level = RESTRICT_MIN;
				if (level > Cmd_ExecLevel)
					level = Cmd_ExecLevel;	//clamp exec levels, so we don't get more rights than we should.
			}
		}
	}

	if (bindmap)
	{
		if (bindmap <= 0 || bindmap > KEY_MODIFIER_ALTBINDMAP)
		{
			if (cl_warncmd.ival)
				Con_Printf ("unsupported bindmap %i\n", bindmap);
			return;
		}
		if (modifier != ~0)
		{
			if (cl_warncmd.ival)
				Con_Printf ("modifiers cannot be combined with bindmaps\n");
			return;
		}
		modifier = (bindmap-1) | KEY_MODIFIER_ALTBINDMAP;
	}

	if (c == 2+isbindlevel)
	{
		if (modifier == ~0)	//modifier unspecified. default to no modifier
			modifier = 0;
		if (keybindings[b][modifier])
		{
			char *alias = Cmd_AliasExist(keybindings[b][modifier], RESTRICT_LOCAL);
			char quotedbind[2048];
			char quotedalias[2048];
			char leveldesc[1024];
			if (bindcmdlevel[b][modifier] != level)
			{
				if (Cmd_ExecLevel > RESTRICT_SERVER)
					Q_snprintfz(leveldesc, sizeof(leveldesc), ", for seat %i", Cmd_ExecLevel - RESTRICT_SERVER);
				else if (Cmd_ExecLevel == RESTRICT_SERVER)
					Q_snprintfz(leveldesc, sizeof(leveldesc), ", bound by server");
				else if (bindcmdlevel[b][modifier]>=RESTRICT_INSECURE)
					Q_snprintfz(leveldesc, sizeof(leveldesc), ", bound by insecure source");
				else
					Q_snprintfz(leveldesc, sizeof(leveldesc), ", at level %i", bindcmdlevel[b][modifier]);
			}
			else
				*leveldesc = 0;
			COM_QuotedString(keybindings[b][modifier], quotedbind, sizeof(quotedbind), false);
			if (alias)
			{
				COM_QuotedString(alias, quotedalias, sizeof(quotedalias), false);
				Con_Printf ("^[\"%s\"\\type\\bind %s %s^] = ^[\"%s\"\\type\\alias %s %s^]%s\n", Cmd_Argv(1), Cmd_Argv(1), quotedbind, keybindings[b][modifier], keybindings[b][modifier], quotedalias, leveldesc);
			}
			else
				Con_Printf ("^[\"%s\"\\type\\bind %s %s^] = \"%s\"%s\n", Cmd_Argv(1), keybindings[b][modifier], Cmd_Argv(1), keybindings[b][modifier], leveldesc);
		}
		else
			Con_Printf ("\"%s\" is not bound\n", Cmd_Argv(1) );
		return;
	}

	if (c > 3+isbindlevel)
	{
		Cmd_ShiftArgs(1+isbindlevel, level==RESTRICT_LOCAL);
		Key_SetBinding (b, modifier, Cmd_Args(), level);
		return;
	}
	
// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2+isbindlevel ; i< c ; i++)
	{
		Q_strncatz (cmd, Cmd_Argv(i), sizeof(cmd));
		if (i != (c-1))
			Q_strncatz (cmd, " ", sizeof(cmd));
	}

	Key_SetBinding (b, modifier, cmd, level);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (vfsfile_t *f)
{
	const char *s;
	int		i, m;
	char *binding, *base;

	char keybuf[256];
	char commandbuf[2048];

	for (i=0 ; i<K_MAX ; i++)	//we rebind the key with all modifiers to get the standard bind, then change the specific ones.
	{						//this does two things, it normally allows us to skip 7 of the 8 possibilities
		base = keybindings[i][0];	//plus we can use the config with other clients.
		if (!base)
			base = "";
		for (m = 0; m < KEY_MODIFIER_ALTBINDMAP; m++)
		{
			binding = keybindings[i][m];
			if (!binding)
				binding = "";
			if (strcmp(binding, base) || (m==0 && keybindings[i][0]) || bindcmdlevel[i][m] != bindcmdlevel[i][0])
			{
				s = Key_KeynumToString(i, m);
				//quote it as required
				if (i == ';' || i <= ' ' || strchr(s, ' ') || strchr(s, '+') || strchr(s, '\"'))
					s = COM_QuotedString(s, keybuf, sizeof(keybuf), false);

				if (bindcmdlevel[i][m] != RESTRICT_LOCAL)
					s = va("bindlevel %s %i %s\n", s, bindcmdlevel[i][m], COM_QuotedString(binding, commandbuf, sizeof(commandbuf), false));
				else
					s = va("bind %s %s\n", s, COM_QuotedString(binding, commandbuf, sizeof(commandbuf), false));
				VFS_WRITE(f, s, strlen(s));
			}
		}
		//now generate some special in_binds for bindmaps.
		for (m = 0; m < KEY_MODIFIER_ALTBINDMAP; m++)
		{
			binding = keybindings[i][m|KEY_MODIFIER_ALTBINDMAP];
			if (binding && *binding)
			{
				s = va("%s", Key_KeynumToString(i, 0));
				//quote it as required
				if (i == ';' || i <= ' ' || i == '\"')
					s = COM_QuotedString(s, keybuf, sizeof(keybuf), false);

				s = va("in_bind %i %s %s\n", m+1, s, COM_QuotedString(binding, commandbuf, sizeof(commandbuf), false));
				VFS_WRITE(f, s, strlen(s));
			}
		}
	}
}

/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;

	for (i=0 ; i<=CON_EDIT_LINES_MASK ; i++)
	{
		key_lines[i] = Z_Malloc(1);
		key_lines[i][0] = '\0';
	}
	key_linepos = 0;

	Key_Dest_Add(kdm_game);
	key_dest_absolutemouse = kdm_centerprint | kdm_console | kdm_cwindows | kdm_menu;

//
// init ascii characters in console mode
//
	for (i=32 ; i<128 ; i++)
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_KP_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_KP_LEFTARROW] = true;
	consolekeys[K_KP_RIGHTARROW] = true;
	consolekeys[K_KP_UPARROW] = true;
	consolekeys[K_KP_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_DEL] = true;
	consolekeys[K_KP_DEL] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_KP_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_KP_END] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_KP_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_KP_PGDN] = true;
	consolekeys[K_LSHIFT] = true;
	consolekeys[K_RSHIFT] = true;
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;
	consolekeys[K_LCTRL] = true;
	consolekeys[K_RCTRL] = true;
	consolekeys[K_LALT] = true;
	consolekeys[K_RALT] = true;
	consolekeys['`'] = false;
	consolekeys['~'] = false;

	//most gamepad keys are not console keys, just because.
	consolekeys[K_GP_DPAD_UP] = true;
	consolekeys[K_GP_DPAD_DOWN] = true;
	consolekeys[K_GP_DPAD_LEFT] = true;
	consolekeys[K_GP_DPAD_RIGHT] = true;
	consolekeys[K_GP_START] = true;
	consolekeys[K_GP_BACK] = true;

	for (i=K_MOUSE1 ; i<K_MOUSE10 ; i++)
	{
		consolekeys[i] = true;
	}
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;
	consolekeys[K_TOUCH] = true;

//
// register our functions
//
	Cmd_AddCommandAD ("bind",Key_Bind_f, Key_Bind_c, "Changes the action associated with each keyboard button. Use eg \"bind ctrl+shift+alt+k kill\" for special modifiers (should be used only after more basic modifiers).");
	Cmd_AddCommand ("in_bind",Key_Bind_f);
	Cmd_AddCommand ("bindlevel",Key_Bind_f);
	Cmd_AddCommandAD ("unbind",Key_Unbind_f, Key_Bind_c, NULL);
	Cmd_AddCommandD ("unbindall",Key_Unbindall_f, "A dangerous command that forgets ALL your key settings. For use only in default.cfg.");

	Cvar_Register (&con_echochat, "Console variables");
}

qboolean Key_MouseShouldBeFree(void)
{
	//returns if the mouse should be a cursor or if it should go to the menu

	//if true, the input code is expected to return mouse cursor positions rather than deltas
	extern cvar_t cl_prydoncursor;
	if (key_dest_absolutemouse & key_dest_mask)
		return true;

	if (cl_prydoncursor.ival)
		return true;

	return false;
}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!

On some systems, keys and (uni)char codes will be entirely separate events.
===================
*/
void Key_Event (unsigned int devid, int key, unsigned int unicode, qboolean down)
{
	unsigned int devbit = 1u<<devid;
	int bl, bkey;
	const char	*dc;
	char	*uc;
	char	p[16];
	int modifierstate;
	int conkey = consolekeys[key] || ((unicode || key == '`') && (key != '`' || key_linepos>0));	//if the input line is empty, allow ` to toggle the console, otherwise enter it as actual text.
	qboolean wasdown = !!(keydown[key]&devbit);

//	Con_Printf ("%i : %i : %i\n", key, unicode, down); //@@@

	//bug: two of my keyboard doesn't fire release events if the other shift is already pressed (so I assume this is a common thing).
	//hack around that by just force-releasing eg left if right is pressed, but only on inital press to avoid potential infinite loops if the state got bad.
	//ctrl+alt don't seem to have the problem.
	if (key == K_LSHIFT && !keydown[K_LSHIFT] && keydown[K_RSHIFT])
		Key_Event(devid, K_RSHIFT, 0, false);
	if (key == K_RSHIFT && !keydown[K_RSHIFT] && keydown[K_LSHIFT])
		Key_Event(devid, K_LSHIFT, 0, false);

	modifierstate = KeyModifier(keydown[K_LSHIFT]|keydown[K_RSHIFT], keydown[K_LALT]|keydown[K_RALT], keydown[K_LCTRL]|keydown[K_RCTRL], devbit);

	if (down)
		keydown[key] |= devbit;
	else
		keydown[key] &= ~devbit;

	if (down)
	{
//		if (key >= 200 && !keybindings[key])	//is this too annoying?
//			Con_Printf ("%s is unbound, hit F4 to set.\n", Key_KeynumToString (key) );
	}

	if (key == K_LSHIFT || key == K_RSHIFT)
	{
		shift_down = keydown[K_LSHIFT]|keydown[K_RSHIFT];
	}

	if ((key == K_ESCAPE  && (shift_down        &devbit)) ||
		(key == K_GP_MENU && (keydown[K_GP_VIEW]&devbit)))
	{
		extern cvar_t con_stayhidden;
		if (down && con_stayhidden.ival < 2)
		{
			if (!Key_Dest_Has(kdm_console))	//don't toggle it when the console is already down. this allows typing blind to not care if its already active.
				Con_ToggleConsole_Force();
			return;
		}
	}

	//yes, csqc is allowed to steal the escape key (unless shift).
	//it is blocked from blocking backtick (unless shift).
	if ((key != '`'||shift_down) && (!down || key != K_ESCAPE || (!Key_Dest_Has(~kdm_game) && !shift_down)) &&
		!Key_Dest_Has(~kdm_game))
	{
#ifdef CSQC_DAT
		if (CSQC_KeyPress(key, unicode, down, devid))	//give csqc a chance to handle it.
			return;
#endif
#ifdef VM_CG
		if (q3 && q3->cg.KeyPressed(key, unicode, down))
			return;
#endif
	}
	else if (!down && key)
	{
#ifdef CSQC_DAT
		//csqc should still be told of up events. note that there's some filering to prevent notifying about events that it shouldn't receive (like all the up events when typing at the console).
		CSQC_KeyPress(key, unicode, down, devid);
#endif
	}

//
// handle escape specialy, so the user can never unbind it
//
	if (key == K_ESCAPE)
	{
		if (!Key_Dest_Has(~kdm_game))
		{
#ifdef VM_UI
//			if (UI_KeyPress(key, unicode, down))	//Allow the UI to see the escape key. It is possible that a developer may get stuck at a menu.
//				return;
#endif
		}

		if (!down)
		{
			if (Key_Dest_Has(kdm_prompt) || (Key_Dest_Has(kdm_menu) && !Key_Dest_Has(kdm_console|kdm_cwindows)))
				Menu_KeyEvent (false, devid, key, unicode);
			return;
		}

		if (Key_Dest_Has(kdm_prompt))
			Menu_KeyEvent (true, devid, key, unicode);
		else if (Key_Dest_Has(kdm_console))
		{
			Key_Dest_Remove(kdm_console);
			Key_Dest_Remove(kdm_cwindows);
			if (!cls.state && !Key_Dest_Has(~kdm_game))
				M_ToggleMenu_f ();
		}
		else if (Key_Dest_Has(kdm_cwindows))
		{
			Key_Dest_Remove(kdm_cwindows);
			if (!cls.state && !Key_Dest_Has(~kdm_game))
				M_ToggleMenu_f ();
		}
		else if (Key_Dest_Has(kdm_menu))
			Menu_KeyEvent (true, devid, key, unicode);
		else if (Key_Dest_Has(kdm_message))
		{
			Key_Dest_Remove(kdm_message);
			if (chat_buffer)
				chat_buffer[0] = 0;
			chat_bufferpos = 0;
		}
		else
			M_ToggleMenu_f ();
		return;
	}

//
// key up events only generate commands if the game key binding is
// a button command (leading + sign).  These will occur even in console mode,
// to keep the character from continuing an action started before a console
// switch.  Button commands include the keynum as a parameter, so multiple
// downs can be matched with ups
//
	if (!down)
	{
		if (Key_Dest_Has(kdm_console|kdm_cwindows))
		{
			console_t *con;
			if (Key_Dest_Has(kdm_console))
				con = con_current;
			else if (Key_Dest_Has(kdm_cwindows))
				con = con_curwindow;
			else
				con = NULL;
			if (con_mouseover && ((key >= K_MOUSE1 && key <= K_MWHEELDOWN) || key == K_TOUCH))
				con = con_mouseover;
			if (con_curwindow && con_curwindow != con)
				con_curwindow->buttonsdown = CB_NONE;
			if (con)
			{
				con->mousecursor[0] = mousecursor_x - ((con->flags & CONF_ISWINDOW)?con->wnd_x+8:0);
				con->mousecursor[1] = mousecursor_y - ((con->flags & CONF_ISWINDOW)?con->wnd_y:0);
				Key_ConsoleRelease(con, key, unicode);
			}
		}
		if (Key_Dest_Has(kdm_menu|kdm_prompt))
			Menu_KeyEvent (false, devid, key, unicode);

		uc = releasecommand[key][devid%MAX_INDEVS];
		if (uc)	//this wasn't down, so don't crash on bad commands.
		{
			releasecommand[key][devid%MAX_INDEVS] = NULL;
			Cbuf_AddText (uc, releasecommandlevel[key][devid%MAX_INDEVS]);
			Z_Free(uc);
		}
		return;
	}

//
// during demo playback, most keys bring up the main menu
//
	if (cls.demoplayback && cls.demoplayback != DPB_MVD && conkey && !Key_Dest_Has(~kdm_game))
	{
		dc = keybindings[key][modifierstate];
		if (!dc || (strcmp(dc, "toggleconsole") && strncmp(dc, "+show", 5) && strncmp(dc, "demo_", 5)))
		{
			extern cvar_t cl_demospeed;
			switch (key)
			{	//these keys don't force the menu to appear while playing the demo reel
			case K_LSHIFT:
			case K_RSHIFT:
			case K_LALT:
			case K_RALT:
			case K_LCTRL:
	//		case K_RCTRL:
				break;
			//demo modifiers...
			case K_DOWNARROW:
			case K_GP_DPAD_DOWN:
				Cvar_SetValue(&cl_demospeed, max(cl_demospeed.value - 0.1, 0));
				Con_Printf("playback speed: %g%%\n", cl_demospeed.value*100);
				return;
			case K_UPARROW:
			case K_GP_DPAD_UP:
				Cvar_SetValue(&cl_demospeed, min(cl_demospeed.value + 0.1, 10));
				Con_Printf("playback speed: %g%%\n", cl_demospeed.value*100);
				return;
			case K_LEFTARROW:
			case K_GP_DPAD_LEFT:
				Cbuf_AddText("demo_jump -10", RESTRICT_LOCAL);	//expensive.
				return;
			case K_RIGHTARROW:
			case K_GP_DPAD_RIGHT:
				Cbuf_AddText("demo_jump +10", RESTRICT_LOCAL);
				return;
			//any other key
			default:
				M_ToggleMenu_f ();
				return;
			}
		}
	}

	//prompts get the first chance
	if (Key_Dest_Has(kdm_prompt))
	{
		if (key < K_F1 || key > K_F15)
		{	//function keys don't get intercepted by the menu...
			Menu_KeyEvent (true, devid, key, unicode);
			return;
		}
	}

//
// if not a consolekey, send to the interpreter no matter what mode is
//
	if (/*conkey &&*/Key_Dest_Has(kdm_console|kdm_cwindows))
	{
		console_t *con = Key_Dest_Has(kdm_console)?con_current:con_curwindow;
		if ((con_mouseover||!Key_Dest_Has(kdm_console)) && ((key >= K_MOUSE1 && key <= K_MWHEELDOWN)||key==K_TOUCH))
			con = con_mouseover;
		if (con)
		{
			con->mousecursor[0] = mousecursor_x - ((con->flags & CONF_ISWINDOW)?con->wnd_x+8:0);
			con->mousecursor[1] = mousecursor_y - ((con->flags & CONF_ISWINDOW)?con->wnd_y:0);
			if (Key_Console (con, key, unicode))
				return;
		}
		else
			Key_Dest_Remove(kdm_cwindows);

	}

	//menus after console stuff
	if (Key_Dest_Has(kdm_menu))
	{
		if (key < K_F1 || key > K_F15)
		{	//function keys don't get intercepted by the menu...
			Menu_KeyEvent (true, devid, key, unicode);
			return;
		}
	}
	if (Key_Dest_Has(kdm_message))
	{
		Key_Message (key, unicode);
		return;
	}

	if (Key_Dest_Has(kdm_centerprint))
		if (Key_Centerprint(key, unicode, devid))
			return;

	//anything else is a key binding.

	/*don't auto-repeat binds as it breaks too many scripts*/
	if (down && wasdown)
		return;

	//first player is normally assumed anyway.
	if (cl_forceseat.ival>0)
		Q_snprintfz (p, sizeof(p), "p %i ", cl_forceseat.ival);
	else if (devid)
		Q_snprintfz (p, sizeof(p), "p %i ", devid+1);
	else
		*p = 0;

	//assume the worst
	dc = NULL;
	bl = 0;
	//try bindmaps if they're set
	if (key_bindmaps[0] && (!dc || !*dc))
	{
		dc = keybindings[key][key_bindmaps[0]];
		bl = bindcmdlevel[key][key_bindmaps[0]];
	}
	if (key_bindmaps[1] && (!dc || !*dc))
	{
		dc = keybindings[key][key_bindmaps[1]];
		bl = bindcmdlevel[key][key_bindmaps[1]];
	}

	//regular ctrl_alt_shift_foo binds
	if (!dc || !*dc)
	{
		dc = keybindings[key][modifierstate];
		bl = bindcmdlevel[key][modifierstate];
	}

	bkey = key;
	if (!dc || !*dc)
	{
		bl = RESTRICT_LOCAL;
		//I've split some keys in the past. Some keys won't be known to csqc/menuqc/configs or whatever so we instead remap them to something else to prevent them from being dead-by-default.
		//csqc key codes may even have no standard keycode so cannot easily be bound from qc.
		switch(key)
		{
		//left+right alts/etc got split
		//the generic name maps to the left key, so the right key needs to try the left
		case K_RALT:
			bkey = K_LALT;
			break;
		case K_RCTRL:
			bkey = K_LCTRL;
			break;
		case K_RSHIFT:
			bkey = K_LSHIFT;
			break;
		case K_RWIN:
			bkey = K_LWIN;
			break;

		//gamepad buttons should get fallbacks out of the box, even if they're not initially listed on the binds menu.
		//these may be redefined later...
		case K_GP_LEFT_SHOULDER:	dc = "+jump";			goto defaultedbind;	//qs: impulse 12 (should be 11 for qw...)
		case K_GP_RIGHT_SHOULDER:	dc = "+weaponwheel";	goto defaultedbind;	//qs: impulse 10
		case K_GP_LEFT_TRIGGER:		dc = "+button3";		goto defaultedbind;	//qs: jump
		case K_GP_RIGHT_TRIGGER:	dc = "+attack";			goto defaultedbind;	//qs: +attack
		case K_GP_START:			dc = "togglemenu";		goto defaultedbind;
		case K_GP_A:				dc = "impulse 10";		goto defaultedbind;
		case K_GP_B:				dc = "+button4";		goto defaultedbind;
		case K_GP_X:				dc = "+button5";		goto defaultedbind;
		case K_GP_Y:				dc = "+button6";		goto defaultedbind;
		case K_GP_BACK:				dc = "+showscores";		goto defaultedbind;
		case K_GP_UNKNOWN:			dc = "+button8";		goto defaultedbind;
		case K_GP_DPAD_UP:			dc = "+forward";		goto defaultedbind;
		case K_GP_DPAD_DOWN:		dc = "+back";			goto defaultedbind;
		case K_GP_DPAD_LEFT:		dc = "+moveleft";		goto defaultedbind;
		case K_GP_DPAD_RIGHT:		dc = "+moveright";		goto defaultedbind;
		case K_GP_GUIDE:			dc = "togglemenu";		goto defaultedbind;

		case K_GP_LEFT_STICK:		dc = "+movedown";		goto defaultedbind;
		case K_GP_RIGHT_STICK:
		default:
			break;
		}

		dc = keybindings[bkey][modifierstate];
		bl = bindcmdlevel[bkey][modifierstate];
	}

defaultedbind:
	if (key == K_TOUCH || (key == K_MOUSE1 && IN_Touch_MouseIsAbs(devid)))
	{
		const char *button = SCR_ShowPics_ClickCommand(mousecursor_x, mousecursor_y, key == K_TOUCH);
		if (!button && cl.mouseplayerview && cl.mousenewtrackplayer>=0)
			Cam_Lock(cl.mouseplayerview, cl.mousenewtrackplayer);
		if (button)
		{
			dc = button;
			bl = RESTRICT_INSECURE;
		}
		else
		{
			int bkey = Sbar_TranslateHudClick();
			if (bkey)
			{
				dc = keybindings[bkey][modifierstate];
				bl = bindcmdlevel[bkey][modifierstate];
			}
			else if (!Key_MouseShouldBeFree())
				return;
		}
		IN_Touch_BlockGestures(devid);
	}
	if (key == K_TOUCHTAP && !dc)
	{	//convert to mouse1 or mouse2 when touchstrafe is on and touchtap is unbound
		bkey = IN_Touch_Fallback(devid);
		if (bkey)
		{
			dc = keybindings[bkey][modifierstate];
			bl = bindcmdlevel[bkey][modifierstate];
		}
	}

	if (dc)
	{
		if (dc[0] == '+')
		{
			uc = va("-%s%s %i\n", p, dc+1, bkey);
			dc = va("+%s%s %i\n", p, dc+1, bkey);
		}
		else
		{
			uc = NULL;
			dc = va("%s%s\n", p, dc);
		}
	}
	else
		uc = NULL;

	//don't mess up if we ran out of devices, just silently release the one that it conflicted with (and only if its in conflict).
	if (releasecommand[key][devid%MAX_INDEVS] && (!uc || strcmp(uc, releasecommand[key][devid%MAX_INDEVS])))
	{
		Cbuf_AddText (releasecommand[key][devid%MAX_INDEVS], releasecommandlevel[key][devid%MAX_INDEVS]);
		Z_Free(releasecommand[key][devid%MAX_INDEVS]);
		releasecommand[key][devid%MAX_INDEVS] = NULL;
	}
	if (dc)
		Cbuf_AddText (dc, bl);
	if (uc)
		releasecommand[key][devid%MAX_INDEVS] = Z_StrDup(uc);
	releasecommandlevel[key][devid%MAX_INDEVS] = bl;
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	for (i=0 ; i<K_MAX ; i++)
		keydown[i] = 0;
}

