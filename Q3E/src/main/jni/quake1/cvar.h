/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2006-2021 DarkPlaces contributors

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

#ifndef CVAR_H
#define CVAR_H

#include "qtypes.h"
#include "qdefs.h"
struct cmd_state_s;
struct qfile_s;

typedef struct cvar_s
{
	unsigned flags;

	const char *name;

	const char *string;
	const char *description;
	int integer;
	float value;
	float vector[3];

	const char *defstring;

	void (*callback)(struct cvar_s *var);

	char **aliases;
	int aliases_size;

	// this is sufficient for Cvar_RestoreInitState()
	const char *initstring;

	int globaldefindex[3];
	int globaldefindex_stringno[3];

	struct cvar_s *next;
} cvar_t;

typedef struct cvar_hash_s
{
	cvar_t *cvar;
	struct cvar_hash_s *next;
} cvar_hash_t;

typedef struct cvar_state_s
{
	cvar_t *vars;
	cvar_hash_t *hashtable[CVAR_HASHSIZE];
}
cvar_state_t;

extern cvar_state_t cvars_all;
extern cvar_state_t cvars_null; // used by cmd_serverfromclient which intentionally has no cvars available

void Cvar_RegisterVirtual(cvar_t *variable, const char *name );

void Cvar_RegisterCallback(cvar_t *variable, void (*callback)(cvar_t *));

/// registers a cvar that already has the name, string, and optionally the
/// archive elements set.
void Cvar_RegisterVariable(cvar_t *variable);

qbool Cvar_Readonly (cvar_t *var, const char *cmd_name);

void Cvar_Callback(cvar_t *var);

/// equivelant to "<name> <variable>" typed at the console
void Cvar_Set (cvar_state_t *cvars, const char *var_name, const char *value);

/// expands value to a string and calls Cvar_Set
void Cvar_SetValue (cvar_state_t *cvars, const char *var_name, float value);

void Cvar_SetQuick (cvar_t *var, const char *value);
void Cvar_SetValueQuick (cvar_t *var, float value);

float Cvar_VariableValueOr (cvar_state_t *cvars, const char *var_name, float def, unsigned neededflags);
// returns def if not defined

float Cvar_VariableValue (cvar_state_t *cvars, const char *var_name, unsigned neededflags);
// returns 0 if not defined or non numeric

const char *Cvar_VariableStringOr (cvar_state_t *cvars, const char *var_name, const char *def, unsigned neededflags);
// returns def if not defined

const char *Cvar_VariableString (cvar_state_t *cvars, const char *var_name, unsigned neededflags);
// returns an empty string if not defined

const char *Cvar_VariableDefString (cvar_state_t *cvars, const char *var_name, unsigned neededflags);
// returns an empty string if not defined

const char *Cvar_VariableDescription (cvar_state_t *cvars, const char *var_name, unsigned neededflags);
// returns an empty string if not defined

const char *Cvar_CompleteVariable (cvar_state_t *cvars, const char *partial, unsigned neededflags);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

void Cvar_PrintHelp(cvar_t *cvar, const char *name, qbool full);

void Cvar_CompleteCvarPrint (cvar_state_t *cvars, const char *partial, unsigned neededflags);

qbool Cvar_Command (struct cmd_state_s *cmd);
// called by Cmd_ExecuteString when Cmd_Argv(cmd, 0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void Cvar_SaveInitState(cvar_state_t *cvars);
void Cvar_RestoreInitState(cvar_state_t *cvars);

void Cvar_UnlockDefaults(struct cmd_state_s *cmd);
void Cvar_LockDefaults_f(struct cmd_state_s *cmd);
void Cvar_ResetToDefaults_All_f(struct cmd_state_s *cmd);
void Cvar_ResetToDefaults_NoSaveOnly_f(struct cmd_state_s *cmd);
void Cvar_ResetToDefaults_SaveOnly_f(struct cmd_state_s *cmd);

void Cvar_WriteVariables (cvar_state_t *cvars, struct qfile_s *f);
// Writes lines containing "set variable value" for all variables
// with the archive flag set to true.

cvar_t *Cvar_FindVar(cvar_state_t *cvars, const char *var_name, unsigned neededflags);
cvar_t *Cvar_FindVarAfter(cvar_state_t *cvars, const char *prev_var_name, unsigned neededflags);

int Cvar_CompleteCountPossible(cvar_state_t *cvars, const char *partial, unsigned neededflags);
const char **Cvar_CompleteBuildList(cvar_state_t *cvars, const char *partial, unsigned neededflags);
// Added by EvilTypeGuy - functions for tab completion system
// Thanks to Fett erich@heintz.com
// Thanks to taniwha

/// Prints a list of Cvars including a count of them to the user console
/// Referenced in cmd.c in Cmd_Init hence it's inclusion here.
/// Added by EvilTypeGuy eviltypeguy@qeradiant.com
/// Thanks to Matthias "Maddes" Buecher, http://www.inside3d.com/qip/
void Cvar_List_f(struct cmd_state_s *cmd);

void Cvar_Set_f(struct cmd_state_s *cmd);
void Cvar_SetA_f(struct cmd_state_s *cmd);
void Cvar_Del_f(struct cmd_state_s *cmd);
// commands to create new cvars (or set existing ones)
// seta creates an archived cvar (saved to config)

/// allocates a cvar by name and returns its address,
/// or merely sets its value if it already exists.
cvar_t *Cvar_Get(cvar_state_t *cvars, const char *name, const char *value, unsigned flags, const char *newdescription);

extern const char *cvar_dummy_description; // ALWAYS the same pointer

void Cvar_UpdateAllAutoCvars(cvar_state_t *cvars); // updates ALL autocvars of the active prog to the cvar values (savegame loading)

#ifdef FILLALLCVARSWITHRUBBISH
void Cvar_FillAll_f(cmd_state_t *cmd);
#endif /* FILLALLCVARSWITHRUBBISH */

#endif

