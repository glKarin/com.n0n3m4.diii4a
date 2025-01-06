//KK-OAX
/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of the Open Arena source code.
Copied from Tremulous under GPL version 2 including any later version.

Open Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Open Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Open Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../qcommon/ns_local.h"

/*
============
Svcmd_status_f
Does Server Status from Console
============
*/
void Svcmd_Status_f( void )
{
    int       i;
    gclient_t *cl;
    char      userinfo[ MAX_INFO_STRING ];

    G_Printf( "slot score ping address               rate     name\n" );
    G_Printf( "---- ----- ---- -------               ----     ----\n" );
    for( i = 0, cl = level.clients; i < level.maxclients; i++, cl++ )
    {
        if( cl->pers.connected == CON_DISCONNECTED )
            continue;

        G_Printf( "%-4d ", i );
        G_Printf( "%-5d ", cl->ps.persistant[ PERS_SCORE ] );

        if( cl->pers.connected == CON_CONNECTING )
            G_Printf( "CNCT " );
        else
            G_Printf( "%-4d ", cl->ps.ping );

        trap_GetUserinfo( i, userinfo, sizeof( userinfo ) );
        G_Printf( "%-21s ", Info_ValueForKey( userinfo, "ip" ) );
        G_Printf( "%-8d ", Info_ValueForKey( userinfo, "rate" ) );
        G_Printf( "%s\n", cl->pers.netname ); // Info_ValueForKey( userinfo, "name" )
    }
}

/*
============
Svcmd_TeamMessage_f
Sends a Chat Message to a Team from the Console
============
*/
void Svcmd_TeamMessage_f( void )
{
  char   teamNum[ 2 ];
  const char*   prefix;
  team_t team;

  if( trap_Argc( ) < 3 )
  {
    G_Printf( "usage: say_team <team> <message>\n" );
    return;
  }

  trap_Argv( 1, teamNum, sizeof( teamNum ) );
  team = G_TeamFromString( teamNum );

  if( team == TEAM_NUM_TEAMS )
  {
    G_Printf( "say_team: invalid team \"%s\"\n", teamNum );
    return;
  }

  prefix = BG_TeamName( team );
  prefix = va( "[%c] ", toupper( *prefix ) );

  G_TeamCommand( team, va( "tchat \"(console): " S_COLOR_CYAN "%s\"", ConcatArgs( 2 ) ) );
  G_LogPrintf( "sayteam: %sconsole: " S_COLOR_CYAN "%s\n", prefix, ConcatArgs( 2 ) );
}

/*
============
Svcmd_CenterPrint_f
Does a CenterPrint from the Console
============
*/
void Svcmd_CenterPrint_f( void )
{
  if( trap_Argc( ) < 2 )
  {
    G_Printf( "usage: cp <message>\n" );
    return;
  }

  trap_SendServerCommand( -1, va( "cp \"%s\"", ConcatArgs( 1 ) ) );
}
/*
============
Svcmd_ReplaceTexture_f
Replace texture
============
*/
void Svcmd_ReplaceTexture_f( void )
{
	char   oldtexture[1024];
	char   newtexture[1024];
  if( trap_Argc( ) == 1 ){
    G_Printf( "usage: replacetexture <oldtexture> <newtexture>\n" );
  return;}
  
  trap_Argv( 1, oldtexture, sizeof( oldtexture ) );
  trap_Argv( 2, newtexture, sizeof( newtexture ) );

  AddRemap(va( "%s", oldtexture), va( "%s", newtexture), level.time * 0.005); 
  trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
}

/*
==================
Svcmd_PropNpc_AS_f
Added for QSandbox.
==================
*/
void Svcmd_PropNpc_AS_f( void ){
	vec3_t		end;
	gentity_t 	*tent;
	char		cord_x[64];
	char		cord_y[64];
	char		cord_z[64];
	char		arg01[64];
	char		arg02[64];
	char		arg03[64];
	char		arg04[64];
	char		arg05[64];
	char		arg06[64];
	char		arg07[64];
	char		arg08[64];
	char		arg09[64];
	char		arg10[64];
	char		arg11[64];
	char		arg12[64];
	char		arg13[64];
	char		arg14[64];
	char		arg15[64];
	char		arg16[64];
	char		arg17[64];
	char		arg18[64];
	char		arg19[64];
	char		arg20[64];
	char		arg21[64];
	char		arg22[64];
	char		arg23[64];
	
	if(g_gametype.integer != GT_SANDBOX){ return; }
		
	//tr.endpos
	trap_Argv( 1, cord_x, sizeof( cord_x ) );
	trap_Argv( 2, cord_y, sizeof( cord_y ) );
	trap_Argv( 3, cord_z, sizeof( cord_z ) );
	trap_Argv( 4, arg01, sizeof( arg01 ) );
	trap_Argv( 5, arg02, sizeof( arg02 ) );
	trap_Argv( 6, arg03, sizeof( arg03 ) );
	trap_Argv( 7, arg04, sizeof( arg04 ) );
	trap_Argv( 8, arg05, sizeof( arg05 ) );
	trap_Argv( 9, arg06, sizeof( arg06 ) );
	trap_Argv( 10, arg07, sizeof( arg07 ) );
	trap_Argv( 11, arg08, sizeof( arg08 ) );
	trap_Argv( 12, arg09, sizeof( arg09 ) );
	trap_Argv( 13, arg10, sizeof( arg10 ) );
	trap_Argv( 14, arg11, sizeof( arg11 ) );
	trap_Argv( 15, arg12, sizeof( arg12 ) );
	trap_Argv( 16, arg13, sizeof( arg13 ) );
	trap_Argv( 17, arg14, sizeof( arg14 ) );
	trap_Argv( 18, arg15, sizeof( arg15 ) );
	trap_Argv( 19, arg16, sizeof( arg16 ) );
	trap_Argv( 20, arg17, sizeof( arg17 ) );
	trap_Argv( 21, arg18, sizeof( arg18 ) );
	trap_Argv( 22, arg19, sizeof( arg19 ) );
	trap_Argv( 23, arg20, sizeof( arg20 ) );
	trap_Argv( 24, arg21, sizeof( arg21 ) );
	trap_Argv( 25, arg22, sizeof( arg22 ) );
	trap_Argv( 26, arg23, sizeof( arg23 ) );
	
	end[0] = atof(cord_x);
	end[1] = atof(cord_y);
	end[2] = atof(cord_z);
	
	if(!Q_stricmp (arg01, "prop")){
	if(!g_allowprops.integer){ return; }
	if(g_safe.integer){
	if(!Q_stricmp (arg03, "script_cmd")){
	return;
	}
	if(!Q_stricmp (arg03, "target_modify")){
	return;
	}
	}
	G_BuildPropSL( arg02, arg03, end, level.player, arg04, arg05, arg06, arg07, arg08, arg09, arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19, arg20, arg21, arg22, arg23);
	
	
	return;
	}
	if(!Q_stricmp (arg01, "npc")){
	if(!g_allownpc.integer){ return; }
	
	tent = G_Spawn();
	tent->sb_ettype = 1;
	VectorCopy( end, tent->s.origin);
	tent->s.origin[2] += 25;
	tent->classname = "target_botspawn";
	CopyAlloc(tent->clientname, arg02);
	tent->type = NPC_ENEMY;
	if(!Q_stricmp (arg03, "NPC_Enemy")){
	tent->type = NPC_ENEMY;
	}
	if(!Q_stricmp (arg03, "NPC_Citizen")){
	tent->type = NPC_CITIZEN;
	}
	if(!Q_stricmp (arg03, "NPC_Guard")){
	tent->type = NPC_GUARD;
	}
	if(!Q_stricmp (arg03, "NPC_Partner")){
	tent->type = NPC_PARTNER;
	}
	if(!Q_stricmp (arg03, "NPC_PartnerEnemy")){
	tent->type = NPC_PARTNERENEMY;
	}
	tent->skill = atof(arg04);
	tent->health = atoi(arg05);
	CopyAlloc(tent->message, arg06);	
	tent->mtype = atoi(arg08);
	if(!Q_stricmp (arg07, "0") ){
	CopyAlloc(tent->target, arg02);	
	} else {
	CopyAlloc(tent->target, arg07);	
	}
	if(tent->health <= 0){
	tent->health = 100;
	}
	if(tent->skill <= 0){
	tent->skill = 1;
	}
	if(!Q_stricmp (tent->message, "0") || !tent->message ){
	CopyAlloc(tent->message, tent->clientname);
	}
	G_AddBot(tent->clientname, tent->skill, "Blue", 0, tent->message, tent->s.number, tent->target, tent->type, tent );
	
	trap_Cvar_Set("g_spSkill", arg04);
	return;
	}
}

/*
============
Svcmd_BannerPrint_f
Does a BannerPrint from the Console
KK-OAX Commented out in g_svccmds.c, so right now it's useless.
============
*/
void Svcmd_BannerPrint_f( void )
{
  if( trap_Argc( ) < 2 )
  {
    G_Printf( "usage: bp <message>\n" );
    return;
  }

  trap_SendServerCommand( -1, va( "bp \"%s\"", ConcatArgs( 1 ) ) );
}
/*
============
Svcmd_EjectClient_f
Kicks a Client from Console
KK-OAX, I'm pretty sure this is also done in the "server" portion 
of the engine code with "kick," but oh well. 
============
*/
void Svcmd_EjectClient_f( void )
{
  char *reason, name[ MAX_STRING_CHARS ];

  if( trap_Argc( ) < 2 )
  {
    G_Printf( "usage: eject <player|-1> <reason>\n" );
    return;
  }

  trap_Argv( 1, name, sizeof( name ) );
  reason = ConcatArgs( 2 );

  if( atoi( name ) == -1 )
  {
    int i;
    for( i = 0; i < level.maxclients; i++ )
    {
      if( level.clients[ i ].pers.connected == CON_DISCONNECTED )
        continue;
      if( level.clients[ i ].pers.localClient )
        continue;
      trap_DropClient( i, reason );
    }
  }
  else
  {
    gclient_t *cl = ClientForString( name );
    if( !cl )
      return;
    if( cl->pers.localClient )
    {
      G_Printf( "eject: cannot eject local clients\n" );
      return;
    }
    trap_DropClient( cl-level.clients, reason );
  }
}

/*
============
Svcmd_DumpUser_f
Shows User Info
============
*/
void Svcmd_DumpUser_f( void )
{
  char name[ MAX_STRING_CHARS ], userinfo[ MAX_INFO_STRING ];
  char key[ BIG_INFO_KEY ], value[ BIG_INFO_VALUE ];
  const char *info;
  gclient_t *cl;

  if( trap_Argc( ) != 2 )
  {
    G_Printf( "usage: dumpuser <player>\n" );
    return;
  }

  trap_Argv( 1, name, sizeof( name ) );
  cl = ClientForString( name );
  if( !cl )
    return;

  trap_GetUserinfo( cl-level.clients, userinfo, sizeof( userinfo ) );
  info = &userinfo[ 0 ];
  G_Printf( "userinfo\n--------\n" );
  //Info_Print( userinfo );
  while( 1 )
  {
    Info_NextPair( &info, key, value );
    if( !*info )
      return;

    G_Printf( "%-20s%s\n", key, value );
  }
}

void Svcmd_Chat_f( void )
{
    trap_SendServerCommand( -1, va( "chat \"%s\"", ConcatArgs( 1 ) ) );
    G_LogPrintf("chat: %s\n", ConcatArgs( 1 ) );
}

/*
=============
Svcmd_ListIP_f
Dumb Wrapper for the trap_Send command
=============
*/
void Svcmd_ListIP_f( void )
{
    trap_SendConsoleCommand( EXEC_NOW, "g_banIPs\n" );
}

/*
=============
Svcmd_MessageWrapper
Dumb wrapper for "a" and "m" and "say"
=============
*/
void Svcmd_MessageWrapper( void )
{
  char cmd[ 5 ];
  trap_Argv( 0, cmd, sizeof( cmd ) );
  if( !Q_stricmp( cmd, "say" ) )
    G_Say( NULL, NULL, SAY_ALL, ConcatArgs( 1 ) );
}

/*
============
Svcmd_NS_OpenScript_f
Opens Noire.Script file
============
*/
void Svcmd_NS_OpenScript_f( void )
{
	char   filename[64];
	if( trap_Argc( ) == 1 ){
		G_Printf( "usage: ns_openscript <filename>\n" );
		return;
	}
  
	trap_Argv( 1, filename, sizeof( filename ) );
  
	NS_OpenScript(filename, NULL, 0);

}

/*
============
Svcmd_NS_Interpret_f
Show Noire.Script variables
============
*/
void Svcmd_NS_Interpret_f( void )
{
	if( trap_Argc( ) == 1 ){
		G_Printf( "usage: ns_interpret <code>\n" );
		return;
	}
  
	Interpret(ConcatArgs( 1 ));

}

/*
============
Svcmd_NS_VariableList_f
Show Noire.Script variables
============
*/
void Svcmd_NS_VariableList_f( void )
{
  
	print_variables();

}

/*
============
Svcmd_NS_ThreadList_f
Show Noire.Script threads
============
*/
void Svcmd_NS_ThreadList_f( void )
{
  
	print_threads();

}

/*
============
Svcmd_NS_SendVariable_f
Opens Noire.Script file
============
*/
void Svcmd_NS_SendVariable_f( void )
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
