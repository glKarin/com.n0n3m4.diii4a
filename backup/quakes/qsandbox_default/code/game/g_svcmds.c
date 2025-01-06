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

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"


/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and you can use '*' to match any value
so you can specify an entire class C network with "addip 192.246.40.*"

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.

TTimo NOTE: for persistence, bans are stored in g_banIPs cvar MAX_CVAR_VALUE_STRING
The size of the cvar string buffer is limiting the banning to around 20 masks
this could be improved by putting some g_banIPs2 g_banIps3 etc. maybe
still, you should rely on PB for banning instead

==============================================================================
*/

typedef struct ipFilter_s
{
	unsigned	mask;
	unsigned	compare;
} ipFilter_t;

#define	MAX_IPFILTERS	1024

static ipFilter_t	ipFilters[MAX_IPFILTERS];
static int			numIPFilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter (char *s, ipFilter_t *f)
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];

	for (i=0 ; i<4 ; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}

	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			if (*s == '*') // 'match any'
			{
				// b[i] and m[i] to 0
				s++;
				if (!*s)
					break;
				s++;
				continue;
			}
                        G_Printf( "Bad filter address: %s\n", s );
			return qfalse;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		m[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;

	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans (void)
{
	byte	b[4];
	byte	m[4];
	int		i,j;
	char	iplist_final[MAX_CVAR_VALUE_STRING];
	char	ip[64];

	*iplist_final = 0;
	for (i = 0 ; i < numIPFilters ; i++)
	{
		if (ipFilters[i].compare == 0xffffffff)
			continue;

		*(unsigned *)b = ipFilters[i].compare;
		*(unsigned *)m = ipFilters[i].mask;
		*ip = 0;
		for (j = 0 ; j < 4 ; j++)
		{
			if (m[j]!=255)
				Q_strcat(ip, sizeof(ip), "*");
			else
				Q_strcat(ip, sizeof(ip), va("%i", b[j]));
			Q_strcat(ip, sizeof(ip), (j<3) ? "." : " ");
		}
		if (strlen(iplist_final)+strlen(ip) < MAX_CVAR_VALUE_STRING)
		{
			Q_strcat( iplist_final, sizeof(iplist_final), ip);
		}
		else
		{
			Com_Printf("g_banIPs overflowed at MAX_CVAR_VALUE_STRING\n");
			break;
		}
	}

	trap_Cvar_Set( "g_banIPs", iplist_final );
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket (char *from)
{
	int		i;
	unsigned	in;
	byte m[4];
	char *p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i]*10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}

	in = *(unsigned *)m;

	for (i=0 ; i<numIPFilters ; i++)
		if ( (in & ipFilters[i].mask) == ipFilters[i].compare)
			return g_filterBan.integer != 0;

	return g_filterBan.integer == 0;
}

/*
=================
AddIP
=================
*/
static void AddIP( char *str )
{
	int		i;

	for (i = 0 ; i < numIPFilters ; i++)
		if (ipFilters[i].compare == 0xffffffff)
			break;		// free spot
	if (i == numIPFilters)
	{
		if (numIPFilters == MAX_IPFILTERS)
		{
                        G_Printf ("IP filter list is full\n");
			return;
		}
		numIPFilters++;
	}

	if (!StringToFilter (str, &ipFilters[i]))
		ipFilters[i].compare = 0xffffffffu;

	UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans(void)
{
	char *s, *t;
	char		str[MAX_CVAR_VALUE_STRING];

	Q_strncpyz( str, g_banIPs.string, sizeof(str) );

	for (t = s = g_banIPs.string; *t; /* */ ) {
		s = strchr(s, ' ');
		if (!s)
			break;
		while (*s == ' ')
			*s++ = 0;
		if (*t)
			AddIP( t );
		t = s;
	}
}


/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f (void)
{
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
                G_Printf("Usage:  addip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	AddIP( str );

}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f (void)
{
	ipFilter_t	f;
	int			i;
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
                G_Printf("Usage:  sv removeip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	if (!StringToFilter (str, &f))
		return;

	for (i=0 ; i<numIPFilters ; i++) {
		if (ipFilters[i].mask == f.mask	&&
			ipFilters[i].compare == f.compare) {
			ipFilters[i].compare = 0xffffffffu;
                        G_Printf ("Removed.\n");

			UpdateIPBans();
			return;
		}
	}
        G_Printf ( "Didn't find %s.\n", str );
}

/*
===================
Svcmd_EntityList_f
===================
*/
void	Svcmd_EntityList_f (void) {
	int			e;
	gentity_t		*check;

	check = g_entities+1;
	for (e = 1; e < level.num_entities ; e++, check++) {
		if ( !check->inuse ) {
			continue;
		}
                G_Printf("%3i:", e);
		switch ( check->s.eType ) {
		case ET_GENERAL:
			G_Printf("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			G_Printf("ET_PLAYER           ");
			break;
		case ET_ITEM:
			G_Printf("ET_ITEM             ");
			break;
		case ET_MISSILE:
			G_Printf("ET_MISSILE          ");
			break;
		case ET_MOVER:
			G_Printf("ET_MOVER            ");
			break;
		case ET_BEAM:
			G_Printf("ET_BEAM             ");
			break;
		case ET_PORTAL:
			G_Printf("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			G_Printf("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			G_Printf("ET_PUSH_TRIGGER     ");
			break;
		case ET_TELEPORT_TRIGGER:
			G_Printf("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			G_Printf("ET_INVISIBLE        ");
			break;
		case ET_GRAPPLE:
			G_Printf("ET_GRAPPLE          ");
			break;
		case ET_WEATHER:
			G_Printf("ET_WEATHER          ");
			break;
		default:
			G_Printf("%3i                 ", check->s.eType);
			break;
		}
		if ( check->classname ) {
			G_Printf("%s", check->classname);
		}
                G_Printf("\n");
	}
}

gclient_t	*ClientForString( const char *s ) {
	gclient_t	*cl;
	int			i;
	int			idnum;

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Com_Printf( "Bad client slot: %i\n", idnum );
			return NULL;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
                        G_Printf( "Client %i is not connected\n", idnum );
			return NULL;
		}
		return cl;
	}

	// check for a name match
	for ( i=0 ; i < level.maxclients ; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname, s ) ) {
			return cl;
		}
	}
        G_Printf( "User %s is not on the server\n", s );

	return NULL;
}

/*
==================
Svcmd_PickTarget_f
Added for QSandbox.
==================
*/
void Svcmd_PickTarget_f( void ){
	char		p[128];
	gentity_t 	*act;

	trap_Argv( 1, p, sizeof( p ) );

	act = G_PickTarget( p );
	if ( act && act->use ) {
		act->use( act, NULL, NULL );
	}
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void	Svcmd_ForceTeam_f( void ) {
	gclient_t	*cl;
	char		str[MAX_TOKEN_CHARS];

	// find the player
	trap_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str );
}

void	ClientKick_f( void ) {
        int idnum, i;
        char	str[MAX_TOKEN_CHARS];

        trap_Argv( 1, str, sizeof( str ) );

        for (i = 0; str[i]; i++) {
		if (str[i] < '0' || str[i] > '9') {
                    G_Printf("not a valid client number: \"%s\"\n",str);
			return;
		}
	}

        idnum = atoi( str );

        //Local client
        if( level.clients[idnum].pers.localClient ) {
            G_Printf("Kick failed - local player\n");
            return;
        }

        //Now clientkick has been moved into game, but we still need to find the idnum the server expects....
        //FIXME: To fix this, we need a relieble way to generate difference between the server's client number and the game's client numbers
        //FIXME: This should not depend on the engine's clientkick at all
        trap_DropClient( idnum, "was kicked" );
        //trap_SendConsoleCommand( EXEC_INSERT, va("clientkick %d\n", level.clients[idnum].ps.clientNum) );

}

void EndGame_f ( void ) {
    ExitLevel();
}

//KK-OAX Moved this Declaration to g_local.h
//char	*ConcatArgs( int start );

/*KK-OAX
===============
Server Command Table
Not Worth Listing Elsewhere
================
*/
struct
{
  char      *cmd;
  qboolean  dedicated; //if it has to be entered from a dedicated server or RCON
  void      ( *function )( void );
} svcmds[ ] = {

  { "entityList", qfalse, Svcmd_EntityList_f },
  { "forceTeam", qfalse, Svcmd_ForceTeam_f },
  { "game_memory", qfalse, Svcmd_GameMem_f },
  { "addbot", qfalse, Svcmd_AddBot_f },
  { "botlist", qfalse, Svcmd_BotList_f },
  { "addip", qfalse, Svcmd_AddIP_f },
  { "removeip", qfalse, Svcmd_RemoveIP_f },

  { "listip", qfalse, Svcmd_ListIP_f },
  { "status", qfalse, Svcmd_Status_f },
  { "eject", qfalse, Svcmd_EjectClient_f },
  { "dumpuser", qfalse, Svcmd_DumpUser_f },
  { "centerprint", qfalse, Svcmd_CenterPrint_f },
  { "replacetexture", qfalse, Svcmd_ReplaceTexture_f },
  { "say_team", qtrue, Svcmd_TeamMessage_f },
  { "say", qtrue, Svcmd_MessageWrapper },
  { "chat", qtrue, Svcmd_Chat_f },
  { "shuffle", qfalse, ShuffleTeams },
  { "clientkick_game", qfalse, ClientKick_f },
  { "endgamenow", qfalse, EndGame_f },
  { "savemap", qfalse, G_WriteMapfile_f },
  { "savemapall", qfalse, G_WriteMapfileAll_f },
  { "loadmap", qfalse, G_LoadMapfile_f },
  { "loadmapall", qfalse, G_LoadMapfileAll_f },
  
  { "hideobjects", qfalse, G_HideObjects },
  { "showobjects", qfalse, G_ShowObjects },
  { "picktarget", qfalse, Svcmd_PickTarget_f },
  { "create", qfalse, Svcmd_PropNpc_AS_f },

  //Noire.Script
  { "ns_openscript", qfalse, Svcmd_NS_OpenScript_f },
  { "ns_interpret", qfalse, Svcmd_NS_Interpret_f },
  { "ns_variablelist", qfalse, Svcmd_NS_VariableList_f },
  { "ns_threadlist", qfalse, Svcmd_NS_ThreadList_f },
  { "ns_sendvariable", qfalse, Svcmd_NS_SendVariable_f },
};

/*
=================
ConsoleCommand

=================
*/
qboolean  ConsoleCommand( void )
{
  char cmd[ MAX_TOKEN_CHARS ];
  int  i;

  trap_Argv( 0, cmd, sizeof( cmd ) );

  for( i = 0; i < sizeof( svcmds ) / sizeof( svcmds[ 0 ] ); i++ )
  {
    if( !Q_stricmp( cmd, svcmds[ i ].cmd ) )
    {
      if( svcmds[ i ].dedicated && !g_dedicated.integer )
        return qfalse;
      svcmds[ i ].function( );
      return qtrue;
    }
  }

  if( g_dedicated.integer )
    G_Printf( "unknown command: %s\n", cmd );

  return qfalse;
}
