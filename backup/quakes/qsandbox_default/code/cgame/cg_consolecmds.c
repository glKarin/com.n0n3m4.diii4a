/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "../qcommon/ns_local.h"

void CG_PrintClientNumbers( void ) {
    int i;

    CG_Printf( "slot score ping name\n" );
    CG_Printf( "---- ----- ---- ----\n" );

    for(i=0;i<cg.numScores;i++) {
        CG_Printf("%-4d",cg.scores[i].client);

        CG_Printf(" %-5d",cg.scores[i].score);

        CG_Printf(" %-4d",cg.scores[i].ping);

        CG_Printf(" %s\n",cgs.clientinfo[cg.scores[i].client].name);
    }
}

void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendConsoleCommand( va( "gc %i %i", targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
		(int)cg.refdefViewAngles[YAW]);
}


static void CG_ScoresDown_f( void ) {
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_Echoo_f( void ) {
	char	message[128];

	trap_Args( message, 128 );
	CG_Printf( "| %s\n", message);
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

/*
============
CG_NS_OpenScript_f
Opens Noire.Script file
============
*/
void CG_NS_OpenScript_f( void )
{
	char   filename[64];
	if( trap_Argc( ) == 1 ){
		CG_Printf( "usage: ns_openscript_cl <filename>\n" );
		return;
	}
  
	trap_Argv( 1, filename, sizeof( filename ) );
  
	NS_OpenScript(filename, NULL, 0);

}

/*
============
CG_NS_Interpret_f
Show Noire.Script variables
============
*/
void CG_NS_Interpret_f( void )
{
	char	code[2048];

	trap_Args( code, 2048 );
  
	Interpret(code);

}

/*
============
CG_NS_VariableList_f
Show Noire.Script variables
============
*/
void CG_NS_VariableList_f( void )
{
  
	print_variables();

}

/*
============
CG_NS_ThreadList_f
Show Noire.Script threads
============
*/
void CG_NS_ThreadList_f( void )
{
  
	print_threads();

}

/*
============
CG_NS_SendVariable_f
Opens Noire.Script file
============
*/
void CG_NS_SendVariable_f( void )
{
	char   varName[MAX_VAR_NAME];
	char   varValue[MAX_VAR_CHAR_BUF];
	char   varType[8];
  
	trap_Argv( 1, varName, sizeof( varName ) );
	trap_Argv( 2, varValue, sizeof( varValue ) );
	trap_Argv( 3, varType, sizeof( varType ) );
  
  	if(!variable_exists(varName)){
		create_variable(varName, varValue, atoi(varType));
	}

	set_variable_value(varName, varValue, atoi(varType));
}

/*
============
CG_ReplaceTexture_f
Replace texture
============
*/
void CG_ReplaceTexture_f( void ){
	char   oldtexture[1024];
	char   newtexture[1024];
  
  trap_Argv( 1, oldtexture, sizeof(oldtexture));
  trap_Argv( 2, newtexture, sizeof(newtexture));

  trap_R_RemapShader( oldtexture, newtexture, "0.005" );
}


typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "cloadmap", CG_CloadMap_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tell_target", CG_TellTarget_f },
	{ "echoo", CG_Echoo_f },
	{ "changetexture", CG_ReplaceTexture_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "tcmd", CG_TargetCommand_f },
  	//Noire.Script
  	{ "ns_openscript_cl", CG_NS_OpenScript_f },
  	{ "ns_interpret_cl", CG_NS_Interpret_f },
  	{ "ns_variablelist_cl", CG_NS_VariableList_f },
  	{ "ns_threadlist_cl", CG_NS_ThreadList_f },
  	{ "ns_sendvariable_cl", CG_NS_SendVariable_f },
    { "clients", CG_PrintClientNumbers }
};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand ("kill");
	trap_AddCommand ("say");
	trap_AddCommand ("say_team");
	trap_AddCommand ("tell");
	trap_AddCommand ("vsay");
	trap_AddCommand ("echoo");
	trap_AddCommand ("vsay_team");
	trap_AddCommand ("vtell");
	trap_AddCommand ("vosay");
	trap_AddCommand ("vosay_team");
	trap_AddCommand ("votell");
	trap_AddCommand ("give");
	trap_AddCommand ("god");
	trap_AddCommand ("notarget");
	trap_AddCommand ("noclip");
	trap_AddCommand ("team");
	trap_AddCommand ("follow");
	trap_AddCommand ("levelshot");
	trap_AddCommand ("addbot");
	trap_AddCommand ("setviewpos");
	trap_AddCommand ("callvote");
	trap_AddCommand ("getmappage");
	trap_AddCommand ("vote");
	trap_AddCommand ("callteamvote");
	trap_AddCommand ("teamvote");
	trap_AddCommand ("stats");
	trap_AddCommand ("teamtask");
	trap_AddCommand ("replacetexture");
	trap_AddCommand ("picktarget");
	trap_AddCommand ("usetarget");
	trap_AddCommand ("random");
	trap_AddCommand ("savemap");
	trap_AddCommand ("savemapall");
	trap_AddCommand ("loadmap");
	trap_AddCommand ("loadmapall");
	trap_AddCommand ("save_menu");
	trap_AddCommand ("load_menu");
	//Noire.Script
	trap_AddCommand ("ns_openscript");
	trap_AddCommand ("ns_interpret");
	trap_AddCommand ("ns_variablelist");
	trap_AddCommand ("ns_threadlist");
	trap_AddCommand ("ns_sendvariable");

	trap_AddCommand ("ns_openscript_cl");
	trap_AddCommand ("ns_interpret_cl");
	trap_AddCommand ("ns_variablelist_cl");
	trap_AddCommand ("ns_threadlist_cl");
	trap_AddCommand ("ns_sendvariable_cl");

	trap_AddCommand ("ns_openscript_ui");
	trap_AddCommand ("ns_interpret_ui");
	trap_AddCommand ("ns_variablelist_ui");
	trap_AddCommand ("ns_threadlist_ui");
	trap_AddCommand ("ns_sendvariable_ui");
}
