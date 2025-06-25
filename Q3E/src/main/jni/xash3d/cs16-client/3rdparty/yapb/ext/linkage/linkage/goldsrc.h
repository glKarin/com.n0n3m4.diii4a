/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

//
// Heavily stripped down Half-Life SDK Headers amalgamated into a single include,
// usable only for simple extension libraries, not for a mod development.
//
// License: https://raw.githubusercontent.com/ValveSoftware/halflife/master/LICENSE
//

#pragma once

constexpr auto INTERFACE_VERSION = 140;

constexpr auto VIEW_FIELD_FULL = -1.0f;
constexpr auto VIEW_FIELD_WIDE = -0.7f;
constexpr auto VIEW_FIELD_NARROW = 0.7f;
constexpr auto VIEW_FIELD_ULTRA_NARROW = 0.9f;

constexpr auto FTRACE_SIMPLEBOX = cr::bit (0);

constexpr auto WALKMOVE_NORMAL = 0;
constexpr auto WALKMOVE_WORLDONLY = 1;
constexpr auto WALKMOVE_CHECKONLY = 2;

constexpr auto MOVETYPE_NONE = 0;
constexpr auto MOVETYPE_WALK = 3;
constexpr auto MOVETYPE_STEP = 4;
constexpr auto MOVETYPE_FLY = 5;
constexpr auto MOVETYPE_TOSS = 6;
constexpr auto MOVETYPE_PUSH = 7;
constexpr auto MOVETYPE_NOCLIP = 8;
constexpr auto MOVETYPE_FLYMISSILE = 9;
constexpr auto MOVETYPE_BOUNCE = 10;
constexpr auto MOVETYPE_BOUNCEMISSILE = 11;
constexpr auto MOVETYPE_FOLLOW = 12;
constexpr auto MOVETYPE_PUSHSTEP = 13;

constexpr auto SOLID_NOT = 0;
constexpr auto SOLID_TRIGGER = 1;
constexpr auto SOLID_BBOX = 2;
constexpr auto SOLID_SLIDEBOX = 3;
constexpr auto SOLID_BSP = 4;

constexpr auto DEAD_NO = 0;
constexpr auto DEAD_DYING = 1;
constexpr auto DEAD_DEAD = 2;
constexpr auto DEAD_RESPAWNABLE = 3;
constexpr auto DEAD_DISCARDBODY = 4;

constexpr auto DAMAGE_NO = 0;
constexpr auto DAMAGE_YES = 1;
constexpr auto DAMAGE_AIM = 2;

constexpr auto EF_BRIGHTFIELD = 1;
constexpr auto EF_MUZZLEFLASH = 2;
constexpr auto EF_BRIGHTLIGHT = 4;
constexpr auto EF_DIMLIGHT = 8;
constexpr auto EF_INVLIGHT = 16;
constexpr auto EF_NOINTERP = 32;
constexpr auto EF_LIGHT = 64;
constexpr auto EF_NODRAW = 128;

constexpr auto TE_BEAMPOINTS = 0;
constexpr auto TE_BEAMENTPOINT = 1;
constexpr auto TE_GUNSHOT = 2;
constexpr auto TE_EXPLOSION = 3;
constexpr auto TE_TAREXPLOSION = 4;
constexpr auto TE_SMOKE = 5;
constexpr auto TE_TRACER = 6;
constexpr auto TE_LIGHTNING = 7;
constexpr auto TE_BEAMENTS = 8;
constexpr auto TE_SPARKS = 9;
constexpr auto TE_LAVASPLASH = 10;
constexpr auto TE_TELEPORT = 11;
constexpr auto TE_EXPLOSION2 = 12;
constexpr auto TE_BSPDECAL = 13;
constexpr auto TE_IMPLOSION = 14;
constexpr auto TE_SPRITETRAIL = 15;
constexpr auto TE_BEAM = 16;
constexpr auto TE_SPRITE = 17;
constexpr auto TE_BEAMSPRITE = 18;
constexpr auto TE_BEAMTORUS = 19;
constexpr auto TE_BEAMDISK = 20;
constexpr auto TE_BEAMCYLINDER = 21;
constexpr auto TE_BEAMFOLLOW = 22;
constexpr auto TE_GLOWSPRITE = 23;
constexpr auto TE_BEAMRING = 24;
constexpr auto TE_STREAK_SPLASH = 25;
constexpr auto TE_BEAMHOSE = 26;
constexpr auto TE_DLIGHT = 27;
constexpr auto TE_ELIGHT = 28;
constexpr auto TE_TEXTMESSAGE = 29;
constexpr auto TE_LINE = 30;
constexpr auto TE_BOX = 31;
constexpr auto TE_KILLBEAM = 99;
constexpr auto TE_LARGEFUNNEL = 100;
constexpr auto TE_BLOODSTREAM = 101;
constexpr auto TE_SHOWLINE = 102;
constexpr auto TE_BLOOD = 103;
constexpr auto TE_DECAL = 104;
constexpr auto TE_FIZZ = 105;
constexpr auto TE_MODEL = 106;
constexpr auto TE_EXPLODEMODEL = 107;
constexpr auto TE_BREAKMODEL = 108;
constexpr auto TE_GUNSHOTDECAL = 109;
constexpr auto TE_SPRITE_SPRAY = 110;
constexpr auto TE_ARMOR_RICOCHET = 111;
constexpr auto TE_PLAYERDECAL = 112;
constexpr auto TE_BUBBLES = 113;
constexpr auto TE_BUBBLETRAIL = 114;
constexpr auto TE_BLOODSPRITE = 115;
constexpr auto TE_WORLDDECAL = 116;
constexpr auto TE_WORLDDECALHIGH = 117;
constexpr auto TE_DECALHIGH = 118;
constexpr auto TE_PROJECTILE = 119;
constexpr auto TE_SPRAY = 120;
constexpr auto TE_PLAYERSPRITES = 121;
constexpr auto TE_PARTICLEBURST = 122;
constexpr auto TE_FIREFIELD = 123;
constexpr auto TE_PLAYERATTACHMENT = 124;
constexpr auto TE_KILLPLAYERATTACHMENTS = 125;
constexpr auto TE_MULTIGUNSHOT = 126;
constexpr auto TE_USERTRACER = 127;

constexpr auto TE_EXPLFLAG_NONE = 0;
constexpr auto TE_EXPLFLAG_NOADDITIVE = 1;
constexpr auto TE_EXPLFLAG_NODLIGHTS = 2;
constexpr auto TE_EXPLFLAG_NOSOUND = 4;
constexpr auto TE_EXPLFLAG_NOPARTICLES = 8;

constexpr auto TEFIRE_FLAG_ALLFLOAT = 1;
constexpr auto TEFIRE_FLAG_SOMEFLOAT = 2;
constexpr auto TEFIRE_FLAG_LOOP = 4;
constexpr auto TEFIRE_FLAG_ALPHA = 8;
constexpr auto TEFIRE_FLAG_PLANAR = 16;

constexpr auto MSG_BROADCAST = 0;
constexpr auto MSG_ONE = 1;
constexpr auto MSG_ALL = 2;
constexpr auto MSG_INIT = 3;
constexpr auto MSG_PVS = 4;
constexpr auto MSG_PAS = 5;
constexpr auto MSG_PVS_R = 6;
constexpr auto MSG_PAS_R = 7;
constexpr auto MSG_ONE_UNRELIABLE = 8;
constexpr auto MSG_SPEC = 9;

constexpr auto CONTENTS_EMPTY = -1;
constexpr auto CONTENTS_SOLID = -2;
constexpr auto CONTENTS_WATER = -3;
constexpr auto CONTENTS_SLIME = -4;
constexpr auto CONTENTS_LAVA = -5;
constexpr auto CONTENTS_SKY = -6;
constexpr auto CONTENTS_LADDER = -16;
constexpr auto CONTENT_FLYFIELD = -17;
constexpr auto CONTENT_GRAVITY_FLYFIELD = -18;
constexpr auto CONTENT_FOG = -19;
constexpr auto CONTENT_EMPTY = -1;
constexpr auto CONTENT_SOLID = -2;
constexpr auto CONTENT_WATER = -3;
constexpr auto CONTENT_SLIME = -4;
constexpr auto CONTENT_LAVA = -5;
constexpr auto CONTENT_SKY = -6;

constexpr auto CHAN_AUTO = 0;
constexpr auto CHAN_WEAPON = 1;
constexpr auto CHAN_VOICE = 2;
constexpr auto CHAN_ITEM = 3;
constexpr auto CHAN_BODY = 4;
constexpr auto CHAN_STREAM = 5;
constexpr auto CHAN_STATIC = 6;
constexpr auto CHAN_NETWORKVOICE_BASE = 7;
constexpr auto CHAN_NETWORKVOICE_END = 500;

constexpr auto ATTN_NONE = 0.0f;
constexpr auto ATTN_NORM = 0.8f;
constexpr auto ATTN_IDLE = 2.0f;
constexpr auto ATTN_STATIC = 1.25f;

constexpr auto PITCH_NORM = 100;
constexpr auto PITCH_LOW = 95;
constexpr auto PITCH_HIGH = 120;

constexpr auto VOL_NORM = 1.0;
constexpr auto PLAT_LOW_TRIGGER = 1;

constexpr auto SF_TRAIN_WAIT_RETRIGGER = 1;
constexpr auto SF_TRAIN_START_ON = 4;
constexpr auto SF_TRAIN_PASSABLE = 8;

enum input_flags_e {
   IN_ATTACK = cr::bit (0),
   IN_JUMP = cr::bit (1),
   IN_DUCK = cr::bit (2),
   IN_FORWARD = cr::bit (3),
   IN_BACK = cr::bit (4),
   IN_USE = cr::bit (5),
   IN_CANCEL = cr::bit (6),
   IN_LEFT = cr::bit (7),
   IN_RIGHT = cr::bit (8),
   IN_MOVELEFT = cr::bit (9),
   IN_MOVERIGHT = cr::bit (10),
   IN_ATTACK2 = cr::bit (11),
   IN_RUN = cr::bit (12),
   IN_RELOAD = cr::bit (13),
   IN_ALT1 = cr::bit (14),
   IN_SCORE = cr::bit (15)
};

enum snd_flags_e {
   SND_SPAWNING = cr::bit (8),
   SND_STOP = cr::bit (5),
   SND_CHANGE_VOL = cr::bit (6),
   SND_CHANGE_PITCH = cr::bit (7)
};

enum hidehud_flags {
   HIDEHUD_WEAPONS = cr::bit (0),
   HIDEHUD_FLASHLIGHT = cr::bit (1),
   HIDEHUD_ALL = cr::bit (2),
   HIDEHUD_HEALTH = cr::bit (3)
};

enum cvar_flags_e {
   FCVAR_ARCHIVE = cr::bit (0),
   FCVAR_USERINFO = cr::bit (1),
   FCVAR_SERVER = cr::bit (2),
   FCVAR_EXTDLL = cr::bit (3),
   FCVAR_CLIENTDLL = cr::bit (4),
   FCVAR_PROTECTED = cr::bit (5),
   FCVAR_SPONLY = cr::bit (6),
   FCVAR_PRINTABLEONLY = cr::bit (7),
   FCVAR_UNLOGGED = cr::bit (8)
};

enum edict_flags_e {
   FL_FLY = cr::bit (0),
   FL_SWIM = cr::bit (1),
   FL_CONVEYOR = cr::bit (2),
   FL_CLIENT = cr::bit (3),
   FL_INWATER = cr::bit (4),
   FL_MONSTER = cr::bit (5),
   FL_GODMODE = cr::bit (6),
   FL_NOTARGET = cr::bit (7),
   FL_SKIPLOCALHOST = cr::bit (8),
   FL_ONGROUND = cr::bit (9),
   FL_PARTIALGROUND = cr::bit (10),
   FL_WATERJUMP = cr::bit (11),
   FL_FROZEN = cr::bit (12),
   FL_FAKECLIENT = cr::bit (13),
   FL_DUCKING = cr::bit (14),
   FL_FLOAT = cr::bit (15),
   FL_GRAPHED = cr::bit (16),
   FL_IMMUNE_WATER = cr::bit (17),
   FL_IMMUNE_SLIME = cr::bit (18),
   FL_IMMUNE_LAVA = cr::bit (19),
   FL_PROXY = cr::bit (20),
   FL_ALWAYSTHINK = cr::bit (21),
   FL_BASEVELOCITY = cr::bit (22),
   FL_MONSTERCLIP = cr::bit (23),
   FL_ONTRAIN = cr::bit (24),
   FL_WORLDBRUSH = cr::bit (25),
   FL_SPECTATOR = cr::bit (26),
   FL_CUSTOMENTITY = cr::bit (29),
   FL_KILLME = cr::bit (30),
   FL_DORMANT = cr::bit (31)
};

enum item_flag_e {
   ITEM_FLAG_SELECTONEMPTY = cr::bit (0),
   ITEM_FLAG_NOAUTORELOAD = cr::bit (1),
   ITEM_FLAG_NOAUTOSWITCHEMPTY =  cr::bit (2),
   ITEM_FLAG_LIMITINWORLD = cr::bit (3),
   ITEM_FLAG_EXHAUSTIBLE = cr::bit (4),
   ITEM_FLAG_NOFIREUNDERWATER = cr::bit (5)
};

constexpr auto VEC_HULL_MIN = cr::Vector (-16.0f, -16.0f, -36.0f);
constexpr auto VEC_HULL_MAX = cr::Vector (16.0f, 16.0f, 36.0f);
constexpr auto VEC_HUMAN_HULL_MIN = cr::Vector (-16.0f, -16.0f, 0.0f);
constexpr auto VEC_HUMAN_HULL_MAX = cr::Vector (16.0f, 16.0f, 72.0f);
constexpr auto VEC_HUMAN_HULL_DUCK = cr::Vector (16.0f, 16.0f, 36.0f);
constexpr auto VEC_VIEW = cr::Vector (0.0f, 0.0f, 28.0f);
constexpr auto VEC_DUCK_HULL_MIN = cr::Vector (-16.0f, -16.0f, -18.0f);
constexpr auto VEC_DUCK_HULL_MAX = cr::Vector (16.0f, 16.0f, 18.0f);
constexpr auto VEC_DUCK_VIEW = cr::Vector (0.0f, 0.0f, 12.0f);

constexpr auto BREAK_TYPEMASK = 0x4F;
constexpr auto BREAK_GLASS = 0x01;
constexpr auto BREAK_METAL = 0x02;
constexpr auto BREAK_FLESH = 0x04;
constexpr auto BREAK_WOOD = 0x08;

constexpr auto BREAK_SMOKE = 0x10;
constexpr auto BREAK_TRANS = 0x20;
constexpr auto BREAK_CONCRETE = 0x40;
constexpr auto BREAK_2 = 0x80;

constexpr auto BOUNCE_GLASS = BREAK_GLASS;
constexpr auto BOUNCE_METAL = BREAK_METAL;
constexpr auto BOUNCE_FLESH = BREAK_FLESH;
constexpr auto BOUNCE_WOOD = BREAK_WOOD;
constexpr auto BOUNCE_SHRAP = 0x10;
constexpr auto BOUNCE_SHELL = 0x20;
constexpr auto BOUNCE_CONCRETE = BREAK_CONCRETE;
constexpr auto BOUNCE_SHOTSHELL = 0x80;

constexpr auto TE_BOUNCE_NULL = 0;
constexpr auto TE_BOUNCE_SHELL = 1;
constexpr auto TE_BOUNCE_SHOTSHELL = 2;

constexpr auto MAX_WEAPON_SLOTS = 5;
constexpr auto MAX_ITEM_TYPES = 6;
constexpr auto MAX_ITEMS = 5;

constexpr auto MAX_AMMO_TYPES = 32;
constexpr auto MAX_AMMO_SLOTS = 32;

constexpr auto HUD_PRINTNOTIFY = 1;
constexpr auto HUD_PRINTCONSOLE = 2;
constexpr auto HUD_PRINTTALK = 3;
constexpr auto HUD_PRINTCENTER = 4;

constexpr auto WEAPON_SUIT = 31;

constexpr auto NUM_AMBIENTS = 4;
constexpr auto MAX_MAP_HULLS = 4;
constexpr auto MAX_PHYSINFO_STRING = 256;
constexpr auto MAX_PHYSENTS = 600;
constexpr auto MAX_MOVEENTS = 64;
constexpr auto MAX_LIGHTSTYLES = 64;
constexpr auto MAX_LIGHTSTYLEVALUE = 256;
constexpr auto MAX_ENT_LEAFS = 48;
constexpr auto MAX_LIGHTMAPS = 4;
constexpr auto SURF_DRAWTILED = 0x20;

constexpr auto AMBIENT_SOUND_STATIC = 0;
constexpr auto AMBIENT_SOUND_EVERYWHERE = 1;
constexpr auto AMBIENT_SOUND_SMALLRADIUS = 2;
constexpr auto AMBIENT_SOUND_MEDIUMRADIUS = 4;
constexpr auto AMBIENT_SOUND_LARGERADIUS = 8;
constexpr auto AMBIENT_SOUND_START_SILENT = 16;
constexpr auto AMBIENT_SOUND_NOT_LOOPING = 32;

constexpr auto LFO_SQUARE = 1;
constexpr auto LFO_TRIANGLE = 2;
constexpr auto LFO_RANDOM = 3;

constexpr auto SF_BRUSH_ROTATE_Y_AXIS = 0;
constexpr auto SF_BRUSH_ROTATE_INSTANT = 1;
constexpr auto SF_BRUSH_ROTATE_BACKWARDS = 2;
constexpr auto SF_BRUSH_ROTATE_Z_AXIS = 4;
constexpr auto SF_BRUSH_ROTATE_X_AXIS = 8;
constexpr auto SF_PENDULUM_AUTO_RETURN = 16;
constexpr auto SF_PENDULUM_PASSABLE = 32;

constexpr auto SF_BRUSH_ROTATE_SMALLRADIUS = 128;
constexpr auto SF_BRUSH_ROTATE_MEDIUMRADIUS = 256;
constexpr auto SF_BRUSH_ROTATE_LARGERADIUS = 512;

constexpr auto PUSH_BLOCK_ONLY_X = 1;
constexpr auto PUSH_BLOCK_ONLY_Y = 2;

constexpr auto SVC_PINGS = 17;
constexpr auto SVC_TEMPENTITY = 23;
constexpr auto SVC_CENTERPRINT = 26;
constexpr auto SVC_INTERMISSION = 30;
constexpr auto SVC_CDTRACK = 32;
constexpr auto SVC_WEAPONANIM = 35;
constexpr auto SVC_ROOMTYPE = 37;
constexpr auto SVC_DIRECTOR = 51;

constexpr auto SF_TRIGGER_ALLOWMONSTERS = 1;
constexpr auto SF_TRIGGER_NOCLIENTS = 2;
constexpr auto SF_TRIGGER_PUSHABLES = 4;

constexpr auto SF_BREAK_TRIGGER_ONLY = 1;
constexpr auto SF_BREAK_TOUCH = 2;
constexpr auto SF_BREAK_PRESSURE = 4;
constexpr auto SF_BREAK_CROWBAR = 256;
constexpr auto SF_PUSH_BREAKABLE = 128;
constexpr auto SF_LIGHT_START_OFF = 1;
constexpr auto SF_TRIG_PUSH_ONCE = 1;

constexpr auto SPAWNFLAG_NOMESSAGE = 1;
constexpr auto SPAWNFLAG_NOTOUCH = 1;
constexpr auto SPAWNFLAG_DROIDONLY = 4;

constexpr auto SPAWNFLAG_USEONLY = 1;

constexpr auto TELE_PLAYER_ONLY = 1;
constexpr auto TELE_SILENT = 2;

enum {
   kRenderNormal,
   kRenderTransColor,
   kRenderTransTexture,
   kRenderGlow,
   kRenderTransAlpha,
   kRenderTransAdd
};

enum {
   kRenderFxNone = 0,
   kRenderFxPulseSlow,
   kRenderFxPulseFast,
   kRenderFxPulseSlowWide,
   kRenderFxPulseFastWide,
   kRenderFxFadeSlow,
   kRenderFxFadeFast,
   kRenderFxSolidSlow,
   kRenderFxSolidFast,
   kRenderFxStrobeSlow,
   kRenderFxStrobeFast,
   kRenderFxStrobeFaster,
   kRenderFxFlickerSlow,
   kRenderFxFlickerFast,
   kRenderFxNoDissipation,
   kRenderFxDistort,
   kRenderFxHologram,
   kRenderFxDeadPlayer,
   kRenderFxExplode,
   kRenderFxGlowShell,
   kRenderFxClampMinScale
};

enum IGNORE_MONSTERS {
   ignore_monsters = 1,
   dont_ignore_monsters = 0,
   missile = 2
};

enum IGNORE_GLASS {
   ignore_glass = 1,
   dont_ignore_glass = 0
};

enum HULL {
   point_hull = 0,
   human_hull = 1,
   large_hull = 2,
   head_hull = 3
};

enum ALERT_TYPE {
   at_notice,
   at_console,
   at_aiconsole,
   at_warning,
   at_error,
   at_logged
};

enum PRINT_TYPE {
   print_console,
   print_center,
   print_chat
};

enum FORCE_TYPE {
   force_exactfile,
   force_model_samebounds,
   force_model_specifybounds
};

enum HLBool : int32_t {
   HLFalse,
   HLTrue
};

typedef int qboolean;
typedef uint8_t byte;
typedef float vec_t;
typedef cr::Vector vec3_t;

struct edict_t;
struct playermove_t;

template <typename StrType, typename IndexType> class HLString final {
public:
   using Type = StrType;

private:
   StrType offset {};

public:
   explicit constexpr HLString () : offset (0) {}
   constexpr HLString (StrType offset) : offset (offset) {}
   constexpr HLString (const char *str) : offset (to (str)) {}
   ~HLString () = default;

public:
   constexpr const char *chars (IndexType shift = 0) const;
   constexpr cr::StringRef str (IndexType shift = 0);

public:
   constexpr static StrType to (const char *str);
   constexpr static const char *from (StrType offset);

public:
   constexpr operator StrType () const {
      return offset;
   }

   constexpr HLString &operator = (const HLString &rhs) {
      offset = rhs.offset;
      return *this;
   }

   constexpr HLString &operator = (const char *str) {
      offset = to (str);
      return *this;
   }
};

#if defined(CR_ARCH_X64)
using string_t = HLString <int32_t, int32_t>;
#else
using string_t = HLString <uint32_t, int32_t>;
#endif

struct cvar_t {
   const char *name {};
   const char *string {};

   int flags {};
   float value {};
   cvar_t *next {};
};

struct hudtextparms_t {
   float x {};
   float y {};
   int effect {};
   uint8_t r1 {}, g1 {}, b1 {}, a1 {};
   uint8_t r2 {}, g2 {}, b2 {}, a2 {};
   float fadeinTime {};
   float fadeoutTime {};
   float holdTime {};
   float fxTime {};
   int channel {};
};

struct TraceResult {
   int fAllSolid {};
   int fStartSolid {};
   int fInOpen {};
   int fInWater {};
   float flFraction {};
   vec3_t vecEndPos {};
   float flPlaneDist {};
   vec3_t vecPlaneNormal {};
   edict_t *pHit {};
   int iHitgroup {};
};

struct KeyValueData {
   char const *szClassName {};
   char const *szKeyName {};
   char *szValue {};
   int32_t fHandled {};
};

struct usercmd_t {
   short lerp_msec {};
   byte msec {};
   vec3_t viewangles {};
   float forwardmove {};
   float sidemove {};
   float upmove {};
   byte lightlevel {};
   unsigned short buttons {};
   byte impulse {};
   byte weaponselect {};
   int impact_index {};
   vec3_t impact_position {};
};

struct mplane_t {
   vec3_t normal {};
   float dist {};
   byte type {};
   byte signbits {};
   byte pad[2] {};
};

struct mvertex_t {
   vec3_t position {};
};

struct mtexinfo_t {
   float vecs[2][4] {};
   float mipadjust {};
   struct texture_t *texture {};
   int flags {};
};

struct mnode_t {
   int contents {};
   int visframe {};
   short minmaxs[6] {};
   mnode_t *parent {};
   mplane_t *plane {};
   mnode_t *children[2] {};
   uint16_t firstsurface {};
   uint16_t numsurfaces {};
};

struct mnode_hw_t {
   int contents {};
   int visframe {};
   float minmaxs[6] {};
   mnode_t *parent {};
   mplane_t *plane {};
   mnode_t *children[2] {};
   uint16_t firstsurface {};
   uint16_t numsurfaces {};
};

struct color24 {
   byte r {}, g {}, b {};
};

struct mdisplaylist_t {
   unsigned int gl_displaylist {};
   int rendermode {};
   float scrolloffset {};
   int renderDetailTexture {};
};

struct msurface_hw_t {
   int visframe {};
   mplane_t *plane {};
   int flags {};
   int firstedge {};
   int numedges {};
   short texturemins[2] {};
   short extents[2] {};
   int light_s {}, light_t {};
   struct glpoly_t *polys {};
   struct msurface_s *texturechain {};
   mtexinfo_t *texinfo {};
   int dlightframe {};
   int dlightbits {};
   int lightmaptexturenum {};
   byte styles[MAX_LIGHTMAPS] {};
   int cached_light[MAX_LIGHTMAPS] {};
   qboolean cached_dlight {};
   color24 *samples {};
   struct decal_t *pdecals {};
};

struct msurface_hw_hl25_t : public msurface_hw_t {
   mdisplaylist_t displaylist {};
};

struct msurface_t {
   int visframe {};
   int dlightframe {};
   int dlightbits {};
   mplane_t *plane {};
   int flags {};
   int firstedge {};
   int numedges {};
   struct surfcache_s *cachespots[4] {};
   short texturemins[2] {};
   short extents[2] {};
   mtexinfo_t *texinfo {};
   byte styles[MAX_LIGHTMAPS] {};
   color24 *samples {};
   struct decal_t *pdecals {};
};

struct cache_user_t {
   void *data {};
};

struct hull_t {
   struct dclipnode_t *clipnodes {};
   mplane_t *planes {};
   int firstclipnode {};
   int lastclipnode {};
   vec3_t clip_mins {};
   vec3_t clip_maxs {};
};

#if defined (CR_ARCH_X64)
using synctype_mempool_t = uint32_t;
#else
using synctype_mempool_t = void *;
#endif

struct model_t {
   char name[64] {};
   qboolean needload {};
   int type {};
   int numframes {};
   synctype_mempool_t synctype_mempool {};
   int flags {};
   vec3_t mins {}, maxs {};
   float radius {};
   int firstmodelsurface {};
   int nummodelsurfaces {};
   int numsubmodels {};
   struct dmodel_t *submodels {};
   int numplanes {};
   mplane_t *planes {};
   int numleafs {};
   struct mleaf_t *leafs {};
   int numvertexes {};
   mvertex_t *vertexes {};
   int numedges {};
   struct medge_t *edges {};
   int numnodes {};
   mnode_t *nodes {};
   int numtexinfo {};
   mtexinfo_t *texinfo {};
   int numsurfaces {};
   msurface_t *surfaces {};
   int numsurfedges {};
   int *surfedges {};
   int numclipnodes {};
   struct dclipnode_t *clipnodes {};
   int nummarksurfaces {};
   msurface_t **marksurfaces {};
   hull_t hulls[MAX_MAP_HULLS] {};
   int numtextures {};
   texture_t **textures {};
   byte *visdata {};
   color24 *lightdata {};
   char *entities {};
   cache_user_t cache {};
};

struct studiohdr_t {
   int id {};
   int version {};
   char name[64] {};
   int length {};
   vec3_t eyeposition {};
   vec3_t min {};
   vec3_t max {};
   vec3_t bbmin {};
   vec3_t bbmax {};
   int flags {};
   int numbones {};
   int boneindex {};
   int numbonecontrollers {};
   int bonecontrollerindex {};
   int numhitboxes {};
   int hitboxindex {};
   int numseq {};
   int seqindex {};
   int numseqgroups {};
   int seqgroupindex {};
   int numtextures {};
   int textureindex {};
   int texturedataindex {};
   int numskinref {};
   int numskinfamilies {};
   int skinindex {};
   int numbodyparts {};
   int bodypartindex {};
   int numattachments {};
   int attachmentindex {};
   int soundtable {};
   int soundindex {};
   int soundgroups {};
   int soundgroupindex {};
   int numtransitions {};
   int transitionindex {};
};

struct mstudiobbox_t {
   int bone {};
   int group {};
   vec3_t bbmin {};
   vec3_t bbmax {};
};

struct lightstyle_t {
   int length {};
   char map[MAX_LIGHTSTYLES] {};
};

struct physent_t {
   char name[32] {};
   int player {};
   vec3_t origin {};
   model_t *model {};
};

struct playermove_t {
   int player_index {};
   qboolean server {};
   qboolean multiplayer {};
   float time {};
   float frametime {};
   vec3_t forward {}, right {}, up {};
   vec3_t origin {};
   vec3_t angles {};
   vec3_t oldangles {};
   vec3_t velocity {};
   vec3_t movedir {};
   vec3_t basevelocity {};
   vec3_t view_ofs {};
   float flDuckTime {};
   qboolean bInDuck {};
   int flTimeStepSound {};
   int iStepLeft {};
   float flFallVelocity {};
   vec3_t punchangle {};
   float flSwimTime {};
   float flNextPrimaryAttack {};
   int effects {};
   int flags {};
   int usehull {};
   float gravity {};
   float friction {};
   int oldbuttons {};
   float waterjumptime {};
   qboolean dead {};
   int deadflag {};
   int spectator {};
   int movetype {};
   int onground {};
   int waterlevel {};
   int watertype {};
   int oldwaterlevel {};
   char sztexturename[256] {};
   char chtexturetype {};
   float maxspeed {};
   float clientmaxspeed {};
   int iuser1 {};
   int iuser2 {};
   int iuser3 {};
   int iuser4 {};
   float fuser1 {};
   float fuser2 {};
   float fuser3 {};
   float fuser4 {};
   vec3_t vuser1 {};
   vec3_t vuser2 {};
   vec3_t vuser3 {};
   vec3_t vuser4 {};
   int numphysent {};
   physent_t physents[MAX_PHYSENTS] {};
};

struct globalvars_t {
   float time {};
   float frametime {};
   float force_retouch {};
   string_t mapname {};
   string_t startspot {};
   float deathmatch {};
   float coop {};
   float teamplay {};
   float serverflags {};
   float found_secrets {};
   vec3_t v_forward {};
   vec3_t v_up {};
   vec3_t v_right {};
   float trace_allsolid {};
   float trace_startsolid {};
   float trace_fraction {};
   vec3_t trace_endpos {};
   vec3_t trace_plane_normal {};
   float trace_plane_dist {};
   edict_t *trace_ent {};
   float trace_inopen {};
   float trace_inwater {};
   int trace_hitgroup {};
   int trace_flags {};
   int msg_entity {};
   int cdAudioTrack {};
   int maxClients {};
   int maxEntities {};
   const char *pStringBase {};
   void *pSaveData {};
   vec3_t vecLandmarkOffset {};
};

struct entvars_t {
   string_t classname {};
   string_t globalname {};
   vec3_t origin {};
   vec3_t oldorigin {};
   vec3_t velocity {};
   vec3_t basevelocity {};
   vec3_t clbasevelocity {};
   vec3_t movedir {};
   vec3_t angles {};
   vec3_t avelocity {};
   vec3_t punchangle {};
   vec3_t v_angle {};
   vec3_t endpos {};
   vec3_t startpos {};
   float impacttime {};
   float starttime {};
   int fixangle {};
   float idealpitch {};
   float pitch_speed {};
   float ideal_yaw {};
   float yaw_speed {};
   int modelindex {};
   string_t model {};
   string_t viewmodel {};
   string_t weaponmodel {};
   vec3_t absmin {};
   vec3_t absmax {};
   vec3_t mins {};
   vec3_t maxs {};
   vec3_t size {};
   float ltime {};
   float nextthink {};
   int movetype {};
   int solid {};
   int skin {};
   int body {};
   int effects {};
   float gravity {};
   float friction {};
   int light_level {};
   int sequence {};
   int gaitsequence {};
   float frame {};
   float animtime {};
   float framerate {};
   uint8_t controller[4] {};
   uint8_t blending[2] {};
   float scale {};
   int rendermode {};
   float renderamt {};
   vec3_t rendercolor {};
   int renderfx {};
   float health {};
   float frags {};
   int weapons {};
   float takedamage {};
   int deadflag {};
   vec3_t view_ofs {};
   int button {};
   int impulse {};
   edict_t *chain {};
   edict_t *dmg_inflictor {};
   edict_t *enemy {};
   edict_t *aiment {};
   edict_t *owner {};
   edict_t *groundentity {};
   int spawnflags {};
   int flags {};
   int colormap {};
   int team {};
   float max_health {};
   float teleport_time {};
   float armortype {};
   float armorvalue {};
   int waterlevel {};
   int watertype {};
   string_t target {};
   string_t targetname {};
   string_t netname {};
   string_t message {};
   float dmg_take {};
   float dmg_save {};
   float dmg {};
   float dmgtime {};
   string_t noise {};
   string_t noise1 {};
   string_t noise2 {};
   string_t noise3 {};
   float speed {};
   float air_finished {};
   float pain_finished {};
   float radsuit_finished {};
   edict_t *pContainingEntity {};
   int playerclass {};
   float maxspeed {};
   float fov {};
   int weaponanim {};
   int pushmsec {};
   int bInDuck {};
   int flTimeStepSound {};
   int flSwimTime {};
   int flDuckTime {};
   int iStepLeft {};
   float flFallVelocity {};
   int gamestate {};
   int oldbuttons {};
   int groupinfo {};
   int iuser1 {};
   int iuser2 {};
   int iuser3 {};
   int iuser4 {};
   float fuser1 {};
   float fuser2 {};
   float fuser3 {};
   float fuser4 {};
   vec3_t vuser1 {};
   vec3_t vuser2 {};
   vec3_t vuser3 {};
   vec3_t vuser4 {};
   edict_t *euser1 {};
   edict_t *euser2 {};
   edict_t *euser3 {};
   edict_t *euser4 {};
};

struct link_t {
   link_t *prev {}, *next {};
};

struct plane_t {
   vec3_t normal {};
   float dist {};
};

struct edict_t {
   int free {};
   int serialnumber {};
   link_t area {};
   int headnode {};
   int num_leafs {};
   short leafnums[MAX_ENT_LEAFS] {};
   float freetime {};
   void *pvPrivateData {};
   entvars_t v {};
};

struct enginefuncs_t {
   int (*pfnPrecacheModel)(const char *s);
   int (*pfnPrecacheSound)(const char *s);
   void (*pfnSetModel)(edict_t *e, const char *m);
   int (*pfnModelIndex)(const char *m);
   int (*pfnModelFrames)(int modelIndex);
   void (*pfnSetSize)(edict_t *e, const float *rgflMin, const float *rgflMax);
   void (*pfnChangeLevel)(char *s1, char *s2);
   void (*pfnGetSpawnParms)(edict_t *ent);
   void (*pfnSaveSpawnParms)(edict_t *ent);
   float (*pfnVecToYaw)(const float *rgflVector);
   void (*pfnVecToAngles)(const float *rgflVectorIn, float *rgflVectorOut);
   void (*pfnMoveToOrigin)(edict_t *ent, const float *pflGoal, float dist, int iMoveType);
   void (*pfnChangeYaw)(edict_t *ent);
   void (*pfnChangePitch)(edict_t *ent);
   edict_t *(*pfnFindEntityByString)(edict_t *pentEdictStartSearchAfter, const char *pszField, const char *pszValue);
   int (*pfnGetEntityIllum)(edict_t *pEnt);
   edict_t *(*pfnFindEntityInSphere)(edict_t *pentEdictStartSearchAfter, const float *org, float rad);
   edict_t *(*pfnFindClientInPVS)(edict_t *ent);
   edict_t *(*pfnEntitiesInPVS)(edict_t *pplayer);
   void (*pfnMakeVectors)(const float *rgflVector);
   void (*pfnAngleVectors)(const float *rgflVector, float *forward, float *right, float *up);
   edict_t *(*pfnCreateEntity)();
   void (*pfnRemoveEntity)(edict_t *e);
   edict_t *(*pfnCreateNamedEntity)(string_t className);
   void (*pfnMakeStatic)(edict_t *ent);
   int (*pfnEntIsOnFloor)(edict_t *e);
   int (*pfnDropToFloor)(edict_t *e);
   int (*pfnWalkMove)(edict_t *ent, float yaw, float dist, int mode);
   void (*pfnSetOrigin)(edict_t *e, const float *rgflOrigin);
   void (*pfnEmitSound)(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch);
   void (*pfnEmitAmbientSound)(edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch);
   void (*pfnTraceLine)(const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr);
   void (*pfnTraceToss)(edict_t *pent, edict_t *pentToIgnore, TraceResult *ptr);
   int (*pfnTraceMonsterHull)(edict_t *ent, const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr);
   void (*pfnTraceHull)(const float *v1, const float *v2, int fNoMonsters, int hullNumber, edict_t *pentToSkip, TraceResult *ptr);
   void (*pfnTraceModel)(const float *v1, const float *v2, int hullNumber, edict_t *pent, TraceResult *ptr);
   const char *(*pfnTraceTexture)(edict_t *pTextureEntity, const float *v1, const float *v2);
   void (*pfnTraceSphere)(const float *v1, const float *v2, int fNoMonsters, float radius, edict_t *pentToSkip, TraceResult *ptr);
   void (*pfnGetAimVector)(edict_t *ent, float speed, float *rgflReturn);
   void (*pfnServerCommand)(char *str);
   void (*pfnServerExecute)();
   void (*pfnClientCommand)(edict_t *ent, char const *szFmt, ...);
   void (*pfnParticleEffect)(const float *org, const float *dir, float color, float count);
   void (*pfnLightStyle)(int style, char *val);
   int (*pfnDecalIndex)(const char *name);
   int (*pfnPointContents)(const float *rgflVector);
   void (*pfnMessageBegin)(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed);
   void (*pfnMessageEnd)();
   void (*pfnWriteByte)(int value);
   void (*pfnWriteChar)(int value);
   void (*pfnWriteShort)(int value);
   void (*pfnWriteLong)(int value);
   void (*pfnWriteAngle)(float flValue);
   void (*pfnWriteCoord)(float flValue);
   void (*pfnWriteString)(const char *sz);
   void (*pfnWriteEntity)(int value);
   void (*pfnCVarRegister)(cvar_t *pCvar);
   float (*pfnCVarGetFloat)(const char *szVarName);
   const char *(*pfnCVarGetString)(const char *szVarName);
   void (*pfnCVarSetFloat)(const char *szVarName, float flValue);
   void (*pfnCVarSetString)(const char *szVarName, const char *szValue);
   void (*pfnAlertMessage)(ALERT_TYPE atype, const char *szFmt, ...);
   void (*pfnEngineFprintf)(void *pfile, char *szFmt, ...);
   void *(*pfnPvAllocEntPrivateData)(edict_t *ent, int32_t cb);
   void *(*pfnPvEntPrivateData)(edict_t *ent);
   void (*pfnFreeEntPrivateData)(edict_t *ent);
   const char *(*pfnSzFromIndex)(int stingPtr);
   string_t::Type (*pfnAllocString)(const char *szValue);
   struct entvars_s *(*pfnGetVarsOfEnt)(edict_t *ent);
   edict_t *(*pfnPEntityOfEntOffset)(int iEntOffset);
   int (*pfnEntOffsetOfPEntity)(const edict_t *ent);
   int (*pfnIndexOfEdict)(const edict_t *ent);
   edict_t *(*pfnPEntityOfEntIndex)(int entIndex);
   edict_t *(*pfnFindEntityByVars)(struct entvars_s *pvars);
   void *(*pfnGetModelPtr)(edict_t *ent);
   int (*pfnRegUserMsg)(const char *pszName, int iSize);
   void (*pfnAnimationAutomove)(const edict_t *ent, float flTime);
   void (*pfnGetBonePosition)(const edict_t *ent, int iBone, float *rgflOrigin, float *rgflAngles);
   uint32_t (*pfnFunctionFromName)(const char *pName);
   const char *(*pfnNameForFunction)(uint32_t function);
   void (*pfnClientPrintf)(edict_t *ent, PRINT_TYPE ptype, const char *szMsg);
   void (*pfnServerPrint)(const char *szMsg);
   const char *(*pfnCmd_Args)();
   const char *(*pfnCmd_Argv)(int argc);
   int (*pfnCmd_Argc)();
   void (*pfnGetAttachment)(const edict_t *ent, int iAttachment, float *rgflOrigin, float *rgflAngles);
   void (*pfnCRC32_Init)(uint32_t *pulCRC);
   void (*pfnCRC32_ProcessBuffer)(uint32_t *pulCRC, void *p, int len);
   void (*pfnCRC32_ProcessByte)(uint32_t *pulCRC, uint8_t ch);
   uint32_t (*pfnCRC32_Final)(uint32_t pulCRC);
   int32_t (*pfnRandomLong)(int32_t lLow, int32_t lHigh);
   float (*pfnRandomFloat)(float flLow, float flHigh);
   void (*pfnSetView)(const edict_t *client, const edict_t *pViewent);
   float (*pfnTime)();
   void (*pfnCrosshairAngle)(const edict_t *client, float pitch, float yaw);
   uint8_t *(*pfnLoadFileForMe)(char const *szFilename, int *pLength);
   void (*pfnFreeFile)(void *buffer);
   void (*pfnEndSection)(const char *pszSectionName);
   int (*pfnCompareFileTime)(char *filename1, char *filename2, int *compare);
   void (*pfnGetGameDir)(char *szGetGameDir);
   void (*pfnCvar_RegisterVariable)(cvar_t *variable);
   void (*pfnFadeClientVolume)(const edict_t *ent, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds);
   void (*pfnSetClientMaxspeed)(const edict_t *ent, float fNewMaxspeed);
   edict_t *(*pfnCreateFakeClient)(const char *netname);
   void (*pfnRunPlayerMove)(edict_t *fakeclient, const float *viewangles, float forwardmove, float sidemove, float upmove, uint16_t buttons, uint8_t impulse, uint8_t msec);
   int (*pfnNumberOfEntities)();
   char *(*pfnGetInfoKeyBuffer)(edict_t *e);
   char *(*pfnInfoKeyValue)(char *infobuffer, char const *key);
   void (*pfnSetKeyValue)(char *infobuffer, char *key, char *value);
   void (*pfnSetClientKeyValue)(int clientIndex, char *infobuffer, char const *key, char const *value);
   int (*pfnIsMapValid)(const char *szFilename);
   void (*pfnStaticDecal)(const float *origin, int decalIndex, int entityIndex, int modelIndex);
   int (*pfnPrecacheGeneric)(char *s);
   int (*pfnGetPlayerUserId)(edict_t *e);
   void (*pfnBuildSoundMsg)(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *ed);
   int (*pfnIsDedicatedServer)();
   cvar_t *(*pfnCVarGetPointer)(const char *szVarName);
   unsigned int (*pfnGetPlayerWONId)(edict_t *e);
   void (*pfnInfo_RemoveKey)(char *s, const char *key);
   const char *(*pfnGetPhysicsKeyValue)(const edict_t *client, const char *key);
   void (*pfnSetPhysicsKeyValue)(const edict_t *client, const char *key, const char *value);
   const char *(*pfnGetPhysicsInfoString)(const edict_t *client);
   uint16_t (*pfnPrecacheEvent)(int type, const char *psz);
   void (*pfnPlaybackEvent)(int flags, const edict_t *pInvoker, uint16_t evIndexOfEntity, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
   uint8_t *(*pfnSetFatPVS)(float *org);
   uint8_t *(*pfnSetFatPAS)(float *org);
   int (*pfnCheckVisibility)(const edict_t *entity, uint8_t *pset);
   void (*pfnDeltaSetField)(struct delta_s *pFields, const char *fieldname);
   void (*pfnDeltaUnsetField)(struct delta_s *pFields, const char *fieldname);
   void (*pfnDeltaAddEncoder)(char *name, void (*conditionalencode)(struct delta_s *pFields, const uint8_t *from, const uint8_t *to));
   int (*pfnGetCurrentPlayer)();
   int (*pfnCanSkipPlayer)(const edict_t *player);
   int (*pfnDeltaFindField)(struct delta_s *pFields, const char *fieldname);
   void (*pfnDeltaSetFieldByIndex)(struct delta_s *pFields, int fieldNumber);
   void (*pfnDeltaUnsetFieldByIndex)(struct delta_s *pFields, int fieldNumber);
   void (*pfnSetGroupMask)(int mask, int op);
   int (*pfnCreateInstancedBaseline)(string_t classname, struct entity_state_s *baseline);
   void (*pfnCvar_DirectSet)(struct cvar_t *var, const char *value);
   void (*pfnForceUnmodified)(FORCE_TYPE type, float *mins, float *maxs, const char *szFilename);
   void (*pfnGetPlayerStats)(const edict_t *client, int *ping, int *packet_loss);
   void (*pfnAddServerCommand)(const char *cmd_name, void (*function)());
   int (*pfnVoice_GetClientListening)(int iReceiver, int iSender);
   int (*pfnVoice_SetClientListening)(int iReceiver, int iSender, int bListen);
   const char *(*pfnGetPlayerAuthId)(edict_t *e);
   struct sequenceEntry_s *(*pfnSequenceGet)(const char *fileName, const char *entryName);
   struct sentenceEntry_s *(*pfnSequencePickSentence)(const char *groupName, int pickMethod, int *picked);
   int (*pfnGetFileSize)(char *szFilename);
   unsigned int (*pfnGetApproxWavePlayLen)(const char *filepath);
   int (*pfnIsCareerMatch)();
   int (*pfnGetLocalizedStringLength)(const char *label);
   void (*pfnRegisterTutorMessageShown)(int mid);
   int (*pfnGetTimesTutorMessageShown)(int mid);
   void (*pfnProcessTutorMessageDecayBuffer)(int *buffer, int bufferLength);
   void (*pfnConstructTutorMessageDecayBuffer)(int *buffer, int bufferLength);
   void (*pfnResetTutorMessageDecayData)();
   void (*pfnQueryClientCVarValue)(const edict_t *player, const char *cvarName);
   void (*pfnQueryClientCVarValue2)(const edict_t *player, const char *cvarName, int requestID);
   int (*pfnCheckParm)(const char *pchCmdLineToken, char **ppnext);
};

struct gamefuncs_t {
   void (*pfnGameInit)();
   int (*pfnSpawn)(edict_t *pent);
   void (*pfnThink)(edict_t *pent);
   void (*pfnUse)(edict_t *pentUsed, edict_t *pentOther);
   void (*pfnTouch)(edict_t *pentTouched, edict_t *pentOther);
   void (*pfnBlocked)(edict_t *pentBlocked, edict_t *pentOther);
   void (*pfnKeyValue)(edict_t *pentKeyvalue, KeyValueData *pkvd);
   void (*pfnSave)(edict_t *pent, struct SAVERESTOREDATA *pSaveData);
   int (*pfnRestore)(edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity);
   void (*pfnSetAbsBox)(edict_t *pent);
   void (*pfnSaveWriteFields)(SAVERESTOREDATA *, const char *, void *, struct TYPEDESCRIPTION *, int);
   void (*pfnSaveReadFields)(SAVERESTOREDATA *, const char *, void *, TYPEDESCRIPTION *, int);
   void (*pfnSaveGlobalState)(SAVERESTOREDATA *);
   void (*pfnRestoreGlobalState)(SAVERESTOREDATA *);
   void (*pfnResetGlobalState)();
   int (*pfnClientConnect)(edict_t *ent, const char *pszName, const char *pszAddress, char szRejectReason[128]);
   void (*pfnClientDisconnect)(edict_t *ent);
   void (*pfnClientKill)(edict_t *ent);
   void (*pfnClientPutInServer)(edict_t *ent);
   void (*pfnClientCommand)(edict_t *ent);
   void (*pfnClientUserInfoChanged)(edict_t *ent, char *infobuffer);
   void (*pfnServerActivate)(edict_t *edictList, int edictCount, int clientMax);
   void (*pfnServerDeactivate)();
   void (*pfnPlayerPreThink)(edict_t *ent);
   void (*pfnPlayerPostThink)(edict_t *ent);
   void (*pfnStartFrame)();
   void (*pfnParmsNewLevel)();
   void (*pfnParmsChangeLevel)();
   const char *(*pfnGetGameDescription)();
   void (*pfnPlayerCustomization)(edict_t *ent, struct customization_t *pCustom);
   void (*pfnSpectatorConnect)(edict_t *ent);
   void (*pfnSpectatorDisconnect)(edict_t *ent);
   void (*pfnSpectatorThink)(edict_t *ent);
   void (*pfnSys_Error)(const char *error_string);
   void (*pfnPM_Move)(playermove_t *ppmove, int server);
   void (*pfnPM_Init)(playermove_t *ppmove);
   char (*pfnPM_FindTextureType)(char *name);
   void (*pfnSetupVisibility)(struct edict_s *pViewEntity, struct edict_s *client, uint8_t **pvs, uint8_t **pas);
   void (*pfnUpdateClientData)(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);
   int (*pfnAddToFullPack)(struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, uint8_t *pSet);
   void (*pfnCreateBaseline)(int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, float *player_mins, float *player_maxs);
   void (*pfnRegisterEncoders)();
   int (*pfnGetWeaponData)(struct edict_s *player, struct weapon_data_s *info);
   void (*pfnCmdStart)(const edict_t *player, usercmd_t *cmd, unsigned int random_seed);
   void (*pfnCmdEnd)(const edict_t *player);
   int (*pfnConnectionlessPacket)(const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size);
   int (*pfnGetHullBounds)(int hullnumber, float *mins, float *maxs);
   void (*pfnCreateInstancedBaselines)();
   int (*pfnInconsistentFile)(const struct edict_s *player, const char *szFilename, char *disconnect_message);
   int (*pfnAllowLagCompensation)();
};

struct newgamefuncs_t {
   void (*pfnOnFreeEntPrivateData)(edict_t *pEnt);
   void (*pfnGameShutdown)();
   int (*pfnShouldCollide)(edict_t *pentTouched, edict_t *pentOther);
   void (*pfnCvarValue)(const edict_t *pEnt, const char *value);
   void (*pfnCvarValue2)(const edict_t *pEnt, int requestID, const char *cvarName, const char *value);
};

extern globalvars_t *globals;
extern enginefuncs_t engfuncs;
extern gamefuncs_t dllapi;

template <typename StrType, typename IndexType>
inline constexpr const char *HLString <StrType, IndexType>::chars (IndexType shift) const {
   auto str = from (offset);
   return cr::strings.isEmpty (str) ? &cr::kNullChar : (str + shift);
}

template <typename StrType, typename IndexType>
inline constexpr cr::StringRef HLString <StrType, IndexType>::str (IndexType shift) {
   return chars (shift);
}

template <typename StrType, typename IndexType>
inline constexpr const char *HLString <StrType, IndexType>::from (StrType offset) {
   return globals->pStringBase + offset;
}

template <typename StrType, typename IndexType>
inline constexpr StrType HLString <StrType, IndexType>::to (const char *str) {
#if defined(CR_ARCH_X64)
   int64_t ptrdiff = str - from (0);

   if (ptrdiff > INT_MAX || ptrdiff < INT_MIN) {
      return engfuncs.pfnAllocString (str);
   }
   return static_cast <int> (ptrdiff);
#else
   return static_cast <StrType> (reinterpret_cast<uint64_t> (str) - reinterpret_cast<uint64_t> (globals->pStringBase));
#endif
}
