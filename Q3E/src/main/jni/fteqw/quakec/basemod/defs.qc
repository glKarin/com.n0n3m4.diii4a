#ifdef FTE
// use real int if using FTE type progs
#define INTEGER int
#else
#define INTEGER float
#endif

#define BOOL INTEGER


#define FALSE ((BOOL)0)
#define TRUE ((BOOL)1)

/*
==============================================================================

			SOURCE FOR GLOBALVARS_T C STRUCTURE

==============================================================================
*/

//
// system globals
//
entity          self;
entity          other;
entity          world;
float           time;
float           frametime;

#ifndef NETQUAKE
entity          newmis;                         // if this is set, the entity that just
								// run created a new missile that should
								// be simulated immediately
#endif

float           force_retouch;          // force all entities to touch triggers
								// next frame.  this is needed because
								// non-moving things don't normally scan
								// for triggers, and when a trigger is
								// created (like a teleport trigger), it
								// needs to catch everything.
								// decremented each frame, so set to 2
								// to guarantee everything is touched
string          mapname;

#ifdef NETQUAKE
float deathmatch;
float coop;
float teamplay;
#endif

float           serverflags;            // propagated from level to level, used to
								// keep track of completed episodes

float           total_secrets;
float           total_monsters;

float           found_secrets;          // number of secrets found
float           killed_monsters;        // number of monsters killed


// spawnparms are used to encode information about clients across server
// level changes
float           parm1, parm2, parm3, parm4, parm5, parm6, parm7, parm8, parm9, parm10, parm11, parm12, parm13, parm14, parm15, parm16;

//
// global variables set by built in functions
//      
vector          v_forward, v_up, v_right;       // set by makevectors()
	
// set by traceline / tracebox
float           trace_allsolid;
float           trace_startsolid;
float           trace_fraction;
vector          trace_endpos;
vector          trace_plane_normal;
float           trace_plane_dist;
entity          trace_ent;
float           trace_inopen;
float           trace_inwater;

entity          msg_entity;                             // destination of single entity writes

//
// required prog functions
//
void()          main;                                           // only for testing

void()          StartFrame;

void()          PlayerPreThink;
void()          PlayerPostThink;

void()          ClientKill;
void()          ClientConnect;
void()          PutClientInServer;              // call after setting the parm1... parms
void()          ClientDisconnect;

void()          SetNewParms;                    // called when a client first connects to
									// a server. sets parms so they can be
									// saved off for restarts

void()          SetChangeParms;                 // call to set parms for self so they can
									// be saved for a level transition


//================================================
void            end_sys_globals;                // flag for structure dumping
//================================================

/*
==============================================================================

			SOURCE FOR ENTVARS_T C STRUCTURE

==============================================================================
*/

//
// system fields (*** = do not set in prog code, maintained by C code)
//
.float          modelindex;             // *** model index in the precached list
.vector         absmin, absmax; // *** origin + mins / maxs

.float          ltime;                  // local time for entity
#ifndef NETQUAKE
.float          lastruntime;    // *** to allow entities to run out of sequence
#endif

.float          movetype;
.float          solid;

.vector         origin;                 // ***
.vector         oldorigin;              // ***
.vector         velocity;
.vector         angles;
.vector         avelocity;

#ifdef NETQUAKE
.vector punchangle;
#endif

.string         classname;              // spawn function
.string         model;
.float          frame;
.float          skin;
.float          effects;

.vector         mins, maxs;             // bounding box extents reletive to origin
.vector         size;                   // maxs - mins

.void()         touch;
.void()         use;
.void()         think;
.void()         blocked;                // for doors or plats, called when can't push other

.float          nextthink;
.entity         groundentity;

// stats
.float          health;
.float          frags;
.float          weapon;                 // one of the IT_SHOTGUN, etc flags
.string         weaponmodel;
.float          weaponframe;
.float          currentammo;
.float          ammo_shells, ammo_nails, ammo_rockets, ammo_cells;

.float          items;                  // bit flags

.float          takedamage;
.entity         chain;
.float          deadflag;

.vector         view_ofs;                       // add to origin to get eye point


.float          button0;                // fire
.float          button1;                // use
.float          button2;                // jump

.float          impulse;                // weapon changes

.float          fixangle;
.vector         v_angle;                // view / targeting angle for players

#ifdef NETQUAKE
.float idealpitch;
#endif

.string         netname;

.entity         enemy;

.float          flags;

.float          colormap;
.float          team;

.float          max_health;             // players maximum health is stored here

.float          teleport_time;  // don't back up

.float          armortype;              // save this fraction of incoming damage
.float          armorvalue;

.float          waterlevel;             // 0 = not in, 1 = feet, 2 = wast, 3 = eyes
.float          watertype;              // a contents value

.float          ideal_yaw;
.float          yaw_speed;

.entity         aiment;

.entity         goalentity;             // a movetarget or an enemy

.float          spawnflags;

.string         target;
.string         targetname;

// damage is accumulated through a frame. and sent as one single
// message, so the super shotgun doesn't generate huge messages
.float          dmg_take;
.float          dmg_save;
.entity         dmg_inflictor;

.entity         owner;          // who launched a missile
.vector         movedir;        // mostly for doors, but also used for waterjump

.string         message;                // trigger messages

.float          sounds;         // either a cd track number or sound number

.string         noise, noise1, noise2, noise3;  // contains names of wavs to play

//================================================
void            end_sys_fields;                 // flag for structure dumping
//================================================

/*
==============================================================================

				VARS NOT REFERENCED BY C CODE

==============================================================================
*/


//
// constants
//

// edict.flags
#define FL_FLY              1
#define FL_SWIM             2
#define FL_CLIENT           8 // set for all client edicts
#define FL_INWATER         16 // for enter / leave water splash
#define FL_MONSTER         32
#define FL_GODMODE         64 // player cheat
#define FL_NOTARGET       128 // player cheat
#define FL_ITEM           256 // extra wide size for bonus items
#define FL_ONGROUND       512 // standing on something
#define FL_PARTIALGROUND 1024 // not all corners are valid
#define FL_WATERJUMP     2048 // player jumping out of water
#define FL_JUMPRELEASED  4096 // for jump debouncing

// edict.movetype values
#define MOVETYPE_NONE           0 // never moves
#define MOVETYPE_WALK           3 // players only
#define MOVETYPE_STEP           4 // discrete, not real time unless fall
#define MOVETYPE_FLY            5
#define MOVETYPE_TOSS           6
#define MOVETYPE_PUSH           7 // no clip to world, push and crush
#define MOVETYPE_NOCLIP         8
#define MOVETYPE_FLYMISSILE     9 // fly with extra size against monsters
#define MOVETYPE_BOUNCE        10 

// edict.solid values
#define SOLID_NOT      0 // no interaction with other objects
#define SOLID_TRIGGER  1 // touch on edge, but not blocking
#define SOLID_BBOX     2 // touch on edge, block
#define SOLID_SLIDEBOX 3 // touch on edge, but not an onground
#define SOLID_BSP      4 // bsp clip, touch on edge, block


// deadflag values
#define DEAD_NO          0
#define DEAD_DYING       1
#define DEAD_DEAD        2
#define DEAD_RESPAWNABLE 3

// takedamage values
#define DAMAGE_NO  0
#define DAMAGE_YES 1
#define DAMAGE_AIM 2

// items
float   IT_AXE                                  = 4096;
float   IT_SHOTGUN                              = 1;
float   IT_SUPER_SHOTGUN                = 2;
float   IT_NAILGUN                              = 4;
float   IT_SUPER_NAILGUN                = 8;
float   IT_GRENADE_LAUNCHER             = 16;
float   IT_ROCKET_LAUNCHER              = 32;
float   IT_LIGHTNING                    = 64;
float   IT_EXTRA_WEAPON                 = 128;

float   IT_SHELLS                               = 256;
float   IT_NAILS                                = 512;
float   IT_ROCKETS                              = 1024;
float   IT_CELLS                                = 2048;

float   IT_ARMOR1                               = 8192;
float   IT_ARMOR2                               = 16384;
float   IT_ARMOR3                               = 32768;
float   IT_SUPERHEALTH                  = 65536;

float   IT_KEY1                                 = 131072;
float   IT_KEY2                                 = 262144;

float   IT_INVISIBILITY                 = 524288;
float   IT_INVULNERABILITY              = 1048576;
float   IT_SUIT                                 = 2097152;
float   IT_QUAD                                 = 4194304;

// point content values

float   CONTENT_EMPTY                   = -1;
float   CONTENT_SOLID                   = -2;
float   CONTENT_WATER                   = -3;
float   CONTENT_SLIME                   = -4;
float   CONTENT_LAVA                    = -5;
float   CONTENT_SKY                             = -6;

vector  VEC_ORIGIN = '0 0 0';
vector  VEC_HULL_MIN = '-16 -16 -24';
vector  VEC_HULL_MAX = '16 16 32';

vector  VEC_HULL2_MIN = '-32 -32 -24';
vector  VEC_HULL2_MAX = '32 32 64';

// protocol bytes
float   SVC_TEMPENTITY          = 23;
float   SVC_KILLEDMONSTER       = 27;
float   SVC_FOUNDSECRET         = 28;
float   SVC_INTERMISSION        = 30;
float   SVC_FINALE                      = 31;
float   SVC_CDTRACK                     = 32;
float   SVC_SELLSCREEN          = 33;
float   SVC_SMALLKICK           = 34;
float   SVC_BIGKICK                     = 35;
float   SVC_MUZZLEFLASH         = 39;

float   TE_SPIKE                = 0;
float   TE_SUPERSPIKE   = 1;
float   TE_GUNSHOT              = 2;
float   TE_EXPLOSION    = 3;
float   TE_TAREXPLOSION = 4;
float   TE_LIGHTNING1   = 5;
float   TE_LIGHTNING2   = 6;
float   TE_WIZSPIKE             = 7;
float   TE_KNIGHTSPIKE  = 8;
float   TE_LIGHTNING3   = 9;
float   TE_LAVASPLASH   = 10;
float   TE_TELEPORT             = 11;
float   TE_BLOOD                = 12;
float   TE_LIGHTNINGBLOOD = 13;

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
#define CHAN_AUTO   0
#define CHAN_WEAPON 1
#define CHAN_VOICE  2
#define CHAN_ITEM   3
#define CHAN_BODY   4
// chan_no_phs_add is defined in engine.qc

#define ATTN_NONE   0.0 // no volume fall off due to range
#define ATTN_NORM   1.0 // small volume fall off (1000 range)
#define ATTN_IDLE   2.0 // medium volume fall off (500 range)
#define ATTN_STATIC 3.0 // large volume fall off (333 range)

// entity effects

//float EF_BRIGHTFIELD  = 1;
float   EF_MUZZLEFLASH  = 2;
float   EF_BRIGHTLIGHT  = 4;
float   EF_DIMLIGHT     = 8;
float   EF_FLAG1                = 16;
float   EF_FLAG2                = 32;
// GLQuakeWorld Stuff
// float 	EF_BLUE		=	64;	// Blue Globe effect for Quad
// float	EF_RED		=	128;	// Red Globe effect for Pentagram

// messages
#define MSG_BROADCAST 0 // unreliable to all
#define MSG_ONE       1 // reliable to one (msg_entity)
#define MSG_ALL       2 // reliable to all
#define MSG_INIT      3 // write to the init string
#define MSG_MULTICAST 4 // for multicast() call

// message levels
#define PRINT_LOW    0
#define PRINT_MEDIUM 1
#define PRINT_HIGH   2
#define PRINT_CHAT   3

// multicast sets
#define MULTICAST_ALL   0 // every client
#define MULTICAST_PHS   1 // within hearing
#define MULTICAST_PVS   2 // within sight
#define MULTICAST_ALL_R 3 // every client, reliable
#define MULTICAST_PHS_R 4 // within hearing, reliable
#define MULTICAST_PVS_R 5 // within sight, reliable




//================================================

//
// globals
//
// float   movedist;

string  string_null;    // null string, nothing should be held here

entity  activator;              // the entity that activated a trigger or brush

entity  damage_attacker;        // set by T_Damage
entity  damage_inflictor;       // set by T_Damage
INTEGER damage_mod;             // set by T_Damage

float   framecount;

//
// cvars checked each frame
//
float           timelimit;
float           fraglimit;
float           rj;

#ifndef NETQUAKE
// these variables are not a part of QW's system fields
float deathmatch;
float coop;
float teamplay;
#endif

float skill;

// coop spawning globals
entity  lastspawn;
float spotspawn;

entity shub; // boss entity

//================================================

#define WT_MEDIEVAL 0
#define WT_METAL    1
#define WT_BASE     2

//================================================

.string killtarget; // used by anything using SUB_UseTargets (items/triggers/etc)

.void(entity attacker, float damage) th_pain; // used by secret doors, monsters, players
.void()         th_die;   // used by anything damagable (doors/buttons/players/monsters/explobox/triggermultiple)

.float speed; // used with door/plats/fireball spawner, inherited
.string map; // used with world/trigger_changelevel, inherited

// used with monsters/players
.INTEGER ammo_shells_real;	// real shells count
.INTEGER ammo_nails_real;	// real nails count
.INTEGER ammo_rockets_real;	// real rockets count
.INTEGER ammo_cells_real;	// real cells count

// Zoid Additions
noref .float		maxspeed;		// Used to set Maxspeed on a player
noref .float		gravity;		// Gravity Multiplier (0 to 1.0)

.float          attack_finished; // used with lots of stuff...
.float          pain_finished; // used with monsters/players

.float          invincible_finished; // only used for players but object check needed for T_Damage
.float          super_damage_finished; // only used for players but object check needed for T_Damage

//
// misc
//
.float cnt; // misc flag used by trains/dropped pent and quad/monsters
	
// intermission
float   intermission_running;
float   intermission_exittime;

#ifdef NETQUAKE
entity newmis;
#endif

// extensions
.float alpha; // DP_ENT_ALPHA

// enums
// ai ranges
enum
{
	RANGE_MELEE,
	RANGE_NEAR,
	RANGE_MID,
	RANGE_FAR
};

// monster attack states
enum
{
	AS_NONE,
	AS_STRAIGHT,
	AS_SLIDING,
	AS_MELEE,
	AS_MISSILE
};

// door/plat states
enum {
	STATE_TOP,
	STATE_BOTTOM,
	STATE_UP,
	STATE_DOWN
};

// heal defines
enumflags {
	H_ROTTEN,
	H_MEGA
};

// weaponstate defines
enum {
	WS_IDLE,
	WS_FIRING1,
	WS_FIRING2 // used with nailgun
};

// ammo type defines
enum {
	AT_NONE,
	AT_SHELLS,
	AT_NAILS,
	AT_ROCKETS,
	AT_CELLS
};

// unions
//floats (includes vectors)
.union {
	struct { // fields used with world object
		float worldtype; 	// world type, 0=medieval 1=metal 2=base
	};
	struct { // fields used with triggers/doors/plats
		float delay;		// time from activation to firing
		float wait;			// time from firing to restarting
		float t_length;		// override length to move sideways
		float t_width; 		// override length to move upwards/downwards
		vector finaldest;		// used with SUB_CalcMove
		vector finalangle;	// used with SUB_CalcMove
		float count;		// for counting triggers
		float dmg;			// damage done by door/train/trigger hurt
		vector mangle;		// initial angle (doors/teleporter/intermission)
		float lip;			// position adjustment (func_door/func_button)
		float state;		// object state (doors/buttons/plats)
		vector pos1;		// top position (doors/buttons/plats)
		vector pos2;		// bottom position (doors/buttons/plats)
		float height;		// height (plats/trigger_monsterjump)
		vector dest1;		// passed to CalcMove (doors)
		vector dest2;		// passed to CalcMove (doors)
	};
	struct { // fields used with players
		float weaponframe_time;		// weapon frame advance time
		float suicide_time;     	// time to allow suicide after spawn
		float healdecay; 			// time when health will decay
		float show_hostile;		// used to alert monsters
		float lightning_sound;		// used for lightning sound playback
		float fly_sound;			// used with trigger_push and maintaining wind sound
		float invincible_time;		// pentagram state
		float invincible_sound;		// pentagram sound playback
		float invisible_time;		// ring state
		float invisible_sound;		// ring sound playback
		float invisible_finished;	// ring current duration
		float super_time;			// quad state
		float super_sound;		// quad sound playback
		float rad_time;			// rad suit state
		float radsuit_finished;		// rad suit current duration
		float dmgtime;			// time slime/lava will damage
		float swim_flag;			// swim sound playback
		float air_finished;		// when time > air_finished, start drowning
		float waterdmg;			// damage water will deal when drowning
		float jump_flag;			// last z velocity used for falling damage
	};
	struct { // fields used with items
		float healamount;	// amount healed with health item
		float ammo_count;	// ammo amount 
	};
	struct { // fields used with generic projectiles
		float damage_direct;		// damage done with a direct hit
		float damage_exp; 		// damage done from radius damage
		float radius_exp;  		// radius of radius damage
		float expire_time;		// time when projectile dies
		float proj_think_time;		// interval between thinks
	};
	struct { // fields used with lights
		float light_lev;	// not used by game, but parsed by light util
		float style;	// lightstyle to use for light
	};
	struct { // fields used with monsters
		float search_time;	// search time intervals
		float attack_state;	// current AI attack state
		float pausetime;		// time to pause for monsters
		float hknightattack;	// hell knight attack pattern
		float lefty;		// ai slide move
		float wizardidle;		// wizard idle sound timer
		float inpain;		// zombie in pain
	};
	// fields used with ambient sounds?
	/*
	struct {
		float waitmin;
		float waitmax;
		float distance;
		float volume;
	};
	*/
};

//integers
.union {

	struct { // fields used with players
		INTEGER weaponstate;		// firing state of current weapon
		INTEGER walkframe;		// used with walking animation
		INTEGER ammo_type;		// ammo type in use
	};
	struct { // fields used with bubbles spawned from drowning
		INTEGER bubble_count;   // keeps track of the number of bubbles
		INTEGER bubble_state;	// associated with bubble progression
	};
	struct { // fields used with items
		INTEGER healtype;	// type of health with health item
	};
	struct { // fields used with generic projectiles
		INTEGER mod_direct;		// mod type for direct hit
		INTEGER mod_exp; 			// mod type for radius damage
		INTEGER proj_effect;		// effect used with projectile
		INTEGER voided;			// projectile has been voided
	};
};

//functions
.union {
	struct { // fields used with triggers/doors/plats
		void() think1;		// used with SUB_CalcMove
	};
	struct { // fields used with generic projectiles
		void() proj_think;		// extra think function used with projectile
		float() proj_touch;		// extra touch function used with projectile
	};
	struct { // fields used with monsters
		void() th_stand;		// standing animation
		void() th_walk;		// walking animation
		void() th_run;		// running animation
		void() th_missile;	// ranged animation
		void() th_melee;		// melee animation
	};
};


//entities
.union {
	struct { // fields used with triggers/doors/plats
		entity trigger_field;	// used with linking (doors)
	};

	struct { // fields used with monsters
		entity oldenemy;		// old enemy
		entity movetarget;	// target entity to move to
	};
};

//strings
.union {
	struct { // fields used with world object
		string wad; //mute it
	};
	struct { // fields used with triggers/doors/plats
		string noise4;		// extra sound (doors)
	};
	struct { // fields used with items
		string mdl;		// model used with SUB_regen
	};
};

//mute those warnings (unions/structs do not support noref mid-struct, but a later def will propogate the noref flag)
noref .string wad;
noref .float light_lev;

//===========================================================================
	

//
// builtin functions
//

void(vector ang)        makevectors             = #1;           // sets v_forward, etc globals
void(entity e, vector o) setorigin      = #2;
void(entity e, string m) setmodel       = #3;           // set movetype and solid first
void(entity e, vector min, vector max) setsize = #4;
// #5 was removed
// void() break                                            = #6;
float() random                                          = #7;           // returns 0 - 1
void(entity e, float chan, string samp, float vol, float atten) sound = #8;
vector(vector v) normalize                      = #9;
void(string e) error                            = #10;
void(string e) objerror                         = #11;
float(vector v) vlen                            = #12;
float(vector v) vectoyaw                        = #13;
entity() spawn                                          = #14;
void(entity e) remove                           = #15;

// sets trace_* globals
// nomonsters can be:
// An entity will also be ignored for testing if forent == test,
// forent->owner == test, or test->owner == forent
// a forent of world is ignored
void(vector v1, vector v2, float nomonsters, entity forent) traceline = #16;    

entity() checkclient                            = #17;  // returns a client to look for
entity(entity start, .string fld, string match) find = #18;
string(string s) precache_sound         = #19;
string(string s) precache_model         = #20;
void(entity client, string s)stuffcmd = #21;
entity(vector org, float rad) findradius = #22;
// bprint moved to engine.qc
// sprint moved to engine.qc
void(string s) dprint                           = #25;
string(float f) ftos                            = #26;
string(vector v) vtos                           = #27;
void() coredump                                         = #28;          // prints all edicts
void() traceon                                          = #29;          // turns statment trace on
void() traceoff                                         = #30;
void(entity e) eprint                           = #31;          // prints an entire edict
float(float yaw, float dist) walkmove   = #32;  // returns TRUE or FALSE
// #33 was removed
float() droptofloor= #34;  // TRUE if landed on floor
void(float style, string value) lightstyle = #35;
float(float v) rint                                     = #36;          // round to nearest int
float(float v) floor                            = #37;          // largest integer <= v
float(float v) ceil                                     = #38;          // smallest integer >= v
// #39 was removed
float(entity e) checkbottom                     = #40;          // true if self is on ground
float(vector v) pointcontents           = #41;          // returns a CONTENT_*
// #42 was removed
float(float f) fabs = #43;
vector(entity e, float speed) aim = #44;                // returns the shooting vector
// cvar moved to engine.qc
void(string s) localcmd = #46;                                  // put string into local que
entity(entity e) nextent = #47;                                 // for looping through all ents
// #48 was removed
void() ChangeYaw = #49;                                         // turn towards self.ideal_yaw
											// at self.yaw_speed
// #50 was removed
vector(vector v) vectoangles                    = #51;

//
// direct client message generation
//
void(float to, float f) WriteByte               = #52;
void(float to, float f) WriteChar               = #53;
void(float to, float f) WriteShort              = #54;
void(float to, float f) WriteLong               = #55;
void(float to, float f) WriteCoord              = #56;
void(float to, float f) WriteAngle              = #57;
void(float to, string s) WriteString    = #58;
void(float to, entity s) WriteEntity    = #59;

// several removed

void(float step) movetogoal                             = #67;

string(string s) precache_file          = #68;  // no effect except for -copy
void(entity e) makestatic               = #69; 
void(string s) changelevel = #70;

//#71 was removed

void(string var, string val) cvar_set = #72;    // sets cvar.value

void(entity client, string s) centerprint = #73;        // sprint, but in middle

void(vector pos, string samp, float vol, float atten) ambientsound = #74;

string(string s) precache_model2        = #75;          // registered version only
string(string s) precache_sound2        = #76;          // registered version only
string(string s) precache_file2         = #77;          // registered version only

void(entity e) setspawnparms            = #78;          // set parm1... to the
												// values at level start
												// for coop respawn
// logfrag moved to engine.qc
// infokey moved to engine.qc
// stof moved to engine.qc

//============================================================================

//
// subs.qc
//
void(vector tdest, float tspeed, void() func) SUB_CalcMove;
void(entity ent, vector tdest, float tspeed, void() func) SUB_CalcMoveEnt;
void(vector destangle, float tspeed, void() func) SUB_CalcAngleMove;
void()  SUB_CalcMoveDone;
void() SUB_CalcAngleMoveDone;
void() SUB_Null;
void() SUB_UseTargets;
void() SUB_Remove;

//
//      combat.qc
//
void(entity targ, entity inflictor, entity attacker, float damage, INTEGER mod) T_Damage;

float (entity e, float healamount, float ignore) T_Heal; // health function

BOOL(entity targ, entity inflictor) CanDamage;

 