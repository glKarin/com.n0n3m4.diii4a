/*
	Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/

#include "quakedef.h"
#include "cl_video.h"
#include "utf8lib.h"
#include "csprogs.h"

cvar_t con_closeontoggleconsole = {CF_CLIENT | CF_ARCHIVE, "con_closeontoggleconsole","1", "allows toggleconsole binds to close the console as well; when set to 2, this even works when not at the start of the line in console input; when set to 3, this works even if the toggleconsole key is the color tag"};

/*
key up events are sent even if in console mode
*/

char		key_line[MAX_INPUTLINE];
int			key_linepos;
qbool	key_insert = true;	// insert key toggle (for editing)
keydest_t	key_dest;
int			key_consoleactive;
char		*keybindings[MAX_BINDMAPS][MAX_KEYS];

int			history_line;
char		history_savedline[MAX_INPUTLINE];
char		history_searchstring[MAX_INPUTLINE];
qbool	history_matchfound = false;
conbuffer_t history;

extern cvar_t	con_textsize;


static void Key_History_Init(void)
{
	qfile_t *historyfile;
	ConBuffer_Init(&history, HIST_TEXTSIZE, HIST_MAXLINES, zonemempool);

// not necessary for mobile
#ifndef DP_MOBILETOUCH
	historyfile = FS_OpenRealFile("darkplaces_history.txt", "rb", false); // rb to handle unix line endings on windows too
	if(historyfile)
	{
		char buf[MAX_INPUTLINE];
		int bufpos;
		int c;

		bufpos = 0;
		for(;;)
		{
			c = FS_Getc(historyfile);
			if(c < 0 || c == 0 || c == '\r' || c == '\n')
			{
				if(bufpos > 0)
				{
					buf[bufpos] = 0;
					ConBuffer_AddLine(&history, buf, bufpos, 0);
					bufpos = 0;
				}
				if(c < 0)
					break;
			}
			else
			{
				if(bufpos < MAX_INPUTLINE - 1)
					buf[bufpos++] = c;
			}
		}

		FS_Close(historyfile);
	}
#endif

	history_line = -1;
}

static void Key_History_Shutdown(void)
{
// not necessary for mobile
#ifndef DP_MOBILETOUCH
	qfile_t *historyfile = FS_OpenRealFile("darkplaces_history.txt", "w", false);
	if(historyfile)
	{
		int i;

		Con_Print("Saving command history to darkplaces_history.txt ...\n");
		for(i = 0; i < CONBUFFER_LINES_COUNT(&history); ++i)
			FS_Printf(historyfile, "%s\n", ConBuffer_GetLine(&history, i));
		FS_Close(historyfile);
	}
	else
		Con_Print(CON_ERROR "Couldn't write darkplaces_history.txt\n");
#endif

	ConBuffer_Shutdown(&history);
}

static void Key_History_Push(void)
{
	if(key_line[1]) // empty?
	if(strcmp(key_line, "]quit")) // putting these into the history just sucks
	if(strncmp(key_line, "]quit ", 6)) // putting these into the history just sucks
	if(strcmp(key_line, "]rcon_password")) // putting these into the history just sucks
	if(strncmp(key_line, "]rcon_password ", 15)) // putting these into the history just sucks
		ConBuffer_AddLine(&history, key_line + 1, (int)strlen(key_line) - 1, 0);
	Con_Printf("%s\n", key_line); // don't mark empty lines as history
	history_line = -1;
	if (history_matchfound)
		history_matchfound = false;
}

static qbool Key_History_Get_foundCommand(void)
{
	if (!history_matchfound)
		return false;
	dp_strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
	key_linepos = (int)strlen(key_line);
	history_matchfound = false;
	return true;
}

static void Key_History_Up(void)
{
	if(history_line == -1) // editing the "new" line
		dp_strlcpy(history_savedline, key_line + 1, sizeof(history_savedline));

	if (Key_History_Get_foundCommand())
		return;

	if(history_line == -1)
	{
		history_line = CONBUFFER_LINES_COUNT(&history) - 1;
		if(history_line != -1)
		{
			dp_strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
			key_linepos = (int)strlen(key_line);
		}
	}
	else if(history_line > 0)
	{
		--history_line; // this also does -1 -> 0, so it is good
		dp_strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
		key_linepos = (int)strlen(key_line);
	}
}

static void Key_History_Down(void)
{
	if(history_line == -1) // editing the "new" line
		return;

	if (Key_History_Get_foundCommand())
		return;

	if(history_line < CONBUFFER_LINES_COUNT(&history) - 1)
	{
		++history_line;
		dp_strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
	}
	else
	{
		history_line = -1;
		dp_strlcpy(key_line + 1, history_savedline, sizeof(key_line) - 1);
	}

	key_linepos = (int)strlen(key_line);
}

static void Key_History_First(void)
{
	if(history_line == -1) // editing the "new" line
		dp_strlcpy(history_savedline, key_line + 1, sizeof(history_savedline));

	if (CONBUFFER_LINES_COUNT(&history) > 0)
	{
		history_line = 0;
		dp_strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
		key_linepos = (int)strlen(key_line);
	}
}

static void Key_History_Last(void)
{
	if(history_line == -1) // editing the "new" line
		dp_strlcpy(history_savedline, key_line + 1, sizeof(history_savedline));

	if (CONBUFFER_LINES_COUNT(&history) > 0)
	{
		history_line = CONBUFFER_LINES_COUNT(&history) - 1;
		dp_strlcpy(key_line + 1, ConBuffer_GetLine(&history, history_line), sizeof(key_line) - 1);
		key_linepos = (int)strlen(key_line);
	}
}

static void Key_History_Find_Backwards(void)
{
	int i;
	const char *partial = key_line + 1;
	char vabuf[1024];
	size_t digits = strlen(va(vabuf, sizeof(vabuf), "%i", HIST_MAXLINES));

	if (history_line == -1) // editing the "new" line
		dp_strlcpy(history_savedline, key_line + 1, sizeof(history_savedline));

	if (strcmp(key_line + 1, history_searchstring)) // different string? Start a new search
	{
		dp_strlcpy(history_searchstring, key_line + 1, sizeof(history_searchstring));
		i = CONBUFFER_LINES_COUNT(&history) - 1;
	}
	else if (history_line == -1)
		i = CONBUFFER_LINES_COUNT(&history) - 1;
	else
		i = history_line - 1;

	if (!*partial)
		partial = "*";
	else if (!( strchr(partial, '*') || strchr(partial, '?') )) // no pattern?
		partial = va(vabuf, sizeof(vabuf), "*%s*", partial);

	for ( ; i >= 0; i--)
		if (matchpattern_with_separator(ConBuffer_GetLine(&history, i), partial, true, "", false))
		{
			Con_Printf("^2%*i^7 %s\n", (int)digits, i+1, ConBuffer_GetLine(&history, i));
			history_line = i;
			history_matchfound = true;
			return;
		}
}

static void Key_History_Find_Forwards(void)
{
	int i;
	const char *partial = key_line + 1;
	char vabuf[1024];
	size_t digits = strlen(va(vabuf, sizeof(vabuf), "%i", HIST_MAXLINES));

	if (history_line == -1) // editing the "new" line
		return;

	if (strcmp(key_line + 1, history_searchstring)) // different string? Start a new search
	{
		dp_strlcpy(history_searchstring, key_line + 1, sizeof(history_searchstring));
		i = 0;
	}
	else i = history_line + 1;

	if (!*partial)
		partial = "*";
	else if (!( strchr(partial, '*') || strchr(partial, '?') )) // no pattern?
		partial = va(vabuf, sizeof(vabuf), "*%s*", partial);

	for ( ; i < CONBUFFER_LINES_COUNT(&history); i++)
		if (matchpattern_with_separator(ConBuffer_GetLine(&history, i), partial, true, "", false))
		{
			Con_Printf("^2%*i^7 %s\n", (int)digits, i+1, ConBuffer_GetLine(&history, i));
			history_line = i;
			history_matchfound = true;
			return;
		}
}

static void Key_History_Find_All(void)
{
	const char *partial = key_line + 1;
	int i, count = 0;
	char vabuf[1024];
	size_t digits = strlen(va(vabuf, sizeof(vabuf), "%i", HIST_MAXLINES));
	Con_Printf("History commands containing \"%s\":\n", key_line + 1);

	if (!*partial)
		partial = "*";
	else if (!( strchr(partial, '*') || strchr(partial, '?') )) // no pattern?
		partial = va(vabuf, sizeof(vabuf), "*%s*", partial);

	for (i=0; i<CONBUFFER_LINES_COUNT(&history); i++)
		if (matchpattern_with_separator(ConBuffer_GetLine(&history, i), partial, true, "", false))
		{
			Con_Printf("%s%*i^7 %s\n", (i == history_line) ? "^2" : "^3", (int)digits, i+1, ConBuffer_GetLine(&history, i));
			count++;
		}
	Con_Printf("%i result%s\n\n", count, (count != 1) ? "s" : "");
}

static void Key_History_f(cmd_state_t *cmd)
{
	char *errchar = NULL;
	int i = 0;
	char vabuf[1024];
	size_t digits = strlen(va(vabuf, sizeof(vabuf), "%i", HIST_MAXLINES));

	if (Cmd_Argc (cmd) > 1)
	{
		if (!strcmp(Cmd_Argv(cmd, 1), "-c"))
		{
			ConBuffer_Clear(&history);
			return;
		}
		i = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
		if ((i < 0) || (i > CONBUFFER_LINES_COUNT(&history)) || (errchar && *errchar))
			i = 0;
		else
			i = CONBUFFER_LINES_COUNT(&history) - i;
	}

	for ( ; i<CONBUFFER_LINES_COUNT(&history); i++)
		Con_Printf("^3%*i^7 %s\n", (int)digits, i+1, ConBuffer_GetLine(&history, i));
	Con_Printf("\n");
}

static int	key_bmap, key_bmap2;
static unsigned char keydown[MAX_KEYS];	// 0 = up, 1 = down, 2 = repeating

typedef struct keyname_s
{
	const char	*name;
	int			keynum;
}
keyname_t;

static const keyname_t   keynames[] = {
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},

	// spacer so it lines up with keys.h

	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"SHIFT", K_SHIFT},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"PAUSE", K_PAUSE},

	{"NUMLOCK", K_NUMLOCK},
	{"CAPSLOCK", K_CAPSLOCK},
	{"SCROLLOCK", K_SCROLLOCK},

	{"KP_INS",			K_KP_INS },
	{"KP_0", K_KP_0},
	{"KP_END",			K_KP_END },
	{"KP_1", K_KP_1},
	{"KP_DOWNARROW",	K_KP_DOWNARROW },
	{"KP_2", K_KP_2},
	{"KP_PGDN",			K_KP_PGDN },
	{"KP_3", K_KP_3},
	{"KP_LEFTARROW",	K_KP_LEFTARROW },
	{"KP_4", K_KP_4},
	{"KP_5", K_KP_5},
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW },
	{"KP_6", K_KP_6},
	{"KP_HOME",			K_KP_HOME },
	{"KP_7", K_KP_7},
	{"KP_UPARROW",		K_KP_UPARROW },
	{"KP_8", K_KP_8},
	{"KP_PGUP",			K_KP_PGUP },
	{"KP_9", K_KP_9},
	{"KP_DEL",			K_KP_DEL },
	{"KP_PERIOD", K_KP_PERIOD},
	{"KP_SLASH",		K_KP_SLASH },
	{"KP_DIVIDE", K_KP_DIVIDE},
	{"KP_MULTIPLY", K_KP_MULTIPLY},
	{"KP_MINUS", K_KP_MINUS},
	{"KP_PLUS", K_KP_PLUS},
	{"KP_ENTER", K_KP_ENTER},
	{"KP_EQUALS", K_KP_EQUALS},

	{"PRINTSCREEN", K_PRINTSCREEN},



	{"MOUSE1", K_MOUSE1},

	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MWHEELUP", K_MWHEELUP},
	{"MWHEELDOWN", K_MWHEELDOWN},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},
	{"MOUSE6", K_MOUSE6},
	{"MOUSE7", K_MOUSE7},
	{"MOUSE8", K_MOUSE8},
	{"MOUSE9", K_MOUSE9},
	{"MOUSE10", K_MOUSE10},
	{"MOUSE11", K_MOUSE11},
	{"MOUSE12", K_MOUSE12},
	{"MOUSE13", K_MOUSE13},
	{"MOUSE14", K_MOUSE14},
	{"MOUSE15", K_MOUSE15},
	{"MOUSE16", K_MOUSE16},




	{"JOY1",  K_JOY1},
	{"JOY2",  K_JOY2},
	{"JOY3",  K_JOY3},
	{"JOY4",  K_JOY4},
	{"JOY5",  K_JOY5},
	{"JOY6",  K_JOY6},
	{"JOY7",  K_JOY7},
	{"JOY8",  K_JOY8},
	{"JOY9",  K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},






	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"X360_DPAD_UP", K_X360_DPAD_UP},
	{"X360_DPAD_DOWN", K_X360_DPAD_DOWN},
	{"X360_DPAD_LEFT", K_X360_DPAD_LEFT},
	{"X360_DPAD_RIGHT", K_X360_DPAD_RIGHT},
	{"X360_START", K_X360_START},
	{"X360_BACK", K_X360_BACK},
	{"X360_LEFT_THUMB", K_X360_LEFT_THUMB},
	{"X360_RIGHT_THUMB", K_X360_RIGHT_THUMB},
	{"X360_LEFT_SHOULDER", K_X360_LEFT_SHOULDER},
	{"X360_RIGHT_SHOULDER", K_X360_RIGHT_SHOULDER},
	{"X360_A", K_X360_A},
	{"X360_B", K_X360_B},
	{"X360_X", K_X360_X},
	{"X360_Y", K_X360_Y},
	{"X360_LEFT_TRIGGER", K_X360_LEFT_TRIGGER},
	{"X360_RIGHT_TRIGGER", K_X360_RIGHT_TRIGGER},
	{"X360_LEFT_THUMB_UP", K_X360_LEFT_THUMB_UP},
	{"X360_LEFT_THUMB_DOWN", K_X360_LEFT_THUMB_DOWN},
	{"X360_LEFT_THUMB_LEFT", K_X360_LEFT_THUMB_LEFT},
	{"X360_LEFT_THUMB_RIGHT", K_X360_LEFT_THUMB_RIGHT},
	{"X360_RIGHT_THUMB_UP", K_X360_RIGHT_THUMB_UP},
	{"X360_RIGHT_THUMB_DOWN", K_X360_RIGHT_THUMB_DOWN},
	{"X360_RIGHT_THUMB_LEFT", K_X360_RIGHT_THUMB_LEFT},
	{"X360_RIGHT_THUMB_RIGHT", K_X360_RIGHT_THUMB_RIGHT},

	{"JOY_UP", K_JOY_UP},
	{"JOY_DOWN", K_JOY_DOWN},
	{"JOY_LEFT", K_JOY_LEFT},
	{"JOY_RIGHT", K_JOY_RIGHT},

	{"SEMICOLON", ';'},			// because a raw semicolon separates commands
	{"TILDE", '~'},
	{"BACKQUOTE", '`'},
	{"QUOTE", '"'},
	{"APOSTROPHE", '\''},
	{"BACKSLASH", '\\'},		// because a raw backslash is used for special characters

	{"MIDINOTE0", K_MIDINOTE0},
	{"MIDINOTE1", K_MIDINOTE1},
	{"MIDINOTE2", K_MIDINOTE2},
	{"MIDINOTE3", K_MIDINOTE3},
	{"MIDINOTE4", K_MIDINOTE4},
	{"MIDINOTE5", K_MIDINOTE5},
	{"MIDINOTE6", K_MIDINOTE6},
	{"MIDINOTE7", K_MIDINOTE7},
	{"MIDINOTE8", K_MIDINOTE8},
	{"MIDINOTE9", K_MIDINOTE9},
	{"MIDINOTE10", K_MIDINOTE10},
	{"MIDINOTE11", K_MIDINOTE11},
	{"MIDINOTE12", K_MIDINOTE12},
	{"MIDINOTE13", K_MIDINOTE13},
	{"MIDINOTE14", K_MIDINOTE14},
	{"MIDINOTE15", K_MIDINOTE15},
	{"MIDINOTE16", K_MIDINOTE16},
	{"MIDINOTE17", K_MIDINOTE17},
	{"MIDINOTE18", K_MIDINOTE18},
	{"MIDINOTE19", K_MIDINOTE19},
	{"MIDINOTE20", K_MIDINOTE20},
	{"MIDINOTE21", K_MIDINOTE21},
	{"MIDINOTE22", K_MIDINOTE22},
	{"MIDINOTE23", K_MIDINOTE23},
	{"MIDINOTE24", K_MIDINOTE24},
	{"MIDINOTE25", K_MIDINOTE25},
	{"MIDINOTE26", K_MIDINOTE26},
	{"MIDINOTE27", K_MIDINOTE27},
	{"MIDINOTE28", K_MIDINOTE28},
	{"MIDINOTE29", K_MIDINOTE29},
	{"MIDINOTE30", K_MIDINOTE30},
	{"MIDINOTE31", K_MIDINOTE31},
	{"MIDINOTE32", K_MIDINOTE32},
	{"MIDINOTE33", K_MIDINOTE33},
	{"MIDINOTE34", K_MIDINOTE34},
	{"MIDINOTE35", K_MIDINOTE35},
	{"MIDINOTE36", K_MIDINOTE36},
	{"MIDINOTE37", K_MIDINOTE37},
	{"MIDINOTE38", K_MIDINOTE38},
	{"MIDINOTE39", K_MIDINOTE39},
	{"MIDINOTE40", K_MIDINOTE40},
	{"MIDINOTE41", K_MIDINOTE41},
	{"MIDINOTE42", K_MIDINOTE42},
	{"MIDINOTE43", K_MIDINOTE43},
	{"MIDINOTE44", K_MIDINOTE44},
	{"MIDINOTE45", K_MIDINOTE45},
	{"MIDINOTE46", K_MIDINOTE46},
	{"MIDINOTE47", K_MIDINOTE47},
	{"MIDINOTE48", K_MIDINOTE48},
	{"MIDINOTE49", K_MIDINOTE49},
	{"MIDINOTE50", K_MIDINOTE50},
	{"MIDINOTE51", K_MIDINOTE51},
	{"MIDINOTE52", K_MIDINOTE52},
	{"MIDINOTE53", K_MIDINOTE53},
	{"MIDINOTE54", K_MIDINOTE54},
	{"MIDINOTE55", K_MIDINOTE55},
	{"MIDINOTE56", K_MIDINOTE56},
	{"MIDINOTE57", K_MIDINOTE57},
	{"MIDINOTE58", K_MIDINOTE58},
	{"MIDINOTE59", K_MIDINOTE59},
	{"MIDINOTE60", K_MIDINOTE60},
	{"MIDINOTE61", K_MIDINOTE61},
	{"MIDINOTE62", K_MIDINOTE62},
	{"MIDINOTE63", K_MIDINOTE63},
	{"MIDINOTE64", K_MIDINOTE64},
	{"MIDINOTE65", K_MIDINOTE65},
	{"MIDINOTE66", K_MIDINOTE66},
	{"MIDINOTE67", K_MIDINOTE67},
	{"MIDINOTE68", K_MIDINOTE68},
	{"MIDINOTE69", K_MIDINOTE69},
	{"MIDINOTE70", K_MIDINOTE70},
	{"MIDINOTE71", K_MIDINOTE71},
	{"MIDINOTE72", K_MIDINOTE72},
	{"MIDINOTE73", K_MIDINOTE73},
	{"MIDINOTE74", K_MIDINOTE74},
	{"MIDINOTE75", K_MIDINOTE75},
	{"MIDINOTE76", K_MIDINOTE76},
	{"MIDINOTE77", K_MIDINOTE77},
	{"MIDINOTE78", K_MIDINOTE78},
	{"MIDINOTE79", K_MIDINOTE79},
	{"MIDINOTE80", K_MIDINOTE80},
	{"MIDINOTE81", K_MIDINOTE81},
	{"MIDINOTE82", K_MIDINOTE82},
	{"MIDINOTE83", K_MIDINOTE83},
	{"MIDINOTE84", K_MIDINOTE84},
	{"MIDINOTE85", K_MIDINOTE85},
	{"MIDINOTE86", K_MIDINOTE86},
	{"MIDINOTE87", K_MIDINOTE87},
	{"MIDINOTE88", K_MIDINOTE88},
	{"MIDINOTE89", K_MIDINOTE89},
	{"MIDINOTE90", K_MIDINOTE90},
	{"MIDINOTE91", K_MIDINOTE91},
	{"MIDINOTE92", K_MIDINOTE92},
	{"MIDINOTE93", K_MIDINOTE93},
	{"MIDINOTE94", K_MIDINOTE94},
	{"MIDINOTE95", K_MIDINOTE95},
	{"MIDINOTE96", K_MIDINOTE96},
	{"MIDINOTE97", K_MIDINOTE97},
	{"MIDINOTE98", K_MIDINOTE98},
	{"MIDINOTE99", K_MIDINOTE99},
	{"MIDINOTE100", K_MIDINOTE100},
	{"MIDINOTE101", K_MIDINOTE101},
	{"MIDINOTE102", K_MIDINOTE102},
	{"MIDINOTE103", K_MIDINOTE103},
	{"MIDINOTE104", K_MIDINOTE104},
	{"MIDINOTE105", K_MIDINOTE105},
	{"MIDINOTE106", K_MIDINOTE106},
	{"MIDINOTE107", K_MIDINOTE107},
	{"MIDINOTE108", K_MIDINOTE108},
	{"MIDINOTE109", K_MIDINOTE109},
	{"MIDINOTE110", K_MIDINOTE110},
	{"MIDINOTE111", K_MIDINOTE111},
	{"MIDINOTE112", K_MIDINOTE112},
	{"MIDINOTE113", K_MIDINOTE113},
	{"MIDINOTE114", K_MIDINOTE114},
	{"MIDINOTE115", K_MIDINOTE115},
	{"MIDINOTE116", K_MIDINOTE116},
	{"MIDINOTE117", K_MIDINOTE117},
	{"MIDINOTE118", K_MIDINOTE118},
	{"MIDINOTE119", K_MIDINOTE119},
	{"MIDINOTE120", K_MIDINOTE120},
	{"MIDINOTE121", K_MIDINOTE121},
	{"MIDINOTE122", K_MIDINOTE122},
	{"MIDINOTE123", K_MIDINOTE123},
	{"MIDINOTE124", K_MIDINOTE124},
	{"MIDINOTE125", K_MIDINOTE125},
	{"MIDINOTE126", K_MIDINOTE126},
	{"MIDINOTE127", K_MIDINOTE127},

	{NULL, 0}
};

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

int Key_ClearEditLine(qbool is_console)
{
	if (is_console)
	{
		key_line[0] = ']';
		key_line[1] = 0;
		return 1;
	}
	else
	{
		chat_buffer[0] = 0;
		return 0;
	}
}

// key modifier states
#define KM_NONE           (!keydown[K_CTRL] && !keydown[K_SHIFT] && !keydown[K_ALT])
#define KM_CTRL_SHIFT_ALT ( keydown[K_CTRL] &&  keydown[K_SHIFT] &&  keydown[K_ALT])
#define KM_CTRL_SHIFT     ( keydown[K_CTRL] &&  keydown[K_SHIFT] && !keydown[K_ALT])
#define KM_CTRL_ALT       ( keydown[K_CTRL] && !keydown[K_SHIFT] &&  keydown[K_ALT])
#define KM_SHIFT_ALT      (!keydown[K_CTRL] &&  keydown[K_SHIFT] &&  keydown[K_ALT])
#define KM_CTRL           ( keydown[K_CTRL] && !keydown[K_SHIFT] && !keydown[K_ALT])
#define KM_SHIFT          (!keydown[K_CTRL] &&  keydown[K_SHIFT] && !keydown[K_ALT])
#define KM_ALT            (!keydown[K_CTRL] && !keydown[K_SHIFT] &&  keydown[K_ALT])

/*
====================
Interactive line editing and console scrollback
====================
*/

signed char chat_mode; // 0 for say, 1 for say_team, -1 for command
char chat_buffer[MAX_INPUTLINE];
int chat_bufferpos = 0;

int Key_AddChar(int unicode, qbool is_console)
{
	char *line;
	char buf[16];
	int len, blen, linepos;

	if (is_console)
	{
		line = key_line;
		linepos = key_linepos;
	}
	else
	{
		line = chat_buffer;
		linepos = chat_bufferpos;
	}

	if (linepos >= MAX_INPUTLINE-1)
		return linepos;

	blen = u8_fromchar(unicode, buf, sizeof(buf));
	if (!blen)
		return linepos;
	len = (int)strlen(&line[linepos]);
	// check insert mode, or always insert if at end of line
	if (key_insert || len == 0)
	{
		if (linepos + len + blen >= MAX_INPUTLINE)
			return linepos;
		// can't use strcpy to move string to right
		len++;
		if (linepos + blen + len >= MAX_INPUTLINE)
			return linepos;
		memmove(&line[linepos + blen], &line[linepos], len);
	}
	else if (linepos + len + blen - u8_bytelen(line + linepos, 1) >= MAX_INPUTLINE)
		return linepos;
	memcpy(line + linepos, buf, blen);
	if (blen > len)
		line[linepos + blen] = 0;
	linepos += blen;
	return linepos;
}

// returns -1 if no key has been recognized
// returns linepos (>= 0) otherwise
// if is_console is true can modify key_line (doesn't change key_linepos)
int Key_Parse_CommonKeys(cmd_state_t *cmd, qbool is_console, int key, int unicode)
{
	char *line;
	int linepos, linestart;
	unsigned int linesize;
	if (is_console)
	{
		line = key_line;
		linepos = key_linepos;
		linesize = sizeof(key_line);
		linestart = 1;
	}
	else
	{
		line = chat_buffer;
		linepos = chat_bufferpos;
		linesize = sizeof(chat_buffer);
		linestart = 0;
	}

	if ((key == 'v' && KM_CTRL) || ((key == K_INS || key == K_KP_INS) && KM_SHIFT))
	{
		char *cbd, *p;
		if ((cbd = Sys_SDL_GetClipboardData()) != 0)
		{
			int i;
#if 1
			p = cbd;
			while (*p)
			{
				if (*p == '\r' && *(p+1) == '\n')
				{
					*p++ = ';';
					*p++ = ' ';
				}
				else if (*p == '\n' || *p == '\r' || *p == '\b')
					*p++ = ';';
				else
					p++;
			}
#else
			strtok(cbd, "\n\r\b");
#endif
			i = (int)strlen(cbd);
			if (i + linepos >= MAX_INPUTLINE)
				i= MAX_INPUTLINE - linepos - 1;
			if (i > 0)
			{
				cbd[i] = 0;
				memmove(line + linepos + i, line + linepos, linesize - linepos - i);
				memcpy(line + linepos, cbd, i);
				linepos += i;
			}
			Z_Free(cbd);
		}
		return linepos;
	}

	if (key == 'u' && KM_CTRL) // like vi/readline ^u: delete currently edited line
	{
		return Key_ClearEditLine(is_console);
	}

	if (key == K_TAB)
	{
		if (is_console && KM_CTRL) // append the cvar value to the cvar name
		{
			int		cvar_len, cvar_str_len, chars_to_move;
			char	k;
			char	cvar[MAX_INPUTLINE];
			const char *cvar_str;

			// go to the start of the variable
			while(--linepos)
			{
				k = line[linepos];
				if(k == '\"' || k == ';' || k == ' ' || k == '\'')
					break;
			}
			linepos++;

			// save the variable name in cvar
			for(cvar_len=0; (k = line[linepos + cvar_len]) != 0; cvar_len++)
			{
				if(k == '\"' || k == ';' || k == ' ' || k == '\'')
					break;
				cvar[cvar_len] = k;
			}
			if (cvar_len==0)
				return linepos;
			cvar[cvar_len] = 0;

			// go to the end of the cvar
			linepos += cvar_len;

			// save the content of the variable in cvar_str
			cvar_str = Cvar_VariableString(&cvars_all, cvar, CF_CLIENT | CF_SERVER);
			cvar_str_len = (int)strlen(cvar_str);
			if (cvar_str_len==0)
				return linepos;

			// insert space and cvar_str in line
			chars_to_move = (int)strlen(&line[linepos]);
			if (linepos + 1 + cvar_str_len + chars_to_move < MAX_INPUTLINE)
			{
				if (chars_to_move)
					memmove(&line[linepos + 1 + cvar_str_len], &line[linepos], chars_to_move);
				line[linepos++] = ' ';
				memcpy(&line[linepos], cvar_str, cvar_str_len);
				linepos += cvar_str_len;
				line[linepos + chars_to_move] = 0;
			}
			else
				Con_Printf("Couldn't append cvar value, edit line too long.\n");
			return linepos;
		}

		if (KM_NONE)
			return Con_CompleteCommandLine(cmd, is_console);
	}

	// Advanced Console Editing by Radix radix@planetquake.com
	// Added/Modified by EvilTypeGuy eviltypeguy@qeradiant.com
	// Enhanced by [515]
	// Enhanced by terencehill

	// move cursor to the previous character
	if (key == K_LEFTARROW || key == K_KP_LEFTARROW)
	{
		if(KM_CTRL) // move cursor to the previous word
		{
			int		pos;
			char	k;
			if (linepos <= linestart + 1)
				return linestart;
			pos = linepos;

			do {
				k = line[--pos];
				if (!(k == '\"' || k == ';' || k == ' ' || k == '\''))
					break;
			} while(pos > linestart); // skip all "; ' after the word

			if (pos == linestart)
				return linestart;

			do {
				k = line[--pos];
				if (k == '\"' || k == ';' || k == ' ' || k == '\'')
				{
					pos++;
					break;
				}
			} while(pos > linestart);

			linepos = pos;
			return linepos;
		}

		if(KM_SHIFT) // move cursor to the previous character ignoring colors
		{
			int		pos;
			size_t          inchar = 0;
			if (linepos <= linestart + 1)
				return linestart;
			pos = (int)u8_prevbyte(line + linestart, linepos - linestart) + linestart;
			while (pos > linestart)
				if(pos-1 >= linestart && line[pos-1] == STRING_COLOR_TAG && isdigit(line[pos]))
					pos-=2;
				else if(pos-4 >= linestart && line[pos-4] == STRING_COLOR_TAG && line[pos-3] == STRING_COLOR_RGB_TAG_CHAR
						&& isxdigit(line[pos-2]) && isxdigit(line[pos-1]) && isxdigit(line[pos]))
					pos-=5;
				else
				{
					if(pos-1 >= linestart && line[pos-1] == STRING_COLOR_TAG && line[pos] == STRING_COLOR_TAG) // consider ^^ as a character
						pos--;
					pos--;
					break;
				}
			if (pos < linestart)
				return linestart;
			// we need to move to the beginning of the character when in a wide character:
			u8_charidx(line, pos + 1, &inchar);
			linepos = (int)(pos + 1 - inchar);
			return linepos;
		}

		if(KM_NONE)
		{
			if (linepos <= linestart + 1)
				return linestart;
			// hide ']' from u8_prevbyte otherwise it could go out of bounds
			linepos = (int)u8_prevbyte(line + linestart, linepos - linestart) + linestart;
			return linepos;
		}
	}

	// delete char before cursor
	if ((key == K_BACKSPACE && KM_NONE) || (key == 'h' && KM_CTRL))
	{
		if (linepos > linestart)
		{
			// hide ']' from u8_prevbyte otherwise it could go out of bounds
			int newpos = (int)u8_prevbyte(line + linestart, linepos - linestart) + linestart;
			dp_strlcpy(line + newpos, line + linepos, linesize + 1 - linepos);
			linepos = newpos;
		}
		return linepos;
	}

	// delete char on cursor
	if ((key == K_DEL || key == K_KP_DEL) && KM_NONE)
	{
		size_t linelen;
		linelen = strlen(line);
		if (linepos < (int)linelen)
			memmove(line + linepos, line + linepos + u8_bytelen(line + linepos, 1), linelen - linepos);
		return linepos;
	}

	// move cursor to the next character
	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
	{
		if (KM_CTRL) // move cursor to the next word
		{
			int		pos, len;
			char	k;
			len = (int)strlen(line);
			if (linepos >= len)
				return linepos;
			pos = linepos;

			while(++pos < len)
			{
				k = line[pos];
				if(k == '\"' || k == ';' || k == ' ' || k == '\'')
					break;
			}

			if (pos < len) // skip all "; ' after the word
				while(++pos < len)
				{
					k = line[pos];
					if (!(k == '\"' || k == ';' || k == ' ' || k == '\''))
						break;
				}
			linepos = pos;
			return linepos;
		}

		if (KM_SHIFT) // move cursor to the next character ignoring colors
		{
			int		pos, len;
			len = (int)strlen(line);
			if (linepos >= len)
				return linepos;
			pos = linepos;

			// go beyond all initial consecutive color tags, if any
			if(pos < len)
				while (line[pos] == STRING_COLOR_TAG)
				{
					if(isdigit(line[pos+1]))
						pos+=2;
					else if(line[pos+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(line[pos+2]) && isxdigit(line[pos+3]) && isxdigit(line[pos+4]))
						pos+=5;
					else
						break;
				}

			// skip the char
			if (line[pos] == STRING_COLOR_TAG && line[pos+1] == STRING_COLOR_TAG) // consider ^^ as a character
				pos++;
			pos += (int)u8_bytelen(line + pos, 1);

			// now go beyond all next consecutive color tags, if any
			if(pos < len)
				while (line[pos] == STRING_COLOR_TAG)
				{
					if(isdigit(line[pos+1]))
						pos+=2;
					else if(line[pos+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(line[pos+2]) && isxdigit(line[pos+3]) && isxdigit(line[pos+4]))
						pos+=5;
					else
						break;
				}
			linepos = pos;
			return linepos;
		}

		if (KM_NONE)
		{
			if (linepos >= (int)strlen(line))
				return linepos;
			linepos += (int)u8_bytelen(line + linepos, 1);
			return linepos;
		}
	}

	if ((key == K_INS || key == K_KP_INS) && KM_NONE) // toggle insert mode
	{
		key_insert ^= 1;
		return linepos;
	}

	if (key == K_HOME || key == K_KP_HOME)
	{
		if (is_console && KM_CTRL)
		{
			con_backscroll = CON_TEXTSIZE;
			return linepos;
		}
		if (KM_NONE)
			return linestart;
	}

	if (key == K_END || key == K_KP_END)
	{
		if (is_console && KM_CTRL)
		{
			con_backscroll = 0;
			return linepos;
		}
		if (KM_NONE)
			return (int)strlen(line);
	}

	return -1;
}

static int Key_Convert_NumPadKey(int key)
{
	// LadyHavoc: copied most of this from Q2 to improve keyboard handling
	switch (key)
	{
		case K_KP_SLASH:      return '/';
		case K_KP_MINUS:      return '-';
		case K_KP_PLUS:       return '+';
		case K_KP_HOME:       return '7';
		case K_KP_UPARROW:    return '8';
		case K_KP_PGUP:       return '9';
		case K_KP_LEFTARROW:  return '4';
		case K_KP_5:          return '5';
		case K_KP_RIGHTARROW: return '6';
		case K_KP_END:        return '1';
		case K_KP_DOWNARROW:  return '2';
		case K_KP_PGDN:       return '3';
		case K_KP_INS:        return '0';
		case K_KP_DEL:        return '.';
	}
	return key;
}

static void Key_Console(cmd_state_t *cmd, int key, int unicode)
{
	int linepos;

	key = Key_Convert_NumPadKey(key);

	// Forbid Ctrl Alt shortcuts since on Windows they are used to type some characters
	// in certain non-English keyboards using the AltGr key (which emulates Ctrl Alt)
	// Reference: "Why Ctrl+Alt shouldn't be used as a shortcut modifier"
	//            https://blogs.msdn.microsoft.com/oldnewthing/20040329-00/?p=40003
	if (keydown[K_CTRL] && keydown[K_ALT])
		goto add_char;

	linepos = Key_Parse_CommonKeys(cmd, true, key, unicode);
	if (linepos >= 0)
	{
		key_linepos = linepos;
		return;
	}

	if ((key == K_ENTER || key == K_KP_ENTER) && KM_NONE)
	{
		// bones_was_here: prepending allows a loop such as `alias foo "bar; wait; foo"; foo`
		// to be broken with an alias or unalias command
		Cbuf_InsertText(cmd, key_line+1); // skip the ]
		Key_History_Push();
		key_linepos = Key_ClearEditLine(true);
		// force an update, because the command may take some time
		if (cls.state == ca_disconnected)
			CL_UpdateScreen ();
		return;
	}

	if (key == 'l' && KM_CTRL)
	{
		Cbuf_AddText (cmd, "clear\n");
		return;
	}

	if (key == 'q' && KM_CTRL) // like zsh ^q: push line to history, don't execute, and clear
	{
		// clear line
		Key_History_Push();
		key_linepos = Key_ClearEditLine(true);
		return;
	}

	// End Advanced Console Editing

	if (((key == K_UPARROW || key == K_KP_UPARROW) && KM_NONE) || (key == 'p' && KM_CTRL))
	{
		Key_History_Up();
		return;
	}

	if (((key == K_DOWNARROW || key == K_KP_DOWNARROW) && KM_NONE) || (key == 'n' && KM_CTRL))
	{
		Key_History_Down();
		return;
	}

	if (keydown[K_CTRL])
	{
		// prints all the matching commands
		if (key == 'f' && KM_CTRL)
		{
			Key_History_Find_All();
			return;
		}
		// Search forwards/backwards, pointing the history's index to the
		// matching command but without fetching it to let one continue the search.
		// To fetch it, it suffices to just press UP or DOWN.
		if (key == 'r' && KM_CTRL_SHIFT)
		{
			Key_History_Find_Forwards();
			return;
		}
		if (key == 'r' && KM_CTRL)
		{
			Key_History_Find_Backwards();
			return;
		}

		// go to the last/first command of the history
		if (key == ',' && KM_CTRL)
		{
			Key_History_First();
			return;
		}
		if (key == '.' && KM_CTRL)
		{
			Key_History_Last();
			return;
		}
	}

	if (key == K_PGUP || key == K_KP_PGUP)
	{
		if (KM_CTRL)
		{
			con_backscroll += ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}
		if (KM_NONE)
		{
			con_backscroll += ((vid_conheight.integer >> 1) / con_textsize.integer)-3;
			return;
		}
	}

	if (key == K_PGDN || key == K_KP_PGDN)
	{
		if (KM_CTRL)
		{
			con_backscroll -= ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}
		if (KM_NONE)
		{
			con_backscroll -= ((vid_conheight.integer >> 1) / con_textsize.integer)-3;
			return;
		}
	}

	if (key == K_MWHEELUP)
	{
		if (KM_CTRL)
		{
			con_backscroll += 1;
			return;
		}
		if (KM_SHIFT)
		{
			con_backscroll += ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}
		if (KM_NONE)
		{
			con_backscroll += 5;
			return;
		}
	}

	if (key == K_MWHEELDOWN)
	{
		if (KM_CTRL)
		{
			con_backscroll -= 1;
			return;
		}
		if (KM_SHIFT)
		{
			con_backscroll -= ((vid_conheight.integer >> 2) / con_textsize.integer)-1;
			return;
		}
		if (KM_NONE)
		{
			con_backscroll -= 5;
			return;
		}
	}

	if (keydown[K_CTRL])
	{
		// text zoom in
		if ((key == '=' || key == '+' || key == K_KP_PLUS) && KM_CTRL)
		{
			if (con_textsize.integer < 128)
				Cvar_SetValueQuick(&con_textsize, con_textsize.integer + 1);
			return;
		}
		// text zoom out
		if ((key == '-' || key == K_KP_MINUS) && KM_CTRL)
		{
			if (con_textsize.integer > 1)
				Cvar_SetValueQuick(&con_textsize, con_textsize.integer - 1);
			return;
		}
		// text zoom reset
		if ((key == '0' || key == K_KP_INS) && KM_CTRL)
		{
			Cvar_SetValueQuick(&con_textsize, atoi(Cvar_VariableDefString(&cvars_all, "con_textsize", CF_CLIENT | CF_SERVER)));
			return;
		}
	}

add_char:

	// non printable
	if (unicode < 32)
		return;

	key_linepos = Key_AddChar(unicode, true);
}

//============================================================================

static void
Key_Message (cmd_state_t *cmd, int key, int ascii)
{
	int linepos;
	char vabuf[1024];

	key = Key_Convert_NumPadKey(key);

	if (key == K_ENTER || key == K_KP_ENTER || ascii == 10 || ascii == 13)
	{
		if(chat_mode < 0)
			Cmd_ExecuteString(cmd, chat_buffer, strlen(chat_buffer), src_local, true); // not Cbuf_AddText to allow semiclons in args; however, this allows no variables then. Use aliases!
		else
			CL_ForwardToServer(va(vabuf, sizeof(vabuf), "%s %s", chat_mode ? "say_team" : "say ", chat_buffer));

		key_dest = key_game;
		chat_bufferpos = Key_ClearEditLine(false);
		return;
	}

	if (key == K_ESCAPE) {
		key_dest = key_game;
		chat_bufferpos = Key_ClearEditLine(false);
		return;
	}

	linepos = Key_Parse_CommonKeys(cmd, false, key, ascii);
	if (linepos >= 0)
	{
		chat_bufferpos = linepos;
		return;
	}

	// ctrl+key generates an ascii value < 32 and shows a char from the charmap
	if (ascii > 0 && ascii < 32 && utf8_enable.integer)
		ascii = 0xE000 + ascii;

	if (!ascii)
		return;							// non printable

	chat_bufferpos = Key_AddChar(ascii, false);
}

//============================================================================


/*
===================
Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int
Key_StringToKeynum (const char *str)
{
	Uchar ch;
	const keyname_t  *kn;

	if (!str || !str[0])
		return -1;
	if (!str[1])
		return tolower(str[0]);

	for (kn = keynames; kn->name; kn++) {
		if (!strcasecmp (str, kn->name))
			return kn->keynum;
	}

	// non-ascii keys are Unicode codepoints, so give the character if it's valid;
	// error message have more than one character, don't allow it
	ch = u8_getnchar(str, &str, 3);
	return (ch == 0 || *str != 0) ? -1 : (int)ch;
}

/*
===================
Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
const char *
Key_KeynumToString (int keynum, char *tinystr, size_t tinystrlength)
{
	const keyname_t  *kn;

	// -1 is an invalid code
	if (keynum < 0)
		return "<KEY NOT FOUND>";

	// search overrides first, because some characters are special
	for (kn = keynames; kn->name; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	// if it is printable, output it as a single character
	if (keynum > 32)
	{
		u8_fromchar(keynum, tinystr, tinystrlength);
		return tinystr;
	}

	// if it is not overridden and not printable, we don't know what to do with it
	return "<UNKNOWN KEYNUM>";
}


qbool
Key_SetBinding (int keynum, int bindmap, const char *binding)
{
	char *newbinding;
	size_t l;

	if (keynum == -1 || keynum >= MAX_KEYS)
		return false;
	if ((bindmap < 0) || (bindmap >= MAX_BINDMAPS))
		return false;

// free old bindings
	if (keybindings[bindmap][keynum]) {
		Z_Free (keybindings[bindmap][keynum]);
		keybindings[bindmap][keynum] = NULL;
	}
	if(!binding[0]) // make "" binds be removed --blub
		return true;
// allocate memory for new binding
	l = strlen (binding);
	newbinding = (char *)Z_Malloc (l + 1);
	memcpy (newbinding, binding, l + 1);
	newbinding[l] = 0;
	keybindings[bindmap][keynum] = newbinding;
	return true;
}

void Key_GetBindMap(int *fg, int *bg)
{
	if(fg)
		*fg = key_bmap;
	if(bg)
		*bg = key_bmap2;
}

qbool Key_SetBindMap(int fg, int bg)
{
	if(fg >= MAX_BINDMAPS)
		return false;
	if(bg >= MAX_BINDMAPS)
		return false;
	if(fg >= 0)
		key_bmap = fg;
	if(bg >= 0)
		key_bmap2 = bg;
	return true;
}

static void
Key_In_Unbind_f(cmd_state_t *cmd)
{
	int         b, m;
	char *errchar = NULL;

	if (Cmd_Argc (cmd) != 3) {
		Con_Print("in_unbind <bindmap> <key> : remove commands from a key\n");
		return;
	}

	m = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
	if ((m < 0) || (m >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(cmd, 1));
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv(cmd, 2));
	if (b == -1) {
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(cmd, 2));
		return;
	}

	if(!Key_SetBinding (b, m, ""))
		Con_Printf("Key_SetBinding failed for unknown reason\n");
}

static void
Key_In_Bind_f(cmd_state_t *cmd)
{
	int         i, c, b, m;
	char        line[MAX_INPUTLINE];
	char *errchar = NULL;

	c = Cmd_Argc (cmd);

	if (c != 3 && c != 4) {
		Con_Print("in_bind <bindmap> <key> [command] : attach a command to a key\n");
		return;
	}

	m = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
	if ((m < 0) || (m >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(cmd, 1));
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv(cmd, 2));
	if (b == -1 || b >= MAX_KEYS) {
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(cmd, 2));
		return;
	}

	if (c == 3) {
		if (keybindings[m][b])
			Con_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(cmd, 2), keybindings[m][b]);
		else
			Con_Printf("\"%s\" is not bound\n", Cmd_Argv(cmd, 2));
		return;
	}
// copy the rest of the command line
	line[0] = 0;							// start out with a null string
	for (i = 3; i < c; i++) {
		dp_strlcat (line, Cmd_Argv(cmd, i), sizeof (line));
		if (i != (c - 1))
			dp_strlcat (line, " ", sizeof (line));
	}

	if(!Key_SetBinding (b, m, line))
		Con_Printf("Key_SetBinding failed for unknown reason\n");
}

static void
Key_In_Bindmap_f(cmd_state_t *cmd)
{
	int         m1, m2, c;
	char *errchar = NULL;

	c = Cmd_Argc (cmd);

	if (c != 3) {
		Con_Print("in_bindmap <bindmap> <fallback>: set current bindmap and fallback\n");
		return;
	}

	m1 = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
	if ((m1 < 0) || (m1 >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(cmd, 1));
		return;
	}

	m2 = strtol(Cmd_Argv(cmd, 2), &errchar, 0);
	if ((m2 < 0) || (m2 >= MAX_BINDMAPS) || (errchar && *errchar)) {
		Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(cmd, 2));
		return;
	}

	key_bmap = m1;
	key_bmap2 = m2;
}

static void
Key_Unbind_f(cmd_state_t *cmd)
{
	int         b;

	if (Cmd_Argc (cmd) != 2) {
		Con_Print("unbind <key> : remove commands from a key\n");
		return;
	}

	b = Key_StringToKeynum (Cmd_Argv(cmd, 1));
	if (b == -1) {
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(cmd, 1));
		return;
	}

	if(!Key_SetBinding (b, 0, ""))
		Con_Printf("Key_SetBinding failed for unknown reason\n");
}

static void
Key_Unbindall_f(cmd_state_t *cmd)
{
	int         i, j;

	for (j = 0; j < MAX_BINDMAPS; j++)
		for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
			if (keybindings[j][i])
				Key_SetBinding (i, j, "");
}

static void
Key_PrintBindList(int j)
{
	char bindbuf[MAX_INPUTLINE];
	char tinystr[TINYSTR_LEN];
	const char *p;
	int i;

	for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
	{
		p = keybindings[j][i];
		if (p)
		{
			Cmd_QuoteString(bindbuf, sizeof(bindbuf), p, "\"\\", false);
			if (j == 0)
				Con_Printf("^2%s ^7= \"%s\"\n", Key_KeynumToString (i, tinystr, TINYSTR_LEN), bindbuf);
			else
				Con_Printf("^3bindmap %d: ^2%s ^7= \"%s\"\n", j, Key_KeynumToString (i, tinystr, TINYSTR_LEN), bindbuf);
		}
	}
}

static void
Key_In_BindList_f(cmd_state_t *cmd)
{
	int m;
	char *errchar = NULL;

	if(Cmd_Argc(cmd) >= 2)
	{
		m = strtol(Cmd_Argv(cmd, 1), &errchar, 0);
		if ((m < 0) || (m >= MAX_BINDMAPS) || (errchar && *errchar)) {
			Con_Printf("%s isn't a valid bindmap\n", Cmd_Argv(cmd, 1));
			return;
		}
		Key_PrintBindList(m);
	}
	else
	{
		for (m = 0; m < MAX_BINDMAPS; m++)
			Key_PrintBindList(m);
	}
}

static void
Key_BindList_f(cmd_state_t *cmd)
{
	Key_PrintBindList(0);
}

static void
Key_Bind_f(cmd_state_t *cmd)
{
	int         i, c, b;
	char        line[MAX_INPUTLINE];

	c = Cmd_Argc (cmd);

	if (c != 2 && c != 3) {
		Con_Print("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(cmd, 1));
	if (b == -1 || b >= MAX_KEYS) {
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(cmd, 1));
		return;
	}

	if (c == 2) {
		if (keybindings[0][b])
			Con_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(cmd, 1), keybindings[0][b]);
		else
			Con_Printf("\"%s\" is not bound\n", Cmd_Argv(cmd, 1));
		return;
	}
// copy the rest of the command line
	line[0] = 0;							// start out with a null string
	for (i = 2; i < c; i++) {
		dp_strlcat (line, Cmd_Argv(cmd, i), sizeof (line));
		if (i != (c - 1))
			dp_strlcat (line, " ", sizeof (line));
	}

	if(!Key_SetBinding (b, 0, line))
		Con_Printf("Key_SetBinding failed for unknown reason\n");
}

/*
============
Writes lines containing "bind key value"
============
*/
void
Key_WriteBindings (qfile_t *f)
{
	int         i, j;
	char bindbuf[MAX_INPUTLINE];
	char tinystr[TINYSTR_LEN];
	const char *p;

	// Override default binds
	FS_Printf(f, "unbindall\n");

	for (j = 0; j < MAX_BINDMAPS; j++)
	{
		for (i = 0; i < (int)(sizeof(keybindings[0])/sizeof(keybindings[0][0])); i++)
		{
			p = keybindings[j][i];
			if (p)
			{
				Cmd_QuoteString(bindbuf, sizeof(bindbuf), p, "\"\\", false); // don't need to escape $ because cvars are not expanded inside bind
				if (j == 0)
					FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString (i, tinystr, TINYSTR_LEN), bindbuf);
				else
					FS_Printf(f, "in_bind %d %s \"%s\"\n", j, Key_KeynumToString (i, tinystr, TINYSTR_LEN), bindbuf);
			}
		}
	}
}


void
Key_Init (void)
{
	Key_History_Init();
	key_linepos = Key_ClearEditLine(true);

//
// register our functions
//
	Cmd_AddCommand(CF_CLIENT, "in_bind", Key_In_Bind_f, "binds a command to the specified key in the selected bindmap");
	Cmd_AddCommand(CF_CLIENT, "in_unbind", Key_In_Unbind_f, "removes command on the specified key in the selected bindmap");
	Cmd_AddCommand(CF_CLIENT, "in_bindlist", Key_In_BindList_f, "bindlist: displays bound keys for all bindmaps, or the given bindmap");
	Cmd_AddCommand(CF_CLIENT, "in_bindmap", Key_In_Bindmap_f, "selects active foreground and background (used only if a key is not bound in the foreground) bindmaps for typing");
	Cmd_AddCommand(CF_CLIENT, "in_releaseall", Key_ReleaseAll_f, "releases all currently pressed keys (debug command)");

	Cmd_AddCommand(CF_CLIENT, "bind", Key_Bind_f, "binds a command to the specified key in bindmap 0");
	Cmd_AddCommand(CF_CLIENT, "unbind", Key_Unbind_f, "removes a command on the specified key in bindmap 0");
	Cmd_AddCommand(CF_CLIENT, "bindlist", Key_BindList_f, "bindlist: displays bound keys for bindmap 0 bindmaps");
	Cmd_AddCommand(CF_CLIENT, "unbindall", Key_Unbindall_f, "removes all commands from all keys in all bindmaps (leaving only shift-escape and escape)");

	Cmd_AddCommand(CF_CLIENT, "history", Key_History_f, "prints the history of executed commands (history X prints the last X entries, history -c clears the whole history)");

	Cvar_RegisterVariable (&con_closeontoggleconsole);
}

void
Key_Shutdown (void)
{
	Key_History_Shutdown();
}

const char *Key_GetBind (int key, int bindmap)
{
	const char *bind;
	if (key < 0 || key >= MAX_KEYS)
		return NULL;
	if(bindmap >= MAX_BINDMAPS)
		return NULL;
	if(bindmap >= 0)
	{
		bind = keybindings[bindmap][key];
	}
	else
	{
		bind = keybindings[key_bmap][key];
		if (!bind)
			bind = keybindings[key_bmap2][key];
	}
	return bind;
}

void Key_FindKeysForCommand (const char *command, int *keys, int numkeys, int bindmap)
{
	int		count;
	int		j;
	const char	*b;

	for (j = 0;j < numkeys;j++)
		keys[j] = -1;

	if(bindmap >= MAX_BINDMAPS)
		return;

	count = 0;

	for (j = 0; j < MAX_KEYS; ++j)
	{
		b = Key_GetBind(j, bindmap);
		if (!b)
			continue;
		if (!strcmp (b, command) )
		{
			keys[count++] = j;
			if (count == numkeys)
				break;
		}
	}
}

/*
===================
Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
static char tbl_keyascii[MAX_KEYS];
static keydest_t tbl_keydest[MAX_KEYS];

typedef struct eventqueueitem_s
{
	int key;
	int ascii;
	qbool down;
}
eventqueueitem_t;
static int events_blocked = 0;
static eventqueueitem_t eventqueue[32];
static unsigned eventqueue_idx = 0;

static void Key_EventQueue_Add(int key, int ascii, qbool down)
{
	if(eventqueue_idx < sizeof(eventqueue) / sizeof(*eventqueue))
	{
		eventqueue[eventqueue_idx].key = key;
		eventqueue[eventqueue_idx].ascii = ascii;
		eventqueue[eventqueue_idx].down = down;
		++eventqueue_idx;
	}
}

void Key_EventQueue_Block(void)
{
	// block key events until call to Unblock
	events_blocked = true;
}

void Key_EventQueue_Unblock(void)
{
	// unblocks key events again
	unsigned i;
	events_blocked = false;
	for(i = 0; i < eventqueue_idx; ++i)
		Key_Event(eventqueue[i].key, eventqueue[i].ascii, eventqueue[i].down);
	eventqueue_idx = 0;
}

void
Key_Event (int key, int ascii, qbool down)
{
	cmd_state_t *cmd = cmd_local;
	const char *bind;
	qbool q;
	keydest_t keydest = key_dest;
	char vabuf[1024];

	if (key < 0 || key >= MAX_KEYS)
		return;

	if(events_blocked)
	{
		Key_EventQueue_Add(key, ascii, down);
		return;
	}

	// get key binding
	bind = keybindings[key_bmap][key];
	if (!bind)
		bind = keybindings[key_bmap2][key];

	if (developer_insane.integer)
		Con_DPrintf("Key_Event(%i, '%c', %s) keydown %i bind \"%s\"\n", key, ascii ? ascii : '?', down ? "down" : "up", keydown[key], bind ? bind : "");

	if(key_consoleactive)
		keydest = key_console;

	if (down)
	{
		// increment key repeat count each time a down is received so that things
		// which want to ignore key repeat can ignore it
		keydown[key] = min(keydown[key] + 1, 2);
		if(keydown[key] == 1) {
			tbl_keyascii[key] = ascii;
			tbl_keydest[key] = keydest;
		} else {
			ascii = tbl_keyascii[key];
			keydest = tbl_keydest[key];
		}
	}
	else
	{
		// clear repeat count now that the key is released
		keydown[key] = 0;
		keydest = tbl_keydest[key];
		ascii = tbl_keyascii[key];
	}

	if(keydest == key_void)
		return;

	// key_consoleactive is a flag not a key_dest because the console is a
	// high priority overlay ontop of the normal screen (designed as a safety
	// feature so that developers and users can rescue themselves from a bad
	// situation).
	//
	// this also means that toggling the console on/off does not lose the old
	// key_dest state

	// specially handle escape (togglemenu) and shift-escape (toggleconsole)
	// engine bindings, these are not handled as normal binds so that the user
	// can recover from a completely empty bindmap
	if (key == K_ESCAPE)
	{
		// ignore key repeats on escape
		if (keydown[key] > 1)
			return;

		// escape does these things:
		// key_consoleactive - close console
		// key_message - abort messagemode
		// key_menu - go to parent menu (or key_game)
		// key_game - open menu

		// in all modes shift-escape toggles console
		if (keydown[K_SHIFT])
		{
			if(down)
			{
				Con_ToggleConsole_f(cmd_local);
				tbl_keydest[key] = key_void; // esc release should go nowhere (especially not to key_menu or key_game)
			}
			return;
		}

		switch (keydest)
		{
			case key_console:
				if(down)
				{
					if(key_consoleactive & KEY_CONSOLEACTIVE_FORCED)
					{
						key_consoleactive &= ~KEY_CONSOLEACTIVE_USER;
#ifdef CONFIG_MENU
						MR_ToggleMenu(1);
#endif
					}
					else
						Con_ToggleConsole_f(cmd_local);
				}
				break;

			case key_message:
				if (down)
					Key_Message (cmd, key, ascii); // that'll close the message input
				break;

			case key_menu:
			case key_menu_grabbed:
#ifdef CONFIG_MENU
				MR_KeyEvent (key, ascii, down);
#endif
				break;

			case key_game:
				// csqc has priority over toggle menu if it wants to (e.g. handling escape for UI stuff in-game.. :sick:)
				q = CL_VM_InputEvent(down ? 0 : 1, key, ascii);
#ifdef CONFIG_MENU
				if (!q && down)
					MR_ToggleMenu(1);
#endif
				break;

			default:
				Con_Printf ("Key_Event: Bad key_dest\n");
		}
		return;
	}

	// send function keydowns to interpreter no matter what mode is (unless the menu has specifically grabbed the keyboard, for rebinding keys)
	// VorteX: Omnicide does bind F* keys
	if (keydest != key_menu_grabbed)
	if (key >= K_F1 && key <= K_F12 && gamemode != GAME_BLOODOMNICIDE)
	{
		if (bind)
		{
			if(keydown[key] == 1 && down)
			{
				// button commands add keynum as a parm
				// prepend to avoid delays from `wait` commands added by other sources
				if (bind[0] == '+')
					Cbuf_InsertText(cmd, va(vabuf, sizeof(vabuf), "%s %i\n", bind, key));
				else
					Cbuf_InsertText(cmd, bind);
			}
			else if(bind[0] == '+' && !down && keydown[key] == 0)
				// append -bind to ensure it's after the +bind in case they arrive in the same frame
				Cbuf_AddText(cmd, va(vabuf, sizeof(vabuf), "-%s %i\n", bind + 1, key));
		}
		return;
	}

	// send input to console if it wants it
	if (keydest == key_console)
	{
		if (!down)
			return;
		// con_closeontoggleconsole enables toggleconsole keys to close the
		// console, as long as they are not the color prefix character
		// (special exemption for german keyboard layouts)
		if (con_closeontoggleconsole.integer && bind && !strncmp(bind, "toggleconsole", strlen("toggleconsole")) && (key_consoleactive & KEY_CONSOLEACTIVE_USER) && (con_closeontoggleconsole.integer >= ((ascii != STRING_COLOR_TAG) ? 2 : 3) || key_linepos == 1))
		{
			Con_ToggleConsole_f(cmd_local);
			return;
		}

		if (Sys_CheckParm ("-noconsole"))
			return; // only allow the key bind to turn off console

		Key_Console (cmd, key, ascii);
		return;
	}

	// handle toggleconsole in menu too
	if (keydest == key_menu)
	{
		if (down && con_closeontoggleconsole.integer && bind && !strncmp(bind, "toggleconsole", strlen("toggleconsole")) && ascii != STRING_COLOR_TAG)
		{
			Cbuf_InsertText(cmd, "toggleconsole\n");  // Deferred to next frame so we're not sending the text event to the console.
			tbl_keydest[key] = key_void; // key release should go nowhere (especially not to key_menu or key_game)
			return;
		}
	}

	// ignore binds while a video is played, let the video system handle the key event
	if (cl_videoplaying)
	{
		if (gamemode == GAME_BLOODOMNICIDE) // menu controls key events
#ifdef CONFIG_MENU
			MR_KeyEvent(key, ascii, down);
#else
			{
			}
#endif
		else
			CL_Video_KeyEvent (key, ascii, keydown[key] != 0);
		return;
	}

	// anything else is a key press into the game, chat line, or menu
	switch (keydest)
	{
		case key_message:
			if (down)
				Key_Message (cmd, key, ascii);
			break;
		case key_menu:
		case key_menu_grabbed:
#ifdef CONFIG_MENU
			MR_KeyEvent (key, ascii, down);
#endif
			break;
		case key_game:
			q = CL_VM_InputEvent(down ? 0 : 1, key, ascii);
			// ignore key repeats on binds and only send the bind if the event hasnt been already processed by csqc
			if (!q && bind)
			{
				if(keydown[key] == 1 && down)
				{
					// button commands add keynum as a parm
					// prepend to avoid delays from `wait` commands added by other sources
					if (bind[0] == '+')
						Cbuf_InsertText(cmd, va(vabuf, sizeof(vabuf), "%s %i\n", bind, key));
					else
						Cbuf_InsertText(cmd, bind);
				}
				else if(bind[0] == '+' && !down && keydown[key] == 0)
					// append -bind to ensure it's after the +bind in case they arrive in the same frame
					Cbuf_AddText(cmd, va(vabuf, sizeof(vabuf), "-%s %i\n", bind + 1, key));
			}
			break;
		default:
			Con_Printf ("Key_Event: Bad key_dest\n");
	}
}

// a helper to simulate release of ALL keys
void
Key_ReleaseAll (void)
{
	int key;
	// clear the event queue first
	eventqueue_idx = 0;
	// then send all down events (possibly into the event queue)
	for(key = 0; key < MAX_KEYS; ++key)
		if(keydown[key])
			Key_Event(key, 0, false);
	// now all keys are guaranteed down (once the event queue is unblocked)
	// and only future events count
}

void Key_ReleaseAll_f(cmd_state_t *cmd)
{
	Key_ReleaseAll();
}
