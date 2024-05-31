/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2020 DarkPlaces contributors

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

#if !defined(WIN32) || defined(__MINGW32__)
# include <unistd.h>
#endif
#include <time.h>

#include "quakedef.h"
#include "thread.h"

// for u8_encodech
#include "ft2.h"

float con_cursorspeed = 4;

// lines up from bottom to display
int con_backscroll;

conbuffer_t con;
void *con_mutex = NULL;

#define CON_LINES(i) CONBUFFER_LINES(&con, i)
#define CON_LINES_LAST CONBUFFER_LINES_LAST(&con)
#define CON_LINES_COUNT CONBUFFER_LINES_COUNT(&con)

cvar_t con_notifytime = {CF_CLIENT | CF_ARCHIVE, "con_notifytime","3", "how long notify lines last, in seconds"};
cvar_t con_notify = {CF_CLIENT | CF_ARCHIVE, "con_notify","4", "how many notify lines to show"};
cvar_t con_notifyalign = {CF_CLIENT | CF_ARCHIVE, "con_notifyalign", "", "how to align notify lines: 0 = left, 0.5 = center, 1 = right, empty string = game default)"};

cvar_t con_chattime = {CF_CLIENT | CF_ARCHIVE, "con_chattime","30", "how long chat lines last, in seconds"};
cvar_t con_chat = {CF_CLIENT | CF_ARCHIVE, "con_chat","0", "how many chat lines to show in a dedicated chat area"};
cvar_t con_chatpos = {CF_CLIENT | CF_ARCHIVE, "con_chatpos","0", "where to put chat (negative: lines from bottom of screen, positive: lines below notify, 0: at top)"};
cvar_t con_chatrect = {CF_CLIENT | CF_ARCHIVE, "con_chatrect","0", "use con_chatrect_x and _y to position con_notify and con_chat freely instead of con_chatpos"};
cvar_t con_chatrect_x = {CF_CLIENT | CF_ARCHIVE, "con_chatrect_x","", "where to put chat, relative x coordinate of left edge on screen (use con_chatwidth for width)"};
cvar_t con_chatrect_y = {CF_CLIENT | CF_ARCHIVE, "con_chatrect_y","", "where to put chat, relative y coordinate of top edge on screen (use con_chat for line count)"};
cvar_t con_chatwidth = {CF_CLIENT | CF_ARCHIVE, "con_chatwidth","1.0", "relative chat window width"};
cvar_t con_textsize = {CF_CLIENT | CF_ARCHIVE, "con_textsize","8", "console text size in virtual 2D pixels"};
cvar_t con_notifysize = {CF_CLIENT | CF_ARCHIVE, "con_notifysize","8", "notify text size in virtual 2D pixels"};
cvar_t con_chatsize = {CF_CLIENT | CF_ARCHIVE, "con_chatsize","8", "chat text size in virtual 2D pixels (if con_chat is enabled)"};
cvar_t con_chatsound = {CF_CLIENT | CF_ARCHIVE, "con_chatsound","1", "enables chat sound to play on message"};
cvar_t con_chatsound_file = {CF_CLIENT, "con_chatsound_file","sound/misc/talk.wav", "The sound to play for chat messages"};
cvar_t con_chatsound_team_file = {CF_CLIENT, "con_chatsound_team_file","sound/misc/talk2.wav", "The sound to play for team chat messages"};
cvar_t con_chatsound_team_mask = {CF_CLIENT, "con_chatsound_team_mask","40","Magic ASCII code that denotes a team chat message"};

cvar_t sys_specialcharactertranslation = {CF_CLIENT | CF_SERVER, "sys_specialcharactertranslation", "1", "terminal console conchars to ASCII translation (set to 0 if your conchars.tga is for an 8bit character set or if you want raw output)"};
#ifdef WIN32
cvar_t sys_colortranslation = {CF_CLIENT | CF_SERVER, "sys_colortranslation", "0", "terminal console color translation (supported values: 0 = strip color codes, 1 = translate to ANSI codes, 2 = no translation)"};
#else
cvar_t sys_colortranslation = {CF_CLIENT | CF_SERVER, "sys_colortranslation", "1", "terminal console color translation (supported values: 0 = strip color codes, 1 = translate to ANSI codes, 2 = no translation)"};
#endif


cvar_t con_nickcompletion = {CF_CLIENT | CF_ARCHIVE, "con_nickcompletion", "1", "tab-complete nicks in console and message input"};
cvar_t con_nickcompletion_flags = {CF_CLIENT | CF_ARCHIVE, "con_nickcompletion_flags", "11", "Bitfield: "
				   "0: add nothing after completion. "
				   "1: add the last color after completion. "
				   "2: add a quote when starting a quote instead of the color. "
				   "4: will replace 1, will force color, even after a quote. "
				   "8: ignore non-alphanumerics. "
				   "16: ignore spaces. "};
#define NICKS_ADD_COLOR 1
#define NICKS_ADD_QUOTE 2
#define NICKS_FORCE_COLOR 4
#define NICKS_ALPHANUMERICS_ONLY 8
#define NICKS_NO_SPACES 16

cvar_t con_completion_playdemo = {CF_CLIENT | CF_ARCHIVE, "con_completion_playdemo", "*.dem", "completion pattern for the playdemo command"};
cvar_t con_completion_timedemo = {CF_CLIENT | CF_ARCHIVE, "con_completion_timedemo", "*.dem", "completion pattern for the timedemo command"};
cvar_t con_completion_exec = {CF_CLIENT | CF_ARCHIVE, "con_completion_exec", "*.cfg", "completion pattern for the exec command"};

cvar_t condump_stripcolors = {CF_CLIENT | CF_SERVER| CF_ARCHIVE, "condump_stripcolors", "0", "strip color codes from console dumps"};

cvar_t rcon_password = {CF_CLIENT | CF_SERVER | CF_PRIVATE, "rcon_password", "", "password to authenticate rcon commands; NOTE: changing rcon_secure clears rcon_password, so set rcon_secure always before rcon_password; may be set to a string of the form user1:pass1 user2:pass2 user3:pass3 to allow multiple user accounts - the client then has to specify ONE of these combinations"};
cvar_t rcon_secure = {CF_CLIENT | CF_SERVER, "rcon_secure", "0", "force secure rcon authentication (1 = time based, 2 = challenge based); NOTE: changing rcon_secure clears rcon_password, so set rcon_secure always before rcon_password"};
cvar_t rcon_secure_challengetimeout = {CF_CLIENT, "rcon_secure_challengetimeout", "5", "challenge-based secure rcon: time out requests if no challenge came within this time interval"};
cvar_t rcon_address = {CF_CLIENT, "rcon_address", "", "server address to send rcon commands to (when not connected to a server)"};

int con_linewidth;
int con_vislines;

qbool con_initialized;

// used for server replies to rcon command
lhnetsocket_t *rcon_redirect_sock = NULL;
lhnetaddress_t *rcon_redirect_dest = NULL;
int rcon_redirect_bufferpos = 0;
char rcon_redirect_buffer[1400];
qbool rcon_redirect_proquakeprotocol = false;

// generic functions for console buffers

void ConBuffer_Init(conbuffer_t *buf, int textsize, int maxlines, mempool_t *mempool)
{
	buf->active = true;
	buf->textsize = textsize;
	buf->text = (char *) Mem_Alloc(mempool, textsize);
	buf->maxlines = maxlines;
	buf->lines = (con_lineinfo_t *) Mem_Alloc(mempool, maxlines * sizeof(*buf->lines));
	buf->lines_first = 0;
	buf->lines_count = 0;
}

/*! The translation table between the graphical font and plain ASCII  --KB */
static char qfont_table[256] = {
	'\0', '#',  '#',  '#',  '#',  '.',  '#',  '#',
	'#',  9,    10,   '#',  ' ',  13,   '.',  '.',
	'[',  ']',  '0',  '1',  '2',  '3',  '4',  '5',
	'6',  '7',  '8',  '9',  '.',  '<',  '=',  '>',
	' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  '[',  '\\', ']',  '^',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  '<',

	'<',  '=',  '>',  '#',  '#',  '.',  '#',  '#',
	'#',  '#',  ' ',  '#',  ' ',  '>',  '.',  '.',
	'[',  ']',  '0',  '1',  '2',  '3',  '4',  '5',
	'6',  '7',  '8',  '9',  '.',  '<',  '=',  '>',
	' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  '[',  '\\', ']',  '^',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  '<'
};

/*
	SanitizeString strips color tags from the string in
	and writes the result on string out
*/
static void SanitizeString(char *in, char *out)
{
	while(*in)
	{
		if(*in == STRING_COLOR_TAG)
		{
			++in;
			if(!*in)
			{
				out[0] = STRING_COLOR_TAG;
				out[1] = 0;
				return;
			}
			else if (*in >= '0' && *in <= '9') // ^[0-9] found
			{
				++in;
				if(!*in)
				{
					*out = 0;
					return;
				} else if (*in == STRING_COLOR_TAG) // ^[0-9]^ found, don't print ^[0-9]
					continue;
			}
			else if (*in == STRING_COLOR_RGB_TAG_CHAR) // ^x found
			{
				if ( isxdigit(in[1]) && isxdigit(in[2]) && isxdigit(in[3]) )
				{
					in+=4;
					if (!*in)
					{
						*out = 0;
						return;
					} else if (*in == STRING_COLOR_TAG) // ^xrgb^ found, don't print ^xrgb
						continue;
				}
				else in--;
			}
			else if (*in != STRING_COLOR_TAG)
				--in;
		}
		*out = qfont_table[*(unsigned char*)in];
		++in;
		++out;
	}
	*out = 0;
}

/*
================
ConBuffer_Clear
================
*/
void ConBuffer_Clear (conbuffer_t *buf)
{
	buf->lines_count = 0;
}

/*
================
ConBuffer_Shutdown
================
*/
void ConBuffer_Shutdown(conbuffer_t *buf)
{
	buf->active = false;
	if (buf->text)
		Mem_Free(buf->text);
	if (buf->lines)
		Mem_Free(buf->lines);
	buf->text = NULL;
	buf->lines = NULL;
}

/*
================
ConBuffer_FixTimes

Notifies the console code about the current time
(and shifts back times of other entries when the time
went backwards)
================
*/
void ConBuffer_FixTimes(conbuffer_t *buf)
{
	int i;
	if(buf->lines_count >= 1)
	{
		double diff = cl.time - CONBUFFER_LINES_LAST(buf).addtime;
		if(diff < 0)
		{
			for(i = 0; i < buf->lines_count; ++i)
				CONBUFFER_LINES(buf, i).addtime += diff;
		}
	}
}

/*
================
ConBuffer_DeleteLine

Deletes the first line from the console history.
================
*/
void ConBuffer_DeleteLine(conbuffer_t *buf)
{
	if(buf->lines_count == 0)
		return;
	--buf->lines_count;
	buf->lines_first = (buf->lines_first + 1) % buf->maxlines;
}

/*
================
ConBuffer_DeleteLastLine

Deletes the last line from the console history.
================
*/
void ConBuffer_DeleteLastLine(conbuffer_t *buf)
{
	if(buf->lines_count == 0)
		return;
	--buf->lines_count;
}

/*
================
ConBuffer_BytesLeft

Checks if there is space for a line of the given length, and if yes, returns a
pointer to the start of such a space, and NULL otherwise.
================
*/
static char *ConBuffer_BytesLeft(conbuffer_t *buf, int len)
{
	if(len > buf->textsize)
		return NULL;
	if(buf->lines_count == 0)
		return buf->text;
	else
	{
		char *firstline_start = buf->lines[buf->lines_first].start;
		char *lastline_onepastend = CONBUFFER_LINES_LAST(buf).start + CONBUFFER_LINES_LAST(buf).len;
		// the buffer is cyclic, so we first have two cases...
		if(firstline_start < lastline_onepastend) // buffer is contiguous
		{
			// put at end?
			if(len <= buf->text + buf->textsize - lastline_onepastend)
				return lastline_onepastend;
			// put at beginning?
			else if(len <= firstline_start - buf->text)
				return buf->text;
			else
				return NULL;
		}
		else // buffer has a contiguous hole
		{
			if(len <= firstline_start - lastline_onepastend)
				return lastline_onepastend;
			else
				return NULL;
		}
	}
}

/*
================
ConBuffer_AddLine

Appends a given string as a new line to the console.
================
*/
void ConBuffer_AddLine(conbuffer_t *buf, const char *line, int len, unsigned mask)
{
	char *putpos;
	con_lineinfo_t *p;

	// developer_memory 1 during shutdown prints while conbuffer_t is being freed
	if (!buf->active)
		return;

	ConBuffer_FixTimes(buf);

	if(len >= buf->textsize)
	{
		// line too large?
		// only display end of line.
		line += len - buf->textsize + 1;
		len = buf->textsize - 1;
	}
	while(!(putpos = ConBuffer_BytesLeft(buf, len + 1)) || buf->lines_count >= buf->maxlines)
		ConBuffer_DeleteLine(buf);
	memcpy(putpos, line, len);
	putpos[len] = 0;
	++buf->lines_count;

	//fprintf(stderr, "Now have %d lines (%d -> %d).\n", buf->lines_count, buf->lines_first, CON_LINES_LAST);

	p = &CONBUFFER_LINES_LAST(buf);
	p->start = putpos;
	p->len = len;
	p->addtime = cl.time;
	p->mask = mask;
	p->height = -1; // calculate when needed
}

int ConBuffer_FindPrevLine(conbuffer_t *buf, unsigned mask_must, unsigned mask_mustnot, int start)
{
	int i;
	if(start == -1)
		start = buf->lines_count;
	for(i = start - 1; i >= 0; --i)
	{
		con_lineinfo_t *l = &CONBUFFER_LINES(buf, i);

		if((l->mask & mask_must) != mask_must)
			continue;
		if(l->mask & mask_mustnot)
			continue;

		return i;
	}

	return -1;
}

const char *ConBuffer_GetLine(conbuffer_t *buf, int i)
{
	static char copybuf[MAX_INPUTLINE]; // client only
	con_lineinfo_t *l = &CONBUFFER_LINES(buf, i);

	dp_ustr2stp(copybuf, sizeof(copybuf), l->start, l->len);
	return copybuf;
}

/*
==============================================================================

LOGGING

==============================================================================
*/

/// \name Logging
//@{
cvar_t log_file = {CF_CLIENT | CF_SERVER, "log_file", "", "filename to log messages to"};
cvar_t log_file_stripcolors = {CF_CLIENT | CF_SERVER, "log_file_stripcolors", "0", "strip color codes from log messages"};
cvar_t log_dest_udp = {CF_CLIENT | CF_SERVER, "log_dest_udp", "", "UDP address to log messages to (in QW rcon compatible format); multiple destinations can be separated by spaces; DO NOT SPECIFY DNS NAMES HERE"};
char log_dest_buffer[1400]; // UDP packet
size_t log_dest_buffer_pos;
unsigned int log_dest_buffer_appending;
char crt_log_file [MAX_OSPATH] = "";
qfile_t* logfile = NULL;

unsigned char* logqueue = NULL;
size_t logq_ind = 0;
size_t logq_size = 0;

void Log_ConPrint (const char *msg);
//@}
static void Log_DestBuffer_Init(void)
{
	memcpy(log_dest_buffer, "\377\377\377\377n", 5); // QW rcon print
	log_dest_buffer_pos = 5;
}

static void Log_DestBuffer_Flush_NoLock(void)
{
	lhnetaddress_t log_dest_addr;
	lhnetsocket_t *log_dest_socket;
	const char *s = log_dest_udp.string;
	qbool have_opened_temp_sockets = false;
	if(s) if(log_dest_buffer_pos > 5)
	{
		++log_dest_buffer_appending;
		log_dest_buffer[log_dest_buffer_pos++] = 0;

		if(!NetConn_HaveServerPorts() && !NetConn_HaveClientPorts()) // then temporarily open one
 		{
			have_opened_temp_sockets = true;
			NetConn_OpenServerPorts(true);
		}

		while(COM_ParseToken_Console(&s))
			if(LHNETADDRESS_FromString(&log_dest_addr, com_token, 26000))
			{
				log_dest_socket = NetConn_ChooseClientSocketForAddress(&log_dest_addr);
				if(!log_dest_socket)
					log_dest_socket = NetConn_ChooseServerSocketForAddress(&log_dest_addr);
				if(log_dest_socket)
					NetConn_WriteString(log_dest_socket, log_dest_buffer, &log_dest_addr);
			}

		if(have_opened_temp_sockets)
			NetConn_CloseServerPorts();
		--log_dest_buffer_appending;
	}
	log_dest_buffer_pos = 0;
}

/*
====================
Log_DestBuffer_Flush
====================
*/
void Log_DestBuffer_Flush(void)
{
	if (con_mutex)
		Thread_LockMutex(con_mutex);
	Log_DestBuffer_Flush_NoLock();
	if (con_mutex)
		Thread_UnlockMutex(con_mutex);
}

static const char* Log_Timestamp (const char *desc)
{
	static char timestamp [128]; // init/shutdown only
	time_t crt_time;
#if _MSC_VER >= 1400
	struct tm crt_tm;
#else
	struct tm *crt_tm;
#endif
	char timestring [64];

	// Build the time stamp (ex: "Wed Jun 30 21:49:08 1993");
	time (&crt_time);
#if _MSC_VER >= 1400
	localtime_s (&crt_tm, &crt_time);
	strftime (timestring, sizeof (timestring), "%a %b %d %H:%M:%S %Y", &crt_tm);
#else
	crt_tm = localtime (&crt_time);
	strftime (timestring, sizeof (timestring), "%a %b %d %H:%M:%S %Y", crt_tm);
#endif

	if (desc != NULL)
		dpsnprintf (timestamp, sizeof (timestamp), "====== %s (%s) ======\n", desc, timestring);
	else
		dpsnprintf (timestamp, sizeof (timestamp), "====== %s ======\n", timestring);

	return timestamp;
}

static void Log_Open (void)
{
	if (logfile != NULL || log_file.string[0] == '\0')
		return;

	logfile = FS_OpenRealFile(log_file.string, "a", false);
	if (logfile != NULL)
	{
		dp_strlcpy (crt_log_file, log_file.string, sizeof (crt_log_file));
		FS_Print (logfile, Log_Timestamp ("Log started"));
	}
}

/*
====================
Log_Close
====================
*/
void Log_Close (void)
{
	qfile_t* l = logfile;

	if (l == NULL)
		return;

	FS_Print (l, Log_Timestamp ("Log stopped"));
	FS_Print (l, "\n");
	logfile = NULL;
	FS_Close (l);

	crt_log_file[0] = '\0';
}


/*
====================
Log_Start
====================
*/
void Log_Start (void)
{
	size_t pos;
	size_t n;
	Log_Open ();

	// Dump the contents of the log queue into the log file and free it
	if (logqueue != NULL)
	{
		unsigned char *temp = logqueue;
		logqueue = NULL;
		if(logq_ind != 0)
		{
			if (logfile != NULL)
				FS_Write (logfile, temp, logq_ind);
			if(*log_dest_udp.string)
			{
				for(pos = 0; pos < logq_ind; )
				{
					if(log_dest_buffer_pos == 0)
						Log_DestBuffer_Init();
					n = min(sizeof(log_dest_buffer) - log_dest_buffer_pos - 1, logq_ind - pos);
					memcpy(log_dest_buffer + log_dest_buffer_pos, temp + pos, n);
					log_dest_buffer_pos += n;
					Log_DestBuffer_Flush_NoLock();
					pos += n;
				}
			}
		}
		Mem_Free (temp);
		logq_ind = 0;
		logq_size = 0;
	}
}



/*
================
Log_ConPrint
================
*/
void Log_ConPrint (const char *msg)
{
	static qbool inprogress = false;

	// don't allow feedback loops with memory error reports
	if (inprogress)
		return;
	inprogress = true;

	// Until the host is completely initialized, we maintain a log queue
	// to store the messages, since the log can't be started before
	if (logqueue != NULL)
	{
		size_t remain = logq_size - logq_ind;
		size_t len = strlen (msg);

		// If we need to enlarge the log queue
		if (len > remain)
		{
			size_t factor = ((logq_ind + len) / logq_size) + 1;
			unsigned char* newqueue;

			logq_size *= factor;
			newqueue = (unsigned char *)Mem_Alloc (tempmempool, logq_size);
			memcpy (newqueue, logqueue, logq_ind);
			Mem_Free (logqueue);
			logqueue = newqueue;
			remain = logq_size - logq_ind;
		}
		memcpy (&logqueue[logq_ind], msg, len);
		logq_ind += len;

		inprogress = false;
		return;
	}

	// Check if log_file has changed
	if (strcmp (crt_log_file, log_file.string) != 0)
	{
		Log_Close ();
		Log_Open ();
	}

	// If a log file is available
	if (logfile != NULL)
	{
		if (log_file_stripcolors.integer)
		{
			// sanitize msg
			size_t len = strlen(msg);
			char* sanitizedmsg = (char*)Mem_Alloc(tempmempool, len + 1);
			memcpy (sanitizedmsg, msg, len);
			SanitizeString(sanitizedmsg, sanitizedmsg); // SanitizeString's in pointer is always ahead of the out pointer, so this should work.
			FS_Print (logfile, sanitizedmsg);
			Mem_Free(sanitizedmsg);
		}
		else 
		{
			FS_Print (logfile, msg);
		}
	}

	inprogress = false;
}


/*
================
Log_Printf
================
*/
void Log_Printf (const char *logfilename, const char *fmt, ...)
{
	qfile_t *file;

	file = FS_OpenRealFile(logfilename, "a", true);
	if (file != NULL)
	{
		va_list argptr;

		va_start (argptr, fmt);
		FS_VPrintf (file, fmt, argptr);
		va_end (argptr);

		FS_Close (file);
	}
}


/*
==============================================================================

CONSOLE

==============================================================================
*/

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(cmd_state_t *cmd)
{
	if (Sys_CheckParm ("-noconsole"))
		if (!(key_consoleactive & KEY_CONSOLEACTIVE_USER))
			return; // only allow the key bind to turn off console

	// toggle the 'user wants console' bit
	key_consoleactive ^= KEY_CONSOLEACTIVE_USER;
	Con_ClearNotify();
}

/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int i;
	for(i = 0; i < CON_LINES_COUNT; ++i)
		if(!(CON_LINES(i).mask & CON_MASK_CHAT))
			CON_LINES(i).mask |= CON_MASK_HIDENOTIFY;
}

static void Con_MsgCmdMode(cmd_state_t *cmd, signed char mode)
{
	if (cls.demoplayback && mode >= 0)
		return;
	key_dest = key_message;
	chat_mode = mode;
	if(Cmd_Argc(cmd) > 1)
	{
		chat_bufferpos = dpsnprintf(chat_buffer, sizeof(chat_buffer), "%s ", Cmd_Args(cmd));
		if (chat_bufferpos < 0)
			chat_bufferpos = 0;
	}
}

/*
================
Con_MessageMode_f

"say"
================
*/
static void Con_MessageMode_f(cmd_state_t *cmd)
{
	Con_MsgCmdMode(cmd, 0);
}

/*
================
Con_MessageMode2_f

"say_team"
================
*/
static void Con_MessageMode2_f(cmd_state_t *cmd)
{
	Con_MsgCmdMode(cmd, 1);
}

/*
================
Con_CommandMode_f
================
*/
static void Con_CommandMode_f(cmd_state_t *cmd)
{
	Con_MsgCmdMode(cmd, -1);
}

/*
================
Con_CheckResize
================
*/
void Con_CheckResize (void)
{
	int i, width;
	float f;

	f = bound(1, con_textsize.value, 128);
	if(f != con_textsize.value)
		Cvar_SetValueQuick(&con_textsize, f);
	width = (int)floor(vid_conwidth.value / con_textsize.value);
	width = bound(1, width, con.textsize/4);
		// FIXME uses con in a non abstracted way

	if (width == con_linewidth)
		return;

	con_linewidth = width;

	for(i = 0; i < CON_LINES_COUNT; ++i)
		CON_LINES(i).height = -1; // recalculate when next needed

	Con_ClearNotify();
	con_backscroll = 0;
}

//[515]: the simplest command ever
//LadyHavoc: not so simple after I made it print usage...
static void Con_Maps_f(cmd_state_t *cmd)
{
	if (Cmd_Argc(cmd) > 2)
	{
		Con_Printf("usage: maps [mapnameprefix]\n");
		return;
	}
	else if (Cmd_Argc(cmd) == 2)
		GetMapList(Cmd_Argv(cmd, 1), NULL, 0);
	else
		GetMapList("", NULL, 0);
}

static void Con_ConDump_f(cmd_state_t *cmd)
{
	int i;
	qfile_t *file;
	if (Cmd_Argc(cmd) != 2)
	{
		Con_Printf("usage: condump <filename>\n");
		return;
	}
	file = FS_OpenRealFile(Cmd_Argv(cmd, 1), "w", false);
	if (!file)
	{
		Con_Printf(CON_ERROR "condump: unable to write file \"%s\"\n", Cmd_Argv(cmd, 1));
		return;
	}
	if (con_mutex) Thread_LockMutex(con_mutex);
	for(i = 0; i < CON_LINES_COUNT; ++i)
	{
		if (condump_stripcolors.integer)
		{
			// sanitize msg
			size_t len = CON_LINES(i).len;
			char* sanitizedmsg = (char*)Mem_Alloc(tempmempool, len + 1);
			memcpy (sanitizedmsg, CON_LINES(i).start, len);
			SanitizeString(sanitizedmsg, sanitizedmsg); // SanitizeString's in pointer is always ahead of the out pointer, so this should work.
			FS_Write(file, sanitizedmsg, strlen(sanitizedmsg));
			Mem_Free(sanitizedmsg);
		}
		else 
		{
			FS_Write(file, CON_LINES(i).start, CON_LINES(i).len);
		}
		FS_Write(file, "\n", 1);
	}
	if (con_mutex) Thread_UnlockMutex(con_mutex);
	FS_Close(file);
}

void Con_Clear_f(cmd_state_t *cmd)
{
	if (con_mutex) Thread_LockMutex(con_mutex);
	ConBuffer_Clear(&con);
	if (con_mutex) Thread_UnlockMutex(con_mutex);
}

static void Con_RCon_ClearPassword_c(cvar_t *var)
{
	// whenever rcon_secure is changed to 0, clear rcon_password for
	// security reasons (prevents a send-rcon-password-as-plaintext
	// attack based on NQ protocol session takeover and svc_stufftext)
	if(var->integer <= 0)
		Cvar_SetQuick(&rcon_password, "");
}

/*
================
Con_Init
================
*/
void Con_Init (void)
{
	con_linewidth = 80;
	ConBuffer_Init(&con, CON_TEXTSIZE, CON_MAXLINES, zonemempool);
	if (Thread_HasThreads())
		con_mutex = Thread_CreateMutex();

	// Allocate a log queue, this will be freed after configs are parsed
	logq_size = MAX_INPUTLINE;
	logqueue = (unsigned char *)Mem_Alloc (tempmempool, logq_size);
	logq_ind = 0;

	Cvar_RegisterVariable (&sys_colortranslation);
	Cvar_RegisterVariable (&sys_specialcharactertranslation);

	Cvar_RegisterVariable (&log_file);
	Cvar_RegisterVariable (&log_file_stripcolors);
	Cvar_RegisterVariable (&log_dest_udp);

	// support for the classic Quake option
// COMMANDLINEOPTION: Console: -condebug logs console messages to qconsole.log, see also log_file
	if (Sys_CheckParm ("-condebug") != 0)
		Cvar_SetQuick (&log_file, "qconsole.log");

	// register our cvars
	Cvar_RegisterVariable (&con_chat);
	Cvar_RegisterVariable (&con_chatpos);
	Cvar_RegisterVariable (&con_chatrect_x);
	Cvar_RegisterVariable (&con_chatrect_y);
	Cvar_RegisterVariable (&con_chatrect);
	Cvar_RegisterVariable (&con_chatsize);
	Cvar_RegisterVariable (&con_chattime);
	Cvar_RegisterVariable (&con_chatwidth);
	Cvar_RegisterVariable (&con_notify);
	Cvar_RegisterVariable (&con_notifyalign);
	Cvar_RegisterVariable (&con_notifysize);
	Cvar_RegisterVariable (&con_notifytime);
	Cvar_RegisterVariable (&con_textsize);
	Cvar_RegisterVariable (&con_chatsound);
	Cvar_RegisterVariable (&con_chatsound_file);
	Cvar_RegisterVariable (&con_chatsound_team_file);
	Cvar_RegisterVariable (&con_chatsound_team_mask);

	// --blub
	Cvar_RegisterVariable (&con_nickcompletion);
	Cvar_RegisterVariable (&con_nickcompletion_flags);

	Cvar_RegisterVariable (&con_completion_playdemo); // *.dem
	Cvar_RegisterVariable (&con_completion_timedemo); // *.dem
	Cvar_RegisterVariable (&con_completion_exec); // *.cfg

	Cvar_RegisterVariable (&condump_stripcolors);

	Cvar_RegisterVariable(&rcon_address);
	Cvar_RegisterVariable(&rcon_secure);
	Cvar_RegisterCallback(&rcon_secure, Con_RCon_ClearPassword_c);
	Cvar_RegisterVariable(&rcon_secure_challengetimeout);
	Cvar_RegisterVariable(&rcon_password);

	// register our commands
	Cmd_AddCommand(CF_CLIENT, "toggleconsole", Con_ToggleConsole_f, "opens or closes the console");
	Cmd_AddCommand(CF_CLIENT, "messagemode", Con_MessageMode_f, "input a chat message to say to everyone");
	Cmd_AddCommand(CF_CLIENT, "messagemode2", Con_MessageMode2_f, "input a chat message to say to only your team");
	Cmd_AddCommand(CF_CLIENT, "commandmode", Con_CommandMode_f, "input a console command");
	Cmd_AddCommand(CF_SHARED, "clear", Con_Clear_f, "clear console history");
	Cmd_AddCommand(CF_SHARED, "maps", Con_Maps_f, "list information about available maps");
	Cmd_AddCommand(CF_SHARED, "condump", Con_ConDump_f, "output console history to a file (see also log_file)");

	con_initialized = true;
}

void Con_Shutdown (void)
{
	if (con_mutex) Thread_LockMutex(con_mutex);
	ConBuffer_Shutdown(&con);
	if (con_mutex) Thread_UnlockMutex(con_mutex);
	if (con_mutex) Thread_DestroyMutex(con_mutex);
	con_mutex = NULL;
}

/*
================
Con_PrintToHistory

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be displayed
If no console is visible, the notify window will pop up.
================
*/
static void Con_PrintToHistory(const char *txt, int mask)
{
	// process:
	//   \n goes to next line
	//   \r deletes current line and makes a new one

	static int cr_pending = 0;
	static char buf[CON_TEXTSIZE]; // con_mutex
	static int bufpos = 0;

	if(!con.text) // FIXME uses a non-abstracted property of con
		return;

	for(; *txt; ++txt)
	{
		if(cr_pending)
		{
			ConBuffer_DeleteLastLine(&con);
			cr_pending = 0;
		}
		switch(*txt)
		{
			case 0:
				break;
			case '\r':
				ConBuffer_AddLine(&con, buf, bufpos, mask);
				bufpos = 0;
				cr_pending = 1;
				break;
			case '\n':
				ConBuffer_AddLine(&con, buf, bufpos, mask);
				bufpos = 0;
				break;
			default:
				buf[bufpos++] = *txt;
				if(bufpos >= con.textsize - 1) // FIXME uses a non-abstracted property of con
				{
					ConBuffer_AddLine(&con, buf, bufpos, mask);
					bufpos = 0;
				}
				break;
		}
	}
}

void Con_Rcon_Redirect_Init(lhnetsocket_t *sock, lhnetaddress_t *dest, qbool proquakeprotocol)
{
	rcon_redirect_sock = sock;
	rcon_redirect_dest = dest;
	rcon_redirect_proquakeprotocol = proquakeprotocol;
	if (rcon_redirect_proquakeprotocol)
	{
		// reserve space for the packet header
		rcon_redirect_buffer[0] = 0;
		rcon_redirect_buffer[1] = 0;
		rcon_redirect_buffer[2] = 0;
		rcon_redirect_buffer[3] = 0;
		// this is a reply to a CCREQ_RCON
		rcon_redirect_buffer[4] = (unsigned char)CCREP_RCON;
	}
	else
		memcpy(rcon_redirect_buffer, "\377\377\377\377n", 5); // QW rcon print
	rcon_redirect_bufferpos = 5;
}

static void Con_Rcon_Redirect_Flush(void)
{
	if(rcon_redirect_sock)
	{
		rcon_redirect_buffer[rcon_redirect_bufferpos] = 0;
		if (rcon_redirect_proquakeprotocol)
		{
			// update the length in the packet header
			StoreBigLong((unsigned char *)rcon_redirect_buffer, NETFLAG_CTL | (rcon_redirect_bufferpos & NETFLAG_LENGTH_MASK));
		}
		NetConn_Write(rcon_redirect_sock, rcon_redirect_buffer, rcon_redirect_bufferpos, rcon_redirect_dest);
	}
	memcpy(rcon_redirect_buffer, "\377\377\377\377n", 5); // QW rcon print
	rcon_redirect_bufferpos = 5;
	rcon_redirect_proquakeprotocol = false;
}

void Con_Rcon_Redirect_End(void)
{
	Con_Rcon_Redirect_Flush();
	rcon_redirect_dest = NULL;
	rcon_redirect_sock = NULL;
}

void Con_Rcon_Redirect_Abort(void)
{
	rcon_redirect_dest = NULL;
	rcon_redirect_sock = NULL;
}

/*
================
Con_Rcon_AddChar
================
*/
/// Adds a character to the rcon buffer.
static void Con_Rcon_AddChar(int c)
{
	if(log_dest_buffer_appending)
		return;
	++log_dest_buffer_appending;

	// if this print is in response to an rcon command, add the character
	// to the rcon redirect buffer

	if (rcon_redirect_dest)
	{
		rcon_redirect_buffer[rcon_redirect_bufferpos++] = c;
		if(rcon_redirect_bufferpos >= (int)sizeof(rcon_redirect_buffer) - 1)
			Con_Rcon_Redirect_Flush();
	}
	else if(*log_dest_udp.string) // don't duplicate rcon command responses here, these are sent another way
	{
		if(log_dest_buffer_pos == 0)
			Log_DestBuffer_Init();
		log_dest_buffer[log_dest_buffer_pos++] = c;
		if(log_dest_buffer_pos >= sizeof(log_dest_buffer) - 1) // minus one, to allow for terminating zero
			Log_DestBuffer_Flush_NoLock();
	}
	else
		log_dest_buffer_pos = 0;

	--log_dest_buffer_appending;
}

/**
 * Convert an RGB color to its nearest quake color.
 * I'll cheat on this a bit by translating the colors to HSV first,
 * S and V decide if it's black or white, otherwise, H will decide the
 * actual color.
 * @param _r Red (0-255)
 * @param _g Green (0-255)
 * @param _b Blue (0-255)
 * @return A quake color character.
 */
static char Sys_Con_NearestColor(const unsigned char _r, const unsigned char _g, const unsigned char _b)
{
	float r = ((float)_r)/255.0;
	float g = ((float)_g)/255.0;
	float b = ((float)_b)/255.0;
	float min = min(r, min(g, b));
	float max = max(r, max(g, b));

	int h; ///< Hue angle [0,360]
	float s; ///< Saturation [0,1]
	float v = max; ///< In HSV v == max [0,1]

	if(max == min)
		s = 0;
	else
		s = 1.0 - (min/max);

	// Saturation threshold. We now say 0.2 is the minimum value for a color!
	if(s < 0.2)
	{
		// If the value is less than half, return a black color code.
		// Otherwise return a white one.
		if(v < 0.5)
			return '0';
		return '7';
	}

	// Let's get the hue angle to define some colors:
	if(max == min)
		h = 0;
	else if(max == r)
		h = (int)(60.0 * (g-b)/(max-min))%360;
	else if(max == g)
		h = (int)(60.0 * (b-r)/(max-min) + 120);
	else // if(max == b) redundant check
		h = (int)(60.0 * (r-g)/(max-min) + 240);

	if(h < 36) // *red* to orange
		return '1';
	else if(h < 80) // orange over *yellow* to evilish-bright-green
		return '3';
	else if(h < 150) // evilish-bright-green over *green* to ugly bright blue
		return '2';
	else if(h < 200) // ugly bright blue over *bright blue* to darkish blue
		return '5';
	else if(h < 270) // darkish blue over *dark blue* to cool purple
		return '4';
	else if(h < 330) // cool purple over *purple* to ugly swiny red
		return '6';
	else // ugly red to red closes the circly
		return '1';
}

/*
================
Con_MaskPrint
================
*/
extern cvar_t timestamps;
extern cvar_t timeformat;
void Con_MaskPrint(unsigned additionalmask, const char *msg)
{
	static unsigned mask = 0;
	static unsigned index = 0;
	static char line[MAX_INPUTLINE];

	if (con_mutex)
		Thread_LockMutex(con_mutex);

	for (;*msg;msg++)
	{
		Con_Rcon_AddChar(*msg);
		// if this is the beginning of a new line, print timestamp
		if (index == 0)
		{
			const char *timestamp = timestamps.integer ? Sys_TimeString(timeformat.string) : "";
			// reset the color
			// FIXME: 1. perhaps we should use a terminal system 2. use a constant instead of 7!
			line[index++] = STRING_COLOR_TAG;
			// assert( STRING_COLOR_DEFAULT < 10 )
			line[index++] = STRING_COLOR_DEFAULT + '0';
			// special color codes for chat messages must always come first
			// for Con_PrintToHistory to work properly
			if (*msg == 1 || *msg == 2 || *msg == 3)
			{
				// play talk wav
				if (*msg == 1)
				{
					if (con_chatsound.value)
					{
						if(msg[1] == con_chatsound_team_mask.integer && cl.foundteamchatsound)
							S_LocalSound (con_chatsound_team_file.string);
						else
							S_LocalSound (con_chatsound_file.string);
					}
				}
				// Send to chatbox for say/tell (1) and messages (3)
				// 3 is just so that a message can be sent to the chatbox without a sound.
				if (*msg == 1 || *msg == 3)
					mask = CON_MASK_CHAT;

				line[index++] = STRING_COLOR_TAG;
				line[index++] = '3';
				msg++;
				Con_Rcon_AddChar(*msg);
			}
			// store timestamp
			for (;*timestamp;index++, timestamp++)
				if (index < (int)sizeof(line) - 2)
					line[index] = *timestamp;
			// add the mask
			mask |= additionalmask;
		}
		// append the character
		line[index++] = *msg;
		// if this is a newline character, we have a complete line to print
		if (*msg == '\n' || index >= (int)sizeof(line) / 2)
		{
			// terminate the line
			line[index] = 0;
			// send to log file
			Log_ConPrint(line);
			// send to scrollable buffer
			if (con_initialized && cls.state != ca_dedicated)
			{
				Con_PrintToHistory(line, mask);
			}
			// send to terminal or dedicated server window
			if (sys.outfd >= 0)
			if (developer.integer || !(mask & CON_MASK_DEVELOPER))
			{
				if(sys_specialcharactertranslation.integer)
				{
					char *p;
					const char *q;
					p = line;
					while(*p)
					{
						int ch = u8_getchar(p, &q);
						if(ch >= 0xE000 && ch <= 0xE0FF && ((unsigned char) qfont_table[ch - 0xE000]) >= 0x20)
						{
							*p = qfont_table[ch - 0xE000];
							if(q > p+1)
								memmove(p+1, q, strlen(q)+1);
							p = p + 1;
						}
						else
							p = p + (q - p);
					}
				}

				if(sys_colortranslation.integer == 1) // ANSI
				{
					static char printline[MAX_INPUTLINE * 4 + 3];
						// 2 can become 7 bytes, rounding that up to 8, and 3 bytes are added at the end
						// a newline can transform into four bytes, but then prevents the three extra bytes from appearing
					int lastcolor = 0;
					const char *in;
					char *out;
					int color;
					for(in = line, out = printline; *in; ++in)
					{
						switch(*in)
						{
							case STRING_COLOR_TAG:
								if( in[1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(in[2]) && isxdigit(in[3]) && isxdigit(in[4]) )
								{
									char r = tolower(in[2]);
									char g = tolower(in[3]);
									char b = tolower(in[4]);
									// it's a hex digit already, so the else part needs no check --blub
									if(isdigit(r)) r -= '0';
									else r -= 87;
									if(isdigit(g)) g -= '0';
									else g -= 87;
									if(isdigit(b)) b -= '0';
									else b -= 87;
									
									color = Sys_Con_NearestColor(r * 17, g * 17, b * 17);
									in += 3; // 3 only, the switch down there does the fourth
								}
								else
									color = in[1];
								
								switch(color)
								{
									case STRING_COLOR_TAG:
										++in;
										*out++ = STRING_COLOR_TAG;
										break;
									case '0':
									case '7':
										// normal color
										++in;
										if(lastcolor == 0) break; else lastcolor = 0;
										*out++ = 0x1B; *out++ = '['; *out++ = 'm';
										break;
									case '1':
										// light red
										++in;
										if(lastcolor == 1) break; else lastcolor = 1;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '1'; *out++ = 'm';
										break;
									case '2':
										// light green
										++in;
										if(lastcolor == 2) break; else lastcolor = 2;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '2'; *out++ = 'm';
										break;
									case '3':
										// yellow
										++in;
										if(lastcolor == 3) break; else lastcolor = 3;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '3'; *out++ = 'm';
										break;
									case '4':
										// light blue
										++in;
										if(lastcolor == 4) break; else lastcolor = 4;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '4'; *out++ = 'm';
										break;
									case '5':
										// light cyan
										++in;
										if(lastcolor == 5) break; else lastcolor = 5;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '6'; *out++ = 'm';
										break;
									case '6':
										// light magenta
										++in;
										if(lastcolor == 6) break; else lastcolor = 6;
										*out++ = 0x1B; *out++ = '['; *out++ = '1'; *out++ = ';'; *out++ = '3'; *out++ = '5'; *out++ = 'm';
										break;
									// 7 handled above
									case '8':
									case '9':
										// bold normal color
										++in;
										if(lastcolor == 8) break; else lastcolor = 8;
										*out++ = 0x1B; *out++ = '['; *out++ = '0'; *out++ = ';'; *out++ = '1'; *out++ = 'm';
										break;
									default:
										*out++ = STRING_COLOR_TAG;
										break;
								}
								break;
							case '\n':
								if(lastcolor != 0)
								{
									*out++ = 0x1B; *out++ = '['; *out++ = 'm';
									lastcolor = 0;
								}
								*out++ = *in;
								break;
							default:
								*out++ = *in;
								break;
						}
					}
					if(lastcolor != 0)
					{
						*out++ = 0x1B;
						*out++ = '[';
						*out++ = 'm';
					}
					*out = '\0';
					Sys_Print(printline, out - printline);
				}
				else if(sys_colortranslation.integer == 2) // Quake
				{
					Sys_Print(line, index);
				}
				else // strip
				{
					static char printline[MAX_INPUTLINE]; // it can only get shorter here
					const char *in;
					char *out;
					for(in = line, out = printline; *in; ++in)
					{
						switch(*in)
						{
							case STRING_COLOR_TAG:
								switch(in[1])
								{
									case STRING_COLOR_RGB_TAG_CHAR:
										if ( isxdigit(in[2]) && isxdigit(in[3]) && isxdigit(in[4]) )
										{
											in+=4;
											break;
										}
										*out++ = STRING_COLOR_TAG;
										*out++ = STRING_COLOR_RGB_TAG_CHAR;
										++in;
										break;
									case STRING_COLOR_TAG:
										++in;
										*out++ = STRING_COLOR_TAG;
										break;
									case '0':
									case '1':
									case '2':
									case '3':
									case '4':
									case '5':
									case '6':
									case '7':
									case '8':
									case '9':
										++in;
										break;
									default:
										*out++ = STRING_COLOR_TAG;
										break;
								}
								break;
							default:
								*out++ = *in;
								break;
						}
					}
					*out = '\0';
					Sys_Print(printline, out - printline);
				}
			}
			// empty the line buffer
			index = 0;
			mask = 0;
		}
	}

	if (con_mutex)
		Thread_UnlockMutex(con_mutex);
}

/*
================
Con_MaskPrintf
================
*/
void Con_MaskPrintf(unsigned mask, const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE];

	va_start(argptr,fmt);
	dpvsnprintf(msg,sizeof(msg),fmt,argptr);
	va_end(argptr);

	Con_MaskPrint(mask, msg);
}

/*
================
Con_Print
================
*/
void Con_Print(const char *msg)
{
	Con_MaskPrint(CON_MASK_PRINT, msg);
}

/*
================
Con_Printf
================
*/
void Con_Printf(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE];

	va_start(argptr,fmt);
	dpvsnprintf(msg,sizeof(msg),fmt,argptr);
	va_end(argptr);

	Con_MaskPrint(CON_MASK_PRINT, msg);
}

/*
================
Con_DPrint
================
*/
void Con_DPrint(const char *msg)
{
	if(developer.integer < 0) // at 0, we still add to the buffer but hide
		return;

	Con_MaskPrint(CON_MASK_DEVELOPER, msg);
}

/*
================
Con_DPrintf
================
*/
void Con_DPrintf(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE];

	if(developer.integer < 0) // at 0, we still add to the buffer but hide
		return;

	va_start(argptr,fmt);
	dpvsnprintf(msg,sizeof(msg),fmt,argptr);
	va_end(argptr);

	Con_MaskPrint(CON_MASK_DEVELOPER, msg);
}


/*
==============================================================================

DRAWING

==============================================================================
*/

/*
================
Con_DrawInput

It draws either the console input line or the chat input line (if is_console is false)
The input line scrolls horizontally if typing goes beyond the right edge

Modified by EvilTypeGuy eviltypeguy@qeradiant.com
================
*/
static void Con_DrawInput(qbool is_console, float x, float v, float inputsize)
{
	int y, i, col_out, linepos, text_start, prefix_start = 0;
	char text[MAX_INPUTLINE + 5 + 9 + 1]; // space for ^xRGB, "say_team:" and \0
	float xo;
	size_t len_out;
	const char *prefix;
	dp_font_t *fnt;

	if (is_console && !key_consoleactive)
		return;		// don't draw anything

	if (is_console)
	{
		// empty prefix because ] is part of the console edit line
		prefix = "";
		dp_strlcpy(text, key_line, sizeof(text));
		linepos = key_linepos;
		fnt = FONT_CONSOLE;
	}
	else
	{
		if (chat_mode < 0)
			prefix = "]";
		else if(chat_mode)
			prefix = "say_team:";
		else
			prefix = "say:";
		dp_strlcpy(text, chat_buffer, sizeof(text));
		linepos = chat_bufferpos;
		fnt = FONT_CHAT;
	}

	y = (int)strlen(text);

	// make the color code visible when the cursor is inside it
	if(text[linepos] != 0)
	{
		for(i=1; i < 5 && linepos - i > 0; ++i)
			if(text[linepos-i] == STRING_COLOR_TAG)
			{
				int caret_pos, ofs = 0;
				caret_pos = linepos - i;
				if(i == 1 && text[caret_pos+1] == STRING_COLOR_TAG)
					ofs = 1;
				else if(i == 1 && isdigit(text[caret_pos+1]))
					ofs = 2;
				else if(text[caret_pos+1] == STRING_COLOR_RGB_TAG_CHAR && isxdigit(text[caret_pos+2]) && isxdigit(text[caret_pos+3]) && isxdigit(text[caret_pos+4]))
					ofs = 5;
				if(ofs && (size_t)(y + ofs + 1) < sizeof(text))
				{
					int carets = 1;
					while(caret_pos - carets >= 1 && text[caret_pos - carets] == STRING_COLOR_TAG)
						++carets;
					if(carets & 1)
					{
						// str^2ing (displayed as string) --> str^2^^2ing (displayed as str^2ing)
						// str^^ing (displayed as str^ing) --> str^^^^ing (displayed as str^^ing)
						memmove(&text[caret_pos + ofs + 1], &text[caret_pos], y - caret_pos);
						text[caret_pos + ofs] = STRING_COLOR_TAG;
						y += ofs + 1;
						text[y] = 0;
					}
				}
				break;
			}
	}

	if (!is_console)
	{
		prefix_start = x;
		x += DrawQ_TextWidth(prefix, 0, inputsize, inputsize, false, fnt);
	}

	len_out = linepos;
	col_out = -1;
	xo = 0;
	if (linepos > 0)
		xo = DrawQ_TextWidth_UntilWidth_TrackColors(text, &len_out, inputsize, inputsize, &col_out, false, fnt, 1000000000);

	text_start = x + (vid_conwidth.value - x) * 0.95 - xo; // scroll
	if(text_start >= x)
		text_start = x;
	else if (!is_console)
		prefix_start -= (x - text_start);

	if (!is_console)
		DrawQ_String(prefix_start, v, prefix, 0, inputsize, inputsize, 1.0, 1.0, 1.0, 1.0, 0, NULL, false, fnt);

	DrawQ_String(text_start, v, text, y + 3, inputsize, inputsize, 1.0, 1.0, 1.0, 1.0, 0, NULL, false, fnt);

	// draw a cursor on top of this
	if ((int)(host.realtime*con_cursorspeed) & 1)		// cursor is visible
	{
		if (!utf8_enable.integer)
		{
			text[0] = 11 + 130 * key_insert;	// either solid or triangle facing right
			text[1] = 0;
		}
		else
		{
			size_t len;
			const char *curbuf;
			char charbuf16[16];
			curbuf = u8_encodech(0xE000 + 11 + 130 * key_insert, &len, charbuf16);
			memcpy(text, curbuf, len);
			text[len] = 0;
		}
		DrawQ_String(text_start + xo, v, text, 0, inputsize, inputsize, 1.0, 1.0, 1.0, 1.0, 0, &col_out, false, fnt);
	}
}

typedef struct
{
	dp_font_t *font;
	float alignment; // 0 = left, 0.5 = center, 1 = right
	float fontsize;
	float x;
	float y;
	float width;
	float ymin, ymax;
	const char *continuationString;

	// PRIVATE:
	int colorindex; // init to -1
}
con_text_info_t;

static float Con_WordWidthFunc(void *passthrough, const char *w, size_t *length, float maxWidth)
{
	con_text_info_t *ti = (con_text_info_t *) passthrough;
	if(w == NULL)
	{
		ti->colorindex = -1;
		return ti->fontsize * ti->font->maxwidth;
	}
	if(maxWidth >= 0)
		return DrawQ_TextWidth_UntilWidth(w, length, ti->fontsize, ti->fontsize, false, ti->font, -maxWidth); // -maxWidth: we want at least one char
	else if(maxWidth == -1)
		return DrawQ_TextWidth(w, *length, ti->fontsize, ti->fontsize, false, ti->font);
	else
	{
		Sys_Printf("Con_WordWidthFunc: can't get here (maxWidth should never be %f)\n", maxWidth);
		// Note: this is NOT a Con_Printf, as it could print recursively
		return 0;
	}
}

static int Con_CountLineFunc(void *passthrough, const char *line, size_t length, float width, qbool isContinuation)
{
	(void) passthrough;
	(void) line;
	(void) length;
	(void) width;
	(void) isContinuation;
	return 1;
}

static int Con_DisplayLineFunc(void *passthrough, const char *line, size_t length, float width, qbool isContinuation)
{
	con_text_info_t *ti = (con_text_info_t *) passthrough;

	if(ti->y < ti->ymin - 0.001)
		(void) 0;
	else if(ti->y > ti->ymax - ti->fontsize + 0.001)
		(void) 0;
	else
	{
		int x = (int) (ti->x + (ti->width - width) * ti->alignment);
		if(isContinuation && *ti->continuationString)
			x = (int) DrawQ_String(x, ti->y, ti->continuationString, strlen(ti->continuationString), ti->fontsize, ti->fontsize, 1.0, 1.0, 1.0, 1.0, 0, NULL, false, ti->font);
		if(length > 0)
			DrawQ_String(x, ti->y, line, length, ti->fontsize, ti->fontsize, 1.0, 1.0, 1.0, 1.0, 0, &(ti->colorindex), false, ti->font);
	}

	ti->y += ti->fontsize;
	return 1;
}

static int Con_DrawNotifyRect(unsigned mask_must, unsigned mask_mustnot, float maxage, float x, float y, float width, float height, float fontsize, float alignment_x, float alignment_y, const char *continuationString)
{
	int i;
	int lines = 0;
	int maxlines = (int) floor(height / fontsize + 0.01f);
	int startidx;
	int nskip = 0;
	int continuationWidth = 0;
	size_t len;
	double t = cl.time; // saved so it won't change
	con_text_info_t ti;

	ti.font = (mask_must & CON_MASK_CHAT) ? FONT_CHAT : FONT_NOTIFY;
	ti.fontsize = fontsize;
	ti.alignment = alignment_x;
	ti.width = width;
	ti.ymin = y;
	ti.ymax = y + height;
	ti.continuationString = continuationString;

	len = 0;
	Con_WordWidthFunc(&ti, NULL, &len, -1);
	len = strlen(continuationString);
	continuationWidth = (int) Con_WordWidthFunc(&ti, continuationString, &len, -1);

	// first find the first line to draw by backwards iterating and word wrapping to find their length...
	startidx = CON_LINES_COUNT;
	for(i = CON_LINES_COUNT - 1; i >= 0; --i)
	{
		con_lineinfo_t *l = &CON_LINES(i);
		int mylines;

		if((l->mask & mask_must) != mask_must)
			continue;
		if(l->mask & mask_mustnot)
			continue;
		if(maxage && (l->addtime < t - maxage))
			continue;

		// WE FOUND ONE!
		// Calculate its actual height...
		mylines = COM_Wordwrap(l->start, l->len, continuationWidth, width, Con_WordWidthFunc, &ti, Con_CountLineFunc, &ti);
		if(lines + mylines >= maxlines)
		{
			nskip = lines + mylines - maxlines;
			lines = maxlines;
			startidx = i;
			break;
		}
		lines += mylines;
		startidx = i;
	}

	// then center according to the calculated amount of lines...
	ti.x = x;
	ti.y = y + alignment_y * (height - lines * fontsize) - nskip * fontsize;

	// then actually draw
	for(i = startidx; i < CON_LINES_COUNT; ++i)
	{
		con_lineinfo_t *l = &CON_LINES(i);

		if((l->mask & mask_must) != mask_must)
			continue;
		if(l->mask & mask_mustnot)
			continue;
		if(maxage && (l->addtime < t - maxage))
			continue;

		COM_Wordwrap(l->start, l->len, continuationWidth, width, Con_WordWidthFunc, &ti, Con_DisplayLineFunc, &ti);
	}

	return lines;
}

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	float x, v;
	float chatstart, notifystart, inputsize, height;
	float align;
	int numChatlines;
	int chatpos;

	if (con_mutex) Thread_LockMutex(con_mutex);
	ConBuffer_FixTimes(&con);

	numChatlines = con_chat.integer;

	chatpos = con_chatpos.integer;

	if (con_notify.integer < 0)
		Cvar_SetValueQuick(&con_notify, 0);
	if (gamemode == GAME_TRANSFUSION)
		v = 8; // vertical offset
	else
		v = 0;

	// GAME_NEXUIZ: center, otherwise left justify
	align = con_notifyalign.value;
	if(!*con_notifyalign.string) // empty string, evaluated to 0 above
	{
		if(IS_OLDNEXUIZ_DERIVED(gamemode))
			align = 0.5;
	}

	if(numChatlines || !con_chatrect.integer)
	{
		if(chatpos == 0)
		{
			// first chat, input line, then notify
			chatstart = v;
			notifystart = v + (numChatlines + 1) * con_chatsize.value;
		}
		else if(chatpos > 0)
		{
			// first notify, then (chatpos-1) empty lines, then chat, then input
			notifystart = v;
			chatstart = v + (con_notify.value + (chatpos - 1)) * con_notifysize.value;
		}
		else // if(chatpos < 0)
		{
			// first notify, then much space, then chat, then input, then -chatpos-1 empty lines
			notifystart = v;
			chatstart = vid_conheight.value - (-chatpos-1 + numChatlines + 1) * con_chatsize.value;
		}
	}
	else
	{
		// just notify and input
		notifystart = v;
		chatstart = 0; // shut off gcc warning
	}

	v = notifystart + con_notifysize.value * Con_DrawNotifyRect(0, CON_MASK_INPUT | CON_MASK_HIDENOTIFY | (numChatlines ? CON_MASK_CHAT : 0) | CON_MASK_DEVELOPER, con_notifytime.value, 0, notifystart, vid_conwidth.value, con_notify.value * con_notifysize.value, con_notifysize.value, align, 0.0, "");

	if(con_chatrect.integer)
	{
		x = con_chatrect_x.value * vid_conwidth.value;
		v = con_chatrect_y.value * vid_conheight.value;
	}
	else
	{
		x = 0;
		if(numChatlines) // only do this if chat area is enabled, or this would move the input line wrong
			v = chatstart;
	}
	height = numChatlines * con_chatsize.value;

	if(numChatlines)
	{
		Con_DrawNotifyRect(CON_MASK_CHAT, CON_MASK_INPUT, con_chattime.value, x, v, vid_conwidth.value * con_chatwidth.value, height, con_chatsize.value, 0.0, 1.0, "^3 ... ");
		v += height;
	}
	if (key_dest == key_message)
	{
		inputsize = (numChatlines ? con_chatsize : con_notifysize).value;
		Con_DrawInput(false, x, v, inputsize);
	}
	else
		chat_bufferpos = 0;

	if (con_mutex) Thread_UnlockMutex(con_mutex);
}

/*
================
Con_LineHeight

Returns the height of a given console line; calculates it if necessary.
================
*/
static int Con_LineHeight(int lineno)
{
	con_lineinfo_t *li = &CON_LINES(lineno);
	if(li->height == -1)
	{
		float width = vid_conwidth.value;
		con_text_info_t ti;
		ti.fontsize = con_textsize.value;
		ti.font = FONT_CONSOLE;
		li->height = COM_Wordwrap(li->start, li->len, 0, width, Con_WordWidthFunc, &ti, Con_CountLineFunc, NULL);
	}
	return li->height;
}

/*
================
Con_DrawConsoleLine

Draws a line of the console; returns its height in lines.
If alpha is 0, the line is not drawn, but still wrapped and its height
returned.
================
*/
static int Con_DrawConsoleLine(unsigned mask_must, unsigned mask_mustnot, float y, int lineno, float ymin, float ymax)
{
	float width = vid_conwidth.value;
	con_text_info_t ti;
	con_lineinfo_t *li = &CON_LINES(lineno);

	if((li->mask & mask_must) != mask_must)
		return 0;
	if((li->mask & mask_mustnot) != 0)
		return 0;

	ti.continuationString = "";
	ti.alignment = 0;
	ti.fontsize = con_textsize.value;
	ti.font = FONT_CONSOLE;
	ti.x = 0;
	ti.y = y - (Con_LineHeight(lineno) - 1) * ti.fontsize;
	ti.ymin = ymin;
	ti.ymax = ymax;
	ti.width = width;

	return COM_Wordwrap(li->start, li->len, 0, width, Con_WordWidthFunc, &ti, Con_DisplayLineFunc, &ti);
}

/*
================
Con_LastVisibleLine

Calculates the last visible line index and how much to show of it based on
con_backscroll.
================
*/
static void Con_LastVisibleLine(unsigned mask_must, unsigned mask_mustnot, int *last, int *limitlast)
{
	int lines_seen = 0;
	int i;

	if(con_backscroll < 0)
		con_backscroll = 0;

	*last = 0;

	// now count until we saw con_backscroll actual lines
	for(i = CON_LINES_COUNT - 1; i >= 0; --i)
	if((CON_LINES(i).mask & mask_must) == mask_must)
	if((CON_LINES(i).mask & mask_mustnot) == 0)
	{
		int h = Con_LineHeight(i);

		// line is the last visible line?
		*last = i;
		if(lines_seen + h > con_backscroll && lines_seen <= con_backscroll)
		{
			*limitlast = lines_seen + h - con_backscroll;
			return;
		}

		lines_seen += h;
	}

	// if we get here, no line was on screen - scroll so that one line is
	// visible then.
	con_backscroll = lines_seen - 1;
	*limitlast = 1;
}

/*
================
Con_DrawConsole

Draws the console with the solid background
The typing input line at the bottom should only be drawn if typing is allowed
================
*/
void Con_DrawConsole (int lines)
{
	float alpha, alpha0;
	double sx, sy;
	int mask_must = 0;
	int mask_mustnot = (developer.integer>0) ? 0 : CON_MASK_DEVELOPER;
	cachepic_t *conbackpic;
	unsigned int conbackflags;

	if (lines <= 0)
		return;

	if (con_mutex) Thread_LockMutex(con_mutex);

	if (con_backscroll < 0)
		con_backscroll = 0;

	con_vislines = lines;

	r_draw2d_force = true;

// draw the background
	alpha0 = cls.signon == SIGNONS ? scr_conalpha.value : 1.0f; // always full alpha when not in game
	if((alpha = alpha0 * scr_conalphafactor.value) > 0)
	{
		sx = scr_conscroll_x.value;
		sy = scr_conscroll_y.value;
		conbackflags = CACHEPICFLAG_FAILONMISSING; // So console is readable when game content is missing
		if (sx != 0 || sy != 0)
			conbackflags &= CACHEPICFLAG_NOCLAMP;
		conbackpic = scr_conbrightness.value >= 0.01f ? Draw_CachePic_Flags("gfx/conback", conbackflags) : NULL;
		sx *= host.realtime; sy *= host.realtime;
		sx -= floor(sx); sy -= floor(sy);
		if (Draw_IsPicLoaded(conbackpic))
			DrawQ_SuperPic(0, lines - vid_conheight.integer, conbackpic, vid_conwidth.integer, vid_conheight.integer,
					0 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0);
		else
			DrawQ_Fill(0, lines - vid_conheight.integer, vid_conwidth.integer, vid_conheight.integer, 0.0f, 0.0f, 0.0f, alpha, 0);
	}
	if((alpha = alpha0 * scr_conalpha2factor.value) > 0)
	{
		sx = scr_conscroll2_x.value;
		sy = scr_conscroll2_y.value;
		conbackpic = Draw_CachePic_Flags("gfx/conback2", (sx != 0 || sy != 0) ? CACHEPICFLAG_NOCLAMP : 0);
		sx *= host.realtime; sy *= host.realtime;
		sx -= floor(sx); sy -= floor(sy);
		if(Draw_IsPicLoaded(conbackpic))
			DrawQ_SuperPic(0, lines - vid_conheight.integer, conbackpic, vid_conwidth.integer, vid_conheight.integer,
					0 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0);
	}
	if((alpha = alpha0 * scr_conalpha3factor.value) > 0)
	{
		sx = scr_conscroll3_x.value;
		sy = scr_conscroll3_y.value;
		conbackpic = Draw_CachePic_Flags("gfx/conback3", (sx != 0 || sy != 0) ? CACHEPICFLAG_NOCLAMP : 0);
		sx *= host.realtime; sy *= host.realtime;
		sx -= floor(sx); sy -= floor(sy);
		if(Draw_IsPicLoaded(conbackpic))
			DrawQ_SuperPic(0, lines - vid_conheight.integer, conbackpic, vid_conwidth.integer, vid_conheight.integer,
					0 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 0 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					1 + sx, 1 + sy, scr_conbrightness.value, scr_conbrightness.value, scr_conbrightness.value, alpha,
					0);
	}
	DrawQ_String(vid_conwidth.integer - DrawQ_TextWidth(engineversion, 0, con_textsize.value, con_textsize.value, false, FONT_CONSOLE), lines - con_textsize.value, engineversion, 0, con_textsize.value, con_textsize.value, 1, 0, 0, 1, 0, NULL, true, FONT_CONSOLE);

// draw the text
#if 0
	{
		int i;
		int count = CON_LINES_COUNT;
		float ymax = con_vislines - 2 * con_textsize.value;
		float y = ymax + con_textsize.value * con_backscroll;
		for (i = 0;i < count && y >= 0;i++)
			y -= Con_DrawConsoleLine(mask_must, mask_mustnot, y - con_textsize.value, CON_LINES_COUNT - 1 - i, 0, ymax) * con_textsize.value;
		// fix any excessive scrollback for the next frame
		if (i >= count && y >= 0)
		{
			con_backscroll -= (int)(y / con_textsize.value);
			if (con_backscroll < 0)
				con_backscroll = 0;
		}
	}
#else
	if(CON_LINES_COUNT > 0)
	{
		int i, last, limitlast;
		float y;
		float ymax = con_vislines - 2 * con_textsize.value;
		Con_LastVisibleLine(mask_must, mask_mustnot, &last, &limitlast);
		//Con_LastVisibleLine(mask_must, mask_mustnot, &last, &limitlast);
		y = ymax - con_textsize.value;

		if(limitlast)
			y += (CON_LINES(last).height - limitlast) * con_textsize.value;
		i = last;

		for(;;)
		{
			y -= Con_DrawConsoleLine(mask_must, mask_mustnot, y, i, 0, ymax) * con_textsize.value;
			if(i == 0)
				break; // top of console buffer
			if(y < 0)
				break; // top of console window
			limitlast = 0;
			--i;
		}
	}
#endif

// draw the input prompt, user text, and cursor if desired
	Con_DrawInput(true, 0, con_vislines - con_textsize.value * 2, con_textsize.value);

	r_draw2d_force = false;
	if (con_mutex) Thread_UnlockMutex(con_mutex);
}

/*
GetMapList

Made by [515]
Prints not only map filename, but also
its format (q1/q2/q3/hl) and even its message
*/
//[515]: here is an ugly hack.. two gotos... oh my... *but it works*
//LadyHavoc: rewrote bsp type detection, rewrote message extraction to do proper worldspawn parsing
//LadyHavoc: added .ent file loading, and redesigned error handling to still try the .ent file even if the map format is not recognized, this also eliminated one goto
//LadyHavoc: FIXME: man this GetMapList is STILL ugly code even after my cleanups...
qbool GetMapList (const char *s, char *completedname, int completednamebufferlength)
{
	fssearch_t	*t;
	char		message[1024];
	int			i, k, max, p, o, min;
	unsigned char *len;
	qfile_t		*f;
	unsigned char buf[1024];

	dpsnprintf(message, sizeof(message), "maps/%s*.bsp", s);
	t = FS_Search(message, 1, true, NULL);
	if(!t)
		return false;
	if (t->numfilenames > 1)
		Con_Printf("^1 %i maps found :\n", t->numfilenames);
	len = (unsigned char *)Z_Malloc(t->numfilenames);
	min = 666;
	for(max=i=0;i<t->numfilenames;i++)
	{
		k = (int)strlen(t->filenames[i]);
		k -= 9;
		if(max < k)
			max = k;
		else
		if(min > k)
			min = k;
		len[i] = k;
	}
	o = (int)strlen(s);
	for(i=0;i<t->numfilenames;i++)
	{
		int lumpofs = 0, lumplen = 0;
		char *entities = NULL;
		const char *data = NULL;
		char keyname[64];
		char entfilename[MAX_QPATH];
		char desc[64];
		desc[0] = 0;
		dp_strlcpy(message, "^1ERROR: open failed^7", sizeof(message));
		p = 0;
		f = FS_OpenVirtualFile(t->filenames[i], true);
		if(f)
		{
			dp_strlcpy(message, "^1ERROR: not a known map format^7", sizeof(message));
			memset(buf, 0, 1024);
			FS_Read(f, buf, 1024);
			if (!memcmp(buf, "IBSP", 4))
			{
				p = LittleLong(((int *)buf)[1]);
				if (p == Q3BSPVERSION || p == Q3BSPVERSION_LIVE || p == Q3BSPVERSION_IG)
				{
					q3dheader_t *header = (q3dheader_t *)buf;
					lumpofs = LittleLong(header->lumps[Q3LUMP_ENTITIES].fileofs);
					lumplen = LittleLong(header->lumps[Q3LUMP_ENTITIES].filelen);
					dpsnprintf(desc, sizeof(desc), "Q3BSP%i", p);
				}
				else if (p == Q2BSPVERSION)
				{
					q2dheader_t *header = (q2dheader_t *)buf;
					lumpofs = LittleLong(header->lumps[Q2LUMP_ENTITIES].fileofs);
					lumplen = LittleLong(header->lumps[Q2LUMP_ENTITIES].filelen);
					dpsnprintf(desc, sizeof(desc), "Q2BSP%i", p);
				}
				else
					dpsnprintf(desc, sizeof(desc), "IBSP%i", p);
			}
			else if (BuffLittleLong(buf) == BSPVERSION)
			{
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				dpsnprintf(desc, sizeof(desc), "BSP29");
			}
			else if (BuffLittleLong(buf) == 30)
			{
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				dpsnprintf(desc, sizeof(desc), "BSPHL");
			}
			else if (!memcmp(buf, "BSP2", 4))
			{
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				dpsnprintf(desc, sizeof(desc), "BSP2");
			}
			else if (!memcmp(buf, "2PSB", 4))
			{
				lumpofs = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES);
				lumplen = BuffLittleLong(buf + 4 + 8 * LUMP_ENTITIES + 4);
				dpsnprintf(desc, sizeof(desc), "BSP2RMQe");
			}
			else if(!memcmp(buf, "VBSP", 4))
			{
				hl2dheader_t *header = (hl2dheader_t *)buf;
				lumpofs = LittleLong(header->lumps[HL2LUMP_ENTITIES].fileofs);
				lumplen = LittleLong(header->lumps[HL2LUMP_ENTITIES].filelen);
				dpsnprintf(desc, sizeof(desc), "VBSP%i", LittleLong(((int *)buf)[1]));
			}
			else
				dpsnprintf(desc, sizeof(desc), "unknown%i", BuffLittleLong(buf));
			dp_strlcpy(entfilename, t->filenames[i], sizeof(entfilename));
			memcpy(entfilename + strlen(entfilename) - 4, ".ent", 5);
			entities = (char *)FS_LoadFile(entfilename, tempmempool, true, NULL);
			if (!entities && lumplen >= 10)
			{
				FS_Seek(f, lumpofs, SEEK_SET);
				entities = (char *)Z_Malloc(lumplen + 1);
				FS_Read(f, entities, lumplen);
			}
			if (entities)
			{
				// if there are entities to parse, a missing message key just
				// means there is no title, so clear the message string now
				message[0] = 0;
				data = entities;
				for (;;)
				{
					int l;
					if (!COM_ParseToken_Simple(&data, false, false, true))
						break;
					if (com_token[0] == '{')
						continue;
					if (com_token[0] == '}')
						break;
					// skip leading whitespace
					for (k = 0;com_token[k] && ISWHITESPACE(com_token[k]);k++);
					for (l = 0;l < (int)sizeof(keyname) - 1 && com_token[k+l] && !ISWHITESPACE(com_token[k+l]);l++)
						keyname[l] = com_token[k+l];
					keyname[l] = 0;
					if (!COM_ParseToken_Simple(&data, false, false, true))
						break;
					if (developer_extra.integer)
						Con_DPrintf("key: %s %s\n", keyname, com_token);
					if (!strcmp(keyname, "message"))
					{
						// get the message contents
						dp_strlcpy(message, com_token, sizeof(message));
						break;
					}
				}
			}
		}
		if (entities)
			Z_Free(entities);
		if(f)
			FS_Close(f);
		*(t->filenames[i]+len[i]+5) = 0;
		Con_Printf("%16s (%-8s) %s\n", t->filenames[i]+5, desc, message);
	}
	Con_Print("\n");
	for(p=o;p<min;p++)
	{
		k = *(t->filenames[0]+5+p);
		if(k == 0)
			goto endcomplete;
		for(i=1;i<t->numfilenames;i++)
			if(*(t->filenames[i]+5+p) != k)
				goto endcomplete;
	}
endcomplete:
	if(p > o && completedname && completednamebufferlength > 0)
	{
		memset(completedname, 0, completednamebufferlength);
		memcpy(completedname, (t->filenames[0]+5), min(p, completednamebufferlength - 1));
	}
	Z_Free(len);
	FS_FreeSearch(t);
	return p > o;
}

/*
	Con_DisplayList

	New function for tab-completion system
	Added by EvilTypeGuy
	MEGA Thanks to Taniwha

*/
void Con_DisplayList(const char **list)
{
	int i = 0, pos = 0, len = 0, maxlen = 0, width = (con_linewidth - 4);
	const char **walk = list;

	while (*walk) {
		len = (int)strlen(*walk);
		if (len > maxlen)
			maxlen = len;
		walk++;
	}
	maxlen += 1;

	while (*list) {
		len = (int)strlen(*list);
		if (pos + maxlen >= width) {
			Con_Print("\n");
			pos = 0;
		}

		Con_Print(*list);
		for (i = 0; i < (maxlen - len); i++)
			Con_Print(" ");

		pos += maxlen;
		list++;
	}

	if (pos)
		Con_Print("\n\n");
}


// Now it becomes TRICKY :D --blub
static char Nicks_list[MAX_SCOREBOARD][MAX_SCOREBOARDNAME];	// contains the nicks with colors and all that
static char Nicks_sanlist[MAX_SCOREBOARD][MAX_SCOREBOARDNAME];	// sanitized list for completion when there are other possible matches.
// means: when somebody uses a cvar's name as his name, we won't ever get his colors in there...
static int Nicks_offset[MAX_SCOREBOARD]; // when nicks use a space, we need this to move the completion list string starts to avoid invalid memcpys
static int Nicks_matchpos;

// co against <<:BLASTER:>> is true!?
static int Nicks_strncasecmp_nospaces(char *a, char *b, unsigned int a_len)
{
	while(a_len)
	{
		if(tolower(*a) == tolower(*b))
		{
			if(*a == 0)
				return 0;
			--a_len;
			++a;
			++b;
			continue;
		}
		if(!*a)
			return -1;
		if(!*b)
			return 1;
		if(*a == ' ')
			return (*a < *b) ? -1 : 1;
		if(*b == ' ')
			++b;
		else
			return (*a < *b) ? -1 : 1;
	}
	return 0;
}
static int Nicks_strncasecmp(char *a, char *b, unsigned int a_len)
{
	char space_char;
	if(!(con_nickcompletion_flags.integer & NICKS_ALPHANUMERICS_ONLY))
	{
		if(con_nickcompletion_flags.integer & NICKS_NO_SPACES)
			return Nicks_strncasecmp_nospaces(a, b, a_len);
		return strncasecmp(a, b, a_len);
	}

	space_char = (con_nickcompletion_flags.integer & NICKS_NO_SPACES) ? 'a' : ' ';

	// ignore non alphanumerics of B
	// if A contains a non-alphanumeric, B must contain it as well though!
	while(a_len)
	{
		qbool alnum_a, alnum_b;

		if(tolower(*a) == tolower(*b))
		{
			if(*a == 0) // end of both strings, they're equal
				return 0;
			--a_len;
			++a;
			++b;
			continue;
		}
		// not equal, end of one string?
		if(!*a)
			return -1;
		if(!*b)
			return 1;
		// ignore non alphanumerics
		alnum_a = ( (*a >= 'a' && *a <= 'z') || (*a >= 'A' && *a <= 'Z') || (*a >= '0' && *a <= '9') || *a == space_char);
		alnum_b = ( (*b >= 'a' && *b <= 'z') || (*b >= 'A' && *b <= 'Z') || (*b >= '0' && *b <= '9') || *b == space_char);
		if(!alnum_a) // b must contain this
			return (*a < *b) ? -1 : 1;
		if(!alnum_b)
			++b;
		// otherwise, both are alnum, they're just not equal, return the appropriate number
		else
			return (*a < *b) ? -1 : 1;
	}
	return 0;
}


/* Nicks_CompleteCountPossible

   Count the number of possible nicks to complete
 */
static int Nicks_CompleteCountPossible(char *line, int pos, char *s, qbool isCon)
{
	char name[MAX_SCOREBOARDNAME];
	int i, p;
	int match;
	int spos;
	int count = 0;

	if(!con_nickcompletion.integer)
		return 0;

	// changed that to 1
	if(!line[0])// || !line[1]) // we want at least... 2 written characters
		return 0;

	for(i = 0; i < cl.maxclients; ++i)
	{
		p = i;
		if(!cl.scores[p].name[0])
			continue;

		SanitizeString(cl.scores[p].name, name);
		//Con_Printf(" ^2Sanitized: ^7%s -> %s", cl.scores[p].name, name);

		if(!name[0])
			continue;

		match = -1;
		spos = pos - 1; // no need for a minimum of characters :)

		while(spos >= 0)
		{
			if(spos > 0 && line[spos-1] != ' ' && line[spos-1] != ';' && line[spos-1] != '\"' && line[spos-1] != '\'')
			{
				if(!(isCon && spos == 1)) // console start
				{
					--spos;
					continue;
				}
			}
			if(isCon && spos == 0)
				break;
			if(Nicks_strncasecmp(line+spos, name, pos-spos) == 0)
				match = spos;
			--spos;
		}
		if(match < 0)
			continue;
		//Con_Printf("Possible match: %s|%s\n", cl.scores[p].name, name);
		dp_strlcpy(Nicks_list[count], cl.scores[p].name, sizeof(Nicks_list[count]));

		// the sanitized list
		dp_strlcpy(Nicks_sanlist[count], name, sizeof(Nicks_sanlist[count]));
		if(!count)
		{
			Nicks_matchpos = match;
		}

		Nicks_offset[count] = s - (&line[match]);
		//Con_Printf("offset for %s: %i\n", name, Nicks_offset[count]);

		++count;
	}
	return count;
}

static void Cmd_CompleteNicksPrint(int count)
{
	int i;
	for(i = 0; i < count; ++i)
		Con_Printf("%s\n", Nicks_list[i]);
}

static void Nicks_CutMatchesNormal(int count)
{
	// cut match 0 down to the longest possible completion
	int i;
	unsigned int c, l;
	c = (unsigned int)strlen(Nicks_sanlist[0]) - 1;
	for(i = 1; i < count; ++i)
	{
		l = (unsigned int)strlen(Nicks_sanlist[i]) - 1;
		if(l < c)
			c = l;

		for(l = 0; l <= c; ++l)
			if(tolower(Nicks_sanlist[0][l]) != tolower(Nicks_sanlist[i][l]))
			{
				c = l-1;
				break;
			}
	}
	Nicks_sanlist[0][c+1] = 0;
	//Con_Printf("List0: %s\n", Nicks_sanlist[0]);
}

static unsigned int Nicks_strcleanlen(const char *s)
{
	unsigned int l = 0;
	while(*s)
	{
		if( (*s >= 'a' && *s <= 'z') ||
		    (*s >= 'A' && *s <= 'Z') ||
		    (*s >= '0' && *s <= '9') ||
		    *s == ' ')
			++l;
		++s;
	}
	return l;
}

static void Nicks_CutMatchesAlphaNumeric(int count)
{
	// cut match 0 down to the longest possible completion
	int i;
	unsigned int c, l;
	char tempstr[sizeof(Nicks_sanlist[0])];
	char *a, *b;
	char space_char = (con_nickcompletion_flags.integer & NICKS_NO_SPACES) ? 'a' : ' '; // yes this is correct, we want NO spaces when no spaces

	c = (unsigned int)strlen(Nicks_sanlist[0]);
	for(i = 0, l = 0; i < (int)c; ++i)
	{
		if( (Nicks_sanlist[0][i] >= 'a' && Nicks_sanlist[0][i] <= 'z') ||
		    (Nicks_sanlist[0][i] >= 'A' && Nicks_sanlist[0][i] <= 'Z') ||
		    (Nicks_sanlist[0][i] >= '0' && Nicks_sanlist[0][i] <= '9') || Nicks_sanlist[0][i] == space_char) // this is what's COPIED
		{
			tempstr[l++] = Nicks_sanlist[0][i];
		}
	}
	tempstr[l] = 0;

	for(i = 1; i < count; ++i)
	{
		a = tempstr;
		b = Nicks_sanlist[i];
		while(1)
		{
			if(!*a)
				break;
			if(!*b)
			{
				*a = 0;
				break;
			}
			if(tolower(*a) == tolower(*b))
			{
				++a;
				++b;
				continue;
			}
			if( (*b >= 'a' && *b <= 'z') || (*b >= 'A' && *b <= 'Z') || (*b >= '0' && *b <= '9') || *b == space_char)
			{
				// b is alnum, so cut
				*a = 0;
				break;
			}
			++b;
		}
	}
	// Just so you know, if cutmatchesnormal doesn't kill the first entry, then even the non-alnums fit
	Nicks_CutMatchesNormal(count);
	//if(!Nicks_sanlist[0][0])
	if(Nicks_strcleanlen(Nicks_sanlist[0]) < strlen(tempstr))
	{
		// if the clean sanitized one is longer than the current one, use it, it has crap chars which definitely are in there
		dp_strlcpy(Nicks_sanlist[0], tempstr, sizeof(Nicks_sanlist[0]));
	}
}

static void Nicks_CutMatchesNoSpaces(int count)
{
	// cut match 0 down to the longest possible completion
	int i;
	unsigned int c, l;
	char tempstr[sizeof(Nicks_sanlist[0])];
	char *a, *b;

	c = (unsigned int)strlen(Nicks_sanlist[0]);
	for(i = 0, l = 0; i < (int)c; ++i)
	{
		if(Nicks_sanlist[0][i] != ' ') // here it's what's NOT copied
		{
			tempstr[l++] = Nicks_sanlist[0][i];
		}
	}
	tempstr[l] = 0;

	for(i = 1; i < count; ++i)
	{
		a = tempstr;
		b = Nicks_sanlist[i];
		while(1)
		{
			if(!*a)
				break;
			if(!*b)
			{
				*a = 0;
				break;
			}
			if(tolower(*a) == tolower(*b))
			{
				++a;
				++b;
				continue;
			}
			if(*b != ' ')
			{
				*a = 0;
				break;
			}
			++b;
		}
	}
	// Just so you know, if cutmatchesnormal doesn't kill the first entry, then even the non-alnums fit
	Nicks_CutMatchesNormal(count);
	//if(!Nicks_sanlist[0][0])
	//Con_Printf("TS: %s\n", tempstr);
	if(Nicks_strcleanlen(Nicks_sanlist[0]) < strlen(tempstr))
	{
		// if the clean sanitized one is longer than the current one, use it, it has crap chars which definitely are in there
		dp_strlcpy(Nicks_sanlist[0], tempstr, sizeof(Nicks_sanlist[0]));
	}
}

static void Nicks_CutMatches(int count)
{
	if(con_nickcompletion_flags.integer & NICKS_ALPHANUMERICS_ONLY)
		Nicks_CutMatchesAlphaNumeric(count);
	else if(con_nickcompletion_flags.integer & NICKS_NO_SPACES)
		Nicks_CutMatchesNoSpaces(count);
	else
		Nicks_CutMatchesNormal(count);
}

static const char **Nicks_CompleteBuildList(int count)
{
	const char **buf;
	int bpos = 0;
	// the list is freed by Con_CompleteCommandLine, so create a char**
	buf = (const char **)Mem_Alloc(tempmempool, count * sizeof(const char *) + sizeof (const char *));

	for(; bpos < count; ++bpos)
		buf[bpos] = Nicks_sanlist[bpos] + Nicks_offset[bpos];

	Nicks_CutMatches(count);

	buf[bpos] = NULL;
	return buf;
}

/*
	Nicks_AddLastColor
	Restores the previous used color, after the autocompleted name.
*/
static int Nicks_AddLastColor(char *buffer, int pos)
{
	qbool quote_added = false;
	int match;
	int color = STRING_COLOR_DEFAULT + '0';
	char r = 0, g = 0, b = 0;

	if(con_nickcompletion_flags.integer & NICKS_ADD_QUOTE && buffer[Nicks_matchpos-1] == '\"')
	{
		// we'll have to add a quote :)
		buffer[pos++] = '\"';
		quote_added = true;
	}

	if((!quote_added && con_nickcompletion_flags.integer & NICKS_ADD_COLOR) || con_nickcompletion_flags.integer & NICKS_FORCE_COLOR)
	{
		// add color when no quote was added, or when flags &4?
		// find last color
		for(match = Nicks_matchpos-1; match >= 0; --match)
		{
			if(buffer[match] == STRING_COLOR_TAG)
			{
				if( isdigit(buffer[match+1]) )
				{
					color = buffer[match+1];
					break;
				}
				else if(buffer[match+1] == STRING_COLOR_RGB_TAG_CHAR)
				{
					if ( isxdigit(buffer[match+2]) && isxdigit(buffer[match+3]) && isxdigit(buffer[match+4]) )
					{
						r = buffer[match+2];
						g = buffer[match+3];
						b = buffer[match+4];
						color = -1;
						break;
					}
				}
			}
		}
		if(!quote_added)
		{
			if( pos >= 2 && buffer[pos-2] == STRING_COLOR_TAG && isdigit(buffer[pos-1]) ) // when thes use &4
				pos -= 2;
			else if( pos >= 5 && buffer[pos-5] == STRING_COLOR_TAG && buffer[pos-4] == STRING_COLOR_RGB_TAG_CHAR
					 && isxdigit(buffer[pos-3]) && isxdigit(buffer[pos-2]) && isxdigit(buffer[pos-1]) )
				pos -= 5;
		}
		buffer[pos++] = STRING_COLOR_TAG;
		if (color == -1)
		{
			buffer[pos++] = STRING_COLOR_RGB_TAG_CHAR;
			buffer[pos++] = r;
			buffer[pos++] = g;
			buffer[pos++] = b;
		}
		else
			buffer[pos++] = color;
	}
	return pos;
}

/*
	Con_CompleteCommandLine

	New function for tab-completion system
	Added by EvilTypeGuy
	Thanks to Fett erich@heintz.com
	Thanks to taniwha
	Enhanced to tab-complete map names by [515]

*/
int Con_CompleteCommandLine(cmd_state_t *cmd, qbool is_console)
{
	const char *text = "";
	char *s;
	const char **list[4] = {0, 0, 0, 0};
	char s2[512];
	char command[512];
	int c, v, a, i, cmd_len, pos, k;
	int n; // nicks --blub
	const char *space, *patterns;
	char vabuf[1024];

	char *line;
	int linestart, linepos;
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

	//find what we want to complete
	pos = linepos;
	while(--pos >= linestart)
	{
		k = line[pos];
		if(k == '\"' || k == ';' || k == ' ' || k == '\'')
			break;
	}
	pos++;

	s = line + pos;
	dp_strlcpy(s2, line + linepos, sizeof(s2)); //save chars after cursor
	line[linepos] = 0; //hide them

	c = v = a = n = cmd_len = 0;
	if (!is_console)
		goto nicks;

	space = strchr(line + 1, ' ');
	if(space && pos == (space - line) + 1)
	{
		// adding 1 to line drops the leading ]
		dp_ustr2stp(command, sizeof(command), line + 1, space - (line + 1));

		patterns = Cvar_VariableString(cmd->cvars, va(vabuf, sizeof(vabuf), "con_completion_%s", command), CF_CLIENT | CF_SERVER); // TODO maybe use a better place for this?
		if(patterns && !*patterns)
			patterns = NULL; // get rid of the empty string

		if(!strcmp(command, "map") || !strcmp(command, "changelevel") || (patterns && !strcmp(patterns, "map")))
		{
			//maps search
			char t[MAX_QPATH];
			if (GetMapList(s, t, sizeof(t)))
			{
				// first move the cursor
				linepos += (int)strlen(t) - (int)strlen(s);

				// and now do the actual work
				*s = 0;
				dp_strlcat(line, t, MAX_INPUTLINE);
				dp_strlcat(line, s2, MAX_INPUTLINE); //add back chars after cursor

				// and fix the cursor
				if(linepos > (int) strlen(line))
					linepos = (int) strlen(line);
			}
			return linepos;
		}
		else
		{
			if(patterns)
			{
				char t[MAX_QPATH];
				stringlist_t resultbuf, dirbuf;

				// Usage:
				//   // store completion patterns (space separated) for command foo in con_completion_foo
				//   set con_completion_foo "foodata/*.foodefault *.foo"
				//   foo <TAB>
				//
				// Note: patterns with slash are always treated as absolute
				// patterns; patterns without slash search in the innermost
				// directory the user specified. There is no way to "complete into"
				// a directory as of now, as directories seem to be unknown to the
				// FS subsystem.
				//
				// Examples:
				//   set con_completion_playermodel "models/player/*.zym models/player/*.md3 models/player/*.psk models/player/*.dpm"
				//   set con_completion_playdemo "*.dem"
				//   set con_completion_play "*.wav *.ogg"
				//
				// TODO somehow add support for directories; these shall complete
				// to their name + an appended slash.

				stringlistinit(&resultbuf);
				stringlistinit(&dirbuf);
				while(COM_ParseToken_Simple(&patterns, false, false, true))
				{
					fssearch_t *search;
					if(strchr(com_token, '/'))
					{
						search = FS_Search(com_token, true, true, NULL);
					}
					else
					{
						const char *slash = strrchr(s, '/');
						if(slash)
						{
							dp_strlcpy(t, s, min(sizeof(t), (unsigned int)(slash - s + 2))); // + 2, because I want to include the slash
							dp_strlcat(t, com_token, sizeof(t));
							search = FS_Search(t, true, true, NULL);
						}
						else
							search = FS_Search(com_token, true, true, NULL);
					}
					if(search)
					{
						for(i = 0; i < search->numfilenames; ++i)
							if(!strncmp(search->filenames[i], s, strlen(s)))
								if(FS_FileType(search->filenames[i]) == FS_FILETYPE_FILE)
									stringlistappend(&resultbuf, search->filenames[i]);
						FS_FreeSearch(search);
					}
				}

				// In any case, add directory names
				{
					fssearch_t *search;
					const char *slash = strrchr(s, '/');
					if(slash)
					{
						dp_strlcpy(t, s, min(sizeof(t), (unsigned int)(slash - s + 2))); // + 2, because I want to include the slash
						dp_strlcat(t, "*", sizeof(t));
						search = FS_Search(t, true, true, NULL);
					}
					else
						search = FS_Search("*", true, true, NULL);
					if(search)
					{
						for(i = 0; i < search->numfilenames; ++i)
							if(!strncmp(search->filenames[i], s, strlen(s)))
								if(FS_FileType(search->filenames[i]) == FS_FILETYPE_DIRECTORY)
									stringlistappend(&dirbuf, search->filenames[i]);
						FS_FreeSearch(search);
					}
				}

				if(resultbuf.numstrings > 0 || dirbuf.numstrings > 0)
				{
					const char *p, *q;
					unsigned int matchchars;
					if(resultbuf.numstrings == 0 && dirbuf.numstrings == 1)
					{
						dpsnprintf(t, sizeof(t), "%s/", dirbuf.strings[0]);
					}
					else
					if(resultbuf.numstrings == 1 && dirbuf.numstrings == 0)
					{
						dpsnprintf(t, sizeof(t), "%s ", resultbuf.strings[0]);
					}
					else
					{
						stringlistsort(&resultbuf, true); // dirbuf is already sorted
						Con_Printf("\n%i possible filenames\n", resultbuf.numstrings + dirbuf.numstrings);
						for(i = 0; i < dirbuf.numstrings; ++i)
						{
							Con_Printf("^4%s^7/\n", dirbuf.strings[i]);
						}
						for(i = 0; i < resultbuf.numstrings; ++i)
						{
							Con_Printf("%s\n", resultbuf.strings[i]);
						}
						matchchars = sizeof(t) - 1;
						if(resultbuf.numstrings > 0)
						{
							p = resultbuf.strings[0];
							q = resultbuf.strings[resultbuf.numstrings - 1];
							for(; *p && *p == *q; ++p, ++q);
							matchchars = (unsigned int)(p - resultbuf.strings[0]);
						}
						if(dirbuf.numstrings > 0)
						{
							p = dirbuf.strings[0];
							q = dirbuf.strings[dirbuf.numstrings - 1];
							for(; *p && *p == *q; ++p, ++q);
							matchchars = min(matchchars, (unsigned int)(p - dirbuf.strings[0]));
						}
						// now p points to the first non-equal character, or to the end
						// of resultbuf.strings[0]. We want to append the characters
						// from resultbuf.strings[0] to (not including) p as these are
						// the unique prefix
						dp_strlcpy(t, (resultbuf.numstrings > 0 ? resultbuf : dirbuf).strings[0], min(matchchars + 1, sizeof(t)));
					}

					// first move the cursor
					linepos += (int)strlen(t) - (int)strlen(s);

					// and now do the actual work
					*s = 0;
					dp_strlcat(line, t, MAX_INPUTLINE);
					dp_strlcat(line, s2, MAX_INPUTLINE); //add back chars after cursor

					// and fix the cursor
					if(linepos > (int) strlen(line))
						linepos = (int) strlen(line);
				}
				stringlistfreecontents(&resultbuf);
				stringlistfreecontents(&dirbuf);

				return linepos; // bail out, when we complete for a command that wants a file name
			}
		}
	}

	// Count number of possible matches and print them
	c = Cmd_CompleteCountPossible(cmd, s);
	if (c)
	{
		Con_Printf("\n%i possible command%s\n", c, (c > 1) ? "s: " : ":");
		Cmd_CompleteCommandPrint(cmd, s);
	}
	v = Cvar_CompleteCountPossible(cmd->cvars, s, CF_CLIENT | CF_SERVER);
	if (v)
	{
		Con_Printf("\n%i possible variable%s\n", v, (v > 1) ? "s: " : ":");
		Cvar_CompleteCvarPrint(cmd->cvars, s, CF_CLIENT | CF_SERVER);
	}
	a = Cmd_CompleteAliasCountPossible(cmd, s);
	if (a)
	{
		Con_Printf("\n%i possible alias%s\n", a, (a > 1) ? "es: " : ":");
		Cmd_CompleteAliasPrint(cmd, s);
	}

nicks:
	n = Nicks_CompleteCountPossible(line, linepos, s, is_console);
	if (n)
	{
		Con_Printf("\n%i possible nick%s\n", n, (n > 1) ? "s: " : ":");
		Cmd_CompleteNicksPrint(n);
	}

	if (!(c + v + a + n))	// No possible matches
	{
		if(s2[0])
			dp_strlcpy(&line[linepos], s2, linesize - linepos);
		return linepos;
	}

	if (c)
		text = *(list[0] = Cmd_CompleteBuildList(cmd, s));
	if (v)
		text = *(list[1] = Cvar_CompleteBuildList(cmd->cvars, s, cmd->cvars_flagsmask));
	if (a)
		text = *(list[2] = Cmd_CompleteAliasBuildList(cmd, s));
	if (n)
	{
		if (is_console)
			text = *(list[3] = Nicks_CompleteBuildList(n));
		else
			text = *(Nicks_CompleteBuildList(n));
	}

	for (cmd_len = (int)strlen(s);;cmd_len++)
	{
		const char **l;
		for (i = 0; i < 3; i++)
			if (list[i])
				for (l = list[i];*l;l++)
					if ((*l)[cmd_len] != text[cmd_len])
						goto done;
		// all possible matches share this character, so we continue...
		if (!text[cmd_len])
		{
			// if all matches ended at the same position, stop
			// (this means there is only one match)
			break;
		}
	}
done:

	// prevent a buffer overrun by limiting cmd_len according to remaining space
	cmd_len = min(cmd_len, (int)linesize - 1 - pos);
	if (text)
	{
		linepos = pos;
		memcpy(&line[linepos], text, cmd_len);
		linepos += cmd_len;
		// if there is only one match, add a space after it
		if (c + v + a + n == 1 && linepos < (int)linesize - 1)
		{
			if(n)
			{ // was a nick, might have an offset, and needs colors ;) --blub
				linepos = pos - Nicks_offset[0];
				cmd_len = (int)strlen(Nicks_list[0]);
				cmd_len = min(cmd_len, (int)linesize - 3 - pos);

				memcpy(&line[linepos] , Nicks_list[0], cmd_len);
				linepos += cmd_len;
				if(linepos < (int)(linesize - 7)) // space for color code (^[0-9] or ^xrgb), space and \0
					linepos = Nicks_AddLastColor(line, linepos);
			}
			line[linepos++] = ' ';
		}
	}

	// use strlcat to avoid a buffer overrun
	line[linepos] = 0;
	dp_strlcat(line, s2, linesize);

	if (!is_console)
		return linepos;

	// free the command, cvar, and alias lists
	for (i = 0; i < 4; i++)
		if (list[i])
			Mem_Free((void *)list[i]);

	return linepos;
}

