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
// cvar.h

/*

cvar_t variables are used to hold scalar or string variables that can be changed or displayed at the console or prog code as well as accessed directly
in C code.

it is sufficient to initialize a cvar_t with just the first two fields, or
you can add a ,true flag for variables that you want saved to the configuration
file when the game is quit:

cvar_t	r_draworder = {"r_draworder","1"};
cvar_t	scr_screensize = {"screensize","1",true};

Cvars must be registered before use, or they will have a 0 value instead of the float interpretation of the string.  Generally, all cvar_t declarations should be registered in the apropriate init function before any console commands are executed:
Cvar_RegisterVariable (&host_framerate);


C code usually just references a cvar in place:
if ( r_draworder.value )

It could optionally ask for the value to be looked up for a string name:
if (Cvar_VariableValue ("r_draworder"))

Interpreted prog code can access cvars with the cvar(name) or
cvar_set (name, value) internal functions:
teamplay = cvar("teamplay");
cvar_set ("registered", "1");

The user can access cvars from the console in two ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.
*/

#include "../qclib/hash.h"

typedef struct cvar_s
{
	//must match q2's definition
	char			*name;
	char			*string;
	char			*latched_string;	// for CVAR_LATCHMASK vars
	unsigned int	flags;
	qboolean		modified;
	float			value;
	struct cvar_s	*next;

	//free style :)
	char			*name2;

	void			(QDECL *callback) (struct cvar_s *var, char *oldvalue);
	const char		*description;
	char			*enginevalue;		//when changing manifest dir, the cvar will be reset to this value. never freed.
	char			*defaultstr;		//this is the current mod's default value. set on first update.
	qbyte			restriction;

	int				ival;
	vec4_t			vec4;	//0,0,0,1 if something didn't parse.
	int				modifiedcount;

#ifdef HLSERVER
	struct hlcvar_s	*hlcvar;
#endif
} cvar_t;

#ifdef MINIMAL
#define CVARAFCD(ConsoleName,Value,ConsoleName2,Flags,Callback,Description)	{ConsoleName, NULL, NULL, Flags, 0, 0, 0, ConsoleName2, Callback, NULL,        Value}
#else
#define CVARAFCD(ConsoleName,Value,ConsoleName2,Flags,Callback,Description)	{ConsoleName, NULL, NULL, Flags, 0, 0, 0, ConsoleName2, Callback, Description, Value}
#endif
#define CVARAFD(ConsoleName,Value,ConsoleName2,Flags,Description)CVARAFCD(ConsoleName, Value, ConsoleName2, Flags, NULL, Description)
#define CVARAFC(ConsoleName,Value,ConsoleName2,Flags,Callback)	CVARAFCD(ConsoleName, Value, ConsoleName2, Flags, Callback, NULL)
#define CVARAF(ConsoleName,Value,ConsoleName2,Flags)			CVARAFCD(ConsoleName, Value, ConsoleName2, Flags, NULL, NULL)
#define CVARFCD(ConsoleName,Value,Flags,Callback,Description)	CVARAFCD(ConsoleName, Value, NULL, Flags, Callback, Description)
#define CVARFC(ConsoleName,Value,Flags,Callback)				CVARAFCD(ConsoleName, Value, NULL, Flags, Callback, NULL)
#define CVARAD(ConsoleName,Value,ConsoleName2,Description)		CVARAFCD(ConsoleName, Value, ConsoleName2, 0, NULL, Description)
#define CVARFD(ConsoleName,Value,Flags,Description)				CVARAFCD(ConsoleName, Value, NULL, Flags, NULL, Description)
#define CVARF(ConsoleName,Value,Flags)							CVARFC(ConsoleName, Value, Flags, NULL)
#define CVARC(ConsoleName,Value,Callback)						CVARFC(ConsoleName, Value, 0, Callback)
#define CVARCD(ConsoleName,Value,Callback,Description)			CVARAFCD(ConsoleName, Value, NULL, 0, Callback, Description)
#define CVARD(ConsoleName,Value,Description)					CVARAFCD(ConsoleName, Value, NULL, 0, NULL, Description)
#define CVAR(ConsoleName,Value)									CVARD(ConsoleName, Value, NULL)

typedef struct cvar_group_s
{
	const char *name;
	struct cvar_group_s *next;

	cvar_t *cvars;
} cvar_group_t;

//q2 constants
#define	CVAR_ARCHIVE		(1<<0)	// set to cause it to be saved to vars.rc
#define	CVAR_USERINFO		(1<<1)	// added to userinfo  when changed
#define	CVAR_SERVERINFO		(1<<2)	// added to serverinfo when changed
#define	CVAR_NOSET		(1<<3)	// don't allow change from console at all,
							// but can be set from the command line
#define	CVAR_MAPLATCH		(1<<4)	// save changes until server restart, to avoid q2 gamecode bugging out.

//freestyle
#define CVAR_POINTER		(1<<5)	// q2 style. May be converted to q1 if needed. These are often specified on the command line and then converted into q1 when registered properly.
//#define CVAR_UNUSED			(1<<6)  //the default string was malloced/needs to be malloced, free on unregister
#define CVAR_NOTFROMSERVER	(1<<7)	//cvar cannot be set by gamecode. the console will ignore changes to cvars if set at from the server or any gamecode. This is to protect against security flaws - like qterm
#define CVAR_USERCREATED	(1<<8)	//write a 'set' or 'seta' in front of the var name.
#define CVAR_CHEAT			(1<<9)	//latch to the default, unless cheats are enabled.
#define CVAR_SEMICHEAT		(1<<10)	//if strict ruleset, force to blank (aka 0).
#define CVAR_RENDERERLATCH	(1<<11)	//requires a vid_reload to reapply.
#define CVAR_SERVEROVERRIDE	(1<<12)	//the server has overridden out local value - should probably be called SERVERLATCH
#define CVAR_RENDERERCALLBACK	(1<<13) //force callback for cvars on renderer change
#define CVAR_NOUNSAFEEXPAND	(1<<14) //cvar cannot be read by gamecode. do not expand cvar value when command is from gamecode.
#define CVAR_RULESETLATCH	(1<<15)	//latched by the ruleset
#define CVAR_SHADERSYSTEM	(1<<16)	//change flushes shaders.
#define CVAR_TELLGAMECODE	(1<<17) //tells the gamecode when it has changed, does not prevent changing, added as an optimisation

#define CVAR_CONFIGDEFAULT	(1<<18)	//this cvar's default value has been changed to match a config.
#define CVAR_NOSAVE			(1<<19) //this cvar should never be saved. ever.
#define CVAR_NORESET		(1<<20) //cvar is not reset by various things.
#define CVAR_TEAMPLAYTAINT	(1<<21)	//current value contains the evaluation of a teamplay macro.

#define CVAR_WATCHED		(1<<22)	//report any attempts to change this cvar.
#define CVAR_VIDEOLATCH		(1<<23)
#define CVAR_WARNONCHANGE	(1<<24)	//print a warning when changed to a value other than its default.
#define CVAR_RENDEREROVERRIDE	(1<<25)	//the renderer has forced the cvar to indicate that only that value is supported

#define CVAR_LASTFLAG CVAR_VIDEOLATCH

#define CVAR_LATCHMASK		(CVAR_MAPLATCH|CVAR_RENDERERLATCH|CVAR_VIDEOLATCH|CVAR_SERVEROVERRIDE|CVAR_CHEAT|CVAR_SEMICHEAT)	//you're only allowed one of these.
#define CVAR_NEEDDEFAULT	CVAR_CHEAT

//an alias
#define CVAR_SAVE CVAR_ARCHIVE

cvar_t *Cvar_Get2 (const char *var_name, const char *value, int flags, const char *description, const char *groupname);
#define Cvar_Get(n,v,f,g) Cvar_Get2(n,v,f,NULL,g)

void Cvar_LockFromServer(cvar_t *var, const char *str);
void Cvar_LockUnsupportedRendererCvar(cvar_t *var, const char *str);

qboolean 	Cvar_Register (cvar_t *variable, const char *cvargroup);
// registers a cvar that already has the name, string, and optionally the
// archive elements set.

cvar_t	*Cvar_ForceSet (cvar_t *var, const char *value);
cvar_t 	*Cvar_Set (cvar_t *var, const char *value);
// equivelant to "<name> <variable>" typed at the console

void	Cvar_ForceSetValue (cvar_t *var, float value);
void	Cvar_SetValue (cvar_t *var, float value);
// expands value to a string and calls Cvar_Set

qboolean Cvar_ApplyLatchFlag(cvar_t *var, char *value, unsigned int newflag, unsigned int ignoreflags);

qboolean Cvar_UnsavedArchive(void);
void Cvar_Saved(void);
void Cvar_ConfigChanged(void);

extern int cvar_watched;	//so that cmd.c knows that it should add messages when configs are execed
void Cvar_ParseWatches(void);	//parse -watch args

int Cvar_ApplyLatches(int latchflag, qboolean clearflag);
//sets vars to their latched values (and optionally forgets the cvarflag in question)

void Cvar_Hook(cvar_t *cvar, void (QDECL *callback) (struct cvar_s *var, char *oldvalue));
//hook a cvar with a given callback function at runtime

void Cvar_Unhook(cvar_t *cvar);
//unhook a cvar

void Cvar_ForceCallback(cvar_t *cvar);
// force a cvar callback

void Cvar_ApplyCallbacks(int callbackflag);
//forces callbacks to be ran for given flags

void QDECL Cvar_Limiter_ZeroToOne_Callback(struct cvar_s *var, char *oldvalue);
//cvar callback to limit cvar value to 0 or 1

float	Cvar_VariableValue (const char *var_name);
// returns 0 if not defined or non numeric

char	*Cvar_VariableString (const char *var_name);
// returns an empty string if not defined

void Cvar_SetNamed (const char *var_name, const char *newvalue);

char 	*Cvar_CompleteVariable (const char *partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

qboolean Cvar_Command (cvar_t *v, int level);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void Cvar_WriteVariables (vfsfile_t *f, qboolean all, qboolean nohidden);
// Writes lines containing "set variable value" for all variables
// with the archive flag set to true.

cvar_t *Cvar_FindVar (const char *var_name);

void Cvar_SetEngineDefault(cvar_t *var, char *val);

void Cvar_Init(void);
void Cvar_Shutdown(void);

void Cvar_ForceCheatVars(qboolean semicheats, qboolean absolutecheats);	//locks/unlocks cheat cvars depending on weather we are allowed them.

//extern cvar_t	*cvar_vars;
