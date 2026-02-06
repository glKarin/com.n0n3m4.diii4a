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
// console.c

#include "quakedef.h"
#include "shader.h"

console_t	*con_head;			// first console in the list
console_t	*con_curwindow;		// the (window) console that's currently got focus.
console_t	*con_current;		// points to whatever is the active console (the one that has focus ONLY when kdm_console)
console_t	*con_mouseover;		// points to whichever console's title is currently mouseovered, or null

console_t	*con_main;			// the default console that text will be thrown at. recreated as needed.
console_t	*con_chat;			// points to a chat console

#define Font_ScreenWidth() (vid.pixelwidth)

static int Con_DrawProgress(int left, int right, int y);
static int Con_DrawConsoleLines(console_t *con, conline_t *l, float displayscroll, int sx, int ex, int y, int top, int selactive, int selsx, int selex, int selsy, int seley, float lineagelimit);

#ifdef QTERM
#include <windows.h>
typedef struct qterm_s {
	console_t *console;
	qboolean running;
	HANDLE process;
	HANDLE pipein;
	HANDLE pipeout;

	HANDLE pipeinih;
	HANDLE pipeoutih;

	struct qterm_s *next;
} qterm_t;

qterm_t *qterms;
qterm_t *activeqterm;
#endif

//int 		con_linewidth;	// characters across screen
//int			con_totallines;		// total lines in console scrollback

static float		con_cursorspeed = 4;


static cvar_t		con_numnotifylines = CVAR("con_notifylines","4");		//max lines to show
static cvar_t		con_notifytime = CVAR("con_notifytime","3");		//seconds
static cvar_t		con_notify_x = CVAR("con_notify_x","0");
static cvar_t		con_notify_y = CVAR("con_notify_y","0");
static cvar_t		con_notify_w = CVAR("con_notify_w","1");
static cvar_t		con_centernotify = CVAR("con_centernotify", "0");
static cvar_t		con_displaypossibilities = CVAR("con_displaypossibilities", "1");
static cvar_t		con_showcompletion = CVAR("con_showcompletion", "1");
static cvar_t		con_maxlines = CVAR("con_maxlines", "1024");
cvar_t				cl_chatmode = CVARD("cl_chatmode", "2", "0(nq) - everything is assumed to be a console command. prefix with 'say', or just use a messagemode bind\n1(q3) - everything is assumed to be chat, unless its prefixed with a /\n2(qw) - anything explicitly recognised as a command will be used as a command, anything unrecognised will be a chat message.\n/ prefix is supported in all cases.\nctrl held when pressing enter always makes any implicit chat into team chat instead.");
static cvar_t		con_numnotifylines_chat = CVAR("con_numnotifylines_chat", "8");
static cvar_t		con_notifytime_chat = CVAR("con_notifytime_chat", "8");
cvar_t				con_separatechat = CVAR("con_separatechat", "0");
static cvar_t		con_timestamps = CVAR("con_timestamps", "0");
static cvar_t		con_timeformat = CVAR("con_timeformat", "(%H:%M:%S) ");
cvar_t				con_textsize = CVARD("con_textsize", "8", "Resize the console text to be a different height, scaled separately from the hud. The value is the height in (virtual) pixels.");
static cvar_t		con_savehistory = CVARD("con_savehistory", "1", "Write/update conhistory.txt");
extern cvar_t log_developer;

void con_window_cb(cvar_t *var, char *oldval)
{
	if (!con_main)
		return;	//doesn't matter right now.

	if (var->ival)
	{
		con_main->flags &= ~CONF_NOTIFY;
		if (!(con_main->flags & CONF_ISWINDOW))
		{
			con_main->flags |= CONF_ISWINDOW;
			if (con_current == con_main)
				Con_SetActive(con_main);
		}
	}
	else
	{
		con_main->flags |= CONF_NOTIFY;
		if (con_main->flags & CONF_ISWINDOW)
		{
			con_main->flags &= ~CONF_ISWINDOW;
			if (con_curwindow == con_main)
				Con_SetActive(con_main);
		}
	}
}
static cvar_t con_window = CVARCD("con_window", "0", con_window_cb, "States whether the console should be a floating window as in source engine games, or a top-of-the-screen-only thing.");

#define	NUM_CON_TIMES 24

qboolean	con_initialized;

/*makes sure the console object works*/
void Con_Finit (console_t *con)
{
	if (con->current == NULL)
	{
		con->oldest = con->current = Z_Malloc(sizeof(conline_t));
		con->linecount = 0;
	}
	if (con->display == NULL)
		con->display = con->current;

	con->selstartline = NULL;
	con->selendline = NULL;

	con->defaultcharbits = CON_WHITEMASK;
	con->parseflags = 0;
}

/*returns a bitmask:
1: currently active
2: has text that has not been seen yet
*/
int Con_IsActive (console_t *con)
{
	return (con == con_current) | (con->unseentext*2);
}
/*kills a console_t object. will never destroy the main console (which will only be cleared)*/
void Con_Destroy (console_t *con)
{
	shader_t *shader;
	console_t **link;
	conline_t *t;

	if (con->close)
	{
		con->close(con, true);
		con->close = NULL;
	}

	/*purge the lines from the console*/
	while (con->current)
	{
		t = con->current;
		con->current = t->older;
		Z_Free(t);
	}
	con->display = con->current = con->oldest = NULL;

	Con_Footerf(con, false, "");
	if (con->completionline)
		Z_Free(con->completionline);
	con->completionline = NULL;

	for (link = &con_head; *link; link = &(*link)->next)
	{
		if (*link == con)
		{
			(*link) = con->next;
			break;
		}
	}

	shader = con->backshader;

	BZ_Free(con);

	//make sure any special references are fixed up now that its gone
	if (con_mouseover == con)
		con_mouseover = NULL;
	if (con_current == con)
		con_current = con_head;

	if (con_curwindow == con)
	{
		for (con_curwindow = con_head; con_curwindow; con_curwindow = con_curwindow->next)
		{
			if (con_curwindow->flags & CONF_ISWINDOW)
				break;
		}
		if (!con_curwindow)
			Key_Dest_Remove(kdm_cwindows);
	}
	con_mouseover = NULL;

	if (shader)
		R_UnloadShader(shader);
}

/*just purges the background images for various consoles on restart/shutdown*/
void Con_FlushBackgrounds(void)
{
	console_t *con;
	//fixme: we really need to handle videomaps differently here, for vid_restarts.
	for (con = con_head; con; con = con->next)
	{
		if (con->backshader)
			R_UnloadShader(con->backshader);
		con->backshader = NULL;
	}
}

/*obtains a console_t without creating*/
console_t *Con_FindConsole(const char *name)
{
	console_t *con;
	if (!strcmp(name, "current") && con_current)
		return con_current;
	if (!strcmp(name, "head") && con_current)
		return con_head;
	for (con = con_head; con; con = con->next)
	{
		if (!strcmp(con->name, name))
			return con;
	}
	return NULL;
}
/*creates a potentially duplicate console_t - please use Con_FindConsole first, as its confusing otherwise*/
console_t *Con_Create(const char *name, unsigned int flags)
{
	console_t *con, *p;
	if (!name)
	{
		static unsigned long seq;
		name = va("c%lu", seq++);
	}
	if (!strcmp(name, "current"))
		return NULL;
	if (!strcmp(name, "head"))
		return NULL;
	con = Z_Malloc(sizeof(console_t));
	Q_strncpyz(con->name, name, sizeof(con->name));
	Q_strncpyz(con->title, name, sizeof(con->title));
	Q_strncpyz(con->prompt, "]", sizeof(con->prompt));

	con->flags = flags;
	Con_Finit(con);

	//insert at end. make it active if you must.
	if (!con_head)
		con_head = con;
	else
	{
		for (p = con_head; p->next; p = p->next)
			;
		p->next = con;
	}

	return con;
}

static qboolean Con_Main_BlockClose(console_t *con, qboolean force)
{
	if (!force)
	{	//trying to close it just hides it (this is to avoid it getting cleared).
		if (con_curwindow == con)
			Key_Dest_Remove(kdm_cwindows);
		return false;
	}
	con_main = NULL;	//its forced to die. and don't forget it.
	return true;
}
console_t *Con_GetMain(void)
{
	if (!con_main)
	{
		con_main = Con_Create("", 0);

		con_main->linebuffered = Con_ExecuteLine;
		con_main->commandcompletion = true;
		con_main->wnd_w = 640;
		con_main->wnd_h = 480;
		con_main->wnd_x = 0;
		con_main->wnd_y = 0;
		con_main->close = Con_Main_BlockClose;
		Q_strncpyz(con_main->title, "MAIN", sizeof(con_main->title));
		Q_strncpyz(con_main->prompt, "]", sizeof(con_main->prompt));

		Cvar_ForceCallback(&con_window);
	}
	return con_main;
}
/*sets a console as the active one*/
void Con_SetActive (console_t *con)
{
	if (con->flags & CONF_ISWINDOW)
	{
		console_t *prev;
		Key_Dest_Add(kdm_cwindows);
		Key_Dest_Remove(kdm_console);

		if (con_curwindow == con)
			return;

		for (prev = con_head; prev; prev = prev->next)
		{
			if (prev->next == con)
			{
				prev->next = con->next;
				while(prev->next)
				{
					prev = prev->next;
				}
				prev->next = con;
				con->next = NULL;
				break;
			}
		}
		con_curwindow = con;
	}
	else
	{
		if (con_curwindow == con)
			con_curwindow = NULL;
		Key_Dest_Add(kdm_console);
		Key_Dest_Remove(kdm_cwindows);
		con_current = con;
	}

	Con_Footerf(con, false, "");
	con->buttonsdown = CB_NONE;
}
/*for enumerating consoles*/
qboolean Con_NameForNum(int num, char *buffer, int buffersize)
{
	console_t *con;
	for (con = con_head; con; con = con->next, num--)
	{
		if (num <= 0)
		{
			Q_strncpyz(buffer, con->name, buffersize);
			return true;
		}
	}
	if (buffersize>0)
		*buffer = '\0';
	return false;
}

#ifdef QTERM
void QT_Kill(qterm_t *qt, qboolean killconsole)
{
	qterm_t **link;
	qt->console->close = NULL;
	qt->console->userdata = NULL;
	qt->console->redirect = NULL;
	if (killconsole)
		Con_Destroy(qt->console);

	//yes this loop will crash if you're not careful. it makes it easier to debug.
	for (link = &qterms; ; link = &(*link)->next)
	{
		if (*link == qt)
		{
			*link = qt->next;
			break;
		}
	}

	CloseHandle(qt->pipein);
	CloseHandle(qt->pipeout);

	CloseHandle(qt->pipeinih);
	CloseHandle(qt->pipeoutih);
	CloseHandle(qt->process);

	Z_Free(qt);
}
void QT_Update(void)
{
	char buffer[2048];
	DWORD ret;
	qterm_t *qt, *n;
	for (qt = qterms; qt; )
	{
		if (qt->running)
		{
			if (WaitForSingleObject(qt->process, 0) == WAIT_TIMEOUT)
			{
				if ((ret=GetFileSize(qt->pipeout, NULL)))
				{
					if (ret!=INVALID_FILE_SIZE)
					{
						ReadFile(qt->pipeout, buffer, sizeof(buffer)-32, &ret, NULL);
						buffer[ret] = '\0';
						Con_PrintCon(qt->console, buffer, PFS_NOMARKUP);
					}
				}
			}
			else
			{
				Con_PrintCon(qt->console, "Process ended\n", PFS_NOMARKUP);
				qt->running = false;
			}
		}

		n = qt->next;
		if (!qt->running)
		{
			if (!Con_IsActive(qt->console))
				QT_Kill(qt, true);
		}
		qt = n;
	}
}

qboolean QT_KeyPress(console_t *con, unsigned int unicode, int key)
{
	qbyte k[2];
	qterm_t *qt = con->userdata;
	DWORD send = key;	//get around a gcc warning


	k[0] = key;
	k[1] = '\0';

	if (qt->running)
	{
		if (*k == '\r')
		{
//					*k = '\r';
//					WriteFile(qt->pipein, k, 1, &key, NULL);
//					Con_PrintCon(k, &qt->console, PFS_NOMARKUP);
			*k = '\n';
		}
//		if (GetFileSize(qt->pipein, NULL)<512)
		{
			WriteFile(qt->pipein, k, 1, &send, NULL);
			Con_PrintCon(qt->console, k, PFS_NOMARKUP);
		}
	}
	return true;
}

qboolean	QT_Close(struct console_s *con, qboolean force)
{
	qterm_t *qt = con->userdata;
	QT_Kill(qt, false);

	return true;
}

void QT_Create(char *command)
{
	HANDLE StdIn[2];
	HANDLE StdOut[2];
	qterm_t *qt;
	SECURITY_ATTRIBUTES sa;
	STARTUPINFO SUInf;
	PROCESS_INFORMATION ProcInfo;

	int ret;

	qt = Z_Malloc(sizeof(*qt));

	memset(&sa,0,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.bInheritHandle=true;

	CreatePipe(&StdOut[0], &StdOut[1], &sa, 1024);
	CreatePipe(&StdIn[1], &StdIn[0], &sa, 1024);

	memset(&SUInf, 0, sizeof(SUInf));
	SUInf.cb = sizeof(SUInf);
	SUInf.dwFlags = STARTF_USESTDHANDLES;
/*
	qt->pipeout		= StdOut[0];
	qt->pipein		= StdIn[0];
*/
	qt->pipeoutih	= StdOut[1];
	qt->pipeinih	= StdIn[1];

	if (!DuplicateHandle(GetCurrentProcess(), StdIn[0],
						GetCurrentProcess(), &qt->pipein, 0,
						FALSE,                  // not inherited
						DUPLICATE_SAME_ACCESS))
		qt->pipein = StdIn[0];
	else
		CloseHandle(StdIn[0]);
	if (!DuplicateHandle(GetCurrentProcess(), StdOut[0],
						GetCurrentProcess(), &qt->pipeout, 0,
						FALSE,                  // not inherited
						DUPLICATE_SAME_ACCESS))
		qt->pipeout = StdOut[0];
	else
		CloseHandle(StdOut[0]);

	SUInf.hStdInput		= qt->pipeinih;
	SUInf.hStdOutput	= qt->pipeoutih;
	SUInf.hStdError		= qt->pipeoutih;	//we don't want to have to bother working out which one was written to first.

	if (!SetStdHandle(STD_OUTPUT_HANDLE, SUInf.hStdOutput))
		Con_Printf("Windows sucks\n");
	if (!SetStdHandle(STD_ERROR_HANDLE, SUInf.hStdError))
		Con_Printf("Windows sucks\n");
	if (!SetStdHandle(STD_INPUT_HANDLE, SUInf.hStdInput))
		Con_Printf("Windows sucks\n");

	printf("Started app\n");
	ret = CreateProcess(NULL, command, NULL, NULL, true, CREATE_NO_WINDOW, NULL, NULL, &SUInf, &ProcInfo);

	qt->process = ProcInfo.hProcess;
	CloseHandle(ProcInfo.hThread);

	qt->running = true;

	qt->console = Con_Create("QTerm", 0);
	qt->console->redirect = QT_KeyPress;
	qt->console->close = QT_Close;
	qt->console->userdata = qt;
	Con_PrintCon(qt->console, "Started Process\n", PFS_NOMARKUP);
	Con_SetActive(qt->console);

	qt->next = qterms;
	qterms = activeqterm = qt;
}

void Con_QTerm_f(void)
{
	if(Cmd_IsInsecure())
		Con_Printf("Server tried stuffcmding a restricted command: qterm %s\n", Cmd_Args());
	else
		QT_Create(Cmd_Args());
}
#endif




void Key_ClearTyping (void)
{
	key_lines[edit_line] = BZ_Realloc(key_lines[edit_line], 1);
	key_lines[edit_line][0] = 0;	// clear any typing
	key_linepos = 0;
}

void Con_History_Load(void)
{
	char line[8192];
	char *cr;
	vfsfile_t *file = FS_OpenVFS("conhistory.txt", "rb", FS_ROOT);

	for (edit_line=0 ; edit_line<=CON_EDIT_LINES_MASK ; edit_line++)
	{
		key_lines[edit_line] = BZ_Realloc(key_lines[edit_line], 1);
		key_lines[edit_line][0] = 0;
	}
	edit_line = 0;
	key_linepos = 0;

	if (file)
	{
		while (VFS_GETS(file, line, sizeof(line)-1))
		{
			//strip a trailing \r if its from windows.
			cr = line + strlen(line);
			if (cr > line && cr[-1] == '\r')
				cr[-1] = '\0';
			key_lines[edit_line] = BZ_Realloc(key_lines[edit_line], strlen(line)+1);
			strcpy(key_lines[edit_line], line);
			edit_line = (edit_line+1) & CON_EDIT_LINES_MASK;
		}
		VFS_CLOSE(file);
	}
	history_line = edit_line;
}
void Con_History_Save(void)
{
	vfsfile_t *file;
	int line;

	if (!FS_GameIsInitialised())
		return;

	if (!con_savehistory.ival)
		return;

	file = FS_OpenVFS("conhistory.txt", "wb", FS_ROOT);
	if (file)
	{
		line = edit_line - CON_EDIT_LINES_MASK;
		if (line < 0)
			line = 0;
		for(; line < edit_line; line++)
		{
			VFS_PUTS(file, key_lines[line]);
#ifdef _WIN32	//use an \r\n for readability with notepad.
			VFS_PUTS(file, "\r\n");
#else
			VFS_PUTS(file, "\n");
#endif
		}
		VFS_CLOSE(file);
	}
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_Force(void)
{
	console_t *con = Con_GetMain();

	SCR_EndLoadingPlaque();
	Key_ClearTyping ();

	if (con->flags & CONF_ISWINDOW)
	{
		if (con_curwindow == con && Key_Dest_Has(kdm_cwindows))
		{
			con_curwindow = NULL;
			Key_Dest_Remove(kdm_cwindows);
		}
		else
		{
			con_curwindow = con;
			Key_Dest_Add(kdm_cwindows);
			VRUI_SnapAngle();
		}
	}
	else
	{
		if (Key_Dest_Has(kdm_console))
			Key_Dest_Remove(kdm_console);
		else
		{
			Key_Dest_Add(kdm_console);
			VRUI_SnapAngle();
		}
	}
}
void Con_ToggleConsole_f (void)
{
	extern cvar_t con_stayhidden;

	Con_GetMain();

	if (!con_curwindow)
	{
		for (con_curwindow = con_head; con_curwindow; con_curwindow = con_curwindow->next)
			if (con_curwindow->flags & CONF_ISWINDOW)
				break;
	}

	if (con_curwindow && !Key_Dest_Has(kdm_cwindows|kdm_console))
	{
		Key_Dest_Add(kdm_cwindows);
		return;
	}

#ifdef CSQC_DAT
	if (!editormodal && CSQC_ConsoleCommand(-1, "toggleconsole"))
	{
		Key_Dest_Remove(kdm_console);
		return;
	}
#endif

	if (con_stayhidden.ival >= 3)
	{
		Key_Dest_Remove(kdm_cwindows);
		return;	//its hiding!
	}

	Con_ToggleConsole_Force();
}

void Con_ClearCon(console_t *con)
{
	conline_t *t;
	while (con->current)
	{
		t = con->current;
		con->current = t->older;
		Z_Free(t);
	}
	con->display = con->current = con->oldest = NULL;
	con->selstartline = NULL;
	con->selendline = NULL;

	/*reset the line pointers, create an active line*/
	Con_Finit(con);
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	console_t *con = Con_FindConsole(Cmd_Argv(1));
	if (!con || Cmd_IsInsecure())
		return;
	Con_ClearCon(con);
}


void Cmd_ConEchoCenter_f(void)
{
	console_t *con;
	con = Con_FindConsole(Cmd_Argv(1));
	if (!con)
		con = Con_Create(Cmd_Argv(1), 0);
	if (con)
	{
		Cmd_ShiftArgs(1, false);
		Con_PrintCon(con, Cmd_Args(), con->parseflags|PFS_NONOTIFY|PFS_CENTERED );
		Con_PrintCon(con, "\n", con->parseflags|PFS_NONOTIFY|PFS_CENTERED);
	}
}
void Cmd_ConEcho_f(void)
{
	console_t *con;
	con = Con_FindConsole(Cmd_Argv(1));
	if (!con)
		con = Con_Create(Cmd_Argv(1), 0);
	if (con)
	{
		Cmd_ShiftArgs(1, false);
		Con_PrintCon(con, Cmd_Args(), con->parseflags);
		Con_PrintCon(con, "\n", con->parseflags);
	}
}

void Cmd_ConClear_f(void)
{
	console_t *con;
	con = Con_FindConsole(Cmd_Argv(1));
	if (con)
		Con_ClearCon(con);
}
void Cmd_ConClose_f(void)
{
	console_t *con;
	con = Con_FindConsole(Cmd_Argv(1));
	if (con)
		Con_Destroy(con);
}
void Cmd_ConActivate_f(void)
{
	console_t *con;
	con = Con_FindConsole(Cmd_Argv(1));
	if (con)
		Con_SetActive(con);
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void)
{
	chat_team = false;
	Key_Dest_Add(kdm_message);
	Key_Dest_Remove(kdm_console);
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	chat_team = true;
	Key_Dest_Add(kdm_message);
	Key_Dest_Remove(kdm_console);
}

void Con_ForceActiveNow(void)
{
	Key_Dest_Add(kdm_console);
	scr_con_target = scr_con_current = vid.height;
}

/*
================
Con_Init
================
*/
void Log_Init (void);

void Con_Init (void)
{
	con_current = NULL;
	con_head = NULL;

	con_main = Con_GetMain();

	con_initialized = true;
//	Con_TPrintf ("Console initialized.\n");

//
// register our commands
//
	Cvar_Register (&con_centernotify, "Console controls");
	Cvar_Register (&con_notifytime, "Console controls");
	Cvar_Register (&con_notify_x, "Console controls");
	Cvar_Register (&con_notify_y, "Console controls");
	Cvar_Register (&con_notify_w, "Console controls");
	Cvar_Register (&con_numnotifylines, "Console controls");
	Cvar_Register (&con_displaypossibilities, "Console controls");
	Cvar_Register (&con_showcompletion, "Console controls");
	Cvar_Register (&cl_chatmode, "Console controls");
	Cvar_Register (&con_maxlines, "Console controls");
	Cvar_Register (&con_numnotifylines_chat, "Console controls");
	Cvar_Register (&con_notifytime_chat, "Console controls");
	Cvar_Register (&con_separatechat, "Console controls");
	Cvar_Register (&con_timestamps, "Console controls");
	Cvar_Register (&con_timeformat, "Console controls");
	Cvar_Register (&con_textsize, "Console controls");
	Cvar_Register (&con_window, "Console controls");
	Cvar_Register (&con_savehistory, "Console controls");
	Cvar_ForceCallback(&con_window);

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
#ifdef QTERM
	Cmd_AddCommand ("qterm", Con_QTerm_f);
#endif

	Cmd_AddCommandD ("conecho_center", Cmd_ConEchoCenter_f, "conecho_center consolename The Text To Echo\nUse \"\" for the main console.\nAny added lines will be aligned to the middle of the console.");
	Cmd_AddCommandD ("conecho", Cmd_ConEcho_f, "conecho consolename The Text To Echo\nEchos text to a named console instead of just the main one.");
	Cmd_AddCommandD ("conclear", Cmd_ConClear_f, "Clears a named console (instead of just the main one)");
	Cmd_AddCommandD ("conclose", Cmd_ConClose_f, "Destroys a named console");
	Cmd_AddCommandD ("conactivate", Cmd_ConActivate_f, "Brings focus to the named console. Will not do anything if the named console is not created yet (so be sure to do any echos before using this command)");

	Log_Init();
}

void Con_Shutdown(void)
{
	int i;

	for (i = 0; i <= CON_EDIT_LINES_MASK; i++)
	{
		BZ_Free(key_lines[i]);
	}

	while(con_head)
		Con_Destroy(con_head);
	con_initialized = false;
}

void TTS_SayConString(conchar_t *stringtosay);

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/

//reallocates a line (with its buffer), and updates its links. if shrinking, be sure to reduce the length
conline_t *Con_ResizeLineBuffer(console_t *con, conline_t *old, unsigned int length)
{
	conline_t *l;

	old->maxlength = length & 0xffff;
	if (old->maxlength < old->length)
		return NULL;	//overflow.
	l = BZ_Realloc(old, sizeof(*l)+(old->maxlength)*sizeof(conchar_t));

	if (l->newer)
		l->newer->older = l;
	if (l->older)
		l->older->newer = l;

	if (con->selstartline == old)
		con->selstartline = l;
	if (con->selendline == old)
		con->selendline = l;
	if (con->display == old)
		con->display = l;
	if (con->oldest == old)
		con->oldest = l;
	if (con->current == old)
		con->current = l;
	if (con->footerline == old)
		con->footerline = l;
	if (con->userline == old)
		con->userline = l;
	if (con->highlightline == old)
		con->highlightline = old;
	return l;
}

qboolean Con_InsertConChars (console_t *con, conline_t *line, int offset, conchar_t *c, int len)
{
	conchar_t *o;

	if (line->length+len > line->maxlength)
	{
		line = Con_ResizeLineBuffer(con, line, line->length+len + 8);
		if (!line)
			return false;	//overflowed!
	}

	o = (conchar_t *)(line+1);
	if (line->length-offset)
		memmove(o+offset+len, o+offset, (line->length-offset)*sizeof(conchar_t));
	memcpy(o+offset, c, sizeof(*o) * len);
	line->length+=len;
	return true;
}

void Con_PrintCon (console_t *con, const char *txt, unsigned int parseflags)
{
	conchar_t expanded[4096];
	conchar_t *c, *n;
	conline_t *reuse;
	int maxlines;
	unsigned flags, codepoint;

	if (con->maxlines)
		maxlines = con->maxlines;
	else
		maxlines = con_maxlines.ival;

	COM_ParseFunString(con->defaultcharbits, txt, expanded, sizeof(expanded), parseflags);

	c = expanded;
	if (*c)
		con->unseentext = true;
	for (;*c; c=n)
	{
		n = Font_Decode(c, &flags, &codepoint);
		if (codepoint=='\r' && !(flags&CON_HIDDEN))
			con->cr = true;
		else if (codepoint=='\n' && !(flags&CON_HIDDEN))
		{
			con->cr = false;
			reuse = NULL;
			while (con->linecount >= maxlines)
			{
				if (con->oldest == con->current)
					break;

				if (con->selstartline == con->oldest)
					con->selstartline = NULL;
				if (con->selendline == con->oldest)
					con->selendline = NULL;

				if (con->display == con->oldest)
					con->display = con->oldest->newer;
				con->oldest = con->oldest->newer;
				if (reuse)
					Z_Free(con->oldest->older);
				else
					reuse = con->oldest->older;
				con->oldest->older = NULL;
				con->linecount--;
			}
			con->linecount++;
			con->current->time = realtime;
			con->current->flags = 0;
			if (parseflags & PFS_CENTERED)
				con->current->flags |= CONL_CENTERED;
			if (parseflags & PFS_NONOTIFY)
				con->current->flags |= CONL_NONOTIFY;
			else if (!Key_Dest_Has(~kdm_game) && (con->flags & CONF_NOTIFY))
				VRUI_SnapAngle();

#if defined(HAVE_SPEECHTOTEXT)
			if (con->current)
				TTS_SayConString((conchar_t*)(con->current+1));
#endif

			if (!reuse)
			{
				reuse = Z_Malloc(sizeof(conline_t) + sizeof(conchar_t));
				reuse->maxlength = 1;
			}
			else
			{
				reuse->newer = NULL;
				reuse->older = NULL;
			}
			reuse->id = (++con->nextlineid) & 0xffff;
			reuse->older = con->current;
			con->current->newer = reuse;
			con->current = reuse;
			con->current->length = 0;
			if (con->display == con->current->older && con->displayscroll==0)
				con->display = con->current;
		}
		else
		{
			if (con->cr)
			{
				con->current->length = 0;
				con->cr = false;
			}
			if (!con->current->numlines)
				con->current->numlines = 1;

			if (!con->current->length && con_timestamps.ival && !(parseflags & PFS_CENTERED))
			{
				char timeasc[64];
				conchar_t timecon[64], *timeconend;
				time_t rawtime;
				time (&rawtime);
				strftime(timeasc, sizeof(timeasc), con_timeformat.string, localtime (&rawtime));
				timeconend = COM_ParseFunString(con->defaultcharbits, timeasc, timecon, sizeof(timecon), false);
				Con_InsertConChars(con, con->current, con->current->length, timecon, timeconend-timecon);
			}

			//FIXME: don't do this a char at a time
			Con_InsertConChars(con, con->current, con->current->length, c, n-c);
		}
	}

	con->current->time = realtime;
}

void Con_CenterPrint(const char *txt)
{
	console_t *c = Con_GetMain();
	int flags = c->parseflags|PFS_NONOTIFY|PFS_CENTERED;
	Con_PrintCon(c, "^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f\n", flags);
	Con_PrintCon(c, txt, flags);	//client console
	Con_PrintCon(c, "\n^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f\n", flags);
}

void Con_Print (const char *txt)
{
	console_t *c = Con_GetMain();
	Con_PrintCon(c, txt, c->parseflags);	//client console
}
void Con_PrintFlags(const char *txt, unsigned int setflags, unsigned int clearflags)
{
	console_t *c = Con_GetMain();
	setflags |= c->parseflags;
	setflags &= ~clearflags;

// also echo to debugging console
	Sys_Printf ("%s", txt);	// also echo to debugging console

// log all messages to file
	Con_Log (txt);

	if (con_initialized)
		Con_PrintCon(c, txt, setflags);
}

void Con_CycleConsole(void)
{
	console_t *first = con_current?con_current:con_head;
	while(1)
	{
		con_current = con_current->next;
		if (!con_current)
			con_current = con_head;
		if (con_current == first)
		{
			if (con_current->flags & (CONF_HIDDEN|CONF_ISWINDOW))
				con_current = NULL; //no valid consoles
			break;	//we wrapped? oh noes
		}

		if (con_current->flags & (CONF_HIDDEN|CONF_ISWINDOW))
			continue;	//this is a valid choice
		break;
	}
}

/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/

#ifdef HAVE_SERVER
extern redirect_t	sv_redirected;
extern char	sv_redirected_buf[8000];
void SV_FlushRedirect (void);
#endif
vfsfile_t *con_pipe;

#define	MAXPRINTMSG	4096
static void Con_PrintFromThread (void *ctx, void *data, size_t a, size_t b)
{
	Con_Printf("%s", (char*)data);
	BZ_Free(data);
}

vfsfile_t *Con_POpen(const char *conname)
{
	if (!conname || !*conname)
	{
		if (con_pipe)
			VFS_CLOSE(con_pipe);
		con_pipe = VFSPIPE_Open(2, false);
		return con_pipe;
	}
	return NULL;
}

// FIXME: make a buffer size safe vsprintf?
void VARGS Con_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg), fmt,argptr);
	va_end (argptr);

	if (!Sys_IsMainThread())
	{
		COM_AddWork(WG_MAIN, Con_PrintFromThread, NULL, Z_StrDup(msg), 0, 0);
		return;
	}

#ifdef HAVE_SERVER
	// add to redirected message
	if (sv_redirected)
	{
		if (strlen (msg) + strlen(sv_redirected_buf) > sizeof(sv_redirected_buf) - 1)
			SV_FlushRedirect ();
		strcat (sv_redirected_buf, msg);
		return;
	}
#endif

// also echo to debugging console
	Sys_Printf ("%s", msg);	// also echo to debugging console

// log all messages to file
	Con_Log (msg);

	if (con_pipe)
		VFS_PUTS(con_pipe, msg);

	if (!con_initialized)
		return;

// write it to the scrollable buffer
	Con_Print (msg);
}

void VARGS Con_SafePrintf (const char *fmt, ...)
{	//obsolete version of the function
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

// write it to the scrollable buffer
	Con_Printf ("%s", msg);
}

void VARGS Con_TPrintf (translation_t text, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	const char *fmt = localtext(text);

	va_start (argptr,text);
	vsnprintf (msg,sizeof(msg), fmt,argptr);
	va_end (argptr);

// write it to the scrollable buffer
	Con_Printf ("%s", msg);
}

void VARGS Con_SafeTPrintf (translation_t text, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	const char *fmt = localtext(text);

	va_start (argptr,text);
	vsnprintf (msg,sizeof(msg), fmt,argptr);
	va_end (argptr);

// write it to the scrollable buffer
	Con_Printf ("%s", msg);
}

static void Con_DPrintFromThread (void *ctx, void *data, size_t a, size_t b)
{
	if (log_developer.ival || !a)
		Con_Log(data);
	if (developer.ival >= (int)a)
	{
		console_t *c = Con_GetMain();
		Sys_Printf ("%s", (const char*)data);	// also echo to debugging console
		Con_PrintCon(c, data, c->parseflags);
	}
	BZ_Free(data);
}
/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void VARGS Con_DPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

#ifdef CRAZYDEBUGGING
	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);
	Sys_Printf("%s", msg);
	return;
#else
	if (!developer.ival && !log_developer.ival)
		return; // early exit
#endif

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

	if (!Sys_IsMainThread())
	{
		COM_AddWork(WG_MAIN, Con_DPrintFromThread, NULL, Z_StrDup(msg), 1, 0);
		return;
	}

	if (log_developer.ival)
		Con_Log(msg);
	if (developer.ival)
	{
		console_t *c = Con_GetMain();
		Sys_Printf ("%s", msg);	// also echo to debugging console
		Con_PrintCon(c, msg, c->parseflags);
	}
}
void VARGS Con_DLPrintf (int level, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

#ifdef CRAZYDEBUGGING
	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);
	Sys_Printf("%s", msg);
	return;
#else
	if (developer.ival<level && (!log_developer.ival && level))
		return; // early exit
#endif

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

	if (!Sys_IsMainThread())
	{
		COM_AddWork(WG_MAIN, Con_DPrintFromThread, NULL, Z_StrDup(msg), level, 0);
		return;
	}

	if (log_developer.ival || !level)
		Con_Log(msg);
	if (developer.ival >= level)
	{
		Sys_Printf ("%s", msg);	// also echo to debugging console
		if (con_initialized)
		{
			console_t *c = Con_GetMain();
			Con_PrintCon(c, msg, c->parseflags);
		}
	}
}

void VARGS Con_ThrottlePrintf (float *timer, int developerlevel, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	float now = realtime;

	if (*timer > now)
		;	//in the future? zomg
	else if (*timer >= now-1)
		return;	//within the last second
	*timer = now;	//in the future? zomg

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

	if (developerlevel)
		Con_DLPrintf(developerlevel, "%s", msg);
	else
		Con_Printf("%s", msg);
}

static void Con_FooterMarked(console_t *con, qboolean append, conchar_t *marked, conchar_t *markedend)
{
	int oldlen, newlen;
	conline_t *newf = NULL, *l;
	unsigned fl, cp;
	conchar_t *nl, *n;

	if (!append)
	{
		while(con->footerline)
		{
			l = con->footerline;
			con->footerline = l->older;
			if (con->selstartline == l)
				con->selstartline = NULL;
			if (con->selendline == l)
				con->selendline = NULL;
			Z_Free(l);
		}
		con->footerline = NULL;
	}
	for (append = true; marked < markedend; marked = n, append = false)
	{
		n = markedend;
		for (nl = marked; nl < markedend; nl=n)
		{
			n = Font_Decode(nl, &fl, &cp);
			if (cp == '\n' && !(fl&CONF_HIDDEN))
				break;
		}

		newlen = nl - marked;
		if (append && con->footerline)
			oldlen = con->footerline->length;
		else
			oldlen = 0;

		if (newlen || !append)
		{
			newf = Z_Malloc(sizeof(*newf) + (oldlen + newlen) * sizeof(conchar_t));
			if (append && con->footerline)
			{
				memcpy(newf, con->footerline, sizeof(*con->footerline)+oldlen*sizeof(conchar_t));
				Z_Free(con->footerline);
			}
			else
				newf->older = con->footerline;
			if (newf->older)
				newf->older->newer = newf;

			memcpy((conchar_t*)(newf+1)+oldlen, marked, newlen*sizeof(conchar_t));
			newf->length = oldlen + newlen;
			con->footerline = newf;
		}
	}
}

/*description text at the bottom of the console*/
void Con_Footerf(console_t *con, qboolean append, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	conchar_t	marked[MAXPRINTMSG], *markedend;

	if (!con)
		con = con_current;
	if (!con)
		return;

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);
	markedend = COM_ParseFunString((COLOR_YELLOW << CON_FGSHIFT)|(con->backshader?CON_NONCLEARBG:0), msg, marked, sizeof(marked), false);

	Con_FooterMarked(con, append, marked, markedend);
}

/*
==============================================================================

DRAWING

==============================================================================
*/

qboolean COM_InsertIME(conchar_t *buffer, size_t buffersize, conchar_t **cursor, conchar_t **textend)
{
	conchar_t *in = vid.ime_preview;
	if (in && *in && *textend+vid.ime_previewlen < buffer+buffersize)
	{
		memmove(buffer + (*cursor-buffer) + vid.ime_previewlen, *cursor, ((*textend-*cursor)+1)*sizeof(conchar_t));
		memcpy(buffer + (*cursor-buffer), in, vid.ime_previewlen*sizeof(conchar_t));
		*cursor += vid.ime_caret;
		*textend += vid.ime_previewlen;

		return true;
	}
	return false;
}

/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
y is the bottom of the input
return value is the top of the region
================
*/
int Con_DrawInput (console_t *con, qboolean focused, int left, int right, int y, qboolean selactive, int selsx, int selex, int selsy, int seley)
{
	int		i;
	int lhs, rhs;
	int p;
	unsigned char	*text, *fname = NULL;
	extern int con_commandmatch;
	conchar_t maskedtext[2048];
	conchar_t *endmtext;
	conchar_t *cursor;
	conchar_t *cchar;
	conchar_t *textstart;
	size_t textsize;
	qboolean cursorframe;
	unsigned int codeflags, codepoint;
	int cursorpos;
	qboolean hidecomplete;

	int x;

	if (focused)
	{
		vid.ime_allow = true;
		vid.ime_position[0] = ((float)left/vid.pixelwidth)*vid.width;
		vid.ime_position[1] = ((float)y/vid.pixelheight)*vid.height;
	}

	if (!con->linebuffered || con->linebuffered == Con_Navigate)
	{
		if (con->footerline)
		{
			y = Con_DrawConsoleLines(con, con->footerline, 0, left, right, y, 0, selactive, selsx, selex, selsy, seley, 0);
		}
		return y;	//fixme: draw any unfinished lines of the current console instead.
	}

	y -= Font_CharHeight();

	if (!focused)
		return y;		// don't draw anything (always draw if not active)

	text = key_lines[edit_line];

	cursorpos = key_linepos;

	//copy it to an alternate buffer and fill in text colouration escape codes.
	//if it's recognised as a command, colour it yellow.
	//if it's not a command, and the cursor is at the end of the line, leave it as is,
	//	but add to the end to show what the compleation will be.

	textstart = COM_ParseFunString(CON_WHITEMASK, con->prompt, maskedtext, sizeof(maskedtext) - sizeof(maskedtext[0]), PFS_FORCEUTF8);
	textsize = (countof(maskedtext) - (textstart-maskedtext) - 1) * sizeof(maskedtext[0]);
	i = text[cursorpos];
	text[cursorpos] = 0;
	cursor = COM_ParseFunString(CON_WHITEMASK, text, textstart, textsize, PFS_KEEPMARKUP | PFS_FORCEUTF8);
	//okay, so that's where the cursor is. heal the input string and reparse (so we don't mess up escapes)
	text[cursorpos] = i;
	endmtext = COM_ParseFunString(CON_WHITEMASK, text, textstart, textsize, PFS_KEEPMARKUP | PFS_FORCEUTF8);
//	endmtext = COM_ParseFunString(CON_WHITEMASK, text+key_linepos, cursor, ((char*)maskedtext)+sizeof(maskedtext) - (char*)(cursor+1), PFS_KEEPMARKUP | PFS_FORCEUTF8);

	hidecomplete = COM_InsertIME(maskedtext, countof(maskedtext), &cursor, &endmtext);
/*	if (cursorpos == strlen(text) && vid.ime_preview)
	{
		endmtext = COM_ParseFunString(COLOR_MAGENTA<<CON_FGSHIFT, vid.ime_preview, endmtext, (countof(maskedtext) - (endmtext-maskedtext) - 1) * sizeof(maskedtext[0]), PFS_KEEPMARKUP | PFS_FORCEUTF8);
		cursor += strlen(vid.ime_preview);
		cursorpos += strlen(vid.ime_preview);
		text = va("%s%s", text, vid.ime_preview);
	}
*/

	if ((char*)endmtext == (char*)(maskedtext-2) + sizeof(maskedtext))
		endmtext[-1] = CON_WHITEMASK | '+' | CON_NONCLEARBG;
	endmtext[1] = 0;

	if ((*cursor & CON_HIDDEN) && cursor[0] != (CON_HIDDEN|'^') && cursor[1] != CON_LINKSTART)
	{	//if we're in the middle of a link (but not at the very first char - to make life prettier) then reveal the hidden text so that you can actually see what you're editing.
		for (i = cursor-textstart; textstart[i]; i++)
		{
			if (textstart[i] == CON_LINKSTART)
				break;
			if (textstart[i] == CON_LINKEND)
			{
				textstart[i] &= ~CON_HIDDEN;
				break;
			}
			textstart[i] &= ~CON_HIDDEN;
		}
		for (i = cursor-textstart; i>=0; i--)
		{
			if (textstart[i] == CON_LINKEND)
				break;
			if (textstart[i] == CON_LINKSTART)
			{
				textstart[i] &= ~CON_HIDDEN;
				if (--i >= 0)	//make the ^ of the ^[ shown too.
					textstart[i] &= ~CON_HIDDEN;
				break;
			}
			textstart[i] &= ~CON_HIDDEN;
		}
	}

	i = 0;
	x = left;

	if (!hidecomplete && con->commandcompletion && con_showcompletion.ival && text[0] && !(text[0] == '/' && !text[1]))
	{
		if (cl_chatmode.ival && (text[0] == '/' || (cl_chatmode.ival == 2 && Cmd_IsCommand(text))))
		{	//color the first token yellow, it's a valid command
			for (p = 0; (textstart[p]&CON_CHARMASK)>' '; p++)
				textstart[p] = (textstart[p]&CON_CHARMASK) | (COLOR_YELLOW<<CON_FGSHIFT);
		}

		if (cursor == endmtext)	//cursor is at end
		{
			int cmdstart;
			cmdstart = text[0] == '/'?1:0;
			fname = Cmd_CompleteCommand(text+cmdstart, true, true, max(1, con_commandmatch), NULL);
			if (fname && strlen(fname) < 256)	//we can compleate it to:
			{
				for (p = min(strlen(fname), cursorpos-cmdstart); fname[p]>0; p++)
					textstart[p+cmdstart] = (unsigned int)fname[p] | (COLOR_GREEN<<CON_FGSHIFT);
				if (p < cursorpos-cmdstart)
					p = cursorpos-cmdstart;
				p = min(p+cmdstart, sizeof(maskedtext)/sizeof(maskedtext[0]) - 3);
				textstart[p] = 0;
				textstart[p+1] = 0;
			}
		}
	}

	if (!vid.activeapp)
		cursorframe = 0;
	else
		cursorframe = ((int)(realtime*con_cursorspeed)&1);

	//FIXME: support tab somehow
	for (lhs = 0, cchar = maskedtext; cchar < cursor; )
	{
		cchar = Font_Decode(cchar, &codeflags, &codepoint);
		lhs += Font_CharWidth(codeflags, codepoint);
	}
	for (rhs = 0, cchar = cursor; *cchar; )
	{
		cchar = Font_Decode(cchar, &codeflags, &codepoint);
		rhs += Font_CharWidth(codeflags, codepoint);
	}

	//put the cursor in the middle
	x = (right-left)/2 + left;
	//move the line to the right if there's not enough text to touch the right hand side
	if (x < right-rhs - Font_CharWidth(CON_WHITEMASK, 0xe000|11))
		x = right - rhs - Font_CharWidth(CON_WHITEMASK, 0xe000|11);
	//if the left hand side is on the right of the left point (overrides right alignment)
	if (x > lhs + left)
		x = lhs + left;

	lhs = x - lhs;
	for (cchar = maskedtext; cchar < cursor; )
	{
		cchar = Font_Decode(cchar, &codeflags, &codepoint);
		lhs = Font_DrawChar(lhs, y, codeflags, codepoint);
	}
	rhs = x;
	cchar = Font_Decode(cursor, &codeflags, &codepoint);
	if (cursorframe)
	{
//		extern cvar_t com_parseutf8;
//		if (com_parseutf8.ival)
//			Font_DrawChar(rhs, y, (*cursor&~(CON_BGMASK|CON_FGMASK)) | (COLOR_BLUE<<CON_BGSHIFT) | CON_NONCLEARBG | CON_WHITEMASK);
//		else
			Font_DrawChar(rhs, y, CON_WHITEMASK, 0xe000|11);
	}
	else if (codepoint)
	{
		Font_DrawChar(rhs, y, codeflags, codepoint);
	}
	if (codepoint)
	{
		rhs += Font_CharWidth(codeflags, codepoint);
		while (*cchar)
		{
			cchar = Font_Decode(cchar, &codeflags, &codepoint);
			rhs = Font_DrawChar(rhs, y, codeflags, codepoint);
		}
	}

	/*if its getting completed to something, show some help about the command that is going to be used*/
	if (con->footerline)
	{
		y = Con_DrawConsoleLines(con, con->footerline, 0, left, right, y, 0, selactive, selsx, selex, selsy, seley, 0);
	}

	/*just above that, we have the tab completion list*/
	if (con_commandmatch && con_displaypossibilities.value)
	{
		conchar_t *end, *s;
		const char *cmd;//, *desc;
		int cmdstart;
		size_t newlen;
		cmd_completion_t *c;
		cmdstart = text[0] == '/'?1:0;
		end = maskedtext;

		if (!con->completionline || con->completionline->length + 512 > con->completionline->maxlength)
		{
			newlen = (con->completionline?con->completionline->length:0) + 2048;

			Z_Free(con->completionline);
			con->completionline = Z_Malloc(sizeof(*con->completionline) + newlen*sizeof(conchar_t));
			con->completionline->maxlength = newlen;
		}
		con->completionline->length = 0;

		c = Cmd_Complete(text+cmdstart, true);

		for (i = 0; i < c->num; i++)
		{
			int col = (con_commandmatch == i+1)?3:2;
			s = (conchar_t*)(con->completionline+1);

			//note: if cl_chatmode is 0, then we shouldn't show the leading /, however that is how the console link stuff recognises it as command text, so we always display it.
			cmd = c->completions[i].text;
//			desc = c->completions[i].desc;
//			if (desc)
//				end = COM_ParseFunString((COLOR_GREEN<<CON_FGSHIFT), va("^[^%i/%s\\tip\\%s^]\t", col, cmd, desc), s+con->completionline->length, (con->completionline->maxlength-con->completionline->length)*sizeof(maskedtext[0]), true);
//			else
				end = COM_ParseFunString((COLOR_GREEN<<CON_FGSHIFT), va("^[^%i/%s^]\t", col, cmd), s+con->completionline->length, (con->completionline->maxlength-con->completionline->length)*sizeof(maskedtext[0]), true);
			con->completionline->length = end - s;
		}
		if (c->extra)
		{
			s = (conchar_t*)(con->completionline+1);
			end = COM_ParseFunString((COLOR_WHITE<<CON_FGSHIFT), va("%u MORE", (unsigned)c->extra), s+con->completionline->length, (con->completionline->maxlength-con->completionline->length)*sizeof(maskedtext[0]), true);
			con->completionline->length = end - s;
		}

		if (con->completionline->length)
			y = Con_DrawConsoleLines(con, con->completionline, 0, left, right, y, 0, selactive, selsx, selex, selsy, seley, 0);
	}

	return y;
}

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotifyOne (console_t *con)
{
	conchar_t *starts[NUM_CON_TIMES], *ends[NUM_CON_TIMES];
	float alphas[NUM_CON_TIMES], a;
	conchar_t *c;
	conline_t *l;
	int lines=con->notif_l;
	int line;
	int nx, y;
	int nw;
	int x;
	unsigned int codeflags, codepoint;

	int maxlines;
	float t;

	Font_BeginString(font_console, con->notif_x * vid.width, con->notif_y * vid.height, &nx, &y);
	Font_Transform(con->notif_w * vid.width, 0, &nw, NULL);

	if (con->notif_l < 0)
		con->notif_l = 0;
	if (con->notif_l > NUM_CON_TIMES)
		con->notif_l = NUM_CON_TIMES;
	lines = maxlines = con->notif_l;

	if (!con->notif_x && !con->notif_y && con->notif_w == 1)
		y = Con_DrawProgress(0, nw, 0);

	l = con->current;
	if (!l->length)
		l = l->older;
	for (; l && lines > con->notif_l-maxlines; l = l->older)
	{
		if (l->flags & CONL_NONOTIFY)
			continue; //hidden from notify
		t = realtime - (l->time+con->notif_t);
		if (t > 0)
		{
			if (t > con->notif_fade)
			{
				l->flags |= CONL_NONOTIFY;
				break;
			}
			a = 1 - (t/con->notif_fade);
		}
		else a = 1;

		line = Font_LineBreaks((conchar_t*)(l+1), (conchar_t*)(l+1)+l->length, nw, lines, starts, ends);
		if (!line && lines > 0)
		{
			lines--;
			starts[lines] = NULL;
			ends[lines] = NULL;
			alphas[lines] = a;
		}
		while(line --> 0 && lines > 0)
		{
			lines--;
			starts[lines] = starts[line];
			ends[lines] = ends[line];
			alphas[lines] = a;
		}
		if (lines == 0)
			break;
	}

	//clamp it properly
	while (lines < con->notif_l-maxlines)
	{
		lines++;
	}
	if (con->flags & CONF_NOTIFY_BOTTOM)
		y -= (con->notif_l - lines) * Font_CharHeight();

	while (lines < con->notif_l)
	{
		x = 0;
		R2D_ImageColours(1, 1, 1, alphas[lines]);
		if (con->flags & CONF_NOTIFY_RIGHT)
		{
			for (c = starts[lines]; c < ends[lines]; )
			{
				c = Font_Decode(c, &codeflags, &codepoint);
				x += Font_CharWidth(codeflags, codepoint);
			}
			x = (nw - x);
		}
		else if (con_centernotify.value)
		{
			for (c = starts[lines]; c < ends[lines]; )
			{
				c = Font_Decode(c, &codeflags, &codepoint);
				x += Font_CharWidth(codeflags, codepoint);
			}
			x = (nw - x) / 2;
		}
		Font_LineDraw(nx+x, y, starts[lines], ends[lines]);

		y += Font_CharHeight();

		lines++;
	}

	Font_EndString(font_console);

	R2D_ImageColours(1,1,1,1);
}

void Con_ClearNotify(void)
{
	console_t *con;
	conline_t *l;
	for (con = con_head; con; con = con->next)
	{
		for (l = con->current; l; l = l->older)
			l->flags |= CONL_NONOTIFY;
	}
}
void Con_DrawNotify (void)
{
	extern int startuppending;
	console_t *con;

	if (con_main)
	{
		/*keep the main console up to date*/
		con_main->notif_l = con_numnotifylines.ival;
		con_main->notif_w = con_notify_w.value;
		con_main->notif_x = con_notify_x.value;
		con_main->notif_y = con_notify_y.value;
		con_main->notif_t = con_notifytime.value;
	}

	if (con_chat)
	{
		con_chat->notif_l = con_numnotifylines_chat.ival;
		con_chat->notif_w = 1;
		con_chat->notif_y = (vid.height - sb_lines - 8*4) / vid.width;
		con_chat->notif_t = con_notifytime_chat.value;
	}

	if (startuppending)
	{
		int x,y;
		Font_BeginString(font_console, 0, 0, &x, &y);
		Con_DrawProgress(0, vid.width, 0);
		Font_EndString(font_console);
	}
	else
	{
		for (con = con_head; con; con = con->next)
		{
			if (con->flags & CONF_NOTIFY)
				Con_DrawNotifyOne(con);
		}
	}

	if (Key_Dest_Has(kdm_message))
	{
		int x, y;
		conchar_t *starts[8];
		conchar_t *ends[8];
		conchar_t markup[MAXCMDLINE+64];
		conchar_t *c, *end;
		char demoji[8192];
		char *foo = va(chat_team?"say_team: %s":"say: %s", Key_Demoji(demoji, sizeof(demoji), chat_buffer?(char*)chat_buffer:""));
		int lines, i, pos;
		Font_BeginString(font_console, 0, 0, &x, &y);
		y = con_numnotifylines.ival * Font_CharHeight();

		i = chat_team?10:5;
		pos = strlen(foo)+i;
		pos = min(pos, chat_bufferpos + i);

		//figure out where the cursor is, if its safe
		i = foo[pos];
		foo[pos] = 0;
		c = COM_ParseFunString(CON_WHITEMASK, foo, markup, sizeof(markup), PFS_KEEPMARKUP|PFS_FORCEUTF8);
		foo[pos] = i;

		//k, build the string properly.
		end = COM_ParseFunString(CON_WHITEMASK, foo, markup, sizeof(markup) - sizeof(markup[0])-1, PFS_KEEPMARKUP | PFS_FORCEUTF8);

		//and overwrite the cursor so that it blinks.
		*end = ' '|CON_WHITEMASK;
		if (((int)(realtime*con_cursorspeed)&1))
			*c = 0xe00b|CON_WHITEMASK;
		if (c == end)
			end++;

		lines = Font_LineBreaks(markup, end, Font_ScreenWidth(), countof(starts), starts, ends);
		for (i = 0; i < lines; i++)
		{
			x = 0;
			Font_LineDraw(x, y, starts[i], ends[i]);
			y += Font_CharHeight();
		}
		Font_EndString(font_console);

		vid.ime_allow = true;
		vid.ime_position[0] = 0;
		vid.ime_position[1] = y;
	}
}

//send all the stuff that was con_printed to sys_print.
//This is so that system consoles in windows can scroll up and have all the text.
void Con_PrintToSys(void)
{
	console_t *curcon = con_main;
	conline_t *l;
	int i;
	conchar_t *t;
	char buf[16];

	if (!curcon)
		return;

	for (l = curcon->oldest; l; l = l->newer)
	{
		t = (conchar_t*)(l+1);
		//fixme: utf8?
		for (i = 0; i < l->length; i++)
		{
			if (!(t[i] & CON_HIDDEN))
			{
				if (com_parseutf8.ival>0)
				{
					int cl = utf8_encode(buf, t[i]&CON_CHARMASK, sizeof(buf)-1);
					if (cl)
					{
						buf[cl] = 0;
						Sys_Printf("%s", buf);
					}
				}
				else
					Sys_Printf("%c", t[i]&0xff);
			}
		}
		Sys_Printf("\n");
	}
}

//returns the bottom of the progress bar
static int Con_DrawProgress(int left, int right, int y)
{
	conchar_t			dlbar[1024], *chr;
	unsigned char	progresspercenttext[128];
	const char *progresstext = NULL;
	const char *txt;
	int x, tw;
	int i;
	int barwidth, barleft;
	float progresspercent = 0;
	unsigned int codeflags, codepoint;
	*progresspercenttext = 0;

	// draw the download bar
	// figure out width
	if (cls.download)
	{
		unsigned int count;
		qofs_t total;
		qboolean extra;
		progresstext = cls.download->localname;
		progresspercent = cls.download->percent;

		if (cls.download->sizeunknown && cls.download->size == 0)
			progresspercent = -1;

		CL_GetDownloadSizes(&count, &total, &extra);

		if (progresspercent < 0)
		{
			if ((int)(realtime/2)&1 || total == 0)
				sprintf(progresspercenttext, " (%ukB/s)", CL_DownloadRate()/1000);
			else
			{
				char tmp[64];
				sprintf(progresspercenttext, " (%s%s)", FS_AbbreviateSize(tmp,sizeof(tmp), total), extra?"+":"");
			}

			//do some marquee thing, so the user gets the impression that SOMETHING is happening.
			progresspercent = realtime - (int)realtime;
			if ((int)realtime & 1)
				progresspercent  = 1 - progresspercent;
			progresspercent *= 100;
		}
		else
		{
			if ((int)(realtime/2)&1 || total == 0)
				sprintf(progresspercenttext, " %5.1f%% (%ukB/s)", progresspercent, CL_DownloadRate()/1000);
			else
			{
				sprintf(progresspercenttext, " %5.1f%% (%u%sKiB)", progresspercent, (int)(total/1024), extra?"+":"");
			}
		}
	}
#ifdef RUNTIMELIGHTING
	else if ((progresstext=RelightGetProgress(&progresspercent)))
	{
		sprintf(progresspercenttext, " %02d%%", (int)progresspercent);
	}
#endif

	//at this point:
	//progresstext: what is being downloaded/done (can end up truncated)
	//progresspercent: its percentage (used only for the slider)
	//progresspercenttext: that percent as text, essentually the right hand part of the bar.

	if (progresstext)
	{
		//chop off any leading path
		if ((txt = strrchr(progresstext, '/')) != NULL)
			txt++;
		else
			txt = progresstext;

		x = 0;
		COM_ParseFunString(CON_WHITEMASK, txt, dlbar, sizeof(dlbar), false);
		for (i=0,chr = dlbar; *chr; )
		{
			chr = Font_Decode(chr, &codeflags, &codepoint);
			x += Font_CharWidth(codeflags, codepoint);
			i++;
		}

		//if the string is wider than a third of the screen
		if (x > (right - left)/3)
		{
			//truncate the file name and add ...
			x += 3*Font_CharWidth(CON_WHITEMASK, '.');
			while (x > (right - left)/3)
			{
				chr = Font_DecodeReverse(chr, dlbar, &codeflags, &codepoint);
				x -= Font_CharWidth(codeflags, codepoint);
			}

			dlbar[i++] = '.'|CON_WHITEMASK;
			dlbar[i++] = '.'|CON_WHITEMASK;
			dlbar[i++] = '.'|CON_WHITEMASK;
			dlbar[i] = 0;
		}

		//i is the char index of the dlbar so far, x is the char width of it.

		//add a couple chars
		dlbar[i] = ':'|CON_WHITEMASK;
		x += Font_CharWidth(CON_WHITEMASK, ':');
		i++;
		dlbar[i] = ' '|CON_WHITEMASK;
		x += Font_CharWidth(CON_WHITEMASK, ' ');
		i++;

		COM_ParseFunString(CON_WHITEMASK, progresspercenttext, dlbar+i, sizeof(dlbar)-i*sizeof(conchar_t), false);
		for (chr = &dlbar[i], tw = 0; *chr; )
		{
			chr = Font_Decode(chr, &codeflags, &codepoint);
			tw += Font_CharWidth(codeflags, codepoint);
		}

		barwidth = (right-left) - (x + tw);

		//draw the right hand side
		x = right - tw;
		for (chr = &dlbar[i]; *chr; )
		{
			chr = Font_Decode(chr, &codeflags, &codepoint);
			x = Font_DrawChar(x, y, codeflags, codepoint);
		}

		//draw the left hand side
		x = left;
		for (chr = dlbar; chr < &dlbar[i]; )
		{
			chr = Font_Decode(chr, &codeflags, &codepoint);
			x = Font_DrawChar(x, y, codeflags, codepoint);
		}

		//and in the middle we have lots of stuff

		barwidth -= (Font_CharWidth(CON_WHITEMASK, 0xe080) + Font_CharWidth(CON_WHITEMASK, 0xe082));
		x = Font_DrawChar(x, y, CON_WHITEMASK, 0xe080);
		barleft = x;
		for(;;)
		{
			if (x + Font_CharWidth(CON_WHITEMASK, 0xe081) > barleft+barwidth)
				break;
			x = Font_DrawChar(x, y, CON_WHITEMASK, 0xe081);
		}
		x = Font_DrawChar(x, y, CON_WHITEMASK, 0xe082);

		if (progresspercent >= 0)
			Font_DrawChar(barleft+(barwidth*progresspercent)/100 - Font_CharWidth(CON_WHITEMASK, 0xe083)/2, y, CON_WHITEMASK, 0xe083);

		y += Font_CharHeight();
	}
	return y;
}

//draws console selection choices at the top of the screen, if multiple consoles are available
//its ctrl+tab to switch between them
int Con_DrawAlternateConsoles(int lines)
{
	char *txt;
	int x, y = 0, lx;
	int consshown = 0;
	console_t *con, *om = con_mouseover;
	conchar_t buffer[512], *end, *start;
	unsigned int codeflags, codepoint;

	for (con = con_head; con; con = con->next)
	{
		if (!(con->flags & (CONF_HIDDEN|CONF_ISWINDOW)))
			consshown++;
	}

	if (lines == (int)scr_con_target && consshown > 1)
	{
		int mx, my, h;
		Font_BeginString(font_console, mousecursor_x, mousecursor_y, &mx, &my);
		Font_BeginString(font_console, 0, y, &x, &y);
		h = Font_CharHeight();
		for (x = 0, con = con_head; con; con = con->next)
		{
			if (con->flags & (CONF_HIDDEN|CONF_ISWINDOW))
				continue;
			txt = con->title;

			//yeah, om is an evil 1-frame delay. whatever
			end = COM_ParseFunString(CON_WHITEMASK, va("^&%c%i%s", ((con!=om)?'F':'B'), (con==con_current)+con->unseentext*4, txt), buffer, sizeof(buffer), false);

			lx = 0;
			for (lx = x, start = buffer; start < end; )
			{
				start = Font_Decode(start, &codeflags, &codepoint);
				lx = Font_CharEndCoord(font_console, lx, codeflags, codepoint);
			}
			if (lx > Font_ScreenWidth())
			{
				x = 0;
				y += h;
			}
			for (lx = x, start = buffer; start < end; )
			{
				start = Font_Decode(start, &codeflags, &codepoint);
				lx = Font_DrawChar(lx, y, codeflags, codepoint);
			}
			lx += 8;
			if (mx >= x && mx < lx && my >= y && my < y+h)
				con_mouseover = con;
			x = lx;
		}
		y+= h;
		Font_EndString(font_console);

		y = (y*(int)vid.height) / (float)vid.rotpixelheight;
	}
	return y;
}

static void Con_DrawImageClip(float x, float y, float w, float h, float bottom, shader_t *pic)
{
	if (bottom < y+h)
	{
		if (bottom <= y)
			return;
		R2D_Image(x,y,w,bottom-y,0,0,1,(bottom-y)/h,pic);
	}
	else
		R2D_Image(x,y,w,h,0,0,1,1,pic);
}

//draws the conline_t list bottom-up within the width of the screen until the top of the screen is reached.
//if text is selected, the selstartline globals will be updated, so make sure the lines persist or check them.
static int Con_DrawConsoleLines(console_t *con, conline_t *l, float displayscroll, int sx, int ex, int y, int top, int selactive, int selsx, int selex, int selsy, int seley, float lineagelimit)
{
	int linecount;
	conchar_t *starts[64], *ends[sizeof(starts)/sizeof(starts[0])];
	conchar_t *s, *e, *c;
	int x;
	int charh = Font_CharHeight();
	unsigned int codeflags, codepoint;
	float alphaval = 1;
	float chop;

	chop = displayscroll * Font_CharHeight();

	if (l != con->completionline)
	if (l != con->footerline)
	if (l != con->current)
	{
		y -= Font_CharHeight();
	// draw arrows to show the buffer is backscrolled
		for (x = sx ; x<ex; )
			x = (Font_DrawChar (x, y, CON_WHITEMASK, '^')-x)*4+x;

		if (chop)
		{
			y -= Font_CharHeight();
			chop += 2*Font_CharHeight();
		}
	}

	y += chop;

	if (selactive != -1)
	{
		if (!selactive)
			selactive = 2;	//calculate, but don't draw (to track mouse-over)

		//deactivate the selection if the start and end is outside
		if (
			(selsx < sx && selex < sx) ||
			(selsx > ex && selex > ex) ||
			(selsy < top && seley < top) ||
			(selsy > y && seley > y)
			)
			selactive = false;	//don't track it at all

		if (selactive)
		{
			//clip it
			if (selsx < sx)
				selsx = sx;
			if (selex < sx)
				selex = sx;

			if (selsy > y)
				selsy = y;
			if (seley > y)
				seley = y;

			//scale the y coord to be in lines instead of pixels
			selsy -= y;
			seley -= y;
	//		selsy -= charh;
	//		seley -= charh;

			//invert the selections to make sense, text-wise
			/*if (selsy == seley)
			{
				//single line selected backwards
				if (selex < selsx)
				{
					x = selex;
					selex = selsx;
					selsx = x;
				}
			}
			*/
			if (seley <= selsy)
			{	//selection goes upwards
				x = selsy;
				selsy = seley;
				seley = x;

				x = selex;
				selex = selsx;
				selsx = x;
				con->flags &= ~CONF_BACKSELECTION;
			}
			else
				con->flags |= CONF_BACKSELECTION;
	//		selsy *= Font_CharHeight();
	//		seley *= Font_CharHeight();
			selsy += y;
			seley += y;
		}
	}

	if (l && l == con->current && l->length == 0 && con->userline != l)
		l = l->older;
	for (; l; l = l->older)
	{
		shader_t *pic = NULL;
		float picw=0, pich=0;
		s = (conchar_t*)(l+1);

		if (lineagelimit)
		{
			alphaval = realtime - (l->time+lineagelimit);
			if (alphaval > 0)
			{
				float fadetime = con->notif_fade?con->notif_fade:1;
				if (alphaval > fadetime)
					break;	//we're done here
				alphaval = 1 - (alphaval/fadetime);
			}
			else
				alphaval = 1;
		}

		if (l->length >= 2 && *s == CON_LINKSTART && (s[1]&CON_CHARMASK) == '\\')
		{	//leading tag with no text, look for an image in there
			conchar_t *e;
			char linkinfo[256];
			int linkinfolen = 0;
			for (e = s+1; e < s+l->length; e++)
			{
				if (*e == CON_LINKEND)
				{
					char *imgname;
					linkinfo[linkinfolen] = 0;

					imgname = Info_ValueForKey(linkinfo, "imgptr");
					if (*imgname)
					{
						image_t *img = Image_TextureIsValid(strtoull(imgname, NULL, 0));
						if (img && (img->flags & IF_TEXTYPEMASK)==IF_TEXTYPE_CUBE)
						{
							pic = R_RegisterShader("tiprawimgcube", 0, "{\nprogram postproc_equirectangular\n{\nmap \"$cube:$reflectcube\"\n}\n}");
							pic->defaulttextures->reflectcube = img;
						}
						else if (img && (img->flags & IF_TEXTYPEMASK)==IF_TEXTYPE_2D_ARRAY)
						{
							pic = R_RegisterShader("tiprawimgarray", 0, "{\nprogram default2danim\n{\nmap \"$2darray:$diffuse\"\n}\n}");
							pic->defaulttextures->base = img;
						}
						else
						{
							pic = R2D_SafeCachePic("tiprawimg");
							pic->defaulttextures->base = img;
						}
						if (img)
						{
							if (!img->width || !img->height || !TEXLOADED(img))
								picw = pich = 64;
							else if (img->width > img->height)
							{
								picw = 64;
								pich = (64.0*img->height)/img->width;
							}
							else
							{
								picw = (64.0*img->width)/img->height;
								pich = 64;
							}
							break;
						}
					}


					imgname = Info_ValueForKey(linkinfo, "img");
					if (*imgname)
					{
						char *fl = Info_ValueForKey(linkinfo, "imgtype");
						if (*fl)
							pic = R_RegisterCustom(NULL, imgname, atoi(fl), NULL, NULL);
						else
							pic = R_RegisterPic(imgname, NULL);
						if (pic)
						{
							imgname = Info_ValueForKey(linkinfo, "s");
							if (*imgname)
							{
								if (pic->width <= 0 || pic->height <= 0)
									picw = pich = 64;
								else if (pic->width > pic->height)
								{
									picw = atof(imgname);
									pich = picw * (float)pic->height/pic->width;
								}
								else
								{
									pich = atof(imgname);
									picw = pich * (float)pic->width/pic->height;
								}
							}
							else
							{
								imgname = Info_ValueForKey(linkinfo, "w");
								if (*imgname)
									picw = atof(imgname);
								else
									picw = -1;
								imgname = Info_ValueForKey(linkinfo, "h");
								if (*imgname)
									pich = atof(imgname);
								else
									pich = -1;

								if (picw<0 && pich<0)
								{
									if (pic->width && pic->height)
									{
										pich = (pic->height * vid.pixelheight) / vid.height;
										picw = (pic->width * vid.pixelwidth) / vid.width;
									}
									else
										picw = pich = 64;
								}
								else if (picw<0)
									picw = pich * (float)pic->width/pic->height;
								else if (pich<0)
									pich = picw * (float)pic->height/pic->width;
							}
							picw *= charh/8.0;
							pich *= charh/8.0;

							if (picw >= ex-sx)
							{
								pich *= (float)(ex-sx) / picw;
								picw = ex-sx;
							}
						}

						//a fall back image (mostly for delay-loading or whatever.
						if (R_GetShaderSizes(pic, NULL, NULL, false) <= 0)
						{
							imgname = Info_ValueForKey(linkinfo, "fbimg");
							if (*imgname)
								pic = R_RegisterPic(imgname, NULL);
						}
					}
					break;
				}
				linkinfolen += unicode_encode(linkinfo+linkinfolen, (*e & CON_CHARMASK), sizeof(linkinfo)-1-linkinfolen, true);
			}
		}

		if (con->flags & CONF_NOWRAP)
		{
			linecount = 1;
			starts[0] = s;
			ends[0] = s+l->length;
		}
		else
		{
			linecount = Font_LineBreaks(s, s+l->length, ex-sx-picw, sizeof(starts)/sizeof(starts[0]), starts, ends);
			//if Con_LineBreaks didn't find any lines at all, then it was an empty line, and we need to ensure that its still drawn
			if (linecount == 0 && !pic)
			{
				linecount = 1;
				starts[0] = ends[0] = s;
			}
		}

		if (pic)
		{
			float szx = (float)vid.width / vid.pixelwidth;
			float szy = (float)vid.height / vid.pixelheight;
			int texth = (linecount) * Font_CharHeight();
			if (R2D_Flush)
				R2D_Flush();
			R2D_ImageColours(1.0, 1.0, 1.0, 1.0);
			if (texth > pich)
			{
				texth = pich + (texth-pich)/2;
				Con_DrawImageClip(sx*szx, (y-texth)*szy, picw*szx, pich*szy, (y-chop+Font_CharHeight())*szy, pic);
				pich = 0;	//don't pad the text...
			}
			else
			{
				Con_DrawImageClip(sx*szx, (y-pich)*szy, picw*szx, pich*szy, (y-chop+Font_CharHeight())*szy, pic);
				pich -= texth;
				y-= pich/2;	//skip some space above and below the text block, to keep the text and image aligned.

				if (chop)
					chop -= pich/2;
			}
			if (R2D_Flush)
				R2D_Flush();

//			if (selsx < picw && selex < picw)

			l->numlines = ceil((texth+pich)/Font_CharHeight());
		}
		else
			l->numlines = linecount;

		while(linecount-- > 0)
		{
			s = starts[linecount];
			e = ends[linecount];

			y -= Font_CharHeight();

			if (chop)
			{
				chop -= Font_CharHeight();
				if (chop < 0)
					chop = 0;
				else
					continue;
			}

			if (top && y < top)
				break;

			if (l->flags & (CONL_BREAKPOINT|CONL_EXECUTION))
			{
				if (l->flags & CONL_EXECUTION)
				{
					if (l->flags & CONL_BREAKPOINT)
						R2D_ImageColours(SRGBA(0.3,0.15,0.0, alphaval));
					else
						R2D_ImageColours(SRGBA(0.3,0.3,0.0, alphaval));
				}
				else //if (l->flags & CONL_BREAKPOINT)
					R2D_ImageColours(SRGBA(0.3,0.0,0.0, alphaval));
				R2D_FillBlock((sx*(float)vid.width)/(float)vid.rotpixelwidth, (y*vid.height)/(float)vid.rotpixelheight, ((ex - sx)*vid.width)/(float)vid.rotpixelwidth, (Font_CharHeight()*vid.height)/(float)vid.rotpixelheight);
				R2D_Flush();
			}

			if (selactive < 0)
			{	//display an existing selection
				int sstart = picw;
				int send = sstart;
				int center;
				if (selactive == -2 || l == con->selendline || l == con->selstartline)
				{
					for (c = s; c < e; )
					{
						c = Font_Decode(c, &codeflags, &codepoint);
						send = Font_CharEndCoord(font_console, send, codeflags, codepoint);
					}
					//show something on blank lines
					if (send == sstart)
						send = Font_CharEndCoord(font_console, send, CON_WHITEMASK, ' ');

					center = sx;
					if (l->flags&CONL_CENTERED)
						center += ((ex-sx) - send)/2;
				
					if (l == con->selendline)
					{
						selactive = -2;	//all following lines need to be selected, until we see the other end of the selection
						send = sstart;
						for (c = s; c < (conchar_t*)(con->selendline+1)+con->selendoffset; )
						{
							c = Font_Decode(c, &codeflags, &codepoint);
							send = Font_CharEndCoord(font_console, send, codeflags, codepoint);
						}
					}
					if (l == con->selstartline)
					{
						for (c = s; c < (conchar_t*)(con->selstartline+1)+con->selstartoffset; )
						{
							c = Font_Decode(c, &codeflags, &codepoint);
							sstart = Font_CharEndCoord(font_console, sstart, codeflags, codepoint);
						}
						if (c == (conchar_t*)(con->selstartline+1)+con->selstartoffset)
							selactive = 0;	//no need to track any other selections.
					}

					sstart += center;
					send += center;

					R2D_ImageColours(SRGBA(0.1,0.1,0.3, alphaval));
					if (send < sstart)
						R2D_FillBlock((send*(float)vid.width)/(float)vid.rotpixelwidth, (y*vid.height)/(float)vid.rotpixelheight, ((sstart - send)*vid.width)/(float)vid.rotpixelwidth, (Font_CharHeight()*vid.height)/(float)vid.rotpixelheight);
					else
						R2D_FillBlock((sstart*(float)vid.width)/(float)vid.rotpixelwidth, (y*vid.height)/(float)vid.rotpixelheight, ((send - sstart)*vid.width)/(float)vid.rotpixelwidth, (Font_CharHeight()*vid.height)/(float)vid.rotpixelheight);
					R2D_Flush();
				}
			}
			else if (selactive)
			{
				if (y+charh >= selsy)
				{
					if (y < seley)
					{
						int sstart;
						int send;
						int center;
						send = sstart = picw;
						for (c = s; c < e; )
						{
							c = Font_Decode(c, &codeflags, &codepoint);
							send = Font_CharEndCoord(font_console, send, codeflags, codepoint);
						}

						//show something on blank lines
						if (send == sstart)
							send = Font_CharEndCoord(font_console, send, CON_WHITEMASK, ' ');

						center = sx;
						if (l->flags&CONL_CENTERED)
							center += ((ex-sx) - send)/2;

						if (y+charh >= seley && y < selsy)
						{	//if they're both on the same line, make sure sx is to the left of ex, so our stuff makes sense
							if (selex < selsx)
							{
								x = selex;
								selex = selsx;
								selsx = x;
							}
						}

						if (y+charh >= seley)
						{
							send = sstart;
							for (c = s; c < e; )
							{
								c = Font_Decode(c, &codeflags, &codepoint);
								send = Font_CharEndCoord(font_console, send, codeflags, codepoint);

								if (send+center > selex)
									break;
							}

							con->selendline = l;
							if (s)
								con->selendoffset = c - (conchar_t*)(l+1);
							else
								con->selendoffset = 0;
						}
						if (y < selsy)
						{
							for (c = s; c < e; )
							{
								Font_Decode(c, &codeflags, &codepoint);
								x = Font_CharEndCoord(font_console, sstart, codeflags, codepoint);
								if (x+center > selsx)
									break;
								c = Font_Decode(c, &codeflags, &codepoint);
								sstart = x;
							}

							con->selstartline = l;
							if (s)
								con->selstartoffset = c - (conchar_t*)(l+1);
							else
								con->selstartoffset = 0;

							if (selactive == 2 && s)
							{	//checking for mouseover
								//scan earlier to find any link enclosure
								for(c--; c >= (conchar_t*)(l+1); c--)
								{
									if (*c == CON_LINKSTART)
									{
										selactive = 3;	//we're mouse-overing a link!
										con->selstartoffset = c - (conchar_t*)(l+1);
										break;
									}
									if (*c == CON_LINKEND)
										break;	//some other link ended here. don't use its start.
								}

								if (selactive == 3 && con->selendline==l)
								{
									for (; c < (conchar_t*)(l+1)+l->length; c++)
										if (*c == CON_LINKEND)
										{
											con->selendoffset = c - (conchar_t*)(l+1);
											break;
										}

									sstart = picw;
									for (c = s; c < (conchar_t*)(l+1)+con->selstartoffset; )
									{
										c = Font_Decode(c, &codeflags, &codepoint);
										sstart = Font_CharEndCoord(font_console, sstart, codeflags, codepoint);
									}
									send = sstart;
									for (; c < (conchar_t*)(l+1)+con->selendoffset; )
									{
										c = Font_Decode(c, &codeflags, &codepoint);
										send = Font_CharEndCoord(font_console, send, codeflags, codepoint);
									}
								}
							}
						}

						sstart += center;
						send += center;

						if (selactive != 2)
						{
							if (selactive == 1)
								R2D_ImageColours(SRGBA(0.1,0.1,0.3, alphaval));	//selected
							else
								R2D_ImageColours(SRGBA(0.3,0.3,0.3, alphaval));	//mouseover.

							if (send < sstart)
							{
								center = sstart;
								sstart = send;
								send = center;
							}
							if (selactive == 3)	//2 pixels high
								R2D_FillBlock((sstart*vid.width)/(float)vid.rotpixelwidth, ((y+Font_CharHeight()-2)*vid.height)/(float)vid.rotpixelheight, ((send - sstart)*vid.width)/(float)vid.rotpixelwidth, (2*vid.height)/(float)vid.rotpixelheight);
							else				//full height
								R2D_FillBlock((sstart*vid.width)/(float)vid.rotpixelwidth, (y*vid.height)/(float)vid.rotpixelheight, ((send - sstart)*vid.width)/(float)vid.rotpixelwidth, (Font_CharHeight()*vid.height)/(float)vid.rotpixelheight);
							R2D_Flush();
						}
					}
				}
			}
			R2D_ImageColours(1.0, 1.0, 1.0, alphaval);

			x = sx + picw;

			if (l->flags&CONL_CENTERED)
			{
				int send = 0;
				for (c = s; c < e; )
				{
					c = Font_Decode(c, &codeflags, &codepoint);
					send = Font_CharEndCoord(font_console, send, codeflags, codepoint);
				}

				x += ((ex-sx) - send)/2;
			}

			Font_LineDraw(x, y, s, e);


			if (con->userline == l && s <= (conchar_t*)(l+1)+con->useroffset && (conchar_t*)(l+1)+con->useroffset <= e)
			if ((int)(realtime*4)&1)
			{
				int sstart;
				sstart = picw;
				for (c = s; c < (conchar_t*)(l+1)+con->useroffset; )
				{
					c = Font_Decode(c, &codeflags, &codepoint);
					sstart = Font_CharEndCoord(font_console, sstart, codeflags, codepoint);
				}
				Font_DrawChar(sx+sstart, y, CON_WHITEMASK, 0xe00b);
			}

			if (y < top)
				break;
		}
		y -= pich/2;
		if (chop)
			chop -= pich/2;
		if (y < top)
			break;
	}
	return y;
}

void Draw_ExpandedString(float x, float y, conchar_t *str);

static void Con_DrawModelPreview(model_t *model, float x, float y, float w, float h)
{
	playerview_t pv;
	entity_t ent;
	vec3_t fwd, rgt, up;
	vec3_t lightpos = {1, 1, 0};
	float transforms[12];
	float scale;

	if (R2D_Flush)
		R2D_Flush();

	memset(&pv, 0, sizeof(pv));

	CL_DecayLights ();
	CL_ClearEntityLists();
	V_ClearRefdef(&pv);
	r_refdef.drawsbar = false;
	V_CalcRefdef(&pv);

	r_refdef.grect.width = w;
	r_refdef.grect.height = h;
	r_refdef.grect.x = x;
	r_refdef.grect.y = y;
	r_refdef.time = realtime;

	r_refdef.flags = RDF_NOWORLDMODEL;

	r_refdef.afov = 60;
	r_refdef.fov_x = 0;
	r_refdef.fov_y = 0;
	r_refdef.dirty |= RDFD_FOV;

	VectorClear(r_refdef.viewangles);
	r_refdef.viewangles[0] = 20;
//	r_refdef.viewangles[1] = realtime * 90;
	AngleVectors(r_refdef.viewangles, fwd, rgt, up);
	VectorScale(fwd, -64, r_refdef.vieworg);

	memset(&ent, 0, sizeof(ent));
	ent.model = model;
	ent.scale = 1;
	ent.angles[1] = realtime*90;//mods->yaw;
//	ent.angles[0] = realtime*23.4;//mods->pitch;
	AngleVectorsMesh(ent.angles, ent.axis[0], ent.axis[1], ent.axis[2]);
	VectorInverse(ent.axis[1]);

	//ent.origin[2] -= (ent.model->maxs[2]-ent.model->mins[2]) * 0.5 + ent.model->mins[2];

	ent.scale = 1;
	scale = max(max(fabs(ent.model->maxs[0]-ent.model->mins[0]), fabs(ent.model->maxs[1]-ent.model->mins[1])), fabs(ent.model->maxs[2]-ent.model->mins[2]));
	scale = scale?64.0/scale:1;
	ent.origin[2] -= (ent.model->maxs[2]-ent.model->mins[2]) * 0.5 + ent.model->mins[2];
	Vector4Set(ent.shaderRGBAf, 1, 1, 1, 1);
	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], scale, ent.axis[2]);
	ent.topcolour = TOP_DEFAULT;
	ent.bottomcolour = BOTTOM_DEFAULT;
//	ent.fatness = sin(realtime)*5;
	ent.playerindex = -1;
	ent.skinnum = 0;
	ent.shaderTime = 0;//realtime;
	ent.framestate.g[FS_REG].lerpweight[0] = 1;
	ent.framestate.g[FS_REG].frametime[0] = ent.framestate.g[FS_REG].frametime[1] = realtime;
	ent.framestate.g[FS_REG].endbone = 0x7fffffff;
	if (model->submodelof)
		;
	else
	{
		ent.customskin = Mod_RegisterSkinFile(va("%s_0.skin", model->publicname));
		if (ent.customskin == 0)
		{
			char haxxor[MAX_QPATH];
			COM_StripExtension(model->publicname, haxxor, sizeof(haxxor));
			ent.customskin = Mod_RegisterSkinFile(va("%s_default.skin", haxxor));
		}
	}

	Vector4Set(ent.shaderRGBAf, 1,1,1,1);
	VectorSet(ent.glowmod, 1,1,1);
	ent.light_avg[0] = ent.light_avg[1] = ent.light_avg[2] = 0.66;
	ent.light_range[0] = ent.light_range[1] = ent.light_range[2] = 0.33;

	V_ApplyRefdef();

	if (ent.model->camerabone>0 && Mod_GetTag(ent.model, ent.model->camerabone, &ent.framestate, transforms))
	{
		VectorClear(ent.origin);
		AngleVectorsMesh(ent.angles, ent.axis[0], ent.axis[1], ent.axis[2]);
		VectorInverse(ent.axis[1]);
		scale = 1;
		{
			vec3_t fwd, up;
			float camera[12], et[12] = {
				ent.axis[0][0], ent.axis[1][0], ent.axis[2][0], ent.origin[0],
				ent.axis[0][1], ent.axis[1][1], ent.axis[2][1], ent.origin[1],
				ent.axis[0][2], ent.axis[1][2], ent.axis[2][2], ent.origin[2],
				};

			R_ConcatTransforms((void*)et, (void*)transforms, (void*)camera);
			VectorSet(fwd, camera[2], camera[6], camera[10]);
			VectorNegate(fwd, fwd);
			VectorSet(up, camera[1], camera[5], camera[9]);
			VectorSet(r_refdef.vieworg, camera[3], camera[7], camera[11]);
			VectorAngles(fwd, up, r_refdef.viewangles, false);
		}
	}
	else
	{
		ent.angles[1] = realtime*90;//mods->yaw;
		AngleVectorsMesh(ent.angles, ent.axis[0], ent.axis[1], ent.axis[2]);
		VectorScale(ent.axis[0], scale, ent.axis[0]);
		VectorScale(ent.axis[1], -scale, ent.axis[1]);
		VectorScale(ent.axis[2], scale, ent.axis[2]);
	}

	ent.scale = scale;

	VectorNormalize(lightpos);
	ent.light_dir[0] = DotProduct(lightpos, ent.axis[0]);
	ent.light_dir[1] = DotProduct(lightpos, ent.axis[1]);
	ent.light_dir[2] = DotProduct(lightpos, ent.axis[2]);

	ent.light_known = 2;

	V_AddEntity(&ent);

	R_RenderView();
}

static void Con_DrawMouseOver(console_t *mouseconsole)
{
	char *tiptext = NULL;
	shader_t *shader = NULL;
	model_t *model = NULL;
	sfx_t	*audio = NULL;

	char *mouseover;
	if (!mouseconsole->mouseover || !mouseconsole->mouseover(mouseconsole, &tiptext, &shader))
	{
		mouseover = Con_CopyConsole(mouseconsole, false, true, true);
		if (mouseover)
		{
			char *end = strstr(mouseover, "^]");
			char *info = strchr(mouseover, '\\');
			if (!info)
				info = "";
			if (end)
				*end = 0;
#ifdef PLUGINS
			if (!Plug_ConsoleLinkMouseOver(mousecursor_x, mousecursor_y, mouseover+2, info))
#endif
			{
				char *key;
				key = Info_ValueForKey(info, "tipimg");
				if (*key)
				{
					char *fl = Info_ValueForKey(info, "tipimgtype");
					if (*fl)
						shader = R_RegisterCustom(NULL, key, atoi(fl), NULL, NULL);
					else
						shader = R2D_SafeCachePic(key);
				}
				else
				{
					image_t *img = NULL;
					key = Info_ValueForKey(info, "tiprawimg");
					if (*key)
					{
						img = Image_FindTexture(key, NULL, IF_NOREPLACE|IF_PREMULTIPLYALPHA|IF_TEXTYPE_ANY);
						if (!img)
							img = Image_FindTexture(key, NULL, IF_NOREPLACE|IF_TEXTYPE_ANY);
						if (!img)
						{
							size_t fsize;
							char *buf;
							img = Image_CreateTexture(key, NULL, IF_NOREPLACE|IF_PREMULTIPLYALPHA|IF_TEXTYPE_ANY);
							if ((buf = FS_LoadMallocFile (key, &fsize)))
								Image_LoadTextureFromMemory(img, img->flags|IF_NOWORKER, key, key, buf, fsize);
						}
					}

					key = Info_ValueForKey(info, "tipimgptr");
					if (*key)
						img = Image_TextureIsValid(strtoull(key, NULL, 0));
					if (img && img->status == TEX_LOADED)
					{
						if ((img->flags & IF_TEXTYPEMASK)==IF_TEXTYPE_CUBE)
						{
							shader = R_RegisterShader("tiprawimgcube", 0, "{\nprogram postproc_equirectangular\n{\nmap \"$cube:$reflectcube\"\n}\n}");
							shader->defaulttextures->reflectcube = img;
						}
						else if ((img->flags & IF_TEXTYPEMASK)==IF_TEXTYPE_2D_ARRAY)
						{
							shader = R_RegisterShader("tiprawimgarray", 0, "{\nprogram default2danim\n{\nmap \"$2darray:$diffuse\"\n}\n}");
							shader->defaulttextures->base = img;
						}
						else if ((img->flags&IF_TEXTYPEMASK) == IF_TEXTYPE_2D)
						{
							shader = R2D_SafeCachePic("tiprawimg");
							shader->defaulttextures->base = img;
						}

						if (shader)
						{
							shader->width = img->width;
							shader->height = img->height;
							if (shader->width > 320)
							{
								shader->height *= 320.0/shader->width;
								shader->width = 320;
							}
							if (shader->height > 240)
							{
								shader->width *= 240.0/shader->height;
								shader->height = 240;
							}
						}
					}
					else
						shader = NULL;
					if (!vrui.enabled)
					{
						key = Info_ValueForKey(info, "modelviewer");
						if (*key)
						{
							model = Mod_ForName(key, MLV_WARN);
							if (model->loadstate != MLS_LOADED)
								model = NULL;
						}
					}

					key = Info_ValueForKey(info, "playaudio");
					if (*key)
					{
						audio = S_PrecacheSound(key);
						if (audio && audio->loadstate != SLS_LOADED)
							audio = NULL;
					}
				}
				tiptext = Info_ValueForKey(info, "tip");
			}
			Z_Free(mouseover);
		}
	}
	if ((tiptext && *tiptext) || shader || model || audio)
	{
		//FIXME: draw a proper background.
		//FIXME: support line breaks.
		conchar_t buffer[2048], *starts[64], *ends[countof(starts)], *eot;
		int lines, i, px, py;
		float tw, th;
		float ih = 0, iw = 0;
		float x = mousecursor_x+8;
		float y = mousecursor_y+8;

		Font_BeginString(font_console, x, y, &px, &py);
		eot = COM_ParseFunString(CON_WHITEMASK, tiptext, buffer, sizeof(buffer), false);
		if (audio)
		{
			struct sfxcache_s cache;
			char name[MAX_OSPATH];
			float len;
			*name = 0;
			len = audio->decoder.querydata?audio->decoder.querydata(audio, &cache, name, sizeof(name)):-1;
			if (len >= 0)
			{
				eot = COM_ParseFunString(CON_WHITEMASK, va("\n\n%s\n%gkhz, %s, %ibit, %g seconds%s",
							name, cache.speed/1000.0, cache.numchannels==1?"mono":"stereo", QAF_BYTES(cache.format)*8, len, audio->loopstart>=0?" looped":""
							), eot, sizeof(buffer)-((char*)eot-(char*)buffer), false);
			}
			else
			{
				cache = *(struct sfxcache_s *)audio->decoder.buf;
				len = (double)cache.length / cache.speed;
				eot = COM_ParseFunString(CON_WHITEMASK, va("\n\n\n%gkhz, %s, %ibit, %g seconds%s",
							cache.speed/1000.0, cache.numchannels==1?"mono":"stereo", QAF_BYTES(cache.format)*8, len, audio->loopstart>=0?" looped":""
							), eot, sizeof(buffer)-((char*)eot-(char*)buffer), false);
			}
		}
		lines = Font_LineBreaks(buffer, eot, (256.0 * vid.pixelwidth) / vid.width, countof(starts), starts, ends);
		th = (Font_CharHeight()*lines * vid.height) / vid.pixelheight;

		if (model)
		{
			iw = 128;
			ih = 128;
		}
		else if (shader)
		{
			int w, h;
			if (R_GetShaderSizes(shader, &w, &h, false) >= 0)
			{
				iw = w;
				ih = h;
			}
			else
				shader = NULL;
		}
		if (iw  > (vid.width/4.0))
		{
			ih *= (vid.width/4.0)/iw;
			iw *= (vid.width/4.0)/iw;
		}
		if (ih  > (vid.height/4.0))
		{
			iw *= (vid.width/4.0)/ih;
			ih *= (vid.width/4.0)/ih;
		}

		if (x + iw/2 + 8 + 256 > vid.width)
			x = vid.width - (iw/2 + 8 + 256);
		if (x < iw/2)
			x = iw/2;
		x += iw/2 + 8;

		if (y+max(th, ih) > vid.height)
			y = mousecursor_y - 8 - max(th, ih);
		if (y < 0)
			y = 0;

		Font_BeginString(font_console, x, y + (max(th, ih) - th)/2, &px, &py);
		for (i = 0, tw = 0; i < lines; i++)
		{
			int lw = Font_LineWidth(starts[i], ends[i]);
			if (lw > tw)
				tw = lw;
		}
		tw *= (float)vid.width / vid.pixelwidth;
		Font_EndString(font_console);
		R2D_ImageColours(0, 0, 0, .75);
		R2D_FillBlock(x, y + (max(th, ih) - th)/2, tw, th);
		R2D_ImageColours(1, 1, 1, 1);
		Font_BeginString(font_console, x, y + (max(th, ih) - th)/2, &px, &py);
		for (i = 0; i < lines; i++)
		{
			Font_LineDraw(px, py, starts[i], ends[i]);
			py += Font_CharHeight();
		}
		Font_EndString(font_console);

		if (model)
			Con_DrawModelPreview(model, x-8-iw, y+((th>ih)?(th-ih)/2:0), iw, ih);
		if (shader)
		{
			if (th > ih)
				y += (th-ih)/2;
			R2D_Image(x-8-iw, y, iw, ih, 0, 0, 1, 1, shader);
		}
	}
}

/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawConsole (int lines, qboolean noback)
{
	extern qboolean scr_con_forcedraw;
	int x, y, sx, ex;
	conline_t *l;
	int selsx, selsy, selex, seley, selactive;
	qboolean haveprogress;
	console_t *w, *mouseconsole;
	float fadetime;

	if (!con_current)
		con_current = Con_GetMain();

	con_mouseover = NULL;

	//draw any windowed consoles (under main console)
	for (w = con_head; w; w = w->next)
	{
		srect_t srect;
		if ((w->flags & (CONF_HIDDEN|CONF_ISWINDOW)) != CONF_ISWINDOW)
			continue;

		if (Key_Dest_Has(kdm_cwindows))
			fadetime = 0;	//nothing fades when focused.
		else
			fadetime = 4;

		if (w->wnd_w > vid.width)
			w->wnd_w = vid.width;
		if (w->wnd_h > vid.height)
			w->wnd_h = vid.height;
		if (w->wnd_w < 64)
			w->wnd_w = 64;
		if (w->wnd_h < 16)
			w->wnd_h = 16;
		//windows that move off the top of the screen somehow are bad.
		if (w->wnd_y > vid.height - 8)
			w->wnd_y = vid.height - 8;
		if (w->wnd_y < 0)
			w->wnd_y = 0;
		if (w->wnd_x > vid.width-32)
			w->wnd_x = vid.width-32;
		if (w->wnd_x < -w->wnd_w+32)
			w->wnd_x = -w->wnd_w+32;

		if (w->wnd_h < 8)
			w->wnd_h = 8;

		if (mousecursor_x >= w->wnd_x && mousecursor_x < w->wnd_x+w->wnd_w && mousecursor_y >= w->wnd_y && mousecursor_y < w->wnd_y+w->wnd_h && mousecursor_y > lines)
			con_mouseover = w;

		w->mousecursor[0] = mousecursor_x - (w->wnd_x+8);
		w->mousecursor[1] = mousecursor_y - w->wnd_y;

		if (Key_Dest_Has(kdm_cwindows))
		{
			int top = 8;	//padding at the top
			if (con_curwindow==w)
				R2D_ImageColours(SRGBA(0.0, 0.05, 0.1, 0.8));
			else
				R2D_ImageColours(SRGBA(0.0, 0.05, 0.1, 0.5));
			R2D_FillBlock(w->wnd_x, w->wnd_y, w->wnd_w, w->wnd_h);
			R2D_ImageColours(1, 1, 1, 1);

			//fixme: scale up this font...
			Draw_FunStringWidth(w->wnd_x, w->wnd_y, w->title, w->wnd_w-top, 2, (con_curwindow==w)?true:false);
			Draw_FunStringWidth(w->wnd_x+w->wnd_w-top, w->wnd_y, "X", top, 2, ((w->buttonsdown == CB_CLOSE && w->mousecursor[0] > w->wnd_w-(8+top) && w->mousecursor[1] < top) || (con_curwindow==w && w->mousecursor[0] >= w->wnd_w-(8+top) && w->mousecursor[0] < w->wnd_w-8 && w->mousecursor[1] >= 0 && w->mousecursor[1] < 8))?true:false);

			if (w->backshader || *w->backimage)
			{
				shader_t *shader = w->backshader;
				if (!shader)
					shader = w->backshader = R_RegisterPic(w->backimage, NULL);// R_RegisterCustom(w->backimage, SUF_NONE, Shader_DefaultCinematic, w->backimage);
				if (shader)
				{
					float backx = w->wnd_x+8;
					float backy = w->wnd_y+top;
					float backw = w->wnd_w-16;
					float backh = w->wnd_h-8-top;
#ifdef HAVE_MEDIA_DECODER
					cin_t *cin = R_ShaderGetCinematic(shader);
					if (cin)
					{
						const char *url = Media_Send_GetProperty(cin, "url");
						if (url)
						{
							float x = 0;
//							float r = x+w->wnd_w-16;
							const char *buttons[] = {"bck", "fwd", "rld", "home", "", ((w->linebuffered == Con_Navigate)?(char*)key_lines[edit_line]:url)};
							const char *buttoncmds[] = {"cmd:back", "cmd:forward", "cmd:refresh",
							#ifdef QUAKETC	//total conversions should have their own website.
								ENGINEWEBSITE
							#else			//otherwise use some more useful page, for quake mods.
								"cmd:home"
							#endif
								, NULL, NULL};
							float tw;
							int i, fl;

							for (i = 0; i < countof(buttons); i++)
							{
								if (i == countof(buttons)-1)
									tw = FLT_MAX;
								else if (i == countof(buttons)-2)
								{
									tw = 8+8;
									if (*w->icon)
										R2D_Image(w->wnd_x+8+x, w->wnd_y+top, tw, tw, 0, 0, 1, 1, R_RegisterPic(w->icon, NULL));
									else
										tw = 0;
								}
								else if (i == countof(buttons)-3)
									tw = 40;
								else
									tw = 32;
								fl = con_curwindow==w;
								if (w->mousecursor[1] >= 8 && w->mousecursor[1] < 16 && w->mousecursor[0] >= x && w->mousecursor[0] < x+tw)
								{
									fl |= 2;
									if (w->buttonsdown == CB_ACTIONBAR)
									{
										w->buttonsdown = CB_NONE;
										if (buttoncmds[i])
											Media_Send_Command(cin, buttoncmds[i]);
										else if (w->linebuffered != Con_Navigate)
										{
											Key_ConsoleReplace(url);
											w->linebuffered = Con_Navigate;
										}
									}
								}
								if (tw > w->wnd_w-16 - x)
									tw = w->wnd_w-16 - x;
								Draw_FunStringWidth(w->wnd_x+8+x, w->wnd_y+top, buttons[i], tw, false, fl);
								x += tw;
							}
							top += 8;
							backy += 8;
							backh -= 8;
						}

						//convert these to pixels.
						backx = (backx*(int)vid.rotpixelwidth) / (float)vid.width;
						backy = (backy*(int)vid.rotpixelheight) / (float)vid.height;
						backw = (backw*(int)vid.rotpixelwidth) / (float)vid.width;
						backh = (backh*(int)vid.rotpixelheight) / (float)vid.height;
						//snap to pixels. this avoids issues with linear filtering
						backx = (int)backx;
						backy = (int)backy;
						backw = (int)backw;
						backh = (int)backh;
						Media_Send_Resize(cin, backw, backh);
						//convert back to screen coords now.
						backx = (backx*(int)vid.width) / (float)vid.rotpixelwidth;
						backy = (backy*(int)vid.height) / (float)vid.rotpixelheight;
						backw = (backw*(int)vid.width) / (float)vid.rotpixelwidth;
						backh = (backh*(int)vid.height) / (float)vid.rotpixelheight;

						Media_Send_MouseMove(cin, (w->mousecursor[0]) / backw, (w->mousecursor[1]-top) / backh);
						if (con_curwindow==w)
							Media_Send_Command(cin, "cmd:focus");
						else
							Media_Send_Command(cin, "cmd:unfocus");
					}
#endif
					R2D_Image(backx, backy, backw, backh, 0, 0, 1, 1, shader);
				}
			}

			w->unseentext = false;
		}
		else
			w->buttonsdown = 0;

		srect.x = (w->wnd_x+8) / vid.width;
		srect.y = (w->wnd_y+8) / vid.height;
		srect.width = (w->wnd_w-16) / vid.width;
		srect.height = (w->wnd_h-16) / vid.height;
		srect.dmin = -99999;
		srect.dmax = 99999;
		srect.y = (1-srect.y) - srect.height;
		if (srect.width && srect.height)
		{
			if (!fadetime)
			{
				R2D_ImageColours(SRGBA(0, 0.1, 0.2, 1.0));
				if ((w->buttonsdown & CB_SIZELEFT) || (con_curwindow==w && w->mousecursor[0] >= -8 && w->mousecursor[0] < 0 && w->mousecursor[1] >= 8 && w->mousecursor[1] < w->wnd_h))
					R2D_FillBlock(w->wnd_x, w->wnd_y+8, 8, w->wnd_h-8);
				if ((w->buttonsdown & CB_SIZERIGHT) || (con_curwindow==w && w->mousecursor[0] >= w->wnd_w-16 && w->mousecursor[0] < w->wnd_w-8 && w->mousecursor[1] >= 8 && w->mousecursor[1] < w->wnd_h))
					R2D_FillBlock(w->wnd_x+w->wnd_w-8, w->wnd_y+8, 8, w->wnd_h-8);
				if ((w->buttonsdown & CB_SIZEBOTTOM) || (con_curwindow==w && w->mousecursor[0] >= -8 && w->mousecursor[0] < w->wnd_w-8 && w->mousecursor[1] >= w->wnd_h-8 && w->mousecursor[1] < w->wnd_h))
					R2D_FillBlock(w->wnd_x, w->wnd_y+w->wnd_h-8, w->wnd_w, 8);
			}
			if (R2D_Flush)
				R2D_Flush();
			BE_Scissor(&srect);
			Con_DrawOneConsole(w, con_curwindow == w && Key_Dest_Has(kdm_console|kdm_cwindows) == kdm_cwindows, font_console, w->wnd_x+8, w->wnd_y, w->wnd_w-16, w->wnd_h-8, fadetime);
			if (R2D_Flush)
				R2D_Flush();
			BE_Scissor(NULL);
		}

		if (w->selstartline)
			mouseconsole = w;
		if (!con_curwindow)
			con_curwindow = w;
	}

	//draw main console...
	if (lines > 0 && con_current && !(con_current->flags & CONF_ISWINDOW))
	{
		int top;
#ifdef QTERM
		if (qterms)
			QT_Update();
#endif

// draw the background
		if (!noback)
			R2D_ConsoleBackground (0, lines, scr_con_forcedraw);

		con_current->unseentext = false;

		con_current->vislines = lines;

		top = Con_DrawAlternateConsoles(lines);

		if (!con_current->display)
			con_current->display = con_current->current;

		x = 8;
		y = lines;

		con_current->mousecursor[0] = mousecursor_x;
		con_current->mousecursor[1] = mousecursor_y;
		if (!(con_current->flags & CONF_KEEPSELECTION))
		{
			con_current->selstartline = NULL;
			con_current->selendline = NULL;
		}
		selactive = Key_GetConsoleSelectionBox(con_current, &selsx, &selsy, &selex, &seley);

		if ((con_current->flags & CONF_KEEPSELECTION) && con_current->selstartline && con_current->selendline && con_current->buttonsdown != CB_SELECTED && con_current->buttonsdown != CB_TAPPED)
			selactive = -1;

		Font_BeginString(font_console, x, y, &x, &y);
		Font_BeginString(font_console, selsx, selsy, &selsx, &selsy);
		Font_BeginString(font_console, selex, seley, &selex, &seley);
		ex = Font_ScreenWidth();
		sx = x;
		ex -= sx;

		y -= Font_CharHeight();
		haveprogress = Con_DrawProgress(x, ex - x, y) != y;
		y = Con_DrawInput (con_current, Key_Dest_Has(kdm_console), x, ex - x, y, selactive, selsx, selex, selsy, seley);

		l = con_current->display;

		y = Con_DrawConsoleLines(con_current, l, con_current->displayscroll, sx, ex, y, top, selactive, selsx, selex, selsy, seley, 0);

		if (!haveprogress && lines == vid.height)
		{
			char *version = version_string();
			int i;
			Font_BeginString(font_console, vid.width, lines, &x, &y);
			y -= Font_CharHeight();
			//assumption: version == ascii
			for (i = 0; version[i]; i++)
				x -= Font_CharWidth(CON_WHITEMASK|CON_HALFALPHA, version[i]);
			for (i = 0; version[i]; i++)
				x = Font_DrawChar(x, y, CON_WHITEMASK|CON_HALFALPHA, version[i]);
		}

		Font_EndString(font_console);
		mouseconsole = con_mouseover?con_mouseover:con_current;


		if (con_current->buttonsdown == CB_SELECTED || con_current->buttonsdown == CB_TAPPED)
		{	//select was released...
			console_t *con = con_current;
			char *buffer;
			qboolean tapped = con->buttonsdown==CB_TAPPED;
			con->buttonsdown = CB_NONE;
			if (con->selstartline)
			{
				if (tapped)
					con->flags &= ~CONF_KEEPSELECTION;
				else
					con->flags |= CONF_KEEPSELECTION;
				if (con->userline)
				{
					if (con->flags & CONF_BACKSELECTION)
					{
						con->userline = con->selendline;
						con->useroffset = con->selendoffset;
					}
					else
					{
						con->userline = con->selstartline;
						con->useroffset = con->selstartoffset;
					}
				}
				if (con->selstartline == con->selendline && con->selendoffset <= con->selstartoffset+1)
				{
					if (keydown[K_LSHIFT] || keydown[K_RSHIFT])
						;
					else
					{
						buffer = Con_CopyConsole(con, false, true, false);
						if (buffer)
						{
							Key_HandleConsoleLink(con, buffer);
							Z_Free(buffer);
						}
					}
				}
				else
				{
					buffer = Con_CopyConsole(con, true, false, true);	//don't keep markup if we're copying to the clipboard
					if (buffer)
					{
						Sys_SaveClipboard(CBT_SELECTION,  buffer);
						Z_Free(buffer);
					}
				}
			}
		}
	}
	else
		mouseconsole = con_mouseover?con_mouseover:NULL;

	if (mouseconsole && mouseconsole->selstartline)
		Con_DrawMouseOver(mouseconsole);
}

void Con_DrawOneConsole(console_t *con, qboolean focused, struct font_s *font, float fx, float fy, float fsx, float fsy, float lineagelimit)
{
	int selactive, selsx, selsy, selex, seley;
	int x, y, sx, sy;
	Font_BeginString(font, fx, fy, &x, &y);
	Font_BeginString(font, fx+fsx, fy+fsy, &sx, &sy);

	if (con == con_current && Key_Dest_Has(kdm_console))
	{
		selactive = false;	//don't change selections if this is the main console and we're looking at the console, because that main console has focus instead anyway.
		selsx = selsy = selex = seley = 0;
	}
	else
	{
		selactive = Key_GetConsoleSelectionBox(con, &selsx, &selsy, &selex, &seley);
		if ((con->flags & CONF_KEEPSELECTION) && con->selstartline && con->selendline)
		{
			selactive = -1;
			selsx = selsy = selex = seley = 0;
		}
		else
		{
			con->selstartline = NULL;
			con->selendline = NULL;
			Font_BeginString(font, selsx, selsy, &selsx, &selsy);
			Font_BeginString(font, selex, seley, &selex, &seley);
			selsx += x;
			selsy += y;
			selex += x;
			seley += y;
		}
	}

	R2D_ImageColours(1, 1, 1, 1);
	sy = Con_DrawInput (con, focused, x, sx, sy, selactive, selsx, selex, selsy, seley);

	sx -= con->displayoffset;
	selsx -= con->displayoffset;
	selex -= con->displayoffset;

	if (!con->display)
		con->display = con->current;
	Con_DrawConsoleLines(con, con->display, con->displayscroll, x, sx, sy, y, selactive, selsx, selex, selsy, seley, lineagelimit);


	if (con->buttonsdown == CB_SELECTED || con->buttonsdown == CB_TAPPED)
	{	//select was released...
		char *buffer;
		qboolean tapped = con->buttonsdown==CB_TAPPED;
		con->buttonsdown = CB_NONE;
		if (con->selstartline)
		{
			if (tapped)
				con->flags &= ~CONF_KEEPSELECTION;
			else
				con->flags |= CONF_KEEPSELECTION;
			if (con->userline)
			{
				if (con->flags & CONF_BACKSELECTION)
				{
					con->userline = con->selendline;
					con->useroffset = con->selendoffset;
				}
				else
				{
					con->userline = con->selstartline;
					con->useroffset = con->selstartoffset;
				}
			}
			if (con->selstartline == con->selendline && con->selendoffset <= con->selstartoffset+1)
			{
				if (keydown[K_LSHIFT] || keydown[K_RSHIFT])
					;
				else
				{
					buffer = Con_CopyConsole(con, false, true, false);
					if (buffer)
					{
						Key_HandleConsoleLink(con, buffer);
						Z_Free(buffer);
					}
				}
			}
			else
			{
				buffer = Con_CopyConsole(con, true, false, true);	//don't keep markup if we're copying to the clipboard
				if (buffer)
				{
					Sys_SaveClipboard(CBT_SELECTION,  buffer);
					Z_Free(buffer);
				}
			}
		}
	}

	Font_EndString(font);
}

//false=don't walk over it.
//true=fine and dandy
//2=ignore only if at the end.
static qbyte Con_IsTokenChar(unsigned int chr)
{
	if (chr >= 0x80)	//unicode chars are all continuation
		return true;
	if (chr == '(' || chr == ')' || chr == '{' || chr == '}')
		return false;
	if (chr == '/' || chr == '\\')
		return 2;	//on left only
	if (chr == '.' || chr == ':')
		return 3;	//disallow only if followed by whitespace
	if (chr >= 'a' && chr <= 'z')
		return true;
	if (chr >= 'A' && chr <= 'Z')
		return true;
	if (chr >= '0' && chr <= '9')
		return true;
	if (chr == '[' || chr == ']' || chr == '_')
		return true;
	return false;
}
void Con_ExpandConsoleSelection(console_t *con)
{
	conchar_t *cur, *n;
	conline_t *l;
	conchar_t *lstart;
	conchar_t *lend;
	unsigned int cf, uc;

	//no selection to expand...
	if (!con->selstartline || !con->selendline)
		return;

	l = con->selstartline;
	lstart = (conchar_t*)(l+1);
	cur = lstart + con->selstartoffset;

	if (con->selstartline == con->selendline)
	{
		if (con->selstartoffset+1 == con->selendoffset)
		{
			//they only selected a single char?
			//fix that up to select the entire token
			while (cur > lstart)
			{
				cur--;
				uc = (*cur & CON_CHARMASK);
				if (!Con_IsTokenChar(uc))
				{
					cur++;
					break;
				}
				if (*cur == CON_LINKSTART)
					break;
			}
			for (n = lstart+con->selendoffset; con->selendoffset < l->length; )
			{
				n = Font_Decode(n, &cf, &uc);
				if (Con_IsTokenChar(uc)==3)
					continue;

				if (Con_IsTokenChar(uc)==1 && lstart[con->selendoffset] != CON_LINKEND)
					con->selendoffset = n-lstart;
				else
					break;
			}
			/*while (con->selendoffset > l->length)
			{
				uc = (((conchar_t*)(l+1))[con->selendoffset] & CON_CHARMASK);
				if (Con_IsTokenChar(uc) == 2)
					con->selendoffset--;
				else
					break;
			}*/
		}
	}

	//scan backwards to find any link enclosure
	for(lend = cur-1; lend >= (conchar_t*)(l+1); lend--)
	{
		if (*lend == CON_LINKSTART)
		{
			//found one
			cur = lend;
			break;
		}
		if (*lend == CON_LINKEND)
		{
			//some other link ended here. don't use its start.
			break;
		}
	}
	//scan forwards to find the end of the selected link
	if (l->length && cur < (conchar_t*)(l+1)+l->length && *cur == CON_LINKSTART)
	{
		for(lend = (conchar_t*)(con->selendline+1) + con->selendoffset; lend < (conchar_t*)(con->selendline+1) + con->selendline->length; lend++)
		{
			if (*lend == CON_LINKEND)
			{
				con->selendoffset = lend+1 - (conchar_t*)(con->selendline+1);
				break;
			}
		}
	}

	con->selstartoffset = cur-(conchar_t*)(l+1);
}
char *Con_CopyConsole(console_t *con, qboolean nomarkup, qboolean onlyiflink, qboolean forceutf8)
{
	conchar_t *cur;
	conline_t *l;
	conchar_t *lend;
	char *result;
	int outlen, maxlen;
	int finalendoffset;
	unsigned int uc;

	if (!con->selstartline || !con->selendline)
		return NULL;

//	for (cur = (conchar_t*)(selstartline+1), finalendoffset = 0; cur < (conchar_t*)(selstartline+1) + selstartline->length; cur++, finalendoffset++)
//		result[finalendoffset] = *cur & 0xffff;

	l = con->selstartline;
	cur = (conchar_t*)(l+1) + con->selstartoffset;
	finalendoffset = con->selendoffset;

	if (con->selstartline == con->selendline)
	{
		if (con->selstartoffset+1 == finalendoffset)
		{
			//they only selected a single char?
			//fix that up to select the entire token
			while (cur > (conchar_t*)(l+1))
			{
				cur--;
				uc = (*cur & CON_CHARMASK);
				if (!Con_IsTokenChar(uc))
				{
					cur++;
					break;
				}
				if (*cur == CON_LINKSTART)
					break;
			}
			while (finalendoffset < l->length)
			{
				uc = (((conchar_t*)(l+1))[finalendoffset] & CON_CHARMASK);
				if (Con_IsTokenChar(uc)==1 && ((conchar_t*)(l+1))[finalendoffset] != CON_LINKEND)
					finalendoffset++;
				else
					break;
			}
			/*while (finalendoffset > l->length)
			{
				uc = (((conchar_t*)(l+1))[finalendoffset] & CON_CHARMASK);
				if (Con_IsTokenChar(uc) == 2)
					finalendoffset--;
				else
					break;
			}*/
		}
	}

	//scan backwards to find any link enclosure
	for(lend = cur-1; lend >= (conchar_t*)(l+1); lend--)
	{
		if (*lend == CON_LINKSTART)
		{
			//found one
			cur = lend;
			break;
		}
		if (*lend == CON_LINKEND)
		{
			//some other link ended here. don't use its start.
			break;
		}
	}
	//scan forwards to find the end of the selected link
	if (l->length && cur < (conchar_t*)(l+1)+l->length && *cur == CON_LINKSTART)
	{
		for(lend = (conchar_t*)(con->selendline+1) + finalendoffset; lend < (conchar_t*)(con->selendline+1) + con->selendline->length; lend++)
		{
			if (*lend == CON_LINKEND)
			{
				finalendoffset = lend+1 - (conchar_t*)(con->selendline+1);
				break;
			}
		}
	}
	else if (onlyiflink)
		return NULL;

	maxlen = 1024*1024;
	result = Z_Malloc(maxlen+1);

	outlen = 0;
	for (;;)
	{
		if (l == con->selendline)
			lend = (conchar_t*)(l+1) + finalendoffset;
		else
			lend = (conchar_t*)(l+1) + l->length;

		outlen = COM_DeFunString(cur, lend, result + outlen, maxlen - outlen, nomarkup, forceutf8||!!(con->parseflags & PFS_FORCEUTF8)) - result;

		if (l == con->selendline)
			break;

		l = l->newer;
		if (!l)
		{
			Con_Printf("Error: Bad console buffer\n");
			break;
		}

		if (outlen+3 > maxlen)
			break;
//#ifdef _WIN32
//		result[outlen++] = '\r';
//#endif
		result[outlen++] = '\n';
		cur = (conchar_t*)(l+1);
	}
	result[outlen++] = 0;

	return result;
}
