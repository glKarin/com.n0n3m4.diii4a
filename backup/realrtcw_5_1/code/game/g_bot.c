/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// g_bot.c

#include "g_local.h"
#include "../botlib/botai.h"


static int g_numBots;
static char g_botInfos[MAX_BOTS][MAX_INFO_STRING];


int g_numArenas;
static char *g_arenaInfos[MAX_ARENAS];


#define BOT_BEGIN_DELAY_BASE        2000
#define BOT_BEGIN_DELAY_INCREMENT   1500

#define BOT_SPAWN_QUEUE_DEPTH   16

typedef struct {
	int clientNum;
	int spawnTime;
} botSpawnQueue_t;

static int botBeginDelay;
static botSpawnQueue_t botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

vmCvar_t bot_minplayers;

extern gentity_t    *podium1;
extern gentity_t    *podium2;
extern gentity_t    *podium3;


/*
===============
G_ParseInfos
===============
*/
static int G_ParseInfos( char *buf, int max, char *infos[] ) {  
    char    *token;
    int count;
    char key[MAX_TOKEN_CHARS];
    char info[MAX_INFO_STRING];

    count = 0; 

    while ( 1 ) {  
        token = COM_Parse( &buf );
        if ( !token[0] ) {  
            break;
        }    
        if ( strcmp( token, "{" ) ) {  
            Com_Printf( "Missing { in info file\n" );
            break;
        }    

        if ( count == max ) {  
            Com_Printf( "Max infos exceeded\n" );
            break;
        }    

        info[0] = '\0';
        while ( 1 ) {  
            token = COM_ParseExt( &buf, qtrue );
            if ( !token[0] ) {  
                Com_Printf( "Unexpected end of info file\n" );
                break;
            }    
            if ( !strcmp( token, "}" ) ) {  
                break;
            }    
            Q_strncpyz( key, token, sizeof( key ) ); 

            token = COM_ParseExt( &buf, qfalse );
            if ( !token[0] ) {
                strcpy( token, "<NULL>" );
            }
            Info_SetValueForKey( info, key, token );
        }
        //NOTE: extra space for arena number
        infos[count] = G_Alloc( strlen( info ) + strlen( "\\num\\" ) + strlen( va( "%d", MAX_ARENAS ) ) + 1 );
        if ( infos[count] ) {
            strcpy( infos[count], info );
            count++;
        }
    }
    return count;
}

/*
===============
G_LoadArenasFromFile
===============
*/
static void G_LoadArenasFromFile( char *filename ) { 
    int len;
    fileHandle_t f;
    char buf[MAX_ARENAS_TEXT];

    len = trap_FS_FOpenFile( filename, &f, FS_READ );
    if ( !f ) { 
        trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
        return;
    }   
    if ( len >= MAX_ARENAS_TEXT ) { 
        trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_ARENAS_TEXT ) );
        trap_FS_FCloseFile( f );
        return;
    }   

    trap_FS_Read( buf, len, f );
    buf[len] = 0;
    trap_FS_FCloseFile( f );

    g_numArenas += G_ParseInfos( buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas] );
}

/*
===============
G_LoadArenas
===============
*/
void G_LoadArenas( void ) { 
    int numdirs;
    vmCvar_t arenasFile;
    char filename[128];
    char dirlist[1024];
    char*       dirptr;
    int i, n;
    int dirlen;
    char        *type;

    trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT | CVAR_ROM );
    if ( *arenasFile.string ) { 
        G_LoadArenasFromFile( arenasFile.string );
    } else {
        G_LoadArenasFromFile( "scripts/arenas.txt" );
    }   

    // get all arenas from .arena files
    numdirs = trap_FS_GetFileList( "scripts", ".arena", dirlist, 1024 );
    dirptr  = dirlist;
    for ( i = 0; i < numdirs; i++, dirptr += dirlen + 1 ) { 
        dirlen = strlen( dirptr );
        strcpy( filename, "scripts/" );
        strcat( filename, dirptr );
        G_LoadArenasFromFile( filename );
    }   
    G_DPrintf( "%i arenas parsed\n", g_numArenas );

	i = 0;
    for ( n = 0; n < g_numArenas; n++ ) { 

        type = Info_ValueForKey( g_arenaInfos[n], "type" );
        if ( *type ) { 
            if ( strstr( type, "sv_normal" ) && i < MAX_MAPS ) { 
				char *map = Info_ValueForKey( g_arenaInfos[n], "map" );
				level.maplist[i] = G_Alloc(strlen(map));
				strcpy(level.maplist[i++], map);
            }   
        }   
    }   
}

/*
===============
G_GetArenaInfoByNumber
===============
*/
const char *G_GetArenaInfoByMap( const char *map ) {
	int n;

	for ( n = 0; n < g_numArenas; n++ ) {
		if ( Q_stricmp( Info_ValueForKey( g_arenaInfos[n], "map" ), map ) == 0 ) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}


/*
=================
PlayerIntroSound
=================
*/
static void PlayerIntroSound( const char *modelAndSkin ) {
	char model[MAX_QPATH];
	char    *skin;

	Q_strncpyz( model, modelAndSkin, sizeof( model ) );
	skin = strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	} else {
		skin = model;
	}

	if ( Q_stricmp( skin, "default" ) == 0 ) {
		skin = model;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "play sound/player/announce/%s.wav\n", skin ) );
}

/*
===============
G_CountBotPlayersByName

Check connected and connecting (delay join) bots.

Returns number of bots with name on specified team or whole server if team is -1.
===============
*/
int G_CountBotPlayersByName( const char *name, int team ) {
	int			i, num;
	gclient_t   *cl;

	num = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		if ( name && Q_stricmp( name, cl->pers.netname ) ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_SelectRandomBotInfo

Get random least used bot info on team or whole server if team is -1.
===============
*/
int G_SelectRandomBotInfo( int team ) {
	int	selection[MAX_BOTS];
	int	n, num;
	int	count, bestCount;
	char	*value;

	// don't add duplicate bots to the server if there are less bots than bot types
	if ( team != -1 && G_CountBotPlayersByName( NULL, -1 ) < g_numBots ) {
		team = -1;
	}

	num = 0;
	bestCount = MAX_CLIENTS;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "funname" );
		if ( !value[0] ) {
			value = Info_ValueForKey( g_botInfos[n], "name" );
		}
		//
		count = G_CountBotPlayersByName( value, team );

		if ( count < bestCount ) {
			bestCount = count;
			num = 0;
		}

		if ( count == bestCount ) {
			selection[num++] = n;

			if ( num == MAX_BOTS ) {
				break;
			}
		}
	}

	if ( num > 0 ) {
		num = random() * ( num - 1 );
		return selection[num];
	}

	return -1;
}

/*
===============
G_AddRandomBot
===============
*/
void G_AddRandomBot( int team ) {
	char	*teamstr;
	int	skill;

	skill = trap_Cvar_VariableIntegerValue( "g_spSkill" );
	if ( team == TEAM_RED ) {
		teamstr = "red";
	} else if ( team == TEAM_BLUE ) {
		teamstr = "blue";
	} else {
		teamstr = "free";
	}
	trap_SendConsoleCommand( EXEC_INSERT, va( "addbot random %i %s %i\n", skill, teamstr, 0 ) );
}

/*
===============
G_RemoveRandomBot
===============
*/
int G_RemoveRandomBot( int team ) {
	int i;
	gclient_t   *cl;

	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		trap_SendConsoleCommand( EXEC_INSERT, va("clientkick %d\n", i) );
		return qtrue;
	}
	return qfalse;
}

/*
===============
G_CountHumanPlayers
===============
*/
int G_CountHumanPlayers( int team ) {
	int i, num;
	gclient_t   *cl;

	num = 0;
	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CountBotPlayers

Check connected and connecting (delay join) bots.
===============
*/
int G_CountBotPlayers( int team ) {
	int i, num;
	gclient_t   *cl;

	num = 0;
	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CheckMinimumPlayers
===============
*/
void G_CheckMinimumPlayers( void ) {
return;
}

/*
===============
G_CheckBotSpawn
===============
*/
void G_CheckBotSpawn( void ) {
	int n;
	char userinfo[MAX_INFO_VALUE];

	G_CheckMinimumPlayers();

	for ( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if ( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		ClientBegin( botSpawnQueue[n].clientNum );
		botSpawnQueue[n].spawnTime = 0;


		trap_GetUserinfo( botSpawnQueue[n].clientNum, userinfo, sizeof( userinfo ) );
		PlayerIntroSound( Info_ValueForKey( userinfo, "model" ) );

	}
}


/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue( int clientNum, int delay ) {
	int n;

	for ( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if ( !botSpawnQueue[n].spawnTime ) {
			botSpawnQueue[n].spawnTime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			return;
		}
	}

	G_Printf( S_COLOR_YELLOW "Unable to delay spawn\n" );
	ClientBegin( clientNum );
}


/*
===============
G_QueueBotBegin
===============
*/
void G_QueueBotBegin( int clientNum ) {
	AddBotToSpawnQueue( clientNum, botBeginDelay );
	botBeginDelay += BOT_BEGIN_DELAY_INCREMENT;
}


/*
===============
G_BotConnect
===============
*/
qboolean G_BotConnect( int clientNum, qboolean restart ) {
	bot_settings_t settings;
	char userinfo[MAX_INFO_STRING];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	Q_strncpyz( settings.characterfile, Info_ValueForKey( userinfo, "characterfile" ), sizeof( settings.characterfile ) );
	settings.skill = atoi( Info_ValueForKey( userinfo, "skill" ) );

	if ( !BotAISetupClient( clientNum, &settings ) ) {
		trap_DropClient( clientNum, "BotAISetupClient failed" );
		return qfalse;
	}

	if ( restart ) {
		g_entities[clientNum].botDelayBegin = qtrue;
	} else {
		g_entities[clientNum].botDelayBegin = qfalse;
	}

	return qtrue;
}


/*
===============
G_AddBot
===============
*/
static void G_AddBot( const char *name, int skill, const char *team, int delay ) {
	int		clientNum;
	int		teamNum;
	int		botinfoNum;
	char            *botinfo;
	char            *key;
	char            *s;
	char            *botname;
	char            *model;
	char userinfo[MAX_INFO_STRING];

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if ( clientNum == -1 ) {
		G_Printf( S_COLOR_RED "Unable to add bot. All player slots are in use.\n" );
		G_Printf( S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n" );
		return;
	}

	// set default team
	if( !team || !*team ) {
		team = "free";
	}

	// get the botinfo from bots.txt
	if ( Q_stricmp( name, "random" ) == 0 ) {
		if ( Q_stricmp( team, "red" ) == 0 || Q_stricmp( team, "r" ) == 0 ) {
			teamNum = TEAM_RED;
		}
		else if ( Q_stricmp( team, "blue" ) == 0 || Q_stricmp( team, "b" ) == 0 ) {
			teamNum = TEAM_BLUE;
		}
		else if ( !Q_stricmp( team, "spectator" ) || !Q_stricmp( team, "s" ) ) {
			teamNum = TEAM_SPECTATOR;
		}
		else {
			teamNum = TEAM_FREE;
		}

		botinfoNum = G_SelectRandomBotInfo( teamNum );

		if ( botinfoNum < 0 ) {
			G_Printf( S_COLOR_RED "Error: Cannot add random bot, no bot info available.\n" );
			trap_BotFreeClient( clientNum );
			return;
		}

		botinfo = G_GetBotInfoByNumber( botinfoNum );
	}
	else {
		botinfo = G_GetBotInfoByName( name );
	}

	if ( !botinfo ) {
		G_Printf( S_COLOR_RED "Error: Bot '%s' not defined\n", name );
		trap_BotFreeClient( clientNum );
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey( botinfo, "funname" );
	if ( !botname[0] ) {
		botname = Info_ValueForKey( botinfo, "name" );
	}
	Info_SetValueForKey( userinfo, "name", botname );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	Info_SetValueForKey( userinfo, "skill", va( "%i", skill ) );
	Info_SetValueForKey( userinfo, "teampref", team );

	if ( skill == 1 ) {
		Info_SetValueForKey( userinfo, "handicap", "50" );
	} else if ( skill == 2 )   {
		Info_SetValueForKey( userinfo, "handicap", "70" );
	} else if ( skill == 3 )   {
		Info_SetValueForKey( userinfo, "handicap", "90" );
	}

	key = "model";
	model = Info_ValueForKey( botinfo, key );
	if ( !*model ) {
		model = "visor/default";
	}
	Info_SetValueForKey( userinfo, key, model );

	key = "gender";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "male";
	}
	Info_SetValueForKey( userinfo, "sex", s );

	key = "color";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "4";
	}
	Info_SetValueForKey( userinfo, key, s );

	s = Info_ValueForKey( botinfo, "aifile" );
	if ( !*s ) {
		trap_Print( S_COLOR_RED "Error: bot has no aifile specified\n" );
		trap_BotFreeClient( clientNum );
		return;
	}
	Info_SetValueForKey( userinfo, "characterfile", s );

	// register the userinfo
	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if ( ClientConnect( clientNum, qtrue, qtrue ) ) {
		return;
	}

	if ( delay == 0 ) {
		ClientBegin( clientNum );
		return;
	}

	AddBotToSpawnQueue( clientNum, delay );
}


/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f( void ) {
	int skill;
	int delay;
	char name[MAX_TOKEN_CHARS];
	char string[MAX_TOKEN_CHARS];
	char team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	// name
	trap_Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		trap_Print( "Usage: Addbot <botname> [skill 1-4] [team] [msec delay]\n" );
		return;
	}

	// skill
	trap_Argv( 2, string, sizeof( string ) );
	if ( !string[0] ) {
		skill = 4;
	} else {
		skill = Com_Clamp( 1, 5, atoi( string ) );
	}

	// team
	trap_Argv( 3, team, sizeof( team ) );

	// delay
	trap_Argv( 4, string, sizeof( string ) );
	if ( !string[0] ) {
		delay = 0;
	} else {
		delay = atoi( string );
	}

	G_AddBot( name, skill, team, delay );

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if ( level.time - level.startTime > 1000 &&
		 trap_Cvar_VariableIntegerValue( "cl_running" ) ) {
		trap_SendServerCommand( -1, "loaddeferred\n" );   // spelling fixed (SA)
	}
}


/*
===============
G_SpawnBots
===============
*/
/*
static void G_SpawnBots( char *botList, int baseDelay ) {
	char		*bot;
	char		*p;
	int			skill;
	int			delay;
	char		bots[MAX_INFO_VALUE];

	podium1 = NULL;
	podium2 = NULL;
	podium3 = NULL;

	skill = trap_Cvar_VariableIntegerValue( "g_spSkill" );
	if( skill < 1 || skill > 5 ) {
		trap_Cvar_Set( "g_spSkill", "2" );
		skill = 2;
	}

	Q_strncpyz( bots, botList, sizeof(bots) );
	p = &bots[0];
	delay = baseDelay;
	while( *p ) {
		//skip spaces
		while( *p && *p == ' ' ) {
			p++;
		}
		if( !*p ) {
			break;
		}

		// mark start of bot name
		bot = p;

		// skip until space of null
		while( *p && *p != ' ' ) {
			p++;
		}
		if( *p ) {
			*p++ = 0;
		}

		// we must add the bot this way, calling G_AddBot directly at this stage
		// does "Bad Things"
		trap_SendConsoleCommand( EXEC_INSERT, va("addbot %s %i free %i\n", bot, skill, delay) );

		delay += BOT_BEGIN_DELAY_INCREMENT;
	}
}
*/


/*
===============
G_LoadBots
===============
*/
// TTimo: unused
/*
static void G_LoadBots( void ) {
#ifdef QUAKESTUFF
	int			len;
	char		*filename;
	vmCvar_t	botsFile;
	fileHandle_t	f;
	char		buf[MAX_BOTS_TEXT];

	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM );
	if( *botsFile.string ) {
		filename = botsFile.string;
	}
	else {
		filename = "scripts/bots.txt";
	}

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	g_numBots = COM_ParseInfos( buf, MAX_BOTS, g_botInfos );
	trap_Print( va( "%i bots parsed\n", g_numBots ) );
#endif
}
*/

/*
===============
G_GetBotInfoByNumber
===============
*/
char *G_GetBotInfoByNumber( int num ) {
	if ( num < 0 || num >= g_numBots ) {
		trap_Print( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}
	return g_botInfos[num];
}


/*
===============
G_GetBotInfoByName
===============
*/
char *G_GetBotInfoByName( const char *name ) {
	int n;
	char    *value;

	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return g_botInfos[n];
		}
	}

	return NULL;
}
