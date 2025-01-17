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

#ifndef CLIENT_H
#define CLIENT_H

#include "matrixlib.h"
#include "snd_main.h"
#include "view.h"
#include "cap.h"
#include "cl_parse.h"
#include "cl_particles.h"
#include "r_stats.h"

// flags for rtlight rendering
#define LIGHTFLAG_NORMALMODE 1
#define LIGHTFLAG_REALTIMEMODE 2

typedef struct tridecal_s
{
	// color and initial alpha value
	float			texcoord2f[3][2];
	float			vertex3f[3][3];
	float			color4f[3][4];
	float			plane[4]; // backface culling
	// how long this decal has lived so far (the actual fade begins at cl_decals_time)
	float			lived;
	// if >= 0 this indicates the decal should follow an animated triangle
	int				triangleindex;
	// for visibility culling
	int				surfaceindex;
	// old decals are killed to obey cl_decals_max
	unsigned int	decalsequence;
}
tridecal_t;

typedef struct decalsystem_s
{
	model_t *model;
	double lastupdatetime;
	int maxdecals;
	int numdecals;
	tridecal_t *decals;
	float *vertex3f;
	float *texcoord2f;
	float *color4f;
	int *element3i;
	unsigned short *element3s;
}
decalsystem_t;

typedef struct effect_s
{
	int active;
	vec3_t origin;
	double starttime;
	float framerate;
	model_t *model;
	int startframe;
	int endframe;
	// these are for interpolation
	int frame;
	double frame1time;
	double frame2time;
}
cl_effect_t;

typedef struct beam_s
{
	int		entity;
	// draw this as lightning polygons, or a model?
	int		lightning;
	struct model_s	*model;
	float	endtime;
	vec3_t	start, end;
}
beam_t;

typedef struct rtlight_particle_s
{
	float origin[3];
	float color[3];
}
rtlight_particle_t;

typedef struct rtlight_s
{
	// note that the world to light matrices are inversely scaled (divided) by lightradius

	// core properties
	/// matrix for transforming light filter coordinates to world coordinates
	matrix4x4_t matrix_lighttoworld;
	/// matrix for transforming world coordinates to light filter coordinates
	matrix4x4_t matrix_worldtolight;
	/// typically 1 1 1, can be lower (dim) or higher (overbright)
	vec3_t color;
	/// size of the light (remove?)
	vec_t radius;
	/// light filter
	char cubemapname[64];
	/// light style to monitor for brightness
	int style;
	/// whether light should render shadows (see castshadows for whether it actually does this frame)
	int shadow;
	/// intensity of corona to render
	vec_t corona;
	/// radius scale of corona to render (1.0 means same as light radius)
	vec_t coronasizescale;
	/// ambient intensity to render
	vec_t ambientscale;
	/// diffuse intensity to render
	vec_t diffusescale;
	/// specular intensity to render
	vec_t specularscale;
	/// LIGHTFLAG_* flags
	int flags;

	// generated properties
	/// used only for casting shadows
	vec3_t shadoworigin;
	/// culling
	vec3_t cullmins;
	vec3_t cullmaxs;
	/// when r_shadow_culllights_trace is set, this is refreshed by each successful trace.
	double trace_timer;

	// rendering properties, updated each time a light is rendered
	// this is rtlight->color * d_lightstylevalue
	vec3_t currentcolor;
	/// used by corona updates, due to occlusion query
	float corona_visibility;
	unsigned int corona_queryindex_visiblepixels;
	unsigned int corona_queryindex_allpixels;
	/// this is R_GetCubemap(rtlight->cubemapname)
	rtexture_t *currentcubemap;
	/// set by R_Shadow_PrepareLight to decide whether R_Shadow_DrawLight should draw it
	qbool draw;
	/// set by R_Shadow_PrepareLight to indicate whether R_Shadow_DrawShadowMaps should do anything
	qbool castshadows;
	/// these fields are set by R_Shadow_PrepareLight for later drawing
	int cached_numlightentities;
	int cached_numlightentities_noselfshadow;
	int cached_numshadowentities;
	int cached_numshadowentities_noselfshadow;
	int cached_numsurfaces;
	struct entity_render_s **cached_lightentities;
	struct entity_render_s **cached_lightentities_noselfshadow;
	struct entity_render_s **cached_shadowentities;
	struct entity_render_s **cached_shadowentities_noselfshadow;
	unsigned char *cached_shadowtrispvs;
	unsigned char *cached_lighttrispvs;
	int *cached_surfacelist;
	// reduced light cullbox from GetLightInfo
	vec3_t cached_cullmins;
	vec3_t cached_cullmaxs;
	// current shadow-caster culling planes based on view
	// (any geometry outside these planes can not contribute to the visible
	//  shadows in any way, and thus can be culled safely)
	int cached_numfrustumplanes;
	mplane_t cached_frustumplanes[5]; // see R_Shadow_ComputeShadowCasterCullingPlanes

	/// static light info
	/// true if this light should be compiled as a static light
	int isstatic;
	/// true if this is a compiled world light, cleared if the light changes
	int compiled;
	/// the size that this light should have (assuming no scene LOD kicking in to reduce it)
	int shadowmapsidesize;
	/// position of this light in the shadowmap atlas
	int shadowmapatlasposition[2];
	/// size of one side of this light in the shadowmap atlas (for omnidirectional shadowmaps this is the min corner of a 2x3 arrangement, or a 4x3 arrangement in the case of noselfshadow entities being present)
	int shadowmapatlassidesize;
	/// optimized and culled mesh to render for world entity shadows
	shadowmesh_t *static_meshchain_shadow_shadowmap;
	/// used for visibility testing (more exact than bbox)
	int static_numleafs;
	int static_numleafpvsbytes;
	int *static_leaflist;
	unsigned char *static_leafpvs;
	/// surfaces seen by light
	int static_numsurfaces;
	int *static_surfacelist;
	/// flag bits indicating which triangles of the world model should cast
	/// shadows, and which ones should be lit
	///
	/// this avoids redundantly scanning the triangles in each surface twice
	/// for whether they should cast shadows, once in culling and once in the
	/// actual shadowmarklist production.
	int static_numshadowtrispvsbytes;
	unsigned char *static_shadowtrispvs;
	/// this allows the lighting batch code to skip backfaces andother culled
	/// triangles not relevant for lighting
	/// (important on big surfaces such as terrain)
	int static_numlighttrispvsbytes;
	unsigned char *static_lighttrispvs;
	/// masks of all shadowmap sides that have any potential static receivers or casters
	int static_shadowmap_receivers;
	int static_shadowmap_casters;
	/// particle-tracing cache for global illumination
	int particlecache_numparticles;
	int particlecache_maxparticles;
	int particlecache_updateparticle;
	rtlight_particle_t *particlecache_particles;

	/// bouncegrid light info
	float bouncegrid_photoncolor[3];
	float bouncegrid_photons;
	int bouncegrid_hits;
	int bouncegrid_traces;
	float bouncegrid_effectiveradius;
}
rtlight_t;

typedef struct dlight_s
{
	// destroy light after this time
	// (dlight only)
	vec_t die;
	// the entity that owns this light (can be NULL)
	// (dlight only)
	struct entity_render_s *ent;
	// location
	// (worldlight: saved to .rtlights file)
	vec3_t origin;
	// worldlight orientation
	// (worldlight only)
	// (worldlight: saved to .rtlights file)
	vec3_t angles;
	// dlight orientation/scaling/location
	// (dlight only)
	matrix4x4_t matrix;
	// color of light
	// (worldlight: saved to .rtlights file)
	vec3_t color;
	// cubemap name to use on this light
	// (worldlight: saved to .rtlights file)
	char cubemapname[64];
	// make light flash while selected
	// (worldlight only)
	int selected;
	// brightness (not really radius anymore)
	// (worldlight: saved to .rtlights file)
	vec_t radius;
	// drop intensity this much each second
	// (dlight only)
	vec_t decay;
	// intensity value which is dropped over time
	// (dlight only)
	vec_t intensity;
	// initial values for intensity to modify
	// (dlight only)
	vec_t initialradius;
	vec3_t initialcolor;
	// light style which controls intensity of this light
	// (worldlight: saved to .rtlights file)
	int style;
	// cast shadows
	// (worldlight: saved to .rtlights file)
	int shadow;
	// corona intensity
	// (worldlight: saved to .rtlights file)
	vec_t corona;
	// radius scale of corona to render (1.0 means same as light radius)
	// (worldlight: saved to .rtlights file)
	vec_t coronasizescale;
	// ambient intensity to render
	// (worldlight: saved to .rtlights file)
	vec_t ambientscale;
	// diffuse intensity to render
	// (worldlight: saved to .rtlights file)
	vec_t diffusescale;
	// specular intensity to render
	// (worldlight: saved to .rtlights file)
	vec_t specularscale;
	// LIGHTFLAG_* flags
	// (worldlight: saved to .rtlights file)
	int flags;
	// linked list of world lights
	// (worldlight only)
	struct dlight_s *next;
	// embedded rtlight struct for renderer
	// (worldlight only)
	rtlight_t rtlight;
}
dlight_t;

// this is derived from processing of the framegroupblend array
// note: technically each framegroupblend can produce two of these, but that
// never happens in practice because no one blends between more than 2
// framegroups at once
#define MAX_FRAMEBLENDS (MAX_FRAMEGROUPBLENDS * 2)
typedef struct frameblend_s
{
	int subframe;
	float lerp;
}
frameblend_t;

// LadyHavoc: this struct is intended for the renderer but some fields are
// used by the client.
//
// The renderer should not rely on any changes to this struct to be persistent
// across multiple frames because temp entities are wiped every frame, but it
// is acceptable to cache things in this struct that are not critical.
//
// For example the r_cullentities_trace code does such caching.
typedef struct entity_render_s
{
	// location
	//vec3_t origin;
	// orientation
	//vec3_t angles;
	// transform matrix for model to world
	matrix4x4_t matrix;
	// transform matrix for world to model
	matrix4x4_t inversematrix;
	// opacity (alpha) of the model
	float alpha;
	// size the model is shown
	float scale;
	// transparent sorting offset
	float transparent_offset;

	// NULL = no model
	model_t *model;
	// number of the entity represents, or 0 for non-network entities
	int entitynumber;
	// literal colormap colors for renderer, if both are 0 0 0 it is not colormapped
	vec3_t colormap_pantscolor;
	vec3_t colormap_shirtcolor;
	// light, particles, etc
	int effects;
	// qw CTF flags and other internal-use-only effect bits
	int internaleffects;
	// for Alias models
	int skinnum;
	// render flags
	int flags;

	// colormod tinting of models
	float colormod[3];
	float glowmod[3];

	// interpolated animation - active framegroups and blend factors
	framegroupblend_t framegroupblend[MAX_FRAMEGROUPBLENDS];

	// time of last model change (for shader animations)
	double shadertime;

	// calculated by the renderer (but not persistent)

	// calculated during R_AddModelEntities
	vec3_t mins, maxs;
	// subframe numbers (-1 if not used) and their blending scalers (0-1), if interpolation is not desired, use subframeblend[0].subframe
	frameblend_t frameblend[MAX_FRAMEBLENDS];
	// skeletal animation data (if skeleton.relativetransforms is not NULL, it overrides frameblend)
	skeleton_t *skeleton;

	// animation cache (pointers allocated using R_FrameData_Alloc)
	// ONLY valid during R_RenderView!  may be NULL (not cached)
	float          *animcache_vertex3f;
	r_meshbuffer_t *animcache_vertex3f_vertexbuffer;
	int             animcache_vertex3f_bufferoffset;
	float          *animcache_normal3f;
	r_meshbuffer_t *animcache_normal3f_vertexbuffer;
	int             animcache_normal3f_bufferoffset;
	float          *animcache_svector3f;
	r_meshbuffer_t *animcache_svector3f_vertexbuffer;
	int             animcache_svector3f_bufferoffset;
	float          *animcache_tvector3f;
	r_meshbuffer_t *animcache_tvector3f_vertexbuffer;
	int             animcache_tvector3f_bufferoffset;
	// gpu-skinning shader needs transforms in a certain format, we have to
	// upload this to a uniform buffer for the shader to use, and also keep a
	// backup copy in system memory for the dynamic batch fallback code
	// if this is not NULL, the other animcache variables are NULL
	float *animcache_skeletaltransform3x4;
	r_meshbuffer_t *animcache_skeletaltransform3x4buffer;
	int animcache_skeletaltransform3x4offset;
	int animcache_skeletaltransform3x4size;

	// CL_UpdateEntityShading reads these fields
	// used only if RENDER_CUSTOMIZEDMODELLIGHT is set
	vec3_t custommodellight_ambient;
	vec3_t custommodellight_diffuse;
	vec3_t custommodellight_lightdir;
	// CSQC entities get their shading from the root of their attachment chain
	float custommodellight_origin[3];

	// derived lighting parameters (CL_UpdateEntityShading)

	// used by MATERIALFLAG_FULLBRIGHT which is MATERIALFLAG_MODELLIGHT with
	// this as ambient color, along with MATERIALFLAG_NORTLIGHT
	float render_fullbright[3];
	// color tint for the base pass glow textures if any
	float render_glowmod[3];
	// MATERIALFLAG_MODELLIGHT uses these parameters
	float render_modellight_ambient[3];
	float render_modellight_diffuse[3];
	float render_modellight_lightdir_world[3];
	float render_modellight_lightdir_local[3];
	float render_modellight_specular[3];
	// lightmap rendering (not MATERIALFLAG_MODELLIGHT)
	float render_lightmap_ambient[3];
	float render_lightmap_diffuse[3];
	float render_lightmap_specular[3];
	// rtlights use these colors for the materials on this entity
	float render_rtlight_diffuse[3];
	float render_rtlight_specular[3];
	// ignore lightmap and use fixed lighting settings on this entity (e.g. FULLBRIGHT)
	qbool render_modellight_forced;
	// do not process per pixel lights on this entity at all (like MATERIALFLAG_NORTLIGHT)
	qbool render_rtlight_disabled;
	// use the 3D lightmap from q3bsp on this entity
	qbool render_lightgrid;

	// storage of decals on this entity
	// (note: if allowdecals is set, be sure to call R_DecalSystem_Reset on removal!)
	int allowdecals;
	decalsystem_t decalsystem;

	// FIELDS UPDATED BY RENDERER:
	// last time visible during trace culling
	double last_trace_visibility;

	// user wavefunc parameters (from csqc)
	vec_t userwavefunc_param[Q3WAVEFUNC_USER_COUNT];
}
entity_render_t;

typedef struct entity_persistent_s
{
	vec3_t trail_origin; // previous position for particle trail spawning
	vec3_t oldorigin; // lerp
	vec3_t oldangles; // lerp
	vec3_t neworigin; // lerp
	vec3_t newangles; // lerp
	vec_t lerpstarttime; // lerp
	vec_t lerpdeltatime; // lerp
	float muzzleflash; // muzzleflash intensity, fades over time
	float trail_time; // residual error accumulation for particle trail spawning (to keep spacing across frames)
	qbool trail_allowed; // set to false by teleports, true by update code, prevents bad lerps
}
entity_persistent_t;

typedef struct entity_s
{
	// baseline state (default values)
	entity_state_t state_baseline;
	// previous state (interpolating from this)
	entity_state_t state_previous;
	// current state (interpolating to this)
	entity_state_t state_current;

	// used for regenerating parts of render
	entity_persistent_t persistent;

	// the only data the renderer should know about
	entity_render_t render;
}
entity_t;

typedef struct lightstyle_s
{
	int		length;
	char	map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct scoreboard_s
{
	char	name[MAX_SCOREBOARDNAME];
	int		frags;
	int		colors; // two 4 bit fields
	// QW fields:
	int		qw_userid;
	char	qw_userinfo[MAX_USERINFO_STRING];
	float	qw_entertime;
	int		qw_ping;
	int		qw_packetloss;
	int		qw_movementloss;
	int		qw_spectator;
	char	qw_team[8];
	char	qw_skin[MAX_QPATH];
} scoreboard_t;

typedef struct cshift_s
{
	float	destcolor[3];
	float	percent;		// 0-255
	float   alphafade;      // (any speed)
} cshift_t;

#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define	CSHIFT_VCSHIFT	4
#define	NUM_CSHIFTS		5

#define	NAME_LENGTH	64


//
// client_state_t should hold all pieces of the client state
//

#define	SIGNONS		4			// signon messages to receive before connected

typedef enum cactive_e
{
	ca_uninitialized,	// during early startup
	ca_dedicated, 		// a dedicated server with no ability to start a client
	ca_disconnected, 	// full screen console with no connection
	ca_connected		// valid netcon, talking to a server
}
cactive_t;

typedef enum qw_downloadtype_e
{
	dl_none,
	dl_single,
	dl_skin,
	dl_model,
	dl_sound
}
qw_downloadtype_t;

#define CL_MAX_DOWNLOADACKS 4

typedef struct cl_downloadack_s
{
	int start, size;
}
cl_downloadack_t;

typedef struct cl_soundstats_s
{
	int mixedsounds;
	int totalsounds;
	int latency_milliseconds;
}
cl_soundstats_t;

//
// the client_static_t structure is persistent through an arbitrary number
// of server connections
//
typedef struct client_static_s
{
	cactive_t state;

	// all client memory allocations go in these pools
	mempool_t *levelmempool;
	mempool_t *permanentmempool;

// demo loop control
	// -1 = don't play demos
	int demonum;
	// list of demos in loop
	char demos[MAX_DEMOS][MAX_DEMONAME];
	// the actively playing demo (set by CL_PlayDemo)
	char demoname[MAX_QPATH];

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qbool demorecording;
	fs_offset_t demo_lastcsprogssize;
	int demo_lastcsprogscrc;
	qbool demoplayback;
	qbool demostarting; // set if currently starting a demo, to stop -demo from quitting when switching to another demo
	qbool timedemo;
	// -1 = use normal cd track
	int forcetrack;
	qfile_t *demofile;
	// realtime at second frame of timedemo (LadyHavoc: changed to double)
	double td_starttime;
	int td_frames; // total frames parsed
	double td_onesecondnexttime;
	double td_onesecondframes;
	double td_onesecondrealtime;
	double td_onesecondminfps;
	double td_onesecondmaxfps;
	double td_onesecondavgfps;
	int td_onesecondavgcount;
	// LadyHavoc: pausedemo
	qbool demopaused;

	// sound mixer statistics for showsound display
	cl_soundstats_t soundstats;

	qbool connect_trying;
	int connect_remainingtries;
	double connect_nextsendtime;
	lhnetsocket_t *connect_mysocket;
	lhnetaddress_t connect_address;
	lhnetaddress_t rcon_address;
	// protocol version of the server we're connected to
	// (kept outside client_state_t because it's used between levels)
	protocolversion_t protocol;

#define MAX_RCONS 16
	int rcon_trying;
	lhnetaddress_t rcon_addresses[MAX_RCONS];
	char rcon_commands[MAX_RCONS][MAX_INPUTLINE];
	double rcon_timeout[MAX_RCONS];
	int rcon_ringpos;

// connection information
	// 0 to SIGNONS
	int signon;
	// network connection
	netconn_t *netcon;

	// download information
	// (note: qw_download variables are also used)
	cl_downloadack_t dp_downloadack[CL_MAX_DOWNLOADACKS];

	// input sequence numbers are not reset on level change, only connect
	unsigned int servermovesequence;

	// quakeworld stuff below

	// value of "qport" cvar at time of connection
	int qw_qport;
	// copied from cls.netcon->qw. variables every time they change, or set by demos (which have no cls.netcon)
	unsigned int qw_incoming_sequence;
	unsigned int qw_outgoing_sequence;

	// current file download buffer (only saved when file is completed)
	char qw_downloadname[MAX_QPATH];
	unsigned char *qw_downloadmemory;
	int qw_downloadmemorycursize;
	int qw_downloadmemorymaxsize;
	int qw_downloadnumber;
	int qw_downloadpercent;
	qw_downloadtype_t qw_downloadtype;
	// transfer rate display
	double qw_downloadspeedtime;
	int qw_downloadspeedcount;
	int qw_downloadspeedrate;
	qbool qw_download_deflate;

	// current file upload buffer (for uploading screenshots to server)
	unsigned char *qw_uploaddata;
	int qw_uploadsize;
	int qw_uploadpos;

	// user infostring
	// this normally contains the following keys in quakeworld:
	// password spectator name team skin topcolor bottomcolor rate noaim msg *ver *ip
	char userinfo[MAX_USERINFO_STRING];

	// extra user info for the "connect" command
	char connect_userinfo[MAX_USERINFO_STRING];

#ifdef CONFIG_VIDEO_CAPTURE
	// video capture stuff
	capturevideostate_t capturevideo;
#endif

	// crypto channel
	crypto_t crypto;

	// ProQuake compatibility stuff
	int proquake_servermod; // 0 = not proquake, 1 = proquake
	int proquake_serverversion; // actual proquake server version * 10 (3.40 = 34, etc)
	int proquake_serverflags; // 0 (PQF_CHEATFREE not supported)

	// don't write-then-read csprogs.dat (useful for demo playback)
	unsigned char *caughtcsprogsdata;
	fs_offset_t caughtcsprogsdatasize;

	int r_speeds_graph_length;
	int r_speeds_graph_current;
	int *r_speeds_graph_data;

	// graph scales
	int r_speeds_graph_datamin[r_stat_count];
	int r_speeds_graph_datamax[r_stat_count];
}
client_static_t;

extern client_static_t	cls;

//[515]: csqc
typedef struct
{
	qbool drawworld;
	qbool drawenginesbar;
	qbool drawcrosshair;
}csqc_vidvars_t;

typedef enum cl_parsingtextmode_e
{
	CL_PARSETEXTMODE_NONE,
	CL_PARSETEXTMODE_PING,
	CL_PARSETEXTMODE_STATUS,
	CL_PARSETEXTMODE_STATUS_PLAYERID,
	CL_PARSETEXTMODE_STATUS_PLAYERIP
}
cl_parsingtextmode_t;

typedef struct cl_locnode_s
{
	struct cl_locnode_s *next;
	char *name;
	vec3_t mins, maxs;
}
cl_locnode_t;

typedef struct showlmp_s
{
	qbool	isactive;
	float		x;
	float		y;
	char		label[32];
	char		pic[128];
}
showlmp_t;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct client_state_s
{
	// true if playing in a local game and no one else is connected
	int islocalgame;

	// send a clc_nop periodically until connected
	float sendnoptime;

	// current input being accumulated by mouse/joystick/etc input
	usercmd_t cmd;
	// latest moves sent to the server that have not been confirmed yet
	usercmd_t movecmd[CL_MAX_USERCMDS];

// information for local display
	// health, etc
	int stats[MAX_CL_STATS];
	float *statsf; // points to stats[] array
	// last known inventory bit flags, for blinking
	int olditems;
	// cl.time of acquiring item, for blinking
	float item_gettime[32];
	// last known STAT_ACTIVEWEAPON
	int activeweapon;
	// cl.time of changing STAT_ACTIVEWEAPON
	float weapontime;
	// use pain anim frame if cl.time < this
	float faceanimtime;
	// for stair smoothing
	float stairsmoothz;
	double stairsmoothtime;

	// color shifts for damage, powerups
	cshift_t cshifts[NUM_CSHIFTS];
	// and content types
	cshift_t prev_cshifts[NUM_CSHIFTS];

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  The server sets punchangle when
// the view is temporarily offset, and an angle reset commands at the start
// of each level and after teleporting.

	// mviewangles is read from demo
	// viewangles is either client controlled or lerped from mviewangles
	vec3_t mviewangles[2], viewangles;
	// update by server, used by qc to do weapon recoil
	vec3_t mpunchangle[2], punchangle;
	// update by server, can be used by mods to kick view around
	vec3_t mpunchvector[2], punchvector;
	// update by server, used for lean+bob (0 is newest)
	vec3_t mvelocity[2], velocity;
	// update by server, can be used by mods for zooming
	vec_t mviewzoom[2], viewzoom;
	// if true interpolation the mviewangles and other interpolation of the
	// player is disabled until the next network packet
	// this is used primarily by teleporters, and when spectating players
	// special checking of the old fixangle[1] is used to differentiate
	// between teleporting and spectating
	qbool fixangle[2];

	// client movement simulation
	// these fields are only updated by CL_ClientMovement (called by CL_SendMove after parsing each network packet)
	// set by CL_ClientMovement_Replay functions
	qbool movement_predicted;
	// if true the CL_ClientMovement_Replay function will update origin, etc
	qbool movement_replay;
	// simulated data (this is valid even if cl.movement is false)
	vec3_t movement_origin;
	vec3_t movement_velocity;
	// whether the replay should allow a jump at the first sequence
	qbool movement_replay_canjump;

	// previous gun angles (for leaning effects)
	vec3_t gunangles_prev;
	vec3_t gunangles_highpass;
	vec3_t gunangles_adjustment_lowpass;
	vec3_t gunangles_adjustment_highpass;
	// previous gun angles (for leaning effects)
	vec3_t gunorg_prev;
	vec3_t gunorg_highpass;
	vec3_t gunorg_adjustment_lowpass;
	vec3_t gunorg_adjustment_highpass;

// pitch drifting vars
	float idealpitch;
	float pitchvel;
	qbool nodrift;
	float driftmove;
	double laststop;

//[515]: added for csqc purposes
	float sensitivityscale;
	csqc_vidvars_t csqc_vidvars;	//[515]: these parms must be set to true by default
	qbool csqc_wantsmousemove;
	struct model_s *csqc_model_precache[MAX_MODELS];

	// local amount for smoothing stepups
	//float crouch;

	// sent by server
	qbool paused;
	qbool onground;
	qbool inwater;

	// used by bob
	qbool oldonground;
	double lastongroundtime;
	double hitgroundtime;
	float bob2_smooth;
	float bobfall_speed;
	float bobfall_swing;
	double calcrefdef_prevtime;

	// don't change view angle, full screen, etc
	int intermission;
	// latched at intermission start
	double completed_time;

	// the timestamp of the last two messages
	double mtime[2];

	// clients view of time, time should be between mtime[0] and mtime[1] to
	// generate a lerp point for other data, oldtime is the previous frame's
	// value of time, frametime is the difference between time and oldtime
	// note: cl.time may be beyond cl.mtime[0] if packet loss is occuring, it
	// is only forcefully limited when a packet is received
	double time, oldtime;
	// how long it has been since the previous client frame in real time
	// (not game time, for that use cl.time - cl.oldtime)
	double realframetime;

	// used by cl_nettimesyncboundmode 7
#define NUM_TS_ERRORS 32 // max 256
	unsigned char ts_error_num;
	float ts_error_stor[NUM_TS_ERRORS];

	// fade var for fading while dead
	float deathfade;

	// motionblur alpha level variable
	float motionbluralpha;

	// copy of realtime from last recieved message, for net trouble icon
	float last_received_message;

// information that is static for the entire time connected to a server
	struct model_s *model_precache[MAX_MODELS];
	struct sfx_s *sound_precache[MAX_SOUNDS];

	// FIXME: this is a lot of memory to be keeping around, this really should be dynamically allocated and freed somehow
	char model_name[MAX_MODELS][MAX_QPATH];
	char sound_name[MAX_SOUNDS][MAX_QPATH];

	// for display on solo scoreboard
	char worldmessage[MAX_QPATH]; // map title (not related to filename)
	// variants of map name
	char worldbasename[MAX_QPATH]; // %s
	char worldname[MAX_QPATH]; // maps/%s.bsp
	char worldnamenoextension[MAX_QPATH]; // maps/%s
	// cl_entitites[cl.viewentity] = player
	int viewentity;
	// the real player entity (normally same as viewentity,
	// different than viewentity if mod uses chasecam or other tricks)
	int realplayerentity;
	// this is updated to match cl.viewentity whenever it is in the clients
	// range, basically this is used in preference to cl.realplayerentity for
	// most purposes because when spectating another player it should show
	// their information rather than yours
	int playerentity;
	// max players that can be in this game
	int maxclients;
	// type of game (deathmatch, coop, singleplayer)
	int gametype;

	// models and sounds used by engine code (particularly cl_parse.c)
	model_t *model_bolt;
	model_t *model_bolt2;
	model_t *model_bolt3;
	model_t *model_beam;
	sfx_t *sfx_wizhit;
	sfx_t *sfx_knighthit;
	sfx_t *sfx_tink1;
	sfx_t *sfx_ric1;
	sfx_t *sfx_ric2;
	sfx_t *sfx_ric3;
	sfx_t *sfx_r_exp3;
	// indicates that the file "sound/misc/talk2.wav" was found (for use by team chat messages)
	qbool foundteamchatsound;

// refresh related state

	// cl_entitites[0].model
	struct model_s *worldmodel;

	// the gun model
	entity_t viewent;

	// cd audio
	int cdtrack, looptrack;

// frag scoreboard

	// [cl.maxclients]
	scoreboard_t *scores;

	// keep track of svc_print parsing state (analyzes ping reports and status reports)
	cl_parsingtextmode_t parsingtextmode;
	int parsingtextplayerindex;
	// set by scoreboard code when sending ping command, this causes the next ping results to be hidden
	// (which could eat the wrong ping report if the player issues one
	//  manually, but they would still see a ping report, just a later one
	//  caused by the scoreboard code rather than the one they intentionally
	//  issued)
	int parsingtextexpectingpingforscores;

	// entity database stuff
	// latest received entity frame numbers
#define LATESTFRAMENUMS 32
	int latestframenumsposition;
	int latestframenums[LATESTFRAMENUMS];
	unsigned int latestsendnums[LATESTFRAMENUMS];
	entityframe_database_t *entitydatabase;
	entityframe4_database_t *entitydatabase4;
	entityframeqw_database_t *entitydatabaseqw;

	// keep track of quake entities because they need to be killed if they get stale
	int lastquakeentity;
	unsigned char isquakeentity[MAX_EDICTS];

	// bounding boxes for clientside movement
	vec3_t playerstandmins;
	vec3_t playerstandmaxs;
	vec3_t playercrouchmins;
	vec3_t playercrouchmaxs;

	// old decals are killed based on this
	unsigned int decalsequence;

	int max_entities;
	int max_csqcrenderentities;
	int max_static_entities;
	int max_effects;
	int max_beams;
	int max_dlights;
	int max_lightstyle;
	int max_brushmodel_entities;
	int max_particles;
	int max_showlmps;

	entity_t *entities;
	entity_render_t *csqcrenderentities;
	unsigned char *entities_active;
	entity_t *static_entities;
	cl_effect_t *effects;
	beam_t *beams;
	dlight_t *dlights;
	lightstyle_t *lightstyle;
	int *brushmodel_entities;
	particle_t *particles;
	showlmp_t *showlmps;

	int num_entities;
	int num_static_entities;
	int num_brushmodel_entities;
	int num_effects;
	int num_beams;
	int num_dlights;
	int num_particles;
	int num_decals;
	int num_showlmps;

	double particles_updatetime;
	double decals_updatetime;
	int free_particle;
	int free_decal;

	// cl_serverextension_download feature
	int loadmodel_current;
	int downloadmodel_current;
	int loadmodel_total;
	int loadsound_current;
	int downloadsound_current;
	int loadsound_total;
	qbool downloadcsqc;
	qbool loadcsqc;
	qbool loadbegun;
	qbool loadfinished;

	// quakeworld stuff

	// local copy of the server infostring
	char qw_serverinfo[MAX_SERVERINFO_STRING];

	// time of last qw "pings" command sent to server while showing scores
	double last_ping_request;

	// used during connect
	int qw_servercount;

	// updated from serverinfo
	int qw_teamplay;

	// unused: indicates whether the player is spectating
	// use cl.scores[cl.playerentity-1].qw_spectator instead
	//qbool qw_spectator;

	// time accumulated since an input packet was sent
	float timesincepacket;
	// how many optimally timed inputs we sent since we received an update from the server
	uint8_t opt_inputs_since_update;

	// movement parameters for client prediction
	unsigned int moveflags;
	float movevars_wallfriction;
	float movevars_waterfriction;
	float movevars_friction;
	float movevars_timescale;
	float movevars_gravity;
	float movevars_stopspeed;
	float movevars_maxspeed;
	float movevars_spectatormaxspeed;
	float movevars_accelerate;
	float movevars_airaccelerate;
	float movevars_wateraccelerate;
	float movevars_entgravity;
	float movevars_jumpvelocity;
	float movevars_edgefriction;
	float movevars_maxairspeed;
	float movevars_stepheight;
	float movevars_airaccel_qw;
	float movevars_airaccel_qw_stretchfactor;
	float movevars_airaccel_sideways_friction;
	float movevars_airstopaccelerate;
	float movevars_airstrafeaccelerate;
	float movevars_maxairstrafespeed;
	float movevars_airstrafeaccel_qw;
	float movevars_aircontrol;
	float movevars_aircontrol_power;
	float movevars_aircontrol_penalty;
	float movevars_warsowbunny_airforwardaccel;
	float movevars_warsowbunny_accel;
	float movevars_warsowbunny_topspeed;
	float movevars_warsowbunny_turnaccel;
	float movevars_warsowbunny_backtosideratio;
	float movevars_ticrate;
	float movevars_airspeedlimit_nonqw;

	// models used by qw protocol
	int qw_modelindex_spike;
	int qw_modelindex_player;
	int qw_modelindex_flag;
	int qw_modelindex_s_explod;

	vec3_t qw_intermission_origin;
	vec3_t qw_intermission_angles;

	// 255 is the most nails the QW protocol could send
	int qw_num_nails;
	vec_t qw_nails[255][6];

	float qw_weaponkick;

	unsigned int qw_validsequence;

	unsigned int qw_deltasequence[QW_UPDATE_BACKUP];

	// csqc stuff:
	// server entity number corresponding to a clientside entity
	unsigned short csqc_server2csqcentitynumber[MAX_EDICTS];
	vec3_t csqc_vieworigin;
	vec3_t csqc_viewangles;
	vec3_t csqc_vieworiginfromengine;
	vec3_t csqc_viewanglesfromengine;
	matrix4x4_t csqc_viewmodelmatrixfromengine;
	qbool csqc_usecsqclistener;
	matrix4x4_t csqc_listenermatrix;
	char csqc_printtextbuf[MAX_INPUTLINE];
	size_t csqc_printtextbuf_len; ///< strlen

	// collision culling data
	world_t world;

	// loc file stuff (points and boxes describing locations in the level)
	cl_locnode_t *locnodes;
	// this is updated to cl.movement_origin whenever health is < 1
	// used by %d print in say/say_team messages if cl_locs_enable is on
	vec3_t lastdeathorigin;

	// processing buffer used by R_BuildLightMap, reallocated as needed,
	// freed on each level change
	size_t buildlightmapmemorysize;
	unsigned char *buildlightmapmemory;

	// used by EntityState5_ReadUpdate
	skeleton_t *engineskeletonobjects;

	// used by underwater sound filter (snd_waterfx)
	qbool view_underwater;
}
client_state_t;

//
// cvars
//
extern cvar_t cl_name;
extern cvar_t cl_color;
extern cvar_t cl_rate;
extern cvar_t cl_rate_burstsize;
extern cvar_t cl_pmodel;
extern cvar_t cl_playermodel;
extern cvar_t cl_playerskin;

extern cvar_t rcon_password;
extern cvar_t rcon_address;

extern cvar_t cl_upspeed;
extern cvar_t cl_forwardspeed;
extern cvar_t cl_backspeed;
extern cvar_t cl_sidespeed;

extern cvar_t cl_movespeedkey;

extern cvar_t cl_yawspeed;
extern cvar_t cl_pitchspeed;

extern cvar_t cl_anglespeedkey;

extern cvar_t cl_autofire;

extern cvar_t cl_shownet;
extern cvar_t cl_nettimesyncfactor;
extern cvar_t cl_nettimesyncboundmode;
extern cvar_t cl_nettimesyncboundtolerance;

extern cvar_t cl_pitchdriftspeed;
extern cvar_t lookspring;
extern cvar_t lookstrafe;
extern cvar_t sensitivity;

extern cvar_t freelook;

extern cvar_t m_pitch;
extern cvar_t m_yaw;
extern cvar_t m_forward;
extern cvar_t m_side;

extern cvar_t cl_autodemo;
extern cvar_t cl_autodemo_nameformat;
extern cvar_t cl_autodemo_delete;
extern cvar_t cl_startdemos;

extern cvar_t r_draweffects;

extern cvar_t cl_explosions_alpha_start;
extern cvar_t cl_explosions_alpha_end;
extern cvar_t cl_explosions_size_start;
extern cvar_t cl_explosions_size_end;
extern cvar_t cl_explosions_lifetime;
extern cvar_t cl_stainmaps;
extern cvar_t cl_stainmaps_clearonload;

extern cvar_t cl_prydoncursor;
extern cvar_t cl_prydoncursor_notrace;

extern cvar_t cl_locs_enable;

extern cvar_t cl_areagrid_link_SOLID_NOT;

extern client_state_t cl;

extern void CL_AllocLightFlash (entity_render_t *ent, matrix4x4_t *matrix, float radius, float red, float green, float blue, float decay, float lifetime, char *cubemapname, int style, int shadowenable, vec_t corona, vec_t coronasizescale, vec_t ambientscale, vec_t diffusescale, vec_t specularscale, int flags);

cl_locnode_t *CL_Locs_FindNearest(const vec3_t point);
void CL_Locs_FindLocationName(char *buffer, size_t buffersize, vec3_t point);

//=============================================================================

//
// cl_main
//

double CL_Frame(double time);

void CL_Shutdown (void);
void CL_Init (void);

void CL_StartVideo(void);

void CL_EstablishConnection(const char *host, int firstarg);

void CL_Disconnect(void);
void CL_DisconnectEx(qbool kicked, const char *reason, ... );
void CL_Disconnect_f(cmd_state_t *cmd);

void CL_UpdateRenderEntity(entity_render_t *ent);
void CL_SetEntityColormapColors(entity_render_t *ent, int colormap);
void CL_UpdateViewEntities(void);

//
// cl_input
//
typedef struct kbutton_s
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
}
kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendMove (void);

void CL_ValidateState(entity_state_t *s);
void CL_MoveLerpEntityStates(entity_t *ent);
void CL_LerpUpdate(entity_t *e);
void CL_ParseTEnt (void);
void CL_NewBeam (int ent, vec3_t start, vec3_t end, model_t *m, int lightning);
void CL_RelinkBeams (void);
void CL_Beam_CalculatePositions (const beam_t *b, vec3_t start, vec3_t end);
void CL_ClientMovement_Replay(void);

void CL_ClearTempEntities (void);
entity_render_t *CL_NewTempEntity (double shadertime);

void CL_Effect(vec3_t org, model_t *model, int startframe, int framecount, float framerate);

void CL_ClearState (void);
void CL_ExpandEntities(int num);
void CL_ExpandCSQCRenderEntities(int num);
void CL_SetInfo(const char *key, const char *value, qbool send, qbool allowstarkey, qbool allowmodel, qbool quiet);


void CL_UpdateWorld (void);
void CL_WriteToServer (void);
void CL_Input (void);
extern int cl_ignoremousemoves;


float CL_KeyState (kbutton_t *key);

//
// cl_cmd.c
//
/// adds the string as a clc_stringcmd to the client message.
/// (used when there is no reason to generate a local command to do it)
void CL_ForwardToServer (const char *s);

/// adds the current command line as a clc_stringcmd to the client message.
/// things like godmode, noclip, etc, are commands directed to the server,
/// so when they are typed in at the console, they will need to be forwarded.
void CL_ForwardToServer_f (cmd_state_t *cmd);
void CL_InitCommands(void);


//
// cl_demo.c
//
void CL_StopPlayback(void);
void CL_ReadDemoMessage(void);
void CL_WriteDemoMessage(sizebuf_t *mesage);

void CL_CutDemo(unsigned char **buf, fs_offset_t *filesize);
void CL_PasteDemo(unsigned char **buf, fs_offset_t *filesize);

void CL_PlayDemo(const char *demo);
void CL_NextDemo(void);
void CL_Stop_f(cmd_state_t *cmd);
void CL_Record_f(cmd_state_t *cmd);
void CL_PlayDemo_f(cmd_state_t *cmd);
void CL_TimeDemo_f(cmd_state_t *cmd);

void CL_Demo_Init(void);


#include "cl_screen.h"

extern qbool sb_showscores;

typedef enum waterlevel_e
{
	WATERLEVEL_NONE,
	WATERLEVEL_WETFEET,
	WATERLEVEL_SWIMMING,
	WATERLEVEL_SUBMERGED
}
waterlevel_t;

typedef struct cl_clientmovement_state_s
{
	// entity to be ignored for movement
	struct prvm_edict_s *self;
	// position
	vec3_t origin;
	vec3_t velocity;
	// current bounding box (different if crouched vs standing)
	vec3_t mins;
	vec3_t maxs;
	// currently on the ground
	qbool onground;
	// currently crouching
	qbool crouched;
	// what kind of water (SUPERCONTENTS_LAVA for instance)
	int watertype;
	// how deep
	waterlevel_t waterlevel;
	// weird hacks when jumping out of water
	// (this is in seconds and counts down to 0)
	float waterjumptime;

	// user command
	usercmd_t cmd;
}
cl_clientmovement_state_t;
void CL_ClientMovement_PlayerMove_Frame(cl_clientmovement_state_t *s);

// warpzone prediction hack (CSQC builtin)
void CL_RotateMoves(const matrix4x4_t *m);

typedef enum meshname_e {
	MESH_SCENE, // CSQC R_PolygonBegin, potentially also engine particles and debug stuff
	MESH_UI,
	NUM_MESHENTITIES,
} meshname_t;
extern entity_t cl_meshentities[NUM_MESHENTITIES];
extern model_t cl_meshentitymodels[NUM_MESHENTITIES];
extern const char *cl_meshentitynames[NUM_MESHENTITIES];
#define CL_Mesh_Scene() (&cl_meshentitymodels[MESH_SCENE])
#define CL_Mesh_UI() (&cl_meshentitymodels[MESH_UI])
void CL_MeshEntities_Init(void);
void CL_MeshEntities_Scene_Clear(void);
void CL_MeshEntities_Scene_AddRenderEntity(void);
void CL_MeshEntities_Scene_FinalizeRenderEntity(void);
void CL_UpdateEntityShading(void);

void CL_NewFrameReceived(int num);
void CL_ParseEntityLump(char *entitystring);
void CL_FindNonSolidLocation(const vec3_t in, vec3_t out, vec_t radius);
void CL_RelinkLightFlashes(void);
void CL_Beam_AddPolygons(const beam_t *b);
void CL_UpdateMoveVars(void);
void CL_Locs_Reload_f(cmd_state_t *cmd);

#endif

