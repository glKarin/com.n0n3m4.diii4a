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
// server.h

#define	QW_SERVER

typedef enum {
	ss_dead,			// no map loaded
	ss_clustermode,
	ss_loading,			// spawning level edicts
	ss_active,			// actively running
	ss_cinematic
} server_state_t;
// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)

#ifdef SVCHAT
typedef struct chatvar_s {
	char varname[64];
	float value;

	struct chatvar_s *next;
} chatvar_t;
typedef struct {
	qboolean active;
	char filename[64];
	edict_t *edict;

	char maintext[1024];
	struct
	{
		float tag;
		char text[256];
	} option[6];
	int options;

	chatvar_t *vars;

	float time;
} svchat_t;
#endif

typedef struct {
	int netstyle;
	char particleeffecttype[64];
	char stain[3];
	qbyte radius;
	qbyte dlightrgb[3];
	qbyte dlightradius;
	qbyte dlighttime;
	qbyte dlightcfade[3];
} svcustomtents_t;

typedef struct laggedpacket_s
{
	double time;
	struct laggedpacket_s *next;
	int length;
	unsigned char data[MAX_QWMSGLEN+10];
} laggedpacket_t;

typedef struct
{
	vec3_t	origin;
	char	angles[3];
	qbyte	modelindex;
	qbyte	frame;
	qbyte	colormap;
	qbyte	skinnum;
	qbyte	effects;

	qbyte	scale;
	qbyte	trans;
	char	fatness;
} mvdentity_state_t;

typedef struct
{
	vec3_t position;
	unsigned short soundnum;
	qbyte volume;
	qbyte attenuation;
} staticsound_state_t;

extern entity_state_t *sv_staticentities;
extern int sv_max_staticentities;
extern staticsound_state_t *sv_staticsounds;
extern int sv_max_staticsounds;

typedef struct server_s
{
	qboolean	active;				// false when server is going down
	server_state_t	state;			// precache commands are only valid during load

	float		gamespeed;	//time progression multiplier, fixed per-level.
	unsigned int csqcchecksum;
	qboolean	mapchangelocked;

#ifdef SAVEDGAMES
	char		loadgame_on_restart[MAX_QPATH];	//saved game to load on map_restart
	double		autosave_time;
#endif
	double		time;			//time passed since map (re)start
	double		starttime;		//Sys_DoubleTime at the start of the map
	double		restarttime;	//for delayed map restarts - map will restart once sv.time reaches this stamp
	int framenum;
	int logindatabase;

	enum
	{
		PAUSE_EXPLICIT	= 1, //someone hit pause
		PAUSE_SERVICE	= 2, //we're running as a service and someone paused us rather than killing us.
		PAUSE_AUTO		= 4	//console is down in a singleplayer game.
	} paused, oldpaused;
	float			pausedstart;

	//check player/eyes models for hacks
	unsigned	model_player_checksum;
	unsigned	eyes_player_checksum;

//	char		name[64];			// file map name (moved to svs, for restart)
	char		mapname[256];		// text description of the map
	char		modelname[MAX_QPATH];		// maps/<name>.bsp, for model_precache[0]

	world_t world;

	union {
#ifdef Q2SERVER
		struct {
			const char *configstring[Q2MAX_CONFIGSTRINGS];
			const char	*q2_extramodels[MAX_PRECACHE_MODELS];	// NULL terminated
			const char	*q2_extrasounds[MAX_PRECACHE_SOUNDS];	// NULL terminated
		};
#endif
		struct {
#ifdef HAVE_LEGACY
			const char	*vw_model_precache[32];
#endif
			const char	*model_precache[MAX_PRECACHE_MODELS];	// NULL terminated
			const char	*particle_precache[MAX_SSPARTICLESPRE];	// NULL terminated
			const char	*sound_precache[MAX_PRECACHE_SOUNDS];	// NULL terminated
		};
		const char *ptrs[1];
	} strings;
	qboolean	stringsalloced;	//if true, we need to free the string pointers safely rather than just memsetting them to 0
	struct
	{
		const char	*str; //double dynamic. urgh, but allows it to be nice and long.
		vec3_t		colours;
	} *lightstyles;
	size_t maxlightstyles;	//limited to MAX_NET_LIGHTSTYLES

#ifdef HEXEN2
	char		h2miditrack[MAX_QPATH];
	qbyte		h2cdtrack;
#endif

	pvec3_t		skyroom_pos;	//parsed from world._skyroom
	qboolean	skyroom_pos_known;

	int			allocated_client_slots;	//number of slots available. (used mostly to stop single player saved games cacking up)
	int			spawned_client_slots; //number of PLAYER slots which are active (ie: putclientinserver was called)
	int			spawned_observer_slots;

	model_t	*models[MAX_PRECACHE_MODELS];

	struct client_s	*skipbprintclient;	//SV_BroadcastPrint skips this client

	// added to every client's unreliable buffer each frame, then cleared
	sizebuf_t	datagram;
	qbyte		datagram_buf[MAX_DATAGRAM];

	// added to every client's reliable buffer each frame, then cleared
	sizebuf_t	reliable_datagram;
	qbyte		reliable_datagram_buf[MAX_QWMSGLEN*2];

	// the multicast buffer is used to send a message to a set of clients
	sizebuf_t	multicast;
	qbyte		multicast_buf[MAX_QWMSGLEN*2];

#ifdef NQPROT
	sizebuf_t	nqdatagram;
	qbyte		nqdatagram_buf[MAX_NQDATAGRAM];

	sizebuf_t	nqreliable_datagram;
	qbyte		nqreliable_datagram_buf[MAX_NQMSGLEN];

	sizebuf_t	nqmulticast;
	qbyte		nqmulticast_buf[MAX_NQMSGLEN];
#endif

#ifdef Q2SERVER
	sizebuf_t	q2multicast[2];	//0=little/legacy coords, 1=big coords (used for q2e compat)
	qbyte		q2multicast_lcbuf[MAX_Q2MSGLEN];
	qbyte		q2multicast_bcbuf[MAX_Q2MSGLEN*2];
#endif

	// the master buffer is used for building log packets
	sizebuf_t	master;
	qbyte		master_buf[MAX_DATAGRAM];

	// the signon buffer will be sent to each client as they connect
	// traditionally includes the entity baselines, the static entities, etc
	// large levels will have >MAX_DATAGRAM sized signons, so
	// multiple signon messages are kept
	// fte only stores writebyted stuff in here. everything else is regenerated based upon the client's extensions.
	sizebuf_t	signon;
	int			used_signon_space;
	qbyte		signon_buffer[MAX_OVERALLMSGLEN]; //flushed after every 512 bytes (two leading bytes says the size of the buffer).

	qboolean gamedirchanged;

#ifdef QUAKESTATS
	qboolean haveitems2;	//use items2 field instead of serverflags for the high bits of STAT_ITEMS
#endif




#ifdef MVD_RECORDING
	qboolean mvdrecording;
#endif

//====================================================
//this lot is for serverside playback of mvds. Use QTV instead.
#ifdef SERVER_DEMO_PLAYBACK
	qboolean mvdplayback;
	float realtime;
	vfsfile_t *demofile;	//also signifies playing the thing.

	int lasttype;
	int lastto;

//playback spikes (svc_nails/nails2)
	int numdemospikes;
	struct {
		vec3_t org;
		qbyte id;
		qbyte pitch;
		qbyte yaw;
		qbyte modelindex;
	} demospikes[255];

//playback of entities (svc_nails/nails2)
	mvdentity_state_t	*demostate;
	mvdentity_state_t	*demobaselines;
	int demomaxents;
	qboolean demostatevalid;

//players
	struct {
		int stats[MAX_CL_STATS];
		int pl;
		int ping;
		int frags;
		int userid;
		int weaponframe;
		char			userinfo[MAX_INFO_STRING];
		vec3_t oldorg;
		vec3_t oldang;
		float updatetime;
	} recordedplayer[MAX_CLIENTS];

//gamestate
	char		demoinfo[MAX_SERVERINFO_STRING];
	char		demmodel_precache[MAX_MODELS][MAX_QPATH];	// NULL terminated
	char		demsound_precache[MAX_SOUNDS][MAX_QPATH];	// NULL terminated
	char		demgamedir[64];
	char		demname[64];			// map name

	qboolean	democausesreconnect;	//this makes current clients go through the connection process (and when the demo ends too)
	sizebuf_t	demosignon;
	int			num_demosignon_buffers;
	int			demosignon_buffer_size[MAX_SIGNON_BUFFERS];
	qbyte		demosignon_buffers[MAX_SIGNON_BUFFERS][MAX_DATAGRAM];
	char		demfullmapname[64];

	char		*demolightstyles[MAX_LIGHTSTYLES];
#endif
//====================================================
//	movevars_t	demomovevars;	//FIXME:!
//end this lot... (demo playback)

	int num_static_entities;
	int num_static_sounds;

	svcustomtents_t customtents[255];

	int		*csqcentversion;//prevents ent versions from going backwards
} server_t;
void SV_WipeServerState(void);

/*
#define CS_EMPTY		0
#define CS_ZOMBIE		(1u<<0)	//just stops the slot from being reused for a bit.
#define CS_CLUSTER		(1u<<1)	//is managed by the cluster host (and will appear on scoreboards).
#define CS_SPAWNED		(1u<<2)	//has an active entity.
#define CS_ACTIVE		(1u<<3)	//has a connection

#define cs_free			(CS_EMPTY)
#define cs_zombie		(CS_ZOMBIE)
#define cs_loadzombie	(CS_SPAWNED)
#define cs_connected	(CS_ACTIVE)
#define cs_spawned		(CS_ACTIVE|CS_SPAWNED)
*/

typedef enum
{
	cs_free,		// can be reused for a new connection
	cs_zombie,		// client has been disconnected, but don't reuse connection for a couple seconds. entity was already cleared.
	cs_loadzombie,	// slot reserved for a client. the player's entity may or may not be known (istobeloaded says that state). parms _ARE_ known.
	cs_connected,	// has been assigned to a client_t, but not in game yet
	cs_spawned		// client is fully in game
} client_conn_state_t;

typedef struct
{
	// received from client

	// reply
	double				senttime;		//time we sent this frame to the client, for ping calcs (realtime)
	int					sequence;		//the outgoing sequence - without mask, meaning we know if its current or stale
	float				ping_time;		//how long it took for the client to ack it, may be negative
	float				move_msecs;		//
	int					packetsizein;	//amount of data received for this frame
	int					packetsizeout;	//amount of data that was sent in the frame

	packet_entities_t	qwentities;		//package containing entity states that were sent in this frame, for deltaing

	struct resendinfo_s
	{
		unsigned int entnum;
		unsigned int bits;	//delta (fte or dpp5+)
		quint64_t flags;	//csqc
	} *resend;
	unsigned int		numresend;
	unsigned int		maxresend;

	unsigned short		resendstats[32];//the number of each entity that was sent in this frame
	unsigned int		numresendstats;	//the bits of each entity that were sent in this frame

	short				baseangles[MAX_SPLITS][3];
	unsigned int		baseanglelocked[MAX_SPLITS];

	//antilag
	//these are to recalculate the player's origin without old knockbacks nor teleporters, to give more accurate weapon start positions (post-command).
	vec3_t				pmorigin;
	vec3_t				pmvelocity;
	int					pmtype;
	unsigned int		pmjumpheld:1;
	unsigned int		pmonladder:1;
	float				pmwaterjumptime;
	usercmd_t			cmd;
	//these are old positions of players, to give more accurate victim positions
	laggedentinfo_t		laggedplayer[MAX_CLIENTS];
	unsigned int		numlaggedplayers;
	float				laggedtime;	//sv.time of when this frame was sent
} client_frame_t;

#ifdef Q2SERVER
typedef struct	//merge?
{
	int					areabytes;
	qbyte				areabits[MAX_Q2MAP_AREAS/8];		// portalarea visibility bits
	q2player_state_t	ps[MAX_SPLITS];		//yuck
	int					clientnum[MAX_SPLITS];
	int					num_entities;
	int					first_entity;		// into the circular sv_packet_entities[]
	int					senttime;			// for ping calculations
	float				ping_time;
} q2client_frame_t;
#endif

#define MAX_BACK_BUFFERS 16

enum
{
	PRESPAWN_INVALID=0,
	PRESPAWN_PROTOCOLSWITCH,	//nq drops unreliables until reliables are acked. this gives us a chance to drop any clc_move packets with formats from the previous map
	PRESPAWN_SERVERINFO,
#ifdef MVD_RECORDING
	PRESPAWN_CSPROGS,			//demos contain a copy of the csprogs.
#endif
	PRESPAWN_SOUNDLIST,			//nq skips these
#ifdef HAVE_LEGACY
	PRESPAWN_VWEPMODELLIST,		//qw ugly extension.
#endif
	PRESPAWN_MODELLIST,
	PRESPAWN_NQSIGNON1,		//gotta send all the trickled model+sounds before this is sent.
	PRESPAWN_MAPCHECK,			//wait for old prespawn command
	PRESPAWN_PARTICLES,
	PRESPAWN_CUSTOMTENTS,
	PRESPAWN_SIGNON_BUF,
	PRESPAWN_SPAWNSTATIC,
	PRESPAWN_AMBIENTSOUND,
	PRESPAWN_BASELINES,
	PRESPAWN_SPAWN,
	PRESPAWN_BRUSHES,
	PRESPAWN_COMPLETED
};

enum
{	//'soft' limits that result in warnings if the client's protocol is too limited.
	PLIMIT_ENTITIES = 1u<<0,
	PLIMIT_MODELS = 1u<<1,
	PLIMIT_SOUNDS = 1u<<2
};

//client_t->spec_print + sv_specprint
#define SPECPRINT_CENTERPRINT	0x1
#define SPECPRINT_SPRINT	0x2
#define SPECPRINT_STUFFCMD	0x4

#define STUFFCMD_IGNOREINDEMO (   1<<0) // do not put in mvd demo
#define STUFFCMD_DEMOONLY     (   1<<1) // put in mvd demo only
#define STUFFCMD_BROADCAST    (   1<<2) // everyone sees it.
#define STUFFCMD_UNRELIABLE   (   1<<3) // someone might not see it. oh well.

#define FIXANGLE_NO		0	//don't override anything
#define FIXANGLE_AUTO	1	//guess (initial=fixed, spammed=fixed, sporadic=relative)
#define FIXANGLE_DELTA	2	//send a relative change
#define FIXANGLE_FIXED	3	//send a absolute angle.

enum serverprotocols_e
{
	SCP_BAD,	//don't send (a bot)
	SCP_QUAKEWORLD,
	SCP_QUAKE2,
	SCP_QUAKE2EX,
	SCP_QUAKE3,
	//all the below are considered netquake clients.
	SCP_NETQUAKE,
	//bjp1, bjp2
	SCP_BJP3,		//16bit angles,model+sound indexes. nothing else (assume raised ent limits too).
	SCP_FITZ666,
	//dp5
	SCP_DARKPLACES6,
	SCP_DARKPLACES7	//extra prediction stuff
	//note, nq is nq+
};

typedef struct client_s
{
	client_conn_state_t	state;

	unsigned int	prespawn_stage;
	unsigned int	prespawn_idx;
	unsigned int	prespawn_idx2;
	qboolean		prespawn_allow_modellist;
	qboolean		prespawn_allow_soundlist;

	int				spectator;			// non-interactive
	int				redirect;			//1=redirected because full, 2=cluster transfer

	qboolean		sendinfo;			// at end of frame, send info to all
										// this prevents malicious multiple broadcasts
	float			lastnametime;		// time of last name change
	int				lastnamecount;		// time of last name change
	unsigned		checksum;			// checksum for calcs
	qboolean		drop;				// lose this guy next opportunity
	int				lossage;			// loss percentage

	int				challenge;
	int				userid;				// identifying number
	infobuf_t		userinfo;			// all of the user's various settings
	infosync_t		infosync;			// information about the infos that the client still doesn't know (server and multiple clients).
	char			*transfer;

	unsigned int	baseanglelock;		// lock the player angles to base (until baseangle sequence is acked)
	short			baseangles[3];		// incoming angle inputs are relative to this value.
	usercmd_t		lastcmd;			// for filling in big drops and partial predictions
	double			localtime;			// of last message
	qboolean jump_held;
	unsigned int	lockanglesseq;		//mod is spamming angle changes, don't do relative changes. outgoing sequence. v_angles isn't really known until netchan.incoming_acknowledged>=lockangles

	float			maxspeed;			// localized maxspeed
	float			entgravity;			// localized ent gravity

	int viewent;		//fake the entity positioning.
	int clientcamera;	//cache for dp_sv_clientcamera.

	edict_t			*edict;				// EDICT_NUM(clientnum+1)
//additional game modes use additional edict pointers. this ensures that references are crashes.
#ifdef Q2SERVER
	q2edict_t		*q2edict;				// EDICT_NUM(clientnum+1)
#endif
#ifdef HLSERVER
	struct hledict_s	*hledict;
#endif

	int				playercolor;
	int				playerclass;
	char			teambuf[32];
	char			*team;
	char			*name;
	char			namebuf[32];			// for printing to other people
										// extracted from userinfo
	char			guid[64]; /*+2 for split+pad*/
	int				messagelevel;		// for filtering printed messages
	int				autoaimdot;			//smallest dotproduct allowed for autoaim
#ifdef HAVE_LEGACY
	float			*dp_ping;
	float			*dp_pl;
#endif

	// the datagram is written to after every frame, but only cleared
	// when it is sent out to the client.  overflow is tolerated.
	sizebuf_t		datagram;
	qbyte			datagram_buf[MAX_OVERALLMSGLEN/2];

	// back buffers for client reliable data
	sizebuf_t	backbuf;
	int			num_backbuf;
	int			backbuf_size[MAX_BACK_BUFFERS];
	qbyte		backbuf_data[MAX_BACK_BUFFERS][MAX_BACKBUFLEN];

	double			connection_started;	// or time of disconnect for zombies
	qboolean		send_message;		// set on frames a datagram arived on

	laggedentinfo_t	laggedents[MAX_CLIENTS];
	unsigned int	laggedents_count;
	float			laggedents_frac;
	float			laggedents_time;

// spawn parms are carried from level to level
	float			spawn_parms[NUM_SPAWN_PARMS];
	char			*spawn_parmstring;	//qc-specified data.
	char			*spawninfo;			//entity-formatted data (for hexen2's ClientReEnter)
	float			spawninfotime;
	float			nextservertimeupdate;	//next time to send STAT_TIME
	float			lastoutgoingphysicstime;//sv.world.physicstime of the last outgoing message.

// client known data for deltas
	int				old_frags;

	unsigned int	pendingstats[((MAX_CL_STATS*2) + 31)>>5];	//these are the stats that have changed and that need sending/resending
	int				statsi[MAX_CL_STATS];
	float			statsf[MAX_CL_STATS];
	char			*statss[MAX_CL_STATS];
	char			*centerprintstring;

	struct
	{
		qboolean active;
		char *header;
		double nextsend;	//qex is a one-off, other clients need spam.
		size_t numopts, maxopts, selected;
		struct
		{
			char *text;
			int impulse;
		} *opt;

		int oldmove[2];
	} prompt;

	union{	//save space
		client_frame_t	*frames;	// updates can be deltad from here
#ifdef Q2SERVER
		q2client_frame_t	*q2frames;
#endif
#ifdef Q3SERVER
		void	*q3frames;
#endif
	} frameunion;
	packet_entities_t sentents;
	unsigned int	*pendingdeltabits;
	quint64_t		*pendingcsqcbits;
	unsigned int	nextdeltaindex;			//splurged round-robin to deal with overflows
	unsigned int	nextcsqcindex;			//splurged round-robin
	#define SENDFLAGS_PRESENT 0x1u	//this entity is present on that client
	#define SENDFLAGS_REMOVED 0x2u	//to handle remove packetloss
	#define SENDFLAGS_RESERVED (SENDFLAGS_PRESENT|SENDFLAGS_REMOVED)
	#define SENDFLAGS_SHIFT 2u
	#define SENDFLAGS_USABLE (~(quint64_t)SENDFLAGS_RESERVED)	//this number of bits are actually safe in a float. not all together, but otherwise safe.

#ifdef HAVE_LEGACY
	char			*dlqueue;			//name\name delimited list of files to ask the client to download.
#endif
	char			downloadfn[MAX_QPATH];
	vfsfile_t		*download;			// file being downloaded
	qofs_t			downloadsize;		// total bytes
	qofs_t			downloadcount;		// bytes sent

#ifdef NQPROT
	qofs_t			downloadacked;		//DP-specific
	qofs_t			downloadstarted;	//DP-specific
#endif

	int				spec_track;			// entnum of player tracking

	unsigned int	spec_print;			//bitfield for things this spectator should see that were directed to the player that they're tracking.

#ifdef Q3SERVER
	int	gamestatesequence;	//the sequence number the initial gamestate was sent in.

	int last_client_command_num;
	char last_client_command[1024];

	//quake3 does reliables only via this mechanism. basically just like q1's stuffcmds.
	int server_command_ack;				//number known to have been received.
	int server_command_sequence;		//number available.
	char server_commands[256][1024];		//the commands, to deal with resends
#endif

	//true/false/persist
	unsigned int	penalties;
	qbyte			istobeloaded;	//spawnparms are known.
	qboolean		spawned;		//gamecode knows about it.

	double			floodprotmessage;
	double			lastspoke;
 	double			lockedtill;
	float			joinobservelockeduntil;

	qboolean		upgradewarn;		// did we warn him?

	vfsfile_t		*upload;
	char			uploadfn[MAX_QPATH];
	netadr_t		snap_from;
	qboolean		remote_snap;

//===== NETWORK ============
	int				chokecount;
	qboolean		waschoked;
	int				delta_sequence;		// -1 = no compression
	int				last_sequence;		//last inputframe sequence received
	netchan_t		netchan;
	qboolean		isindependant;

	int				lastsequence_acknowledged;

#ifdef VOICECHAT
	unsigned int voice_read;	/*place in ring*/
	unsigned char voice_mute[(MAX_CLIENTS+7)/8];
	qboolean voice_active;
	enum
	{
		/*note - when recording an mvd, only 'all' will be received by non-spectating viewers. all other chat will only be heard when spectating the receiver(or sender) of said chat*/

		/*should we add one to respond to the last speaker? or should that be an automagic +voip_reply instead?*/
		VT_TEAM,
		VT_ALL,
		VT_NONMUTED,	/*cheap, but allows custom private channels with no external pesters*/
		VT_SPECSELF,	/*sends to self, audiable to people spectating self*/
		VT_PLAYERSLOT0
		/*player0+...*/
	} voice_target;
#endif

#ifdef SVCHAT
	svchat_t chat;
#endif
#ifdef SVRANKING
	int rankid;

	int	kills;
	int	deaths;

	double			stats_started;
#endif

	qboolean		csqcactive;
	qboolean		pextknown;
	unsigned int	fteprotocolextensions;
	unsigned int	fteprotocolextensions2;
	unsigned int	ezprotocolextensions1;
	unsigned int	zquake_extensions;
	unsigned int	max_net_ents; /*highest entity number the client can receive (limited by either protocol or client's buffer size)*/
	unsigned int	max_net_staticents; /*limit to the number of static ents supported by the client*/
	unsigned int	max_net_clients; /*max number of player slots supported by the client */
	unsigned int	maxmodels; /*max models supported by whatever the protocol is*/

	enum serverprotocols_e protocol;
	unsigned int	supportedprotocols;
	qboolean proquake_angles_hack;	//expect 16bit client->server angles .
	qboolean qex;	//qex sends strange clc inputs and needs workarounds for its prediction. it also favours fitzquake's protocol but violates parts of it.

	unsigned int lastruncmd;	//for non-qw physics. timestamp they were last run, so switching between physics modes isn't a (significant) cheat
	unsigned int hoverms;	//purely for sv_showpredloss to avoid excessive spam
//speed cheat testing
#define NEWSPEEDCHEATPROT
	float msecs;
#ifndef NEWSPEEDCHEATPROT
	int msec_cheating;
	float last_check;
#endif

	qboolean gibfilter;

	int trustlevel;

	vec3_t	specorigin;	//mvds need to use a different origin from the one QC has.
	vec3_t	specvelocity;

	int language;	//the clients language

//	struct {
//		qbyte vweap;
//	} otherclientsknown[MAX_CLIENTS];	//updated as needed. Flag at a time, or all flags.

	struct client_s *controller;	/*first in splitscreen chain, NULL=nosplitscreen*/
	struct client_s *controlled;	/*next in splitscreen chain*/
	qbyte seat;

	/*these are the current rates*/
	float ratetime;
	float inrate;
	float outrate;

	int rate;
	int drate;

	netadr_t realip;
	int realip_status;
	int realip_num;
	int realip_ping;
	char *reversedns;

	unsigned int plimitwarned;

	float delay;
	laggedpacket_t *laggedpacket;
	laggedpacket_t *laggedpacket_last;

	size_t lastseen_count;
	float *lastseen_time;	//timer for cullentities_trace, so we can get away with fewer traces per test

#ifdef VM_Q1
	int hideentity;
	qboolean hideplayers;
#endif
} client_t;

#if defined(NQPROT) || defined(Q2SERVER) || defined(Q3SERVER)
#define ISQWCLIENT(cl) ((cl)->protocol == SCP_QUAKEWORLD)
#define ISQ2CLIENT(cl) ((cl)->protocol >= SCP_QUAKE2 && (cl)->protocol <= SCP_QUAKE2EX)
#define ISQ3CLIENT(cl) ((cl)->protocol == SCP_QUAKE3)
#define ISNQCLIENT(cl) ((cl)->protocol >= SCP_NETQUAKE)
#define ISDPCLIENT(cl) ((cl)->protocol >= SCP_DARKPLACES6)
#else
#define ISQWCLIENT(cl) ((cl)->protocol != SCP_BAD)
#define ISQ2CLIENT(cl) false
#define ISQ3CLIENT(cl) false
#define ISNQCLIENT(cl) false
#define ISDPCLIENT(cl) false
#endif

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer




//=============================================================================

//mvd stuff
#ifdef MVD_RECORDING
#define	MSG_BUF_SIZE 8192

typedef struct
{
	vec3_t	origin;
	vec3_t	angles;
	int		weaponframe;
	int		skinnum;
	int		model;
	int		effects;
}	demoinfo_t;

typedef struct
{
	demoinfo_t	info;
	float		sec;
	int			parsecount;
	qboolean	fixangle;
	vec3_t		angle;
	float		cmdtime;
	int			flags;
	int			frame;
} demo_client_t;

typedef struct {
	qbyte type;
	qbyte full;
	int to;
	int size;
	qbyte data[1]; //gcc doesn't allow [] (?)
} mvd_header_t;

typedef struct
{
	sizebuf_t sb;
	int		bufsize;
	mvd_header_t *h;
} demobuf_t;

typedef struct
{
	demo_client_t clients[MAX_CLIENTS];
	double		time;
	demobuf_t	buf;
} demo_frame_t;

typedef struct {
	qbyte	*data;
	int		start, end, last;
	int		maxsize;
} dbuffer_t;

#define DEMO_FRAMES 64	//why is this not just 2?
#define DEMO_FRAMES_MASK (DEMO_FRAMES - 1)

typedef struct
{
//	demobuf_t	*dbuf;
//	dbuffer_t	dbuffer;
	sizebuf_t	datagram;
	qbyte		datagram_data[MSG_BUF_SIZE];
	int			lastto;
	int			lasttype;
	double		time, pingtime;
	int			statsi[MAX_CLIENTS][MAX_CL_STATS]; // ouch!
	float		statsf[MAX_CLIENTS][MAX_CL_STATS]; // ouch!
	char		*statss[MAX_CLIENTS][MAX_CL_STATS]; // ouch!
	client_t	recorder;
	qboolean	playerreset[MAX_CLIENTS];	//will ensure that the model etc is written when this player is next written.
	qboolean	fixangle[MAX_CLIENTS];
	float		fixangletime[MAX_CLIENTS];
	vec3_t		angles[MAX_CLIENTS];
	qboolean	resetdeltas;
	int			parsecount;
	int			lastwritten;
	demo_frame_t	frames[DEMO_FRAMES];
	demoinfo_t	info[MAX_CLIENTS];
	qbyte		buffer[20*MAX_QWMSGLEN];
	int			bufsize;
	int			forceFrame;

	struct mvddest_s *dest;
} demo_t;
#endif

//=============================================================================

#define SVSTATS_PERIOD 10
typedef struct
{
	double	active;
	double	idle;
	int		count;
	int		packets;
	double	maxresponse;	//longest (active) frame time within the current period
	int		maxpackets;		//max packet count in a single frame

	double	latched_time;	//time that the current period ends
	double	latched_active;	//active time in the last period
	double	latched_idle;
	int		latched_count;
	int		latched_packets;
	int		latched_maxpackets;
	double	latched_maxresponse;
} svstats_t;

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	1024

typedef struct
{
	netadr_t	adr;
	int			challenge;
	int			time;
} challenge_t;

typedef struct bannedips_s {
	unsigned int banflags;
	struct bannedips_s *next;
	netadr_t	adr;
	netadr_t	adrmask;
	time_t expiretime;
	char reason[1];
} bannedips_t;

typedef enum {
	GT_PROGS,	//q1, qw, h2 are similar enough that we consider it only one game mode. (We don't support the h2 protocol)
	GT_Q1QVM,
	GT_HALFLIFE,
	GT_QUAKE2,	//q2 servers run from a q2 game dll
	GT_QUAKE3,	//q3 servers run off the q3 qvm api
#ifdef VM_LUA
	GT_LUA,		//for the luls
#endif
	GT_MAX
} gametype_e;

typedef struct levelcache_s {
	struct levelcache_s *next;
	char *mapname;
	gametype_e gametype;
	unsigned char savedplayers[(MAX_CLIENTS+7)>>3];	//bitmask to say which players are actually stored in the cache. so that restarts can restore.
} levelcache_t;

#ifdef TCPCONNECT
typedef struct svtcpstream_s {
	int socketnum;
	int inlen;
	qboolean waitingforprotocolconfirmation;
	char inbuffer[1500];
	float timeouttime;
	netadr_t remoteaddr;
	struct svtcpstream_s *next;
} svtcpstream_t;
#endif

typedef struct server_static_s
{
	gametype_e	gametype;
	int			spawncount;			// number of servers spawned since start,
									// used to check late spawns
	int framenum;	//physics frame number for out-of-sequence thinks (fix for slow rockets)
	int clusterserverid;			// which server we are in the cluster. for gamecode to track with stats.

	struct ftenet_connections_s *sockets;

	int			allocated_client_slots;	//number of entries in the clients array
	client_t	*clients;			//[svs.allocated_client_slots]
	int			serverflags;		// episode completion information

	double		last_heartbeat;
	int			heartbeat_sequence;
	svstats_t	stats;

	infobuf_t	info;
	infobuf_t	localinfo;

	// log messages are used so that fraglog processes can get stats
	int			logsequence;	// the message currently being filled
	double		logtime;		// time of last swap

#define FRAGLOG_BUFFERS	8
	sizebuf_t	log[FRAGLOG_BUFFERS];
	qbyte		log_buf[FRAGLOG_BUFFERS][MAX_DATAGRAM];

	challenge_t	challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting

	bannedips_t *bannedips;

	char progsnames[MAX_PROGS][MAX_QPATH];
	progsnum_t progsnum[MAX_PROGS];
	int numprogs;

	struct netprim_s netprim;

	laggedpacket_t *free_lagged_packet;
	packet_entities_t entstatebuffer; /*just a temp buffer*/

	char		name[64];			// map name (base filename). static because of restart command after disconnecting.
	levelcache_t *levcache;

	struct frameendtasks_s
	{
		struct frameendtasks_s *next;
		void(*callback)(struct frameendtasks_s *task);
		void *ctxptr;
		intptr_t ctxint;
		char data[1];
	} *frameendtasks;
} server_static_t;

//=============================================================================

/*
// edict->movetype values
#define	MOVETYPE_NONE			0		// never moves
#define	MOVETYPE_ANGLENOCLIP	1
#define	MOVETYPE_ANGLECLIP		2
#define	MOVETYPE_WALK			3		// gravity
#define	MOVETYPE_STEP			4		// gravity, special edge handling
#define	MOVETYPE_FLY			5
#define	MOVETYPE_TOSS			6		// gravity
#define	MOVETYPE_PUSH			7		// no clip to world, push and crush
#define	MOVETYPE_NOCLIP			8
#define	MOVETYPE_FLYMISSILE		9		// extra size to monsters
#define	MOVETYPE_BOUNCE			10
#define MOVETYPE_BOUNCEMISSILE	11		// bounce w/o gravity
#define MOVETYPE_FOLLOW			12		// track movement of aiment
#define MOVETYPE_H2PUSHPULL		13		// pushable/pullable object
#define MOVETYPE_H2SWIM			14		// should keep the object in water
#define MOVETYPE_PHYSICS		32
#define MOVETYPE_FLY_WORLDONLY	33

// edict->solid values
#define	SOLID_NOT				0		// no interaction with other objects
#define	SOLID_TRIGGER			1		// touch on edge, but not blocking
#define	SOLID_BBOX				2		// touch on edge, block
#define	SOLID_SLIDEBOX			3		// touch on edge, but not an onground
#define	SOLID_BSP				4		// bsp clip, touch on edge, block
#define	SOLID_PHASEH2			5
#define	SOLID_CORPSE			5
#define SOLID_LADDER			20		//dmw. touch on edge, not blocking. Touching players have different physics. Otherwise a SOLID_TRIGGER. deprecated. use solid_bsp and skin=-16

#define	DAMAGE_NO				0
#define	DAMAGE_YES				1
#define	DAMAGE_AIM				2
*/

#define PVSF_NORMALPVS		0x0
#define PVSF_NOTRACECHECK	0x1
#define PVSF_USEPHS			0x2
#define PVSF_IGNOREPVS		0x3
#define PVSF_MODE_MASK		0x3
#define PVSF_NOREMOVE		0x80

// entity effects

//define	EF_BRIGHTFIELD			1
//define	EF_MUZZLEFLASH 			2
//#define	EF_BRIGHTLIGHT 			(1<<2)
//#define	EF_DIMLIGHT 			(1<<4)

//#define	EF_FULLBRIGHT			512


#define	SPAWNFLAG_NOT_EASY			(1<<8)
#define	SPAWNFLAG_NOT_MEDIUM		(1<<9)
#define	SPAWNFLAG_NOT_HARD			(1<<10)
#define	SPAWNFLAG_NOT_DEATHMATCH	(1<<11)

#define SPAWNFLAG_NOT_H2PALADIN			(1<<8)
#define SPAWNFLAG_NOT_H2CLERIC			(1<<9)
#define SPAWNFLAG_NOT_H2NECROMANCER		(1<<10)
#define SPAWNFLAG_NOT_H2THEIF			(1<<11)
#define	SPAWNFLAG_NOT_H2EASY			(1<<12)
#define	SPAWNFLAG_NOT_H2MEDIUM			(1<<13)
#define	SPAWNFLAG_NOT_H2HARD		    (1<<14)
#define	SPAWNFLAG_NOT_H2DEATHMATCH		(1<<15)
#define SPAWNFLAG_NOT_H2COOP			(1<<16)
#define SPAWNFLAG_NOT_H2SINGLE			(1<<17)

#if 0//ndef Q2SERVER
typedef enum multicast_e
{
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,
	MULTICAST_PVS_R
} multicast_t;
#endif


//shared with qc
#define MSG_PRERELONE	-100
#define	MSG_BROADCAST	0		// unreliable to all
#define	MSG_ONE			1		// reliable to one (msg_entity)
#define	MSG_ALL			2		// reliable to all
#define	MSG_INIT		3		// write to the init string
#define	MSG_MULTICAST	4		// for multicast()
#define MSG_CSQC		5		// for writing csqc entities

//============================================================================

extern	cvar_t	sv_mintic, sv_maxtic, sv_limittics;
extern	cvar_t	sv_maxspeed;
extern	cvar_t	sv_antilag;
extern	cvar_t	sv_antilag_frac;
extern	cvar_t	sv_nqplayerphysics;

void SV_Master_ReResolve(void);
void SV_Master_Shutdown(void);
void SV_Master_Heartbeat (void);
qboolean SV_Master_AddressIsMaster(netadr_t *adr);
void SV_Master_HeartbeatResponse(netadr_t *adr, const char *challenge);
extern	cvar_t	sv_antilag_frac;

extern	cvar_t	pr_ssqc_progs;
extern	cvar_t	sv_csqcdebug;
extern	cvar_t	spawn;
extern	cvar_t	teamplay;
extern	cvar_t	deathmatch;
extern	cvar_t	coop;
extern	cvar_t	fraglimit;
extern	cvar_t	timelimit;

extern	server_static_t	svs;				// persistant server info
extern	server_t		sv;					// local server

extern	client_t	*host_client;

extern	edict_t		*sv_player;

//extern	char		localmodels[MAX_MODELS][5];	// inline model names for precache

extern	vfsfile_t	*sv_fraglogfile;

//===========================================================

void SV_AddDebugPolygons(void);
const char *SV_CheckRejectConnection(netadr_t *adr, const char *uinfo, unsigned int protocol, unsigned int pext1, unsigned int pext2, unsigned int ezpext1, char *guid);

//
//sv_ccmds.c
//
char *SV_BannedReason (netadr_t *a);
void SV_EvaluatePenalties(client_t *cl);
void SV_AutoAddPenalty (client_t *cl, unsigned int banflag, int duration, char *reason);
void SV_AutoBanSender (int duration, char *reason);	//bans net_from

//note: not all penalties are actually penalties, but they can still expire.
#define BAN_BAN			(1u<<0)	//user is banned from the server
#define	BAN_PERMIT		(1u<<1)	//+user can evade block bans or filterban
#define	BAN_CUFF		(1u<<2)	//can't shoot/use impulses
#define	BAN_MUTE		(1u<<3)	//can't use say/say_team/voip
#define	BAN_CRIPPLED	(1u<<4)	//can't move
#define	BAN_DEAF		(1u<<5)	//can't see say/say_team
#define	BAN_LAGGED		(1u<<6)	//given an extra 200ms
#define BAN_VIP			(1u<<7)	//+mods might give the user special rights, via the *VIP infokey. the engine itself currently does not do anything but track it.
#define BAN_BLIND		(1u<<8)	//player's pvs is wiped.
#define BAN_SPECONLY	(1u<<9) //player is forced to spectate
#define BAN_STEALTH		(1u<<10)//player is not told of their bans
#define BAN_USER1		(1u<<11)//mod-specified
#define BAN_USER2		(1u<<12)//mod-specified
#define BAN_USER3		(1u<<13)//mod-specified
#define BAN_USER4		(1u<<14)//mod-specified
#define BAN_USER5		(1u<<15)//mod-specified
#define BAN_USER6		(1u<<16)//mod-specified
#define BAN_USER7		(1u<<17)//mod-specified
#define BAN_USER8		(1u<<18)//mod-specified
#define BAN_MAPPER		(1u<<19)//+player is allowed to use the brush/entity editing clc.
#define BAN_VMUTE		(1u<<20)//can't use voip (but can still use other forms of chat)

#define BAN_NOLOCALHOST	(BAN_BAN|BAN_PERMIT|BAN_SPECONLY)	//any attempt to ban localhost is denied, but you can vip, lag, cripple, etc them.

//
// sv_main.c
//
NORETURN void VARGS SV_Error (const char *error, ...) LIKEPRINTF(1);
void SV_Shutdown (void);
float SV_Frame (void);
void SV_ReadPacket(void);
void SV_FinalMessage (char *message);
void SV_DropClient (client_t *drop);
void SV_DropClient_ByAddress (netadr_t *addr);
struct quakeparms_s;
void SV_Init (struct quakeparms_s *parms);
void SV_ExecInitialConfigs(char *defaultexec);
void SV_ArgumentOverrides(void);

int SV_CalcPing (client_t *cl, qboolean forcecalc);
void SV_FullClientUpdate (client_t *client, client_t *to);
char *SV_PlayerPublicAddress(client_t *cl);

void SV_GeneratePublicServerinfo(char *info, const char *endinfo);
const char *SV_GetProtocolVersionString(void);	//decorate the protocol version field of server queries with extra features...
qboolean SVC_GetChallenge (qboolean respond_dp);
int SV_NewChallenge (void);
void SVC_DirectConnect(int expectedreliablesequence);
typedef struct
{
	enum serverprotocols_e protocol;		//protocol used to talk to this client.
#ifdef NQPROT
	qboolean proquakeanglehack;				//specifies that the client will expect proquake angles if we give a proquake CCREP_ACCEPT response.
	qboolean isqex;							//yay quirks...
	unsigned int expectedreliablesequence;	//required for nq connection cookies (like tcp's syn cookies).
	unsigned int supportedprotocols;		//1<<SCP_* bitmask
#endif
	unsigned int ftepext1;
	unsigned int ftepext2;
	unsigned int ezpext1;
	int			qport;						//part of the qw protocol to avoid issues with buggy routers that periodically renumber cl2sv ports.
#ifdef HUFFNETWORK
	int			huffcrc;					//network compression stuff
#endif
	int			challenge;					//the challenge used at connect. remembered to make life harder for proxies.
	int			mtu;						//allowed fragment size (also signifies that it supports fragmented qw packets)
	int seats;
	struct	{
		char		info[2048];				//random userinfo data. no blobs, obviously.
	} seat[MAX_SPLITS];
	char		guid[128];					//user's guid data
	netadr_t	adr;						//the address the connect request came from (so we can check passwords before accepting)
} svconnectinfo_t;
void SV_DoDirectConnect(svconnectinfo_t *fte_restrict info);

int SV_ModelIndex (const char *name);

void SV_WriteClientdataToMessage (client_t *client, sizebuf_t *msg);
void SVQW_WriteDelta (entity_state_t *from, entity_state_t *to, sizebuf_t *msg, qboolean force, unsigned int protext, unsigned int ezext);

client_t *SV_AddSplit(client_t *controller, char *info, int id);
void SV_SpawnParmsToQC(client_t *client);
void SV_SpawnParmsToClient(client_t *client);
void SV_GetNewSpawnParms(client_t *cl);
void SV_SaveSpawnparms (void);
void SV_SaveSpawnparmsClient(client_t *client, float *transferparms);	//if transferparms, calls SetTransferParms instead, and does not modify the player.
void SV_SaveLevelCache(const char *savename, qboolean dontharmgame);
void SV_Savegame (const char *savename, qboolean autosave);
qboolean SV_LoadLevelCache(const char *savename, const char *level, const char *startspot, qboolean ignoreplayers);

void SV_Physics_Client (edict_t	*ent, int num);

void SV_ExecuteUserCommand (const char *s, qboolean fromQC);
void SV_InitOperatorCommands (void);

void SV_SendServerinfo (client_t *client);
void SV_ExtractFromUserinfo (client_t *cl, qboolean verbose);

void SV_SaveInfos(vfsfile_t *f);

void SV_FixupName(const char *in, char *out, unsigned int outlen);

#ifdef SUBSERVERS
//cluster stuff
void SSV_UpdateAddresses(void);
void SSV_InitiatePlayerTransfer(client_t *cl, const char *newserver);
void SSV_InstructMaster(sizebuf_t *cmd);
void SSV_CheckFromMaster(void);
qboolean SSV_PrintToMaster(char *s);
void SSV_ReadFromControlServer(void);
void SSV_SavePlayerStats(client_t *cl, int reason);	//initial, periodic (in case of node crashes), part
void SSV_RequestShutdown(void); //asks the cluster to not send us new players

vfsfile_t *Sys_ForkServer(void);
vfsfile_t *Sys_GetStdInOutStream(void);		//obtains a bi-directional pipe for reading/writing via stdin/stdout. make sure the system code won't be using it.

qboolean MSV_NewNetworkedNode(vfsfile_t *stream, qbyte *reqstart, qbyte *buffered, size_t buffersize, const char *remoteaddr);	//call to register a pipe to a newly discovered node.
void SSV_SetupControlPipe(vfsfile_t *stream, qboolean remote);	//call to register the pipe.
enum clusterslavemode_e
{
	CLSV_no,
	CLSV_forked,
	CLSV_remote
};
extern enum clusterslavemode_e isClusterSlave;
#define SSV_IsSubServer() isClusterSlave


void MSV_SubServerCommand_f(void);
void MSV_MapCluster_f(void);
void MSV_MapCluster_Setup(const char *landingmap, qboolean use_database, qboolean singleplyaer);
void MSV_Shutdown(void);
void SSV_Send(const char *dest, const char *src, const char *cmd, const char *msg);
qboolean MSV_ClusterLogin(svconnectinfo_t *info);
void MSV_PollSlaves(void);
qboolean MSV_ForwardToAutoServer(void);	//forwards console command to a default subserver. ie: whichever one our client is on.
void MSV_SendCvarChange(cvar_t *var);	//when autooffloading, replicates the client's cvar changes to the server
void MSV_Status(void);
void MSV_OpenUserDatabase(void);
#else
#define SSV_UpdateAddresses() ((void)0)
#define MSV_ClusterLogin(info) false
#define SSV_IsSubServer() false
#define MSV_OpenUserDatabase()
#define MSV_PollSlaves()
#define MSV_ForwardToAutoServer() false
#define MSV_SendCvarChange(v)
#endif

//
// sv_init.c
//
void SV_SpawnServer (const char *server, const char *startspot, qboolean noents, qboolean usecinematic, int playerslots);
void SV_UnspawnServer (void);
void SV_FlushSignon (qboolean force);
void SV_UpdateMaxPlayers(int newmax);

void SV_FilterImpulseInit(void);
qboolean SV_FilterImpulse(int imp, int level);

//svq2_game.c
qboolean SVQ2_InitGameProgs(void);
void VARGS SVQ2_ShutdownGameProgs (void);
void VARGS PFQ2_Configstring (int i, const char *val); //for engine cheats.

//svq2_ents.c
void SVQ2_BuildClientFrame (client_t *client);
void SVQ2_WriteFrameToClient (client_t *client, sizebuf_t *msg);
#ifdef Q2SERVER
void MSGQ2_WriteDeltaEntity (q2entity_state_t *from, q2entity_state_t *to, sizebuf_t *msg, qboolean force, qboolean newentity, qboolean q2ex);
void SVQ2_BuildBaselines(void);
#endif

//
// sv_send.c
//
void SV_CalcNetRates(client_t *cl, double *ftime, int *frames, double *minf, double *maxf);	//gets received framerate etc info
qboolean SV_ChallengePasses(int challenge);
void SV_QCStatName(int type, char *name, int statnum);
void SV_QCStatFieldIdx(int type, unsigned int fieldindex, int statnum);
void SV_QCStatGlobal(int type, const char *globalname, int statnum);
void SV_QCStatPtr(int type, void *ptr, int statnum);
void SV_ClearQCStats(void);

void SV_SendClientMessages (void);

void SV_SendLightstyle(client_t *cl, sizebuf_t *forcemsg, int style, qboolean initial);
void VARGS SV_Multicast (vec3_t origin, multicast_t to);
#define FULLDIMENSIONMASK 0xffffffff
void SV_MulticastProtExt(vec3_t origin, multicast_t to, int dimension_mask, int with, int without);
void SV_MulticastCB(vec3_t origin, multicast_t to, const char *reliableinfokey, int dimension_mask, void (*callback)(client_t *cl, sizebuf_t *msg, void *ctx), void *ctx);

void SV_StartSound (int ent, vec3_t origin, float *velocity, int seenmask, int channel, const char *sample, int volume, float attenuation, float pitchadj, float timeofs, unsigned int flags);
void QDECL SVQ1_StartSound (vec_t *origin, wedict_t *entity, int channel, const char *sample, int volume, float attenuation, float pitchadj, float timeofs, unsigned int chflags);
void SV_PrintToClient(client_t *cl, int level, const char *string);
void SV_TPrintToClient(client_t *cl, int level, const char *string);
void SV_StuffcmdToClient(client_t *cl, const char *string);
void SV_StuffcmdToClient_Unreliable(client_t *cl, const char *string);
void VARGS SV_ClientPrintf (client_t *cl, int level, const char *fmt, ...) LIKEPRINTF(3);
void VARGS SV_ClientTPrintf (client_t *cl, int level, translation_t text, ...);
void VARGS SV_BroadcastPrintf (int level, const char *fmt, ...) LIKEPRINTF(2);
void SV_BroadcastPrint_QexLoc (unsigned int flags, int level, const char **arg, int args);
void SV_BroadcastPrint (unsigned int flags, int level, const char *text);
	//flags exposed to ktx.
	#define BPRINT_IGNOREINDEMO  (1<<0) // broad cast print will be not put in demo
	#define BPRINT_IGNORECLIENTS (1<<1) // broad cast print will not be seen by clients, but may be seen in demo
	//#define BPRINT_QTVONLY       (1<<2) // if broad cast print goes to demo, then it will be only qtv sream, but not file
	#define BPRINT_IGNORECONSOLE (1<<3) // broad cast print will not be put in server console
void VARGS SV_BroadcastTPrintf (int level, translation_t fmt, ...);
void VARGS SV_BroadcastCommand (const char *fmt, ...) LIKEPRINTF(1);
void SV_SendMessagesToAll (void);
void SV_FindModelNumbers (void);
void SV_SendFixAngle(client_t *client, sizebuf_t *msg, int fixtype, qboolean roll);

void SV_BroadcastUserinfoChange(client_t *about, qboolean isbasic, const char *key, const char *newval);

void SV_Prompt_Resend(client_t *cl);
void SV_Prompt_Clear(client_t *cl);
void SV_Prompt_Input(client_t *cl, usercmd_t *ucmd);

//
// sv_user.c
//
#ifdef NQPROT
void SVNQ_New_f (void);
void SVNQ_ExecuteClientMessage (client_t *cl);
#endif
qboolean SV_UserInfoIsBasic(const char *infoname);	//standard message.
void SV_ExecuteClientMessage (client_t *cl);
void SVQ2_ExecuteClientMessage (client_t *cl);
int SV_PMTypeForClient (client_t *cl, edict_t *ent);
void SV_UserInit (void);
qboolean SV_TogglePause (client_t *cl);

#ifdef PEXT2_VOICECHAT
void SV_VoiceInitClient(client_t *client);
void SV_VoiceSendPacket(client_t *client, sizebuf_t *buf);
#endif

void SV_SetSSQCInputs(usercmd_t *ucmd);
void SV_ClientThink (void);
void SV_Begin_Core(client_t *split);	//sets up the player's gamecode state
void SV_DespawnClient(client_t *cl);	//shuts down the gamecode state.

void VoteFlushAll(void);
void SV_SetUpClientEdict (client_t *cl, edict_t *ent);
void SV_UpdateToReliableMessages (void);
void SV_FlushBroadcasts (void);
qboolean SV_CanTrack(client_t *client, int entity);

#ifdef HAVE_LEGACY
void SV_DownloadQueueNext(client_t *client);
void SV_DownloadQueueClear(client_t *client);
#endif
#ifdef NQPROT
void SV_DarkPlacesDownloadChunk(client_t *cl, sizebuf_t *msg);
#endif
void SV_New_f (void);

void SV_PreRunCmd(void);
void SV_RunCmd (usercmd_t *ucmd, qboolean recurse);
void SV_PostRunCmd(void);
void SV_RunCmdCleanup(void);

void SV_SendClientPrespawnInfo(client_t *client);
void SV_ClientProtocolExtensionsChanged(client_t *client);

//sv_master.c
float SVM_Think(void);
vfsfile_t *SVM_GenerateIndex(const char *requesthost, const char *fname, const char **mimetype, const char *query);
void SVM_AddBrokerGame(const char *brokerid, const char *info);
void SVM_RemoveBrokerGame(const char *brokerid);
qboolean SVM_FixupServerAddress(netadr_t *adr, struct dtlspeercred_s *cred);
void SVM_SelectRelay(netadr_t *benefitiary, const char *brokerid, char *out, size_t outsize);
void FTENET_TCP_ICEResponse(struct ftenet_connections_s *col, int type, const char *cid, const char *sdp);


//
// svonly.c
//
typedef enum {RD_NONE, RD_CLIENT, RD_PACKET, RD_PACKET_LOG, RD_OBLIVION, RD_MASTER} redirect_t;	//oblivion is provided so people can read the output before the buffer is wiped.
void SV_BeginRedirect (redirect_t rd, int lang);
void SV_EndRedirect (void);
extern char	sv_redirected_buf[8000];
extern redirect_t	sv_redirected;
extern int sv_redirectedlang;


qboolean PR_GameCodePacket(char *s);
qboolean PR_GameCodePausedTic(float pausedtime);
qboolean PR_ShouldTogglePause(client_t *initiator, qboolean pausedornot);

//
// sv_ents.c
//
void SV_WriteEntitiesToClient (client_t *client, sizebuf_t *msg, qboolean ignorepvs);
void SVFTE_EmitBaseline(entity_state_t *to, qboolean numberisimportant, sizebuf_t *msg, unsigned int pext2, unsigned int ezext);
void SVQ3Q1_BuildEntityPacket(client_t *client, packet_entities_t *pack);
void SV_Snapshot_BuildStateQ1(entity_state_t *state, edict_t *ent, client_t *client, packet_entities_t *pack);
int SV_HullNumForPlayer(int h2hull, float *mins, float *maxs);
void SV_GibFilterInit(void);
void SV_GibFilterPurge(void);
void SV_CleanupEnts(void);
void SV_ProcessSendFlags(client_t *c);

void SV_AckEntityFrame(client_t *cl, int framenum);
void SV_ReplaceEntityFrame(client_t *cl, int framenum);

//
// sv_nchan.c
//

void ClientReliableCheckBlock(client_t *cl, int maxsize);
sizebuf_t *ClientReliable_StartWrite(client_t *cl, int maxsize);	//MUST be followed by a call to ClientReliable_FinishWrite before the next start
void ClientReliable_FinishWrite(client_t *cl);
void ClientReliableWrite_Begin(client_t *cl, int c, int maxsize);
client_t *ClientReliableWrite_BeginSplit(client_t *cl, int svc, int svclen);
void ClientReliableWrite_Angle(client_t *cl, float f);
void ClientReliableWrite_Angle16(client_t *cl, float f);
void ClientReliableWrite_Byte(client_t *cl, int c);
void ClientReliableWrite_Char(client_t *cl, int c);
void ClientReliableWrite_Float(client_t *cl, float f);
void ClientReliableWrite_Double(client_t *cl, double f);
void ClientReliableWrite_Coord(client_t *cl, float f);
void ClientReliableWrite_Int64(client_t *cl, qint64_t c);
void ClientReliableWrite_Long(client_t *cl, int c);
void ClientReliableWrite_Short(client_t *cl, int c);
void ClientReliableWrite_Entity(client_t *cl, int c);
void ClientReliableWrite_String(client_t *cl, const char *s);
void ClientReliableWrite_SZ(client_t *cl, const void *data, int len);


#ifdef  SVRANKING
//flags
#define RANK_MUTED		2
#define RANK_CUFFED		4
#define RANK_CRIPPLED	8	//ha ha... get speed cheaters with this!... :o)

#define NUM_RANK_SPAWN_PARMS 32

typedef struct {	//stats info
	int kills;
	int deaths;
	float parm[NUM_RANK_SPAWN_PARMS];
	float timeonserver;
	qbyte flags1;
	qbyte trustlevel;
	char pad2;
	char pad3;

#if NUM_RANK_SPAWN_PARMS>32
	quint64_t created;
	quint64_t lastseen;
#endif
} rankstats_t;

typedef struct {	//name, identity and order.
	int prev;		//score is held for convineance.
	int next;
	char name[32];
	int pwd;
	float score;
} rankheader_t;

typedef struct {
	rankheader_t h;
	rankstats_t s;
} rankinfo_t;

int Rank_GetPlayerID(char *guid, const char *name, int pwd, qboolean allowcreate, qboolean requirepasswordtobeset);
void Rank_SetPlayerStats(int id, rankstats_t *stats);
rankstats_t *Rank_GetPlayerStats(int id, rankstats_t *buffer);
rankinfo_t *Rank_GetPlayerInfo(int id, rankinfo_t *buffer);
qboolean Rank_OpenRankings(void);
void Rank_Flush (void);

void Rank_ListTop10_f (void);
void Rank_RegisterCommands(void);
int Rank_GetPass (char *name);

extern cvar_t rank_needlogin;
qboolean ReloadRanking(client_t *cl, const char *newname);
#endif

client_t *SV_GetClientForString(const char *name, int *id);
qboolean    SV_MayCheat(void);







void NPP_Flush(void);
void NPP_NQWriteByte(int dest, qbyte data);
void NPP_NQWriteChar(int dest, char data);
void NPP_NQWriteShort(int dest, short data);
void NPP_NQWriteLong(int dest, long data);
void NPP_NQWriteAngle(int dest, float data);
void NPP_NQWriteCoord(int dest, float data);
void NPP_NQWriteFloat(int dest, float data);
void NPP_NQWriteString(int dest, const char *data);
void NPP_NQWriteEntity(int dest, int data);

void NPP_QWWriteByte(int dest, qbyte data);
void NPP_QWWriteChar(int dest, char data);
void NPP_QWWriteShort(int dest, short data);
void NPP_QWWriteLong(int dest, long data);
void NPP_QWWriteAngle(int dest, float data);
void NPP_QWWriteCoord(int dest, float data);
void NPP_QWWriteFloat(int dest, float data);
void NPP_QWWriteString(int dest, const char *data);
void NPP_QWWriteEntity(int dest, int data);



void NPP_MVDForceFlush(void);


//replacement rand function (useful cos it's fully portable, with seed grabbing)
void predictablesrand(unsigned int x);
int predictablerandgetseed(void);
int predictablerand(void);







#ifdef SVCHAT
void SV_WipeChat(client_t *client);
int SV_ChatMove(int impulse);
void SV_ChatThink(client_t *client);
#endif


#ifdef MVD_RECORDING
/*
//
// sv_mvd.c
//
//qtv proxies are meant to send a small header now, bit like http
//this header gives supported version numbers and stuff
typedef struct mvdpendingdest_s {
	qboolean error;	//disables writers, quit ASAP.
#ifdef _WIN32
	qintptr_t socket;
#else
	int socket;
#endif

	char inbuffer[2048];
	char outbuffer[2048];

	char challenge[64];
	qboolean hasauthed;
	qboolean isreverse;

	int insize;
	int outsize;

	struct mvdpendingdest_s *nextdest;
} mvdpendingdest_t;*/

typedef struct mvddest_s {
	qboolean error;	//disables writers, quit ASAP.
	qboolean droponmapchange;

	enum {DEST_NONE, DEST_FILE, DEST_BUFFEREDFILE, DEST_THREADEDFILE, DEST_STREAM} desttype;

	vfsfile_t *file;

	int id;
	char filename[MAX_QPATH];	//demos/foo.mvd (or a username)
	char simplename[MAX_QPATH];	//foo.mvd (or a qtv resource)

	int flushing;	//worker has a cache (used as a sync point)
	char *cache;
	char *altcache;
	int cacheused;
	int maxcachesize;

	unsigned int totalsize;

	struct mvddest_s *nextdest;
} mvddest_t;
void SV_MVDPings (void);
void SV_MVD_FullClientUpdate(sizebuf_t *msg, client_t *player);
sizebuf_t *MVDWrite_Begin(qbyte type, int to, int size);
void MVDSetMsgBuf(demobuf_t *prev,demobuf_t *cur);

enum mvdclosereason_e
{
	MVD_CLOSE_STOPPED,
	MVD_CLOSE_SIZELIMIT,
	MVD_CLOSE_CANCEL,
	MVD_CLOSE_DISCONNECTED,	//qtv disconnected
	MVD_CLOSE_FSERROR
};

void SV_MVDStop (enum mvdclosereason_e reason, qboolean mvdonly);
void SV_MVDStop_f (void);
qboolean SV_MVDWritePackets (int num);
void MVD_Init (void);
void SV_MVD_RunPendingConnections(void);
void SV_MVD_SendInitialGamestate(mvddest_t *dest);

extern demo_t			demo;				// server demo struct

extern cvar_t	sv_demoDir, sv_demoDirAlt;
extern cvar_t	sv_demoAutoRecord;
extern cvar_t	sv_demofps;
extern cvar_t	sv_demoPings;
extern cvar_t	sv_demoUseCache;
extern cvar_t	sv_demoMaxSize;
extern cvar_t	sv_demoMaxDirSize;

qboolean MVD_CheckSpace(qboolean broadcastwarnings);
void SV_MVD_AutoRecord (void);
char *SV_Demo_CurrentOutput(void);
void SV_Demo_PrintOutputs(void);
void SV_MVDInit(void);
char *SV_MVDNum(char *buffer, int bufferlen, int num);
const char *SV_MVDLastNum(unsigned int num);
void SV_SendMVDMessage(void);
void SV_MVD_WriteReliables(qboolean writebroadcasts);
qboolean SV_ReadMVD (void);
void SV_FlushDemoSignon (void);
void DestFlush(qboolean compleate);

typedef struct
{
	qboolean hasauthed;
	qboolean isreverse;
	char challenge[64];	//aka nonce
} qtvpendingstate_t;
int SV_MVD_GotQTVRequest(vfsfile_t *clientstream, char *headerstart, char *headerend, qtvpendingstate_t *p);
#endif

// savegame.c
void SV_Savegame_f (void);
void SV_DeleteSavegame_f (void);
void SV_Savegame_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx);
void SV_Loadgame_f (void);
qboolean SV_Loadgame (const char *unsafe_savename);
void SV_AutoSave(void);
void SV_FlushLevelCache(void);
extern cvar_t sv_autosave;
extern cvar_t sv_savefmt;


int SV_RateForClient(client_t *cl);

void SVVC_Frame (qboolean enabled);
void SV_CalcPHS (void);

void SV_GetConsoleCommands (void);
void SV_CheckTimer(void);

void SV_LogPlayer(client_t *cl, char *msg);

extern vec3_t pmove_mins, pmove_maxs;	//abs min/max extents
#ifdef USEAREAGRID
void AddAllLinksToPmove (world_t *w, wedict_t *player);
#else
void AddLinksToPmove (world_t *w, wedict_t *player, areanode_t *node);
void AddLinksToPmove_Force (world_t *w, wedict_t *player, areanode_t *node);
#define AddAllLinksToPmove(w,p) do{AddLinksToPmove(w,p,(w)->areanodes);AddLinksToPmove_Force(w,p,&(w)->portallist);}while(0)
#endif


#ifdef HLSERVER
void SVHL_SaveLevelCache(char *filename);

//network frame info
void SVHL_Snapshot_Build(client_t *client, packet_entities_t *pack, qbyte *pvs, edict_t *clent, qboolean ignorepvs);
qbyte	*SVHL_Snapshot_SetupPVS(client_t *client, qbyte *pvs, unsigned int pvsbufsize);
void SVHL_BuildStats(client_t *client, int *si, float *sf, char **ss);

//gamecode entry points
int SVHL_InitGame(void);
void SVHL_SetupGame(void);
void SVHL_SpawnEntities(char *entstring);
void SVHL_RunFrame (void);
qboolean SVHL_ClientConnect(client_t *client, netadr_t adr, char rejectmessage[128]);
void SVHL_PutClientInServer(client_t *client);
void SVHL_RunPlayerCommand(client_t *cl, usercmd_t *oldest, usercmd_t *oldcmd, usercmd_t *newcmd);
qboolean HLSV_ClientCommand(client_t *client);
void SVHL_DropClient(client_t *drop);
void SVHL_ShutdownGame(void);
#endif

