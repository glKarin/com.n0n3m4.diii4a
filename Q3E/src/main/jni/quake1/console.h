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

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>
#include "qtypes.h"
#include "cmd.h"
#include "lhnet.h"

//
// console
//
extern int con_totallines;
extern int con_backscroll;
extern qbool con_initialized;

void Con_Rcon_Redirect_Init(lhnetsocket_t *sock, lhnetaddress_t *dest, qbool proquakeprotocol);
void Con_Rcon_Redirect_End(void);
void Con_Rcon_Redirect_Abort(void);

/// If the line width has changed, reformat the buffer.
void Con_CheckResize (void);
void Con_Init (void);
void Con_Init_Commands (void);
void Con_Shutdown (void);
void Con_DrawConsole (int lines, qbool forcedfullscreen);

/// Prints to a chosen console target
void Con_MaskPrint(unsigned additionalmask, const char *msg);

// Prints to a chosen console target
void Con_MaskPrintf(unsigned mask, const char *fmt, ...) DP_FUNC_PRINTF(2);

/// Prints to all appropriate console targets, and adds timestamps
void Con_Print(const char *txt);

/// Prints to all appropriate console targets.
void Con_Printf(const char *fmt, ...) DP_FUNC_PRINTF(1);

/// A Con_Print that only shows up if the "developer" cvar is set.
void Con_DPrint(const char *msg);

/// A Con_Printf that only shows up if the "developer" cvar is set
void Con_DPrintf(const char *fmt, ...) DP_FUNC_PRINTF(1);

const char *Con_Quakebar(int len, char *bar, size_t bufsize);
void Con_CenterPrintf(int maxLineLength, const char *fmt, ...) DP_FUNC_PRINTF(2);
void Con_CenterPrint(const char *str);

void Con_Clear_f(cmd_state_t *cmd);
void Con_DrawNotify (void);

/// Clear all notify lines.
void Con_ClearNotify (void);
void Con_ToggleConsole_f(cmd_state_t *cmd);

qbool GetMapList (const char *s, char *completedname, int completednamebufferlength);

/// wrapper function to attempt to either complete the command line
/// or to list possible matches grouped by type
/// (i.e. will display possible variables, aliases, commands
/// that match what they've typed so far)
int Con_CompleteCommandLine(cmd_state_t *cmd, qbool is_console);

/// Generic libs/util/console.c function to display a list
/// formatted in columns on the console
void Con_DisplayList(const char **list);


/*! \name log
 * @{
 */
void Log_Init (void);
void Log_Close (void);
void Log_Start (void);
void Log_DestBuffer_Flush (void); ///< call this once per frame to send out replies to rcon streaming clients

void Log_Printf(const char *logfilename, const char *fmt, ...) DP_FUNC_PRINTF(2);
//@}

#define CON_WARN "^3"
#define CON_ERROR "^1"

// CON_MASK_PRINT is the default (Con_Print/Con_Printf)
// CON_MASK_DEVELOPER is used by Con_DPrint/Con_DPrintf
#define CON_MASK_HIDENOTIFY 128
#define CON_MASK_CHAT 1
#define CON_MASK_INPUT 2
#define CON_MASK_DEVELOPER 4
#define CON_MASK_PRINT 8

typedef struct con_lineinfo_s
{
	char *start;
	size_t len;
	unsigned mask;

	/// used only by console.c
	double addtime;
	int height; ///< recalculated line height when needed (-1 to unset)
}
con_lineinfo_t;

typedef struct conbuffer_s
{
	qbool active;
	int textsize;
	char *text;
	int maxlines;
	con_lineinfo_t *lines;
	int lines_first;
	int lines_count; ///< cyclic buffer
}
conbuffer_t;

#define CONBUFFER_LINES(buf, i) (buf)->lines[((buf)->lines_first + (i)) % (buf)->maxlines]
#define CONBUFFER_LINES_COUNT(buf) ((buf)->lines_count)
#define CONBUFFER_LINES_LAST(buf) CONBUFFER_LINES(buf, CONBUFFER_LINES_COUNT(buf) - 1)

void ConBuffer_Init(conbuffer_t *buf, int textsize, int maxlines, mempool_t *mempool);
void ConBuffer_Clear (conbuffer_t *buf);
void ConBuffer_Shutdown(conbuffer_t *buf);

/*! Notifies the console code about the current time
 * (and shifts back times of other entries when the time
 * went backwards)
 */
void ConBuffer_FixTimes(conbuffer_t *buf);

/// Deletes the first line from the console history.
void ConBuffer_DeleteLine(conbuffer_t *buf);

/// Deletes the last line from the console history.
void ConBuffer_DeleteLastLine(conbuffer_t *buf);

/// Appends a given string as a new line to the console.
void ConBuffer_AddLine(conbuffer_t *buf, const char *line, int len, unsigned mask);
int ConBuffer_FindPrevLine(conbuffer_t *buf, unsigned mask_must, unsigned mask_mustnot, int start);
int ConBuffer_FindNextLine(conbuffer_t *buf, unsigned mask_must, unsigned mask_mustnot, int start);
const char *ConBuffer_GetLine(conbuffer_t *buf, int i);

#endif

