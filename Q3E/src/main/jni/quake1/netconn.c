/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2002 Mathieu Olivier
Copyright (C) 2003 Ashley Rose Hale (LadyHavoc)

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

#include "quakedef.h"
#include "thread.h"
#include "lhnet.h"

// for secure rcon authentication
#include "hmac.h"
#include "mdfour.h"
#include <time.h>

#define QWMASTER_PORT 27000
#define DPMASTER_PORT 27950

// note this defaults on for dedicated servers, off for listen servers
cvar_t sv_public = {CF_SERVER, "sv_public", "0", "1: advertises this server on the master server (so that players can find it in the server browser); 0: allow direct queries only; -1: do not respond to direct queries; -2: do not allow anyone to connect; -3: already block at getchallenge level"};
cvar_t sv_public_rejectreason = {CF_SERVER, "sv_public_rejectreason", "The server is closing.", "Rejection reason for connects when sv_public is -2"};
static cvar_t sv_heartbeatperiod = {CF_SERVER | CF_ARCHIVE, "sv_heartbeatperiod", "120", "how often to send heartbeat in seconds (only used if sv_public is 1)"};
extern cvar_t sv_status_privacy;

static cvar_t sv_masters [] =
{
	{CF_CLIENT | CF_SERVER | CF_ARCHIVE, "sv_master1", "", "user-chosen master server 1"},
	{CF_CLIENT | CF_SERVER | CF_ARCHIVE, "sv_master2", "", "user-chosen master server 2"},
	{CF_CLIENT | CF_SERVER | CF_ARCHIVE, "sv_master3", "", "user-chosen master server 3"},
	{CF_CLIENT | CF_SERVER | CF_ARCHIVE, "sv_master4", "", "user-chosen master server 4"},
	{CF_CLIENT | CF_SERVER, "sv_masterextra1", "dpmaster.deathmask.net", "dpmaster.deathmask.net - default master server 1 (admin: Willis)"},
	{CF_CLIENT | CF_SERVER, "sv_masterextra2", "dpmaster.tchr.no", "dpmaster.tchr.no - default master server 2 (admin: tChr)"},
	{CF_CLIENT | CF_SERVER, "sv_masterextra3", "dpm.dpmaster.org:27777", "dpm.dpmaster.org - default master server 3 (admin: gazby/soylent_cow)"},
};

// asgaard.morphos-team.net resolves to the same ipv4 as qwmaster.fodquake.net,
// its reverse PTR is for asgaard.morphos-team.net but qwmaster.fodquake.net seems more popular.
// qwmaster.ocrana.de seems long dead.
// https://www.quakeservers.net/quakeworld/master_servers/
#ifdef CONFIG_MENU
static cvar_t sv_qwmasters [] =
{
	{CF_CLIENT | CF_ARCHIVE, "sv_qwmaster1", "", "user-chosen qwmaster server 1"},
	{CF_CLIENT | CF_ARCHIVE, "sv_qwmaster2", "", "user-chosen qwmaster server 2"},
	{CF_CLIENT | CF_ARCHIVE, "sv_qwmaster3", "", "user-chosen qwmaster server 3"},
	{CF_CLIENT | CF_ARCHIVE, "sv_qwmaster4", "", "user-chosen qwmaster server 4"},
	{CF_CLIENT, "sv_qwmasterextra1", "master.quakeservers.net:27000", "QW master in Germany, admin: unknown"},
	{CF_CLIENT, "sv_qwmasterextra2", "qwmaster.fodquake.net:27000", "QW master in Germany, same IP as asgaard.morphos-team.net, admin: bigfoot"},
	{CF_CLIENT, "sv_qwmasterextra3", "master.quakeworld.nu:27000", "QW master in Sweden, admin: unknown"},
};
#endif

static double nextheartbeattime = 0;

sizebuf_t cl_message;
sizebuf_t sv_message;
static unsigned char cl_message_buf[NET_MAXMESSAGE];
static unsigned char sv_message_buf[NET_MAXMESSAGE];
char cl_readstring[MAX_INPUTLINE];
char sv_readstring[MAX_INPUTLINE];

cvar_t net_test = {CF_CLIENT | CF_SERVER, "net_test", "0", "internal development use only, leave it alone (usually does nothing anyway)"};
cvar_t net_usesizelimit = {CF_SERVER, "net_usesizelimit", "2", "use packet size limiting (0: never, 1: in non-CSQC mode, 2: always)"};
cvar_t net_burstreserve = {CF_SERVER, "net_burstreserve", "0.3", "how much of the burst time to reserve for packet size spikes"};
cvar_t net_messagetimeout = {CF_CLIENT | CF_SERVER, "net_messagetimeout","300", "drops players who have not sent any packets for this many seconds"};
cvar_t net_connecttimeout = {CF_CLIENT | CF_SERVER, "net_connecttimeout","15", "after requesting a connection, the client must reply within this many seconds or be dropped (cuts down on connect floods). Must be above 10 seconds."};
cvar_t net_connect_entnum_ofs = {CF_SERVER, "net_connect_entnum_ofs", "0", "entity number offset of human clients (for developer testing only)"};
cvar_t net_connectfloodblockingtimeout = {CF_SERVER, "net_connectfloodblockingtimeout", "5", "when a connection packet is received, it will block all future connect packets from that IP address for this many seconds (cuts down on connect floods). Note that this does not include retries from the same IP; these are handled earlier and let in."};
cvar_t net_challengefloodblockingtimeout = {CF_SERVER, "net_challengefloodblockingtimeout", "0.5", "when a challenge packet is received, it will block all future challenge packets from that IP address for this many seconds (cuts down on challenge floods). DarkPlaces clients retry once per second, so this should be <= 1. Failure here may lead to connect attempts failing."};
cvar_t net_getstatusfloodblockingtimeout = {CF_SERVER, "net_getstatusfloodblockingtimeout", "1", "when a getstatus packet is received, it will block all future getstatus packets from that IP address for this many seconds (cuts down on getstatus floods). DarkPlaces retries every net_slist_timeout seconds, and qstat retries once per second, so this should be <= 1. Failure here may lead to server not showing up in the server list."};
cvar_t net_sourceaddresscheck = {CF_CLIENT, "net_sourceaddresscheck", "1", "compare the source IP address for replies (more secure, may break some bad multihoming setups"};
cvar_t hostname = {CF_SERVER | CF_ARCHIVE, "hostname", "UNNAMED", "server message to show in server browser"};
cvar_t developer_networking = {CF_CLIENT | CF_SERVER, "developer_networking", "0", "prints all received and sent packets (recommended only for debugging)"};

cvar_t net_fakelag = {CF_CLIENT, "net_fakelag","0", "lags local loopback connection by this much ping time (useful to play more fairly on your own server with people with higher pings)"};
static cvar_t net_fakeloss_send = {CF_CLIENT, "net_fakeloss_send","0", "drops this percentage of outgoing packets, useful for testing network protocol robustness (jerky movement, prediction errors, etc)"};
static cvar_t net_fakeloss_receive = {CF_CLIENT, "net_fakeloss_receive","0", "drops this percentage of incoming packets, useful for testing network protocol robustness (jerky movement, effects failing to start, sounds failing to play, etc)"};

#ifdef CONFIG_MENU
static cvar_t net_slist_debug = {CF_CLIENT, "net_slist_debug", "0", "enables verbose messages for master server queries"};
static cvar_t net_slist_favorites = {CF_CLIENT | CF_ARCHIVE, "net_slist_favorites", "", "contains a list of IP addresses and ports to always query explicitly"};
static cvar_t net_slist_interval = {CF_CLIENT, "net_slist_interval", "1", "minimum number of seconds to wait between getstatus queries to the same DP server, must be >= server's net_getstatusfloodblockingtimeout"};
static cvar_t net_slist_maxping = {CF_CLIENT | CF_ARCHIVE, "net_slist_maxping", "420", "server query responses are ignored if their ping in milliseconds is higher than this"};
static cvar_t net_slist_maxtries = {CF_CLIENT, "net_slist_maxtries", "3", "how many times to ask the same server for information (more times gives better ping reports but takes longer)"};
static cvar_t net_slist_pause = {CF_CLIENT, "net_slist_pause", "0", "when set to 1, the server list sorting in the menu won't update until it is set back to 0"};
static cvar_t net_slist_queriespersecond = {CF_CLIENT, "net_slist_queriespersecond", "128", "how many server information requests to send per second"};
static cvar_t net_slist_queriesperframe = {CF_CLIENT, "net_slist_queriesperframe", "2", "maximum number of server information requests to send each rendered frame (guards against low framerates causing problems)"};
static cvar_t net_slist_timeout = {CF_CLIENT, "net_slist_timeout", "4", "minimum number of seconds to wait between status queries to the same QW server, determines which response belongs to which query so low values will cause impossible pings; also a warning is printed if a dpmaster query fails to complete within this time"};
#endif

static cvar_t net_tos_dscp = {CF_CLIENT | CF_ARCHIVE, "net_tos_dscp", "32", "DiffServ Codepoint for network sockets (may need game restart to apply)"};
static cvar_t gameversion = {CF_SERVER, "gameversion", "0", "version of game data (mod-specific) to be sent to querying clients"};
static cvar_t gameversion_min = {CF_CLIENT | CF_SERVER, "gameversion_min", "-1", "minimum version of game data (mod-specific), when client and server gameversion mismatch in the server browser the server is shown as incompatible; if -1, gameversion is used alone"};
static cvar_t gameversion_max = {CF_CLIENT | CF_SERVER, "gameversion_max", "-1", "maximum version of game data (mod-specific), when client and server gameversion mismatch in the server browser the server is shown as incompatible; if -1, gameversion is used alone"};
static cvar_t rcon_restricted_password = {CF_SERVER | CF_PRIVATE, "rcon_restricted_password", "", "password to authenticate rcon commands in restricted mode; may be set to a string of the form user1:pass1 user2:pass2 user3:pass3 to allow multiple user accounts - the client then has to specify ONE of these combinations"};
static cvar_t rcon_restricted_commands = {CF_SERVER, "rcon_restricted_commands", "", "allowed commands for rcon when the restricted mode password was used"};
static cvar_t rcon_secure_maxdiff = {CF_SERVER, "rcon_secure_maxdiff", "5", "maximum time difference between rcon request and server system clock (to protect against replay attack)"};
extern cvar_t rcon_secure;
extern cvar_t rcon_secure_challengetimeout;

double masterquerytime = -1000;
unsigned masterquerycount = 0;
unsigned masterreplycount = 0;
unsigned serverquerycount = 0;
unsigned serverreplycount = 0;

challenge_t challenges[MAX_CHALLENGES];

#define DPMASTER_COUNT sizeof(sv_masters) / sizeof(cvar_t)
#define QWMASTER_COUNT sizeof(sv_qwmasters) / sizeof(cvar_t)

/// bitfield because in theory we could be doing QW & DP simultaneously
uint8_t serverlist_querystage = 0;

#ifdef CONFIG_MENU
#define SLIST_QUERYSTAGE_DPMASTERS 1
#define SLIST_QUERYSTAGE_QWMASTERS 2
#define SLIST_QUERYSTAGE_SERVERS 4

static uint8_t dpmasterstatus[DPMASTER_COUNT] = {0};
static uint8_t qwmasterstatus[QWMASTER_COUNT] = {0};
#define MASTER_TX_QUERY 1    ///< we sent the query
#define MASTER_RX_RESPONSE 2 ///< we got at least 1 packet of the response
#define MASTER_RX_COMPLETE 3 ///< we saw the EOT marker (assumes dpmaster >= 2.0, see dpmaster/doc/techinfo.txt)

/// the hash password for timestamp verification
char serverlist_dpserverquerykey[12]; // challenge_t uses [12]
#endif

static unsigned cl_numsockets;
static lhnetsocket_t *cl_sockets[16];
static unsigned sv_numsockets;
static lhnetsocket_t *sv_sockets[16];

netconn_t *netconn_list = NULL;
mempool_t *netconn_mempool = NULL;
void *netconn_mutex = NULL;

cvar_t cl_netport = {CF_CLIENT, "cl_port", "0", "forces client to use chosen port number if not 0"};
cvar_t sv_netport = {CF_SERVER, "port", "26000", "server port for players to connect to"};
cvar_t net_address = {CF_CLIENT | CF_SERVER, "net_address", "", "network address to open ipv4 ports on (if empty, use default interfaces)"};
cvar_t net_address_ipv6 = {CF_CLIENT | CF_SERVER, "net_address_ipv6", "", "network address to open ipv6 ports on (if empty, use default interfaces)"};

char cl_net_extresponse[NET_EXTRESPONSE_MAX][1400];
unsigned cl_net_extresponse_count = 0;
unsigned cl_net_extresponse_last = 0;

char sv_net_extresponse[NET_EXTRESPONSE_MAX][1400];
unsigned sv_net_extresponse_count = 0;
unsigned sv_net_extresponse_last = 0;

#ifdef CONFIG_MENU
// ServerList interface
serverlist_mask_t serverlist_andmasks[SERVERLIST_ANDMASKCOUNT];
serverlist_mask_t serverlist_ormasks[SERVERLIST_ORMASKCOUNT];

serverlist_infofield_t serverlist_sortbyfield;
unsigned serverlist_sortflags;

unsigned serverlist_viewcount = 0;
uint16_t serverlist_viewlist[SERVERLIST_VIEWLISTSIZE];

unsigned serverlist_maxcachecount = 0;
unsigned serverlist_cachecount = 0;
serverlist_entry_t *serverlist_cache = NULL;

static qbool serverlist_consoleoutput;

static unsigned nFavorites = 0;
static lhnetaddress_t favorites[MAX_FAVORITESERVERS];
static unsigned nFavorites_idfp = 0;
static char favorites_idfp[MAX_FAVORITESERVERS][FP64_SIZE+1];

void NetConn_UpdateFavorites_c(cvar_t *var)
{
	const char *p;
	nFavorites = 0;
	nFavorites_idfp = 0;
	p = var->string;
	while((size_t) nFavorites < sizeof(favorites) / sizeof(*favorites) && COM_ParseToken_Console(&p))
	{
		if(com_token[0] != '[' && strlen(com_token) == FP64_SIZE && !strchr(com_token, '.'))
		// currently 44 bytes, longest possible IPv6 address: 39 bytes, so this works
		// (if v6 address contains port, it must start with '[')
		{
			dp_strlcpy(favorites_idfp[nFavorites_idfp], com_token, sizeof(favorites_idfp[nFavorites_idfp]));
			++nFavorites_idfp;
		}
		else 
		{
			if(LHNETADDRESS_FromString(&favorites[nFavorites], com_token, 26000))
				++nFavorites;
		}
	}
}

/// helper function to insert a value into the viewset
/// spare entries will be removed
static void _ServerList_ViewList_Helper_InsertBefore(unsigned index, serverlist_entry_t *entry)
{
	unsigned i;

	if( serverlist_viewcount < SERVERLIST_VIEWLISTSIZE ) {
		i = serverlist_viewcount++;
	} else {
		i = SERVERLIST_VIEWLISTSIZE - 1;
	}

	for( ; i > index ; i-- )
		serverlist_viewlist[ i ] = serverlist_viewlist[ i - 1 ];

	serverlist_viewlist[index] = (int)(entry - serverlist_cache);
}

/// we suppose serverlist_viewcount to be valid, ie > 0
static inline void _ServerList_ViewList_Helper_Remove(unsigned index)
{
	serverlist_viewcount--;
	for( ; index < serverlist_viewcount ; index++ )
		serverlist_viewlist[index] = serverlist_viewlist[index + 1];
}

/// \returns true if A should be inserted before B
static qbool _ServerList_Entry_Compare( serverlist_entry_t *A, serverlist_entry_t *B )
{
	int result = 0; // > 0 if for numbers A > B and for text if A < B

	if( serverlist_sortflags & SLSF_CATEGORIES )
	{
		result = A->info.category - B->info.category;
		if (result != 0)
			return result < 0;
	}

	if( serverlist_sortflags & SLSF_FAVORITES )
	{
		if(A->info.isfavorite != B->info.isfavorite)
			return A->info.isfavorite;
	}
	
	switch( serverlist_sortbyfield ) {
		case SLIF_PING:
			result = A->info.ping - B->info.ping;
			break;
		case SLIF_MAXPLAYERS:
			result = A->info.maxplayers - B->info.maxplayers;
			break;
		case SLIF_NUMPLAYERS:
			result = A->info.numplayers - B->info.numplayers;
			break;
		case SLIF_NUMBOTS:
			result = A->info.numbots - B->info.numbots;
			break;
		case SLIF_NUMHUMANS:
			result = A->info.numhumans - B->info.numhumans;
			break;
		case SLIF_FREESLOTS:
			result = A->info.freeslots - B->info.freeslots;
			break;
		case SLIF_PROTOCOL:
			result = A->info.protocol - B->info.protocol;
			break;
		case SLIF_CNAME:
			result = strcmp( B->info.cname, A->info.cname );
			break;
		case SLIF_GAME:
			result = strcasecmp( B->info.game, A->info.game );
			break;
		case SLIF_MAP:
			result = strcasecmp( B->info.map, A->info.map );
			break;
		case SLIF_MOD:
			result = strcasecmp( B->info.mod, A->info.mod );
			break;
		case SLIF_NAME:
			result = strcasecmp( B->info.name, A->info.name );
			break;
		case SLIF_QCSTATUS:
			result = strcasecmp( B->info.qcstatus, A->info.qcstatus ); // not really THAT useful, though
			break;
		case SLIF_CATEGORY:
			result = A->info.category - B->info.category;
			break;
		case SLIF_ISFAVORITE:
			result = !!B->info.isfavorite - !!A->info.isfavorite;
			break;
		default:
			Con_DPrint( "_ServerList_Entry_Compare: Bad serverlist_sortbyfield!\n" );
			break;
	}

	if (result != 0)
	{
		if( serverlist_sortflags & SLSF_DESCENDING )
			return result > 0;
		else
			return result < 0;
	}

	// if the chosen sort key is identical, sort by index
	// (makes this a stable sort, so that later replies from servers won't
	//  shuffle the servers around when they have the same ping)
	return A < B;
}

static qbool _ServerList_CompareInt( int A, serverlist_maskop_t op, int B )
{
	// This should actually be done with some intermediate and end-of-function return
	switch( op ) {
		case SLMO_LESS:
			return A < B;
		case SLMO_LESSEQUAL:
			return A <= B;
		case SLMO_EQUAL:
			return A == B;
		case SLMO_GREATER:
			return A > B;
		case SLMO_NOTEQUAL:
			return A != B;
		case SLMO_GREATEREQUAL:
		case SLMO_CONTAINS:
		case SLMO_NOTCONTAIN:
		case SLMO_STARTSWITH:
		case SLMO_NOTSTARTSWITH:
			return A >= B;
		default:
			Con_DPrint( "_ServerList_CompareInt: Bad op!\n" );
			return false;
	}
}

static qbool _ServerList_CompareStr( const char *A, serverlist_maskop_t op, const char *B )
{
	int i;
	char bufferA[ 1400 ], bufferB[ 1400 ]; // should be more than enough
	COM_StringDecolorize(A, 0, bufferA, sizeof(bufferA), false);
	for (i = 0;i < (int)sizeof(bufferA)-1 && bufferA[i];i++)
		bufferA[i] = (bufferA[i] >= 'A' && bufferA[i] <= 'Z') ? (bufferA[i] + 'a' - 'A') : bufferA[i];
	bufferA[i] = 0;
	for (i = 0;i < (int)sizeof(bufferB)-1 && B[i];i++)
		bufferB[i] = (B[i] >= 'A' && B[i] <= 'Z') ? (B[i] + 'a' - 'A') : B[i];
	bufferB[i] = 0;

	// Same here, also using an intermediate & final return would be more appropriate
	// A info B mask
	switch( op ) {
		case SLMO_CONTAINS:
			return *bufferB && !!strstr( bufferA, bufferB ); // we want a real bool
		case SLMO_NOTCONTAIN:
			return !*bufferB || !strstr( bufferA, bufferB );
		case SLMO_STARTSWITH:
			//Con_Printf("startsWith: %s %s\n", bufferA, bufferB);
			return *bufferB && !memcmp(bufferA, bufferB, strlen(bufferB));
		case SLMO_NOTSTARTSWITH:
			return !*bufferB || memcmp(bufferA, bufferB, strlen(bufferB));
		case SLMO_LESS:
			return strcmp( bufferA, bufferB ) < 0;
		case SLMO_LESSEQUAL:
			return strcmp( bufferA, bufferB ) <= 0;
		case SLMO_EQUAL:
			return strcmp( bufferA, bufferB ) == 0;
		case SLMO_GREATER:
			return strcmp( bufferA, bufferB ) > 0;
		case SLMO_NOTEQUAL:
			return strcmp( bufferA, bufferB ) != 0;
		case SLMO_GREATEREQUAL:
			return strcmp( bufferA, bufferB ) >= 0;
		default:
			Con_DPrint( "_ServerList_CompareStr: Bad op!\n" );
			return false;
	}
}

static qbool _ServerList_Entry_Mask( serverlist_mask_t *mask, serverlist_info_t *info )
{
	if( !_ServerList_CompareInt( info->ping, mask->tests[SLIF_PING], mask->info.ping ) )
		return false;
	if( !_ServerList_CompareInt( info->maxplayers, mask->tests[SLIF_MAXPLAYERS], mask->info.maxplayers ) )
		return false;
	if( !_ServerList_CompareInt( info->numplayers, mask->tests[SLIF_NUMPLAYERS], mask->info.numplayers ) )
		return false;
	if( !_ServerList_CompareInt( info->numbots, mask->tests[SLIF_NUMBOTS], mask->info.numbots ) )
		return false;
	if( !_ServerList_CompareInt( info->numhumans, mask->tests[SLIF_NUMHUMANS], mask->info.numhumans ) )
		return false;
	if( !_ServerList_CompareInt( info->freeslots, mask->tests[SLIF_FREESLOTS], mask->info.freeslots ) )
		return false;
	if( !_ServerList_CompareInt( info->protocol, mask->tests[SLIF_PROTOCOL], mask->info.protocol ))
		return false;
	if( *mask->info.cname
		&& !_ServerList_CompareStr( info->cname, mask->tests[SLIF_CNAME], mask->info.cname ) )
		return false;
	if( *mask->info.game
		&& !_ServerList_CompareStr( info->game, mask->tests[SLIF_GAME], mask->info.game ) )
		return false;
	if( *mask->info.mod
		&& !_ServerList_CompareStr( info->mod, mask->tests[SLIF_MOD], mask->info.mod ) )
		return false;
	if( *mask->info.map
		&& !_ServerList_CompareStr( info->map, mask->tests[SLIF_MAP], mask->info.map ) )
		return false;
	if( *mask->info.name
		&& !_ServerList_CompareStr( info->name, mask->tests[SLIF_NAME], mask->info.name ) )
		return false;
	if( *mask->info.qcstatus
		&& !_ServerList_CompareStr( info->qcstatus, mask->tests[SLIF_QCSTATUS], mask->info.qcstatus ) )
		return false;
	if( *mask->info.players
		&& !_ServerList_CompareStr( info->players, mask->tests[SLIF_PLAYERS], mask->info.players ) )
		return false;
	if( !_ServerList_CompareInt( info->category, mask->tests[SLIF_CATEGORY], mask->info.category ) )
		return false;
	if( !_ServerList_CompareInt( info->isfavorite, mask->tests[SLIF_ISFAVORITE], mask->info.isfavorite ))
		return false;
	return true;
}

static void ServerList_ViewList_Insert( serverlist_entry_t *entry )
{
	unsigned start, end, mid, i;
	lhnetaddress_t addr;

	// reject incompatible servers
	if(
		entry->info.gameversion != gameversion.integer
		&&
		!(
			   gameversion_min.integer >= 0 // min/max range set by user/mod?
			&& gameversion_max.integer >= 0
			&& gameversion_min.integer <= entry->info.gameversion // version of server in min/max range?
			&& gameversion_max.integer >= entry->info.gameversion
		 )
	)
		return;

	// also display entries that are currently being refreshed [11/8/2007 Black]
	// bones_was_here: if their previous ping was acceptable (unset if timeout occurs)
	if (!entry->info.ping)
		return;

	// refresh the "favorite" status
	entry->info.isfavorite = false;
	if(LHNETADDRESS_FromString(&addr, entry->info.cname, 26000))
	{
		char idfp[FP64_SIZE+1];
		for(i = 0; i < nFavorites; ++i)
		{
			if(LHNETADDRESS_Compare(&addr, &favorites[i]) == 0)
			{
				entry->info.isfavorite = true;
				break;
			}
		}
		if(Crypto_RetrieveHostKey(&addr, 0, NULL, 0, idfp, sizeof(idfp), NULL, NULL))
		{
			for(i = 0; i < nFavorites_idfp; ++i)
			{
				if(!strcmp(idfp, favorites_idfp[i]))
				{
					entry->info.isfavorite = true;
					break;
				}
			}
		}
	}

	// refresh the "category"
	entry->info.category = MR_GetServerListEntryCategory(entry);

	// FIXME: change this to be more readable (...)
	// now check whether it passes through the masks
	for( start = 0 ; start < SERVERLIST_ANDMASKCOUNT && serverlist_andmasks[start].active; start++ )
		if( !_ServerList_Entry_Mask( &serverlist_andmasks[start], &entry->info ) )
			return;

	for( start = 0 ; start < SERVERLIST_ORMASKCOUNT && serverlist_ormasks[start].active ; start++ )
		if( _ServerList_Entry_Mask( &serverlist_ormasks[start], &entry->info ) )
			break;
	if( start == SERVERLIST_ORMASKCOUNT || (start > 0 && !serverlist_ormasks[start].active) )
		return;

	if( !serverlist_viewcount ) {
		_ServerList_ViewList_Helper_InsertBefore( 0, entry );
		return;
	}
	// ok, insert it, we just need to find out where exactly:

	// two special cases
	// check whether to insert it as new first item
	if( _ServerList_Entry_Compare( entry, ServerList_GetViewEntry(0) ) ) {
		_ServerList_ViewList_Helper_InsertBefore( 0, entry );
		return;
	} // check whether to insert it as new last item
	else if( !_ServerList_Entry_Compare( entry, ServerList_GetViewEntry(serverlist_viewcount - 1) ) ) {
		_ServerList_ViewList_Helper_InsertBefore( serverlist_viewcount, entry );
		return;
	}
	start = 0;
	end = serverlist_viewcount - 1;
	while( end > start + 1 )
	{
		mid = (start + end) / 2;
		// test the item that lies in the middle between start and end
		if( _ServerList_Entry_Compare( entry, ServerList_GetViewEntry(mid) ) )
			// the item has to be in the upper half
			end = mid;
		else
			// the item has to be in the lower half
			start = mid;
	}
	_ServerList_ViewList_Helper_InsertBefore( start + 1, entry );
}

static void ServerList_ViewList_Remove( serverlist_entry_t *entry )
{
	unsigned i;

	for( i = 0; i < serverlist_viewcount; i++ )
	{
		if (ServerList_GetViewEntry(i) == entry)
		{
			_ServerList_ViewList_Helper_Remove(i);
			break;
		}
	}
}

void ServerList_RebuildViewList(cvar_t *var)
{
	unsigned i;

	if (net_slist_pause.integer)
		return;

	serverlist_viewcount = 0;
	for (i = 0; i < serverlist_cachecount; ++i)
		ServerList_ViewList_Insert(&serverlist_cache[i]);
}

void ServerList_ResetMasks(void)
{
	int i;

	memset( &serverlist_andmasks, 0, sizeof( serverlist_andmasks ) );
	memset( &serverlist_ormasks, 0, sizeof( serverlist_ormasks ) );
	// numbots needs to be compared to -1 to always succeed
	for(i = 0; i < SERVERLIST_ANDMASKCOUNT; ++i)
		serverlist_andmasks[i].info.numbots = -1;
	for(i = 0; i < SERVERLIST_ORMASKCOUNT; ++i)
		serverlist_ormasks[i].info.numbots = -1;
}

void ServerList_GetPlayerStatistics(unsigned *numplayerspointer, unsigned *maxplayerspointer)
{
	unsigned i;
	unsigned numplayers = 0, maxplayers = 0;

	for (i = 0;i < serverlist_cachecount;i++)
	{
		if (serverlist_cache[i].info.ping)
		{
			numplayers += serverlist_cache[i].info.numhumans;
			maxplayers += serverlist_cache[i].info.maxplayers;
		}
	}
	*numplayerspointer = numplayers;
	*maxplayerspointer = maxplayers;
}

#if 0
static void _ServerList_Test(void)
{
	int i;
	if (serverlist_maxcachecount <= 1024)
	{
		serverlist_maxcachecount = 1024;
		serverlist_cache = (serverlist_entry_t *)Mem_Realloc(netconn_mempool, (void *)serverlist_cache, sizeof(serverlist_entry_t) * serverlist_maxcachecount);
	}
	for( i = 0 ; i < 1024 ; i++ ) {
		memset( &serverlist_cache[serverlist_cachecount], 0, sizeof( serverlist_entry_t ) );
		serverlist_cache[serverlist_cachecount].info.ping = 1000 + 1024 - i;
		dpsnprintf( serverlist_cache[serverlist_cachecount].info.name, sizeof(serverlist_cache[serverlist_cachecount].info.name), "Black's ServerList Test %i", i );
		serverlist_cache[serverlist_cachecount].finished = true;
		dpsnprintf( serverlist_cache[serverlist_cachecount].line1, sizeof(serverlist_cache[serverlist_cachecount].info.line1), "%i %s", serverlist_cache[serverlist_cachecount].info.ping, serverlist_cache[serverlist_cachecount].info.name );
		ServerList_ViewList_Insert( &serverlist_cache[serverlist_cachecount] );
		serverlist_cachecount++;
	}
}
#endif

/*
====================
ServerList_BuildDPServerQuery

Generates the string for pinging DP servers with a hash-verified timestamp
to provide reliable pings while preventing ping cheating,
and discard spurious getstatus/getinfo packets.

14 bytes of header including the mandatory space,
22 bytes of base64-encoded hash,
up to 16 bytes of unsigned hexadecimal milliseconds (typically 4-5 bytes sent),
null terminator.

The maximum challenge length (after the space) for existing DP7 servers is 49.
====================
*/
static inline void ServerList_BuildDPServerQuery(char *buffer, size_t buffersize, double querytime)
{
	unsigned char hash[24]; // 4*(16/3) rounded up to 4 byte multiple
	uint64_t timestamp = querytime * 1000.0; // no rounding up as that could make small pings go <= 0

	HMAC_MDFOUR_16BYTES(hash,
		(unsigned char *)&timestamp, sizeof(timestamp),
		(unsigned char *)serverlist_dpserverquerykey, sizeof(serverlist_dpserverquerykey));
	base64_encode(hash, 16, sizeof(hash));
	dpsnprintf(buffer, buffersize, "\377\377\377\377getstatus %.22s%" PRIx64, hash, timestamp);
}

static void NetConn_BuildChallengeString(char *buffer, int bufferlength);
void ServerList_QueryList(qbool resetcache, qbool querydp, qbool queryqw, qbool consoleoutput)
{
	unsigned i;
	lhnetaddress_t broadcastaddress;
	char dpquery[53]; // theoretical max: 14+22+16+1

	if (resetcache)
	{
		serverquerycount = 0;
		serverreplycount = 0;
		serverlist_cachecount = 0;
		serverlist_viewcount = 0;
		serverlist_maxcachecount = 0;
		serverlist_cache = (serverlist_entry_t *)Mem_Realloc(netconn_mempool, (void *)serverlist_cache, sizeof(serverlist_entry_t) * serverlist_maxcachecount);
	}
	else
	{
		if (serverlist_querystage)
		{
			if (net_slist_debug.integer)
				Con_Printf(CON_WARN "Ignoring server list refresh request: already refreshing!\n");
			return; // unsetting `responded` now would cause live servers to be timed out
		}

		// refresh all entries
		for (i = 0; i < serverlist_cachecount; ++i)
			serverlist_cache[i].responded = false;
	}
	serverlist_querystage = (querydp ? SLIST_QUERYSTAGE_DPMASTERS : 0) | (queryqw ? SLIST_QUERYSTAGE_QWMASTERS : 0);
	masterquerycount = 0;
	masterreplycount = 0;
	serverlist_consoleoutput = consoleoutput;
	if (net_slist_debug.integer)
		Con_Printf("^2Querying %s master, favourite and LAN servers, reset=%u\n",
				querydp && queryqw ? "DP and QW" : querydp ? "DP" : "QW",
				resetcache);

	//_ServerList_Test();

	NetConn_QueryMasters(querydp, queryqw);

	// Generate new DP server query key string
	// Used to prevent ping cheating and discard spurious getstatus/getinfo packets
	if (!serverlist_querystage) // don't change key while updating
		NetConn_BuildChallengeString(serverlist_dpserverquerykey, sizeof(serverlist_dpserverquerykey));

	// LAN search

	// Master and and/or favourite queries were likely delayed by DNS lag,
	// for correct pings we need to know what host.realtime would be if it were updated now.
	masterquerytime = host.realtime + (Sys_DirtyTime() - host.dirtytime);
	ServerList_BuildDPServerQuery(dpquery, sizeof(dpquery), masterquerytime);

	// 26000 is the default quake server port, servers on other ports will not be found
	// note this is IPv4-only, I doubt there are IPv6-only LANs out there
	LHNETADDRESS_FromString(&broadcastaddress, "255.255.255.255", 26000);

	for (i = 0; i < cl_numsockets; ++i)
	{
		if (!cl_sockets[i])
			continue;
		if (LHNETADDRESS_GetAddressType(LHNET_AddressFromSocket(cl_sockets[i])) != LHNETADDRESS_GetAddressType(&broadcastaddress))
			continue;

		if (querydp)
		{
			// search LAN for Quake servers
			SZ_Clear(&cl_message);
			// save space for the header, filled in later
			MSG_WriteLong(&cl_message, 0);
			MSG_WriteByte(&cl_message, CCREQ_SERVER_INFO);
			MSG_WriteString(&cl_message, "QUAKE");
			MSG_WriteByte(&cl_message, NET_PROTOCOL_VERSION);
			StoreBigLong(cl_message.data, NETFLAG_CTL | (cl_message.cursize & NETFLAG_LENGTH_MASK));
			NetConn_Write(cl_sockets[i], cl_message.data, cl_message.cursize, &broadcastaddress);
			SZ_Clear(&cl_message);

			// search LAN for DarkPlaces servers
			NetConn_WriteString(cl_sockets[i], dpquery, &broadcastaddress);
		}

		if (queryqw)
			// search LAN for QuakeWorld servers
			NetConn_WriteString(cl_sockets[i], "\377\377\377\377status\n", &broadcastaddress);
	}
}
#endif

// rest

int NetConn_Read(lhnetsocket_t *mysocket, void *data, int maxlength, lhnetaddress_t *peeraddress)
{
	int length;
	unsigned i;

	if (mysocket->address.addresstype == LHNETADDRESSTYPE_LOOP && netconn_mutex)
		Thread_LockMutex(netconn_mutex);
	length = LHNET_Read(mysocket, data, maxlength, peeraddress);
	if (mysocket->address.addresstype == LHNETADDRESSTYPE_LOOP && netconn_mutex)
		Thread_UnlockMutex(netconn_mutex);
	if (length == 0)
		return 0;
	if (net_fakeloss_receive.integer)
		for (i = 0;i < cl_numsockets;i++)
			if (cl_sockets[i] == mysocket && (rand() % 100) < net_fakeloss_receive.integer)
				return 0;
	if (developer_networking.integer)
	{
		char addressstring[128], addressstring2[128];
		LHNETADDRESS_ToString(LHNET_AddressFromSocket(mysocket), addressstring, sizeof(addressstring), true);
		if (length > 0)
		{
			LHNETADDRESS_ToString(peeraddress, addressstring2, sizeof(addressstring2), true);
			Con_Printf("LHNET_Read(%p (%s), %p, %i, %p) = %i from %s:\n", (void *)mysocket, addressstring, (void *)data, maxlength, (void *)peeraddress, length, addressstring2);
			Com_HexDumpToConsole((unsigned char *)data, length);
		}
		else
			Con_Printf("LHNET_Read(%p (%s), %p, %i, %p) = %i\n", (void *)mysocket, addressstring, (void *)data, maxlength, (void *)peeraddress, length);
	}
	return length;
}

int NetConn_Write(lhnetsocket_t *mysocket, const void *data, int length, const lhnetaddress_t *peeraddress)
{
	int ret;
	unsigned i;

	if (net_fakeloss_send.integer)
		for (i = 0;i < cl_numsockets;i++)
			if (cl_sockets[i] == mysocket && (rand() % 100) < net_fakeloss_send.integer)
				return length;
	if (mysocket->address.addresstype == LHNETADDRESSTYPE_LOOP && netconn_mutex)
		Thread_LockMutex(netconn_mutex);
	ret = LHNET_Write(mysocket, data, length, peeraddress);
	if (mysocket->address.addresstype == LHNETADDRESSTYPE_LOOP && netconn_mutex)
		Thread_UnlockMutex(netconn_mutex);
	if (developer_networking.integer)
	{
		char addressstring[128], addressstring2[128];
		LHNETADDRESS_ToString(LHNET_AddressFromSocket(mysocket), addressstring, sizeof(addressstring), true);
		LHNETADDRESS_ToString(peeraddress, addressstring2, sizeof(addressstring2), true);
		Con_Printf("LHNET_Write(%p (%s), %p, %i, %p (%s)) = %i%s\n", (void *)mysocket, addressstring, (void *)data, length, (void *)peeraddress, addressstring2, length, ret == length ? "" : " (ERROR)");
		Com_HexDumpToConsole((unsigned char *)data, length);
	}
	return ret;
}

int NetConn_WriteString(lhnetsocket_t *mysocket, const char *string, const lhnetaddress_t *peeraddress)
{
	// note this does not include the trailing NULL because we add that in the parser
	return NetConn_Write(mysocket, string, (int)strlen(string), peeraddress);
}

qbool NetConn_CanSend(netconn_t *conn)
{
	conn->outgoing_packetcounter = (conn->outgoing_packetcounter + 1) % NETGRAPH_PACKETS;
	conn->outgoing_netgraph[conn->outgoing_packetcounter].time            = host.realtime;
	conn->outgoing_netgraph[conn->outgoing_packetcounter].unreliablebytes = NETGRAPH_NOPACKET;
	conn->outgoing_netgraph[conn->outgoing_packetcounter].reliablebytes   = NETGRAPH_NOPACKET;
	conn->outgoing_netgraph[conn->outgoing_packetcounter].ackbytes        = NETGRAPH_NOPACKET;
	conn->outgoing_netgraph[conn->outgoing_packetcounter].cleartime       = conn->cleartime;
	if (host.realtime > conn->cleartime)
		return true;
	else
	{
		conn->outgoing_netgraph[conn->outgoing_packetcounter].unreliablebytes = NETGRAPH_CHOKEDPACKET;
		return false;
	}
}

static void NetConn_UpdateCleartime(double *cleartime, int rate, int burstsize, int len)
{
	double bursttime = burstsize / (double)rate;

	// delay later packets to obey rate limit
	if (*cleartime < host.realtime - bursttime)
		*cleartime = host.realtime - bursttime;
	*cleartime = *cleartime + len / (double)rate;

	// limit bursts to one packet in size ("dialup mode" emulating old behaviour)
	if (net_test.integer)
	{
		if (*cleartime < host.realtime)
			*cleartime = host.realtime;
	}
}

static int NetConn_AddCryptoFlag(crypto_t *crypto)
{
	// HACK: if an encrypted connection is used, randomly set some unused
	// flags. When AES encryption is enabled, that will make resends differ
	// from the original, so that e.g. substring filters in a router/IPS
	// are unlikely to match a second time. See also "startkeylogger".
	int flag = 0;
	if (crypto->authenticated)
	{
		// Let's always set at least one of the bits.
		int r = rand() % 7 + 1;
		if (r & 1)
			flag |= NETFLAG_CRYPTO0;
		if (r & 2)
			flag |= NETFLAG_CRYPTO1;
		if (r & 4)
			flag |= NETFLAG_CRYPTO2;
	}
	return flag;
}

int NetConn_SendUnreliableMessage(netconn_t *conn, sizebuf_t *data, protocolversion_t protocol, int rate, int burstsize, qbool quakesignon_suppressreliables)
{
	int totallen = 0;
	unsigned char sendbuffer[NET_HEADERSIZE+NET_MAXMESSAGE];
	unsigned char cryptosendbuffer[NET_HEADERSIZE+NET_MAXMESSAGE+CRYPTO_HEADERSIZE];

	// if this packet was supposedly choked, but we find ourselves sending one
	// anyway, make sure the size counting starts at zero
	// (this mostly happens on level changes and disconnects and such)
	if (conn->outgoing_netgraph[conn->outgoing_packetcounter].unreliablebytes == NETGRAPH_CHOKEDPACKET)
		conn->outgoing_netgraph[conn->outgoing_packetcounter].unreliablebytes = NETGRAPH_NOPACKET;

	conn->outgoing_netgraph[conn->outgoing_packetcounter].cleartime = conn->cleartime;

	if (protocol == PROTOCOL_QUAKEWORLD)
	{
		int packetLen;
		qbool sendreliable;

		// note that it is ok to send empty messages to the qw server,
		// otherwise it won't respond to us at all

		sendreliable = false;
		// if the remote side dropped the last reliable message, resend it
		if (conn->qw.incoming_acknowledged > conn->qw.last_reliable_sequence && conn->qw.incoming_reliable_acknowledged != conn->qw.reliable_sequence)
			sendreliable = true;
		// if the reliable transmit buffer is empty, copy the current message out
		if (!conn->sendMessageLength && conn->message.cursize)
		{
			memcpy (conn->sendMessage, conn->message.data, conn->message.cursize);
			conn->sendMessageLength = conn->message.cursize;
			SZ_Clear(&conn->message); // clear the message buffer
			conn->qw.reliable_sequence ^= 1;
			sendreliable = true;
		}
		// outgoing unreliable packet number, and outgoing reliable packet number (0 or 1)
		StoreLittleLong(sendbuffer, conn->outgoing_unreliable_sequence | (((unsigned int)sendreliable)<<31));
		// last received unreliable packet number, and last received reliable packet number (0 or 1)
		StoreLittleLong(sendbuffer + 4, conn->qw.incoming_sequence | (((unsigned int)conn->qw.incoming_reliable_sequence)<<31));
		packetLen = 8;
		conn->outgoing_unreliable_sequence++;
		// client sends qport in every packet
		if (conn == cls.netcon)
		{
			*((short *)(sendbuffer + 8)) = LittleShort(cls.qw_qport);
			packetLen += 2;
			// also update cls.qw_outgoing_sequence
			cls.qw_outgoing_sequence = conn->outgoing_unreliable_sequence;
		}
		if (packetLen + (sendreliable ? conn->sendMessageLength : 0) > 1400)
		{
			Con_Printf ("NetConn_SendUnreliableMessage: reliable message too big %u\n", data->cursize);
			return -1;
		}

		conn->outgoing_netgraph[conn->outgoing_packetcounter].unreliablebytes += packetLen + 28;

		// add the reliable message if there is one
		if (sendreliable)
		{
			conn->outgoing_netgraph[conn->outgoing_packetcounter].reliablebytes += conn->sendMessageLength + 28;
			memcpy(sendbuffer + packetLen, conn->sendMessage, conn->sendMessageLength);
			packetLen += conn->sendMessageLength;
			conn->qw.last_reliable_sequence = conn->outgoing_unreliable_sequence;
		}

		// add the unreliable message if possible
		if (packetLen + data->cursize <= 1400)
		{
			conn->outgoing_netgraph[conn->outgoing_packetcounter].unreliablebytes += data->cursize + 28;
			memcpy(sendbuffer + packetLen, data->data, data->cursize);
			packetLen += data->cursize;
		}

		NetConn_Write(conn->mysocket, (void *)&sendbuffer, packetLen, &conn->peeraddress);

		conn->packetsSent++;
		conn->unreliableMessagesSent++;

		totallen += packetLen + 28;
	}
	else
	{
		unsigned int packetLen;
		unsigned int dataLen;
		unsigned int eom;
		const void *sendme;
		size_t sendmelen;

		// if a reliable message fragment has been lost, send it again
		if (conn->sendMessageLength && (host.realtime - conn->lastSendTime) > 1.0)
		{
			if (conn->sendMessageLength <= MAX_PACKETFRAGMENT)
			{
				dataLen = conn->sendMessageLength;
				eom = NETFLAG_EOM;
			}
			else
			{
				dataLen = MAX_PACKETFRAGMENT;
				eom = 0;
			}

			packetLen = NET_HEADERSIZE + dataLen;

			StoreBigLong(sendbuffer, packetLen | (NETFLAG_DATA | eom | NetConn_AddCryptoFlag(&conn->crypto)));
			StoreBigLong(sendbuffer + 4, conn->nq.sendSequence - 1);
			memcpy(sendbuffer + NET_HEADERSIZE, conn->sendMessage, dataLen);

			conn->outgoing_netgraph[conn->outgoing_packetcounter].reliablebytes += packetLen + 28;

			sendme = Crypto_EncryptPacket(&conn->crypto, &sendbuffer, packetLen, &cryptosendbuffer, &sendmelen, sizeof(cryptosendbuffer));
			if (sendme && NetConn_Write(conn->mysocket, sendme, (int)sendmelen, &conn->peeraddress) == (int)sendmelen)
			{
				conn->lastSendTime = host.realtime;
				conn->packetsReSent++;
			}

			totallen += (int)sendmelen + 28;
		}

		// if we have a new reliable message to send, do so
		if (!conn->sendMessageLength && conn->message.cursize && !quakesignon_suppressreliables)
		{
			if (conn->message.cursize > (int)sizeof(conn->sendMessage))
			{
				Con_Printf("NetConn_SendUnreliableMessage: reliable message too big (%u > %u)\n", conn->message.cursize, (int)sizeof(conn->sendMessage));
				conn->message.overflowed = true;
				return -1;
			}

			if (developer_networking.integer && conn == cls.netcon)
			{
				Con_Print("client sending reliable message to server:\n");
				SZ_HexDumpToConsole(&conn->message);
			}

			memcpy(conn->sendMessage, conn->message.data, conn->message.cursize);
			conn->sendMessageLength = conn->message.cursize;
			SZ_Clear(&conn->message);

			if (conn->sendMessageLength <= MAX_PACKETFRAGMENT)
			{
				dataLen = conn->sendMessageLength;
				eom = NETFLAG_EOM;
			}
			else
			{
				dataLen = MAX_PACKETFRAGMENT;
				eom = 0;
			}

			packetLen = NET_HEADERSIZE + dataLen;

			StoreBigLong(sendbuffer, packetLen | (NETFLAG_DATA | eom | NetConn_AddCryptoFlag(&conn->crypto)));
			StoreBigLong(sendbuffer + 4, conn->nq.sendSequence);
			memcpy(sendbuffer + NET_HEADERSIZE, conn->sendMessage, dataLen);

			conn->nq.sendSequence++;

			conn->outgoing_netgraph[conn->outgoing_packetcounter].reliablebytes += packetLen + 28;

			sendme = Crypto_EncryptPacket(&conn->crypto, &sendbuffer, packetLen, &cryptosendbuffer, &sendmelen, sizeof(cryptosendbuffer));
			if(sendme)
				NetConn_Write(conn->mysocket, sendme, (int)sendmelen, &conn->peeraddress);

			conn->lastSendTime = host.realtime;
			conn->packetsSent++;
			conn->reliableMessagesSent++;

			totallen += (int)sendmelen + 28;
		}

		// if we have an unreliable message to send, do so
		if (data->cursize)
		{
			packetLen = NET_HEADERSIZE + data->cursize;

			if (packetLen > (int)sizeof(sendbuffer))
			{
				Con_Printf("NetConn_SendUnreliableMessage: message too big %u\n", data->cursize);
				return -1;
			}

			StoreBigLong(sendbuffer, packetLen | NETFLAG_UNRELIABLE | NetConn_AddCryptoFlag(&conn->crypto));
			StoreBigLong(sendbuffer + 4, conn->outgoing_unreliable_sequence);
			memcpy(sendbuffer + NET_HEADERSIZE, data->data, data->cursize);

			conn->outgoing_unreliable_sequence++;

			conn->outgoing_netgraph[conn->outgoing_packetcounter].unreliablebytes += packetLen + 28;

			sendme = Crypto_EncryptPacket(&conn->crypto, &sendbuffer, packetLen, &cryptosendbuffer, &sendmelen, sizeof(cryptosendbuffer));
			if(sendme)
				NetConn_Write(conn->mysocket, sendme, (int)sendmelen, &conn->peeraddress);

			conn->packetsSent++;
			conn->unreliableMessagesSent++;

			totallen += (int)sendmelen + 28;
		}
	}

	NetConn_UpdateCleartime(&conn->cleartime, rate, burstsize, totallen);

	return 0;
}

qbool NetConn_HaveClientPorts(void)
{
	return !!cl_numsockets;
}

qbool NetConn_HaveServerPorts(void)
{
	return !!sv_numsockets;
}

void NetConn_CloseClientPorts(void)
{
	for (;cl_numsockets > 0;cl_numsockets--)
		if (cl_sockets[cl_numsockets - 1])
			LHNET_CloseSocket(cl_sockets[cl_numsockets - 1]);
}

static void NetConn_OpenClientPort(const char *addressstring, lhnetaddresstype_t addresstype, int defaultport)
{
	lhnetaddress_t address;
	lhnetsocket_t *s;
	int success;
	char addressstring2[1024];
	if (addressstring && addressstring[0])
		success = LHNETADDRESS_FromString(&address, addressstring, defaultport);
	else
		success = LHNETADDRESS_FromPort(&address, addresstype, defaultport);
	if (success)
	{
		if ((s = LHNET_OpenSocket_Connectionless(&address)))
		{
			cl_sockets[cl_numsockets++] = s;
			LHNETADDRESS_ToString(LHNET_AddressFromSocket(s), addressstring2, sizeof(addressstring2), true);
			if (addresstype != LHNETADDRESSTYPE_LOOP)
				Con_Printf("Client opened a socket on address %s\n", addressstring2);
		}
		else
		{
			LHNETADDRESS_ToString(&address, addressstring2, sizeof(addressstring2), true);
			Con_Printf(CON_ERROR "Client failed to open a socket on address %s\n", addressstring2);
		}
	}
	else
		Con_Printf(CON_ERROR "Client unable to parse address %s\n", addressstring);
}

void NetConn_OpenClientPorts(void)
{
	int port;
	NetConn_CloseClientPorts();

	SV_LockThreadMutex(); // FIXME recursive?
	Crypto_LoadKeys(); // client sockets
	SV_UnlockThreadMutex();

	port = bound(0, cl_netport.integer, 65535);
	if (cl_netport.integer != port)
		Cvar_SetValueQuick(&cl_netport, port);
	if(port == 0)
		Con_Printf("Client using an automatically assigned port\n");
	else
		Con_Printf("Client using port %i\n", port);
	NetConn_OpenClientPort(NULL, LHNETADDRESSTYPE_LOOP, 2);
	NetConn_OpenClientPort(net_address.string, LHNETADDRESSTYPE_INET4, port);
#ifndef NOSUPPORTIPV6
	NetConn_OpenClientPort(net_address_ipv6.string, LHNETADDRESSTYPE_INET6, port);
#endif
}

void NetConn_CloseServerPorts(void)
{
	for (;sv_numsockets > 0;sv_numsockets--)
		if (sv_sockets[sv_numsockets - 1])
			LHNET_CloseSocket(sv_sockets[sv_numsockets - 1]);
}

static qbool NetConn_OpenServerPort(const char *addressstring, lhnetaddresstype_t addresstype, int defaultport, int range)
{
	lhnetaddress_t address;
	lhnetsocket_t *s;
	int port;
	char addressstring2[1024];
	int success;

	for (port = defaultport; port <= defaultport + range; port++)
	{
		if (addressstring && addressstring[0])
			success = LHNETADDRESS_FromString(&address, addressstring, port);
		else
			success = LHNETADDRESS_FromPort(&address, addresstype, port);
		if (success)
		{
			if ((s = LHNET_OpenSocket_Connectionless(&address)))
			{
				sv_sockets[sv_numsockets++] = s;
				LHNETADDRESS_ToString(LHNET_AddressFromSocket(s), addressstring2, sizeof(addressstring2), true);
				if (addresstype != LHNETADDRESSTYPE_LOOP)
					Con_Printf("Server listening on address %s\n", addressstring2);
				return true;
			}
			else
			{
				LHNETADDRESS_ToString(&address, addressstring2, sizeof(addressstring2), true);
				Con_Printf(CON_ERROR "Server failed to open socket on address %s\n", addressstring2);
			}
		}
		else
		{
			Con_Printf(CON_ERROR "Server unable to parse address %s\n", addressstring);
			// if it cant parse one address, it wont be able to parse another for sure
			return false;
		}
	}
	return false;
}

void NetConn_OpenServerPorts(int opennetports)
{
	int port;
	NetConn_CloseServerPorts();

	SV_LockThreadMutex(); // FIXME recursive?
	Crypto_LoadKeys(); // server sockets
	SV_UnlockThreadMutex();

	NetConn_UpdateSockets();
	port = bound(0, sv_netport.integer, 65535);
	if (port == 0)
		port = 26000;
	if (sv_netport.integer != port)
		Cvar_SetValueQuick(&sv_netport, port);
	if (cls.state != ca_dedicated)
		NetConn_OpenServerPort(NULL, LHNETADDRESSTYPE_LOOP, 1, 1);
	if (opennetports)
	{
#ifndef NOSUPPORTIPV6
		qbool ip4success = NetConn_OpenServerPort(net_address.string, LHNETADDRESSTYPE_INET4, port, 100);
		NetConn_OpenServerPort(net_address_ipv6.string, LHNETADDRESSTYPE_INET6, port, ip4success ? 1 : 100);
#else
		NetConn_OpenServerPort(net_address.string, LHNETADDRESSTYPE_INET4, port, 100);
#endif
	}
	if (sv_numsockets == 0)
		Host_Error("NetConn_OpenServerPorts: unable to open any ports!");
}

lhnetsocket_t *NetConn_ChooseClientSocketForAddress(lhnetaddress_t *address)
{
	unsigned i;
	lhnetaddresstype_t a = LHNETADDRESS_GetAddressType(address);

	for (i = 0;i < cl_numsockets;i++)
		if (cl_sockets[i] && LHNETADDRESS_GetAddressType(LHNET_AddressFromSocket(cl_sockets[i])) == a)
			return cl_sockets[i];
	return NULL;
}

lhnetsocket_t *NetConn_ChooseServerSocketForAddress(lhnetaddress_t *address)
{
	unsigned i;
	lhnetaddresstype_t a = LHNETADDRESS_GetAddressType(address);

	for (i = 0;i < sv_numsockets;i++)
		if (sv_sockets[i] && LHNETADDRESS_GetAddressType(LHNET_AddressFromSocket(sv_sockets[i])) == a)
			return sv_sockets[i];
	return NULL;
}

netconn_t *NetConn_Open(lhnetsocket_t *mysocket, lhnetaddress_t *peeraddress)
{
	netconn_t *conn;
	conn = (netconn_t *)Mem_Alloc(netconn_mempool, sizeof(*conn));
	conn->mysocket = mysocket;
	conn->peeraddress = *peeraddress;
	conn->lastMessageTime = host.realtime;
	conn->message.data = conn->messagedata;
	conn->message.maxsize = sizeof(conn->messagedata);
	conn->message.cursize = 0;
	// LadyHavoc: (inspired by ProQuake) use a short connect timeout to
	// reduce effectiveness of connection request floods
	conn->timeout = host.realtime + net_connecttimeout.value;
	LHNETADDRESS_ToString(&conn->peeraddress, conn->address, sizeof(conn->address), true);
	conn->next = netconn_list;
	netconn_list = conn;
	return conn;
}

void NetConn_ClearFlood(lhnetaddress_t *peeraddress, server_floodaddress_t *floodlist, size_t floodlength);
void NetConn_Close(netconn_t *conn)
{
	netconn_t *c;
	// remove connection from list

	// allow the client to reconnect immediately
	NetConn_ClearFlood(&(conn->peeraddress), sv.connectfloodaddresses, sizeof(sv.connectfloodaddresses) / sizeof(sv.connectfloodaddresses[0]));

	if (conn == netconn_list)
		netconn_list = conn->next;
	else
	{
		for (c = netconn_list;c;c = c->next)
		{
			if (c->next == conn)
			{
				c->next = conn->next;
				break;
			}
		}
		// not found in list, we'll avoid crashing here...
		if (!c)
			return;
	}
	// free connection
	Mem_Free(conn);
}

static int clientport = -1;
static int clientport2 = -1;

// Call on disconnect, during startup, or if cl_port/cl_netport is changed
static void NetConn_CL_UpdateSockets_Callback(cvar_t *var)
{
	if(cls.state != ca_dedicated)
	{
		if (clientport2 != var->integer)
		{
			clientport2 = var->integer;
			if (cls.state == ca_connected)
				Con_Print("Changing \"cl_port\" will not take effect until you reconnect.\n");
		}

		if (cls.state == ca_disconnected && clientport != clientport2)
		{
			clientport = clientport2;
			NetConn_CloseClientPorts();
		}
		if (cl_numsockets == 0)
			NetConn_OpenClientPorts();
	}
}

static int hostport = -1;

// Call when port/sv_netport is changed
static void NetConn_sv_netport_Callback(cvar_t *var)
{
	if (hostport != var->integer)
	{
		hostport = var->integer;
		if (sv.active)
			Con_Print("Changing \"port\" will not take effect until \"map\" command is executed.\n");
	}
}

void NetConn_UpdateSockets(void)
{
	int i, j;

	// TODO add logic to automatically close sockets if needed
	LHNET_DefaultDSCP(net_tos_dscp.integer);

	for (j = 0;j < MAX_RCONS;j++)
	{
		i = (cls.rcon_ringpos + j + 1) % MAX_RCONS;
		if(cls.rcon_commands[i][0])
		{
			if(host.realtime > cls.rcon_timeout[i])
			{
				char s[128];
				LHNETADDRESS_ToString(&cls.rcon_addresses[i], s, sizeof(s), true);
				Con_Printf("rcon to %s (for command %s) failed: challenge request timed out\n", s, cls.rcon_commands[i]);
				cls.rcon_commands[i][0] = 0;
				--cls.rcon_trying;
				break;
			}
		}
	}
}

static int NetConn_ReceivedMessage(netconn_t *conn, const unsigned char *data, size_t length, protocolversion_t protocol, double newtimeout)
{
	int originallength = (int)length;
	unsigned char sendbuffer[NET_HEADERSIZE+NET_MAXMESSAGE];
	unsigned char cryptosendbuffer[NET_HEADERSIZE+NET_MAXMESSAGE+CRYPTO_HEADERSIZE];
	unsigned char cryptoreadbuffer[NET_HEADERSIZE+NET_MAXMESSAGE+CRYPTO_HEADERSIZE];
	if (length < 8)
		return 0;

	if (protocol == PROTOCOL_QUAKEWORLD)
	{
		unsigned int sequence, sequence_ack;
		qbool reliable_ack, reliable_message;
		int count;
		//int qport;

		sequence = LittleLong(*((int *)(data + 0)));
		sequence_ack = LittleLong(*((int *)(data + 4)));
		data += 8;
		length -= 8;

		if (conn != cls.netcon)
		{
			// server only
			if (length < 2)
				return 0;
			// TODO: use qport to identify that this client really is who they say they are?  (and elsewhere in the code to identify the connection without a port match?)
			//qport = LittleShort(*((int *)(data + 8)));
			data += 2;
			length -= 2;
		}

		conn->packetsReceived++;
		reliable_message = (sequence >> 31) != 0;
		reliable_ack = (sequence_ack >> 31) != 0;
		sequence &= ~(1u<<31);
		sequence_ack &= ~(1u<<31);
		if (sequence <= conn->qw.incoming_sequence)
		{
			//Con_DPrint("Got a stale datagram\n");
			return 0;
		}
		count = sequence - (conn->qw.incoming_sequence + 1);
		if (count > 0)
		{
			conn->droppedDatagrams += count;
			//Con_DPrintf("Dropped %u datagram(s)\n", count);
			// If too may packets have been dropped, only write the
			// last NETGRAPH_PACKETS ones to the netgraph. Why?
			// Because there's no point in writing more than
			// these as the netgraph is going to be full anyway.
			if (count > NETGRAPH_PACKETS)
				count = NETGRAPH_PACKETS;
			while (count--)
			{
				conn->incoming_packetcounter = (conn->incoming_packetcounter + 1) % NETGRAPH_PACKETS;
				conn->incoming_netgraph[conn->incoming_packetcounter].time            = host.realtime;
				conn->incoming_netgraph[conn->incoming_packetcounter].cleartime       = conn->incoming_cleartime;
				conn->incoming_netgraph[conn->incoming_packetcounter].unreliablebytes = NETGRAPH_LOSTPACKET;
				conn->incoming_netgraph[conn->incoming_packetcounter].reliablebytes   = NETGRAPH_NOPACKET;
				conn->incoming_netgraph[conn->incoming_packetcounter].ackbytes        = NETGRAPH_NOPACKET;
			}
		}
		conn->incoming_packetcounter = (conn->incoming_packetcounter + 1) % NETGRAPH_PACKETS;
		conn->incoming_netgraph[conn->incoming_packetcounter].time            = host.realtime;
		conn->incoming_netgraph[conn->incoming_packetcounter].cleartime       = conn->incoming_cleartime;
		conn->incoming_netgraph[conn->incoming_packetcounter].unreliablebytes = originallength + 28;
		conn->incoming_netgraph[conn->incoming_packetcounter].reliablebytes   = NETGRAPH_NOPACKET;
		conn->incoming_netgraph[conn->incoming_packetcounter].ackbytes        = NETGRAPH_NOPACKET;
		NetConn_UpdateCleartime(&conn->incoming_cleartime, cl_rate.integer, cl_rate_burstsize.integer, originallength + 28);

		// limit bursts to one packet in size ("dialup mode" emulating old behaviour)
		if (net_test.integer)
		{
			if (conn->cleartime < host.realtime)
				conn->cleartime = host.realtime;
		}

		if (reliable_ack == conn->qw.reliable_sequence)
		{
			// received, now we will be able to send another reliable message
			conn->sendMessageLength = 0;
			conn->reliableMessagesReceived++;
		}
		conn->qw.incoming_sequence = sequence;
		if (conn == cls.netcon)
			cls.qw_incoming_sequence = conn->qw.incoming_sequence;
		conn->qw.incoming_acknowledged = sequence_ack;
		conn->qw.incoming_reliable_acknowledged = reliable_ack;
		if (reliable_message)
			conn->qw.incoming_reliable_sequence ^= 1;
		conn->lastMessageTime = host.realtime;
		conn->timeout = host.realtime + newtimeout;
		conn->unreliableMessagesReceived++;
		if (conn == cls.netcon)
		{
			SZ_Clear(&cl_message);
			SZ_Write(&cl_message, data, (int)length);
			MSG_BeginReading(&cl_message);
		}
		else
		{
			SZ_Clear(&sv_message);
			SZ_Write(&sv_message, data, (int)length);
			MSG_BeginReading(&sv_message);
		}
		return 2;
	}
	else
	{
		unsigned int count;
		unsigned int flags;
		unsigned int sequence;
		size_t qlength;
		const void *sendme;
		size_t sendmelen;

		originallength = (int)length;
		data = (const unsigned char *) Crypto_DecryptPacket(&conn->crypto, data, length, cryptoreadbuffer, &length, sizeof(cryptoreadbuffer));
		if(!data)
			return 0;
		if(length < 8)
			return 0;

		qlength = (unsigned int)BuffBigLong(data);
		flags = qlength & ~NETFLAG_LENGTH_MASK;
		qlength &= NETFLAG_LENGTH_MASK;
		// control packets were already handled
		if (!(flags & NETFLAG_CTL) && qlength == length)
		{
			sequence = BuffBigLong(data + 4);
			conn->packetsReceived++;
			data += 8;
			length -= 8;
			if (flags & NETFLAG_UNRELIABLE)
			{
				if (sequence >= conn->nq.unreliableReceiveSequence)
				{
					if (sequence > conn->nq.unreliableReceiveSequence)
					{
						count = sequence - conn->nq.unreliableReceiveSequence;
						conn->droppedDatagrams += count;
						//Con_DPrintf("Dropped %u datagram(s)\n", count);
						// If too may packets have been dropped, only write the
						// last NETGRAPH_PACKETS ones to the netgraph. Why?
						// Because there's no point in writing more than
						// these as the netgraph is going to be full anyway.
						if (count > NETGRAPH_PACKETS)
							count = NETGRAPH_PACKETS;
						while (count--)
						{
							conn->incoming_packetcounter = (conn->incoming_packetcounter + 1) % NETGRAPH_PACKETS;
							conn->incoming_netgraph[conn->incoming_packetcounter].time            = host.realtime;
							conn->incoming_netgraph[conn->incoming_packetcounter].cleartime       = conn->incoming_cleartime;
							conn->incoming_netgraph[conn->incoming_packetcounter].unreliablebytes = NETGRAPH_LOSTPACKET;
							conn->incoming_netgraph[conn->incoming_packetcounter].reliablebytes   = NETGRAPH_NOPACKET;
							conn->incoming_netgraph[conn->incoming_packetcounter].ackbytes        = NETGRAPH_NOPACKET;
						}
					}
					conn->incoming_packetcounter = (conn->incoming_packetcounter + 1) % NETGRAPH_PACKETS;
					conn->incoming_netgraph[conn->incoming_packetcounter].time            = host.realtime;
					conn->incoming_netgraph[conn->incoming_packetcounter].cleartime       = conn->incoming_cleartime;
					conn->incoming_netgraph[conn->incoming_packetcounter].unreliablebytes = originallength + 28;
					conn->incoming_netgraph[conn->incoming_packetcounter].reliablebytes   = NETGRAPH_NOPACKET;
					conn->incoming_netgraph[conn->incoming_packetcounter].ackbytes        = NETGRAPH_NOPACKET;
					NetConn_UpdateCleartime(&conn->incoming_cleartime, cl_rate.integer, cl_rate_burstsize.integer, originallength + 28);

					conn->nq.unreliableReceiveSequence = sequence + 1;
					conn->lastMessageTime = host.realtime;
					conn->timeout = host.realtime + newtimeout;
					conn->unreliableMessagesReceived++;
					if (length > 0)
					{
						if (conn == cls.netcon)
						{
							SZ_Clear(&cl_message);
							SZ_Write(&cl_message, data, (int)length);
							MSG_BeginReading(&cl_message);
						}
						else
						{
							SZ_Clear(&sv_message);
							SZ_Write(&sv_message, data, (int)length);
							MSG_BeginReading(&sv_message);
						}
						return 2;
					}
				}
				//else
				//	Con_DPrint("Got a stale datagram\n");
				return 1;
			}
			else if (flags & NETFLAG_ACK)
			{
				conn->incoming_netgraph[conn->incoming_packetcounter].ackbytes += originallength + 28;
				NetConn_UpdateCleartime(&conn->incoming_cleartime, cl_rate.integer, cl_rate_burstsize.integer, originallength + 28);

				if (sequence == (conn->nq.sendSequence - 1))
				{
					if (sequence == conn->nq.ackSequence)
					{
						conn->nq.ackSequence++;
						if (conn->nq.ackSequence != conn->nq.sendSequence)
							Con_DPrint("ack sequencing error\n");
						conn->lastMessageTime = host.realtime;
						conn->timeout = host.realtime + newtimeout;
						if (conn->sendMessageLength > MAX_PACKETFRAGMENT)
						{
							unsigned int packetLen;
							unsigned int dataLen;
							unsigned int eom;

							conn->sendMessageLength -= MAX_PACKETFRAGMENT;
							memmove(conn->sendMessage, conn->sendMessage+MAX_PACKETFRAGMENT, conn->sendMessageLength);

							if (conn->sendMessageLength <= MAX_PACKETFRAGMENT)
							{
								dataLen = conn->sendMessageLength;
								eom = NETFLAG_EOM;
							}
							else
							{
								dataLen = MAX_PACKETFRAGMENT;
								eom = 0;
							}

							packetLen = NET_HEADERSIZE + dataLen;

							StoreBigLong(sendbuffer, packetLen | (NETFLAG_DATA | eom | NetConn_AddCryptoFlag(&conn->crypto)));
							StoreBigLong(sendbuffer + 4, conn->nq.sendSequence);
							memcpy(sendbuffer + NET_HEADERSIZE, conn->sendMessage, dataLen);

							conn->nq.sendSequence++;

							sendme = Crypto_EncryptPacket(&conn->crypto, &sendbuffer, packetLen, &cryptosendbuffer, &sendmelen, sizeof(cryptosendbuffer));
							if (sendme && NetConn_Write(conn->mysocket, sendme, (int)sendmelen, &conn->peeraddress) == (int)sendmelen)
							{
								conn->lastSendTime = host.realtime;
								conn->packetsSent++;
							}
						}
						else
							conn->sendMessageLength = 0;
					}
					//else
					//	Con_DPrint("Duplicate ACK received\n");
				}
				//else
				//	Con_DPrint("Stale ACK received\n");
				return 1;
			}
			else if (flags & NETFLAG_DATA)
			{
				unsigned char temppacket[8];
				conn->incoming_netgraph[conn->incoming_packetcounter].reliablebytes   += originallength + 28;
				NetConn_UpdateCleartime(&conn->incoming_cleartime, cl_rate.integer, cl_rate_burstsize.integer, originallength + 28);

				conn->outgoing_netgraph[conn->outgoing_packetcounter].ackbytes        += 8 + 28;

				StoreBigLong(temppacket, 8 | NETFLAG_ACK | NetConn_AddCryptoFlag(&conn->crypto));
				StoreBigLong(temppacket + 4, sequence);
				sendme = Crypto_EncryptPacket(&conn->crypto, temppacket, 8, &cryptosendbuffer, &sendmelen, sizeof(cryptosendbuffer));
				if(sendme)
					NetConn_Write(conn->mysocket, sendme, (int)sendmelen, &conn->peeraddress);
				if (sequence == conn->nq.receiveSequence)
				{
					conn->lastMessageTime = host.realtime;
					conn->timeout = host.realtime + newtimeout;
					conn->nq.receiveSequence++;
					if( conn->receiveMessageLength + length <= (int)sizeof( conn->receiveMessage ) ) {
						memcpy(conn->receiveMessage + conn->receiveMessageLength, data, length);
						conn->receiveMessageLength += (int)length;
					} else {
						Con_Printf( "Reliable message (seq: %i) too big for message buffer!\n"
									"Dropping the message!\n", sequence );
						conn->receiveMessageLength = 0;
						return 1;
					}
					if (flags & NETFLAG_EOM)
					{
						conn->reliableMessagesReceived++;
						length = conn->receiveMessageLength;
						conn->receiveMessageLength = 0;
						if (length > 0)
						{
							if (conn == cls.netcon)
							{
								SZ_Clear(&cl_message);
								SZ_Write(&cl_message, conn->receiveMessage, (int)length);
								MSG_BeginReading(&cl_message);
							}
							else
							{
								SZ_Clear(&sv_message);
								SZ_Write(&sv_message, conn->receiveMessage, (int)length);
								MSG_BeginReading(&sv_message);
							}
							return 2;
						}
					}
				}
				else
					conn->receivedDuplicateCount++;
				return 1;
			}
		}
	}
	return 0;
}

static void NetConn_ConnectionEstablished(lhnetsocket_t *mysocket, lhnetaddress_t *peeraddress, protocolversion_t initialprotocol)
{
	crypto_t *crypto;

	cls.connect_trying = false;
	// Disconnect from the current server or stop demo playback
	if(cls.state == ca_connected || cls.demoplayback)
		CL_Disconnect();
	// allocate a net connection to keep track of things
	cls.netcon = NetConn_Open(mysocket, peeraddress);
	dp_strlcpy(cl_connect_status, "Connection established", sizeof(cl_connect_status));
	crypto = &cls.netcon->crypto;
	if(cls.crypto.authenticated)
	{
		Crypto_FinishInstance(crypto, &cls.crypto);
		Con_Printf("%s connection to %s has been established: server is %s@%s%.*s, I am %.*s@%s%.*s\n",
				crypto->use_aes ? "Encrypted" : "Authenticated",
				cls.netcon->address,
				crypto->server_idfp[0] ? crypto->server_idfp : "-",
				(crypto->server_issigned || !crypto->server_keyfp[0]) ? "" : "~",
				crypto_keyfp_recommended_length, crypto->server_keyfp[0] ? crypto->server_keyfp : "-",
				crypto_keyfp_recommended_length, crypto->client_idfp[0] ? crypto->client_idfp : "-",
				(crypto->client_issigned || !crypto->client_keyfp[0]) ? "" : "~",
				crypto_keyfp_recommended_length, crypto->client_keyfp[0] ? crypto->client_keyfp : "-"
				);
	}
	else
		Con_Printf("%s to %s\n", cl_connect_status, cls.netcon->address);

	key_dest = key_game;
#ifdef CONFIG_MENU
	m_state = m_none;
#endif
	cls.demonum = -1;			// not in the demo loop now
	cls.state = ca_connected;
	cls.signon = 0;				// need all the signon messages before playing
	cls.protocol = initialprotocol;
	// reset move sequence numbering on this new connection
	cls.servermovesequence = 0;
	if (cls.protocol == PROTOCOL_QUAKEWORLD)
		CL_ForwardToServer("new");
	if (cls.protocol == PROTOCOL_QUAKE)
	{
		// write a keepalive (clc_nop) as it seems to greatly improve the
		// chances of connecting to a netquake server
		sizebuf_t msg;
		unsigned char buf[4];
		memset(&msg, 0, sizeof(msg));
		msg.data = buf;
		msg.maxsize = sizeof(buf);
		MSG_WriteChar(&msg, clc_nop);
		NetConn_SendUnreliableMessage(cls.netcon, &msg, cls.protocol, 10000, 0, false);
	}
}

int NetConn_IsLocalGame(void)
{
	if (cls.state == ca_connected && sv.active && cl.maxclients == 1)
		return true;
	return false;
}

#ifdef CONFIG_MENU
static qbool hmac_mdfour_time_matching(lhnetaddress_t *peeraddress, const char *password, const char *hash, const char *s, int slen);
static int NetConn_ClientParsePacket_ServerList_ProcessReply(const char *addressstring, const char *challenge)
{
	unsigned n;
	float ping;
	double currentrealtime;
	serverlist_entry_t *entry;

	// search the cache for this server and update it
	for (n = 0; n < serverlist_cachecount; ++n)
	{
		entry = &serverlist_cache[n];
		if (!strcmp(addressstring, entry->info.cname))
			break;
	}

	if (n == serverlist_cachecount)
	{
		if (net_slist_debug.integer)
			Con_Printf("^6Received reply from unlisted %sserver %s\n", challenge ? "DarkPlaces " : "", addressstring);

		// find a slot
		if (serverlist_cachecount >= SERVERLIST_TOTALSIZE)
			return -1;

		if (serverlist_maxcachecount <= serverlist_cachecount)
		{
			serverlist_maxcachecount += 64;
			serverlist_cache = (serverlist_entry_t *)Mem_Realloc(netconn_mempool, (void *)serverlist_cache, sizeof(serverlist_entry_t) * serverlist_maxcachecount);
		}
		++serverlist_cachecount;
		entry = &serverlist_cache[n];

		memset(entry, 0, sizeof(*entry));
		entry->info.cname_len = dp_strlcpy(entry->info.cname, addressstring, sizeof(entry->info.cname));

		// use the broadcast as the first query, NetConn_QueryQueueFrame() will send more
		entry->querytime = masterquerytime;
		// protocol is one of these at all
		// NetConn_ClientParsePacket_ServerList_PrepareQuery() callsites
		entry->protocol = challenge ? PROTOCOL_DARKPLACES7 : PROTOCOL_QUAKEWORLD;

		if (serverlist_consoleoutput)
			Con_Printf("querying %s\n", addressstring);
	}

	// If the client stalls partway through a frame (test command: `alias a a;a`)
	// for correct pings we need to know what host.realtime would be if it were updated now.
	currentrealtime = host.realtime + (Sys_DirtyTime() - host.dirtytime);

	if (challenge)
	{
		unsigned char hash[24]; // 4*(16/3) rounded up to 4 byte multiple
		uint64_t timestamp = strtoull(&challenge[22], NULL, 16);

		HMAC_MDFOUR_16BYTES(hash,
			(unsigned char *)&timestamp, sizeof(timestamp),
			(unsigned char *)serverlist_dpserverquerykey, sizeof(serverlist_dpserverquerykey));
		base64_encode(hash, 16, sizeof(hash));
		if (memcmp(hash, challenge, 22) != 0)
			return -1;

		ping = currentrealtime * 1000.0 - timestamp;
	}
	else
		ping = 1000 * (currentrealtime - entry->querytime);

	if (ping <= 0 || ping > net_slist_maxping.value
	|| (entry->info.ping && ping > entry->info.ping + 100)) // server loading map, client stall, etc
		return -1;

	// never round down to 0, 0 latency is impossible, 0 means no data available
	if (ping < 1)
		ping = 1;

	if (entry->info.ping)
		entry->info.ping = (entry->info.ping + ping) * 0.5 + 0.5; // "average" biased toward most recent results
	else
	{
		entry->info.ping = ping + 0.5;
		serverreplycount++;
	}
	entry->responded = true;

	// other server info is updated by the caller
	return n;
}

static void NetConn_ClientParsePacket_ServerList_UpdateCache(int n)
{
	serverlist_entry_t *entry = &serverlist_cache[n];
	serverlist_info_t *info = &entry->info;

	// update description strings for engine menu and console output
	entry->line1_len = dpsnprintf(entry->line1, sizeof(serverlist_cache[n].line1), "^%c%5.0f^7 ^%c%3u^7/%3u %-65.65s",
	           info->ping >= 300 ? '1' : (info->ping >= 200 ? '3' : '7'),
	           info->ping ? info->ping : INFINITY, // display inf when a listed server times out and net_slist_pause blocks its removal
	           ((info->numhumans > 0 && info->numhumans < info->maxplayers) ? (info->numhumans >= 4 ? '7' : '3') : '1'),
	           info->numplayers,
	           info->maxplayers,
	           info->name);
	entry->line2_len = dpsnprintf(entry->line2, sizeof(serverlist_cache[n].line2), "^4%-21.21s %-19.19s ^%c%-17.17s^4 %-20.20s", info->cname, info->game,
			(
			 info->gameversion != gameversion.integer
			 &&
			 !(
				    gameversion_min.integer >= 0 // min/max range set by user/mod?
				 && gameversion_max.integer >= 0
				 && gameversion_min.integer <= info->gameversion // version of server in min/max range?
				 && gameversion_max.integer >= info->gameversion
			  )
			) ? '1' : '4',
			info->mod, info->map);

	if(!net_slist_pause.integer)
	{
		ServerList_ViewList_Remove(entry);
		ServerList_ViewList_Insert(entry);
	}

	if (serverlist_consoleoutput)
		Con_Printf("%s\n%s\n", entry->line1, entry->line2);
}

// returns true, if it's sensible to continue the processing
static qbool NetConn_ClientParsePacket_ServerList_PrepareQuery(int protocol, const char *ipstring, qbool isfavorite)
{
	unsigned n;
	serverlist_entry_t *entry;

	// ignore the rest of the message if the serverlist is full
	if (serverlist_cachecount >= SERVERLIST_TOTALSIZE)
		return false;

	for (n = 0; n < serverlist_cachecount; ++n)
		if (!strcmp(ipstring, serverlist_cache[n].info.cname))
			break;

	// also ignore it if we have already queried it (other master server response)
	if (n < serverlist_cachecount)
		return true;

	if (serverlist_maxcachecount <= n)
	{
		serverlist_maxcachecount += 64;
		serverlist_cache = (serverlist_entry_t *)Mem_Realloc(netconn_mempool, (void *)serverlist_cache, sizeof(serverlist_entry_t) * serverlist_maxcachecount);
	}

	entry = &serverlist_cache[n];
	memset(entry, 0, sizeof(*entry));
	entry->protocol = protocol;
	entry->info.cname_len = dp_strlcpy(entry->info.cname, ipstring, sizeof(entry->info.cname));
	entry->info.isfavorite = isfavorite;

	serverlist_cachecount++;
	serverquerycount++;

	return true;
}

static void NetConn_ClientParsePacket_ServerList_ParseDPList(lhnetaddress_t *masteraddress, const char *masteraddressstring, const unsigned char *data, int length, qbool isextended)
{
	unsigned masternum;
	lhnetaddress_t testaddress;

	for (masternum = 0; masternum < DPMASTER_COUNT; ++masternum)
		if (sv_masters[masternum].string[0]
		&& LHNETADDRESS_FromString(&testaddress, sv_masters[masternum].string, DPMASTER_PORT)
		&& LHNETADDRESS_Compare(&testaddress, masteraddress) == 0)
			break;
	if (net_sourceaddresscheck.integer && masternum >= DPMASTER_COUNT)
	{
		Con_Printf(CON_WARN "ignoring DarkPlaces %sserver list from unrecognised master %s\n",
				isextended ? "extended " : "", masteraddressstring);
		return;
	}

	masterreplycount++;
	if (dpmasterstatus[masternum] < MASTER_RX_RESPONSE)   // Don't reduce status if it's already COMPLETE
		dpmasterstatus[masternum] = MASTER_RX_RESPONSE; // which happens when packets are out of order.

	if (dpmasterstatus[masternum] == MASTER_RX_COMPLETE)
		Con_Printf(CON_WARN "Received out of order DarkPlaces server list %sfrom %s\n",
				isextended ? "(extended) " : "", sv_masters[masternum].string);
	else if (serverlist_consoleoutput || net_slist_debug.integer)
		Con_Printf("^5Received DarkPlaces server list %sfrom %s\n",
				isextended ? "(extended) " : "", sv_masters[masternum].string);

	while (length >= 7)
	{
		char ipstring [128];

		// IPv4 address
		if (data[0] == '\\')
		{
			unsigned short port = data[5] * 256 + data[6];

			if (port != 0 && (data[1] != 0xFF || data[2] != 0xFF || data[3] != 0xFF || data[4] != 0xFF))
				dpsnprintf (ipstring, sizeof (ipstring), "%u.%u.%u.%u:%hu", data[1], data[2], data[3], data[4], port);
			else if (port == 0 && data[1] == 'E' && data[2] == 'O' && data[3] == 'T' && data[4] == '\0')
			{
				dpmasterstatus[masternum] = MASTER_RX_COMPLETE;
				if (net_slist_debug.integer)
					Con_Printf("^4End Of Transmission %sfrom %s\n", isextended ? "(extended) " : "", sv_masters[masternum].string);
				break;
			}

			// move on to next address in packet
			data += 7;
			length -= 7;
		}
		// IPv6 address
		else if (data[0] == '/' && isextended && length >= 19)
		{
			unsigned short port = data[17] * 256 + data[18];

			if (port != 0)
			{
#ifdef WHY_JUST_WHY
				const char *ifname;
				char ifnamebuf[16];

				/// \TODO: make some basic checks of the IP address (broadcast, ...)

				ifname = LHNETADDRESS_GetInterfaceName(senderaddress, ifnamebuf, sizeof(ifnamebuf));
				if (ifname != NULL)
				{
					dpsnprintf (ipstring, sizeof (ipstring), "[%x:%x:%x:%x:%x:%x:%x:%x%%%s]:%hu",
								(data[1] << 8) | data[2], (data[3] << 8) | data[4], (data[5] << 8) | data[6], (data[7] << 8) | data[8],
								(data[9] << 8) | data[10], (data[11] << 8) | data[12], (data[13] << 8) | data[14], (data[15] << 8) | data[16],
								ifname, port);
				}
				else
#endif
				{
					dpsnprintf (ipstring, sizeof (ipstring), "[%x:%x:%x:%x:%x:%x:%x:%x]:%hu",
								(data[1] << 8) | data[2], (data[3] << 8) | data[4], (data[5] << 8) | data[6], (data[7] << 8) | data[8],
								(data[9] << 8) | data[10], (data[11] << 8) | data[12], (data[13] << 8) | data[14], (data[15] << 8) | data[16],
								port);
				}
			}

			// move on to next address in packet
			data += 19;
			length -= 19;
		}
		else
		{
			Con_Print(CON_WARN "Error while parsing the server list\n");
			break;
		}

		if (serverlist_consoleoutput && developer_networking.integer)
			Con_Printf("Requesting info from DarkPlaces server %s\n", ipstring);

		if (!NetConn_ClientParsePacket_ServerList_PrepareQuery(PROTOCOL_DARKPLACES7, ipstring, false))
			break;
	}

	if (serverlist_querystage & SLIST_QUERYSTAGE_QWMASTERS)
		return; // we must wait if we're (also) querying QW as its protocol has no EOT marker
	// begin or resume serverlist queries
	for (masternum = 0; masternum < DPMASTER_COUNT; ++masternum)
		if (dpmasterstatus[masternum] && dpmasterstatus[masternum] < MASTER_RX_COMPLETE)
			break; // was queried but no EOT marker received yet
	if (masternum >= DPMASTER_COUNT)
	{
		serverlist_querystage = SLIST_QUERYSTAGE_SERVERS;
		if (net_slist_debug.integer)
			Con_Print("^2Starting to query servers early (got EOT from all masters)\n");
	}
}

static void NetConn_ClientParsePacket_ServerList_ParseQWList(lhnetaddress_t *masteraddress, const char *masteraddressstring, const unsigned char *data, int length)
{
	uint8_t masternum;
	lhnetaddress_t testaddress;

	for (masternum = 0; masternum < QWMASTER_COUNT; ++masternum)
		if (sv_qwmasters[masternum].string[0]
		&& LHNETADDRESS_FromString(&testaddress, sv_qwmasters[masternum].string, QWMASTER_PORT)
		&& LHNETADDRESS_Compare(&testaddress, masteraddress) == 0)
			break;
	if (net_sourceaddresscheck.integer && masternum >= QWMASTER_COUNT)
	{
		Con_Printf(CON_WARN "ignoring QuakeWorld server list from unrecognised master %s\n", masteraddressstring);
		return;
	}

	masterreplycount++;
	qwmasterstatus[masternum] = MASTER_RX_RESPONSE;
	if (serverlist_consoleoutput || net_slist_debug.integer)
		Con_Printf("^5Received QuakeWorld server list from %s\n", sv_qwmasters[masternum].string);

	while (length >= 6 && (data[0] != 0xFF || data[1] != 0xFF || data[2] != 0xFF || data[3] != 0xFF) && data[4] * 256 + data[5] != 0)
	{
		char ipstring[32];

		dpsnprintf (ipstring, sizeof (ipstring), "%u.%u.%u.%u:%u", data[0], data[1], data[2], data[3], data[4] * 256 + data[5]);
		if (serverlist_consoleoutput && developer_networking.integer)
			Con_Printf("Requesting info from QuakeWorld server %s\n", ipstring);

		if (!NetConn_ClientParsePacket_ServerList_PrepareQuery(PROTOCOL_QUAKEWORLD, ipstring, false))
			break;

		// move on to next address in packet
		data += 6;
		length -= 6;
	}

	// Unlike in NetConn_ClientParsePacket_ServerList_ParseDPList()
	// we can't start to query servers early here because QW has no EOT marker.
}
#endif

static int NetConn_ClientParsePacket(lhnetsocket_t *mysocket, unsigned char *data, int length, lhnetaddress_t *peeraddress)
{
	qbool fromserver;
	int ret, c;
	char *string, addressstring2[128];
	char stringbuf[16384];
	char senddata[NET_HEADERSIZE+NET_MAXMESSAGE+CRYPTO_HEADERSIZE];
	size_t sendlength;
#ifdef CONFIG_MENU
	char infostringvalue[MAX_INPUTLINE];
#endif

	// quakeworld ingame packet
	fromserver = cls.netcon && mysocket == cls.netcon->mysocket && !LHNETADDRESS_Compare(&cls.netcon->peeraddress, peeraddress);

	// convert the address to a string incase we need it
	LHNETADDRESS_ToString(peeraddress, addressstring2, sizeof(addressstring2), true);

	if (length >= 5 && data[0] == 255 && data[1] == 255 && data[2] == 255 && data[3] == 255)
	{
		// received a command string - strip off the packaging and put it
		// into our string buffer with NULL termination
		data += 4;
		length -= 4;
		length = min(length, (int)sizeof(stringbuf) - 1);
		memcpy(stringbuf, data, length);
		stringbuf[length] = 0;
		string = stringbuf;

		if (developer_networking.integer)
		{
			Con_Printf("NetConn_ClientParsePacket: %s sent us a command:\n", addressstring2);
			Com_HexDumpToConsole(data, length);
		}

		sendlength = sizeof(senddata) - 4;
		switch(Crypto_ClientParsePacket(string, length, senddata+4, &sendlength, peeraddress, addressstring2))
		{
			case CRYPTO_NOMATCH:
				// nothing to do
				break;
			case CRYPTO_MATCH:
				if(sendlength)
				{
					memcpy(senddata, "\377\377\377\377", 4);
					NetConn_Write(mysocket, senddata, (int)sendlength+4, peeraddress);
				}
				break;
			case CRYPTO_DISCARD:
				if(sendlength)
				{
					memcpy(senddata, "\377\377\377\377", 4);
					NetConn_Write(mysocket, senddata, (int)sendlength+4, peeraddress);
				}
				return true;
				break;
			case CRYPTO_REPLACE:
				string = senddata+4;
				length = (int)sendlength;
				break;
		}

		if (length >= 10 && !memcmp(string, "challenge ", 10) && cls.rcon_trying)
		{
			int i = 0, j;
			for (j = 0;j < MAX_RCONS;j++)
			{
				// note: this value from i is used outside the loop too...
				i = (cls.rcon_ringpos + j) % MAX_RCONS;
				if(cls.rcon_commands[i][0])
					if (!LHNETADDRESS_Compare(peeraddress, &cls.rcon_addresses[i]))
						break;
			}
			if (j < MAX_RCONS)
			{
				char buf[1500];
				char argbuf[1500];
				const char *e;
				int n;
				dpsnprintf(argbuf, sizeof(argbuf), "%s %s", string + 10, cls.rcon_commands[i]);
				memcpy(buf, "\377\377\377\377srcon HMAC-MD4 CHALLENGE ", 29);

				e = strchr(rcon_password.string, ' ');
				n = e ? e-rcon_password.string : (int)strlen(rcon_password.string);

				if(HMAC_MDFOUR_16BYTES((unsigned char *) (buf + 29), (unsigned char *) argbuf, (int)strlen(argbuf), (unsigned char *) rcon_password.string, n))
				{
					int k;
					buf[45] = ' ';
					dp_strlcpy(buf + 46, argbuf, sizeof(buf) - 46);
					NetConn_Write(mysocket, buf, 46 + (int)strlen(buf + 46), peeraddress);
					cls.rcon_commands[i][0] = 0;
					--cls.rcon_trying;

					for (k = 0;k < MAX_RCONS;k++)
						if(cls.rcon_commands[k][0])
							if (!LHNETADDRESS_Compare(peeraddress, &cls.rcon_addresses[k]))
								break;
					if(k < MAX_RCONS)
					{
						int l;
						NetConn_WriteString(mysocket, "\377\377\377\377getchallenge", peeraddress);
						// extend the timeout on other requests as we asked for a challenge
						for (l = 0;l < MAX_RCONS;l++)
							if(cls.rcon_commands[l][0])
								if (!LHNETADDRESS_Compare(peeraddress, &cls.rcon_addresses[l]))
									cls.rcon_timeout[l] = host.realtime + rcon_secure_challengetimeout.value;
					}

					return true; // we used up the challenge, so we can't use this oen for connecting now anyway
				}
			}
		}
		if (length >= 10 && !memcmp(string, "challenge ", 10) && cls.connect_trying)
		{
			// darkplaces or quake3
			char protocolnames[1400];

			if (net_sourceaddresscheck.integer && LHNETADDRESS_Compare(peeraddress, &cls.connect_address)) {
				Con_Printf(CON_WARN "ignoring challenge message from wrong server %s\n", addressstring2);
				return true;
			}
			Con_DPrintf("\"%s\" received, sending connect request back to %s\n", string, addressstring2);
			dp_strlcpy(cl_connect_status, "Connect: replying to challenge...", sizeof(cl_connect_status));

			Protocol_Names(protocolnames, sizeof(protocolnames));
			// update the server IP in the userinfo (QW servers expect this, and it is used by the reconnect command)
			InfoString_SetValue(cls.userinfo, sizeof(cls.userinfo), "*ip", addressstring2);
			// TODO: add userinfo stuff here instead of using NQ commands?
			memcpy(senddata, "\377\377\377\377", 4);
			dpsnprintf(senddata+4, sizeof(senddata)-4, "connect\\protocol\\darkplaces 3\\protocols\\%s%s\\challenge\\%s", protocolnames, cls.connect_userinfo, string + 10);
			NetConn_WriteString(mysocket, senddata, peeraddress);
			return true;
		}
		if (length == 6 && !memcmp(string, "accept", 6) && cls.connect_trying)
		{
			// darkplaces or quake3
			if (net_sourceaddresscheck.integer && LHNETADDRESS_Compare(peeraddress, &cls.connect_address)) {
				Con_Printf(CON_WARN "ignoring accept message from wrong server %s\n", addressstring2);
				return true;
			}
			NetConn_ConnectionEstablished(mysocket, peeraddress, PROTOCOL_DARKPLACES3);
			return true;
		}
		if (length > 7 && !memcmp(string, "reject ", 7) && cls.connect_trying)
		{
			if (net_sourceaddresscheck.integer && LHNETADDRESS_Compare(peeraddress, &cls.connect_address)) {
				Con_Printf(CON_WARN "ignoring reject message from wrong server %s\n", addressstring2);
				return true;
			}
			cls.connect_trying = false;
			string += 7;
			length = min(length - 7, (int)sizeof(cl_connect_status) - 1);
			dpsnprintf(cl_connect_status, sizeof(cl_connect_status), "Connect: rejected, %.*s", length, string);
			Con_Printf(CON_ERROR "Connect: rejected by %s\n" CON_ERROR "%.*s\n", addressstring2, length, string);
			return true;
		}
#ifdef CONFIG_MENU
		if(key_dest != key_game)
		{
			if (length >= 15 && !memcmp(string, "statusResponse\x0A", 15))
			{
				serverlist_info_t *info;
				char *p;
				int n;

				string += 15;
				// search the cache for this server and update it
				// the challenge is (ab)used to return the query time
				InfoString_GetValue(string, "challenge", infostringvalue, sizeof(infostringvalue));
				n = NetConn_ClientParsePacket_ServerList_ProcessReply(addressstring2, infostringvalue);
				if (n < 0)
					return true;

				info = &serverlist_cache[n].info;
				p = strchr(string, '\n');
				if(p)
				{
					*p = 0; // cut off the string there
					++p;
					info->players_len = dp_strlcpy(info->players, p, sizeof(info->players));
				}
				else
				{
					Con_Printf("statusResponse without players block?\n");
					info->players_len = info->players[0] = 0;
				}
				info->game_len     = InfoString_GetValue(string, "gamename", info->game,     sizeof(info->game));
				info->mod_len      = InfoString_GetValue(string, "modname",  info->mod,      sizeof(info->mod));
				info->map_len      = InfoString_GetValue(string, "mapname",  info->map,      sizeof(info->map));
				info->name_len     = InfoString_GetValue(string, "hostname", info->name,     sizeof(info->name));
				info->qcstatus_len = InfoString_GetValue(string, "qcstatus", info->qcstatus, sizeof(info->qcstatus));
				info->protocol    = InfoString_GetValue(string, "protocol"     , infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : -1;
				info->numplayers  = InfoString_GetValue(string, "clients"      , infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : 0;
				info->numbots     = InfoString_GetValue(string, "bots"         , infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : -1;
				info->maxplayers  = InfoString_GetValue(string, "sv_maxclients", infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : 0;
				info->gameversion = InfoString_GetValue(string, "gameversion"  , infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : 0;
				info->numhumans = info->numplayers - max(0, info->numbots);
				info->freeslots = info->maxplayers - info->numplayers;

				NetConn_ClientParsePacket_ServerList_UpdateCache(n);

				return true;
			}
			if (length >= 13 && !memcmp(string, "infoResponse\x0A", 13))
			{
				serverlist_info_t *info;
				int n;

				string += 13;
				// search the cache for this server and update it
				// the challenge is (ab)used to return the query time
				InfoString_GetValue(string, "challenge", infostringvalue, sizeof(infostringvalue));
				n = NetConn_ClientParsePacket_ServerList_ProcessReply(addressstring2, infostringvalue);
				if (n < 0)
					return true;

				info = &serverlist_cache[n].info;
				info->players_len = info->players[0] = 0;
				info->game_len     = InfoString_GetValue(string, "gamename", info->game,     sizeof(info->game));
				info->mod_len      = InfoString_GetValue(string, "modname",  info->mod,      sizeof(info->mod));
				info->map_len      = InfoString_GetValue(string, "mapname",  info->map,      sizeof(info->map));
				info->name_len     = InfoString_GetValue(string, "hostname", info->name,     sizeof(info->name));
				info->qcstatus_len = InfoString_GetValue(string, "qcstatus", info->qcstatus, sizeof(info->qcstatus));
				info->protocol    = InfoString_GetValue(string, "protocol"     , infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : -1;
				info->numplayers  = InfoString_GetValue(string, "clients"      , infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : 0;
				info->numbots     = InfoString_GetValue(string, "bots"         , infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : -1;
				info->maxplayers  = InfoString_GetValue(string, "sv_maxclients", infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : 0;
				info->gameversion = InfoString_GetValue(string, "gameversion"  , infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : 0;
				info->numhumans = info->numplayers - max(0, info->numbots);
				info->freeslots = info->maxplayers - info->numplayers;

				NetConn_ClientParsePacket_ServerList_UpdateCache(n);

				return true;
			}
			if (!strncmp(string, "getserversResponse\\", 19) && serverlist_cachecount < SERVERLIST_TOTALSIZE)
			{
				// Extract the IP addresses
				data += 18;
				length -= 18;
				NetConn_ClientParsePacket_ServerList_ParseDPList(peeraddress, addressstring2, data, length, false);
				return true;
			}
			if (!strncmp(string, "getserversExtResponse", 21) && serverlist_cachecount < SERVERLIST_TOTALSIZE)
			{
				// Extract the IP addresses
				data += 21;
				length -= 21;
				NetConn_ClientParsePacket_ServerList_ParseDPList(peeraddress, addressstring2, data, length, true);
				return true;
			}
			if (!memcmp(string, "d\n", 2) && serverlist_cachecount < SERVERLIST_TOTALSIZE)
			{
				// Extract the IP addresses
				data += 2;
				length -= 2;
				NetConn_ClientParsePacket_ServerList_ParseQWList(peeraddress, addressstring2, data, length);
				return true;
			}
		}
#endif
		if (!strncmp(string, "extResponse ", 12))
		{
			++cl_net_extresponse_count;
			if(cl_net_extresponse_count > NET_EXTRESPONSE_MAX)
				cl_net_extresponse_count = NET_EXTRESPONSE_MAX;
			cl_net_extresponse_last = (cl_net_extresponse_last + 1) % NET_EXTRESPONSE_MAX;
			dpsnprintf(cl_net_extresponse[cl_net_extresponse_last], sizeof(cl_net_extresponse[cl_net_extresponse_last]), "\"%s\" %s", addressstring2, string + 12);
			return true;
		}
		if (!strncmp(string, "ping", 4))
		{
			if (developer_extra.integer)
				Con_DPrintf("Received ping from %s, sending ack\n", addressstring2);
			NetConn_WriteString(mysocket, "\377\377\377\377ack", peeraddress);
			return true;
		}
		if (!strncmp(string, "ack", 3))
			return true;
		// QuakeWorld compatibility
		if (length > 1 && string[0] == 'c' && (string[1] == '-' || (string[1] >= '0' && string[1] <= '9')) && cls.connect_trying)
		{
			// challenge message
			if (net_sourceaddresscheck.integer && LHNETADDRESS_Compare(peeraddress, &cls.connect_address)) {
				Con_Printf(CON_WARN "ignoring c message from wrong server %s\n", addressstring2);
				return true;
			}
			Con_DPrintf("challenge %s received, sending QuakeWorld connect request back to %s\n", string + 1, addressstring2);
			dp_strlcpy(cl_connect_status, "Connect: replying to challenge...", sizeof(cl_connect_status));

			cls.qw_qport = qport.integer;
			// update the server IP in the userinfo (QW servers expect this, and it is used by the reconnect command)
			InfoString_SetValue(cls.userinfo, sizeof(cls.userinfo), "*ip", addressstring2);
			memcpy(senddata, "\377\377\377\377", 4);
			dpsnprintf(senddata+4, sizeof(senddata)-4, "connect %i %i %i \"%s%s\"\n", 28, cls.qw_qport, atoi(string + 1), cls.userinfo, cls.connect_userinfo);
			NetConn_WriteString(mysocket, senddata, peeraddress);
			return true;
		}
		if (length >= 1 && string[0] == 'j' && cls.connect_trying)
		{
			// accept message
			if (net_sourceaddresscheck.integer && LHNETADDRESS_Compare(peeraddress, &cls.connect_address)) {
				Con_Printf(CON_WARN "ignoring j message from wrong server %s\n", addressstring2);
				return true;
			}
			NetConn_ConnectionEstablished(mysocket, peeraddress, PROTOCOL_QUAKEWORLD);
			return true;
		}
		if (length > 2 && !memcmp(string, "n\\", 2))
		{
#ifdef CONFIG_MENU
			serverlist_info_t *info;
			int n;
			const char *s;

			// qw server status
			if (serverlist_consoleoutput && developer_networking.integer >= 2)
				Con_Printf("QW server status from server at %s:\n%s\n", addressstring2, string + 1);

			string += 1;
			// search the cache for this server and update it
			n = NetConn_ClientParsePacket_ServerList_ProcessReply(addressstring2, NULL);
			if (n < 0)
				return true;

			info = &serverlist_cache[n].info;
			dp_strlcpy(info->game, "QuakeWorld", sizeof(info->game));
			info->mod_len  = InfoString_GetValue(string, "*gamedir", info->mod, sizeof(info->mod));
			info->map_len  = InfoString_GetValue(string, "map"     , info->map, sizeof(info->map));
			info->name_len = InfoString_GetValue(string, "hostname", info->name, sizeof(info->name));
			info->protocol = 0;
			info->numplayers = 0; // updated below
			info->numhumans = 0; // updated below
			info->maxplayers  = InfoString_GetValue(string, "maxclients" , infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : 0;
			info->gameversion = InfoString_GetValue(string, "gameversion", infostringvalue, sizeof(infostringvalue)) ? atoi(infostringvalue) : 0;

			// count active players on server
			// (we could gather more info, but we're just after the number)
			s = strchr(string, '\n');
			if (s)
			{
				s++;
				while (s < string + length)
				{
					for (;s < string + length && *s != '\n';s++)
						;
					if (s >= string + length)
						break;
					info->numplayers++;
					info->numhumans++;
					s++;
				}
			}

			NetConn_ClientParsePacket_ServerList_UpdateCache(n);
#endif
			return true;
		}
		if (string[0] == 'n')
		{
			// qw print command, used by rcon replies too
			if (net_sourceaddresscheck.integer && LHNETADDRESS_Compare(peeraddress, &cls.connect_address) && LHNETADDRESS_Compare(peeraddress, &cls.rcon_address)) {
				Con_Printf(CON_WARN "ignoring n message from wrong server %s\n", addressstring2);
				return true;
			}
			Con_Printf("QW print command from server at %s:\n%s\n", addressstring2, string + 1);
		}
		// we may not have liked the packet, but it was a command packet, so
		// we're done processing this packet now
		return true;
	}
	// quakeworld ingame packet
	if (fromserver && cls.protocol == PROTOCOL_QUAKEWORLD && length >= 8 && (ret = NetConn_ReceivedMessage(cls.netcon, data, length, cls.protocol, net_messagetimeout.value)) == 2)
	{
		ret = 0;
		CL_ParseServerMessage();
		return ret;
	}
	// netquake control packets, supported for compatibility only
	if (length >= 5 && BuffBigLong(data) == ((int)NETFLAG_CTL | length) && !ENCRYPTION_REQUIRED)
	{
#ifdef CONFIG_MENU
		int n;
		serverlist_info_t *info;
#endif

		data += 4;
		length -= 4;
		SZ_Clear(&cl_message);
		SZ_Write(&cl_message, data, length);
		MSG_BeginReading(&cl_message);
		c = MSG_ReadByte(&cl_message);
		switch (c)
		{
		case CCREP_ACCEPT:
			if (developer_extra.integer)
				Con_DPrintf("Datagram_ParseConnectionless: received CCREP_ACCEPT from %s.\n", addressstring2);
			if (cls.connect_trying)
			{
				lhnetaddress_t clientportaddress;
				if (net_sourceaddresscheck.integer && LHNETADDRESS_Compare(peeraddress, &cls.connect_address)) {
					Con_Printf(CON_WARN "ignoring CCREP_ACCEPT message from wrong server %s\n", addressstring2);
					break;
				}
				clientportaddress = *peeraddress;
				LHNETADDRESS_SetPort(&clientportaddress, MSG_ReadLong(&cl_message));
				// extra ProQuake stuff
				if (length >= 6)
					cls.proquake_servermod = MSG_ReadByte(&cl_message); // MOD_PROQUAKE
				else
					cls.proquake_servermod = 0;
				if (length >= 7)
					cls.proquake_serverversion = MSG_ReadByte(&cl_message); // version * 10
				else
					cls.proquake_serverversion = 0;
				if (length >= 8)
					cls.proquake_serverflags = MSG_ReadByte(&cl_message); // flags (mainly PQF_CHEATFREE)
				else
					cls.proquake_serverflags = 0;
				if (cls.proquake_servermod == 1)
					Con_Printf("Connected to ProQuake %.1f server, enabling precise aim\n", cls.proquake_serverversion / 10.0f);
				// update the server IP in the userinfo (QW servers expect this, and it is used by the reconnect command)
				InfoString_SetValue(cls.userinfo, sizeof(cls.userinfo), "*ip", addressstring2);
				NetConn_ConnectionEstablished(mysocket, &clientportaddress, PROTOCOL_QUAKE);
			}
			break;
		case CCREP_REJECT:
			if (net_sourceaddresscheck.integer && LHNETADDRESS_Compare(peeraddress, &cls.connect_address))
			{
				Con_Printf(CON_WARN "ignoring CCREP_REJECT message from wrong server %s\n", addressstring2);
				break;
			}
			cls.connect_trying = false;
			dpsnprintf(cl_connect_status, sizeof(cl_connect_status), "Connect: rejected, %s", MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)));
			Con_Printf(CON_ERROR "Connect: rejected by %s\n%s\n", addressstring2, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)));
			break;
		case CCREP_SERVER_INFO:
			if (developer_extra.integer)
				Con_DPrintf("Datagram_ParseConnectionless: received CCREP_SERVER_INFO from %s.\n", addressstring2);
#ifdef CONFIG_MENU
			// LadyHavoc: because the quake server may report weird addresses
			// we just ignore it and keep the real address
			MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
			// search the cache for this server and update it
			n = NetConn_ClientParsePacket_ServerList_ProcessReply(addressstring2, NULL);
			if (n < 0)
				break;

			info = &serverlist_cache[n].info;
			info->game_len = dp_strlcpy(info->game, "Quake", sizeof(info->game));
			info->mod_len  = dp_strlcpy(info->mod, "", sizeof(info->mod)); // mod name is not specified
			info->name_len = dp_strlcpy(info->name, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof(info->name));
			info->map_len  = dp_strlcpy(info->map, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)), sizeof(info->map));
			info->numplayers = MSG_ReadByte(&cl_message);
			info->maxplayers = MSG_ReadByte(&cl_message);
			info->protocol = MSG_ReadByte(&cl_message);

			NetConn_ClientParsePacket_ServerList_UpdateCache(n);
#endif
			break;
		case CCREP_RCON: // RocketGuy: ProQuake rcon support
			if (net_sourceaddresscheck.integer && LHNETADDRESS_Compare(peeraddress, &cls.rcon_address)) {
				Con_Printf(CON_WARN "ignoring CCREP_RCON message from wrong server %s\n", addressstring2);
				break;
			}
			if (developer_extra.integer)
				Con_DPrintf("Datagram_ParseConnectionless: received CCREP_RCON from %s.\n", addressstring2);

			Con_Printf("%s\n", MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)));
			break;
		case CCREP_PLAYER_INFO:
			// we got a CCREP_PLAYER_INFO??
			//if (developer_extra.integer)
				Con_Printf("Datagram_ParseConnectionless: received CCREP_PLAYER_INFO from %s.\n", addressstring2);
			break;
		case CCREP_RULE_INFO:
			// we got a CCREP_RULE_INFO??
			//if (developer_extra.integer)
				Con_Printf("Datagram_ParseConnectionless: received CCREP_RULE_INFO from %s.\n", addressstring2);
			break;
		default:
			break;
		}
		SZ_Clear(&cl_message);
		// we may not have liked the packet, but it was a valid control
		// packet, so we're done processing this packet now
		return true;
	}
	ret = 0;
	if (fromserver && length >= (int)NET_HEADERSIZE && (ret = NetConn_ReceivedMessage(cls.netcon, data, length, cls.protocol, net_messagetimeout.value)) == 2)
		CL_ParseServerMessage();
	return ret;
}

#ifdef CONFIG_MENU
void NetConn_QueryQueueFrame(void)
{
	unsigned index;
	unsigned maxqueries;
	char dpquery[53]; // theoretical max: 14+22+16+1
	double currentrealtime;
	static double querycounter = 0;
	static unsigned pass = 0, server = 0;
	unsigned queriesperserver = bound(1, net_slist_maxtries.integer, 8);

	if (!serverlist_querystage)
		return;

	// If the client stalls partway through a frame (test command: `alias a a;a`)
	// for correct pings we need to know what host.realtime would be if it were updated now.
	currentrealtime = host.realtime + (Sys_DirtyTime() - host.dirtytime);

	// apply a cool down time after master server replies,
	// to avoid messing up the ping times on the servers
	if (serverlist_querystage < SLIST_QUERYSTAGE_SERVERS)
	{
		lhnetaddress_t masteraddress;

		if (currentrealtime < masterquerytime + net_slist_timeout.value)
			return;

		// Report the masters that timed out or whose response was incomplete.
		// Some people have IPv6 DNS but no v6 connectivity
		// so v6 master timeouts are currently silent by default.
		for (index = 0; index < DPMASTER_COUNT; ++index)
		{
			if (dpmasterstatus[index] == MASTER_TX_QUERY)
			{
				if (net_slist_debug.integer ||
				(LHNETADDRESS_FromString(&masteraddress, sv_masters[index].string, DPMASTER_PORT)
				&& LHNETADDRESS_GetAddressType(&masteraddress) != LHNETADDRESSTYPE_INET6))
					Con_Printf(CON_WARN "WARNING: dpmaster %s query timed out\n", sv_masters[index].string);
			}
			else if (dpmasterstatus[index] && dpmasterstatus[index] < MASTER_RX_COMPLETE)
				Con_Printf(CON_WARN "WARNING: dpmaster %s list incomplete (packet loss)\n", sv_masters[index].string);
		}

		for (index = 0; index < QWMASTER_COUNT; ++index)
			if (qwmasterstatus[index] && qwmasterstatus[index] < MASTER_RX_RESPONSE // no EOT marker in QW lists
			&& LHNETADDRESS_FromString(&masteraddress, sv_qwmasters[index].string, DPMASTER_PORT)
			&& LHNETADDRESS_GetAddressType(&masteraddress) != LHNETADDRESSTYPE_INET6) // some people have v6 DNS but no connectivity
				Con_Printf(CON_WARN "WARNING: qwmaster %s query timed out\n", sv_qwmasters[index].string);

		serverlist_querystage = SLIST_QUERYSTAGE_SERVERS;
	}

	// each time querycounter reaches 1.0 issue a query
	querycounter += cl.realframetime * net_slist_queriespersecond.value;
	maxqueries = bound(0, (int)querycounter, net_slist_queriesperframe.integer);
	querycounter -= maxqueries;
	if (maxqueries == 0)
		return;

	if (pass < queriesperserver)
	{
		// QW depends on waiting "long enough" between queries that responses "definitely" refer to the most recent querytime
		// DP servers can echo back a timestamp for reliable (and more frequent, see net_slist_interval) pings
		ServerList_BuildDPServerQuery(dpquery, sizeof(dpquery), currentrealtime);

		for (unsigned queries = 0; server < serverlist_cachecount; ++server)
		{
			lhnetaddress_t address;
			unsigned socket;
			serverlist_entry_t *entry = &serverlist_cache[server];

			if (queries >= maxqueries
			|| currentrealtime <= entry->querytime + (entry->protocol == PROTOCOL_QUAKEWORLD ? net_slist_timeout : net_slist_interval).value)
				return; // continue this pass at the current server on a later frame

			LHNETADDRESS_FromString(&address, entry->info.cname, 0);
			if (entry->protocol == PROTOCOL_QUAKEWORLD)
			{
				for (socket = 0; socket < cl_numsockets; ++socket)
					if (cl_sockets[socket])
						NetConn_WriteString(cl_sockets[socket], "\377\377\377\377status\n", &address);
			}
			else
			{
				for (socket = 0; socket < cl_numsockets; ++socket)
					if (cl_sockets[socket])
						NetConn_WriteString(cl_sockets[socket], dpquery, &address);
			}

			entry->querytime = currentrealtime;
			queries++;

			if (serverlist_consoleoutput)
				Con_Printf("querying %25s (%i. try)\n", entry->info.cname, pass + 1);
		}
	}
	else
	{
		// check timeouts
		for (; server < serverlist_cachecount; ++server)
		{
			serverlist_entry_t *entry = &serverlist_cache[server];

			if (!entry->responded // no acceptable response during this refresh cycle
			&& entry->info.ping) // visible in the list (has old ping from previous refresh cycle)
			{
				if (currentrealtime > entry->querytime + net_slist_maxping.value/1000.0f)
				{
					// you have no chance to survive make your timeout
					serverreplycount--;
					if(!net_slist_pause.integer)
					{
						if (net_slist_debug.integer)
							Con_Printf(CON_WARN "Removing timed out server %s from viewlist\n", entry->info.cname);
						ServerList_ViewList_Remove(entry);
					}
					entry->info.ping = 0; // removed later by ServerList_ViewList_Insert if net_slist_pause
				}
				else // still has time
					return; // continue this pass at the current server on a later frame
			}
		}
	}

	// We finished the pass, ie didn't stop at maxqueries
	// or a server that can't be (re)queried or timed out yet.
	++pass;
	server = 0;

	if (pass == queriesperserver)
	{
		// timeout pass begins next frame
		if (net_slist_debug.integer)
		{
			int dpmastersqueried = 0, qwmastersqueried = 0;

			for (index = 0; index < DPMASTER_COUNT; ++index)
				if (dpmasterstatus[index])
					++dpmastersqueried;
			for (index = 0; index < QWMASTER_COUNT; ++index)
				if (qwmasterstatus[index])
					++qwmastersqueried;
			Con_Printf("^2Finished querying %i DP %i QW masters and %i servers in %f seconds\n",
					dpmastersqueried, qwmastersqueried, serverlist_cachecount, currentrealtime - masterquerytime);
		}
	}
	else if (pass > queriesperserver)
	{
		// Nothing else to do until the next refresh cycle.
		serverlist_querystage = 0;
		pass = 0;
		if (net_slist_debug.integer && serverlist_cachecount)
			Con_Printf("^4Finished checking server timeouts in %f\n",
					currentrealtime - serverlist_cache[serverlist_cachecount - 1].querytime);
		if (serverlist_cachecount >= SERVERLIST_TOTALSIZE)
			Con_Printf(CON_ERROR "ERROR: too many servers, some will not be listed!\n");
	}
}
#endif

void NetConn_ClientFrame(void)
{
	unsigned i;
	int length;
	lhnetaddress_t peeraddress;
	unsigned char readbuffer[NET_HEADERSIZE+NET_MAXMESSAGE];

	NetConn_UpdateSockets();

	if (cls.connect_trying && cls.connect_nextsendtime < host.realtime)
	{
		if (cls.connect_remainingtries > 0)
		{
			cls.connect_remainingtries--;
			dpsnprintf(cl_connect_status, sizeof(cl_connect_status), "Connect: sending initial request, %i %s left...", cls.connect_remainingtries, cls.connect_remainingtries == 1 ? "retry" : "retries");
		}
		else
		{
			char address[128];

			cls.connect_trying = false;
			LHNETADDRESS_ToString(&cls.connect_address, address, sizeof(address), true);
			dp_strlcpy(cl_connect_status, "Connect: failed, no reply", sizeof(cl_connect_status));
			Con_Printf(CON_ERROR "%s from %s\n", cl_connect_status, address);
			return;
		}
		cls.connect_nextsendtime = host.realtime + 1;

		// try challenge first (newer DP server or QW)
		NetConn_WriteString(cls.connect_mysocket, "\377\377\377\377getchallenge", &cls.connect_address);
		// then try netquake as a fallback (old server, or netquake)
		SZ_Clear(&cl_message);
		// save space for the header, filled in later
		MSG_WriteLong(&cl_message, 0);
		MSG_WriteByte(&cl_message, CCREQ_CONNECT);
		MSG_WriteString(&cl_message, "QUAKE");
		MSG_WriteByte(&cl_message, NET_PROTOCOL_VERSION);
		// extended proquake stuff
		MSG_WriteByte(&cl_message, 1); // mod = MOD_PROQUAKE
		// this version matches ProQuake 3.40, the first version to support
		// the NAT fix, and it only supports the NAT fix for ProQuake 3.40 or
		// higher clients, so we pretend we are that version...
		MSG_WriteByte(&cl_message, 34); // version * 10
		MSG_WriteByte(&cl_message, 0); // flags
		MSG_WriteLong(&cl_message, 0); // password
		// write the packetsize now...
		StoreBigLong(cl_message.data, NETFLAG_CTL | (cl_message.cursize & NETFLAG_LENGTH_MASK));
		NetConn_Write(cls.connect_mysocket, cl_message.data, cl_message.cursize, &cls.connect_address);
		SZ_Clear(&cl_message);
	}

	for (i = 0;i < cl_numsockets;i++)
	{
		while (cl_sockets[i] && (length = NetConn_Read(cl_sockets[i], readbuffer, sizeof(readbuffer), &peeraddress)) > 0)
		{
//			R_TimeReport("clientreadnetwork");
			NetConn_ClientParsePacket(cl_sockets[i], readbuffer, length, &peeraddress);
//			R_TimeReport("clientparsepacket");
		}
	}
#ifdef CONFIG_MENU
	NetConn_QueryQueueFrame();
#endif
	if (cls.netcon && host.realtime > cls.netcon->timeout && !sv.active)
		CL_DisconnectEx(true, "Connection timed out");
}

static void NetConn_BuildChallengeString(char *buffer, int bufferlength)
{
	int i;
	char c;
	for (i = 0;i < bufferlength - 1;i++)
	{
		do
		{
			c = rand () % (127 - 33) + 33;
		} while (c == '\\' || c == ';' || c == '"' || c == '%' || c == '/');
		buffer[i] = c;
	}
	buffer[i] = 0;
}

/// (div0) build the full response only if possible; better a getinfo response than no response at all if getstatus won't fit
static qbool NetConn_BuildStatusResponse(const char* challenge, char* out_msg, size_t out_size, qbool fullstatus)
{
	prvm_prog_t *prog = SVVM_prog;
	char qcstatus[256];
	unsigned int nb_clients = 0, nb_bots = 0, i;
	int length;
	char teambuf[3];
	const char *crypto_idstring;
	const char *worldstatusstr;

	// How many clients are there?
	for (i = 0;i < (unsigned int)svs.maxclients;i++)
	{
		if (svs.clients[i].active)
		{
			nb_clients++;
			if (!svs.clients[i].netconnection)
				nb_bots++;
		}
	}

	*qcstatus = 0;
	worldstatusstr = PRVM_GetString(prog, PRVM_serverglobalstring(worldstatus));
	if(worldstatusstr && *worldstatusstr)
	{
		char *p;
		const char *q;
		p = qcstatus;
		for(q = worldstatusstr; *q && (size_t)(p - qcstatus) < (sizeof(qcstatus) - 1); ++q)
			if(*q != '\\' && *q != '\n')
				*p++ = *q;
		*p = 0;
	}

	/// \TODO: we should add more information for the full status string
	crypto_idstring = Crypto_GetInfoResponseDataString();
	length = dpsnprintf(out_msg, out_size,
						"\377\377\377\377%s\x0A"
						"\\gamename\\%s\\modname\\%s\\gameversion\\%d\\sv_maxclients\\%d"
						"\\clients\\%d\\bots\\%d\\mapname\\%s\\hostname\\%s\\protocol\\%d"
						"%s%s"
						"%s%s"
						"%s%s"
						"%s",
						fullstatus ? "statusResponse" : "infoResponse",
						gamenetworkfiltername, com_modname, gameversion.integer, svs.maxclients,
						nb_clients, nb_bots, sv.worldbasename, hostname.string, NET_PROTOCOL_VERSION,
						*qcstatus ? "\\qcstatus\\" : "", qcstatus,
						challenge ? "\\challenge\\" : "", challenge ? challenge : "",
						crypto_idstring ? "\\d0_blind_id\\" : "", crypto_idstring ? crypto_idstring : "",
						fullstatus ? "\n" : "");

	// Make sure it fits in the buffer
	if (length < 0)
		goto bad;

	if (fullstatus)
	{
		char *ptr;
		int left;
		int savelength;

		savelength = length;

		ptr = out_msg + length;
		left = (int)out_size - length;

		for (i = 0;i < (unsigned int)svs.maxclients;i++)
		{
			client_t *client = &svs.clients[i];
			if (client->active)
			{
				int nameind, cleanind, pingvalue;
				char curchar;
				char cleanname [sizeof(client->name)];
				const char *statusstr;
				prvm_edict_t *ed;

				// Remove all characters '"' and '\' in the player name
				nameind = 0;
				cleanind = 0;
				do
				{
					curchar = client->name[nameind++];
					if (curchar != '"' && curchar != '\\')
					{
						cleanname[cleanind++] = curchar;
						if (cleanind == sizeof(cleanname) - 1)
							break;
					}
				} while (curchar != '\0');
				cleanname[cleanind] = 0; // cleanind is always a valid index even at this point

				pingvalue = (int)(client->ping * 1000.0f);
				if(client->netconnection)
					pingvalue = bound(1, pingvalue, 9999);
				else
					pingvalue = 0;

				*qcstatus = 0;
				ed = PRVM_EDICT_NUM(i + 1);
				statusstr = PRVM_GetString(prog, PRVM_serveredictstring(ed, clientstatus));
				if(statusstr && *statusstr)
				{
					char *p;
					const char *q;
					p = qcstatus;
					for(q = statusstr; *q && p != qcstatus + sizeof(qcstatus) - 1; ++q)
						if(*q != '\\' && *q != '"' && !ISWHITESPACE(*q))
							*p++ = *q;
					*p = 0;
				}

				if (IS_NEXUIZ_DERIVED(gamemode) && (teamplay.integer > 0))
				{
					if(client->frags == -666) // spectator
						dp_strlcpy(teambuf, " 0", sizeof(teambuf));
					else if(client->colors == 0x44) // red team
						dp_strlcpy(teambuf, " 1", sizeof(teambuf));
					else if(client->colors == 0xDD) // blue team
						dp_strlcpy(teambuf, " 2", sizeof(teambuf));
					else if(client->colors == 0xCC) // yellow team
						dp_strlcpy(teambuf, " 3", sizeof(teambuf));
					else if(client->colors == 0x99) // pink team
						dp_strlcpy(teambuf, " 4", sizeof(teambuf));
					else
						dp_strlcpy(teambuf, " 0", sizeof(teambuf));
				}
				else
					*teambuf = 0;

				// note: team number is inserted according to SoF2 protocol
				if(*qcstatus)
					length = dpsnprintf(ptr, left, "%s %d%s \"%s\"\n",
										qcstatus,
										pingvalue,
										teambuf,
										cleanname);
				else
					length = dpsnprintf(ptr, left, "%d %d%s \"%s\"\n",
										client->frags,
										pingvalue,
										teambuf,
										cleanname);

				if(length < 0)
				{
					// out of space?
					// turn it into an infoResponse!
					out_msg[savelength] = 0;
					memcpy(out_msg + 4, "infoResponse\x0A", 13);
					memmove(out_msg + 17, out_msg + 19, savelength - 19);
					break;
				}
				left -= length;
				ptr += length;
			}
		}
	}

	return true;

bad:
	return false;
}

static qbool NetConn_PreventFlood(lhnetaddress_t *peeraddress, server_floodaddress_t *floodlist, size_t floodlength, double floodtime, qbool renew)
{
	size_t floodslotnum, bestfloodslotnum;
	double bestfloodtime;
	lhnetaddress_t noportpeeraddress;
	// see if this is a connect flood
	noportpeeraddress = *peeraddress;
	LHNETADDRESS_SetPort(&noportpeeraddress, 0);
	bestfloodslotnum = 0;
	bestfloodtime = floodlist[bestfloodslotnum].lasttime;
	for (floodslotnum = 0;floodslotnum < floodlength;floodslotnum++)
	{
		if (bestfloodtime >= floodlist[floodslotnum].lasttime)
		{
			bestfloodtime = floodlist[floodslotnum].lasttime;
			bestfloodslotnum = floodslotnum;
		}
		if (floodlist[floodslotnum].lasttime && LHNETADDRESS_Compare(&noportpeeraddress, &floodlist[floodslotnum].address) == 0)
		{
			// this address matches an ongoing flood address
			if (host.realtime < floodlist[floodslotnum].lasttime + floodtime)
			{
				if(renew)
				{
					// renew the ban on this address so it does not expire
					// until the flood has subsided
					floodlist[floodslotnum].lasttime = host.realtime;
				}
				//Con_Printf("Flood detected!\n");
				return true;
			}
			// the flood appears to have subsided, so allow this
			bestfloodslotnum = floodslotnum; // reuse the same slot
			break;
		}
	}
	// begin a new timeout on this address
	floodlist[bestfloodslotnum].address = noportpeeraddress;
	floodlist[bestfloodslotnum].lasttime = host.realtime;
	//Con_Printf("Flood detection initiated!\n");
	return false;
}

void NetConn_ClearFlood(lhnetaddress_t *peeraddress, server_floodaddress_t *floodlist, size_t floodlength)
{
	size_t floodslotnum;
	lhnetaddress_t noportpeeraddress;
	// see if this is a connect flood
	noportpeeraddress = *peeraddress;
	LHNETADDRESS_SetPort(&noportpeeraddress, 0);
	for (floodslotnum = 0;floodslotnum < floodlength;floodslotnum++)
	{
		if (floodlist[floodslotnum].lasttime && LHNETADDRESS_Compare(&noportpeeraddress, &floodlist[floodslotnum].address) == 0)
		{
			// this address matches an ongoing flood address
			// remove the ban
			floodlist[floodslotnum].address.addresstype = LHNETADDRESSTYPE_NONE;
			floodlist[floodslotnum].lasttime = 0;
			//Con_Printf("Flood cleared!\n");
		}
	}
}

typedef qbool (*rcon_matchfunc_t) (lhnetaddress_t *peeraddress, const char *password, const char *hash, const char *s, int slen);

static qbool hmac_mdfour_time_matching(lhnetaddress_t *peeraddress, const char *password, const char *hash, const char *s, int slen)
{
	char mdfourbuf[16];
	long t1, t2;

	if (!password[0]) {
		Con_Print(CON_ERROR "LOGIC ERROR: RCon_Authenticate should never call the comparator with an empty password. Please report.\n");
		return false;
	}

	t1 = (long) time(NULL);
	t2 = strtol(s, NULL, 0);
	if(labs(t1 - t2) > rcon_secure_maxdiff.integer)
		return false;

	if(!HMAC_MDFOUR_16BYTES((unsigned char *) mdfourbuf, (unsigned char *) s, slen, (unsigned char *) password, (int)strlen(password)))
		return false;

	return !memcmp(mdfourbuf, hash, 16);
}

static qbool hmac_mdfour_challenge_matching(lhnetaddress_t *peeraddress, const char *password, const char *hash, const char *s, int slen)
{
	char mdfourbuf[16];
	int i;

	if (!password[0]) {
		Con_Print(CON_ERROR "LOGIC ERROR: RCon_Authenticate should never call the comparator with an empty password. Please report.\n");
		return false;
	}

	if(slen < (int)(sizeof(challenges[0].string)) - 1)
		return false;

	// validate the challenge
	for (i = 0;i < MAX_CHALLENGES;i++)
		if(challenges[i].time > 0)
			if (!LHNETADDRESS_Compare(peeraddress, &challenges[i].address) && !strncmp(challenges[i].string, s, sizeof(challenges[0].string) - 1))
				break;
	// if the challenge is not recognized, drop the packet
	if (i == MAX_CHALLENGES)
		return false;

	if(!HMAC_MDFOUR_16BYTES((unsigned char *) mdfourbuf, (unsigned char *) s, slen, (unsigned char *) password, (int)strlen(password)))
		return false;

	if(memcmp(mdfourbuf, hash, 16))
		return false;

	// unmark challenge to prevent replay attacks
	challenges[i].time = 0;

	return true;
}

static qbool plaintext_matching(lhnetaddress_t *peeraddress, const char *password, const char *hash, const char *s, int slen)
{
	if (!password[0]) {
		Con_Print(CON_ERROR "LOGIC ERROR: RCon_Authenticate should never call the comparator with an empty password. Please report.\n");
		return false;
	}

	return !strcmp(password, hash);
}

/// returns a string describing the user level, or NULL for auth failure
static const char *RCon_Authenticate(lhnetaddress_t *peeraddress, const char *password, const char *s, const char *endpos, rcon_matchfunc_t comparator, const char *cs, int cslen)
{
	const char *text, *userpass_start, *userpass_end, *userpass_startpass;
	static char buf[MAX_INPUTLINE];
	qbool hasquotes;
	qbool restricted = false;
	qbool have_usernames = false;
	static char vabuf[1024];

	userpass_start = rcon_password.string;
	while((userpass_end = strchr(userpass_start, ' ')))
	{
		have_usernames = true;
		dp_ustr2stp(buf, sizeof(buf), userpass_start, userpass_end - userpass_start);
		if(buf[0])  // Ignore empty entries due to leading/duplicate space.
			if(comparator(peeraddress, buf, password, cs, cslen))
				goto allow;
		userpass_start = userpass_end + 1;
	}
	if(userpass_start[0])  // Ignore empty trailing entry due to trailing space or password not set.
	{
		userpass_end = userpass_start + strlen(userpass_start);
		if(comparator(peeraddress, userpass_start, password, cs, cslen))
			goto allow;
	}

	restricted = true;
	have_usernames = false;
	userpass_start = rcon_restricted_password.string;
	while((userpass_end = strchr(userpass_start, ' ')))
	{
		have_usernames = true;
		dp_ustr2stp(buf, sizeof(buf), userpass_start, userpass_end - userpass_start);
		if(buf[0])  // Ignore empty entries due to leading/duplicate space.
			if(comparator(peeraddress, buf, password, cs, cslen))
				goto check;
		userpass_start = userpass_end + 1;
	}
	if(userpass_start[0])  // Ignore empty trailing entry due to trailing space or password not set.
	{
		userpass_end = userpass_start + strlen(userpass_start);
		if(comparator(peeraddress, userpass_start, password, cs, cslen))
			goto check;
	}
	
	return NULL; // DENIED

check:
	for(text = s; text != endpos; ++text)
		if((signed char) *text > 0 && ((signed char) *text < (signed char) ' ' || *text == ';'))
			return NULL; // block possible exploits against the parser/alias expansion

	while(s != endpos)
	{
		size_t l = strlen(s);
		if(l)
		{
			hasquotes = (strchr(s, '"') != NULL);
			// sorry, we can't allow these substrings in wildcard expressions,
			// as they can mess with the argument counts
			text = rcon_restricted_commands.string;
			while(COM_ParseToken_Console(&text))
			{
				// com_token now contains a pattern to check for...
				if(strchr(com_token, '*') || strchr(com_token, '?')) // wildcard expression, * can only match a SINGLE argument
				{
					if(!hasquotes)
						if(matchpattern_with_separator(s, com_token, true, " ", true)) // note how we excluded tab, newline etc. above
							goto match;
				}
				else if(strchr(com_token, ' ')) // multi-arg expression? must match in whole
				{
					if(!strcmp(com_token, s))
						goto match;
				}
				else // single-arg expression? must match the beginning of the command
				{
					if(!strcmp(com_token, s))
						goto match;
					if(!memcmp(va(vabuf, sizeof(vabuf), "%s ", com_token), s, strlen(com_token) + 1))
						goto match;
				}
			}
			// if we got here, nothing matched!
			return NULL;
		}
match:
		s += l + 1;
	}

allow:
	userpass_startpass = strchr(userpass_start, ':');
	if(have_usernames && userpass_startpass && userpass_startpass < userpass_end)
		return va(vabuf, sizeof(vabuf), "%srcon (username %.*s)", restricted ? "restricted " : "", (int)(userpass_startpass-userpass_start), userpass_start);

	return va(vabuf, sizeof(vabuf), "%srcon", restricted ? "restricted " : "");
}

static void RCon_Execute(lhnetsocket_t *mysocket, lhnetaddress_t *peeraddress, const char *addressstring2, const char *userlevel, const char *s, const char *endpos, qbool proquakeprotocol)
{
	if(userlevel)
	{
		// looks like a legitimate rcon command with the correct password
		const char *s_ptr = s;
		Con_Printf("server received %s command from %s: ", userlevel, host_client ? host_client->name : addressstring2);
		while(s_ptr != endpos)
		{
			size_t l = strlen(s_ptr);
			if(l)
				Con_Printf(" %s;", s_ptr);
			s_ptr += l + 1;
		}
		Con_Printf("\n");

		if (!host_client || !host_client->netconnection || LHNETADDRESS_GetAddressType(&host_client->netconnection->peeraddress) != LHNETADDRESSTYPE_LOOP)
			Con_Rcon_Redirect_Init(mysocket, peeraddress, proquakeprotocol);
		while(s != endpos)
		{
			size_t l = strlen(s);
			if(l)
			{
				client_t *host_client_save = host_client;
				Cmd_PreprocessAndExecuteString(cmd_local, s, l, src_local, true);
				host_client = host_client_save;
				// in case it is a command that changes host_client (like restart)
			}
			s += l + 1;
		}
		Con_Rcon_Redirect_End();
	}
	else
	{
		if (!host_client || !host_client->netconnection || LHNETADDRESS_GetAddressType(&host_client->netconnection->peeraddress) != LHNETADDRESSTYPE_LOOP)
			Con_Rcon_Redirect_Init(mysocket, peeraddress, proquakeprotocol);
		Con_Printf(CON_ERROR "server denied rcon access to %s\n", host_client ? host_client->name : addressstring2);
		Con_Rcon_Redirect_End();
	}
}

static int NetConn_ServerParsePacket(lhnetsocket_t *mysocket, unsigned char *data, int length, lhnetaddress_t *peeraddress)
{
	int i, ret, clientnum, best;
	double besttime;
	char *string, response[2800], addressstring2[128];
	static char stringbuf[16384]; // server only
	qbool islocal = (LHNETADDRESS_GetAddressType(peeraddress) == LHNETADDRESSTYPE_LOOP);
	char senddata[NET_HEADERSIZE+NET_MAXMESSAGE+CRYPTO_HEADERSIZE];
	size_t sendlength, response_len;
	char infostringvalue[MAX_INPUTLINE];

	if (!sv.active)
		return false;

	// convert the address to a string incase we need it
	LHNETADDRESS_ToString(peeraddress, addressstring2, sizeof(addressstring2), true);

	// see if we can identify the sender as a local player
	// (this is necessary for rcon to send a reliable reply if the client is
	//  actually on the server, not sending remotely)
	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
		if (host_client->netconnection && host_client->netconnection->mysocket == mysocket && !LHNETADDRESS_Compare(&host_client->netconnection->peeraddress, peeraddress))
			break;
	if (i == svs.maxclients)
		host_client = NULL;

	if (length >= 5 && data[0] == 255 && data[1] == 255 && data[2] == 255 && data[3] == 255)
	{
		// received a command string - strip off the packaging and put it
		// into our string buffer with NULL termination
		data += 4;
		length -= 4;
		length = min(length, (int)sizeof(stringbuf) - 1);
		memcpy(stringbuf, data, length);
		stringbuf[length] = 0;
		string = stringbuf;

		if (developer_extra.integer)
		{
			Con_Printf("NetConn_ServerParsePacket: %s sent us a command:\n", addressstring2);
			Com_HexDumpToConsole(data, length);
		}

		sendlength = sizeof(senddata) - 4;
		switch(Crypto_ServerParsePacket(string, length, senddata+4, &sendlength, peeraddress))
		{
			case CRYPTO_NOMATCH:
				// nothing to do
				break;
			case CRYPTO_MATCH:
				if(sendlength)
				{
					memcpy(senddata, "\377\377\377\377", 4);
					NetConn_Write(mysocket, senddata, (int)sendlength+4, peeraddress);
				}
				break;
			case CRYPTO_DISCARD:
				if(sendlength)
				{
					memcpy(senddata, "\377\377\377\377", 4);
					NetConn_Write(mysocket, senddata, (int)sendlength+4, peeraddress);
				}
				return true;
				break;
			case CRYPTO_REPLACE:
				string = senddata+4;
				length = (int)sendlength;
				break;
		}

		if (length >= 12 && !memcmp(string, "getchallenge", 12) && (islocal || sv_public.integer > -3))
		{
			for (i = 0, best = 0, besttime = host.realtime;i < MAX_CHALLENGES;i++)
			{
				if(challenges[i].time > 0)
					if (!LHNETADDRESS_Compare(peeraddress, &challenges[i].address))
						break;
				if (besttime > challenges[i].time)
					besttime = challenges[best = i].time;
			}
			// if we did not find an exact match, choose the oldest and
			// update address and string
			if (i == MAX_CHALLENGES)
			{
				i = best;
				challenges[i].address = *peeraddress;
				NetConn_BuildChallengeString(challenges[i].string, sizeof(challenges[i].string));
			}
			else
			{
				// flood control: drop if requesting challenge too often
				if(challenges[i].time > host.realtime - net_challengefloodblockingtimeout.value)
					return true;
			}
			challenges[i].time = host.realtime;
			// send the challenge
			memcpy(response, "\377\377\377\377", 4);
			dpsnprintf(response+4, sizeof(response)-4, "challenge %s", challenges[i].string);
			response_len = strlen(response) + 1;
			Crypto_ServerAppendToChallenge(string, length, response, &response_len, sizeof(response));
			NetConn_Write(mysocket, response, (int)response_len, peeraddress);
			return true;
		}
		if (length > 8 && !memcmp(string, "connect\\", 8))
		{
			client_t *client;
			crypto_t *crypto = Crypto_ServerGetInstance(peeraddress);
			string += 7;
			length -= 7;

			if(crypto && crypto->authenticated)
			{
				// no need to check challenge
				if(crypto_developer.integer)
				{
					Con_Printf("%s connection to %s is being established: client is %s@%s%.*s, I am %.*s@%s%.*s\n",
							crypto->use_aes ? "Encrypted" : "Authenticated",
							addressstring2,
							crypto->client_idfp[0] ? crypto->client_idfp : "-",
							(crypto->client_issigned || !crypto->client_keyfp[0]) ? "" : "~",
							crypto_keyfp_recommended_length, crypto->client_keyfp[0] ? crypto->client_keyfp : "-",
							crypto_keyfp_recommended_length, crypto->server_idfp[0] ? crypto->server_idfp : "-",
							(crypto->server_issigned || !crypto->server_keyfp[0]) ? "" : "~",
							crypto_keyfp_recommended_length, crypto->server_keyfp[0] ? crypto->server_keyfp : "-"
						  );
				}
			}
			else
			{
				if (InfoString_GetValue(string, "challenge", infostringvalue, sizeof(infostringvalue)))
				{
					// validate the challenge
					for (i = 0;i < MAX_CHALLENGES;i++)
						if(challenges[i].time > 0)
							if (!LHNETADDRESS_Compare(peeraddress, &challenges[i].address) && !strcmp(challenges[i].string, infostringvalue))
								break;
					// if the challenge is not recognized, drop the packet
					if (i == MAX_CHALLENGES)
						return true;
				}
			}

			if(InfoString_GetValue(string, "message", infostringvalue, sizeof(infostringvalue)))
				Con_DPrintf("Connecting client %s sent us the message: %s\n", addressstring2, infostringvalue);

			if(!(islocal || sv_public.integer > -2))
			{
				if (developer_extra.integer)
					Con_Printf("Datagram_ParseConnectionless: sending \"reject %s\" to %s.\n", sv_public_rejectreason.string, addressstring2);
				memcpy(response, "\377\377\377\377", 4);
				dpsnprintf(response+4, sizeof(response)-4, "reject %s", sv_public_rejectreason.string);
				NetConn_WriteString(mysocket, response, peeraddress);
				return true;
			}

			// check engine protocol
			if(!InfoString_GetValue(string, "protocol", infostringvalue, sizeof(infostringvalue)) || strcmp(infostringvalue, "darkplaces 3"))
			{
				if (developer_extra.integer)
					Con_Printf("Datagram_ParseConnectionless: sending \"reject Wrong game protocol.\" to %s.\n", addressstring2);
				NetConn_WriteString(mysocket, "\377\377\377\377reject Wrong game protocol.", peeraddress);
				return true;
			}

			// see if this is a duplicate connection request or a disconnected
			// client who is rejoining to the same client slot
			for (clientnum = 0, client = svs.clients;clientnum < svs.maxclients;clientnum++, client++)
			{
				if (client->netconnection && LHNETADDRESS_Compare(peeraddress, &client->netconnection->peeraddress) == 0)
				{
					// this is a known client...
					if(crypto && crypto->authenticated)
					{
						// reject if changing key!
						if(client->netconnection->crypto.authenticated)
						{
							if(
									strcmp(client->netconnection->crypto.client_idfp, crypto->client_idfp)
									||
									strcmp(client->netconnection->crypto.server_idfp, crypto->server_idfp)
									||
									strcmp(client->netconnection->crypto.client_keyfp, crypto->client_keyfp)
									||
									strcmp(client->netconnection->crypto.server_keyfp, crypto->server_keyfp)
							  )
							{
								if (developer_extra.integer)
									Con_Printf("Datagram_ParseConnectionless: sending \"reject Attempt to change key of crypto.\" to %s.\n", addressstring2);
								NetConn_WriteString(mysocket, "\377\377\377\377reject Attempt to change key of crypto.", peeraddress);
								return true;
							}
						}
					}
					else
					{
						// reject if downgrading!
						if(client->netconnection->crypto.authenticated)
						{
							if (developer_extra.integer)
								Con_Printf("Datagram_ParseConnectionless: sending \"reject Attempt to downgrade crypto.\" to %s.\n", addressstring2);
							NetConn_WriteString(mysocket, "\377\377\377\377reject Attempt to downgrade crypto.", peeraddress);
							return true;
						}
					}
					if (client->begun)
					{
						// client crashed and is coming back,
						// keep their stuff intact
						if (developer_extra.integer)
							Con_Printf("Datagram_ParseConnectionless: sending \"accept\" to %s.\n", addressstring2);
						NetConn_WriteString(mysocket, "\377\377\377\377accept", peeraddress);
						if(crypto && crypto->authenticated)
							Crypto_FinishInstance(&client->netconnection->crypto, crypto);
						SV_SendServerinfo(client);
					}
					else
					{
						// client is still trying to connect,
						// so we send a duplicate reply
						if (developer_extra.integer)
							Con_Printf("Datagram_ParseConnectionless: sending duplicate accept to %s.\n", addressstring2);
						if(crypto && crypto->authenticated)
							Crypto_FinishInstance(&client->netconnection->crypto, crypto);
						NetConn_WriteString(mysocket, "\377\377\377\377accept", peeraddress);
					}
					return true;
				}
			}

			if (NetConn_PreventFlood(peeraddress, sv.connectfloodaddresses, sizeof(sv.connectfloodaddresses) / sizeof(sv.connectfloodaddresses[0]), net_connectfloodblockingtimeout.value, true))
				return true;

			// find an empty client slot for this new client
			for (clientnum = 0; clientnum < svs.maxclients; clientnum++)
			{
				netconn_t *conn;
				int offset_clientnum = (net_connect_entnum_ofs.integer > 0)
						? (clientnum + net_connect_entnum_ofs.integer) % svs.maxclients
						: clientnum;

				if (!svs.clients[offset_clientnum].active && (conn = NetConn_Open(mysocket, peeraddress)))
				{
					// allocated connection
					if (developer_extra.integer)
						Con_Printf("Datagram_ParseConnectionless: sending \"accept\" to %s.\n", conn->address);
					NetConn_WriteString(mysocket, "\377\377\377\377accept", peeraddress);
					// now set up the client
					if(crypto && crypto->authenticated)
						Crypto_FinishInstance(&conn->crypto, crypto);
					SV_ConnectClient(offset_clientnum, conn);
					NetConn_Heartbeat(1);
					return true;
				}
			}

			// no empty slots found - server is full
			if (developer_extra.integer)
				Con_Printf("Datagram_ParseConnectionless: sending \"reject Server is full.\" to %s.\n", addressstring2);
			NetConn_WriteString(mysocket, "\377\377\377\377reject Server is full.", peeraddress);

			return true;
		}
		if (length >= 7 && !memcmp(string, "getinfo", 7) && (islocal || sv_public.integer > -1))
		{
			const char *challenge = NULL;

			if (NetConn_PreventFlood(peeraddress, sv.getstatusfloodaddresses, sizeof(sv.getstatusfloodaddresses) / sizeof(sv.getstatusfloodaddresses[0]), net_getstatusfloodblockingtimeout.value, false))
				return true;

			// If there was a challenge in the getinfo message
			if (length > 8 && string[7] == ' ')
				challenge = string + 8;

			if (NetConn_BuildStatusResponse(challenge, response, sizeof(response), false))
			{
				if (developer_extra.integer)
					Con_DPrintf("Sending reply to master %s - %s\n", addressstring2, response);
				NetConn_WriteString(mysocket, response, peeraddress);
			}
			return true;
		}
		if (length >= 9 && !memcmp(string, "getstatus", 9) && (islocal || sv_public.integer > -1))
		{
			const char *challenge = NULL;

			if (NetConn_PreventFlood(peeraddress, sv.getstatusfloodaddresses, sizeof(sv.getstatusfloodaddresses) / sizeof(sv.getstatusfloodaddresses[0]), net_getstatusfloodblockingtimeout.value, false))
				return true;

			// If there was a challenge in the getinfo message
			if (length > 10 && string[9] == ' ')
				challenge = string + 10;

			if (NetConn_BuildStatusResponse(challenge, response, sizeof(response), true))
			{
				if (developer_extra.integer)
					Con_DPrintf("Sending reply to client %s - %s\n", addressstring2, response);
				NetConn_WriteString(mysocket, response, peeraddress);
			}
			return true;
		}
		if (length >= 37 && !memcmp(string, "srcon HMAC-MD4 TIME ", 20))
		{
			char *password = string + 20;
			char *timeval = string + 37;
			char *s = strchr(timeval, ' ');
			char *endpos = string + length + 1; // one behind the NUL, so adding strlen+1 will eventually reach it
			const char *userlevel;

			if(rcon_secure.integer > 1)
				return true;

			if(!s)
				return true; // invalid packet
			++s;

			userlevel = RCon_Authenticate(peeraddress, password, s, endpos, hmac_mdfour_time_matching, timeval, endpos - timeval - 1); // not including the appended \0 into the HMAC
			RCon_Execute(mysocket, peeraddress, addressstring2, userlevel, s, endpos, false);
			return true;
		}
		if (length >= 42 && !memcmp(string, "srcon HMAC-MD4 CHALLENGE ", 25))
		{
			char *password = string + 25;
			char *challenge = string + 42;
			char *s = strchr(challenge, ' ');
			char *endpos = string + length + 1; // one behind the NUL, so adding strlen+1 will eventually reach it
			const char *userlevel;
			if(!s)
				return true; // invalid packet
			++s;

			userlevel = RCon_Authenticate(peeraddress, password, s, endpos, hmac_mdfour_challenge_matching, challenge, endpos - challenge - 1); // not including the appended \0 into the HMAC
			RCon_Execute(mysocket, peeraddress, addressstring2, userlevel, s, endpos, false);
			return true;
		}
		if (length >= 5 && !memcmp(string, "rcon ", 5))
		{
			int j;
			char *s = string + 5;
			char *endpos = string + length + 1; // one behind the NUL, so adding strlen+1 will eventually reach it
			char password[64];

			if(rcon_secure.integer > 0)
				return true;

			for (j = 0;!ISWHITESPACE(*s);s++)
				if (j < (int)sizeof(password) - 1)
					password[j++] = *s;
			if(ISWHITESPACE(*s) && s != endpos) // skip leading ugly space
				++s;
			password[j] = 0;
			if (!ISWHITESPACE(password[0]))
			{
				const char *userlevel = RCon_Authenticate(peeraddress, password, s, endpos, plaintext_matching, NULL, 0);
				RCon_Execute(mysocket, peeraddress, addressstring2, userlevel, s, endpos, false);
			}
			return true;
		}
		if (!strncmp(string, "extResponse ", 12))
		{
			++sv_net_extresponse_count;
			if(sv_net_extresponse_count > NET_EXTRESPONSE_MAX)
				sv_net_extresponse_count = NET_EXTRESPONSE_MAX;
			sv_net_extresponse_last = (sv_net_extresponse_last + 1) % NET_EXTRESPONSE_MAX;
			dpsnprintf(sv_net_extresponse[sv_net_extresponse_last], sizeof(sv_net_extresponse[sv_net_extresponse_last]), "'%s' %s", addressstring2, string + 12);
			return true;
		}
		if (!strncmp(string, "ping", 4))
		{
			if (developer_extra.integer)
				Con_DPrintf("Received ping from %s, sending ack\n", addressstring2);
			NetConn_WriteString(mysocket, "\377\377\377\377ack", peeraddress);
			return true;
		}
		if (!strncmp(string, "ack", 3))
			return true;
		// we may not have liked the packet, but it was a command packet, so
		// we're done processing this packet now
		return true;
	}
	// netquake control packets, supported for compatibility only, and only
	// when running game protocols that are normally served via this connection
	// protocol
	// (this protects more modern protocols against being used for
	//  Quake packet flood Denial Of Service attacks)
	if (length >= 5 && (i = BuffBigLong(data)) && (i & (~NETFLAG_LENGTH_MASK)) == (int)NETFLAG_CTL && (i & NETFLAG_LENGTH_MASK) == length && (sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE || sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3 || sv.protocol == PROTOCOL_DARKPLACES1 || sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3) && !ENCRYPTION_REQUIRED)
	{
		int c;
		int protocolnumber;
		const char *protocolname;
		client_t *knownclient;
		client_t *newclient;
		data += 4;
		length -= 4;
		SZ_Clear(&sv_message);
		SZ_Write(&sv_message, data, length);
		MSG_BeginReading(&sv_message);
		c = MSG_ReadByte(&sv_message);
		switch (c)
		{
		case CCREQ_CONNECT:
			if (developer_extra.integer)
				Con_DPrintf("Datagram_ParseConnectionless: received CCREQ_CONNECT from %s.\n", addressstring2);
			if(!(islocal || sv_public.integer > -2))
			{
				if (developer_extra.integer)
					Con_DPrintf("Datagram_ParseConnectionless: sending CCREP_REJECT \"%s\" to %s.\n", sv_public_rejectreason.string, addressstring2);
				SZ_Clear(&sv_message);
				// save space for the header, filled in later
				MSG_WriteLong(&sv_message, 0);
				MSG_WriteByte(&sv_message, CCREP_REJECT);
				MSG_WriteUnterminatedString(&sv_message, sv_public_rejectreason.string);
				MSG_WriteString(&sv_message, "\n");
				StoreBigLong(sv_message.data, NETFLAG_CTL | (sv_message.cursize & NETFLAG_LENGTH_MASK));
				NetConn_Write(mysocket, sv_message.data, sv_message.cursize, peeraddress);
				SZ_Clear(&sv_message);
				break;
			}

			protocolname = MSG_ReadString(&sv_message, sv_readstring, sizeof(sv_readstring));
			protocolnumber = MSG_ReadByte(&sv_message);
			if (strcmp(protocolname, "QUAKE") || protocolnumber != NET_PROTOCOL_VERSION)
			{
				if (developer_extra.integer)
					Con_DPrintf("Datagram_ParseConnectionless: sending CCREP_REJECT \"Incompatible version.\" to %s.\n", addressstring2);
				SZ_Clear(&sv_message);
				// save space for the header, filled in later
				MSG_WriteLong(&sv_message, 0);
				MSG_WriteByte(&sv_message, CCREP_REJECT);
				MSG_WriteString(&sv_message, "Incompatible version.\n");
				StoreBigLong(sv_message.data, NETFLAG_CTL | (sv_message.cursize & NETFLAG_LENGTH_MASK));
				NetConn_Write(mysocket, sv_message.data, sv_message.cursize, peeraddress);
				SZ_Clear(&sv_message);
				break;
			}

			// see if this connect request comes from a known client
			for (clientnum = 0, knownclient = svs.clients;clientnum < svs.maxclients;clientnum++, knownclient++)
			{
				if (knownclient->netconnection && LHNETADDRESS_Compare(peeraddress, &knownclient->netconnection->peeraddress) == 0)
				{
					// this is either a duplicate connection request
					// or coming back from a timeout
					// (if so, keep their stuff intact)

					crypto_t *crypto = Crypto_ServerGetInstance(peeraddress);
					if((crypto && crypto->authenticated) || knownclient->netconnection->crypto.authenticated)
					{
						if (developer_extra.integer)
							Con_Printf("Datagram_ParseConnectionless: sending CCREP_REJECT \"Attempt to downgrade crypto.\" to %s.\n", addressstring2);
						SZ_Clear(&sv_message);
						// save space for the header, filled in later
						MSG_WriteLong(&sv_message, 0);
						MSG_WriteByte(&sv_message, CCREP_REJECT);
						MSG_WriteString(&sv_message, "Attempt to downgrade crypto.\n");
						StoreBigLong(sv_message.data, NETFLAG_CTL | (sv_message.cursize & NETFLAG_LENGTH_MASK));
						NetConn_Write(mysocket, sv_message.data, sv_message.cursize, peeraddress);
						SZ_Clear(&sv_message);
						return true;
					}

					// send a reply
					if (developer_extra.integer)
						Con_DPrintf("Datagram_ParseConnectionless: sending duplicate CCREP_ACCEPT to %s.\n", addressstring2);
					SZ_Clear(&sv_message);
					// save space for the header, filled in later
					MSG_WriteLong(&sv_message, 0);
					MSG_WriteByte(&sv_message, CCREP_ACCEPT);
					MSG_WriteLong(&sv_message, LHNETADDRESS_GetPort(LHNET_AddressFromSocket(knownclient->netconnection->mysocket)));
					StoreBigLong(sv_message.data, NETFLAG_CTL | (sv_message.cursize & NETFLAG_LENGTH_MASK));
					NetConn_Write(mysocket, sv_message.data, sv_message.cursize, peeraddress);
					SZ_Clear(&sv_message);

					// if client is already spawned, re-send the
					// serverinfo message as they'll need it to play
					if (knownclient->begun)
						SV_SendServerinfo(knownclient);
					return true;
				}
			}

			// this is a new client, check for connection flood
			if (NetConn_PreventFlood(peeraddress, sv.connectfloodaddresses, sizeof(sv.connectfloodaddresses) / sizeof(sv.connectfloodaddresses[0]), net_connectfloodblockingtimeout.value, true))
				break;

			// find a slot for the new client
			for (clientnum = 0, newclient = svs.clients;clientnum < svs.maxclients;clientnum++, newclient++)
			{
				netconn_t *conn;
				if (!newclient->active && (newclient->netconnection = conn = NetConn_Open(mysocket, peeraddress)) != NULL)
				{
					// connect to the client
					// everything is allocated, just fill in the details
					dp_strlcpy (conn->address, addressstring2, sizeof (conn->address));
					if (developer_extra.integer)
						Con_DPrintf("Datagram_ParseConnectionless: sending CCREP_ACCEPT to %s.\n", addressstring2);
					// send back the info about the server connection
					SZ_Clear(&sv_message);
					// save space for the header, filled in later
					MSG_WriteLong(&sv_message, 0);
					MSG_WriteByte(&sv_message, CCREP_ACCEPT);
					MSG_WriteLong(&sv_message, LHNETADDRESS_GetPort(LHNET_AddressFromSocket(conn->mysocket)));
					StoreBigLong(sv_message.data, NETFLAG_CTL | (sv_message.cursize & NETFLAG_LENGTH_MASK));
					NetConn_Write(mysocket, sv_message.data, sv_message.cursize, peeraddress);
					SZ_Clear(&sv_message);
					// now set up the client struct
					SV_ConnectClient(clientnum, conn);
					NetConn_Heartbeat(1);
					return true;
				}
			}

			if (developer_extra.integer)
				Con_DPrintf("Datagram_ParseConnectionless: sending CCREP_REJECT \"Server is full.\" to %s.\n", addressstring2);
			// no room; try to let player know
			SZ_Clear(&sv_message);
			// save space for the header, filled in later
			MSG_WriteLong(&sv_message, 0);
			MSG_WriteByte(&sv_message, CCREP_REJECT);
			MSG_WriteString(&sv_message, "Server is full.\n");
			StoreBigLong(sv_message.data, NETFLAG_CTL | (sv_message.cursize & NETFLAG_LENGTH_MASK));
			NetConn_Write(mysocket, sv_message.data, sv_message.cursize, peeraddress);
			SZ_Clear(&sv_message);
			break;
		case CCREQ_SERVER_INFO:
			if (developer_extra.integer)
				Con_DPrintf("Datagram_ParseConnectionless: received CCREQ_SERVER_INFO from %s.\n", addressstring2);
			if(!(islocal || sv_public.integer > -1))
				break;

			if (NetConn_PreventFlood(peeraddress, sv.getstatusfloodaddresses, sizeof(sv.getstatusfloodaddresses) / sizeof(sv.getstatusfloodaddresses[0]), net_getstatusfloodblockingtimeout.value, false))
				break;

			if (sv.active && !strcmp(MSG_ReadString(&sv_message, sv_readstring, sizeof(sv_readstring)), "QUAKE"))
			{
				int numclients;
				char myaddressstring[128];
				if (developer_extra.integer)
					Con_DPrintf("Datagram_ParseConnectionless: sending CCREP_SERVER_INFO to %s.\n", addressstring2);
				SZ_Clear(&sv_message);
				// save space for the header, filled in later
				MSG_WriteLong(&sv_message, 0);
				MSG_WriteByte(&sv_message, CCREP_SERVER_INFO);
				LHNETADDRESS_ToString(LHNET_AddressFromSocket(mysocket), myaddressstring, sizeof(myaddressstring), true);
				MSG_WriteString(&sv_message, myaddressstring);
				MSG_WriteString(&sv_message, hostname.string);
				MSG_WriteString(&sv_message, sv.worldbasename);
				// How many clients are there?
				for (i = 0, numclients = 0;i < svs.maxclients;i++)
					if (svs.clients[i].active)
						numclients++;
				MSG_WriteByte(&sv_message, numclients);
				MSG_WriteByte(&sv_message, svs.maxclients);
				MSG_WriteByte(&sv_message, NET_PROTOCOL_VERSION);
				StoreBigLong(sv_message.data, NETFLAG_CTL | (sv_message.cursize & NETFLAG_LENGTH_MASK));
				NetConn_Write(mysocket, sv_message.data, sv_message.cursize, peeraddress);
				SZ_Clear(&sv_message);
			}
			break;
		case CCREQ_PLAYER_INFO:
			if (developer_extra.integer)
				Con_DPrintf("Datagram_ParseConnectionless: received CCREQ_PLAYER_INFO from %s.\n", addressstring2);
			if(!(islocal || sv_public.integer > -1))
				break;

			if (NetConn_PreventFlood(peeraddress, sv.getstatusfloodaddresses, sizeof(sv.getstatusfloodaddresses) / sizeof(sv.getstatusfloodaddresses[0]), net_getstatusfloodblockingtimeout.value, false))
				break;

			if (sv.active)
			{
				int playerNumber, activeNumber, clientNumber;
				client_t *client;

				playerNumber = MSG_ReadByte(&sv_message);
				activeNumber = -1;
				for (clientNumber = 0, client = svs.clients; clientNumber < svs.maxclients; clientNumber++, client++)
					if (client->active && ++activeNumber == playerNumber)
						break;
				if (clientNumber != svs.maxclients)
				{
					SZ_Clear(&sv_message);
					// save space for the header, filled in later
					MSG_WriteLong(&sv_message, 0);
					MSG_WriteByte(&sv_message, CCREP_PLAYER_INFO);
					MSG_WriteByte(&sv_message, playerNumber);
					MSG_WriteString(&sv_message, client->name);
					MSG_WriteLong(&sv_message, client->colors);
					MSG_WriteLong(&sv_message, client->frags);
					MSG_WriteLong(&sv_message, (int)(host.realtime - client->connecttime));
					if(sv_status_privacy.integer)
						MSG_WriteString(&sv_message, client->netconnection ? "hidden" : "botclient");
					else
						MSG_WriteString(&sv_message, client->netconnection ? client->netconnection->address : "botclient");
					StoreBigLong(sv_message.data, NETFLAG_CTL | (sv_message.cursize & NETFLAG_LENGTH_MASK));
					NetConn_Write(mysocket, sv_message.data, sv_message.cursize, peeraddress);
					SZ_Clear(&sv_message);
				}
			}
			break;
		case CCREQ_RULE_INFO:
			if (developer_extra.integer)
				Con_DPrintf("Datagram_ParseConnectionless: received CCREQ_RULE_INFO from %s.\n", addressstring2);
			if(!(islocal || sv_public.integer > -1))
				break;

			// no flood check here, as it only returns one cvar for one cvar and clients may iterate quickly

			if (sv.active)
			{
				char *prevCvarName;
				cvar_t *var;

				// find the search start location
				prevCvarName = MSG_ReadString(&sv_message, sv_readstring, sizeof(sv_readstring));
				var = Cvar_FindVarAfter(&cvars_all, prevCvarName, CF_NOTIFY);

				// send the response
				SZ_Clear(&sv_message);
				// save space for the header, filled in later
				MSG_WriteLong(&sv_message, 0);
				MSG_WriteByte(&sv_message, CCREP_RULE_INFO);
				if (var)
				{
					MSG_WriteString(&sv_message, var->name);
					MSG_WriteString(&sv_message, var->string);
				}
				StoreBigLong(sv_message.data, NETFLAG_CTL | (sv_message.cursize & NETFLAG_LENGTH_MASK));
				NetConn_Write(mysocket, sv_message.data, sv_message.cursize, peeraddress);
				SZ_Clear(&sv_message);
			}
			break;
		case CCREQ_RCON:
			if (developer_extra.integer)
				Con_DPrintf("Datagram_ParseConnectionless: received CCREQ_RCON from %s.\n", addressstring2);
			if (sv.active && !rcon_secure.integer)
			{
				char password[2048];
				char cmd[2048];
				char *s;
				char *endpos;
				const char *userlevel;
				dp_strlcpy(password, MSG_ReadString(&sv_message, sv_readstring, sizeof(sv_readstring)), sizeof(password));
				dp_strlcpy(cmd, MSG_ReadString(&sv_message, sv_readstring, sizeof(sv_readstring)), sizeof(cmd));
				s = cmd;
				endpos = cmd + strlen(cmd) + 1; // one behind the NUL, so adding strlen+1 will eventually reach it
				userlevel = RCon_Authenticate(peeraddress, password, s, endpos, plaintext_matching, NULL, 0);
				RCon_Execute(mysocket, peeraddress, addressstring2, userlevel, s, endpos, true);
				return true;
			}
			break;
		default:
			break;
		}
		SZ_Clear(&sv_message);
		// we may not have liked the packet, but it was a valid control
		// packet, so we're done processing this packet now
		return true;
	}
	if (host_client)
	{
		if ((ret = NetConn_ReceivedMessage(host_client->netconnection, data, length, sv.protocol, host_client->begun ? net_messagetimeout.value : net_connecttimeout.value)) == 2)
		{
			SV_ReadClientMessage();
			return ret;
		}
	}
	return 0;
}

void NetConn_ServerFrame(void)
{
	unsigned i;
	int length;
	lhnetaddress_t peeraddress;
	unsigned char readbuffer[NET_HEADERSIZE+NET_MAXMESSAGE];

	for (i = 0;i < sv_numsockets;i++)
		while (sv_sockets[i] && (length = NetConn_Read(sv_sockets[i], readbuffer, sizeof(readbuffer), &peeraddress)) > 0)
			NetConn_ServerParsePacket(sv_sockets[i], readbuffer, length, &peeraddress);
}

#ifdef CONFIG_MENU
void NetConn_QueryMasters(qbool querydp, qbool queryqw)
{
	unsigned i, j;
	unsigned masternum;
	lhnetaddress_t masteraddress;
	char request[256];
	char lookupstring[128];

	if (serverlist_cachecount >= SERVERLIST_TOTALSIZE)
		return;

	memset(dpmasterstatus, 0, sizeof(dpmasterstatus));
	memset(qwmasterstatus, 0, sizeof(qwmasterstatus));

	if (querydp)
	{
		for (i = 0;i < cl_numsockets;i++)
		{
			if (cl_sockets[i])
			{
				const char *cmdname, *extraoptions;
				lhnetaddresstype_t af = LHNETADDRESS_GetAddressType(LHNET_AddressFromSocket(cl_sockets[i]));

				// build the getservers message to send to the dpmaster master servers
				if (LHNETADDRESS_GetAddressType(LHNET_AddressFromSocket(cl_sockets[i])) == LHNETADDRESSTYPE_INET6)
				{
					cmdname = "getserversExt";
					extraoptions = " ipv4 ipv6";  // ask for IPv4 and IPv6 servers
				}
				else
				{
					cmdname = "getservers";
					extraoptions = "";
				}
				dpsnprintf(request, sizeof(request), "\377\377\377\377%s %s %u empty full%s",
					cmdname, gamenetworkfiltername, NET_PROTOCOL_VERSION, extraoptions);

				// search internet
				for (masternum = 0; masternum < DPMASTER_COUNT; ++masternum)
				{
					if(sv_masters[masternum].string[0]
					&& LHNETADDRESS_FromString(&masteraddress, sv_masters[masternum].string, DPMASTER_PORT)
					&& LHNETADDRESS_GetAddressType(&masteraddress) == af)
					{
						if (serverlist_consoleoutput || net_slist_debug.integer)
						{
							LHNETADDRESS_ToString(&masteraddress, lookupstring, sizeof(lookupstring), true);
							Con_Printf("Querying DP master %s (resolved from %s)\n", lookupstring, sv_masters[masternum].string);
						}
						masterquerycount++;
						NetConn_WriteString(cl_sockets[i], request, &masteraddress);
						dpmasterstatus[masternum] = MASTER_TX_QUERY;
					}
				}

				// search favorite servers
				for(j = 0; j < nFavorites; ++j)
					if(LHNETADDRESS_GetAddressType(&favorites[j]) == af
					&& LHNETADDRESS_ToString(&favorites[j], lookupstring, sizeof(lookupstring), true))
						NetConn_ClientParsePacket_ServerList_PrepareQuery(PROTOCOL_DARKPLACES7, lookupstring, true);
			}
		}
	}

	// only query QuakeWorld servers when the user wants to
	if (queryqw)
	{
		dpsnprintf(request, sizeof(request), "c\n");

		for (i = 0;i < cl_numsockets;i++)
		{
			if (cl_sockets[i])
			{
				lhnetaddresstype_t af = LHNETADDRESS_GetAddressType(LHNET_AddressFromSocket(cl_sockets[i]));

				// search internet
				for (masternum = 0; masternum < QWMASTER_COUNT; ++masternum)
				{
					if(sv_qwmasters[masternum].string[0]
					&& LHNETADDRESS_FromString(&masteraddress, sv_qwmasters[masternum].string, QWMASTER_PORT)
					&& LHNETADDRESS_GetAddressType(&masteraddress) == af)
					{
						if (serverlist_consoleoutput || net_slist_debug.integer)
						{
							LHNETADDRESS_ToString(&masteraddress, lookupstring, sizeof(lookupstring), true);
							Con_Printf("Querying QW master %s (resolved from %s)\n", lookupstring, sv_qwmasters[masternum].string);
						}
						masterquerycount++;
						NetConn_Write(cl_sockets[i], request, (int)strlen(request) + 1, &masteraddress);
						qwmasterstatus[masternum] = MASTER_TX_QUERY;
					}
				}

				// search favorite servers
				for(j = 0; j < nFavorites; ++j)
					if(LHNETADDRESS_GetAddressType(&favorites[j]) == af
					&& LHNETADDRESS_ToString(&favorites[j], lookupstring, sizeof(lookupstring), true))
						NetConn_ClientParsePacket_ServerList_PrepareQuery(PROTOCOL_QUAKEWORLD, lookupstring, true);
			}
		}
	}

	if (!masterquerycount)
	{
		Con_Print(CON_ERROR "Unable to query master servers, no suitable network sockets active.\n");
		dp_strlcpy(cl_connect_status, "No network", sizeof(cl_connect_status));
	}
}
#endif

void NetConn_Heartbeat(int priority)
{
	lhnetaddress_t masteraddress;
	uint8_t masternum;
	lhnetsocket_t *mysocket;

	// if it's a state change (client connected), limit next heartbeat to no
	// more than 30 sec in the future
	if (priority == 1 && nextheartbeattime > host.realtime + 30.0)
		nextheartbeattime = host.realtime + 30.0;

	// limit heartbeatperiod to 30 to 270 second range,
	// lower limit is to avoid abusing master servers with excess traffic,
	// upper limit is to avoid timing out on the master server (which uses
	// 300 sec timeout)
	if (sv_heartbeatperiod.value < 30)
		Cvar_SetValueQuick(&sv_heartbeatperiod, 30);
	if (sv_heartbeatperiod.value > 270)
		Cvar_SetValueQuick(&sv_heartbeatperiod, 270);

	// make advertising optional and don't advertise singleplayer games, and
	// only send a heartbeat as often as the admin wants
	if (sv.active && sv_public.integer > 0 && svs.maxclients >= 2 && (priority > 1 || host.realtime > nextheartbeattime))
	{
		nextheartbeattime = host.realtime + sv_heartbeatperiod.value;
		for (masternum = 0; masternum < DPMASTER_COUNT; ++masternum)
			if (sv_masters[masternum].string[0]
			&& LHNETADDRESS_FromString(&masteraddress, sv_masters[masternum].string, DPMASTER_PORT)
			&& (mysocket = NetConn_ChooseServerSocketForAddress(&masteraddress)))
				NetConn_WriteString(mysocket, "\377\377\377\377heartbeat DarkPlaces\x0A", &masteraddress);
	}
}

static void Net_Heartbeat_f(cmd_state_t *cmd)
{
	if (sv.active)
		NetConn_Heartbeat(2);
	else
		Con_Print("No server running, can not heartbeat to master server.\n");
}

static void PrintStats(netconn_t *conn)
{
	if ((cls.state == ca_connected && cls.protocol == PROTOCOL_QUAKEWORLD) || (sv.active && sv.protocol == PROTOCOL_QUAKEWORLD))
		Con_Printf("address=%21s canSend=%u sendSeq=%6u recvSeq=%6u\n", conn->address, !conn->sendMessageLength, conn->outgoing_unreliable_sequence, conn->qw.incoming_sequence);
	else
		Con_Printf("address=%21s canSend=%u sendSeq=%6u recvSeq=%6u\n", conn->address, !conn->sendMessageLength, conn->nq.sendSequence, conn->nq.receiveSequence);
	Con_Printf("unreliable messages sent   = %i\n", conn->unreliableMessagesSent);
	Con_Printf("unreliable messages recv   = %i\n", conn->unreliableMessagesReceived);
	Con_Printf("reliable messages sent     = %i\n", conn->reliableMessagesSent);
	Con_Printf("reliable messages received = %i\n", conn->reliableMessagesReceived);
	Con_Printf("packetsSent                = %i\n", conn->packetsSent);
	Con_Printf("packetsReSent              = %i\n", conn->packetsReSent);
	Con_Printf("packetsReceived            = %i\n", conn->packetsReceived);
	Con_Printf("receivedDuplicateCount     = %i\n", conn->receivedDuplicateCount);
	Con_Printf("droppedDatagrams           = %i\n", conn->droppedDatagrams);
}

void Net_Stats_f(cmd_state_t *cmd)
{
	netconn_t *conn;
	Con_Print("connections                =\n");
	for (conn = netconn_list;conn;conn = conn->next)
		PrintStats(conn);
}

#ifdef CONFIG_MENU
void Net_Refresh_f(cmd_state_t *cmd)
{
	if (m_state != m_slist)
	{
		Con_Print("Sending new requests to DP master servers\n");
		ServerList_QueryList(false, true, false, true);
		Con_Print("Listening for replies...\n");
	}
	else
		ServerList_QueryList(false, true, false, false);
}

void Net_Slist_f(cmd_state_t *cmd)
{
	ServerList_ResetMasks();
	serverlist_sortbyfield = SLIF_PING;
	serverlist_sortflags = 0;
	if (m_state != m_slist)
	{
		Con_Print("Sending requests to DP master servers\n");
		ServerList_QueryList(true, true, false, true);
		Con_Print("Listening for replies...\n");
	}
	else
		ServerList_QueryList(true, true, false, false);
}

void Net_SlistQW_f(cmd_state_t *cmd)
{
	ServerList_ResetMasks();
	serverlist_sortbyfield = SLIF_PING;
	serverlist_sortflags = 0;
	if (m_state != m_slist)
	{
		Con_Print("Sending requests to QW master servers\n");
		ServerList_QueryList(true, false, true, true);
		Con_Print("Listening for replies...\n");
	}
	else
		ServerList_QueryList(true, false, true, false);
}
#endif

void NetConn_Init(void)
{
	int i;
	unsigned j;
	lhnetaddress_t tempaddress;

	netconn_mempool = Mem_AllocPool("network connections", 0, NULL);
	Cmd_AddCommand(CF_SHARED, "net_stats", Net_Stats_f, "print network statistics");
#ifdef CONFIG_MENU
	Cmd_AddCommand(CF_CLIENT, "net_slist", Net_Slist_f, "query dp master servers and print all server information");
	Cmd_AddCommand(CF_CLIENT, "net_slistqw", Net_SlistQW_f, "query qw master servers and print all server information");
	Cmd_AddCommand(CF_CLIENT, "net_refresh", Net_Refresh_f, "query dp master servers and refresh all server information");
#endif
	Cmd_AddCommand(CF_SERVER, "heartbeat", Net_Heartbeat_f, "send a heartbeat to the master server (updates your server information)");
	Cvar_RegisterVariable(&net_test);
	Cvar_RegisterVariable(&net_usesizelimit);
	Cvar_RegisterVariable(&net_burstreserve);
	Cvar_RegisterVariable(&rcon_restricted_password);
	Cvar_RegisterVariable(&rcon_restricted_commands);
	Cvar_RegisterVariable(&rcon_secure_maxdiff);

#ifdef CONFIG_MENU
	Cvar_RegisterVariable(&net_slist_debug);
	Cvar_RegisterVariable(&net_slist_favorites);
	Cvar_RegisterCallback(&net_slist_favorites, NetConn_UpdateFavorites_c);
	Cvar_RegisterVariable(&net_slist_interval);
	Cvar_RegisterVariable(&net_slist_maxping);
	Cvar_RegisterVariable(&net_slist_maxtries);
	Cvar_RegisterVariable(&net_slist_pause);
	Cvar_RegisterCallback(&net_slist_pause, ServerList_RebuildViewList);
	Cvar_RegisterVariable(&net_slist_queriespersecond);
	Cvar_RegisterVariable(&net_slist_queriesperframe);
	Cvar_RegisterVariable(&net_slist_timeout);
#endif

#ifdef IP_TOS // register cvar only if supported
	Cvar_RegisterVariable(&net_tos_dscp);
#endif
	Cvar_RegisterVariable(&net_messagetimeout);
	Cvar_RegisterVariable(&net_connecttimeout);
	Cvar_RegisterVariable(&net_connect_entnum_ofs);
	Cvar_RegisterVariable(&net_connectfloodblockingtimeout);
	Cvar_RegisterVariable(&net_challengefloodblockingtimeout);
	Cvar_RegisterVariable(&net_getstatusfloodblockingtimeout);
	Cvar_RegisterVariable(&net_sourceaddresscheck);
	Cvar_RegisterVariable(&net_fakelag);
	Cvar_RegisterVariable(&net_fakeloss_send);
	Cvar_RegisterVariable(&net_fakeloss_receive);
	Cvar_RegisterVirtual(&net_fakelag, "cl_netlocalping");
	Cvar_RegisterVirtual(&net_fakeloss_send, "cl_netpacketloss_send");
	Cvar_RegisterVirtual(&net_fakeloss_receive, "cl_netpacketloss_receive");
	Cvar_RegisterVariable(&hostname);
	Cvar_RegisterVariable(&developer_networking);
	Cvar_RegisterVariable(&cl_netport);
	Cvar_RegisterCallback(&cl_netport, NetConn_CL_UpdateSockets_Callback);
	Cvar_RegisterVariable(&sv_netport);
	Cvar_RegisterCallback(&sv_netport, NetConn_sv_netport_Callback);
	Cvar_RegisterVariable(&net_address);
	Cvar_RegisterVariable(&net_address_ipv6);
	Cvar_RegisterVariable(&sv_public);
	Cvar_RegisterVariable(&sv_public_rejectreason);
	Cvar_RegisterVariable(&sv_heartbeatperiod);
	for (j = 0; j < DPMASTER_COUNT; ++j)
		Cvar_RegisterVariable(&sv_masters[j]);
#ifdef CONFIG_MENU
	for (j = 0; j < QWMASTER_COUNT; ++j)
		Cvar_RegisterVariable(&sv_qwmasters[j]);
#endif
	Cvar_RegisterVariable(&gameversion);
	Cvar_RegisterVariable(&gameversion_min);
	Cvar_RegisterVariable(&gameversion_max);
// COMMANDLINEOPTION: Server: -ip <ipaddress> sets the ip address of this machine for purposes of networking (default 0.0.0.0 also known as INADDR_ANY), use only if you have multiple network adapters and need to choose one specifically.
	if ((i = Sys_CheckParm("-ip")) && i + 1 < sys.argc)
	{
		if (LHNETADDRESS_FromString(&tempaddress, sys.argv[i + 1], 0) == 1)
		{
			Con_Printf("-ip option used, setting net_address to \"%s\"\n", sys.argv[i + 1]);
			Cvar_SetQuick(&net_address, sys.argv[i + 1]);
		}
		else
			Con_Printf(CON_ERROR "-ip option used, but unable to parse the address \"%s\"\n", sys.argv[i + 1]);
	}
// COMMANDLINEOPTION: Server: -port <portnumber> sets the port to use for a server (default 26000, the same port as QUAKE itself), useful if you host multiple servers on your machine
	if (((i = Sys_CheckParm("-port")) || (i = Sys_CheckParm("-ipport")) || (i = Sys_CheckParm("-udpport"))) && i + 1 < sys.argc)
	{
		i = atoi(sys.argv[i + 1]);
		if (i >= 0 && i < 65536)
		{
			Con_Printf("-port option used, setting port cvar to %i\n", i);
			Cvar_SetValueQuick(&sv_netport, i);
		}
		else
			Con_Printf(CON_ERROR "-port option used, but %i is not a valid port number\n", i);
	}
	cl_numsockets = 0;
	sv_numsockets = 0;
	cl_message.data = cl_message_buf;
	cl_message.maxsize = sizeof(cl_message_buf);
	cl_message.cursize = 0;
	sv_message.data = sv_message_buf;
	sv_message.maxsize = sizeof(sv_message_buf);
	sv_message.cursize = 0;
	LHNET_Init();
	if (Thread_HasThreads())
		netconn_mutex = Thread_CreateMutex();
}

void NetConn_Shutdown(void)
{
	NetConn_CloseClientPorts();
	NetConn_CloseServerPorts();
	LHNET_Shutdown();
	if (netconn_mutex)
		Thread_DestroyMutex(netconn_mutex);
	netconn_mutex = NULL;
}

