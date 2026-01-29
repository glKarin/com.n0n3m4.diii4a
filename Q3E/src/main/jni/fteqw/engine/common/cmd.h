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

// cmd.h -- Command buffer and command execution

//===========================================================================

/*FIXME: rewrite this to use something like the following
typedef struct
{
   	qbyte groups:27;
   	qbyte cvarlatch:1;	//latches cvars so the user cannot change them till next map. should be RESTRICT_LOCAL to avoid users cheating by restrict-to-block server commands.
	qbyte seat:4;		//for splitscreen binds etc. 1 based! 0 uses in_forceseat/first.
} cmdaccess_t;
enum {
//core groups
	CAG_OWNER		= 1<<0,	//commands that came from the keyboard (or stdin).
	CAG_MENUQC		= 1<<1,	//menuqc (mostly allowed to do stuff - but often also has CAG_DOWNLOADED)
	CAG_CSQC		= 1<<2,	//csqc localcmds, nearly always has CAG_DOWNLOADED, so a load of denies
	CAG_SSQC		= 1<<3,	//ssqc localcmds, able to poke quite a lot of things, generally unrestricted.
	CAG_SERVER		= 1<<4,	//ssqc stuffcmds, typically same access as csqc
//special groups:
	CAG_DOWNLOADED	= 1<<5,	//for execs that read from a downloaded(untrusted) package. also flagged by qc modules if they were from such packages (so usually included from csqc).
	CAG_SCRIPT		= 1<<6,	//added for binds (with more than one command), or for aliases/configs. exists for +lookdown's denies (using explicit checks, to avoid cheat bypasses).
//custom groups:
	CAG_RCON		= 1<<7,	//commands that came from rcon.

//groups of groups, for default masks or special denies
	CAG_DEFAULTALLOW = CAG_OWNER|CAG_MENUQC|CAG_CSQC|CAG_SSQC|CAG_SERVER|CAG_RCON,
	CAG_INSECURE	= CAG_DOWNLOADED|CAG_SERVER|CAG_CSQC,	//csqc included mostly for consistency.

	//	userN: defaults to no commands
	//	vip: BAN_VIP users
	//	mapper: BAN_MAPPER users
};

	//stuff can be accessed when:
	//	if ((command.allows&cbuf.groups) && !(command.denies&cbuf.groups)) accessisallowed;
	//cvars:
	//	separate allow-read, deny-read, allow-write, deny-write masks (set according to the older flags, to keep things simple)
	//aliases:
	//	alias execution ors in the group(s) that created it (and 'script'). some execution chains could end up with a LOT of groups... FIXME: is that a problem? just don't put potentially restricted things in aliases?
	//binds:
	//	also ors the creator's group - no `bind w doevil` and waiting.
	//multiple cbufs:
	//	ssqc still has a dedicated cbuf, so that wait commands cause THAT cbuf to wait, not others.
	//	rcon+readcmd have a separate cbuf, to try to isolate prints
	//seats:
	//	there's no security needed between seats, but server should maybe be blocked from seat switching commands (acting only as asserts), to catch bugs.
	//
	//'p2 nameofalias' sets seat to 2, overriding in_forceseat.
	//+lookup etc is blocked when allow_scripts==0 and indirect is set.
typedef struct
{
	cmdpermissions_t p; //access rights
} cmdstate_t;
void Cbuf_AddText(const char *text, qboolean addnl, qboolean insert); //null for local? all commands must be \n terminated.
char *Cbuf_GetNext(cmdpermissions_t permissions, qboolean ignoresemicolon);
void Cbuf_Execute(cbuf_t *cbuf);
void Cmd_ExecuteString (const char *text, const cmdstate_t *cstate);

//rcon can use a private cbuf, allowing it to parse properly despite level changes
//cbuf must internally track cmdpermissions_t of different blocks. inserstions/additions may merge, others force \n termination (network can do its own buffering).

typedef void (*xcommand_t) (const cmdstate_t *cstate);
*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

The game starts with a Cbuf_AddText ("exec quake.rc\n"); Cbuf_Execute ();

*/

void Cbuf_Waited(void);

void Cbuf_Init (void);
// allocates an initial text buffer that will grow as needed

void Cbuf_AddText (const char *text, int level);
// as new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.

void Cbuf_InsertText (const char *text, int level, qboolean addnl);
// when a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining unexecuted
// commands.

char *Cbuf_GetNext(int level, qboolean ignoresemicolon);

void Cbuf_Execute (void);
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!

extern qboolean cmd_blockwait;
void Cbuf_ExecuteLevel(int level);
//executes only a single cbuf level. can be used to restrict cbuf execution to some 'safe' set of commands, so there are no surprise 'map' commands.
//will not magically make all commands safe to exec, but will prevent user commands slipping in too.

//===========================================================================

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

*/

typedef void (*xcommand_t) (void);
struct xcommandargcompletioncb_s
{
	//if repl is specified, then that is the text that will be used if this is the sole autocomplete, to complete using strings that are not actually valid.
	void(*cb)(const char *arg, const char *desc, const char *repl, struct xcommandargcompletioncb_s *ctx);
	//private stuff follows.
};
typedef void (*xcommandargcompletion_t)(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx);

int Cmd_Level(const char *name);
void Cmd_EnumerateLevel(int level, char *buf, size_t bufsize);

void	Cmd_Init (void);
void	Cmd_Shutdown(void);
void	Cmd_StuffCmds (void);

void	Cmd_RemoveCommands (xcommand_t function);	//unregister all commands that use the same function. for wrappers and stuff.
void	Cmd_RemoveCommand (const char *cmd_name);
qboolean	Cmd_AddCommand (const char *cmd_name, xcommand_t function);
qboolean	Cmd_AddCommandD (const char *cmd_name, xcommand_t function, const char *description);
qboolean	Cmd_AddCommandAD (const char *cmd_name, xcommand_t function, xcommandargcompletion_t argcomplete, const char *description);
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_stringcmd instead of executed locally

qboolean Cmd_Exists (const char *cmd_name);
char *Cmd_AliasExist(const char *name, int restrictionlevel);
// used by the cvar code to check for cvar / command name overlap

const char *Cmd_Describe (const char *cmd_name);

typedef struct
{
	char *guessed;	//this is the COMPLETED partial.
	char *partial;	//the requested string that we completed
	qboolean caseinsens;
	size_t num, extra;	//valid count, and ommitted count (if we were too lazy to find more)
	struct cmd_completion_opt_s {
		qboolean text_alloced:1;
		qboolean desc_alloced:1;
		const char *text;
		const char *repl;	//used for sole matches
		const char *desc;
	} completions[50];
} cmd_completion_t;
cmd_completion_t *Cmd_Complete(const char *partial, qboolean caseinsens);	//calculates and caches info.

//these should probably be removed some time
char *Cmd_CompleteCommand (const char *partial, qboolean fullonly, qboolean caseinsens, int matchnum, const char **descptr);
qboolean Cmd_IsCommand (const char *line);
// attempts to match a partial command for automatic command line completion
// returns NULL if nothing fits

int		VARGS Cmd_Argc (void);
char	*VARGS Cmd_Argv (int arg);
char	*VARGS Cmd_Args (void);
extern int	Cmd_ExecLevel;

//if checkheader is false, an opening { is expected to already have been parsed.
//otherwise returns the contents of the block much like c.
//returns a zoned string.
char *Cmd_ParseMultiline(qboolean checkheader);

extern cvar_t cmd_gamecodelevel, cmd_allowaccess;
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are always safe.

int Cmd_CheckParm (const char *parm);
// Returns the position (1 to argc-1) in the command's argument list
// where the given parameter apears, or 0 if not present

char *Cmd_AliasExist(const char *name, int restrictionlevel);
void Alias_WipeStuffedAliases(void);

void Cmd_AddMacro(char *s, char *(*f)(void), int disputableintentions);
#define Cmd_AddMacroD(s,f,unsafe,desc) Cmd_AddMacro(s,f,unsafe)

void Cmd_TokenizePunctation (char *text, char *punctuation);
const char *Cmd_TokenizeString (const char *text, qboolean expandmacros, qboolean qctokenize);
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.

void	Cmd_ExecuteString (const char *text, int restrictionlevel);

void Cmd_Args_Set(const char *newargs, size_t len);

//higher levels have greater access, BUT BE SURE TO CHECK Cmd_IsInsecure()
#define RESTRICT_MAX_TOTAL  31
#define RESTRICT_MAX_USER	29
#define RESTRICT_DEFAULT	20
#define RESTRICT_MIN		1
#define RESTRICT_TEAMPLAY	0	//this is blocked from everything but aliases.

#define RESTRICT_MAX RESTRICT_MAX_USER

#define RESTRICT_LOCAL		(RESTRICT_MAX)		//commands typed at the console
#define RESTRICT_INSECURE	(RESTRICT_MAX+1)	//commands from csqc or untrusted sources (really should be a separate flag, requires cbuf rewrite)
#define RESTRICT_SERVER		(RESTRICT_MAX+2)		//commands from ssqc (untrusted, but allowed to lock cvars)
#define RESTRICT_SERVERSEAT(x) (RESTRICT_SERVER+x)
#define RESTRICT_RCON	rcon_level.ival
//#define RESTRICT_SSQC	RESTRICT_MAX-2

#define Cmd_FromGamecode() (Cmd_ExecLevel>=RESTRICT_SERVER)	//cheat provention. block cheats if its not fromgamecode
#define Cmd_IsInsecure() (Cmd_ExecLevel>=RESTRICT_INSECURE)	//prevention from the server from breaking/crashing/wiping us. if this returns true, block file access etc.

// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console

void	Cmd_ForwardToServer (void);
// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

qboolean Cmd_FilterMessage (char *message, qboolean sameteam);
void Cmd_MessageTrigger (char *message, int type);

void Cmd_ShiftArgs (int ammount, qboolean expandstring);

const char *TP_ParseFunChars (const char *s);
char *Cmd_ExpandString (const char *data, char *dest, int destlen, int *accesslevel, qboolean expandargs, qboolean expandcvars, qboolean expandmacros);
qboolean If_EvaluateBoolean(const char *text, int restriction);

extern cvar_t rcon_level;

void Cmd_AddTimer(float delay, void(*callback)(int iarg, void *data), int iarg, void *data, size_t datasize); //wrong place, gah

