/*
serverside master heartbeat code
clientside master queries and server ping/polls
*/

#include "quakedef.h"
#include "cl_master.h"
#include "netinc.h"

#define FAVOURITESFILE "favourites.txt"

qboolean	sb_favouriteschanged;	//some server's favourite flag got changed. we'll need to resave the list.
qboolean	sb_enablequake2;
qboolean	sb_enablequake3;
qboolean	sb_enablenetquake;
qboolean	sb_enabledarkplaces;
qboolean	sb_enablequakeworld;

void Master_DetermineMasterTypes(void)
{
	if (com_protocolname.modified)
	{
		char tok[MAX_QPATH];
		char *prot = com_protocolname.string;
		char *game;
		com_protocolname.modified = 0;

		sb_enabledarkplaces = true;	//dpmaster is not specific to any single game/mod, so can be left enabled even when running q2 etc, for extra redundancy.
		sb_enablequake2 = false;
		sb_enablequake3 = false;
		sb_enablenetquake = false;
		sb_enablequakeworld = false;

		for (prot = com_protocolname.string; *prot;)
		{
			prot = COM_ParseOut(prot, tok, sizeof(tok));
			game = tok;
			//this is stupid, but hey
			if (!Q_strncasecmp(game, "FTE-", 4))
				game += 4;
			else if (!Q_strncasecmp(game, "DarkPlaces-", 11))
				game += 11;

			if (!strcmp(game, "Quake2"))
				sb_enablequake2 = true;
			if (!strcmp(game, "Quake3"))
				sb_enablequake3 = true;
			//for DP compatibility, we consider these separate(ish) games.
			if (!strcmp(game, "Quake") || !strcmp(game, "Hipnotic") || !strcmp(game, "Rogue"))
				sb_enablenetquake = sb_enablequakeworld = true;
		}
	}
}
qboolean Master_MasterProtocolIsEnabled(enum masterprotocol_e protocol)
{
	switch (protocol)
	{
	case MP_DPMASTER:
		return sb_enabledarkplaces;
	#ifdef Q2SERVER
	case MP_QUAKE2:
		return sb_enablequake2;
	#endif
	#ifdef Q3SERVER
	case MP_QUAKE3:
		return sb_enablequake3;
	#endif
	case MP_QUAKEWORLD:
		return sb_enablequakeworld;
	default:
		return false;
	}
}

#define MAX_MASTER_ADDRESSES 4	//each master might have multiple dns addresses, typically both ipv4+ipv6. we want to report to both address families so we work with remote single-stack hosts.

#ifdef HAVE_SERVER
static void QDECL Net_Masterlist_Callback(struct cvar_s *var, char *oldvalue);
#ifdef HAVE_LEGACY
static void SV_SetMaster_f (void);
#endif
#else
#define Net_Masterlist_Callback NULL
#endif

extern cvar_t sv_public;
extern cvar_t sv_reportheartbeats;
extern cvar_t sv_heartbeat_interval;
extern cvar_t sv_heartbeat_checks;

extern cvar_t sv_listen_qw;
extern cvar_t sv_listen_nq;
extern cvar_t sv_listen_dp;
extern cvar_t sv_listen_q3;

typedef struct {
	enum masterprotocol_e protocol;
	cvar_t		cv;
	char		*comment;

	qboolean	announced;		//when set, hide when sv_reportheartbeats 2
	qboolean	needsresolve;	//set any time the cvar is modified
	qboolean	resolving;	//set any time the cvar is modified
	netadr_t	adr[MAX_MASTER_ADDRESSES];
} net_masterlist_t;
static net_masterlist_t net_masterlist[] = {
#if 0	//for debugging
	{MP_DPMASTER,	CVARFC("net_masterextra1",		"localhost:27950",										CVAR_NOSAVE, Net_Masterlist_Callback)}, //admin: the reader...
#else

#ifndef QUAKETC
	//user-specified master lists.
	{MP_QUAKEWORLD, CVARC("net_qwmaster1", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster2", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster3", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster4", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster5", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster6", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster7", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster8", "", Net_Masterlist_Callback)},
#endif

	//dpmaster is the generic non-quake-specific master protocol that we use for custom stand-alone mods.
	{MP_DPMASTER,	CVARAFC("net_master1", "", "sv_master1", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master2", "", "sv_master2", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master3", "", "sv_master3", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master4", "", "sv_master4", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master5", "", "sv_master5", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master6", "", "sv_master6", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master7", "", "sv_master7", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master8", "", "sv_master8", 0, Net_Masterlist_Callback)},

#ifdef Q2CLIENT
	{MP_QUAKE2,		CVARC("net_q2master1", "", Net_Masterlist_Callback)},
	{MP_QUAKE2,		CVARC("net_q2master2", "", Net_Masterlist_Callback)},
	{MP_QUAKE2,		CVARC("net_q2master3", "", Net_Masterlist_Callback)},
	{MP_QUAKE2,		CVARC("net_q2master4", "", Net_Masterlist_Callback)},
#endif

#ifdef Q3CLIENT
	{MP_QUAKE3,		CVARC("net_q3master1", "", Net_Masterlist_Callback)},
	{MP_QUAKE3,		CVARC("net_q3master2", "", Net_Masterlist_Callback)},
	{MP_QUAKE3,		CVARC("net_q3master3", "", Net_Masterlist_Callback)},
	{MP_QUAKE3,		CVARC("net_q3master4", "", Net_Masterlist_Callback)},
#endif

#ifdef HAVE_PACKET
#ifndef QUAKETC
	//engine-specified/maintained master lists (so users can be lazy and update the engine without having to rewrite all their configs).
	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra1", ""/*"qwmaster.ocrana.de:27000" not responding*/,	CVAR_NOSAVE, Net_Masterlist_Callback),	"Ocrana(2nd)"},	//german. admin unknown
	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra2", ""/*"masterserver.exhale.de:27000" dns dead*/,		CVAR_NOSAVE, Net_Masterlist_Callback)},	//german. admin unknown
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra3", "asgaard.morphos-team.net:27000",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Germany, admin: bigfoot"},
	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra4", "master.quakeservers.net:27000",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Germany, admin: raz0?"},
	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra5", "qwmaster.fodquake.net:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"admin: bigfoot"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27000",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27002",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master For CTF Servers"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27003",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master For TeamFortress Servers"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27004",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master For Miscilaneous Servers"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27006",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master For Deathmatch Servers"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"150.254.66.120:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"Poland"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"62.112.145.129:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"Ocrana (original)"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"master.edome.net",							CVAR_NOSAVE, Net_Masterlist_Callback),	"edome"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"qwmaster.barrysworld.com",					CVAR_NOSAVE, Net_Masterlist_Callback),	"barrysworld"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"213.221.174.165:27000",					CVAR_NOSAVE, Net_Masterlist_Callback),	"unknown1"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"195.74.0.8",								CVAR_NOSAVE, Net_Masterlist_Callback),	"unknown2"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"204.182.161.2",							CVAR_NOSAVE, Net_Masterlist_Callback),	"unknown5"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"kubus.rulez.pl:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"kubus.rulez.pl"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"telefrag.me:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"telefrag.me"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"master.teamdamage.com:27000",				CVAR_NOSAVE, Net_Masterlist_Callback),	"master.teamdamage.com"},

	//Total conversions will need to define their own in defaults.cfg or whatever.
	{MP_DPMASTER,	CVARFC("net_masterextra1",		"master.frag-net.com:27950",					CVAR_NOSAVE, Net_Masterlist_Callback)}, //admin: Eukara
//	{MP_DPMASTER,	CVARFC("net_masterextra1",		""/*"ghdigital.com:27950"*/,					CVAR_NOSAVE, Net_Masterlist_Callback)}, //(was 69.59.212.88) admin: LordHavoc
	{MP_DPMASTER,	CVARFC("net_masterextra2",		"dpmaster.deathmask.net:27950",					CVAR_NOSAVE, Net_Masterlist_Callback)}, //admin: Willis
	{MP_DPMASTER,	CVARFC("net_masterextra3",		"dpmaster.tchr.no:27950",						CVAR_NOSAVE, Net_Masterlist_Callback)}, //admin: tChr
#else
	{MP_DPMASTER,	CVARFC("net_masterextra1",		"",												CVAR_NOSAVE, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARFC("net_masterextra2",		"",												CVAR_NOSAVE, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARFC("net_masterextra3",		"",												CVAR_NOSAVE, Net_Masterlist_Callback)},
#endif
	{MP_DPMASTER,	CVARFC("net_masterextra4",		"",												CVAR_NOSAVE, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARFC("net_masterextra5",		"",												CVAR_NOSAVE, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARFC("net_masterextra6",		"",												CVAR_NOSAVE, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARFC("net_masterextra7",		"",												CVAR_NOSAVE, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARFC("net_masterextra8",		"",												CVAR_NOSAVE, Net_Masterlist_Callback)},

#ifdef Q2CLIENT
//	{MP_QUAKE2,		CVARFC("net_q2masterextra1",	"satan.idsoftware.com:27900",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Official Quake2 master server"},
//	{MP_QUAKE2,		CVARFC("net_q2masterextra1",	"master.planetgloom.com:27900",					CVAR_NOSAVE, Net_Masterlist_Callback)},	//?
	{MP_QUAKE2,		CVARFC("net_q2masterextra1",	"netdome.biz:27900",							CVAR_NOSAVE, Net_Masterlist_Callback)},	//?
	{MP_QUAKE2,		CVARFC("net_q2masterextra2",	"master.quakeservers.net:27900",				CVAR_NOSAVE, Net_Masterlist_Callback)},	//?
	{MP_QUAKE2,		CVARFC("net_q2masterextra3",	"master.q2servers.com:27900",					CVAR_NOSAVE, Net_Masterlist_Callback)},	//fucked. server doesn't give responses... but we DO want to send it heartbeats so we can at least get listed to eg q2rtx. maybe they'll fix their bugs.
#endif

#ifdef Q3CLIENT
//	{MP_QUAKE3,		CVARFC("net_q3masterextra1",	"masterserver.exhale.de:27950",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Official Quake3 master server"},
	{MP_QUAKE3,		CVARFC("net_q3masterextra1",	"master.quake3arena.com:27950",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Official Quake3 master server"},
	{MP_QUAKE3,		CVARFC("net_q3masterextra2",	"master0.excessiveplus.net:27950",				CVAR_NOSAVE, Net_Masterlist_Callback),	"DE: Excessive Plus"},
	{MP_QUAKE3,		CVARFC("net_q3masterextra3",	"master.ioquake3.org:27950",					CVAR_NOSAVE, Net_Masterlist_Callback),	"DE: ioquake3"},
	{MP_QUAKE3,		CVARFC("net_q3masterextra4",	"master.huxxer.de:27950",						CVAR_NOSAVE, Net_Masterlist_Callback),	"DE: BMA Team"},
	{MP_QUAKE3,		CVARFC("net_q3masterextra5",	"master.maverickservers.com:27950",				CVAR_NOSAVE, Net_Masterlist_Callback),	"US: Maverickservers.com"},
	{MP_QUAKE3,		CVARFC("net_q3masterextra8",	"master3.idsoftware.com:27950",					CVAR_NOSAVE, Net_Masterlist_Callback),	"US: id Software Quake III Master"},
#endif
#endif
#endif

	{MP_UNSPECIFIED, CVAR(NULL, NULL)}
};
#ifdef HAVE_SERVER
qboolean SV_Master_AddressIsMaster(netadr_t *adr)
{
	size_t i, j;
	//never throttle packets from master servers. we don't want to go missing.
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		if (!net_masterlist[i].protocol || net_masterlist[i].resolving || net_masterlist[i].needsresolve)
			continue;
		for (j = 0; j < MAX_MASTER_ADDRESSES; j++)
			if (net_masterlist[i].adr[j].type != NA_INVALID)
				if (NET_CompareBaseAdr(&net_from, &net_masterlist[i].adr[j]))
					return true;
	}
	return false;
}
void SV_Master_HeartbeatResponse(netadr_t *adr, const char *challenge)
{
//ftemaster responds from two different ports. one direct, one indirect.
//if there's a NAT/firewall issue then the indirect response is lost.
//thus if we get more than X direct responses without any indirect ones in that time, start warning because the server is unreachable and thus probably pointless.
	qboolean directresponse = (*challenge=='?');	//this is a fallback (direct) response.
	static size_t directresponse_count;
	static double okaytimestamp;	//timer throttle, in case there's spoofing going on. we won't get any DoS attacks but its still annoying.

	//'?' denotes the master trying to send a direct response.
	//length>0&&length<12 denotes a dpmaster query
	//no challengedenotes a client (if they get our address then its all good)
	if (*challenge && strlen(challenge) <= 12)
		return;	//outdated dpmaster. these are (probably) direct responses that don't really mean anything.

	if (NET_ClassifyAddress(adr, NULL) != ASCOPE_NET)
		return;	//ignore any broadcast lan probes.

	//if we're getting fake-direct responses without typical indirect ones then someone's probably being obnoxious and trying to trigger some false positives.
	//FIXME: spoofed fake-direct responses can still be an annoyance, but this is informative only, so not a real issue.
	if (directresponse && !SV_Master_AddressIsMaster(adr))
		directresponse = false;

	if (directresponse && sv_public.ival == 1)
		directresponse_count++;		//bad...
	else
		directresponse_count = 0;	//yay! we're reachable!... for now...

	if (directresponse_count >= 4)
	{
		if ((realtime - okaytimestamp) > 60)
		{
			Con_Printf(CON_ERROR"WARNING: 'sv_public %s' is ineffective, this server appears unreachable due to NAT/Firewall issues\n", sv_public.string);
			okaytimestamp = realtime;
			directresponse_count = 0;
		}
	}
	else
		okaytimestamp = realtime;
}

static void QDECL Net_Masterlist_Callback(struct cvar_s *var, char *oldvalue)
{
	int i;

	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		if (var == &net_masterlist[i].cv)
			break;
	}

	if (!net_masterlist[i].cv.name)
		return;

	net_masterlist[i].needsresolve = true;
}

static void SV_Master_SingleHeartbeat(net_masterlist_t *master)
{
	char		string[2048];
	qboolean	madeqwstring = false;
	char		adr[MAX_ADR_SIZE];
	netadr_t	*na;
	int i;
	int e;

	for (i = 0; i < MAX_MASTER_ADDRESSES; i++)
	{
		na = &master->adr[i];
		if (na->port)
		{
			e = -1;
			switch(master->protocol)
			{
			case MP_QUAKEWORLD:
				if ((svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM
#ifdef VM_LUA
					|| svs.gametype == GT_LUA
#endif
					) && sv_listen_qw.value && na->type == NA_IP)
				{
					if (!madeqwstring)
					{
						int active, j;

						// count active users
						active = 0;
						for (j=0 ; j<svs.allocated_client_slots ; j++)
							if (svs.clients[j].state == cs_connected ||
							svs.clients[j].state == cs_spawned )
								active++;

						sprintf (string, "%c\n%i\n%i\n", S2M_HEARTBEAT,
							svs.heartbeat_sequence, active);

						madeqwstring = true;
					}

					e = NET_SendPacket (svs.sockets, strlen(string), string, na);
				}
				break;
#ifdef Q2SERVER
			case MP_QUAKE2:
				if (svs.gametype == GT_QUAKE2 && sv_listen_qw.value && na->type == NA_IP)	//sv_listen==sv_listen_qw, yes, weird.
				{
					char *str = "\377\377\377\377""heartbeat\n%s\n%s";
					char info[8192];
					char q2users[8192];
					size_t i;
					const char *infos[] = {"hostname", "*version", "deathmatch", "fraglimit", "timelimit", "gamedir", "mapname", "maxclients", "dmflags", NULL};
					InfoBuf_ToString(&svs.info, info, sizeof(info), NULL, NULL, infos, NULL, NULL);
					q2users[0] = 0;
					for (i = 0; i < sv.allocated_client_slots; i++)
					{
						if (svs.clients[i].state >= cs_connected)
							Q_strncatz(q2users, va("%i %i \"%s\"\n", svs.clients[i].old_frags, SV_CalcPing(&svs.clients[i], false), svs.clients[i].name), sizeof(q2users));
					}
					e = NET_SendPacket (svs.sockets, strlen(str), va(str, info, q2users), na);
				}
				break;
#endif
			case MP_DPMASTER:
				//fte normally uses quakeworld masters for clients that support qw protocols, and dpmaster for clients that support nq protocols.
				//unfortunately qwmasters don't support ipv6, and total conversions don't want to use qwmasters.
				//we default to FTE-Quake when running quake so at least that part is fair.
				//however, I made QSS also look for FTE-Quake servers too, so that's messy with listen_nq 0, but that's true if listen_dp is set.
				//so we want to be quite permissive here, at least with custom builds that will default to these cvars set to 0.
				//Note that Darkplaces clients are supposed to be able to use the qw protocol, so it should be okay to heartbeat as Darkplaces-Quake here even when not doing any nq protocols.
				//either way, custom protocols tend to require ftemaster/dpmaster so we want to heartbeat regardless.
#if defined(NQPROT) && !defined(QUAKETC)
//				if (sv_listen_dp.value || sv_listen_nq.value || strcasecmp(com_protocolname.string, "FTE-Quake"))
#endif
				{
					//darkplaces here refers to the master server protocol, rather than the game protocol
					//(specifies that the server responds to infoRequest packets from the master)
					char *str = "\377\377\377\377""heartbeat DarkPlaces\n";
					e = NET_SendPacket (svs.sockets, strlen(str), str, na);
				}
				break;
			default:
				e = -2;
				break;
			}
			switch(e)
			{
			case -1:	//master not enabled for this game type
				break;
			case NETERR_SENT:
				if (sv_reportheartbeats.value)
				{
					if (sv_reportheartbeats.ival != 2 || !master->announced)
					{
						COM_Parse(master->cv.string);
						Con_TPrintf (S_COLOR_GRAY"Sending heartbeat to %s (%s)\n", NET_AdrToString (adr, sizeof(adr), na), com_token);
						master->announced = true;
					}
				}
				break;
			case NETERR_NOROUTE:
				if (sv_reportheartbeats.value)
				{
					if (sv_reportheartbeats.ival != 2 || !master->announced)
					{
						COM_Parse(master->cv.string);
						Con_TPrintf (CON_WARNING"No route for heartbeat to %s (%s)\n", NET_AdrToString (adr, sizeof(adr), na), com_token);
						master->announced = true;
					}
				}
				break;
			default:
			case NETERR_DISCONNECTED:
			case NETERR_MTU:
			case NETERR_CLOGGED:
				if (sv_reportheartbeats.value)
				{
					if (sv_reportheartbeats.ival != 2 || !master->announced)
					{
						COM_Parse(master->cv.string);
						Con_TPrintf (CON_ERROR"Failed to send heartbeat to %s (%s)\n", NET_AdrToString (adr, sizeof(adr), na), com_token);
						master->announced = true;
					}
				}
				break;
			}
		}
	}
}

//main thread
struct thr_res
{
	qboolean success;
	netadr_t na[MAX_MASTER_ADDRESSES];
	char str[1];	//trailing
};
static void SV_Master_Worker_Resolved(void *ctx, void *data, size_t a, size_t b)
{
	char adr[256];
	int i, j;
	struct thr_res *work = data;
	netadr_t *na;
	net_masterlist_t *master = &net_masterlist[a];

	master->resolving = false;

	//only accept the result if the master wasn't changed while we were still resolving it. no race conditions please.
	if (!strcmp(master->cv.string, work->str))
	{
		master->needsresolve = false;
		for (i = 0; i < MAX_MASTER_ADDRESSES; i++)
		{
			na = &master->adr[i];
			*na = work->na[i];
			master->needsresolve = false;

			switch (master->protocol)
			{
#ifdef Q2SERVER
			case MP_QUAKE2:
#endif
			case MP_QUAKEWORLD:
				//these masters have no ipv6 results query, so don't sent ipv6 heartbeats
				//(its possible that a router will convert to ipv4, but such a router is probably natted and its not really worth it)
				if (na->type != NA_IP)
					na->type = NA_INVALID;
				break;
			default:
				//these masters should do ipv4+ipv6, but not others.
				if (na->type != NA_IP && na->type != NA_IPV6)
					na->type = NA_INVALID;
				break;	//protocol
			}
			if (na->type != NA_INVALID)
			{
				for (j = 0; j < i; j++)
				{
					if (NET_CompareAdr(&master->adr[j], na))
					{	//a dupe of a previous one...
						na->type = NA_INVALID;
						break;
					}
				}
			}

			if (na->type == NA_INVALID)
				memset(na, 0, sizeof(*na));
			else
			{
				Con_DPrintf ("Resolved master \"%s\" to %s\n", master->cv.string, NET_AdrToString(adr, sizeof(adr), na));
				//fix up default ports if not specified
				if (!na->port)
				{
					safeswitch (master->protocol)
					{
					case MP_UNSPECIFIED:
#ifdef NQPROT
					case MP_NETQUAKE:
#endif
					case MP_DPMASTER:	na->port = BigShort (27950);	break;
#if defined(Q2CLIENT) || defined(Q2SERVER)
					case MP_QUAKE2:		na->port = BigShort (27900);	break;	//FIXME: verify
#endif
#ifdef Q3SERVER
					case MP_QUAKE3:		na->port = BigShort (27950);	break;
#endif
					case MP_QUAKEWORLD:	na->port = BigShort (27000);	break;
					safedefault:		na->port = BigShort (27950);	break;
					}
				}

				//some master servers require a ping to get them going or so
				if (sv.state)
				{
					//tcp masters require a route
					if (NET_AddrIsReliable(na))
					{
						struct dtlspeercred_s cred = {master->cv.string};
						NET_EnsureRoute(svs.sockets, master->cv.name, &cred, NULL, na, true);
					}

					//q2+qw masters are given a ping to verify that they're still up
					switch (master->protocol)
					{
#ifdef Q2SERVER
					case MP_QUAKE2:
						NET_SendPacket (svs.sockets, 8, "\xff\xff\xff\xffping", na);
						break;
#endif
					case MP_QUAKEWORLD:
						//qw does this for some reason, keep the behaviour even though its unreliable thus pointless
						NET_SendPacket (svs.sockets, 2, "k\0", na);
						break;
					default:
						break;
					}
				}
			}
		}
		if (!work->success)
			Con_TPrintf ("Couldn't resolve master \"%s\"\n", master->cv.string);
		else
			SV_Master_SingleHeartbeat(master);
	}
	Z_Free(work);
}

#if defined(SUPPORT_ICE)
struct stunheader_s
{
	unsigned short msgtype;
	unsigned short msglen;
	unsigned int magiccookie;
	unsigned int transactid[3];
};
static void SV_Master_Worker_Resolved_Broker(void *ctx, void *data, size_t a, size_t b)
{
	struct thr_res *work = data;
	if (svs.sockets && work->na[0].type != NA_INVALID)	//something resolved...
	{
		struct stunheader_s msg = {htons(1), htons(sizeof(msg)-20), BigLong(0x2112a442), {42,42,42}};

		//randomize the transaction id to avoid poisoning.
		if (!Sys_RandomBytes((qbyte*)msg.transactid, sizeof(msg.transactid)))
		{ 	//FIXME: not really random enough to avoid hacks. oh well.
			msg.transactid[0] = rand();
			msg.transactid[1] = rand();
			msg.transactid[2] = rand();
		}

		svs.sockets->srflx_tid[0] = msg.transactid[0];
		svs.sockets->srflx_tid[1] = msg.transactid[1];
		svs.sockets->srflx_tid[2] = msg.transactid[2];

		NET_SendPacket(svs.sockets, sizeof(msg), &msg, &work->na[0]);
	}
	Z_Free(work);
}
#endif
//worker thread
static void SV_Master_Worker_Resolve(void *ctx, void *data, size_t a, size_t b)
{
	char token[1024];
	int found = 0;
	qboolean first = true;
	char *str;
	struct thr_res *work = data;
	str = work->str;
	while (str && *str)
	{
		str = COM_ParseOut(str, token, sizeof(token));
		if (*token)
			found += NET_StringToAdr2(token, 0, &work->na[found], countof(work->na)-found, NULL);
		if (first && found)
			break;	//if we found one by name, don't try any fallback ip addresses.
		first = false;
	}
	work->success = !!found;

#if defined(SUPPORT_ICE)
	if (a==~(size_t)0)
		COM_AddWork(WG_MAIN, SV_Master_Worker_Resolved_Broker, NULL, work, a, b);
	else
#endif
		COM_AddWork(WG_MAIN, SV_Master_Worker_Resolved, NULL, work, a, b);
}

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
void SV_Master_Heartbeat (void)
{
	int			i;
	int interval = bound(85, sv_heartbeat_interval.ival, 600);

	if (sv_public.ival<=0 || SSV_IsSubServer())
		return;
#ifdef SUPPORT_ICE
	if (sv_public.ival == 2)
		return;	//using our broker service. we're configured as behind a nat so these addresses won't work anyway.
#endif

	if (realtime-interval - svs.last_heartbeat < interval)
		return;		// not time to send yet

	if ((sv.allocated_client_slots == 1) && !isDedicated)
		return;		//don't heartbeat in single-player, we don't even have a public socket open!

	svs.last_heartbeat = realtime-interval;

	svs.heartbeat_sequence++;

	Master_DetermineMasterTypes();

	// send to group master
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		if (!Master_MasterProtocolIsEnabled(net_masterlist[i].protocol))
			continue;

		if (net_masterlist[i].resolving)
			continue;

		if (net_masterlist[i].needsresolve)
		{
			if (!*net_masterlist[i].cv.string || *net_masterlist[i].cv.string == '*')
				memset(net_masterlist[i].adr, 0, sizeof(net_masterlist[i].adr));
			else
			{
				struct thr_res *work = Z_Malloc(sizeof(*work) + strlen(net_masterlist[i].cv.string));
				strcpy(work->str, net_masterlist[i].cv.string);
				net_masterlist[i].resolving = true;	//don't spam work
				COM_AddWork(WG_MAIN, SV_Master_Worker_Resolve, NULL, work, i, 0);
			}
		}
		else
			SV_Master_SingleHeartbeat(&net_masterlist[i]);
	}

#if defined(SUPPORT_ICE)
	if (*net_ice_broker.string)
	{
		const char *s = net_ice_broker.string;
		struct thr_res *work = Z_Malloc(sizeof(*work) + strlen(s));
		if (!strncmp(s, "tls://", 6) || !strncmp(s, "tcp://", 6) || !strncmp(s, "wss://", 6))
			s+=6;	//ignore weird prefixes here
		else if (!strncmp(s, "ws://", 5))
			s+=5;	//ignore dumb prefixes here
		strcpy(work->str, s);
		COM_AddWork(WG_MAIN, SV_Master_Worker_Resolve, NULL, work, ~(size_t)0, 0);
	}
#endif
}

#ifdef HAVE_LEGACY
static void SV_Master_Add(int type, char *stringadr)
{
	int i;

	//don't do dupes...
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		if (net_masterlist[i].protocol == type)
			if (!strcmp(net_masterlist[i].cv.string, stringadr))
			{
				svs.last_heartbeat = -99999;
				return;
			}
	}

	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		if (net_masterlist[i].protocol != type)
			continue;
		if (net_masterlist[i].cv.flags & CVAR_NOSAVE)
			continue;	//ignore our extras
		if (!*net_masterlist[i].cv.string)
			break;
	}

	if (!net_masterlist[i].cv.name)
	{
		Con_Printf ("Too many masters\n");
		return;
	}

	Cvar_Set(&net_masterlist[i].cv, stringadr);
	Con_Printf(CON_WARNING"setting %s to \"%s\"\n", net_masterlist[i].cv.name, stringadr);

	svs.last_heartbeat = -99999;
}

static void SV_Master_ClearType(int type)
{
	int i;
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		if (net_masterlist[i].protocol == type)
		{
			if (net_masterlist[i].cv.flags & CVAR_NOSAVE)
				continue;	//ignore our extras
			if (*net_masterlist[i].cv.string)
				Con_Printf(CON_WARNING"clearing %s (was \"%s\")\n", net_masterlist[i].cv.name, net_masterlist[i].cv.string);
			Cvar_Set(&net_masterlist[i].cv, "");
		}
	}
}
static void SV_Master_ClearAll(void)
{
	int i;
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		Cvar_Set(&net_masterlist[i].cv, "");
	}
}

/*
====================
SV_SetMaster_f

Make a master server current. deprecated in favour of setting numbered masters via configs/engine source code.
only supports qw masters.
====================
*/
static void SV_SetMaster_f (void)
{
	int		i;

	if (!strcmp(Cmd_Argv(1), "none"))
	{
		Cvar_Set(&sv_public, "0");	//go private.

		SV_Master_ClearAll();
		if (cl_warncmd.ival)
			Con_Printf ("Entering no-master mode\n");
		return;
	}
	if (!strcmp(Cmd_Argv(1), "clear"))
	{
		SV_Master_ClearType(MP_QUAKEWORLD);
		return;
	}

	if (sv_public.ival < 1)
		Con_Printf(CON_WARNING"%s used on private server (sv_public is \"%s\")\n", Cmd_Argv(0), sv_public.string);
	if (!strcmp(Cmd_Argv(1), "default"))
	{
		for (i = 0; net_masterlist[i].cv.name; i++)
			Cvar_Set(&net_masterlist[i].cv, net_masterlist[i].cv.enginevalue);
		return;
	}

	i = 1;
	if (!strcmp(Cmd_Argv(1), "add"))
		i++;
	else
		SV_Master_ClearType(MP_QUAKEWORLD);
	for ( ; i<Cmd_Argc() ; i++)
	{
		SV_Master_Add(MP_QUAKEWORLD, Cmd_Argv(i));
	}

	svs.last_heartbeat = -99999;
}
#endif

void SV_Master_ReResolve(void)
{
	int i;
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		net_masterlist[i].needsresolve = true;
	}
	//trigger a heartbeat at the next available opportunity.
	svs.last_heartbeat = -9999;
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void SV_Master_Shutdown (void)
{
	char		string[2048];
	char		adr[MAX_ADR_SIZE];
	int			i, j;
	netadr_t	*na;

	//note that if a master server actually blindly listens to this then its exploitable.
	//we send it out anyway as for us its all good.
	//master servers ought to try and check up on the status of the server first, if they listen to this.

	sprintf (string, "%c\n", S2M_SHUTDOWN);

	// send to group master
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		for (j = 0; j < MAX_MASTER_ADDRESSES; j++)
		{
			na = &net_masterlist[i].adr[j];
			if (na->port)
			{
				switch(net_masterlist[i].protocol)
				{
				case MP_QUAKEWORLD:
					if (sv_reportheartbeats.value)
						Con_TPrintf ("Sending shutdown to %s\n", NET_AdrToString (adr, sizeof(adr), na));

					NET_SendPacket (svs.sockets, strlen(string), string, na);
					break;
				//dp has no shutdown
				default:
					break;
				}
			}
		}
	}
}
#endif




#if defined(CL_MASTER) && !defined(SERVERONLY)

#define NET_GAMENAME_NQ		"QUAKE"

//rename to cl_master.c sometime

//the networking operates seperatly from the main code. This is so we can have full control over all parts of the server sending prints.
//when we send status to the server, it replys with a print command. The text to print contains the serverinfo.
//Q2's print command is a compleate 'print', while qw is just a 'p', thus we can distinguish the two easily.

//save favorites and allow addition of new ones from game?
//add filters some time

//remove dead servers.
//master was polled a minute ago and server was not on list - server on multiple masters would be awkward.

#include "netinc.h"

//the number of servers should be limited only by memory.

cvar_t slist_cacheinfo = CVAR("slist_cacheinfo", "0");	//this proves dangerous, memory wise.
cvar_t slist_writeserverstxt = CVAR("slist_writeservers", "1");

static void MasterInfo_RemoveAllPlayers(void);

master_t *master;
player_t *mplayers;
serverinfo_t *firstserver;
struct selectedserver_s selectedserver;

static serverinfo_t **visibleservers;
static int numvisibleservers;
static int maxvisibleservers;

static hostcachekey_t sortfield;
static qboolean sort_decreasing;
static qboolean sort_favourites;
static qboolean sort_categories;
static serverinfo_t *categorisingserver;	//returned for sorted server -1 (hacky)




typedef struct {
	hostcachekey_t fieldindex;

	float operandi;
	const char *operands;

	qboolean or;
	int compareop;
} visrules_t;
#define MAX_VISRULES 8
visrules_t visrules[MAX_VISRULES];
int numvisrules;




#define SLIST_MAXKEYS 64
char slist_keyname[SLIST_MAXKEYS][MAX_INFO_KEY];
int slist_customkeys;


#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifdef HAVE_IPV4
#define POLLUDP4SOCKETS 64	//it's big so we can have lots of messages when behind a firewall. Basically if a firewall only allows replys, and only remembers 3 servers per socket, we need this big cos it can take a while for a packet to find a fast optimised route and we might be waiting for a few secs for a reply the first time around.
int lastpollsockUDP4;
#else
#define POLLUDP4SOCKETS 0
#endif

#ifdef HAVE_IPV6
#define POLLUDP6SOCKETS 4	//it's non-zero so we can have lots of messages when behind a firewall. Basically if a firewall only allows replys, and only remembers 3 servers per socket, we need this big cos it can take a while for a packet to find a fast optimised route and we might be waiting for a few secs for a reply the first time around.
int lastpollsockUDP6;
#else
#define POLLUDP6SOCKETS 0
#endif

#ifdef HAVE_IPX
#define POLLIPXSOCKETS	2	//ipx isn't used as much. In fact, we only expect local servers to be using it. I'm not sure why I implemented it anyway. You might see a q2 server using it. Rarely.
int lastpollsockIPX;
#else
#define POLLIPXSOCKETS 0
#endif

#define FIRSTIPXSOCKET (0)
#define FIRSTUDP4SOCKET (FIRSTIPXSOCKET+POLLIPXSOCKETS)
#define FIRSTUDP6SOCKET (FIRSTUDP4SOCKET+POLLUDP4SOCKETS)
#define POLLTOTALSOCKETS (FIRSTUDP6SOCKET+POLLUDP6SOCKETS)
#if POLLTOTALSOCKETS>0
SOCKET pollsocketsList[POLLTOTALSOCKETS];
char pollsocketsBCast[POLLTOTALSOCKETS];

void Master_SetupSockets(void)
{
	int i;
	for (i = 0; i < POLLTOTALSOCKETS; i++)
		pollsocketsList[i] = INVALID_SOCKET;
}

static void CL_MasterListParse(netadrtype_t adrtype, int type, qboolean slashpad);
static int CL_ReadServerInfo(char *msg, enum masterprotocol_e prototype, qboolean favorite);
static void CL_ReadPingList(void);
#else
void Master_SetupSockets(void)
{
}
void Master_CheckPollSockets(void)
{
}
#endif


unsigned int Master_TotalCount(void)
{
	unsigned int count=0;
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		count++;
	}
	return count;
}

unsigned int Master_NumPolled(void)
{
	unsigned int count=0;
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		if (!info->sends)
			count++;
	}
	return count;
}
unsigned int Master_NumAlive(void)
{
	unsigned int count=0;
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		if (info->status&SRVSTATUS_ALIVE)
			count++;
	}
	return count;
}

//true if server is on a different master's list.
serverinfo_t *Master_InfoForServer (netadr_t *addr, const char *brokerid)
{
	serverinfo_t *info;
	if (!brokerid)
		brokerid="";

	for (info = firstserver; info; info = info->next)
	{
		if (!strcmp(info->brokerid, brokerid) && NET_CompareAdr(&info->adr, addr))
			return info;
	}
	return NULL;
}
serverinfo_t *Master_InfoForNum (int num)
{
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		if (num-- <=0)
			return info;
	}
	return NULL;
}

static void Master_HideServer(serverinfo_t *server)
{
	int i, j;
	for (i = 0; i < numvisibleservers;)
	{
		if (visibleservers[i] == server)
		{
			for (j = i; j < numvisibleservers-1; j++)
				visibleservers[j] = visibleservers[j+1];
			numvisibleservers--;
		}
		else
			 i++;
	}
	server->status &= ~SRVSTATUS_DISPLAYED;
}

static void Master_InsertAt(serverinfo_t *server, int pos)
{
	int i;
	if (numvisibleservers >= maxvisibleservers)
	{
		maxvisibleservers = maxvisibleservers+10;
		visibleservers = BZ_Realloc(visibleservers, maxvisibleservers*sizeof(serverinfo_t*));
	}
	for (i = numvisibleservers; i > pos; i--)
	{
		visibleservers[i] = visibleservers[i-1];
	}
	visibleservers[pos] = server;
	numvisibleservers++;

	server->status |= SRVSTATUS_DISPLAYED;
}

static qboolean Master_CompareInteger(int a, int b, slist_test_t rule)
{
	switch(rule)
	{
	case SLIST_TEST_CONTAINS:
		return !!(a&b);
	case SLIST_TEST_NOTCONTAIN:
		return !(a&b);
	case SLIST_TEST_LESSEQUAL:
		return a<=b;
	case SLIST_TEST_LESS:
		return a<b;
	case SLIST_TEST_STARTSWITH:
	case SLIST_TEST_EQUAL:
		return a==b;
	case SLIST_TEST_GREATER:
		return a>b;
	case SLIST_TEST_GREATEREQUAL:
		return a>=b;
	case SLIST_TEST_NOTSTARTSWITH:
	case SLIST_TEST_NOTEQUAL:
		return a!=b;
	}
	return false;
}
static qboolean Master_CompareString(const char *a, const char *b, slist_test_t rule)
{
	switch(rule)
	{
	case SLIST_TEST_STARTSWITH:
		return Q_strncasecmp(a, b, strlen(b))==0;
	case SLIST_TEST_NOTSTARTSWITH:
		return Q_strncasecmp(a, b, strlen(b))!=0;
	case SLIST_TEST_CONTAINS:
		return !!Q_strcasestr(a, b);
	case SLIST_TEST_NOTCONTAIN:
		return !Q_strcasestr(a, b);
	case SLIST_TEST_LESSEQUAL:
		return Q_strcasecmp(a, b)<=0;
	case SLIST_TEST_LESS:
		return Q_strcasecmp(a, b)<0;
	case SLIST_TEST_EQUAL:
		return Q_strcasecmp(a, b)==0;
	case SLIST_TEST_GREATER:
		return Q_strcasecmp(a, b)>0;
	case SLIST_TEST_GREATEREQUAL:
		return Q_strcasecmp(a, b)>=0;
	case SLIST_TEST_NOTEQUAL:
		return Q_strcasecmp(a, b)!=0;
	}
	return false;
}

char	*Master_ServerToString (char *s, int len, serverinfo_t *a)
{
	if (*a->brokerid)
	{
		if (a->adr.type==NA_INVALID)
			*s = 0;	//default broker... skip it for brevity.
		else
			NET_AdrToString(s, len, &a->adr);
//		Q_strncatz(s, "/", len);
		Q_strncatz(s, a->brokerid, len);
		return s;
	}
	return NET_AdrToString(s, len, &a->adr);
}

static int Master_BaseGame(serverinfo_t *a)
{
	int prot = a->special&SS_PROTOCOLMASK;
	return prot;
}

static qboolean Master_ServerIsGreater(serverinfo_t *a, serverinfo_t *b)
{
	if (sort_categories)
		if (a->qccategory != b->qccategory)
			return Master_CompareInteger(a->qccategory, b->qccategory, SLIST_TEST_LESS);

	if (sort_favourites)
		if ((a->special & SS_FAVORITE) != (b->special & SS_FAVORITE))
			return Master_CompareInteger(a->special & SS_FAVORITE, b->special & SS_FAVORITE, SLIST_TEST_LESS);

	switch(sortfield)
	{
	case SLKEY_ADDRESS:
		if (a->adr.type != b->adr.type)
			return a->adr.type < b->adr.type;
		if (a->adr.type == NA_IP)
		{
			int i;
			for (i = 0; i < 4; i++)
			{
				if (a->adr.address.ip[i] != b->adr.address.ip[i])
					return a->adr.address.ip[i] < b->adr.address.ip[i];
			}
		}
		if (a->adr.type == NA_IPV6)
		{
			int i;
			for (i = 0; i < 16; i++)
			{
				if (a->adr.address.ip6[i] != b->adr.address.ip6[i])
					return a->adr.address.ip6[i] < b->adr.address.ip6[i];
			}
		}
		return false;

	case SLKEY_BASEGAME:
		return Master_CompareInteger(Master_BaseGame(a), Master_BaseGame(b), SLIST_TEST_LESS);
	case SLKEY_FLAGS:
		return Master_CompareInteger(a->special&~SS_PROTOCOLMASK, b->special&~SS_PROTOCOLMASK, SLIST_TEST_LESS);
	case SLKEY_CUSTOM:
		break;
	case SLKEY_FRAGLIMIT:
		return Master_CompareInteger(a->fl, b->fl, SLIST_TEST_LESS);
	case SLKEY_FREEPLAYERS:
		return Master_CompareInteger(a->maxplayers - a->players, b->maxplayers - b->players, SLIST_TEST_LESS);
	case SLKEY_GAMEDIR:
		return Master_CompareString(a->gamedir, b->gamedir, SLIST_TEST_LESS);
	case SLKEY_MAP:
		return Master_CompareString(a->map, b->map, SLIST_TEST_LESS);
	case SLKEY_MAXPLAYERS:
		return Master_CompareInteger(a->maxplayers, b->maxplayers, SLIST_TEST_LESS);
	case SLKEY_NAME:
		return Master_CompareString(a->name, b->name, SLIST_TEST_LESS);
	case SLKEY_NUMPLAYERS:
		return Master_CompareInteger(a->players, b->players, SLIST_TEST_LESS);
	case SLKEY_NUMHUMANS:
		return Master_CompareInteger(a->numhumans, b->numhumans, SLIST_TEST_LESS);
	case SLKEY_NUMSPECTATORS:
		return Master_CompareInteger(a->numspectators, b->numspectators, SLIST_TEST_LESS);
	case SLKEY_NUMBOTS:
		return Master_CompareInteger(a->numbots, b->numbots, SLIST_TEST_LESS);
	case SLKEY_PING:
		return Master_CompareInteger(a->ping, b->ping, SLIST_TEST_LESS);
	case SLKEY_TIMELIMIT:
		return Master_CompareInteger(a->tl, b->tl, SLIST_TEST_LESS);
	case SLKEY_TOOMANY:
		break;

	case SLKEY_ISPROXY:
		return Master_CompareInteger(a->special & SS_PROXY, b->special & SS_PROXY, SLIST_TEST_LESS);
	case SLKEY_ISLOCAL:
		return Master_CompareInteger(a->special & SS_LOCAL, b->special & SS_LOCAL, SLIST_TEST_LESS);
	case SLKEY_ISFAVORITE:
		return Master_CompareInteger(a->special & SS_FAVORITE, b->special & SS_FAVORITE, SLIST_TEST_LESS);

	case SLKEY_CATEGORY:
		return Master_CompareInteger(a->qccategory, b->qccategory, SLIST_TEST_LESS);

	case SLKEY_MOD:
	case SLKEY_PROTOCOL:
	case SLKEY_QCSTATUS:
	case SLKEY_SERVERINFO:
	case SLKEY_PLAYER0:
	default:
		break;

	}
	return false;
}

qboolean Master_PassesMasks(serverinfo_t *a)
{
	int i;
	qboolean val, res;
//	qboolean enabled;

	//always filter out dead/unresponsive servers.
	if (!(a->status & SRVSTATUS_ALIVE))
		return false;

	val = 1;

	for (i = 0; i < numvisrules; i++)
	{
		safeswitch(visrules[i].fieldindex)
		{
		case SLKEY_PING:
			res = Master_CompareInteger(a->ping, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_NUMPLAYERS:
			res = Master_CompareInteger(a->players, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_MAXPLAYERS:
			res = Master_CompareInteger(a->maxplayers, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_FREEPLAYERS:
			res = Master_CompareInteger(a->freeslots, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_NUMBOTS:
			res = Master_CompareInteger(a->numbots, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_NUMHUMANS:
			res = Master_CompareInteger(a->numhumans, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_NUMSPECTATORS:
			res = Master_CompareInteger(a->numspectators, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_TIMELIMIT:
			res = Master_CompareInteger(a->tl, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_FRAGLIMIT:
			res = Master_CompareInteger(a->fl, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_PROTOCOL:
			res = Master_CompareInteger(a->fl, visrules[i].operandi, visrules[i].compareop);
			break;

		case SLKEY_MAP:
			res = Master_CompareString(a->map, visrules[i].operands, visrules[i].compareop);
			break;
		case SLKEY_NAME:
			res = Master_CompareString(a->name, visrules[i].operands, visrules[i].compareop);
			break;
		case SLKEY_GAMEDIR:
			res = Master_CompareString(a->gamedir, visrules[i].operands, visrules[i].compareop);
			break;

		case SLKEY_BASEGAME:
			res = Master_CompareInteger(Master_BaseGame(a), visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_FLAGS:
			res = Master_CompareInteger(a->special&~SS_PROTOCOLMASK, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_MOD:
			res = Master_CompareString(a->modname, visrules[i].operands, visrules[i].compareop);
			break;
		case SLKEY_QCSTATUS:
			res = Master_CompareString(a->qcstatus, visrules[i].operands, visrules[i].compareop);
			break;
		case SLKEY_CATEGORY:
			res = Master_CompareInteger(a->qccategory, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_ISFAVORITE:
			res = Master_CompareInteger(a->special&SS_FAVORITE, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_ISLOCAL:
			res = Master_CompareInteger(a->special&SS_LOCAL, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_ISPROXY:
			res = Master_CompareInteger(a->special&SS_PROXY, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_ADDRESS:		//not convinced its useful, but hey...
		case SLKEY_SERVERINFO:	//not really the sort of thing you sort.
		case SLKEY_TOOMANY:		//not real
		case SLKEY_PLAYER0://...SLKEY_CUSTOM
		case SLKEY_CUSTOM:
		safedefault:
			res = Master_CompareString(Master_ReadKeyString(a, visrules[i].fieldindex), visrules[i].operands, visrules[i].compareop);
			break;
		}
		if (visrules[i].or)
			val |= res;
		else
			val &= res;
	}

	return val;
}

void Master_ClearMasks(void)
{
	numvisrules = 0;
}

void Master_SetMaskString(qboolean or, hostcachekey_t field, const char *param, slist_test_t testop)
{
	if (numvisrules == MAX_VISRULES)
		return;	//just don't add it.

	visrules[numvisrules].fieldindex = field;
	visrules[numvisrules].compareop = testop;
	visrules[numvisrules].operands = param;
	visrules[numvisrules].operandi = atof(param);
	visrules[numvisrules].or = or;
	numvisrules++;
}
void Master_SetMaskInteger(qboolean or, hostcachekey_t field, int param, slist_test_t testop)
{
	if (numvisrules == MAX_VISRULES)
		return;	//just don't add it.

	visrules[numvisrules].fieldindex = field;
	visrules[numvisrules].compareop = testop;
	visrules[numvisrules].operandi = param;
	visrules[numvisrules].operands = "";	//don't crash if they used the wrong arg type.
	visrules[numvisrules].or = or;
	numvisrules++;
}
void Master_SetSortField(hostcachekey_t field, unsigned int sortflags)
{
	sortfield = field;
	sort_decreasing = sortflags & 1;
	sort_favourites = sortflags & 2;
	sort_categories = sortflags & 4;
}
hostcachekey_t Master_GetSortField(void)
{
	return sortfield;
}
qboolean Master_GetSortDescending(void)
{
	return sort_decreasing;
}

void Master_ShowServer(serverinfo_t *server)
{
	int i;
	if (!numvisibleservers)
	{
		Master_InsertAt(server, 0);
		return;
	}

	if (sort_decreasing)
	{
		for (i = 0; i < numvisibleservers; i++)
		{
			if (!Master_ServerIsGreater(server, visibleservers[i]))
			{
				Master_InsertAt(server, i);
				return;
			}
		}

	}
	else
	{
		for (i = 0; i < numvisibleservers; i++)
		{
			if (Master_ServerIsGreater(server, visibleservers[i]))
			{
				Master_InsertAt(server, i);
				return;
			}
		}
	}

	Master_InsertAt(server, numvisibleservers);
}

void Master_ResortServer(serverinfo_t *server)
{
	if (server->status&SRVSTATUS_DISPLAYED)
	{
		if (!Master_PassesMasks(server))
			Master_HideServer(server);
	}
	else
	{
		if (Master_PassesMasks(server))
			Master_ShowServer(server);
	}
}

int Master_SortServers(void)
{
	serverinfo_t *server;

	int total = Master_TotalCount();
	if (maxvisibleservers < total)
	{
		maxvisibleservers = total;
		visibleservers = BZ_Realloc(visibleservers, maxvisibleservers*sizeof(serverinfo_t*));
	}

	{
		numvisibleservers = 0;
		for (server = firstserver; server; server = server->next)
			server->status &= ~SRVSTATUS_DISPLAYED;
	}

	for (server = firstserver; server; server = server->next)
	{
		Master_ResortServer(server);
	}

	return numvisibleservers;
}

serverinfo_t *Master_SortedServer(int idx)
{
	if (idx < 0 || idx >= numvisibleservers)
	{
		if (idx == -1)
			return categorisingserver;
		return NULL;
	}

	return visibleservers[idx];
}

int Master_NumSorted(void)
{
	return numvisibleservers;
}


float Master_ReadKeyFloat(serverinfo_t *server, unsigned int keynum)
{
	if (!server)
		return -1;
	else if (keynum < SLKEY_CUSTOM)
	{
		switch((hostcachekey_t)keynum)
		{
		case SLKEY_PING:
			return server->ping;
		case SLKEY_NUMPLAYERS:
			return server->players;
		case SLKEY_MAXPLAYERS:
			return server->maxplayers;
		case SLKEY_FREEPLAYERS:
			return server->maxplayers - server->players;
		case SLKEY_BASEGAME:
			return Master_BaseGame(server);
		case SLKEY_FLAGS:
			return server->special&~SS_PROTOCOLMASK;
		case SLKEY_TIMELIMIT:
			return server->tl;
		case SLKEY_FRAGLIMIT:
			return server->fl;
		case SLKEY_PROTOCOL:
			return server->protocol;
		case SLKEY_NUMBOTS:
			return server->numbots;
		case SLKEY_NUMHUMANS:
			return server->numhumans;
		case SLKEY_NUMSPECTATORS:
			return server->numspectators;
		case SLKEY_ISFAVORITE:
			return !!(server->special & SS_FAVORITE);
		case SLKEY_ISLOCAL:
			return !!(server->special & SS_LOCAL);
		case SLKEY_ISPROXY:
			return !!(server->special & SS_PROXY);
		case SLKEY_CATEGORY:
			return server->qccategory;

		default:
			return atof(Master_ReadKeyString(server, keynum));
		}
	}
	else if (server->moreinfo)
		return atof(Info_ValueForKey(server->moreinfo->info, slist_keyname[keynum-SLKEY_CUSTOM]));

	return 0;
}

void Master_DecodeColour(vec3_t ret, int col)
{
	if (col < 16)
	{
		col = Sbar_ColorForMap(col);
		VectorSet(ret, host_basepal[col*3+0]/255.0, host_basepal[col*3+1]/255.0, host_basepal[col*3+2]/255.0);
	}
	else
		VectorSet(ret, ((col&0xff0000)>>16)/255.0, ((col&0x00ff00)>>8)/255.0, ((col&0x0000ff)>>0)/255.0);
}

char *Master_ReadKeyString(serverinfo_t *server, unsigned int keynum)
{
	static char adr[MAX_ADR_SIZE];

	if (!server)
		return "";

	if (keynum >= SLKEY_CUSTOM)
	{
		if (server->moreinfo)
		{
			keynum -= SLKEY_CUSTOM;
			if (keynum < sizeof(slist_keyname)/sizeof(slist_keyname[0]))
				return Info_ValueForKey(server->moreinfo->info, slist_keyname[keynum]);
		}
		else if (!(server->special & SS_KEEPINFO))
		{
			server->special |= SS_KEEPINFO;
			server->sends++;
		}
	}
	else if (keynum >= SLKEY_PLAYER0)
	{
		if (server->moreinfo)
		{
			keynum -= SLKEY_PLAYER0;
			if (keynum < server->moreinfo->numplayers)
			{
				vec3_t top, bot;
				Master_DecodeColour(top, server->moreinfo->players[keynum].topc);
				Master_DecodeColour(bot, server->moreinfo->players[keynum].botc);

				return va("%i %i %g %i \"%s\" \"%s\" '%g %g %g' '%g %g %g'", server->moreinfo->players[keynum].userid, server->moreinfo->players[keynum].frags, server->moreinfo->players[keynum].time, server->moreinfo->players[keynum].ping, server->moreinfo->players[keynum].name, server->moreinfo->players[keynum].skin, top[0],top[1],top[2], bot[0], bot[1], bot[2]);
			}
		}
		else if (!(server->special & SS_KEEPINFO))
		{
			server->special |= SS_KEEPINFO;
			server->sends++;
		}
	}
	else
	{
		switch((hostcachekey_t)keynum)
		{
		case SLKEY_MAP:
			return server->map;
		case SLKEY_NAME:
			return server->name;
		case SLKEY_ADDRESS:
			return Master_ServerToString(adr, sizeof(adr), server);
		case SLKEY_GAMEDIR:
			return server->gamedir;

		case SLKEY_MOD:
			return server->modname;
		case SLKEY_QCSTATUS:
			return server->qcstatus;
		case SLKEY_SERVERINFO:
			if (server->moreinfo)
				return server->moreinfo->info;
			return "";

		default:
			{
				static char s[64];
				sprintf(s, "%f", Master_ReadKeyFloat(server, keynum));
				return s;
			}
		}
	}

	return "";
}

hostcachekey_t Master_KeyForName(const char *keyname)
{
	int i;
	if (!strcmp(keyname, "map"))
		return SLKEY_MAP;
	else if (!strcmp(keyname, "ping"))
		return SLKEY_PING;
	else if (!strcmp(keyname, "name") || !strcmp(keyname, "hostname"))
		return SLKEY_NAME;
	else if (!strcmp(keyname, "address") || !strcmp(keyname, "cname"))
		return SLKEY_ADDRESS;
	else if (!strcmp(keyname, "maxplayers"))
		return SLKEY_MAXPLAYERS;
	else if (!strcmp(keyname, "numplayers"))
		return SLKEY_NUMPLAYERS;
	else if (!strcmp(keyname, "freeplayers") || !strcmp(keyname, "freeslots"))
		return SLKEY_FREEPLAYERS;
	else if (!strcmp(keyname, "gamedir") || !strcmp(keyname, "game") || !strcmp(keyname, "*gamedir"))
		return SLKEY_GAMEDIR;
	else if (!strcmp(keyname, "basegame"))
		return SLKEY_BASEGAME;
	else if (!strcmp(keyname, "flags"))
		return SLKEY_FLAGS;
	else if (!strcmp(keyname, "mod"))
		return SLKEY_MOD;
	else if (!strcmp(keyname, "protocol"))
		return SLKEY_PROTOCOL;
	else if (!strcmp(keyname, "numbots"))
		return SLKEY_NUMBOTS;
	else if (!strcmp(keyname, "numhumans"))
		return SLKEY_NUMHUMANS;
	else if (!strcmp(keyname, "numspectators"))
		return SLKEY_NUMSPECTATORS;
	else if (!strcmp(keyname, "qcstatus"))
		return SLKEY_QCSTATUS;
	else if (!strcmp(keyname, "isfavorite"))
		return SLKEY_ISFAVORITE;
	else if (!strcmp(keyname, "islocal"))
		return SLKEY_ISLOCAL;
	else if (!strcmp(keyname, "isproxy"))
		return SLKEY_ISPROXY;
	else if (!strcmp(keyname, "category"))
		return SLKEY_CATEGORY;
	else if (!strcmp(keyname, "serverinfo"))
		return SLKEY_SERVERINFO;
	else if (!strncmp(keyname, "player", 6))
		return SLKEY_PLAYER0 + atoi(keyname+6);

	else if (slist_customkeys == SLIST_MAXKEYS)
		return SLKEY_TOOMANY;
	else
	{
		for (i = 0; i < slist_customkeys; i++)
		{
			if (!strcmp(slist_keyname[i], keyname))
			{
				return i + SLKEY_CUSTOM;
			}
		}
		Q_strncpyz(slist_keyname[slist_customkeys], keyname, MAX_INFO_KEY);

		slist_customkeys++;

		return slist_customkeys-1 + SLKEY_CUSTOM;
	}
}


#ifdef HAVE_PACKET
static void Master_FloodRoute(serverinfo_t *node)
{
	unsigned int i;
	struct peers_s *peer = node->peers;
	for (i = 0; i < node->numpeers; i++, peer++)
	{
		if (peer->ping && peer->ping != PING_DEAD)
		if ((unsigned int)(peer->peer->cost) > (unsigned int)(node->cost + peer->ping))
		{	//we found a shorter route. flood into it.
			peer->peer->prevpeer = node;
			peer->peer->cost = node->cost + peer->ping;
			Master_FloodRoute(peer->peer);
		}
	}
}
serverinfo_t *Master_FindRoute(netadr_t target)
{
	serverinfo_t *info, *targ, *prox;
	extern cvar_t cl_proxyaddr;
	targ = Master_InfoForServer(&target, NULL);
	if (!targ)	//you wot?
		return NULL;

	//never flood into a peer if its just going to be more expensive than a direct connection
	if (*cl_proxyaddr.string && NET_ClassifyAddress(&target, NULL) >= ASCOPE_NET)
	{
		//fixme: we don't handle chained proxies properly, as we assume we can directly hop to the named final proxy.
		//fixme: we'll find the same route, we just won't display the correct expected ping.
		netadr_t pa;
		char *chain = strchr(cl_proxyaddr.string, '@');
		if (chain)
			*chain = 0;

		if (NET_StringToAdr(cl_proxyaddr.string, 0, &pa))
			prox = Master_InfoForServer(&pa, NULL);
		else
			prox = NULL;
		if (chain)
			*chain = '@';
	}
	else
		prox = NULL;

	if (prox)
	{
		for (info = firstserver; info; info = info->next)
		{
			info->cost = info->ping;
			info->prevpeer = prox;
		}
		prox->cost = prox->ping;
		prox->prevpeer = NULL;
		Master_FloodRoute(prox);
	}
	else
	{
		for (info = firstserver; info; info = info->next)
		{
			info->cost = info->ping;
			info->prevpeer = NULL;
		}

		//flood through all proxies
		for (info = firstserver; info; info = info->next)
			Master_FloodRoute(info);
	}

	if (targ->prevpeer)
		return targ;
	return NULL;
}

int Master_FindBestRoute(char *server, char *out, size_t outsize, int *directcost, int *chainedcost)
{
	serverinfo_t *route;
	netadr_t adr;
	int ret = 0;
	char buf[256];
	*out = 0;
	*directcost = 0;
	*chainedcost = 0;
	if (!NET_StringToAdr(server, 0, &adr))
		return -1;

	if (!firstserver)
	{	//routing isn't initialised. you do actually need to refresh the serverbrowser for this junk
		Q_strncpyz(out, server, outsize);
		return -1;
	}

	route = Master_FindRoute(adr);

	if (!route)
	{	//routing didn't find anything, just go directly.
		Q_strncpyz(out, server, outsize);
		return 0;
	}

	*directcost = route->ping;
	*chainedcost = route->cost;

	Q_strncatz(out, Master_ServerToString(buf, sizeof(buf), route), outsize);
	for (ret = 0, route = route->prevpeer; route; route = route->prevpeer, ret++)
		Q_strncatz(out, va("@%s", Master_ServerToString(buf, sizeof(buf), route)), outsize);

	return ret;
}







//main thread
void CLMaster_AddMaster_Worker_Resolved(void *ctx, void *data, size_t a, size_t b)
{
	master_t *mast = data;
	master_t *oldmast;

	if (mast->adr.type == NA_INVALID)
	{
		if (b)
			Con_Printf("Failed to resolve master address \"%s\"\n", mast->address);
		//else master not enabled anyway. the lookup was skipped.
	}
	else if (mast->adr.type != NA_IP && mast->adr.type != NA_IPV6 && mast->adr.type != NA_IPX)
	{
		Con_Printf("Fixme: unable to poll address family for \"%s\"\n", mast->address);
	}
	else
	{
		if (mast->mastertype == MT_BCAST)	//broadcasts
		{
			if (mast->adr.type == NA_IP)
				memset(mast->adr.address.ip+4, 0xff, sizeof(mast->adr.address.ip));
			if (mast->adr.type == NA_IPX)
			{
				memset(mast->adr.address.ipx+0, 0, 4);
				memset(mast->adr.address.ipx+4, 0xff, 6);
			}
			if (mast->adr.type == NA_IPV6)
			{
				memset(mast->adr.address.ip6, 0, sizeof(mast->adr.address.ip6));
				mast->adr.address.ip6[0]	= 0xff;
				mast->adr.address.ip6[1]	= 0x02;
				mast->adr.address.ip6[15]	= 0x01;
			}
		}

		//fix up default ports if not specified
		if (!mast->adr.port)
		{
			switch (mast->protocoltype)
			{
			case MP_DPMASTER:	mast->adr.port = BigShort (27950);	break;
#ifdef Q2CLIENT
			case MP_QUAKE2:		mast->adr.port = BigShort (27900);	break;	//FIXME: verify
#endif
#ifdef Q3CLIENT
			case MP_QUAKE3:		mast->adr.port = BigShort (27950);	break;
#endif
			case MP_QUAKEWORLD:	mast->adr.port = BigShort (PORT_QWMASTER);	break;
			}
		}

		for (oldmast = master; oldmast; oldmast = oldmast->next)
		{
			if (NET_CompareAdr(&oldmast->adr, &mast->adr) && oldmast->mastertype == mast->mastertype && oldmast->protocoltype == mast->protocoltype)	//already exists.
				break;
		}
		if (!oldmast)
		{
			mast->next = master;
			master = mast;
			return;
		}
		else if (oldmast->nosave && !mast->nosave)
			oldmast->nosave = false;
	}
	Z_Free(mast);
}
//worker thread
void CLMaster_AddMaster_Worker_Resolve(void *ctx, void *data, size_t a, size_t b)
{
	netadr_t adrs[MAX_MASTER_ADDRESSES];
	char token[1024];
	int found = 0, j, i;
//	qboolean first = true;
	char *str;
	master_t *work = data;

	if (!b)
		;
	else
	{
		//resolve all the addresses
		str = work->address;
		while (str && *str)
		{
			str = COM_ParseOut(str, token, sizeof(token));
			if (*token)
				found += NET_StringToAdr2(token, 0, &adrs[found], MAX_MASTER_ADDRESSES-found, NULL);
			//we don't do this logic because windows doesn't look up ipv6 names if it only has teredo
			//this means an ipv4+teredo client cannot see ivp6-only servers. and that sucks.
//			if (first && found)
//				break;	//if we found one by name, don't try any fallback ip addresses.
//			first = false;
		}
	}

	//add the main ip address
	if (found)
		work->adr = adrs[0];
	else
		memset(&work->adr, 0, sizeof(work->adr));

	//add dupes too (eg: ipv4+ipv6)
	for (i = 1; i < found; i++)
	{
//		master_t *alt;
		if (adrs[i].type == NA_INVALID)
			continue;

		//don't add the same ip twice, because that's silly
		for (j = 0; j < i; j++)
		{
			if (NET_CompareAdr(&adrs[j], &adrs[i]))
				break;
		}
		if (j == i)
		{	//not already added, hurrah
			master_t *alt = Z_Malloc(sizeof(master_t)+strlen(work->name)+1+strlen(work->address)+1);
			alt->address = alt->name + strlen(work->name)+1;
			alt->mastertype = work->mastertype;
			alt->protocoltype = work->protocoltype;
			strcpy(alt->name, work->name);
			strcpy(alt->address, work->address);
			alt->sends = 1;
			alt->nosave = true;
			alt->adr = adrs[i];

			COM_AddWork(WG_MAIN, CLMaster_AddMaster_Worker_Resolved, NULL, work, a, b);
			work = alt;
		}
	}

	COM_AddWork(WG_MAIN, CLMaster_AddMaster_Worker_Resolved, NULL, work, a, b);
}

void Master_AddMaster (char *address, enum mastertype_e mastertype, enum masterprotocol_e protocol, char *description)
{
	master_t *mast;

	if (!address || !*address)
		return;

	if (!description)
		description = address;

	mast = Z_Malloc(sizeof(master_t)+strlen(description)+1+strlen(address)+1);
	mast->address = mast->name + strlen(description)+1;
	mast->mastertype = mastertype;
	mast->protocoltype = protocol;
	strcpy(mast->name, description);
	strcpy(mast->address, address);
	mast->sends = 1;

	COM_AddWork(WG_LOADER, CLMaster_AddMaster_Worker_Resolve, NULL, mast, 0, true/*Master_MasterProtocolIsEnabled(protocol)*/);
}
#else
void Master_AddMaster (char *address, enum mastertype_e mastertype, enum masterprotocol_e protocol, char *description)
{	//pretend it didn't resolve.
}
#endif

static void MasterInfo_RemoveAllPlayers(void)
{
	player_t *p;
	while(mplayers)
	{
		p = mplayers;
		mplayers = p->next;
		Z_Free(p);
	}
}
void MasterInfo_Shutdown(void)
{
	master_t *mast;
	serverinfo_t *sv;
	MasterInfo_RemoveAllPlayers();
	while(firstserver)
	{
		sv = firstserver;
		firstserver = sv->next;
		if (sv->peers)
			Z_Free(sv->peers);
		Z_Free(sv);
	}
	while(master)
	{
		mast = master;
		master = mast->next;
#ifdef WEBCLIENT
		if (mast->dl)
			DL_Close(mast->dl);
#endif
		Z_Free(mast);
	}

	maxvisibleservers = 0;
	numvisibleservers = 0;
	Z_Free(visibleservers);
}

void Master_AddMasterHTTP (char *address, int mastertype, int protocoltype, char *description)
{
	master_t *mast;
/*	int servertype;

	if (protocoltype == MP_DP)
		servertype = SS_DARKPLACES;
	else if (protocoltype == MP_Q2)
		servertype = SS_QUAKE2;
	else if (protocoltype == MP_Q3)
		servertype = SS_QUAKE3;
	else if (protocoltype == MP_NQ)
		servertype = SS_NETQUAKE;
	else
		servertype = 0;
*/
	for (mast = master; mast; mast = mast->next)
	{
		if (!strcmp(mast->address, address) && mast->mastertype == mastertype && mast->protocoltype == protocoltype)	//already exists.
			return;
	}
	mast = Z_Malloc(sizeof(master_t)+strlen(description)+1+strlen(address)+1);
	mast->address = mast->name + strlen(description)+1;
	mast->mastertype = mastertype;
	mast->protocoltype = protocoltype;
//	mast->servertype = servertype;
	strcpy(mast->name, description);
	strcpy(mast->address, address);

	mast->next = master;
	master = mast;
}

//build a linked list of masters.	Doesn't duplicate addresses.
qboolean Master_LoadMasterList (char *filename, qboolean withcomment, int defaulttype, int defaultprotocol, int depth)
{
	vfsfile_t *f;
	char line[1024];
	char name[1024];
	char entry[1024];
	char *next, *sep;
	int servertype;
	int protocoltype;
	qboolean favourite;

	if (depth <= 0)
		return false;
	depth--;

	f = FS_OpenVFS(filename, "rb", FS_ROOT);
	if (!f)
		return false;

	while(VFS_GETS(f, line, sizeof(line)-1))
	{
		if (*line == '#')	//comment
			continue;

		*name = 0;
		favourite = false;
		servertype = defaulttype;
		protocoltype = defaultprotocol;

		next = COM_ParseOut(line, entry, sizeof(entry));
		if (!*com_token)
			continue;

		//special cases. Add a port if you have a host named 'file'... (unlikly)
		if (!strcmp(entry, "file"))
		{
			if (withcomment)
				next = COM_ParseOut(next, name, sizeof(name));
			next = COM_ParseOut(next, entry, sizeof(entry));
			if (!next)
				continue;
			servertype = MT_BAD;
		}
		else if (!strcmp(entry, "master"))
		{
			if (withcomment)
				next = COM_ParseOut(next, name, sizeof(name));
			next = COM_ParseOut(next, entry, sizeof(entry));
			if (!next)
				continue;
			servertype = MT_MASTERUDP;
		}
		else if (!strcmp(entry, "url"))
		{
			if (withcomment)
				next = COM_ParseOut(next, name, sizeof(name));
			next = COM_ParseOut(next, entry, sizeof(entry));
			servertype = MT_MASTERHTTP;
		}

		next = COM_Parse(next);

		for(sep = com_token; sep; sep = next)
		{
			next = strchr(sep, ':');
			if (next)
				*next = 0;

			if (!strcmp(sep, "single"))
				servertype = MT_SINGLE;
			else if (!strcmp(sep, "master"))
				servertype = MT_MASTERUDP;
			else if (!strcmp(sep, "masterhttp"))
				servertype = MT_MASTERHTTP;
			else if (!strcmp(sep, "bcast"))
				servertype = MT_BCAST;

#ifndef QUAKETC
			else if (!strcmp(com_token, "qw"))
				protocoltype = MP_QUAKEWORLD;
#endif
#ifdef Q2CLIENT
			else if (!strcmp(com_token, "q2"))
				protocoltype = MP_QUAKE2;
#endif
#ifdef Q3CLIENT
			else if (!strcmp(com_token, "q3"))
				protocoltype = MP_QUAKE3;
#endif
#ifdef NQPROT
			else if (!strcmp(com_token, "nq"))
				protocoltype = MP_NETQUAKE;
#endif
			else if (!strcmp(com_token, "dp"))
				protocoltype = MP_DPMASTER;

			//legacy compat
#ifdef NQPROT
			else if (!strcmp(com_token, "httpnq"))
			{
				servertype = MT_MASTERHTTP;
				protocoltype = MP_NETQUAKE;
			}
#endif
#ifndef QUAKETC
			else if (!strcmp(com_token, "httpqw"))
			{
				servertype = MT_MASTERHTTP;
				protocoltype = MP_QUAKEWORLD;
			}
#endif

			else if (!strcmp(com_token, "favourite") || !strcmp(com_token, "favorite"))
				favourite = true;
		}

		if (!*name && next)
		{
			sep = name;
			while(*next == ' ' || *next == '\t')
				next++;
			while (*next && sep < name+sizeof(name)-1)
				*sep++ = *next++;
			*sep = 0;
		}

		if (servertype == MT_BAD)
			Master_LoadMasterList(entry, false, servertype, protocoltype, depth);
		else
		{
#if POLLTOTALSOCKETS>0
			//favourites are added explicitly, with their name and stuff
			if (favourite && servertype == MT_SINGLE)
			{
				if (NET_StringToAdr(entry, 0, &net_from))
					CL_ReadServerInfo(va("\\hostname\\%s", name), -servertype, true);
				else
					Con_Printf("Failed to resolve address - \"%s\"\n", entry);
			}
#endif

			switch (servertype)
			{
			case MT_MASTERHTTP:
				Master_AddMasterHTTP(entry, servertype, protocoltype, name);
				break;
			default:
				Master_AddMaster(entry, servertype, protocoltype, name);
				break;
			}
		}
	}
	VFS_CLOSE(f);

	return true;
}

#if POLLTOTALSOCKETS>0
qboolean NET_SendPollPacket(int len, void *data, netadr_t to)
{
	unsigned long bcast;
	int ret;
	struct sockaddr_qstorage	addr;
	char buf[128];

	NetadrToSockadr (&to, &addr);
#ifdef HAVE_IPX
	if (((struct sockaddr*)&addr)->sa_family == AF_IPX)
	{
		lastpollsockIPX++;
		if (lastpollsockIPX>=POLLIPXSOCKETS)
			lastpollsockIPX=0;
		if (pollsocketsList[FIRSTIPXSOCKET+lastpollsockIPX]==INVALID_SOCKET)
		{
			pollsocketsList[FIRSTIPXSOCKET+lastpollsockIPX] = IPX_OpenSocket(PORT_ANY);
			pollsocketsBCast[FIRSTIPXSOCKET+lastpollsockIPX] = false;
		}
		if (pollsocketsList[FIRSTIPXSOCKET+lastpollsockIPX]==INVALID_SOCKET)
			return true;	//bother

		bcast = !memcmp(to.address.ipx, "\0\0\0\0\xff\xff\xff\xff\xff\xff", sizeof(to.address.ipx));
		if (pollsocketsBCast[FIRSTIPXSOCKET+lastpollsockIPX] != bcast)
		{
			if (setsockopt(pollsocketsList[FIRSTIPXSOCKET+lastpollsockIPX], SOL_SOCKET, SO_BROADCAST, (char *)&bcast, sizeof(bcast)) == -1)
				return true;
			pollsocketsBCast[FIRSTIPXSOCKET+lastpollsockIPX] = bcast;
		}
		ret = sendto (pollsocketsList[FIRSTIPXSOCKET+lastpollsockIPX], data, len, 0, (struct sockaddr *)&addr, sizeof(addr) );
	}
	else
#endif
#ifdef HAVE_IPV6
	if (((struct sockaddr*)&addr)->sa_family == AF_INET6)
	{
		lastpollsockUDP6++;
		if (lastpollsockUDP6>=POLLUDP6SOCKETS)
			lastpollsockUDP6=0;
		if (pollsocketsList[FIRSTUDP6SOCKET+lastpollsockUDP6]==INVALID_SOCKET)
		{
			pollsocketsList[FIRSTUDP6SOCKET+lastpollsockUDP6] = UDP6_OpenSocket(PORT_ANY);
			pollsocketsBCast[FIRSTUDP6SOCKET+lastpollsockUDP6] = false;
		}
		if (pollsocketsList[FIRSTUDP6SOCKET+lastpollsockUDP6]==INVALID_SOCKET)
			return true;	//bother
		ret = sendto (pollsocketsList[FIRSTUDP6SOCKET+lastpollsockUDP6], data, len, 0, (struct sockaddr *)&addr, sizeof(addr) );
	}
	else
#endif
#ifdef HAVE_IPV4
		if (((struct sockaddr*)&addr)->sa_family == AF_INET)
	{
		lastpollsockUDP4++;
		if (lastpollsockUDP4>=POLLUDP4SOCKETS)
			lastpollsockUDP4=0;
		if (pollsocketsList[FIRSTUDP4SOCKET+lastpollsockUDP4]==INVALID_SOCKET)
		{
			pollsocketsList[FIRSTUDP4SOCKET+lastpollsockUDP4] = UDP_OpenSocket(PORT_ANY);
			pollsocketsBCast[FIRSTUDP4SOCKET+lastpollsockUDP4] = false;
		}
		if (pollsocketsList[FIRSTUDP4SOCKET+lastpollsockUDP4]==INVALID_SOCKET)
			return true;	//bother

		bcast = !memcmp(to.address.ip, "\xff\xff\xff\xff", sizeof(to.address.ip));
		if (pollsocketsBCast[FIRSTUDP4SOCKET+lastpollsockUDP4] != bcast)
		{
			if (setsockopt(pollsocketsList[FIRSTUDP4SOCKET+lastpollsockUDP4], SOL_SOCKET, SO_BROADCAST, (char *)&bcast, sizeof(bcast)) == -1)
				return true;
			pollsocketsBCast[FIRSTUDP4SOCKET+lastpollsockUDP4] = bcast;
		}
		ret = sendto (pollsocketsList[FIRSTUDP4SOCKET+lastpollsockUDP4], data, len, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) );
	}
	else
#endif
		return true;

	if (ret == -1)
	{
		int er = neterrno();
// wouldblock is silent
		if (er == NET_EWOULDBLOCK)
			return false;

		if (er == NET_ECONNREFUSED)
			return true;

		if (er == NET_ENETUNREACH)
			Con_DPrintf("NET_SendPollPacket Warning: unreachable: %s\n", NET_AdrToString(buf, sizeof(buf), &to));
		else
#ifdef _WIN32
		if (er == NET_EADDRNOTAVAIL)
			Con_DPrintf("NET_SendPollPacket Warning: %i\n", er);
		else
			Con_Printf ("NET_SendPollPacket ERROR: %i\n", er);
#else
		if (er == NET_EADDRNOTAVAIL)
			Con_DPrintf("NET_SendPollPacket Warning: %s\n", strerror(er));
		else
			Con_Printf ("NET_SendPollPacket ERROR: %s\n", strerror(er));
#endif
	}
	return true;
}

void Master_CheckPollSockets(void)
{
	int sock;
	SOCKET usesocket;
	char adr[MAX_ADR_SIZE];

	for (sock = 0; sock < POLLTOTALSOCKETS; sock++)
	{
		int 	ret;
		struct sockaddr_qstorage	from;
		int		fromlen;

		usesocket = pollsocketsList[sock];

		if (usesocket == INVALID_SOCKET)
			continue;
		fromlen = sizeof(from);
		ret = recvfrom (usesocket, (char *)net_message_buffer, sizeof(net_message_buffer), 0, (struct sockaddr *)&from, &fromlen);

		if (ret == -1)
		{
			int e = neterrno();
			if (e == NET_EWOULDBLOCK)
				continue;
			if (e == NET_EMSGSIZE)
			{
				SockadrToNetadr (&from, fromlen, &net_from);
				Con_Printf ("Warning:  Oversize packet from %s\n",
					NET_AdrToString (adr, sizeof(adr), &net_from));
				continue;
			}
			if (e == NET_ECONNABORTED || e == NET_ECONNRESET)
			{
//				Con_Printf ("Connection lost or aborted\n");
				continue;
			}


			Con_Printf ("NET_CheckPollSockets: %i, %s\n", e, strerror(e));
			continue;
		}
		SockadrToNetadr (&from, fromlen, &net_from);

		net_message.cursize = ret;
		if (ret >= sizeof(net_message_buffer) )
		{
			Con_Printf ("Oversize packet from %s\n", NET_AdrToString (adr, sizeof(adr), &net_from));
			continue;
		}

		if (*(int *)net_message.data == -1)
		{
			int c;
			char *s;

			MSG_BeginReading (&net_message, msg_nullnetprim);
			MSG_ReadLong ();        // skip the -1

			c = net_message.currentbit;
			s = MSG_ReadStringLine();	//peek for q2 messages.
#ifdef Q2CLIENT
			if (!strcmp(s, "print"))
			{
				CL_ReadServerInfo(MSG_ReadString(), MP_QUAKE2, false);
				continue;
			}
			if (!strcmp(s, "info"))	//parse a bit more...
			{
				CL_ReadServerInfo(MSG_ReadString(), MP_QUAKE2, false);
				continue;
			}
#ifdef HAVE_IPV6
			if (!strncmp(s, "server6", 7))	//parse a bit more...
			{
				net_message.currentbit = c+(7<<3);
				CL_MasterListParse(NA_IPV6, SS_QUAKE2, false);
				continue;
			}
#endif
			if (!strncmp(s, "servers", 7))	//parse a bit more...
			{
				net_message.currentbit = c+(7<<3);
				CL_MasterListParse(NA_IP, SS_QUAKE2, false);
				continue;
			}
#endif
			//q3/dpm server responses
			if (!strcmp(s, "statusResponse"))
			{	//originally q3, but oh well.
				CL_ReadServerInfo(MSG_ReadString(), MP_DPMASTER, false);
				continue;
			}
			if (!strcmp(s, "infoResponse"))	//parse a bit more...
			{	//originally q3, but oh well.
				CL_ReadServerInfo(MSG_ReadString(), MP_DPMASTER, false);
				continue;
			}
			//q3/dpm master responses
#ifdef HAVE_IPV6
			if (!strncmp(s, "getserversResponse6", 19) && (s[19] == '\\' || s[19] == '/'))	//parse a bit more...
			{
				net_message.currentbit = (c+19-1)<<3;
				CL_MasterListParse(NA_IPV6, SS_GETINFO, true);
				continue;
			}
#endif
			if (!strncmp(s, "getserversExtResponse", 21) && (s[21] == '\\' || s[21] == '/'))	//parse a bit more...
			{
				net_message.currentbit = (c+21-1)<<3;
				CL_MasterListParse(NA_IP, SS_GETINFO, true);
				continue;
			}
			if (!strncmp(s, "getserversResponse", 18) && (s[18] == '\\' || s[18] == '/'))	//parse a bit more...
			{
				net_message.currentbit = (c+18-1)<<3;
				CL_MasterListParse(NA_IP, SS_GETINFO, true);
				continue;
			}

#ifdef HAVE_IPV6
			if (!strncmp(s, "qw_slist6\\", 10))	//parse a bit more...
			{
				net_message.currentbit = (c+9-1)<<3;
				CL_MasterListParse(NA_IPV6, SS_QUAKEWORLD, false);
				continue;
			}
#endif
			if (!strncmp(s, "pinglist", 8))	//parse a bit more...
			{
				net_message.currentbit = (c+8-1)<<3;
				CL_ReadPingList();
				continue;
			}

			net_message.currentbit = c;

			c = MSG_ReadByte ();

			if (c == A2C_PRINT)	//qw server reply.
			{
				CL_ReadServerInfo(MSG_ReadString(), MP_QUAKEWORLD, false);
				continue;
			}

			if (c == M2C_MASTER_REPLY)	//qw master reply.
			{
				CL_MasterListParse(NA_IP, SS_QUAKEWORLD, false);
				continue;
			}
		}
#ifdef NQPROT
		else
		{	//connected packet? Must be a NQ packet.
			char name[32];
			char map[16];
			int users, maxusers;

			int control;
			int ccrep;

			MSG_BeginReading (&net_message, msg_nullnetprim);
			control = BigLong(*((int *)net_message.data));
			MSG_ReadLong();
			if (control == -1)
				continue;
			if ((control & (~NETFLAG_LENGTH_MASK)) !=  NETFLAG_CTL)
				continue;
			if ((control & NETFLAG_LENGTH_MASK) != ret)
				continue;

			ccrep = MSG_ReadByte();

			if (ccrep == CCREP_PLAYER_INFO)
			{
				serverinfo_t *selserver = selectedserver.inuse?Master_InfoForServer(&selectedserver.adr, selectedserver.brokerid):NULL;
				serverinfo_t *info = Master_InfoForServer(&net_from, NULL);
				info = Master_InfoForServer(&net_from, NULL);
				if (selserver == info)
				{
					char playeraddrbuf[256];
					int playernum = MSG_ReadByte();
					char *playername = MSG_ReadString();
					int playercolor = MSG_ReadLong();
					int playerfrags = MSG_ReadLong();
					int secsonserver = MSG_ReadLong();
					char *playeraddr = MSG_ReadStringBuffer(playeraddrbuf, sizeof(playeraddrbuf));
					if (msg_badread)
						continue;

					//might as well
					IPLog_Add(playeraddr, playername);

					selectedserver.lastplayer = playernum+1;

					memset(&info->moreinfo->players[playernum], 0, sizeof(info->moreinfo->players[playernum]));
					info->moreinfo->players[playernum].userid = 0;
					info->moreinfo->players[playernum].frags = playerfrags;
					info->moreinfo->players[playernum].time = secsonserver;
					info->moreinfo->players[playernum].ping = 0;	//*sigh*
					Q_strncpyz(info->moreinfo->players[playernum].name, playername, sizeof(info->moreinfo->players[playernum].name));
					Q_strncpyz(info->moreinfo->players[playernum].skin, "", sizeof(info->moreinfo->players[playernum].skin));
					Q_strncpyz(info->moreinfo->players[playernum].team, "", sizeof(info->moreinfo->players[playernum].team));
					info->moreinfo->players[playernum].topc = playercolor>>4;
					info->moreinfo->players[playernum].botc = playercolor&15;
					info->moreinfo->players[playernum].isspec = false;
					info->moreinfo->numplayers = max(info->moreinfo->numplayers, playernum+1);

					//... and now try to query the next one... because everyone gives up after the first, right?... dude... I hate this shit.
					SZ_Clear(&net_message);
					MSG_WriteLong(&net_message, 0);// save space for the header, filled in later
					MSG_WriteByte(&net_message, CCREQ_PLAYER_INFO);
					MSG_WriteByte(&net_message, selectedserver.lastplayer);
					*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
					NET_SendPollPacket(net_message.cursize, net_message.data, info->adr);
					SZ_Clear(&net_message);
				}
			}
			else if (ccrep == CCREP_RULE_INFO)
			{
				serverinfo_t *selserver = selectedserver.inuse?Master_InfoForServer(&selectedserver.adr, selectedserver.brokerid):NULL;
				serverinfo_t *info = Master_InfoForServer(&net_from, NULL);
				char *s, *old;
				info = Master_InfoForServer(&net_from, NULL);
				if (selserver == info)
				{
					s = MSG_ReadString();
					if (msg_badread)
						continue;
					Q_strncpyz(selectedserver.lastrule, s, sizeof(selectedserver.lastrule));
					s = MSG_ReadString();
					
					old = Info_ValueForKey(info->moreinfo->info, selectedserver.lastrule);
					if (strcmp(s, old) && !strchr(s, '\"') && !strchr(s, '\\'))
						Info_SetValueForStarKey(info->moreinfo->info, selectedserver.lastrule, s, sizeof(info->moreinfo->info));

					//... and now try to query the next one... because everyone gives up after the first, right?... dude... I hate this shit.
					SZ_Clear(&net_message);
					MSG_WriteLong(&net_message, 0);// save space for the header, filled in later
					MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
					MSG_WriteString(&net_message, selectedserver.lastrule);
					*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
					NET_SendPollPacket(net_message.cursize, net_message.data, info->adr);
					SZ_Clear(&net_message);
				}
			}
			else if (ccrep == CCREP_SERVER_INFO)
			{
				/*this is an address string sent from the server. its not usable. if its replying to serverinfos, its possible to send it connect requests, while the address that it claims is 50% bugged*/
				MSG_ReadString();

				Q_strncpyz(name, MSG_ReadString(), sizeof(name));
				Q_strncpyz(map, MSG_ReadString(), sizeof(map));
				users = MSG_ReadByte();
				maxusers = MSG_ReadByte();
				if (MSG_ReadByte() != NQ_NETCHAN_VERSION)
				{
//					Q_strcpy(name, "*");
//					Q_strcat(name, name);
				}

				CL_ReadServerInfo(va("\\hostname\\%s\\map\\%s\\maxclients\\%i\\clients\\%i", name, map, maxusers, users), MP_NETQUAKE, false);
			}
		}
#endif
		continue;
	}
}
#endif

void Master_RemoveKeepInfo(serverinfo_t *sv)
{
	sv->special &= ~SS_KEEPINFO;
	if (sv->moreinfo)
	{
		Z_Free(sv->moreinfo);
		sv->moreinfo = NULL;
	}
}

void SListOptionChanged(serverinfo_t *newserver)
{
	if (selectedserver.inuse)
	{
		serverinfo_t *oldserver;

		selectedserver.detail = NULL;

		if (!slist_cacheinfo.value)	//we have to flush it. That's the rules.
		{
			for (oldserver = firstserver; oldserver; oldserver=oldserver->next)
			{
				if (NET_CompareAdr(&selectedserver.adr, &oldserver->adr) && !strcmp(selectedserver.brokerid, oldserver->brokerid))
				{
					if (oldserver->moreinfo && oldserver->ping!=PING_UNKNOWN)
					{
						Z_Free(oldserver->moreinfo);
						oldserver->moreinfo = NULL;
					}
					break;
				}
			}
		}

		if (!newserver)
			return;

		selectedserver.adr = newserver->adr;
		strcpy(selectedserver.brokerid, newserver->brokerid);

		if (newserver->moreinfo)	//we cached it.
		{
			selectedserver.detail = newserver->moreinfo;
			return;
		}
//we don't know all the info, so send a request for it.
		selectedserver.detail = newserver->moreinfo = Z_Malloc(sizeof(serverdetailedinfo_t));

		newserver->moreinfo->numplayers = newserver->players;
		strcpy(newserver->moreinfo->info, "");
		Info_SetValueForKey(newserver->moreinfo->info, "hostname", newserver->name, sizeof(newserver->moreinfo->info));


		selectedserver.refreshtime = realtime+4;
#if POLLTOTALSOCKETS>0
		newserver->sends++;
		Master_QueryServer(newserver);

#if defined(NQPROT)
		selectedserver.lastplayer = 0;
		*selectedserver.lastrule = 0;
		if ((newserver->special&(SS_PROTOCOLMASK|SS_GETINFO)) == SS_NETQUAKE)
		{	//start spamming the server to get all of its details. silly protocols.
			SZ_Clear(&net_message);
			net_message.packing = SZ_RAWBYTES;
			net_message.currentbit = 0;
			MSG_WriteLong(&net_message, 0);// save space for the header, filled in later
			MSG_WriteByte(&net_message, CCREQ_PLAYER_INFO);
			MSG_WriteByte(&net_message, selectedserver.lastplayer);
			*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
			NET_SendPollPacket(net_message.cursize, net_message.data, newserver->adr);
			SZ_Clear(&net_message);
			MSG_WriteLong(&net_message, 0);// save space for the header, filled in later
			MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
			MSG_WriteString(&net_message, selectedserver.lastrule);
			*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
			NET_SendPollPacket(net_message.cursize, net_message.data, newserver->adr);
			SZ_Clear(&net_message);
		}
#endif
#endif
	}
}

static qboolean MasterInfo_ReadProtocol(serverinfo_t *info, const char *infostring)
{
	char *token = Info_ValueForKey(infostring, "protocol");
	if (*token)
	{
		//read the protocol number
		info->protocol = strtoul(token, &token, 0);

		//and try to figure out which filter it should be under.
		info->special &= ~SS_PROTOCOLMASK;
		if (*token)
		{
			while (*token)
			{
				if (*token == 'w')
					info->special |= SS_QUAKEWORLD;
				else if (*token == 'n' || *token == 'd')
					info->special |= SS_NETQUAKE;
				else if (*token == 'x')
					info->special |= SS_QEPROT;

				else if (*token == 't')
					info->special |= SS_PROXY;	//qtv
				else if (*token == 'r')
				{
#if POLLTOTALSOCKETS>0
					char *msg = "\xff\xff\xff\xffpingstatus ext";
					if (info->peers)
					{	//forget the old, to let them timeout.
						Z_Free(info->peers);
						info->peers = NULL;
						info->numpeers = 0;
					}
					NET_SendPollPacket(strlen(msg), msg, info->adr);
#endif
					info->special |= SS_PROXY|SS_RELAY;	//qwfwd relay, ask it for its pinglist
				}
				else
					continue;
				break;
			}
		}
		else switch(info->protocol)
		{
		case PROTOCOL_VERSION_QW:	info->special |= SS_QUAKEWORLD;	break;
#ifdef NQPROT
		case PROTOCOL_VERSION_NQ:	info->special |= SS_NETQUAKE;	break;
		case PROTOCOL_VERSION_H2:	info->special |= SS_NETQUAKE;	break;	//erk
		case PROTOCOL_VERSION_NEHD:	info->special |= SS_NETQUAKE;	break;
		case PROTOCOL_VERSION_FITZ:	info->special |= SS_NETQUAKE;	break;
		case PROTOCOL_VERSION_RMQ:	info->special |= SS_NETQUAKE;	break;
		case PROTOCOL_VERSION_DP5:	info->special |= SS_NETQUAKE;	break;	//dp actually says 3... but hey, that's dp being WEIRD.
		case PROTOCOL_VERSION_DP6:	info->special |= SS_NETQUAKE;	break;
		case PROTOCOL_VERSION_DP7:	info->special |= SS_NETQUAKE;	break;
		case NQ_NETCHAN_VERSION_QEX:info->special |= SS_QEPROT;		break;
		case NQ_NETCHAN_VERSION:
#endif
		default:
			if ((info->special&SS_PROTOCOLMASK) == SS_UNKNOWN)
			{	//guesses...
				if (PROTOCOL_VERSION_Q2 >= info->protocol && info->protocol >= PROTOCOL_VERSION_Q2_MIN)
					info->special |= SS_QUAKE2;	//q2 has a range!
				else if (info->protocol > 60)
					info->special |= SS_QUAKE3;
				else if (!strcmp(Info_ValueForKey(infostring, "gamename"), "DarkPlaces-Quake") || *Info_ValueForKey(infostring, "nqprotocol"))
					info->special |= SS_NETQUAKE;
				else
					info->special |= SS_QUAKEWORLD;
			}
			break;
		}
		return true;
	}
	info->protocol = 0;
	return false;
}

#ifdef WEBCLIENT
static void MasterInfo_ProcessHTTPInfo(serverinfo_t *srv, const char *info)
{
	char adrbuf[MAX_ADR_SIZE];
	if (info && (!(srv->status & SRVSTATUS_ALIVE) || srv->ping == PING_UNKNOWN))
	{
		if (srv->adr.prot != NP_DGRAM)
		{
			srv->sends = 0;	//no point pinging it, it won't work.
			srv->ping = PING_UNKNOWN;
			srv->status |= SRVSTATUS_ALIVE;	//or at least wouldn't have been reported this time around.
		}

		Q_strncpyz(srv->name, Info_ValueForKey(info, "hostname"), sizeof(srv->name));
		Q_strncpyz(srv->gamedir, Info_ValueForKey(info, "modname"), sizeof(srv->gamedir));
		Q_strncpyz(srv->map, Info_ValueForKey(info, "mapname"), sizeof(srv->map));
		srv->players = atoi(Info_ValueForKey(info, "clients"));
		srv->maxplayers = atoi(Info_ValueForKey(info, "maxclients"));

		if (!MasterInfo_ReadProtocol(srv, info))
			srv->special = (srv->special&~SS_PROTOCOLMASK)|SS_UNKNOWN;	//assume its an older fteqw server.

		srv->numbots = 0;
		srv->numhumans = srv->players - srv->numbots;
		srv->freeslots = srv->maxplayers - srv->players;


		if (!srv->moreinfo)// && ((slist_cacheinfo.value == 2 || (NET_CompareAdr(&srv->adr, &selectedserver.adr)&&!strcmp(srv->brokerid,selectedserver.brokerid))) || (srv->special & SS_KEEPINFO)))
			srv->moreinfo = Z_Malloc(sizeof(serverdetailedinfo_t));
		if (srv->moreinfo)
			Q_strncpyz(srv->moreinfo->info, info, sizeof(srv->moreinfo->info));
	}
	if (!*srv->name)
		Q_snprintfz(srv->name, sizeof(srv->name), "%s h", Master_ServerToString(adrbuf, sizeof(adrbuf), srv));
}
static void MasterInfo_ProcessHTTP(struct dl_download *dl)
{
	master_t *mast = dl->user_ctx;
	vfsfile_t *file = dl->file;
	int protocoltype;
	netadr_t adr;
	char *s;
	char *el;
	serverinfo_t *info;
	char linebuffer[2048];
	const char *brokerid;
	char *infostring;
	netadr_t brokeradr;

	if (mast)
	{
		brokeradr = mast->adr;
		mast->dl = NULL;
		protocoltype = mast->protocoltype;
	}
	else
	{
#ifdef Q3CLIENT
		NET_StringToAdr("/", PORT_ICEBROKER, &brokeradr);
		protocoltype = MP_QUAKE3;
#else
		return;
#endif
	}

	if (!file)
		return;

	brokeradr.type = NA_INVALID;	//should be the default broker...
	brokeradr.prot = NP_RTC_TLS;
	for (info = firstserver; info; info = info->next)
	{
		if (NET_CompareAdr(&info->adr, &brokeradr))
		{
			info->ping = PING_DEAD;
			info->status &= ~SRVSTATUS_ALIVE;
		}
	}

	while(VFS_GETS(file, linebuffer, sizeof(linebuffer)))
	{
		s = linebuffer;
		while (*s == '\t' || *s == ' ')
			s++;

		el = s + strlen(s);
		if (el>s && el[-1] == '\r')
			el[-1] = '\0';

		if (*s == '#')	//hash is a comment, apparently.
			continue;

		for (infostring = s; *infostring && *infostring != ' ' && *infostring != '\t'; )
			infostring++;
		if (*infostring == ' ' || *infostring == '\t')
		{
			*infostring++ = 0;	//null terminate the address
			while(*infostring == ' ' || *infostring == '\t')
				infostring++;	//skip over any whitespace...
			if (*infostring != '\\')
				infostring = NULL;	//err... no. not an info string. probably a comment.
		}
		else
			infostring = NULL;

		if (!strncmp(s, "ice:///", 7) || !strncmp(s, "ices:///", 8) || !strncmp(s, "rtc:///", 7) || !strncmp(s, "rtcs:///", 8))
		{	//implicitly using the ip:port of the responder, instead of that being specified (giving a consistent route to it instead of it having to guess what hostname we used).
			brokerid = s+((s[4]==':')?7:6);
			adr = brokeradr;
			if (!*brokerid)
				continue;	//invalid...
		}
		else if (*s == '/')
		{
			brokerid = s;
			adr = brokeradr;
		}
#ifndef HAVE_PACKET
		else if ((*s=='[' || (*s >= '0' && *s <= '9')) && infostring)
		{	//if we don't have support for udp packets here, convert any raw address to an rtc:///udp/ADDRESS one instead, via this master's brokering services... hopefully.
			brokerid = va("/udp/%s", s);
			adr = brokeradr;
		}
#endif
		else
		{
			if (!NET_StringToAdr2(s, 80, &adr, 1, &brokerid))
				continue;
		}

		if ((info = Master_InfoForServer(&adr, brokerid)))	//remove if the server already exists.
		{
			info->sends = 1;	//reset.
			MasterInfo_ProcessHTTPInfo(info, infostring);

			Master_ResortServer(info);
		}
		else
		{
			info = Z_Malloc(sizeof(serverinfo_t));
			info->adr = adr;
			info->sends = 1;
			Q_strncpyz(info->brokerid, brokerid?brokerid:"", sizeof(info->brokerid));

			info->special = 0;
			if (protocoltype == MP_QUAKEWORLD)
				info->special |= SS_QUAKEWORLD;
			else if (protocoltype == MP_DPMASTER)	//actually ftemaster... so assume fteqw servers not ftenq ones unless otherwise indicated.
				info->special |= SS_UNKNOWN|SS_GETINFO;
#if defined(Q2CLIENT) || defined(Q2SERVER)
			else if (protocoltype == MP_QUAKE2)
				info->special |= SS_QUAKE2;
#endif
#if defined(Q3CLIENT) || defined(Q3SERVER)
			else if (protocoltype == MP_QUAKE3)
				info->special |= SS_QUAKE3;
#endif
#ifdef NQPROT
			else if (protocoltype == MP_NETQUAKE)
				info->special |= SS_NETQUAKE;
#endif

			info->refreshtime = 0;
			info->ping = PING_DEAD;

			MasterInfo_ProcessHTTPInfo(info, infostring);

			info->next = firstserver;
			firstserver = info;

			Master_ResortServer(info);
		}
		info->status |= SRVSTATUS_GLOBAL;
	}
}
#endif

//don't try sending to servers we don't support
static void MasterInfo_Request(master_t *mast)
{
	if (!mast)
		return;

	if (mast->sends)
		mast->sends--;

	//these are generic requests
	switch(mast->mastertype)
	{
#ifdef WEBCLIENT
	case MT_MASTERHTTP:
		if (!mast->dl)
		{
			mast->dl = HTTP_CL_Get(mast->address, NULL, MasterInfo_ProcessHTTP);
			if (mast->dl)
			{
				mast->dl->user_ctx = mast;
				mast->dl->isquery = true;
			}
		}
		break;
#endif
#if POLLTOTALSOCKETS>0
	case MT_MASTERUDP:
		switch(mast->protocoltype)
		{
#ifdef Q3CLIENT
		case MP_QUAKE3:
			{
				char *str;
				if (mast->adr.type == NA_IPV6)
					str = va("%c%c%c%cgetserversExt %u empty full ipv6\n", 255, 255, 255, 255, 68);
				else
					str = va("%c%c%c%cgetservers %u empty full\n", 255, 255, 255, 255, 68);
				NET_SendPollPacket (strlen(str), str, mast->adr);
			}
			break;
#endif
#ifdef Q2CLIENT
		case MP_QUAKE2:
			if (mast->adr.type == NA_IP)	//qw masters have no ipx/ipv6 reporting, so its a bit pointless
				NET_SendPollPacket (6, "query", mast->adr);
			break;
#endif
		case MP_QUAKEWORLD:
			if (mast->adr.type == NA_IP)	//qw masters have no ipx/ipv6 reporting, so its a bit pointless
				NET_SendPollPacket (3, "c\n", mast->adr);
			break;
#ifdef NQPROT
		case MP_NETQUAKE:
			//there is no nq udp master protocol
			break;
#endif
		case MP_DPMASTER:
			{
				char *str;
				char game[MAX_QPATH];
				char *games = com_protocolname.string;
				while(*games)
				{	//send a request for each game listed.
					games = COM_ParseOut(games, game, sizeof(game));

					//for compat with dp, we use the nq netchan version. which is stupid, but whatever
					//we ask for ipv6 addresses from ipv6 masters (assuming it resolved okay)
					if (mast->adr.type == NA_IPV6)
						str = va("%c%c%c%cgetserversExt %s %u empty full ipv6", 255, 255, 255, 255, game, com_protocolversion.ival);
					else
						str = va("%c%c%c%cgetservers %s %u empty full", 255, 255, 255, 255, game, com_protocolversion.ival);
					NET_SendPollPacket (strlen(str), str, mast->adr);
				}
			}
			break;
		}
		break;
	case MT_BCAST:
	case MT_SINGLE:	//FIXME: properly add the server and flag it for resending instead of directly pinging it
		switch(mast->protocoltype)
		{
#ifdef Q3CLIENT
		case MP_QUAKE3:
			NET_SendPollPacket (14, va("%c%c%c%cgetstatus\n", 255, 255, 255, 255), mast->adr);
			break;
#endif
#ifdef Q2CLIENT
		case MP_QUAKE2:
			NET_SendPollPacket (11, va("%c%c%c%cstatus\n", 255, 255, 255, 255), mast->adr);
			break;
#endif
		case MP_QUAKEWORLD:
			NET_SendPollPacket (14, va("%c%c%c%cstatus %i\n", 255, 255, 255, 255, STATUS_QTVLIST|STATUS_SHOWTEAMS|STATUS_SPECTATORS|STATUS_PLAYERS|STATUS_SERVERINFO), mast->adr);
			break;
#ifdef NQPROT
		case MP_NETQUAKE:
			SZ_Clear(&net_message);
			net_message.packing = SZ_RAWBYTES;
			net_message.currentbit = 0;
			MSG_WriteLong(&net_message, 0);// save space for the header, filled in later
			MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
			MSG_WriteString(&net_message, NET_GAMENAME_NQ);	//look for either sort of server
			MSG_WriteByte(&net_message, NQ_NETCHAN_VERSION);
			*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
			NET_SendPollPacket(net_message.cursize, net_message.data, mast->adr);
			SZ_Clear(&net_message);
			break;
#endif
		case MP_DPMASTER:	//fixme
			{
				char *str;
				str = va("%c%c%c%cgetinfo", 255, 255, 255, 255);
				NET_SendPollPacket (strlen(str), str, mast->adr);
			}
			break;
		}
		break;
#endif
	}
}


void MasterInfo_WriteServers(void)
{
	serverinfo_t *server;
	vfsfile_t *qws;
	char adr[MAX_ADR_SIZE];

	if (slist_writeserverstxt.ival && sb_favouriteschanged)
	{
		qws = FS_OpenVFS(FAVOURITESFILE, "wt", FS_ROOT);
		if (qws)
		{
			sb_favouriteschanged = false;
			for (server = firstserver; server; server = server->next)
			{
				if (server->special & SS_FAVORITE)
				{
					switch(server->special & SS_PROTOCOLMASK)
					{
					case SS_QUAKE3:
						VFS_PUTS(qws, va("%s\t%s\t%s\n", Master_ServerToString(adr, sizeof(adr), server), "favorite:q3", server->name));
						break;
					case SS_QUAKE2:
						VFS_PUTS(qws, va("%s\t%s\t%s\n", Master_ServerToString(adr, sizeof(adr), server), "favorite:q2", server->name));
						break;
					case SS_NETQUAKE:
						VFS_PUTS(qws, va("%s\t%s\t%s\n", Master_ServerToString(adr, sizeof(adr), server), "favorite:nq", server->name));
						break;
//					case SS_DARKPLACES:
//						VFS_PUTS(qws, va("%s\t%s\t%s\n", Master_ServerToString(adr, sizeof(adr), server), "favorite:dp", server->name));
//						break;
					case SS_QUAKEWORLD:
						VFS_PUTS(qws, va("%s\t%s\t%s\n", Master_ServerToString(adr, sizeof(adr), server), "favorite:qw", server->name));
						break;
					}
				}
			}

			VFS_CLOSE(qws);
		}
	}
}

//poll master servers for server lists.
void MasterInfo_Refresh(qboolean doreset)
{
	master_t *mast;
	serverinfo_t *info;
	qboolean loadedone;

	if (doreset)
	{
		for (info = firstserver; info; info = info->next)
			info->status &= ~SRVSTATUS_ALIVE;	//hide until we get a new response from it.
	}

	loadedone = false;
	loadedone |= Master_LoadMasterList("masters.txt", false, MT_MASTERUDP, MP_QUAKEWORLD, 5);	//fte listing

	Master_LoadMasterList(FAVOURITESFILE, false, MT_MASTERUDP, MP_QUAKEWORLD, 1);

	if (!loadedone)
	{
		int i;

		Master_LoadMasterList("sources.txt", true, MT_MASTERUDP, MP_QUAKEWORLD, 5);	//merge with ezquake compat listing

		Master_LoadMasterList("servers.txt", false, MT_MASTERUDP, MP_QUAKEWORLD, 1);

		Master_AddMaster("255.255.255.255:"STRINGIFY(PORT_DEFAULTSERVER),			MT_BCAST,			MP_DPMASTER, "Nearby Game Servers.");
#ifndef QUAKETC
		Master_AddMaster("255.255.255.255:"STRINGIFY(PORT_QWSERVER),				MT_BCAST,			MP_QUAKEWORLD, "Nearby QuakeWorld UDP servers.");
//		Master_AddMasterHTTP("http://www.gameaholic.com/servers/qspy-quakeworld",	MT_MASTERHTTP,		MP_QUAKEWORLD, "gameaholic's QW master");
//		Master_AddMasterHTTP("https://www.quakeservers.net/lists/servers/global.txt",MT_MASTERHTTP,		MP_QUAKEWORLD, "QuakeServers.net (http)");
#endif
#ifdef NQPROT
//		Master_AddMasterHTTP("http://www.gameaholic.com/servers/qspy-quake",		MT_MASTERHTTP,		MP_NETQUAKE, "gameaholic's NQ master");
//		Master_AddMasterHTTP("http://servers.quakeone.com/index.php?format=json",	MT_MASTERHTTPJSON,	MP_NETQUAKE, "quakeone's server listing");
		Master_AddMaster("255.255.255.255:"STRINGIFY(PORT_NQSERVER),				MT_BCAST,			MP_NETQUAKE, "Nearby Quake1 servers");
		Master_AddMaster("255.255.255.255:"STRINGIFY(PORT_NQSERVER),				MT_BCAST,			MP_DPMASTER, "Nearby DarkPlaces servers");	//only responds to one type, depending on active protocol.
#endif
#ifdef Q2CLIENT
//		Master_AddMasterHTTP("http://www.gameaholic.com/servers/qspy-quake2",		MT_MASTERHTTP,		MP_QUAKE2, "gameaholic's Q2 master");
		Master_AddMasterHTTP("http://q2servers.com/?raw=1",							MT_MASTERHTTP,		MP_QUAKE2, "q2servers.com");	//https is fucked. binary version is defective as it has no way to represent ipv6, so don't use that.
		Master_AddMaster("255.255.255.255:27910",									MT_BCAST,			MP_QUAKE2, "Nearby Quake2 UDP servers.");
#endif
#ifdef Q3CLIENT
//		Master_AddMasterHTTP("http://www.gameaholic.com/servers/qspy-quake3",		MT_MASTERHTTP,		MP_QUAKE3, "gameaholic's Q3 master");
		Master_AddMaster("255.255.255.255:"STRINGIFY(PORT_Q3SERVER),				MT_BCAST,			MP_QUAKE3, "Nearby Quake3 UDP servers.");
#endif

		if (!*net_ice_broker.string)
			;	//nope, sorry, not configured.
		else
		{
			char *url;
			COM_Parse(com_protocolname.string);
			if (!strncmp(net_ice_broker.string, "tls://", 6))
				url = va("https://%s/raw/%s", net_ice_broker.string+6, com_token);
			else if (!strncmp(net_ice_broker.string, "tcp://", 6))
				url = va("http://%s/raw/%s", net_ice_broker.string+6, com_token);
			else
				url = va("http://%s/raw/%s", net_ice_broker.string, com_token);
			Master_AddMasterHTTP(url,				MT_MASTERHTTP,		MP_DPMASTER, "Public Servers Potentially Behind A NAT.");
		}

		for (i = 0; net_masterlist[i].cv.name; i++)
		{
			Master_AddMaster(net_masterlist[i].cv.string, MT_MASTERUDP, net_masterlist[i].protocol, net_masterlist[i].comment);
		}
	}


	for (mast = master; mast; mast=mast->next)
	{
		mast->sends = 1;
	}

	Master_SortServers();
}

#if POLLTOTALSOCKETS>0
void Master_QueryServer(serverinfo_t *server)
{
	char	data[2048];
	server->sends--;
	if (*server->brokerid)
		return;	//don't even try. we have no direct route.
	server->refreshtime = Sys_DoubleTime();

	if (server->special & SS_GETINFO)
	{
		if (server->moreinfo)
			Q_snprintfz(data, sizeof(data), "%c%c%c%cgetstatus", 255, 255, 255, 255);
		else
			Q_snprintfz(data, sizeof(data), "%c%c%c%cgetinfo", 255, 255, 255, 255);
	}
	else switch(server->special & SS_PROTOCOLMASK)
	{
	case SS_QUAKE3:
		Q_snprintfz(data, sizeof(data), "%c%c%c%cgetstatus", 255, 255, 255, 255);
		break;
#ifdef NQPROT
	case SS_NETQUAKE:
		SZ_Clear(&net_message);
		net_message.packing = SZ_RAWBYTES;
		net_message.currentbit = 0;
		MSG_WriteLong(&net_message, 0);// save space for the header, filled in later
		MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
		MSG_WriteString(&net_message, NET_GAMENAME_NQ);	//look for either sort of server
		MSG_WriteByte(&net_message, NQ_NETCHAN_VERSION);
		*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		NET_SendPollPacket(net_message.cursize, net_message.data, server->adr);
		SZ_Clear(&net_message);
		return;
#endif
	case SS_QUAKEWORLD:
		Q_snprintfz(data, sizeof(data), "%c%c%c%cstatus %i\n", 255, 255, 255, 255, STATUS_QTVLIST|STATUS_SHOWTEAMS|STATUS_SPECTATORS|STATUS_PLAYERS|STATUS_SERVERINFO);
		break;
	case SS_QUAKE2:
		Q_snprintfz(data, sizeof(data), "%c%c%c%cstatus\n", 255, 255, 255, 255);
		break;
	default:
		return;
	}
	if (!NET_SendPollPacket (strlen(data), data, server->adr))
		server->sends++; //if we failed, just try again later
}
//send a packet to each server in sequence.
qboolean CL_QueryServers(void)
{
	static int poll;
	int op;
	serverinfo_t *server;
	master_t *mast;

	Master_DetermineMasterTypes();

	op = poll;

	for (mast = master; mast; mast=mast->next)
	{
		switch (mast->protocoltype)
		{
		case MP_UNSPECIFIED:
			continue;
		case MP_DPMASTER:	//dpmaster allows the client to specify the protocol to query. this means it always matches the current game type, so don't bother allowing the user to disable it.
			if (!sb_enabledarkplaces)
				continue;
			break;
#ifdef NQPROT
		case MP_NETQUAKE:
			if (!sb_enablenetquake)
				continue;
			break;
#endif
		case MP_QUAKEWORLD:
			if (!sb_enablequakeworld)
				continue;
			break;
#ifdef Q2CLIENT
		case MP_QUAKE2:
			if (!sb_enablequake2)
				continue;
			break;
#endif
#ifdef Q3CLIENT
		case MP_QUAKE3:
			if (!sb_enablequake3)
				continue;
			break;
#endif
		}

		if (mast->sends > 0)
			MasterInfo_Request(mast);
	}


	for (server = firstserver; op>0 && server; server=server->next, op--);

	if (!server)
	{
		poll = 0;
		return false;
	}

	if (op == 0)
	{

		//we only want to send poll packets to servers which will not be filtered (otherwise it's pointless)
		while(server)
		{
			qboolean enabled;
			switch(Master_BaseGame(server))
			{
			case SS_UNKNOWN: enabled = true; break;
			case SS_QUAKE3: enabled = sb_enablequake3; break;
			case SS_QUAKE2: enabled = sb_enablequake2; break;
			case SS_NETQUAKE: enabled = sb_enablenetquake; break;
			case SS_QUAKEWORLD: enabled = sb_enablequakeworld; break;
			default: enabled = false; break;
			}
			if (enabled)
			{
				if (server && server->sends > 0)
				{
					Master_QueryServer(server);
					poll++;
					return true;
				}
			}
			server = server->next;
			poll++;
		}
		if (!server)
		{
			poll = 0;
			server = firstserver;
			while (server)
			{
				qboolean enabled;
				switch(Master_BaseGame(server))
				{
				case SS_UNKNOWN: enabled = true; break;
				case SS_QUAKE3: enabled = sb_enablequake3; break;
				case SS_QUAKE2: enabled = sb_enablequake2; break;
				case SS_NETQUAKE: enabled = sb_enablenetquake; break;
				case SS_QUAKEWORLD: enabled = sb_enablequakeworld; break;
				default: enabled = false; break;
				}
				if (enabled)
				{
					if (server && server->sends > 0)
					{
						Master_QueryServer(server);
						poll++;
						return true;
					}
				}
				server = server->next;
				poll++;
			}
		}
	}

	return false;
}

void MasterInfo_RemovePlayers(netadr_t *adr)
{
	player_t *p, *prev;
	prev = NULL;
	for (p = mplayers; p; )
	{
		if (NET_CompareAdr(&p->adr, adr))
		{
			if (prev)
				prev->next = p->next;
			else
				mplayers = p->next;
			Z_Free(p);
			p=prev;

			continue;
		}
		else
			prev = p;

		p = p->next;
	}
}

void MasterInfo_AddPlayer(netadr_t *serveradr, char *name, int ping, int frags, int colours, char *skin, char *team)
{
	player_t *p;
	p = Z_Malloc(sizeof(player_t));
	p->next = mplayers;
	p->adr = *serveradr;
	p->colour = colours;
	p->frags = frags;
	Q_strncpyz(p->team, team, sizeof(p->team));
	Q_strncpyz(p->name, name, sizeof(p->name));
	Q_strncpyz(p->skin, skin, sizeof(p->skin));
	mplayers = p;
}

static void CL_ReadPingListEntry(serverinfo_t *info, netadrtype_t type, size_t *maxpeers)
{
	serverinfo_t *peer;
	unsigned short ping;
	int i;
	netadr_t pa;
	char adr[MAX_ADR_SIZE];
	memset(&pa, 0, sizeof(pa));

	pa.type = type;
	if (type == NA_IP)
	{
		for (i = 0; i < countof(pa.address.ip); i++)
			pa.address.ip[i] = MSG_ReadByte();
	}
	else if (type == NA_IPV6)
	{
		for (i = 0; i < countof(pa.address.ip6); i++)
			pa.address.ip6[i] = MSG_ReadByte();
	}
	else
	{
		Sys_Error("CL_ReadPingListEntry: Unsupported netadrtype_t\n");
		return;	//error...
	}
	pa.port = htons(MSG_ReadShort());	//little endian... stored into a network-endian variable...
	ping  = MSG_ReadShort();

	if (NET_ClassifyAddress(&pa, NULL) >= ASCOPE_NET)
	{
		peer = Master_InfoForServer(&pa, NULL);
		if (!peer)
		{
			//generate some lame peer node that we can use.
			peer = Z_Malloc(sizeof(serverinfo_t));
			peer->adr = pa;
			peer->sends = 1;
			peer->special = SS_QUAKEWORLD;
			peer->refreshtime = 0;
			peer->ping = PING_DEAD;
			Q_snprintfz(peer->name, sizeof(peer->name), "%s p", Master_ServerToString(adr, sizeof(adr), peer));
			peer->next = firstserver;
			firstserver = peer;
		}

		for (i = 0; i < info->numpeers; i++)
		{
			if (info->peers[i].peer == peer)
				break;
		}
		if (i == *maxpeers)
		{	//need a new one
			info->numpeers = i+1;
			Z_ReallocElements((void**)&info->peers, maxpeers, info->numpeers+64, sizeof(*info->peers));
		}
		info->peers[i].peer = peer;
		info->peers[i].ping = ping;
	}
}
static void CL_ReadPingList(void)
{
	serverinfo_t *info = Master_InfoForServer(&net_from, NULL);
	size_t count = info->numpeers;
	for(;;)
	{
		int type = MSG_ReadByte();
		if (type == '\\')
			CL_ReadPingListEntry(info, NA_IP, &count);
		else if (type == '/')
			CL_ReadPingListEntry(info, NA_IPV6, &count);
		else
			break;	//don't know, don't corrupt it.
	}
	if (count > info->numpeers)
	{	//trim it...
		Z_ReallocElements((void**)&info->peers, &count, info->numpeers, sizeof(*info->peers));
		info->numpeers = count;
	}
}

//we got told about a server, parse it's info
static int CL_ReadServerInfo(char *msg, enum masterprotocol_e prototype, qboolean favorite)
{
	serverdetailedinfo_t details;

	char *token;
	char *nl;
	char *name;
	int ping;
	int len, j, k;
	serverinfo_t *info;
	char adr[MAX_ADR_SIZE];

	info = Master_InfoForServer(&net_from, NULL);

	if (!info)	//not found...
	{
		if (atoi(Info_ValueForKey(msg, "sv_punkbuster")))
			return false;	//never add servers that require punkbuster. :(
//		if (atoi(Info_ValueForKey(msg, "sv_pure")))
//			return false;	//we don't support the filesystem hashing. :(

		info = Z_Malloc(sizeof(serverinfo_t));

		info->adr = net_from;

		Q_snprintfz(info->name, sizeof(info->name), "%s ?", Master_ServerToString(adr, sizeof(adr), info));

		info->next = firstserver;
		firstserver = info;

		//server replied from a broadcast message, make sure we ping it to retrieve its actual ping
		info->sends = 1;
		info->ping = PING_DEAD;	//not known
		info->special |= SS_LOCAL;
	}
	else
	{
		//determine the ping
		if (info->refreshtime)
		{
			ping = (Sys_DoubleTime() - info->refreshtime)*1000;
			if (ping > PING_MAX)
				info->ping = PING_MAX;	//highest (that is known)
			else
				info->ping = ping;
		}
		info->refreshtime = 0;
	}

	info->status |= SRVSTATUS_ALIVE;

	nl = strchr(msg, '\n');
	if (nl)
	{
		*nl = '\0';
		nl++;
	}

	if (info->special & SS_PROXY)
	{
		if (!*Info_ValueForKey(msg, "hostname"))
		{	//qq, you suck
			//this is a proxy peer list, not an actual serverinfo update.
			size_t count = info->numpeers;
			int remaining = (net_message.cursize - 5) / 8;
			net_message.currentbit = (5)<<3;
			while (remaining --> 0)
				CL_ReadPingListEntry(info, NA_IP, &count);
			info->numpeers = count;
			return false;
		}
	}

	name = Info_ValueForKey(msg, "hostname");
	if (!*name)
		name = Info_ValueForKey(msg, "sv_hostname");
	Q_strncpyz(info->name, name, sizeof(info->name));
	info->special = info->special & (SS_FAVORITE | SS_KEEPINFO | SS_LOCAL | SS_GETINFO);	//favorite+local is never cleared
	if (!strcmp(DISTRIBUTION, Info_ValueForKey(msg, "*distrib")))	//outdated
		info->special |= SS_FTESERVER;
	else if (!strncmp(DISTRIBUTION, Info_ValueForKey(msg, "*version"), strlen(DISTRIBUTION)))
		info->special |= SS_FTESERVER;

	if (!MasterInfo_ReadProtocol(info, msg))
	{	//try and guess.
		if (0)
			;
#ifdef Q2CLIENT
		else if (prototype == MP_QUAKE2)
			info->special |= SS_QUAKE2;
#endif
#ifdef Q3CLIENT
		else if (prototype == MP_QUAKE3 || prototype == MP_DPMASTER/*if no protocol, assume q3 behaviours*/)
			info->special |= SS_QUAKE3;
#endif
#ifdef NQPROT
		else if (prototype == MP_NETQUAKE)
			info->special |= SS_NETQUAKE;
#endif
		else
			info->special |= SS_QUAKEWORLD;
	}
	if (favorite)	//was specifically named, not retrieved from a master.
		info->special |= SS_FAVORITE;

	info->players = 0;
	ping = atoi(Info_ValueForKey(msg, "maxclients"));
	if (!ping)
		ping = atoi(Info_ValueForKey(msg, "sv_maxclients"));
	info->maxplayers = bound(0, ping, 255);

	ping = atoi(Info_ValueForKey(msg, "timelimit"));
	info->tl = bound(-32768, ping, 32767);
	ping = atoi(Info_ValueForKey(msg, "fraglimit"));
	info->fl = bound(-32768, ping, 32767);

	if (*Info_ValueForKey(msg, "*qtv") || *Info_ValueForKey(msg, "*QTV"))
		info->special |= SS_PROXY|SS_FTESERVER;	//qtv
	else if (!strcmp(Info_ValueForKey(msg, "*progs"), "666") && !strcmp(Info_ValueForKey(msg, "*version"), "2.91"))
		info->special |= SS_PROXY;	//qizmo
	else if (!Q_strncmp(Info_ValueForKey(msg, "*version"), "qwfwd", 5))
	{
		char *msg = "\xff\xff\xff\xffpingstatus ext";
		NET_SendPollPacket(strlen(msg), msg, info->adr);
		if (info->peers)
		{	//let em time out
			Z_Free(info->peers);
			info->peers = NULL;
			info->numpeers = 0;
		}
		info->special |= SS_PROXY|SS_RELAY;	//qwfwd
	}
	else if (!Q_strncasecmp(Info_ValueForKey(msg, "*version"), "qtv ", 4))
		info->special |= SS_PROXY;	//eztv

	token = Info_ValueForKey(msg, "map");
	if (!*token)
		token = Info_ValueForKey(msg, "mapname");
	Q_strncpyz(info->map,		token,	sizeof(info->map));

	token = Info_ValueForKey(msg, "*gamedir");
	if (!*token)
		token = Info_ValueForKey(msg, "gamedir");
	if (!*token)
		token = Info_ValueForKey(msg, "modname");
	Q_strncpyz(info->gamedir,	token,	sizeof(info->gamedir));
	Q_strncpyz(info->qcstatus,		Info_ValueForKey(msg, "qcstatus"),	sizeof(info->qcstatus));
	Q_strncpyz(info->modname,		Info_ValueForKey(msg, "modname"),	sizeof(info->modname));

//	info->gameversion = atoi(Info_ValueForKey(msg, "gameversion"));

	info->numbots = 0;//atoi(Info_ValueForKey(msg, "bots"));
	info->numhumans = info->players - info->numbots;
	info->freeslots = info->maxplayers - info->players;

	strcpy(details.info, msg);
	msg = msg+strlen(msg)+1;

	//clear player info. unless its an NQ server, which have some really annoying protocol to find out the players.
	if ((info->special & (SS_PROTOCOLMASK|SS_GETINFO)) == SS_NETQUAKE)
	{
		if (!info->moreinfo && ((slist_cacheinfo.value == 2 || NET_CompareAdr(&info->adr, &selectedserver.adr)) || (info->special & SS_KEEPINFO)))
			info->moreinfo = Z_Malloc(sizeof(serverdetailedinfo_t));
		info->numhumans = info->players = atoi(Info_ValueForKey(details.info, "clients"));
	}
	else
	{
		MasterInfo_RemovePlayers(&info->adr);
		info->players=details.numplayers = 0;
		if (!strchr(msg, '\n'))
			info->numhumans = info->players = atoi(Info_ValueForKey(details.info, "clients"));
		else
		{
			int clnum;

			for (clnum=0; ; clnum++)
			{
				nl = strchr(msg, '\n');
				if (!nl)
					break;
				*nl = '\0';

				if (!strncmp(msg, "qtv ", 4))
				{	//qtv destnum "proxyname" "stream@host:port" viewercount
					char proxstream[128];
					char tokval[256];
					token = msg+4;

					token = COM_ParseOut(token, tokval,sizeof(tokval));
					//destnum = atoi(tokval);
					token = COM_ParseOut(token, tokval,sizeof(tokval));
					//proxy name...
					token = COM_ParseOut(token, proxstream,sizeof(proxstream));
					token = COM_ParseOut(token, tokval,sizeof(tokval));
					//viewercount = atoi(tokval);

					if (*proxstream)
						Info_SetValueForKey(details.info, "qtvstream", proxstream, sizeof(details.info));
					continue;
				}

				if (clnum == MAX_CLIENTS)
					break;

				details.players[clnum].isspec = 0;
				details.players[clnum].team[0] = 0;
				details.players[clnum].skin[0] = 0;

				token = msg;
				if (!token)
					break;
				details.players[clnum].userid = atoi(token);
				token = strchr(token+1, ' ');
				if (!token)
					break;
				details.players[clnum].frags = atoi(token);
				token = strchr(token+1, ' ');
				if (!token)
					break;
				details.players[clnum].time = atoi(token);
				msg = token;
				token = COM_Parse(token);
				if (!*token)	//probably q2 response
				{
					//see if this is actually a Quake2 server.
					token = strchr(msg+1, '\"');
					if (!token)	//it wasn't.
						break;

					details.players[clnum].ping = details.players[clnum].frags;
					details.players[clnum].frags = details.players[clnum].userid;

					msg = strchr(token+1, '\"');
					if (!msg)
						break;
					len = msg - token;
					if (len >= sizeof(details.players[clnum].name))
						len = sizeof(details.players[clnum].name);
					Q_strncpyz(details.players[clnum].name, token+1, len);

					details.players[clnum].skin[0] = '\0';

					details.players[clnum].topc = 0;
					details.players[clnum].botc = 0;
					details.players[clnum].time = 0;
				}
				else	//qw response
				{
					details.players[clnum].ping = atoi(token);
					msg = token;
					token = strchr(msg+1, ' ');
					if (!token)
						break;

					token = strchr(token+1, '\"');
					if (!token)
						break;
					msg = strchr(token+1, '\"');
					if (!msg)
						break;
					len = msg - token;
					if (len >= sizeof(details.players[clnum].name))
						len = sizeof(details.players[clnum].name);
					if (!strncmp(token, "\"\\s\\", 4))
					{
						details.players[clnum].isspec |= 1;
						Q_strncpyz(details.players[clnum].name, token+4, len-3);
					}
					else
						Q_strncpyz(details.players[clnum].name, token+1, len);
					details.players[clnum].name[len] = '\0';

					token = strchr(msg+1, '\"');
					if (!token)
						break;
					msg = strchr(token+1, '\"');
					if (!msg)
						break;
					len = msg - token;
					if (len >= sizeof(details.players[clnum].skin))
						len = sizeof(details.players[clnum].skin);
					Q_strncpyz(details.players[clnum].skin, token+1, len);
					details.players[clnum].skin[len] = '\0';

					token = strchr(msg+1, ' ');
					if (!token)
						break;
					details.players[clnum].topc = atoi(token);
					token = strchr(token+1, ' ');
					if (!token)
						break;
					details.players[clnum].botc = atoi(token);

					token = strchr(msg+1, '\"');
					Q_strncpyz(details.players[clnum].team, "", sizeof(details.players[clnum].team));
					if (token)
					{
						msg = strchr(token+1, '\"');
						if (msg)
						{
							len = msg - token;
							if (len >= sizeof(details.players[clnum].team))
								len = sizeof(details.players[clnum].team);
							Q_strncpyz(details.players[clnum].team, token+1, len);
							details.players[clnum].team[len] = '\0';
						}
					}
				}

				MasterInfo_AddPlayer(&info->adr, details.players[clnum].name, details.players[clnum].ping, details.players[clnum].frags, details.players[clnum].topc*4 | details.players[clnum].botc, details.players[clnum].skin, details.players[clnum].team);

				//WallFly is some q2 bot
				//[ServeMe] is some qw bot
				if (!strncmp(details.players[clnum].name, "WallFly", 7) || !strcmp(details.players[clnum].name, "[ServeMe]"))
				{
					//not players nor real people. they don't count towards any metric
					details.players[clnum].isspec |= 3;
				}
				//807 excludes the numerous bot names on some annoying qwtf server
				//BOT: excludes fte's botclients (which always have a bot: prefix)
				else if (details.players[clnum].ping == 807 || !strncmp(details.players[clnum].name, "BOT:", 4))
				{
					info->numbots++;
					details.players[clnum].isspec |= 2;
				}
				else if (details.players[clnum].isspec & 1)
				{
					info->numspectators++;
				}
				else
					info->numhumans++;

				for (k = clnum, j = clnum-1; j >= 0; j--)
				{
					if ((details.players[k].isspec != details.players[j].isspec && !details.players[k].isspec) ||
						details.players[k].frags > details.players[j].frags)
					{
						struct serverdetailedplayerinfo_s t = details.players[j];
						details.players[j] = details.players[k];
						details.players[k] = t;
						k = j;
					}
					else
						break;
				}
				details.numplayers++;

				info->players++;

				msg = nl;
				if (!msg)
					break;	//erm...
				msg++;
			}
		}
		if (!info->numbots)
		{
			info->numbots = atoi(Info_ValueForKey(details.info, "bots"));
			if (info->numbots > info->players)
				info->numbots = info->players;
			info->numhumans -= info->numbots;
		}


		if (!info->moreinfo && ((slist_cacheinfo.value == 2 || (NET_CompareAdr(&info->adr, &selectedserver.adr)&&!strcmp(info->brokerid,selectedserver.brokerid))) || (info->special & SS_KEEPINFO)))
			info->moreinfo = Z_Malloc(sizeof(serverdetailedinfo_t));
		if (NET_CompareAdr(&info->adr, &selectedserver.adr)&&!strcmp(info->brokerid,selectedserver.brokerid))
			selectedserver.detail = info->moreinfo;

		if (info->moreinfo)
			memcpy(info->moreinfo, &details, sizeof(serverdetailedinfo_t));
	}

	info->qccategory = 0;
#ifdef MENU_DAT
	categorisingserver = info;
	info->qccategory = MP_GetServerCategory(-1);
	categorisingserver = NULL;
#endif

	return true;
}

//rewrite to scan for existing server instead of wiping all.
void CL_MasterListParse(netadrtype_t adrtype, int type, qboolean slashpad)
{
	serverinfo_t *info;
	serverinfo_t *last, *old;
	int adrlen;

	int p1, p2;
	char adr[MAX_ADR_SIZE];
	int i;

	char madr[MAX_ADR_SIZE];

	switch(adrtype)
	{
	case NA_IP:
		adrlen = 4;
		break;
	case NA_IPV6:
		adrlen = 16;
		break;
	case NA_IPX:
		adrlen = 10;
		break;
	default:
		return;
	}

	NET_AdrToString(madr, sizeof(madr), &net_from);

	MSG_ReadByte ();	//should be \n

	last = firstserver;

	while((net_message.currentbit>>3)+1+2 < net_message.cursize)
	{
		if (slashpad)
		{
			switch(MSG_ReadByte())
			{
			case '\\':
				adrtype = NA_IP;
				adrlen = 4;
				break;
			case '/':
				adrtype = NA_IPV6;
				adrlen = 16;
				break;
			default:
				firstserver = last;
				return;
			}
		}

		info = Z_Malloc(sizeof(serverinfo_t));
		info->adr.type = adrtype;
		switch(adrtype)
		{
		case NA_IP:
		case NA_IPV6:
		case NA_IPX:
			//generic fixed-length addresses
			for (i = 0; i < adrlen; i++)
				((qbyte *)&info->adr.address)[i] = MSG_ReadByte();
			break;
		default:
			break;
		}
		info->ping = PING_DEAD;

		p1 = MSG_ReadByte();
		p2 = MSG_ReadByte();
		info->adr.port = htons((unsigned short)((p1<<8)|p2));
		if (!info->adr.port)
		{
			Z_Free(info);
			break;
		}
		if ((old = Master_InfoForServer(&info->adr, NULL)))	//remove if the server already exists.
		{
			if ((old->special & (SS_PROTOCOLMASK|SS_GETINFO)) != (type & (SS_PROTOCOLMASK|SS_GETINFO)))
				old->special = type | (old->special & (SS_FAVORITE|SS_LOCAL));
			old->sends = 1;	//reset.
			old->status |= SRVSTATUS_GLOBAL;
			Z_Free(info);
		}
		else
		{
			info->sends = 1;

			info->special = type;
			info->refreshtime = 0;
			info->status |= SRVSTATUS_GLOBAL;

			Q_snprintfz(info->name, sizeof(info->name), "%s (via %s)", Master_ServerToString(adr, sizeof(adr), info), madr);

			info->next = last;
			last = info;

			Master_ResortServer(info);
		}
	}

	firstserver = last;
}
#else
void Master_QueryServer(serverinfo_t *server){}
qboolean CL_QueryServers(void)
{
	master_t *mast;

	Master_DetermineMasterTypes();

	for (mast = master; mast; mast=mast->next)
	{
		switch (mast->protocoltype)
		{
		case MP_UNSPECIFIED:
			continue;
		case MP_DPMASTER:	//dpmaster allows the client to specify the protocol to query. this means it always matches the current game type, so don't bother allowing the user to disable it.
			if (!sb_enabledarkplaces)
				continue;
			break;
#ifdef NQPROT
		case MP_NETQUAKE:
			if (!sb_enablenetquake)
				continue;
			break;
#endif
		case MP_QUAKEWORLD:
			if (!sb_enablequakeworld)
				continue;
			break;
#ifdef Q2CLIENT
		case MP_QUAKE2:
			if (!sb_enablequake2)
				continue;
			break;
#endif
#ifdef Q3CLIENT
		case MP_QUAKE3:
			if (!sb_enablequake3)
				continue;
			break;
#endif
		}

		if (mast->sends > 0)
			MasterInfo_Request(mast);
	}

	return false;	//false to say 'done'.
}
#endif


void CL_Connect_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	serverinfo_t *info;
	char buf[512];
	int len;
	if (argn == 1)
	{
		len = strlen(partial);
		if (len > 1 && partial[len-1] == '\"')
			len--;
		for (info = firstserver; info; info = info->next)
		{
			if (info->ping != PING_DEAD)
			{
				if (len && !Q_strncasecmp(partial, info->name, len))
				{
					if (info->ping == PING_UNKNOWN)
						ctx->cb(info->name, va("^[%s^], %i players, unknown ping", info->name, info->players), Master_ServerToString(buf, sizeof(buf), info), ctx);
					else
						ctx->cb(info->name, va("^[%s^], %i players, %i ping", info->name, info->players, info->ping), Master_ServerToString(buf, sizeof(buf), info), ctx);
					continue;
				}

				Master_ServerToString(buf, sizeof(buf), info);
				//there are too many meaningless servers out there, so only suggest IP addresses if those servers are actually significant (ie: active, or favourite)
				if (!strncmp(partial, buf, len))
				{
					if (info->players || (info->special & SS_FAVORITE) || NET_ClassifyAddress(&info->adr, NULL)<=ASCOPE_LAN || len == strlen(buf))
					{
						if (info->ping == PING_UNKNOWN)
							ctx->cb(buf, va("^[%s^], %i players, unknown ping", info->name, info->players), NULL, ctx);
						else
							ctx->cb(buf, va("^[%s^], %i players, %i ping", info->name, info->players, info->ping), NULL, ctx);
						continue;
					}
				}
			}
		}
	}
}

#else
void CL_Connect_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
}
#endif

#ifdef Q3CLIENT
#if defined(CL_MASTER)
static void NetQ3_LocalServers_f(void)
{
	MasterInfo_Refresh(true);

#if POLLTOTALSOCKETS>0
	{
		netadr_t na;
		if (NET_StringToAdr("255.255.255.255", PORT_Q3SERVER, &na))
			NET_SendPollPacket (14, va("%c%c%c%cgetstatus\n", 255, 255, 255, 255), na);
	}
#endif
}
static void NetQ3_GlobalServers_Request(size_t masternum, int protocol, const char *keywords)
{
#ifdef WEBCLIENT
	if (masternum == countof(net_masterlist))
	{
		const char *url;
		struct dl_download *dl;
		COM_Parse(com_protocolname.string);
		if (*net_ice_broker.string)
		{
			if (!strncmp(net_ice_broker.string, "tls://", 6))
				url = va("https://%s/raw/%s", net_ice_broker.string+6, com_token);
			else if (!strncmp(net_ice_broker.string, "tcp://", 6))
				url = va("http://%s/raw/%s", net_ice_broker.string+6, com_token);
			else
				url = va("http://%s/raw/%s", net_ice_broker.string, com_token);

			dl = HTTP_CL_Get(url, NULL, MasterInfo_ProcessHTTP);
			if (dl)
				dl->isquery = true;
		}
	}
#endif
#if POLLTOTALSOCKETS>0
	if (masternum >= countof(net_masterlist))
		return; //erk
	if (net_masterlist[masternum].protocol == MP_QUAKE3)
	{
		netadr_t adr[16];
		char *str;
		size_t i, n;
		COM_Parse(net_masterlist[masternum].cv.string);	//only want the first one
		n = NET_StringToAdr2(com_token, 0, adr, countof(adr), NULL);
		str = va("%c%c%c%cgetservers %u %s\n", 255, 255, 255, 255, protocol, keywords);
		for (i = 0; i < n; i++)
			NET_SendPollPacket (strlen(str), str, adr[i]);
	}
#endif
}
static void NetQ3_GlobalServers_f(void)
{
	size_t masternum = atoi(Cmd_Argv(1));
	int protocol = atoi(Cmd_Argv(2));
	char *keywords;
	MasterInfo_Refresh(true);

	Cmd_ShiftArgs(2, false);
	keywords = Cmd_Args();

	if (!masternum)
	{
		for (masternum = 0; masternum <= countof(net_masterlist); masternum++)
			NetQ3_GlobalServers_Request(masternum, protocol, keywords);
	}
	else
		NetQ3_GlobalServers_Request(masternum-1, protocol, keywords);
}
#else
static void NetQ3_LocalServers_f(void){}
static void NetQ3_GlobalServers_f(void){}
#endif
#endif
void Net_Master_Init(void)
{
	int i;
	for (i = 0; net_masterlist[i].cv.name; i++)
		Cvar_Register(&net_masterlist[i].cv, "master servers");
#if defined(HAVE_SERVER) && defined(HAVE_LEGACY)
	Cmd_AddCommand ("setmaster", SV_SetMaster_f);
#endif

#ifdef Q3CLIENT
	Cmd_AddCommand ("localservers", NetQ3_LocalServers_f);
	Cmd_AddCommand ("globalservers", NetQ3_GlobalServers_f);
#endif
}

