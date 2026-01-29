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
// client.h

#include "../common/particles.h"

enum
{
	SKIN_NOTLOADED,	//not trying to load it. shouldn't really happen, but can.
	SKIN_LOADING,	//still loading. just do something else for now...
	SKIN_LOADED,
	SKIN_FAILED
};
typedef struct qwskin_s
{
	char		name[64];
	int			loadstate;		// the name isn't a valid skin

	//qw skin info
	int			width;
	int			height;
	void		*skindata;

	//for hardware 32bit texture overrides
	texnums_t	textures;
} qwskin_t;

// player_state_t is the information needed by a player entity
// to do move prediction and to generate a drawable entity
typedef struct
{
	int			messagenum;		// all player's won't be updated each frame

	double		state_time;		// not the same as the packet time,
								// because player commands come asyncronously

	usercmd_t	command;		// last command for prediction

	vec3_t		origin;
	vec3_t		predorigin;		// pre-predicted pos for other players (allowing us to just lerp when active)
	vec3_t		viewangles;		// only for demos, not from server
	vec3_t		velocity;
	int			weaponframe;

	unsigned int			modelindex;
	int			frame;
	int			skinnum;
	int			effects;

#ifdef PEXT_SCALE
	float scale;
#endif
	qbyte colourmod[3];
	qbyte alpha;
#ifdef PEXT_FATNESS
	float fatness;
#endif

	int			flags;			// dead, gib, etc

	int			pm_type;

	vec3_t		szmins, szmaxs;

	//maybe-propagated... use the networked value if available.
	float		waterjumptime;	//never networked...
	qboolean	onground;	//networked with Z_EXT_PF_ONGROUND||replacementdeltas
	qboolean	jump_held;	//networked with Z_EXT_PM_TYPE
	int			jump_msec;	// hack for fixing bunny-hop flickering on non-ZQuake servers
	vec3_t		gravitydir;	//networked with replacementdeltas
} player_state_t;


#if defined(Q2CLIENT) || defined(Q2SERVER)
typedef enum
{
	// can accelerate and turn
	Q2PM_NORMAL,
	Q2PM_SPECTATOR,
	// no acceleration or turning
	Q2PM_DEAD,
	Q2PM_GIB,		// different bounding box
	Q2PM_FREEZE
} q2pmtype_t;
enum
{	//urgh.
	// can accelerate and turn
	Q2EPM_NORMAL,
	Q2EPM_GRAPPLE,
	Q2EPM_SPECTATOR,
	Q2EPM_SPECTATOR2,
	// no acceleration or turning
	Q2EPM_DEAD,
	Q2EPM_GIB,		// different bounding box
	Q2EPM_FREEZE
};
typedef struct
{	//shared with q2 dll

	q2pmtype_t	pm_type;

	short		origin[3];		// 13.3
	short		velocity[3];	// 13.3
	qbyte		pm_flags;		// ducked, jump_held, etc
	qbyte		pm_time;		// each unit = 8 ms
	short		gravity;
	short		delta_angles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters
//	short		pad;
} q2pmove_state_t;

typedef struct
{	//shared with q2 dll so cannot be changed

	q2pmove_state_t	pmove;		// for prediction

	// these fields do not need to be communicated bit-precise

	vec3_t		viewangles;		// for fixed views
	vec3_t		viewoffset;		// add to pmovestate->origin
	vec3_t		kick_angles;	// add to view direction to get render angles
								// set by weapon kicks, pain effects, etc

	vec3_t		gunangles;
	vec3_t		gunoffset;
	int			gunindex;
	int			gunframe;

	float		blend[4];		// rgba full screen effect

	float		fov;			// horizontal field of view

	int			rdflags;		// refdef flags

	short		stats[Q2MAX_STATS];		// fast status bar updates
} q2player_state_t;
#endif

typedef struct colourised_s {
	char name[64];
	unsigned int topcolour;
	unsigned int bottomcolour;
	char skin[64];
	struct colourised_s *next;
} colourised_t;

#define	MAX_SCOREBOARDNAME	64
#define MAX_DISPLAYEDNAME	16
typedef struct player_info_s
{
	int			userid;
	infobuf_t	userinfo;
	qboolean	userinfovalid;	//set if we actually know the userinfo (ie: false on vanilla nq servers)
	char		teamstatus[128];
	float		teamstatustime;

	// scoreboard information
	int		spectator;
	char	name[MAX_SCOREBOARDNAME];
	char	team[MAX_INFO_KEY];
	float	realentertime;	//pegged against realtime, to cope with potentially not knowing the server's time when we first receive this message
	int		frags;
	int		ping;
	qbyte	pl;
	char	ruleset[19]; //has colour markup to say if its accepted or not

	char	ip[128];

	struct
	{
		float time;	//invalid if too old.
		int health;
		int armour;
		unsigned int items;
		vec3_t org;
		char nick[8];	//kinda short, yes.
	} tinfo;

	qboolean ignored;
	qboolean vignored;
	unsigned int chatstate;

	// skin information
	unsigned int		rtopcolor;	//real, according to their userinfo
	unsigned int		rbottomcolor;

#ifdef QWSKINS
	colourised_t *colourised;

	qwskin_t	*qwskin;
	qwskin_t	*lastskin;	//last-known-good skin

	unsigned int		ttopcolor;	//team, according to colour forcing
	unsigned int		tbottomcolor;

	struct model_s	*model;
	#define dtopcolor ttopcolor
	#define dbottomcolor tbottomcolor
#else
	#define dtopcolor rtopcolor
	#define dbottomcolor rbottomcolor
#endif
	skinid_t	skinid;

//	unsigned short vweapindex;
#ifdef HEXEN2
	unsigned char h2playerclass;
#endif

	int prevcount;

#ifdef QUAKEHUD	
	struct wstats_s 
	{
		char wname[16];
		unsigned int hit;
		unsigned int total;
	} weaponstats[16];
#endif

	int stats[MAX_CL_STATS];
	float statsf[MAX_CL_STATS];
} player_info_t;


typedef struct
{
	double		senttime;			// time cmd was sent off
	float		latency;			// the time the packet was acked. -1=choked, -2=dropped, -3=never sent, -4=reply came back invalid

	// generated on client side
	usercmd_t	cmd[MAX_SPLITS];	// cmd that generated the frame
	int			cmd_sequence;		//the outgoing move sequence. if not equal to expected, that index was stale and is no longer valid
	int			server_message_num;	//the inbound frame that was valid when this command was generated

	int server_time;
	int client_time;
	float sentgametime;	//nq timings are based upon server time echos.
} outframe_t;

typedef struct
{
	// received from server
	int			frameid;		//the sequence number of the frame, so we can easily detect which frames are valid without poking all in advance, etc
	int			ackframe;		//the outgoing sequence this frame acked (for prediction backlerping).
	double		receivedtime;	// time message was received, or -1
	player_state_t	playerstate[MAX_CLIENTS+MAX_SPLITS];	// message received that reflects performing
								// the usercmd
	packet_entities_t	packet_entities;
	qboolean	invalid;		// true if the packet_entities delta was invalid
} inframe_t;

#ifdef Q2CLIENT
typedef struct
{
	qboolean		valid;			// cleared if delta parsing was invalid
	int				serverframe;
	int				servertime;		// server time the message is valid for (in msec)
	int				deltaframe;
	struct {
		qbyte				areabits[MAX_Q2MAP_AREAS/8];		// portalarea visibility bits
		q2player_state_t	playerstate;
		int					clientnum;
	} seat[MAX_SPLITS];
	int				num_entities;
	int				parse_entities;	// non-masked index into cl_parse_entities array
} q2frame_t;
#endif

typedef struct
{
	int		destcolor[3];
	float		percent;		// 0-256
} cshift_t;

#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define CSHIFT_SERVER	4
#define	NUM_CSHIFTS		5


//
// client_state_t should hold all pieces of the client state
//
//the light array works thusly:
//dlights are allocated DL_LAST downwards to 0, static wlights are allocated DL_LAST+1 to MAX_RTLIGHTS.
//thus to clear the dlights but not rtlights, set the first light to RTL_FIRST
#define dlightbitmask_t		size_t
#define DL_LAST				(sizeof(dlightbitmask_t)*8-1)
#define RTL_FIRST			(sizeof(dlightbitmask_t)*8)

#define LFLAG_NORMALMODE	(1<<0) /*ppl with r_shadow_realtime_dlight*/
#define LFLAG_REALTIMEMODE	(1<<1) /*ppl with r_shadow_realtime_world*/
#define LFLAG_LIGHTMAP		(1<<2)
#define LFLAG_FLASHBLEND	(1<<3)

#define LFLAG_NOSHADOWS		(1<<8)
#define LFLAG_SHADOWMAP		(1<<9)
#define LFLAG_CREPUSCULAR	(1<<10)	//weird type of sun light that gives god rays
#define LFLAG_ORTHO			(1<<11)	//sun-style -light
#define LFLAG_FORCECACHE	(1<<12)	//shadowmap/surfaces should be cached from one frame to the next.

#define LFLAG_INTERNAL		(LFLAG_LIGHTMAP|LFLAG_FLASHBLEND|LFLAG_FORCECACHE)	//these are internal to FTE, and never written to disk (ie: .rtlights files shouldn't contain these)
#define LFLAG_DYNAMIC (LFLAG_LIGHTMAP | LFLAG_FLASHBLEND | LFLAG_NORMALMODE)

typedef struct dlight_s
{
	int		key;				// so entities can reuse same entry
	vec3_t	origin;
	vec3_t	axis[3];
	vec3_t	angles;				//used only for reflection, to avoid things getting rounded/cycled.
	vec3_t	rotation;			//cubemap/spotlight rotation
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	float   color[3];
	float	channelfade[3];
#ifdef RTLIGHTS
	vec3_t lightcolourscales; //ambient, diffuse, specular
#endif
	float	corona;
	float	coronascale;
	float	fade[2];

	unsigned int flags;
	char	cubemapname[MAX_QPATH];
	char	*customstyle;

	int coronaocclusionquery;
	unsigned int coronaocclusionresult;

	//the following are used for rendering (client code should clear on create)
	qboolean rebuildcache;
	struct	shadowmesh_s *worldshadowmesh;
	texid_t cubetexture;
	struct {
		float updatetime;
	} face [6];
	int style;	//multiply by style values if >= 0 && < MAX_LIGHTSTYLES
	float	fov; //spotlight
	float	nearclip; //for spotlights...
	struct dlight_s *next;
} dlight_t;

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
	vec3_t	colours;
	int		colourkey;	//compacted version of the colours, for comparison against caches
} lightstyle_t;

#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

typedef enum {
ca_disconnected, 	// full screen console with no connection
ca_demostart,		// waiting to start up a demo (still disconnected but there should be a playdemo command in the cbuf somewhere so don't do other stuff)
ca_connected,		// netchan_t established, waiting for svc_serverdata
ca_onserver,		// processing data lists, donwloading, etc
ca_active			// everything is in, so frames can be rendered
} cactive_t;

typedef enum {
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_wad,
	dl_single,
	dl_singlestuffed
} dltype_t;		// download type

typedef struct qdownload_s
{
	enum {DL_QW, DL_QWCHUNKS, DL_Q3, DL_DARKPLACES, DL_QWPENDING, DL_HTTP, DL_FTP} method;
	vfsfile_t		*file;		// file transfer from server
	char			dclname[MAX_OSPATH];	//file to read/write the chunklist from, for download resumption.
	char			tempname[MAX_OSPATH];	//file its currently writing to.
	char			localname[MAX_OSPATH];	//file its going to be renamed to.
	int				prefixbytes;			//number of bytes that prefix the above names (ie: package/ or nothing).
	char			remotename[MAX_OSPATH];	//file its coming from.
	float			percent;				//for progress indicator.
	float			starttime;				//for speed info
	qofs_t			completedbytes;			//number of bytes downloaded, for progress/speed info
	qofs_t			size;					//total size (may be a guess)
	qboolean		sizeunknown;			//says that size is a guess
	unsigned int	filesequence;			//unique file id.
	enum fs_relative fsroot;				//where the local+temp file is meant to be relative to.

	double			ratetime;				//periodically updated
	int				rate;					//ratebytes/ratetimedelta
	int				ratebytes;				//updated by download reception code, and cleared when ratetime is bumped
	unsigned int	flags;

	//chunked downloads uses this
	struct dlblock_s
	{
		qofs_t start;
		qofs_t end;
		enum
		{
			DLB_MISSING,
			DLB_PENDING,
			DLB_RECEIVED
		} state:16;
		unsigned int sequence;	//sequence is only valid on pending blocks.

		struct dlblock_s *next;
	} *dlblocks;
} qdownload_t;
enum qdlabort
{
	QDL_FAILED,		//delete file, tell server.
	QDL_DISCONNECT,	//delete file, don't tell server.
	QDL_COMPLETED,	//rename file, tell server.
};
qboolean DL_Begun(qdownload_t *dl);
void DL_Abort(qdownload_t *dl, enum qdlabort aborttype);		//just frees the download's resources. does not delete the temp file.
qboolean CL_AllowArbitaryDownload(const char *oldname, const char *localfile);


//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
// connection information
	cactive_t	state;

	/*Specifies which protocol family we're speaking*/
	enum {
		CP_UNKNOWN,
		CP_QUAKEWORLD,
		CP_NETQUAKE,
		CP_QUAKE2,
		CP_QUAKE3,
		CP_PLUGIN
	} protocol;

	/*QuakeWorld protocol flags*/
#ifdef PROTOCOLEXTENSIONS
	unsigned int fteprotocolextensions;
	unsigned int fteprotocolextensions2;
	unsigned int ezprotocolextensions1;
#endif
	unsigned int z_ext;

	/*NQ Protocol flags*/
	enum
	{
		CPNQ_ID,
		CPNQ_NEHAHRA,
		CPNQ_BJP1,	//16bit models, strict 8bit sounds (otherwise based on nehahra)
		CPNQ_BJP2,	//16bit models, strict 16bit sounds
		CPNQ_BJP3,	//16bit models, flagged 16bit sounds, 8bit static sounds.
		CPNQ_H2MP,	//urgh
		CPNQ_FITZ666, /*and rmqe999 protocol, which is a strict superset*/
		CPNQ_DP5,
		CPNQ_DP6,
		CPNQ_DP7
	} protocol_nq;
	#define CPNQ_IS_DP (cls.protocol_nq >= CPNQ_DP5)
	#define CPNQ_IS_BJP (cls.protocol_nq >= CPNQ_BJP1 && cls.protocol_nq <= CPNQ_BJP3)
	qboolean proquake_angles_hack;	//angles are always 16bit
#ifdef NQPROT
	qboolean qex;	//we're connected to a QuakeEx server, which means lots of special workarounds that are not controlled via the actual protocol version.
#endif

	int protocol_q2;
	char downloadurl[MAX_OSPATH];	//where to download files from (for q2pro compat)

	qboolean findtrack;

	int framecount;

	int realip_ident;
	netadr_t realserverip;

// network stuff
	netchan_t	netchan;
	float lastarbiatarypackettime;	//used to mark when packets were sent to prevent mvdsv servers from causing us to disconnect.

	infobuf_t	userinfo[MAX_SPLITS];
	infosync_t	userinfosync;

	char		serverurl[MAX_OSPATH*4];	// eg qw://foo:27500/join?fp=blah
	char		servername[MAX_OSPATH];		// internal parsing, eg dtls://foo:27500

	struct ftenet_connections_s *sockets;

	qdownload_t *download;

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	vfsfile_t	*demooutfile;

	enum{DPB_NONE,DPB_QUAKEWORLD,DPB_MVD,
#ifdef NQPROT
		DPB_NETQUAKE,
#endif
#ifdef Q2CLIENT
		DPB_QUAKE2
#endif
	}	demoplayback, demorecording;
	unsigned int demoeztv_ext;
		#define EZTV_DOWNLOAD		(1u<<0)	//also changes modellist/soundlist stuff to keep things synced
		#define EZTV_SETINFO		(1u<<1)	//proxy wants setinfo + ptrack commands
		#define EZTV_QTVUSERLIST	(1u<<2)	//'//qul cmd id [name]' commands from proxy.
	qboolean	demohadkeyframe;	//q2 needs to wait for a packet with a key frame, supposedly.
	enum
	{
		DEMOSEEK_NOT,
		DEMOSEEK_TIME,	//stops one we reach demoseektime
		DEMOSEEK_MARK,	//stops once we reach a '//demomark'
		DEMOSEEK_INTERMISSION,	//stops once we reach an svc_intermission
	} demoseeking;
	float		demoseektime;
	int			demotrack;
	qboolean	timedemo;
	char		lastdemoname[MAX_OSPATH];	//empty if is a qtv stream
	qboolean	lastdemowassystempath;
	vfsfile_t	*demoinfile;
	float		td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo
	float		demostarttime;		// the time of the first frame, so we don't get weird results with qw demos

	struct qtvviewers_s
	{	//(other) people on a qtv. in case people give a damn.
		struct qtvviewers_s *next;
		int			userid;
		char		name[128];
	} *qtvviewers;

	int			challenge;

	float		latency;		// rolling average

	char		allow_unmaskedskyboxes;	//skyboxes/domes do not need to be depth-masked when set. FIXME: we treat this as an optimisation hint, but some hl/q2/q3 maps require strict do-not-mask rules to look right.
	qboolean	allow_anyparticles;
	qboolean	allow_watervis;	//fixme: not checked any more
	float		allow_fbskins;	//fraction of allowance
	qboolean	allow_cheats;
	qboolean	allow_semicheats;	//defaults to true, but this allows a server to enforce a strict ruleset (smackdown type rules).
	qboolean	allow_csqc;			//disables some legacy/compat things, like proquake parsing.
	float		maxfps;	//server capped
	int			deathmatch;

#ifdef NQPROT
	int signon;
#endif

	colourised_t *colourised;
	qboolean	nqexpectingstatusresponse;
} client_static_t;

extern client_static_t	cls;

enum dlfailreason_e
{
	DLFAIL_UNTRIED,		//...
	DLFAIL_UNSUPPORTED,	//eg vanilla nq
	DLFAIL_CORRUPTED,	//something weird happened (hash fail)
	DLFAIL_CLIENTCVAR,	//clientside cvar blocked the download
	DLFAIL_CLIENTFILE,	//some sort of error writing the file
	DLFAIL_SERVERCVAR,	//serverside setting blocked the download
	DLFAIL_REDIRECTED,	//server told us to download a different file
	DLFAIL_SERVERFILE,	//server couldn't find the file
};
typedef struct downloadlist_s {
	char rname[128];
	char localname[128];
	unsigned int size;
	unsigned int flags;
#define DLLF_VERBOSE		(1u<<0)		//tell the user that its downloading
#define DLLF_REQUIRED		(1u<<1)		//means that it won't load models etc until its downloaded (ie: requiredownloads 0 makes no difference)
#define DLLF_OVERWRITE		(1u<<2)		//overwrite it even if it already exists
#define DLLF_SIZEUNKNOWN	(1u<<3)		//download's size isn't known
#define DLLF_IGNOREFAILED	(1u<<4)		//
#define DLLF_NONGAME		(1u<<5)		//means the requested download filename+localname is gamedir explicit (so id1/foo.txt is distinct from qw/foo.txt)
#define DLLF_TEMPORARY		(1u<<6)		//download it, but don't actually save it (DLLF_OVERWRITE doesn't actually overwrite, but does ignore any local files)
#define DLLF_USEREXPLICIT	(1u<<7)		//use explicitly requested it, ignore the cl_downloads cvar.

#define DLLF_BEGUN			(1u<<8)		//server has confirmed that the file exists, is readable, and we've opened a file. should not be set on new requests.
#define DLLF_ALLOWWEB		(1u<<9)		//failed http downloads should retry but from the game server itself
#define DLLF_TRYWEB			(1u<<10)	//should be trying to download it from a website...

	enum dlfailreason_e failreason;
	struct downloadlist_s *next;
} downloadlist_t;


typedef struct {
	//current persistant state
	trailkey_t trailstate;	//when to next throw out a trail
	trailkey_t emitstate;    //when to next emit

	//current origin
	vec3_t origin;	//current render position
	vec3_t angles;

	//previous rendering frame (for trails)
	vec3_t lastorigin;
	qboolean isnew;
	qboolean isplayer;

	//intermediate values for frame lerping
	//separate upper+lower lerps
	float framelerpdeltatime[FS_COUNT];
	float newframestarttime[FS_COUNT];
	int newframe[FS_COUNT];
	float oldframestarttime[FS_COUNT];
	int oldframe[FS_COUNT];
	qbyte basebone;

	//intermediate values for origin lerping of stepping things
	int newsequence;
	float orglerpdeltatime;
	float orglerpstarttime;
	vec3_t neworigin; /*origin that we're lerping towards*/
	vec3_t oldorigin; /*origin that we're lerping away from*/
	vec3_t newangle;
	vec3_t oldangle;

	//for further info
	int skeletalobject;
	int sequence;	/*so we know if the ent is still valid*/
	entity_state_t *entstate;
} lerpents_t;

enum
{
	FOGTYPE_AIR,
	FOGTYPE_WATER,
	FOGTYPE_SKYROOM,

	FOGTYPE_COUNT
};

//state associated with each player 'seat' (one for each splitscreen client)
//note that this doesn't include networking inputlog info.
struct playerview_s
{
	int			playernum;		//cl.players index for this player.
	qboolean	nolocalplayer;	//inhibit use of qw-style players, predict based on entities.
	qboolean	spectator;
#ifdef PEXT_SETVIEW
	int			viewentity;		//view is attached to this entity.
#endif

	// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	float		statsf[MAX_CL_STATS];	// health, etc
	char		*statsstr[MAX_CL_STATS];	// health, etc
	float		item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;		// use anim frame if cl.time < this

#ifdef QUAKEHUD
	qboolean	sb_showscores;
	qboolean	sb_showteamscores;
#ifdef HEXEN2
	int			sb_hexen2_cur_item;//hexen2 hud
	float		sb_hexen2_item_time;
	float		sb_hexen2_extra_info_lines;
	qboolean	sb_hexen2_extra_info;//show the extra stuff
	qboolean	sb_hexen2_infoplaque;
#endif
#endif


// the client maintains its own idea of view angles, which are
// sent to the server each frame.  And only reset at level change
// and teleport times
	vec3_t		aimangles;			//angles actually being sent to the server (different due to in_vraim)
	vec3_t		viewangles;			//current angles
	vec3_t		viewanglechange;	//angles set by input code this frame
	short		baseangles[3];		//networked angles are relative to this value
	vec3_t		intermissionangles;	//absolute angles for intermission
	vec3_t		gravitydir;

	// pitch drifting vars
	float		pitchvel;
	qboolean	nodrift;
	float		driftmove;
	double		laststop;

	int gamerectknown;		//equals cls.framecount if valid
	vrect_t	gamerect;		//position the player's main view was drawn at this frame.

	//prediction state
	int			pmovetype;
	float		entgravity;
	float		maxspeed;
	vec3_t		simorg;
	vec3_t		simvel;
	vec3_t		simangles;
	float		rollangle;
	float		hdr_last;

	int			chatstate;	//1=talking, 2=afk

	float		crouch;			// local amount for smoothing stepups
	vec3_t		oldorigin;		// to track step smoothing
	float		oldz, extracrouch, crouchspeed; // to track step smoothing
	qboolean	onground;
	float		viewheight;
	int			waterlevel;		//for smartjump

	//for values that are propagated from one frame to the next
	//the next frame will always predict from the one we're tracking, where possible.
	//these values should all regenerate naturally from networked state (misses should be small/rare and fade when the physics catches up).
	struct playerpredprop_s
	{
		float		waterjumptime;
		qboolean	onground;
		vec3_t		gravitydir;
		qboolean	jump_held;
		int			jump_msec;		// hack for fixing bunny-hop flickering on non-ZQuake servers

		int			sequence;
	} prop;

#ifdef Q2CLIENT
	float forcefov;
	int handedness;	//0=right,1=left,2=center/hidden
	vec3_t predicted_origin;
	vec3_t predicted_angles;
	vec3_t prediction_error;
	float predicted_step_time;
	float predicted_step;
#endif

	//temporary view kick from weapon firing, angles+origins
	float		punchangle_cl;		// qw-style angles
	vec3_t		punchangle_sv;		// nq-style
	vec3_t		punchorigin;		// nq-style

	float		v_dmg_time;		//various view knockbacks.
	float		v_dmg_roll;
	float		v_dmg_pitch;

	double		bobtime;		//sine wave
	double		bobcltime;		//for tracking time increments
	float		bob;			//bob height


	vec3_t		cam_desired_position;	// where the camera wants to be
	int			cam_oldbuttons;			//
	double		cam_lastviewtime;		// timer for wallcam
	float		cam_reautotrack;		// timer to throttle tracking changes.
	int			cam_spec_track;			// player# of who we are tracking / want to track / might want to track
	enum
	{
		CAM_FREECAM	= 0,		//not attached to another player. we are our own thing (or actually playing).
		CAM_PENDING = 1,		//we want to lock on to cam_spec_track, but we don't have their position / stats yet. still freecamming
		CAM_WALLCAM = 2,		//locked, cl_chasecam=0. we're watching them from a wall.
		CAM_EYECAM	= 3			//locked, cl_chasecam=1. we know where they are, we're in their eyes.

#define CAM_ISLOCKED(pv) ((pv)->cam_state > CAM_PENDING)
	} cam_state;

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups and content types
	vec4_t		screentint;
	vec4_t		bordertint;	//won't contain v_cshift values, only powerup+contents+damage+bf flashes

//	entity_t	viewent;	// is this not utterly redundant yet?
	struct
	{
		struct model_s *oldmodel;
		float lerptime;
		float oldlerptime;
		float frameduration;
		int prevframe;
		int oldframe;
	} vm;

	struct
	{
		qboolean defaulted;
		int entnum;
		vec3_t origin;
		vec3_t forward;
		vec3_t right;
		vec3_t up;
		size_t reverbtype;
		vec3_t velocity;
	} audio;

	struct vrdevinfo_s vrdev[VRDEV_COUNT];
};

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int			fpd;
	int			servercount;	// server identification for prespawns

	int			protocol_qw;

	float		gamespeed;
	qboolean	csqcdebug;	//redundant, remove '*csqcdebug' serverinfo key.
	qboolean	allowsendpacket;

	qboolean	stillloading;	// set when doing something slow, and the game is still loading.

	qboolean	haveserverinfo;	//nq servers will usually be false. don't override stuff if we already know better.
	infobuf_t	serverinfo;
	char		*serverpacknames;
	char		*serverpackhashes;
	qboolean	serverpakschanged;

	int			parsecount;		// server message counter
	int			oldparsecount;
	int			oldvalidsequence;
	int			ackedmovesequence;	//in quakeworld/q2 this is always equal to validsequence. nq can differ. may only be updated when validsequence is updated.
	int			lastackedmovesequence;	//can be higher than ackedmovesequence when the received frame was unusable.
	int			validsequence;	// this is the sequence number of the last good
								// packetentity_t we got.  If this is 0, we can't
								// render a frame yet
	int			movesequence;	// client->server frames
	float		movesequence_time;	// client->server frame timestamp (vs cl.time)

//	int			spectator;
	int			autotrack_hint;		//the latest hint from the mod, might be negative for invalid.
	int			autotrack_killer;	//if someone kills the guy we're tracking, this is the guy we should switch to.

	double		last_ping_request;	// while showing scoreboard
	double		last_servermessage;

	//list of ent frames that still need to be acked.
	unsigned int numackframes;
	int ackframes[64];

#ifdef Q2CLIENT
	q2frame_t	q2frame;
	q2frame_t	q2frames[Q2UPDATE_BACKUP];
#endif

// sentcmds[cl.netchan.outgoing_sequence & UPDATE_MASK] = cmd
	outframe_t	outframes[UPDATE_BACKUP];	//user inputs (cl.ackedmovesequence+1 to cl.movesequence are still pending)
	inframe_t	inframes[UPDATE_BACKUP];	//server state (cl.validsequence is the most recent set)
	lerpents_t	*lerpents;
	int			maxlerpents;	//number of slots allocated.
	int			lerpentssequence;
	lerpents_t	lerpplayers[MAX_CLIENTS];

	//when running splitscreen, we have multiple viewports all active at once
	unsigned int	splitclients;	//we are running this many clients split screen.
	playerview_t	playerview[MAX_SPLITS];
	unsigned int	defaultnetsplit;//which multiview splitscreen to parse the message for (set by mvd playback code)

	// localized movement vars
	float		bunnyspeedcap;

// the client simulates or interpolates movement to get these values
	double		time;			// this is the time value that the client
								// is rendering at.  always <= realtime
	double		lasttime;		//cl.time from last frame.
	double		lastlinktime;	//cl.time from last frame.
	double		mapstarttime;	//for computing csqc's cltime.

	double servertime;	//current server time, bound between gametime and gametimemark
	float mtime;		//server time as on the server when we last received a packet. not allowed to decrease.
	float oldmtime;		//server time as on the server for the previously received packet.

	double gametime;
	double gametimemark;
	double oldgametime;		//used as the old time to lerp cl.time from.
	double oldgametimemark;	//if it's 0, cl.time will casually increase.
	float demogametimebias;	//mvd timings are weird.
	int	  demonudge;		//
	float demopausedtilltime;//demo is paused until realtime>this

	float minpitch;
	float maxpitch;

	qboolean	paused;			// send over by server
	qboolean	implicitpause;	//for csqc. only a hint, respected only in singleplayer.

	enum
	{
		IM_NONE,		//off.
		IM_NQSCORES,	//+showscores forced, view still attached to regular view
		IM_NQFINALE,	//slow centerprint text etc, view still attached to regular view. no hud
		IM_NQCUTSCENE,	//IM_NQFINALE, but without the 'finale' header on centerprints.
		IM_H2FINALE,	//IM_NQFINALE, but with the view offset by the player's viewheight.

		IM_QWSCORES		//intermission, view locked at a specific point
	} intermissionmode;	// don't change view angle, full screen, etc
	float		completed_time;	// latched ffrom time at intermission start

//
// information that is static for the entire time connected to a server
//
#ifdef HAVE_LEGACY
	char				*model_name_vwep[MAX_VWEP_MODELS];
	struct model_s		*model_precache_vwep[MAX_VWEP_MODELS];
#endif
	char				*model_name[MAX_PRECACHE_MODELS];
	struct model_s		*model_precache[MAX_PRECACHE_MODELS];
	char				*sound_name[MAX_PRECACHE_SOUNDS];
	struct sfx_s		*sound_precache[MAX_PRECACHE_SOUNDS];
	char				*particle_ssname[MAX_SSPARTICLESPRE];
	int					particle_ssprecache[MAX_SSPARTICLESPRE];	//these are actually 1-based, so 0 can be used to lazy-init them. I cheat.

#ifdef Q2CLIENT
#define Q2MAX_VISIBLE_WEAPONS 32 //q2 has about 20.
	int		numq2visibleweapons;	//q2 sends out visible-on-model weapons in a wierd gender-nutral way.
	char	*q2visibleweapons[Q2MAX_VISIBLE_WEAPONS];//model names beginning with a # are considered 'sexed', and are loaded on a per-client basis. yay. :(

	char		*configstring_general[Q2MAX_CLIENTS|Q2MAX_GENERAL];
	char		*image_name[Q2MAX_IMAGES];
	char		*item_name[Q2MAX_ITEMS];
	short		inventory[MAX_SPLITS][Q2MAX_ITEMS];
#endif

	char				model_csqcname[MAX_CSMODELS][MAX_QPATH];
	struct model_s		*model_csqcprecache[MAX_CSMODELS];
	char				*particle_csname[MAX_CSPARTICLESPRE];
	int					particle_csprecache[MAX_CSPARTICLESPRE];	//these are actually 1-based, so we can be lazy and do a simple negate.

	qboolean			particle_ssprecaches;	//says to not try to do any dp-compat hacks.
	qboolean			particle_csprecaches;	//says to not try to do any dp-compat hacks.

	//used for q2 sky/configstrings
	char skyname[MAX_QPATH];
	float skyrotate;
	qboolean skyautorotate;
	vec3_t skyaxis;

	qboolean	fog_locked;			//FIXME: make bitmask
	fogstate_t	fog[FOGTYPE_COUNT];	//0 = air, 1 = water. if water has no density fall back on air.
	fogstate_t	oldfog[FOGTYPE_COUNT];

	char		levelname[40];	// for display on solo scoreboard
	char		*windowtitle;	// fully overrides the window caption.

// refresh related state
	struct model_s	*worldmodel;	// cl_entitites[0].model
	int			num_entities;	// stored bottom up in cl_entities array
	int			num_statics;	// stored top down in cl_entitiers

// all player information
	unsigned int    allocated_client_slots;
	player_info_t	players[MAX_CLIENTS];


	downloadlist_t *downloadlist;
	downloadlist_t *faileddownloads;

	qboolean gamedirchanged;

#ifdef Q2CLIENT
	char		q2airaccel[16];
	char		q2statusbar[1024];
	char		q2layout[MAX_SPLITS][1024];
	int			q2mapchecksum;
	int parse_entities;
	float lerpfrac;
	float q2svnetrate; //number of frames we expect to receive per second (required to calculate the server time correctly).
#endif

	char lastcenterprint[1024];	//prevents too much spam with console centerprint logging.



	struct itemtimer_s
	{
		float end;
		int entnum;
		float start;
		float duration;
		vec3_t origin;
		vec3_t rgb;
		float radius;
		struct itemtimer_s *next;
	} *itemtimers;

	//interpolation+snapshots
	float	packfrac;
	packet_entities_t	*currentpackentities;
	packet_entities_t	*previouspackentities;
	float				currentpacktime;
	qboolean			do_lerp_players;

	playerview_t		*mouseplayerview;	//for mouse/scoreboard interaction when playing mvds.
	int					mousenewtrackplayer;

	int teamplay;
	int deathmatch;

	qboolean teamfortress;	// *sigh*. This is used for teamplay stuff. This sucks.
	qboolean hexen2pickups;
	qboolean disablemouse;	//no mouse inputs (for controller-only games, though we do also allow keyboards if only because of joy2key type stuff)

	qboolean sendprespawn;
	int contentstage;

	double matchgametimestart;
	enum {
		MATCH_DONTKNOW,	//assumed to be in progress.
		MATCH_COUNTDOWN,
		MATCH_STANDBY,
		MATCH_INPROGRESS
	} matchstate;

	enum {
		CLNQPP_NONE,
		CLNQPP_PINGS,
		CLNQPP_STATUS, //"host:    *\n" ... "players: *\n\n"
		CLNQPP_STATUSPLAYER,	//#...\n
		CLNQPP_STATUSPLAYERIP,	//   foobar\n
	}	nqparseprint;
	int			nqparseprintplayer;
	float		nqplayernamechanged;
} client_state_t;

extern unsigned int		cl_teamtopcolor;
extern unsigned int		cl_teambottomcolor;
extern unsigned int		cl_enemytopcolor;
extern unsigned int		cl_enemybottomcolor;

//FPD values
//(commented out ones are ones that we don't support)
//#define FPD_NO_SAY_MACROS			(1 << 0)
//#define FPD_NO_TIMERS				(1 << 1)
//#define FPD_NO_SOUNDTRIGGERS		(1 << 2)
#define FPD_NO_FAKE_LAG				(1 << 3)
#define FPD_ANOUNCE_FAKE_LAG		(1 << 4)
//#define FPD_HIDE_ENEMY_VICINITY	(1 << 5)
//#define FPD_NO_SPEC_CHAT			(1 << 6)
//#define FPD_HIDE_X_Y_MACRO		(1 << 7)
#define FPD_NO_FORCE_SKIN			(1 << 8)
#define FPD_NO_FORCE_COLOR			(1 << 9)
#define FPD_LIMIT_PITCH				(1 << 14)	//limit scripted pitch changes
#define FPD_LIMIT_YAW				(1 << 15)	//limit scripted yaw changes

//
// cvars
//
extern  cvar_t	cl_warncmd;
extern	cvar_t	cl_upspeed;
extern	cvar_t	cl_forwardspeed;
extern	cvar_t	cl_backspeed;
extern	cvar_t	cl_sidespeed;

extern	cvar_t	cl_movespeedkey;

extern	cvar_t	cl_yawspeed;
extern	cvar_t	cl_pitchspeed;

extern	cvar_t	cl_anglespeedkey;

extern	cvar_t	cl_shownet;
extern	cvar_t	cl_sbar;
extern	cvar_t	cl_hudswap;

extern	cvar_t	cl_pitchdriftspeed;
extern	cvar_t	lookspring;
extern	cvar_t	lookstrafe;
extern	cvar_t	sensitivity;

extern	cvar_t	m_pitch;
extern	cvar_t	m_yaw;
extern	cvar_t	m_forward;
extern	cvar_t	m_side;

#ifndef SERVERONLY
extern	cvar_t	name;
#endif


extern cvar_t ruleset_allow_playercount;
extern cvar_t ruleset_allow_frj;
extern cvar_t ruleset_allow_semicheats;
extern cvar_t ruleset_allow_packet;
extern cvar_t ruleset_allow_particle_lightning;
extern cvar_t ruleset_allow_overlongsounds;
extern cvar_t ruleset_allow_larger_models;
extern cvar_t ruleset_allow_modified_eyes;
extern cvar_t ruleset_allow_sensitive_texture_replacements;
extern cvar_t ruleset_allow_localvolume;
extern cvar_t ruleset_allow_shaders;
extern cvar_t ruleset_allow_watervis;
extern cvar_t ruleset_allow_triggers;

#ifndef SERVERONLY
extern	client_state_t	cl;
#endif

typedef struct
{
	entity_t		ent;
	entity_state_t	state;
	trailkey_t      emit;
	int	mdlidx;	/*negative are csqc indexes*/
} static_entity_t;

// FIXME, allocate dynamically
extern	entity_state_t *cl_baselines;
extern	static_entity_t		*cl_static_entities;
extern	unsigned int	cl_max_static_entities;
extern	lightstyle_t	*cl_lightstyle;
extern	size_t			cl_max_lightstyles;
extern	dlight_t		*cl_dlights;
extern	size_t cl_maxdlights;

extern	int				d_lightstylevalue[MAX_NET_LIGHTSTYLES];

extern size_t rtlights_first, rtlights_max;
extern int cl_baselines_count;

extern	qboolean	nomaster;

//=============================================================================


//
// cl_main
//
void CL_InitDlights(void);
void CL_FreeDlights(void);
dlight_t *CL_AllocDlight (int key);	//allocates or reuses the light with the specified key index
dlight_t *CL_AllocDlightOrg (int keyidx, vec3_t keyorg); //reuses the light at the specified origin...
dlight_t *CL_AllocSlight (void);	//allocates a new static light
dlight_t *CL_NewDlight (int key, const vec3_t origin, float radius, float time, float r, float g, float b);
dlight_t *CL_NewDlightCube (int key, const vec3_t origin, vec3_t angles, float radius, float time, vec3_t colours);
void CL_CloneDlight(dlight_t *dl, dlight_t *src);	//copies one light to another safely
void	CL_DecayLights (void);

void CLQW_ParseDelta (struct entity_state_s *from, struct entity_state_s *to, int bits);

void CL_Init (void);
void Host_WriteConfiguration (void);
void CL_CheckServerInfo(void);
void CL_CheckServerPacks(void);

void CL_EstablishConnection (char *host);

void CL_Disconnect (const char *reason);
void CL_Disconnect_f (void);
void CL_Reconnect_f (void);
void CL_NextDemo (void);
void CL_Startdemos_f (void);
void CL_Demos_f (void);
void CL_Stopdemo_f (void);
void CL_Changing_f (void);
void CL_Reconnect_f (void);
void CL_ConnectionlessPacket (void);
qboolean CL_DemoBehind(void);
void CL_SaveInfo(vfsfile_t *f);
void CL_SetInfo (int pnum, const char *key, const char *value);
void CL_SetInfoBlob (int pnum, const char *key, const char *value, size_t valuesize);

char *CL_TryingToConnect(void);

void CL_ExecInitialConfigs(char *defaultexec, qboolean fullvidrestart);

extern	int				cl_framecount;	//number of times the entity lists have been cleared+reset.
extern	int				cl_numvisedicts;
extern	int				cl_maxvisedicts;
extern	entity_t		*cl_visedicts;

/*these are for q3 really*/
typedef struct {
	struct shader_s *shader;
	int firstvert;
	int firstidx;
	int numvert;
	int numidx;
	unsigned int flags;
} scenetris_t;
extern scenetris_t		*cl_stris;
extern vecV_t			*fte_restrict cl_strisvertv;
extern vec4_t			*fte_restrict cl_strisvertc;
extern vec2_t			*fte_restrict cl_strisvertt;
//extern vec3_t			*fte_restrict cl_strisvertn[3];
extern index_t			*fte_restrict cl_strisidx;
extern unsigned int cl_numstrisidx;
extern unsigned int cl_maxstrisidx;
extern unsigned int cl_numstrisvert;
extern unsigned int cl_maxstrisvert;
extern unsigned int cl_numstrisnormals;
extern unsigned int cl_maxstrisnormals;
extern unsigned int cl_numstris;
extern unsigned int cl_maxstris;

#define cl_stris_ExpandVerts(max) \
	do {			\
		cl_maxstrisvert = max;	\
		cl_strisvertv = BZ_Realloc(cl_strisvertv, sizeof(*cl_strisvertv)*cl_maxstrisvert);	\
		cl_strisvertt = BZ_Realloc(cl_strisvertt, sizeof(*cl_strisvertt)*cl_maxstrisvert);	\
		cl_strisvertc = BZ_Realloc(cl_strisvertc, sizeof(*cl_strisvertc)*cl_maxstrisvert);	\
/*		cl_strisvertn[0] = BZ_Realloc(cl_strisvertn[0], sizeof(*cl_strisvertn[0])*cl_maxstrisvert);	\
		cl_strisvertn[1] = BZ_Realloc(cl_strisvertn[1], sizeof(*cl_strisvertn[1])*cl_maxstrisvert);	\
		cl_strisvertn[2] = BZ_Realloc(cl_strisvertn[2], sizeof(*cl_strisvertn[2])*cl_maxstrisvert);	\
*/	} while(0)

extern char emodel_name[], pmodel_name[], prespawn_name[], modellist_name[], soundlist_name[];

//CL_TraceLine traces against network(positive)+csqc(negative) ents. returns frac(1 on failure), and impact, normal, ent values
float CL_TraceLine (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal, int *ent);
entity_t *TraceLineR (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal, qboolean bsponly);

//
// cl_input
//
typedef struct kbutton_s
{
	int		down[MAX_SPLITS][2];		// key nums holding it down
	int		state[MAX_SPLITS];			// low bit is down state

	struct kbutton_s *suppressed[MAX_SPLITS];	//the button that was suppressed by this one getting pressed
} kbutton_t;

extern	kbutton_t	in_mlook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

extern	float in_sensitivityscale;

void CL_MakeActive(char *gamename);
void CL_UpdateWindowTitle(void);

#ifdef QUAKESTATS
const char *IN_GetPreselectedViewmodelName(unsigned int pnum);
qboolean IN_WeaponWheelAccumulate(int pnum, float x, float y, float threshhold);
qboolean IN_DrawWeaponWheel(int pnum);
qboolean IN_WeaponWheelIsShown(void);	//to decide when the game should be auto-paused.
#endif
void CL_InitInput (void);
void CL_SendCmd (double frametime, qboolean mainloop);
void CL_SendMove (usercmd_t *cmd);
#ifdef NQPROT
void CL_ParseTEnt (qboolean nqprot);
#else
void CL_ParseTEnt (void);
#endif
void CL_ParseTEnt_Sized (void);
void CL_UpdateTEnts (void);

enum beamtype_e
{	//these are internal ids, matching the beam table
	BT_Q1LIGHTNING1,
	BT_Q1LIGHTNING2,
	BT_Q1LIGHTNING3,
	BT_Q1BEAM,

	BT_Q2PARASITE,
	BT_Q2GRAPPLE,
	BT_Q2HEATBEAM,
	BT_Q2LIGHTNING,

	BT_H2LIGHTNING_SMALL,
	BT_H2CHAIN,
	BT_H2SUNSTAFF1,
	BT_H2SUNSTAFF1_SUB,	//inner beam hack
	BT_H2SUNSTAFF2,		//same model as 1, but different particle effect
	BT_H2LIGHTNING,
	BT_H2COLORBEAM,
	BT_H2ICECHUNKS,
	BT_H2GAZE,
	BT_H2FAMINE,
};
typedef struct beam_s beam_t;
beam_t *CL_AddBeam (enum beamtype_e tent, int ent, vec3_t start, vec3_t end);

void CL_ClearState (qboolean gamestart);
void CLQ2_ClearState(void);

void CL_ReadPackets (void);
void CL_ReadPacket(void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);

int Master_FindBestRoute(char *server, char *out, size_t outsize, int *directcost, int *chainedcost);

float CL_KeyState (kbutton_t *key, int pnum, qboolean noslowstart);
const char *Key_KeynumToString (int keynum, int modifier);
const char *Key_KeynumToLocalString (int keynum, int modifier);
int Key_StringToKeynum (const char *str, int *modifier);
const char *Key_GetBinding(int keynum, int bindmap, int modifier);
void Key_GetBindMap(int *bindmaps);
void Key_SetBindMap(int *bindmaps);

void CL_UseIndepPhysics(qboolean allow);
extern qboolean runningindepphys;
qboolean CL_AllowIndependantSendCmd(qboolean allow);	//returns previous state.

void CL_FlushClientCommands(void);
void VARGS CL_SendClientCommand(qboolean reliable, char *format, ...) LIKEPRINTF(2);
void VARGS CL_SendSeatClientCommand(qboolean reliable, unsigned int seat, char *format, ...) LIKEPRINTF(3);
float CL_FilterTime (double time, float wantfps, float limit, qboolean ignoreserver);
int CL_RemoveClientCommands(char *command);

//
// cl_demo.c
//
void CL_StopPlayback (void);
qboolean CL_GetDemoMessage (void);
void CL_WriteDemoCmd (usercmd_t *pcmd);
void CL_Demo_ClientCommand(char *commandtext);	//for QTV.

void CL_WriteSetDemoMessage (void);	//'restarts' a qwd, when we have reloads/map changes in them
void CL_WriteRecordQ2DemoMessage(sizebuf_t *msg);
void CL_Stop_f (void);
void CL_Record_f (void);
void CL_ReRecord_f (void);
void CL_DemoList_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx);
void CL_PlayDemo_f (void);
void CL_QTVPlay_f (void);
void CL_QTVPoll (void);
void CL_QTVList_f (void);
void CL_QTVDemos_f (void);
void CL_DemoJump_f(void);
void CL_DemoNudge_f(void);
void CL_ProgressDemoTime(void);
void CL_TimeDemo_f (void);
typedef struct 
{
	enum
	{
		QTVCT_NONE,
		QTVCT_STREAM,
		QTVCT_CONNECT,
		QTVCT_JOIN,
		QTVCT_OBSERVE,
		QTVCT_MAP
	} connectiontype;
	enum
	{
		QTVCT_NETQUAKE,
		QTVCT_QUAKEWORLD,
		QTVCT_QUAKE2,
		QTVCT_QUAKE3
	} protocol;
	char server[256];
	char splashscreen[256];
	//char *datafiles;
} qtvfile_t;
void CL_ParseQTVFile(vfsfile_t *f, const char *fname, qtvfile_t *result);

//
// cl_parse.c
//
#define NET_TIMINGS 256
#define NET_TIMINGSMASK 255
extern float	packet_latency[NET_TIMINGS];
int CL_CalcNet (float scale);
void CL_CalcNet2 (float *pings, float *pings_min, float *pings_max, float *pingms_stddev, float *pingfr, int *pingfr_min, int *pingfr_max, float *dropped, float *choked, float *invalid);
void CL_ClearParseState(void);
void CL_Parse_Disconnected(void);
void CL_DumpPacket(void);
void CL_ParseEstablished(void);
void CLQW_ParseServerMessage (void);
void CLEZ_ParseHiddenDemoMessage (void);
void CLNQ_ParseServerMessage (void);
#ifdef Q2CLIENT
void CLQ2_ParseServerMessage (void);
#endif
void CL_ShowTrafficUsage(float x, float y);
void CL_NewTranslation (int slot);

int CL_IsDownloading(const char *localname);
qboolean CL_CheckDLFile(const char *filename);
qboolean CL_CheckOrEnqueDownloadFile (const char *filename, const char *localname, unsigned int flags);
qboolean CL_EnqueDownload(const char *filename, const char *localname, unsigned int flags);
downloadlist_t *CL_DownloadFailed(const char *name, qdownload_t *qdl, enum dlfailreason_e failreason);
int CL_DownloadRate(void);
void CL_GetDownloadSizes(unsigned int *filecount, qofs_t *totalsize, qboolean *somesizesunknown);
qboolean CL_ParseOOBDownload(void);
void CL_DownloadFinished(qdownload_t *dl);
void CL_RequestNextDownload (void);
void CL_SendDownloadReq(sizebuf_t *msg);
void Sound_CheckDownload(const char *s); /*checkorenqueue a sound file*/

qboolean CL_IsUploading(void);
void CL_NextUpload(void);
void CL_StartUpload (qbyte *data, int size);
void CL_StopUpload(void);

qboolean CL_CheckBaselines (int size);

//
// view.c
//
void V_StartPitchDrift (playerview_t *pv);
void V_StopPitchDrift (playerview_t *pv);

void V_RenderView (qboolean no2d);
void V_Register (void);
void V_ParseDamage (playerview_t *pv);
void V_SetContentsColor (int contents);

//used directly by csqc
void V_CalcRefdef (playerview_t *pv);
void V_ClearRefdef(playerview_t *pv);
void V_ApplyRefdef(void);
void V_CalcGunPositionAngle (playerview_t *pv, float bob);
float V_CalcBob (playerview_t *pv, qboolean queryold);
void DropPunchAngle (playerview_t *pv);

int V_EditExternalModels(int newviewentity, entity_t *viewentities, int maxviewenties);
void V_DepthSortEntities(float *vieworg);


//
// cl_tent
//
void CL_RegisterParticles(void);
void CL_InitTEnts (void);
void CL_InitTEntSounds (void);
void CL_ClearTEnts (void);
void CL_ClearTEntParticleState (void);
void CL_ClearCustomTEnts(void);
void CL_ParseCustomTEnt(void);
qboolean CL_WriteCustomTEnt(sizebuf_t *buf, int id);
void CL_ParseEffect (qboolean effect2);

void CLNQ_ParseParticleEffect (void);
void CL_ParseParticleEffect2 (void);
void CL_ParseParticleEffect3 (void);
void CL_ParseParticleEffect4 (void);

int CL_TranslateParticleFromServer(int sveffect);
void CL_ParseTrailParticles(void);
void CL_ParsePointParticles(qboolean compact);
void CL_SpawnSpriteEffect(vec3_t org, vec3_t dir, vec3_t orientationup, struct model_s *model, int startframe, int framecount, float framerate, float alpha, float scale, float randspin, float gravity, int traileffect, unsigned int renderflags, int skinnum, float red, float green, float blue);	/*called from the particlesystem*/

//
// cl_ents.c
//
void CL_SetSolidPlayers (void);
void CL_SetUpPlayerPrediction(qboolean dopred);
void CL_LinkStaticEntities(void *pvs, int *areas);
void CL_TransitionEntities (void); /*call at the start of the frame*/
void CL_EmitEntities (void);
void CL_ClearProjectiles (void);
void CL_ParseProjectiles (int modelindex, qboolean nails2);
void CLQW_ParsePacketEntities (qboolean delta);
void CLFTE_ParseEntities (void);
void CLFTE_ParseBaseline(entity_state_t *es, qboolean numberisimportant);
void CL_SetSolidEntities (void);
void CLQW_ParsePlayerinfo (void);
void CL_ParseClientPersist(void);
//these last ones are needed for csqc handling of engine-bound ents.
void CL_ClearEntityLists(void);
void CL_FreeVisEdicts(void);
void CL_LinkViewModel(void);
void CL_LinkPlayers (void);
void CL_LinkPacketEntities (void);
void CL_LinkProjectiles (void);
void CL_ClearLerpEntsParticleState (void);
qboolean CL_MayLerp(void);

//
//pr_csqc.c
//
#ifdef CSQC_DAT
qboolean CSQC_Inited(void);
void	 CSQC_RendererRestarted(qboolean initing);
qboolean CSQC_UnconnectedOkay(qboolean inprinciple);
qboolean CSQC_UnconnectedInit(void);
qboolean CSQC_CheckDownload(const char *name, unsigned int checksum, size_t checksize);	//reports whether we already have a usable csprogs.dat
qboolean CSQC_Init (qboolean anycsqc, const char *csprogsname, unsigned int checksum, size_t progssize);
qboolean CSQC_ConsoleLink(char *text, char *info);
void	 CSQC_RegisterCvarsAndThings(void);
qboolean CSQC_SetupToRenderPortal(int entnum);
qboolean CSQC_DrawView(void);
qboolean CSQC_DrawHud(playerview_t *pv);
qboolean CSQC_DrawScores(playerview_t *pv);
qboolean CSQC_UseGamecodeLoadingScreen(void);
void	 CSQC_Shutdown(void);
qboolean CSQC_StuffCmd(int lplayernum, char *cmd, char *cmdend);
void	 CSQC_MapEntityEdited(int modelindex, int idx, const char *newe);
//qboolean CSQC_LoadResource(char *resname, char *restype);
qboolean CSQC_ParsePrint(char *message, int printlevel);
qboolean CSQC_ParseGamePacket(int seat, qboolean sized);
qboolean CSQC_CenterPrint(int seat, const char *cmd);
void	 CSQC_ServerInfoChanged(void);
void	 CSQC_PlayerInfoChanged(int player);
qboolean CSQC_Parse_Damage(int seat, float save, float take, vec3_t source);
qboolean CSQC_Parse_SetAngles(int seat, vec3_t newangles, qboolean wasdelta);
void	 CSQC_Input_Frame(int seat, usercmd_t *cmd);
void	 CSQC_WorldLoaded(void);
qboolean CSQC_ParseTempEntity(void);
qboolean CSQC_ConsoleCommand(int seat, const char *cmd);
qboolean CSQC_KeyPress(int key, int unicode, qboolean down, unsigned int devid);
qboolean CSQC_MouseMove(float xdelta, float ydelta, unsigned int devid);
qboolean CSQC_MousePosition(float xabs, float yabs, unsigned int devid);
qboolean CSQC_JoystickAxis(int axis, float value, unsigned int devid);
qboolean CSQC_Accelerometer(float x, float y, float z);
qboolean CSQC_Gyroscope(float x, float y, float z);
int		 CSQC_StartSound(int entnum, int channel, char *soundname, vec3_t pos, float vol, float attenuation, float pitchmod, float timeofs, unsigned int flags);
void	 CSQC_ParseEntities(qboolean sized);
const char *CSQC_GetExtraFieldInfo(void *went, char *out, size_t outsize);
void	 CSQC_ResetTrails(void);

qboolean CSQC_DeltaPlayer(int playernum, player_state_t *state);
void	 CSQC_DeltaStart(float time);
qboolean CSQC_DeltaUpdate(entity_state_t *src);
void	 CSQC_DeltaEnd(void);

void	 CSQC_CvarChanged(cvar_t *var);
#else
#define CSQC_UnconnectedOkay(inprinciple) false
#define CSQC_UnconnectedInit() false
#define CSQC_UseGamecodeLoadingScreen() false
#define CSQC_Parse_SetAngles(seat,newangles,wasdelta) false
#define CSQC_ServerInfoChanged()
#define CSQC_PlayerInfoChanged(player)
#endif

//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);
void CL_PredictUsercmd (int seat, int entnum, player_state_t *from, player_state_t *to, usercmd_t *u);
#ifdef Q2CLIENT
void CLQ2_CheckPredictionError (void);
#endif
void CL_CalcClientTime(void);

//
// cl_cam.c
//
qboolean Cam_DrawViewModel(playerview_t *pv);
int Cam_TrackNum(playerview_t *pv);
void Cam_Unlock(playerview_t *pv);				//revert to freecam or so, because that entity failed.
void Cam_Lock(playerview_t *pv, int playernum);	//attempt to lock on to the given player.
void Cam_NowLocked(playerview_t *pv);						//player was located, track them now
void Cam_SelfTrack(playerview_t *pv);
void Cam_Track(playerview_t *pv, usercmd_t *cmd);
void Cam_SetModAutoTrack(int userid);
void Cam_FinishMove(playerview_t *pv, usercmd_t *cmd);
void Cam_Reset(void);
void Cam_TrackPlayer(int seat, char *cmdname, char *plrarg);
void CL_InitCam(void);
void Cam_AutoTrack_Update(const char *mode);	//reset autotrack setting (because we started a new map or whatever)

//
//zqtp.c
//
#define TPM_UNKNOWN    0
#define TPM_NORMAL     1
#define TPM_TEAM       2
#define TPM_SPECTATOR  4
#define TPM_FAKED     16
#define TPM_OBSERVEDTEAM  32
#define TPM_QTV       64		//should only be qtv_say_game/qtv_say_team_game

void		CL_Say (qboolean team, char *extra);
int			TP_CategorizeMessage (char *s, int *offset, player_info_t **plr);
void		TP_ExecTrigger (char *s, qboolean indemos);		//executes one of the user's f_foo aliases from some engine-defined event.
qboolean	TP_FilterMessage (char *s);
void		TP_Init(void);
qboolean	TP_HaveLocations(void);
char*		TP_LocationName (const vec3_t location);
void		TP_NewMap (void);
qboolean	TP_CheckSoundTrigger (char *str);				//plays sound files when some substring exists in chat.
void		TP_SearchForMsgTriggers (char *s, int level);	//msg_trigger: executes aliases when a chat message contains some user-defined string.
qboolean	TP_SuppressMessage(char *buf);	//true when the message contains macro results that the local player isn't meant to see (teamplay messages that contain enemy player counts for instance)
char		*TP_GenerateDemoName(void);		//makes something up.
#ifdef QUAKESTATS
//hack zone: this stuff makes assumptions about quake-only stats+items+rules and stuff.
void		TP_CheckPickupSound(char *s, vec3_t org, int seat);
void		TP_ParsePlayerInfo(player_state_t *oldstate, player_state_t *state, player_info_t *info);
qboolean	TP_IsPlayerVisible(vec3_t origin);
qboolean	TP_SoundTrigger(char *message);
void		TP_StatChanged (int stat, int value);
void		TP_UpdateAutoStatus(void);
#endif
#ifdef QWSKINS
colourised_t *TP_FindColours(char *name);
#endif

//
// skin.c
//

typedef struct
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
//    unsigned char	data;			// unbounded
} pcx_t;
qbyte *ReadPCXData(qbyte *buf, int length, int width, int height, qbyte *result);


qwskin_t *Skin_Lookup (char *fullname);
char *Skin_FindName (player_info_t *sc);
void	Skin_Find (player_info_t *sc);
qbyte	*Skin_TryCache8 (qwskin_t *skin);
void	Skin_Skins_f (void);
void	Skin_FlushSkin(char *name);
void	Skin_AllSkins_f (void);
void	Skin_NextDownload (void);
void Skin_FlushPlayers(void);
void Skin_FlushAll(void);

#define RSSHOT_WIDTH 320
#define RSSHOT_HEIGHT 200





//valid.c
#define RULESET_USERINFO	"*rs"	//the userinfo used to report the ruleset name to other clients. FIXME: remove the _dbg when we think its all working properly.
void		Ruleset_Check(char *keyval, char *out, size_t outsize);
void		Ruleset_Scan(void);
void		Ruleset_Shutdown(void);
const char *Ruleset_GetRulesetName(void);
qboolean	Ruleset_FileLoaded(const char *filename, const qbyte *filedata, size_t filesize);	//return false if the file is not permitted.
void	Validation_Apply_Ruleset(void);
void	Validation_FlushFileList(void);
void	Validation_CheckIfResponse(char *text);
void	Validation_DelatchRulesets(void);
void	InitValidation(void);
void	Validation_Auto_Response(int playernum, char *s);

extern	qboolean f_modified_particles;
extern	qboolean care_f_modified;


//random files (fixme: clean up)

#ifdef Q2CLIENT
unsigned int CLQ2_GatherSounds(vec3_t *positions, unsigned int *entnums, sfx_t **sounds, unsigned int max);
void CLQ2_ParseTEnt (void);
void CLQ2_AddEntities (void);
void CLQ2_ParseBaseline (void);
void CLQ2_ClearParticleState(void);
void CLR1Q2_ParsePlayerUpdate(void);
void CLQ2_ParseFrame (int extrabits);
void CLQ2_ParseMuzzleFlash (void);
void CLQ2_ParseMuzzleFlash2 (void);
void CLQ2EX_ParseMuzzleFlash3 (void);
void CLQ2_ParseInventory (int seat);
int CLQ2_RegisterTEntModels (void);
void CLQ2_WriteDemoBaselines(sizebuf_t *buf);
#endif

#ifdef HLCLIENT
//networking
void CLHL_LoadClientGame(void);
int CLHL_ParseGamePacket(void);
int CLHL_AnimateViewEntity(entity_t *ent);
//screen
int CLHL_DrawHud(void);
//inputs
int CLHL_GamecodeDoesMouse(void);
int CLHL_MouseEvent(unsigned int buttonmask);
void CLHL_SetMouseActive(int activate);
int CLHL_BuildUserInput(int msecs, usercmd_t *cmd);
#endif

#ifdef NQPROT
void CLNQ_ParseEntity(unsigned int bits);
void NQ_P_ParseParticleEffect (void);
void CLNQ_SignonReply (void);
void NQ_BeginConnect(char *to);
void NQ_ContinueConnect(char *to);
int CLNQ_GetMessage (void);
#endif

void CL_BeginServerReconnect(void);
qboolean CL_IsPendingServerAddress(netadr_t *adr);
void CL_Transfer(netadr_t *adr);

void SV_User_f (void);	//called by client version of the function
void SV_Serverinfo_f (void);
void SV_ConSay_f(void);



#ifdef TEXTEDITOR
extern console_t *editormodal;
void Editor_Draw(void);
void Editor_Init(void);
struct pubprogfuncs_s;
void Editor_ProgsKilled(struct pubprogfuncs_s *dead);
#else
#define editormodal false
#endif

void SCR_StringToRGB (char *rgbstring, float *rgb, float rgbinputscale);

struct model_s;
void CL_AddVWeapModel(entity_t *player, struct model_s *model);

typedef struct cin_s cin_t;
#ifdef HAVE_MEDIA_DECODER

#ifdef Q2CLIENT /*q2 cinematics*/
struct cinematics_s;
void CIN_StopCinematic (struct cinematics_s *cin);
struct cinematics_s *CIN_PlayCinematic (char *arg);
int CIN_RunCinematic (struct cinematics_s *cin, float playbacktime, qbyte **outdata, int *outwidth, int *outheight, qbyte **outpalette);
void CIN_Rewind(struct cinematics_s *cin);
#endif

typedef enum
{
	CINSTATE_INVALID,	//also reported for not playing
	CINSTATE_PLAY,
	CINSTATE_LOOP,
	CINSTATE_PAUSE,
	CINSTATE_ENDED,
	CINSTATE_FLUSHED,	//video will restart from beginning
} cinstates_t;
/*media playing system*/
qboolean Media_PlayFilm(char *name, qboolean enqueue);
qboolean Media_StopFilm(qboolean all);
struct cin_s *Media_StartCin(char *name);
texid_tf Media_UpdateForShader(cin_t *cin);
void Media_ShutdownCin(cin_t *cin);

//these accept NULL for cin to mean the current fullscreen video
void Media_Send_Command(cin_t *cin, const char *command);
void Media_Send_MouseMove(cin_t *cin, float x, float y);
void Media_Send_Resize(cin_t *cin, int x, int y);
void Media_Send_GetSize(cin_t *cin, int *x, int *y, float *aspect);
void Media_Send_KeyEvent(cin_t *cin, int button, int unicode, int event);
void Media_Send_Reset(cin_t *cin);
void Media_SetState(cin_t *cin, cinstates_t newstate);
cinstates_t Media_GetState(cin_t *cin);
const char *Media_Send_GetProperty(cin_t *cin, const char *key);

#else
#define Media_PlayFilm(n,e) false
#define Media_StopFilm(a) (void)true
#endif

void Media_SaveTracks(vfsfile_t *outcfg);
void Media_Init(void);
qboolean Media_NamedTrack(const char *initialtrack, const char *looptrack);	//new background music interface
void Media_NumberedTrack(unsigned int initialtrack, unsigned int looptrack);				//legacy cd interface for protocols that only support numbered tracks.
void Media_EndedTrack(void);	//cd is no longer running, media code needs to pick a new track (cd track or faketrack)

void MVD_Interpolate(void);

int Stats_GetKills(int playernum);
int Stats_GetTKills(int playernum);
int Stats_GetDeaths(int playernum);
int Stats_GetTouches(int playernum);
int Stats_GetCaptures(int playernum);
qboolean Stats_HaveFlags(int mode);
qboolean Stats_HaveKills(void);
float Stats_GetLastOwnFrag(int seat, char *res, int reslen);
void VARGS Stats_Message(char *msg, ...) LIKEPRINTF(1);
qboolean Stats_ParsePrintLine(const char *line);
qboolean Stats_ParsePickups(const char *line);
void Stats_NewMap(void);
void Stats_Clear(void);
void Stats_Init(void);

//enum uploadfmt;
/*struct mediacallbacks_s
{	//functions provided by the engine/renderer, for faster/off-thread updates
	qboolean pbocanoffthread;
	qboolean (VARGS *PBOLock)(struct mediacallbacks_s *ctx, size_t width, size_t height, uploadfmt_t fmt, qboolean *lost);
	void (VARGS *PBOUpdate)(struct mediacallbacks_s *ctx, void *data, size_t width, size_t height, int stride);
	void (VARGS *PBOUnlock)(struct mediacallbacks_s *ctx);

	void (VARGS *AudioStream) (void *auddata, int rate, int frames, int channels, int width);

	void (VARGS *WorkQueue) (void *wctx, void (VARGS *callback)(void *data), void *data);
	void (VARGS *WorkSync)  (void *wctx, int *address, int oldvalue);	//blocks until the address changes. make sure you queued something that will change it from that value. oldvalue is present to avoid races. if you're reading the address then you should probably volatile it to avoid compiler opts reading it twice (fixme: needs a proper barrier).
};
*/
typedef struct
{
	size_t structsize;
	const char *drivername;
	void *(VARGS *createdecoder)(const char *name);
	qboolean (VARGS *decodeframe)(void *ctx, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ectx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ectx);
	void (VARGS *shutdown)(void *ctx);
	void (VARGS *rewind)(void *ctx);

	//these are any interactivity functions you might want...
	void (VARGS *cursormove) (void *ctx, float posx, float posy);	//pos is 0-1
	void (VARGS *key) (void *ctx, int code, int unicode, int event);
	qboolean (VARGS *setsize) (void *ctx, int width, int height);
	void (VARGS *getsize) (void *ctx, int *width, int *height);
	void (VARGS *changestream) (void *ctx, const char *streamname);

	qboolean (VARGS *getproperty) (void *ctx, const char *field, char *out, size_t *outsize);	//if out is null, returns required buffer size. returns 0 on failure / buffer too small

//	void *(VARGS *createdecoderCB)(const char *name, struct mediacallbacks_s *callbacks);
} media_decoder_funcs_t;
typedef struct
{
	size_t structsize;
	const char *drivername;
	const char *description;
	const char *extensions;
	void *(VARGS *capture_begin) (char *streamname, int videorate, int width, int height, int *sndkhz, int *sndchannels, int *sndbits);
	void (VARGS *capture_video) (void *ctx, int frame, void *data, int stride, int width, int height, enum uploadfmt fmt);
	void (VARGS *capture_audio) (void *ctx, void *data, int bytes);
	void (VARGS *capture_end) (void *ctx);
} media_encoder_funcs_t;
extern struct plugin_s *currentplug;
qboolean Media_RegisterDecoder(struct plugin_s *plug, media_decoder_funcs_t *funcs);
qboolean Media_UnregisterDecoder(struct plugin_s *plug, media_decoder_funcs_t *funcs);
qboolean Media_RegisterEncoder(struct plugin_s *plug, media_encoder_funcs_t *funcs);
qboolean Media_UnregisterEncoder(struct plugin_s *plug, media_encoder_funcs_t *funcs);
