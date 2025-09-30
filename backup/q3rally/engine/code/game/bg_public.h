/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// bg_public.h -- definitions shared by both the server game and client game modules

// STONELANCE
#include "bg_physics.h"
// END

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#define GAME_VERSION		BASEGAME "-2"

#define	DEFAULT_GRAVITY		800
#define	GIB_HEALTH			-40
#define	ARMOR_PROTECTION	0.66

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define DEFAULT_SHOTGUN_SPREAD	700
#define DEFAULT_SHOTGUN_COUNT	11

#define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection

#define LIGHTNING_RANGE         768

#define SCORE_NOT_PRESENT       -9999   // for the CS_SCORES[12] when only one player is present

#define VOTE_TIME                       30000   // 30 seconds before vote times out

// STONELANCE
//#define       MINS_Z                          -24
#define MINS_Z                          -10
// END
#define DEFAULT_VIEWHEIGHT      26
#define CROUCH_VIEWHEIGHT       12
#define DEAD_VIEWHEIGHT         -16

// STONELANCE
#define TIME_BONUS_PER_FRAG     500
#define RACE_OBSERVER_DELAY     5000

#define NORMAL_WEAPON_TIME_MASK 0x0000ffff
#define REAR_WEAPON_TIME_MASK 0xffff0000
// END

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define CS_MUSIC                                2
#define CS_MESSAGE                              3               // from the map worldspawn's message field
#define CS_MOTD                                 4               // g_motd string for server message of the day
#define CS_WARMUP                               5               // server time when the match will be restarted
#define CS_SCORES1                              6
#define CS_SCORES2                              7
#define CS_VOTE_TIME                    8
#define CS_VOTE_STRING                  9
#define CS_VOTE_YES                             10
#define CS_VOTE_NO                              11

#define CS_TEAMVOTE_TIME                12
#define CS_TEAMVOTE_STRING              14
#define CS_TEAMVOTE_YES                 16
#define CS_TEAMVOTE_NO                  18

#define CS_GAME_VERSION                 20
#define CS_LEVEL_START_TIME             21              // so the timer only shows the current level
#define CS_INTERMISSION                 22              // when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS                   23              // string indicating flag status in CTF
#define CS_SHADERSTATE                  24
#define CS_BOTINFO                              25

#define CS_ITEMS                                26              // string of 0's and 1's that tell which items are present
// Q3Rally Code Start


#define CS_SCORES3                              27
#define CS_SCORES4                              28
#define CS_REFLECTION_IMAGE             29
#define CS_SIGILSTATUS                  30
// Q3Rally Code END

#define CS_MODELS                               32
#define CS_SOUNDS                               (CS_MODELS+MAX_MODELS)
// STONELANCE
//#define       CS_PLAYERS                              (CS_SOUNDS+MAX_SOUNDS)
#define CS_SCRIPTS                              (CS_SOUNDS+MAX_SOUNDS)
#define CS_PLAYERS                              (CS_SCRIPTS+MAX_SCRIPTS)
// END
#define CS_LOCATIONS                    (CS_PLAYERS+MAX_CLIENTS)
#define CS_PARTICLES                    (CS_LOCATIONS+MAX_LOCATIONS)
#define CS_MAX                                  (CS_PARTICLES+MAX_LOCATIONS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum {
// Q3Rally Code Start - UPDATE fix this and remove non q3rally gametypes
        /*
        GT_FFA,                         // free for all
        GT_TOURNAMENT,          // one on one tournament
        GT_SINGLE_PLAYER,       // single player tournament

        //-- team games go after this --

        GT_TEAM,                        // team deathmatch
        GT_CTF,                         // capture the flag
        GT_1FCTF,
        GT_OBELISK,
        GT_HARVESTER,
*/
        GT_RACING,                      // racing
        GT_RACING_DM,           // racing with weapons
        GT_SINGLE_PLAYER,       // single player tournament
        GT_DERBY,                       // demolition derby
        GT_LCS,                       // last car standing
        GT_DEATHMATCH,          // random destruction

        //-- team games go after this --

        GT_TEAM,                        // team deathmatch
        GT_TEAM_RACING,         // team racing
        GT_TEAM_RACING_DM,      // team racing with weapons
        GT_CTF,                         // capture the flag
        GT_DOMINATION,              // domination
// Q3Rally Code END
        GT_MAX_GAME_TYPE
} gametype_t;

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

// STONELANCE
typedef enum {
        OM_LEFT,                // Left door
        OM_RIGHT,               // Right door
        OM_FRONT,               // Front Bumper
        OM_REAR,                // Rear Bumper
        OM_HOOD,                // Hood
        OM_ROOF,                // Roof
        OM_TRACK_SIDE,  // Use observer spots
        OM_TRACK,               // Camera that moves along the track
        OM_OVERHEAD,    // Overhead view
        OM_AHEAD                // Camera looking back at car
} observerMode_t;
// END

typedef enum {
        PM_NORMAL,              // can accelerate and turn
        PM_NOCLIP,              // noclip movement
        PM_SPECTATOR,   // still run into walls
        PM_DEAD,                // no acceleration or turning, but free falling
        PM_FREEZE,              // stuck in place with no control
        PM_INTERMISSION,        // no movement or status bar
        PM_SPINTERMISSION       // no movement or status bar
} pmtype_t;

typedef enum {
        WEAPON_READY,
        WEAPON_RAISING,
        WEAPON_DROPPING,
        WEAPON_FIRING,
// STONELANCE
        WEAPON_REARFIRING
// END
} weaponstate_t;

// pmove->pm_flags
#define PMF_DUCKED                      1
#define PMF_JUMP_HELD           2
// STONELANCE
/*
#define PMF_BACKWARDS_JUMP      8               // go into backwards land
#define PMF_BACKWARDS_RUN       16              // coast down to backwards run
*/
// END
#define PMF_TIME_LAND           32              // pm_time is time before rejump
#define PMF_TIME_KNOCKBACK      64              // pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP      256             // pm_time is waterjump
#define PMF_RESPAWNED           512             // clear after attack and jump buttons come up
#define PMF_USE_ITEM_HELD       1024
#define PMF_GRAPPLE_PULL        2048    // pull towards grapple location
#define PMF_FOLLOW                      4096    // spectate following another player
#define PMF_SCOREBOARD          8192    // spectate as a scoreboard
#ifdef MISSIONPACK
#define PMF_INVULEXPAND         16384   // invulnerability sphere set to full size
#endif
// STONELANCE
#define PMF_OBSERVE                     32768   // spectate as an observer
// END


#define PMF_ALL_TIMES   (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define MAXTOUCH        32
typedef struct {
        // state (in / out)
        playerState_t   *ps;

        // command (in)
        usercmd_t       cmd;
        int                     tracemask;                      // collide against these types of surfaces
        int                     debugLevel;                     // if set, diagnostic output will be printed
        qboolean        noFootsteps;            // if the game is setup for no footsteps by the server
        qboolean        gauntletHit;            // true if a gauntlet attack would actually hit something

        int                     framecount;

        // results (out)
        int                     numtouch;
        int                     touchents[MAXTOUCH];
// STONELANCE
        vec3_t          touchPos[MAXTOUCH];
// END

        vec3_t          mins, maxs;                     // bounding box size

        int                     watertype;
        int                     waterlevel;

        float           xyspeed;

        // for fixed msec Pmove
        int                     pmove_fixed;
        int                     pmove_msec;

        // callbacks to test the world
        // these will be different functions during game and cgame
        void            (*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
        int                     (*pointcontents)( const vec3_t point, int passEntityNum );

// STONELANCE
        car_t           *car;
        car_t           **cars;

        int                     pDebug;

        qboolean        client;

        int                     controlMode;
        qboolean        manualShift;
        collisionDamage_t       damage;

        qboolean        (*frictionFunc)( const carPoint_t *point, float *sCOF, float *kCOF );

        float           car_spring;
        float           car_shock_up;
        float           car_shock_down;
        float           car_swaybar;
        float           car_wheel;
        float           car_wheel_damp;

        float           car_frontweight_dist;
        float           car_IT_xScale;
        float           car_IT_yScale;
        float           car_IT_zScale;
        float           car_body_elasticity;

        float           car_air_cof;
        float           car_air_frac_to_df;
        float           car_friction_scale;
// END
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
// STONELANCE
// void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd, int controlMode );
// END
void Pmove (pmove_t *pmove);

//===================================================================================


// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
        STAT_HEALTH,
        STAT_HOLDABLE_ITEM,
#ifdef MISSIONPACK
        STAT_PERSISTANT_POWERUP,
#endif
        STAT_WEAPONS,                                   // 16 bit fields
        STAT_ARMOR,                            
        STAT_DEAD_YAW,                                  // look this direction when dead (FIXME: get rid of?)
        STAT_CLIENTS_READY,                             // bit mask of clients wishing to exit the intermission (FIXME: configstring?)
// STONELANCE
//      STAT_MAX_HEALTH                                 // health / armor limit, changeable by handicap
        STAT_MAX_HEALTH,                                        // health / armor limit, changeable by handicap
        STAT_RPM,
        STAT_GEAR,
        STAT_DAMAGE_DEALT,
        STAT_DAMAGE_TAKEN, // STONELANCE - really need this one?
        STAT_NEXT_CHECKPOINT,
        STAT_POSITION,
        STAT_FRAC_TO_NEXT_CHECKPOINT
// END
} statIndex_t;


// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
        PERS_SCORE,                                             // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
        PERS_HITS,                                              // total points damage inflicted so damage beeps can sound on change
        PERS_RANK,                                              // player rank or team rank
        PERS_TEAM,                                              // player team
        PERS_SPAWN_COUNT,                               // incremented every respawn
        PERS_PLAYEREVENTS,                              // 16 bits that can be flipped for events
        PERS_ATTACKER,                                  // clientnum of last damage inflicter
        PERS_ATTACKEE_ARMOR,                    // health/armor of last person we attacked
        PERS_KILLED,                                    // count of the number of times you died
        // player awards tracking
        PERS_IMPRESSIVE_COUNT,                  // two railgun hits in a row
        PERS_IMPRESSIVETELEFRAG_COUNT,          // two telefrag hits in a row
        PERS_EXCELLENT_COUNT,                   // two successive kills in a short amount of time
        PERS_DEFEND_COUNT,                              // defend awards
        PERS_ASSIST_COUNT,                              // assist awards
        PERS_GAUNTLET_FRAG_COUNT,               // kills with the guantlet
        PERS_CAPTURES                                   // captures
} persEnum_t;


// entityState_t->eFlags
#define EF_DEAD                         0x00000001              // don't draw a foe marker over players with EF_DEAD
#define EF_SMOKE_LIGHT                  0x00000002              // for light smoke
#define EF_TELEPORT_BIT                 0x00000004              // toggled every time the origin abruptly changes
#define EF_AWARD_EXCELLENT              0x00000008              // draw an excellent sprite
#define EF_PLAYER_EVENT                 0x00000010
#define EF_SMOKE_DARK                   0x00000020              // for dark smoke
#define EF_AWARD_GAUNTLET               0x00000040              // draw a gauntlet sprite
#define EF_NODRAW                       0x00000080              // may have an event, but no model (unspawned items)
#define EF_FIRING                       0x00000100              // for lightning gun
#define EF_BRAKE                        0x00000200              // player is braking
#define EF_REVERSE                      0x00000400              // player is in reverse
#define EF_AWARD_CAP                    0x00000800              // draw the capture sprite
#define EF_TALK                         0x00001000              // draw a talk balloon
#define EF_CONNECTION                   0x00002000              // draw a connection trouble sprite
#define EF_VOTED                        0x00004000              // already cast a vote
#define EF_AWARD_IMPRESSIVE             0x00008000              // draw an impressive sprite
#define EF_AWARD_IMPRESSIVETELEFRAG     0x00010000              // draw a telefragged sprite
#define EF_AWARD_DEFEND                 0x00020000              // draw a defend sprite
#define EF_AWARD_ASSIST                 0x00030000              // draw a assist sprite
#define EF_AWARD_DENIED                 0x00080000              // denied
#define EF_TEAMVOTED                    0x00100000              // already cast a team vote
#define EF_KAMIKAZE                     0x00200000
#define EF_BOUNCE                       0x00400000              // for missiles
#define EF_BOUNCE_HALF                  0x00800000              // for missiles
#define EF_BOUNCE_NONE                  0x01000000              // for mines
#define EF_MOVER_STOP                   0x02000000              // will push otherwise

// NOTE: may not have more than 16
typedef enum {
        PW_NONE,

        PW_QUAD,
        PW_BATTLESUIT,
        PW_HASTE,
        PW_INVIS,
        PW_REGEN,
// Q3Rally Code Start
//      PW_FLIGHT,
        PW_SHIELD,
        PW_TURBO,
// Q3Rally Code END

        PW_REDFLAG,
        PW_BLUEFLAG,
//        PW_GREENFLAG,
//        PW_YELLOWFLAG,
        PW_NEUTRALFLAG,

#ifdef MISSIONPACK
        PW_SCOUT,
        PW_GUARD,
        PW_DOUBLER,
        PW_AMMOREGEN,
        PW_INVULNERABILITY,
#endif

        PW_SIGILWHITE,
        PW_SIGILRED,
        PW_SIGILBLUE,
        PW_SIGILGREEN,
        PW_SIGILYELLOW,

        PW_NUM_POWERUPS

} powerup_t;

typedef enum {
        HI_NONE,

        HI_TELEPORTER,
        HI_MEDKIT,
        HI_KAMIKAZE,
        HI_PORTAL,
        HI_INVULNERABILITY,
// STONELANCE
        HI_TURBO,
// END
        HI_NUM_HOLDABLE
} holdable_t;


typedef enum {
        WP_NONE,

        WP_GAUNTLET,
        WP_MACHINEGUN,
        WP_SHOTGUN,
        WP_GRENADE_LAUNCHER,
        WP_ROCKET_LAUNCHER,
        WP_LIGHTNING,
        WP_RAILGUN,
        WP_PLASMAGUN,
        WP_BFG,
        WP_FLAME_THROWER,

#ifdef MISSIONPACK
        WP_NAILGUN,
        WP_PROX_LAUNCHER,
        WP_CHAINGUN,
#endif

        RWP_SMOKE,
        RWP_OIL,
        RWP_MINE,
        RWP_FLAME,
        RWP_BIO,

        WP_NUM_WEAPONS
} weapon_t;


// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define PLAYEREVENT_DENIEDREWARD                0x0001
#define PLAYEREVENT_GAUNTLETREWARD              0x0002
#define PLAYEREVENT_HOLYSHIT                    0x0004

// entityState_t->event values
// entity events are for effects that take place relative
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1           0x00000100
#define EV_EVENT_BIT2           0x00000200
#define EV_EVENT_BITS           (EV_EVENT_BIT1|EV_EVENT_BIT2)

#define EVENT_VALID_MSEC        300

typedef enum {
        EV_NONE,

        EV_FOOTSTEP,
        EV_FOOTSTEP_METAL,
        EV_FOOTSPLASH,
        EV_FOOTWADE,
        EV_SWIM,

        EV_STEP_4,
        EV_STEP_8,
        EV_STEP_12,
        EV_STEP_16,

        EV_FALL_SHORT, // 10
        EV_FALL_MEDIUM,
        EV_FALL_FAR,

        EV_JUMP_PAD,                    // boing sound at origin, jump sound on player

        EV_JUMP,
        EV_WATER_TOUCH, // foot touches
        EV_WATER_LEAVE, // foot leaves
        EV_WATER_UNDER, // head touches
        EV_WATER_CLEAR, // head leaves

        EV_ITEM_PICKUP,                 // normal item pickups are predictable
        EV_GLOBAL_ITEM_PICKUP,  // powerup / team sounds are broadcast to everyone

        EV_NOAMMO, // 21
        EV_CHANGE_WEAPON,
        EV_FIRE_WEAPON,
// STONELANCE
        EV_HAZARD,
        EV_ALTFIRE_WEAPON,
        EV_FIRE_REARWEAPON,
// END

        EV_USE_ITEM0,
        EV_USE_ITEM1,
        EV_USE_ITEM2,
        EV_USE_ITEM3,
        EV_USE_ITEM4, // 30
        EV_USE_ITEM5,
        EV_USE_ITEM6,
        EV_USE_ITEM7,
        EV_USE_ITEM8,
        EV_USE_ITEM9,
        EV_USE_ITEM10,
        EV_USE_ITEM11,
        EV_USE_ITEM12,
        EV_USE_ITEM13,
        EV_USE_ITEM14, // 40
        EV_USE_ITEM15,

        EV_ITEM_RESPAWN,
        EV_ITEM_POP,
        EV_PLAYER_TELEPORT_IN,
        EV_PLAYER_TELEPORT_OUT,

        EV_GRENADE_BOUNCE,              // eventParm will be the soundindex

        EV_GENERAL_SOUND,
        EV_GLOBAL_SOUND,                // no attenuation
        EV_GLOBAL_TEAM_SOUND,

        EV_BULLET_HIT_FLESH, // 50
        EV_BULLET_HIT_WALL,

        EV_MISSILE_HIT,
        EV_MISSILE_MISS,
        EV_MISSILE_MISS_METAL,
        EV_RAILTRAIL,
        EV_SHOTGUN,
        EV_BULLET,                              // otherEntity is the shooter

        EV_PAIN,
        EV_DEATH1,
        EV_DEATH2, // 60
        EV_DEATH3,
        EV_OBITUARY,

        EV_POWERUP_QUAD,
        EV_POWERUP_BATTLESUIT,
// STONELANCE
        EV_POWERUP_SHIELD,
        EV_POWERUP_TURBO,
// END
        EV_POWERUP_REGEN,

        EV_GIB_PLAYER,                  // gib a previously living player
        EV_BREAK_GLASS,
	      EV_BREAKWOOD,
	      EV_BREAKMETAL,
        EV_SCOREPLUM,                   // score plum

#ifdef MISSIONPACK
        EV_PROXIMITY_MINE_STICK,
        EV_PROXIMITY_MINE_TRIGGER, // 70
        EV_KAMIKAZE,                    // kamikaze explodes
        EV_OBELISKEXPLODE,              // obelisk explodes
        EV_OBELISKPAIN,                 // obelisk is in pain
        EV_INVUL_IMPACT,                // invulnerability sphere impact
        EV_JUICED,                              // invulnerability juiced effect
        EV_LIGHTNINGBOLT,               // lightning bolt bounced of invulnerability sphere
#endif

        EV_DEBUG_LINE,
        EV_STOPLOOPINGSOUND,
        EV_TAUNT,
        EV_TAUNT_YES, // 80
        EV_TAUNT_NO,
        EV_TAUNT_FOLLOWME,
        EV_TAUNT_GETFLAG,
        EV_TAUNT_GUARDBASE,
        EV_TAUNT_PATROL,
        
		EV_EMIT_DEBRIS_LIGHT,		// emit light concrete chunks
		EV_EMIT_DEBRIS_DARK,		// emit dark concrete chunks
		EV_EMIT_DEBRIS_LIGHT_LARGE,	// emit light large concrete chunks
		EV_EMIT_DEBRIS_DARK_LARGE,	// emit dark large concrete chunks
		EV_EMIT_DEBRIS_WOOD,		// emit wooden chunks
		EV_EMIT_DEBRIS_FLESH,		// emit gibs
		EV_EMIT_DEBRIS_GLASS,		// emite shards of glass
        EV_EMIT_DEBRIS_STONE,
        EV_EARTHQUAKE,
    
		EV_EXPLOSION,
		EV_PARTICLES_GRAVITY,
		EV_PARTICLES_LINEAR,
		EV_PARTICLES_LINEAR_UP,
		EV_PARTICLES_LINEAR_DOWN,

} entity_event_t;


typedef enum {
        GTS_RED_CAPTURE,
        GTS_BLUE_CAPTURE,
        GTS_RED_RETURN,
        GTS_BLUE_RETURN,
        GTS_RED_TAKEN,
        GTS_BLUE_TAKEN,
        GTS_REDOBELISK_ATTACKED,
        GTS_BLUEOBELISK_ATTACKED,
        GTS_REDTEAM_SCORED,
        GTS_BLUETEAM_SCORED,
        GTS_REDTEAM_TOOK_LEAD,
        GTS_BLUETEAM_TOOK_LEAD,
        GTS_TEAMS_ARE_TIED,
        GTS_KAMIKAZE
} global_team_sound_t;


// animations
typedef enum {
        BOTH_DEATH1,
        BOTH_DEAD1,
        BOTH_DEATH2,
        BOTH_DEAD2,
        BOTH_DEATH3,
        BOTH_DEAD3,

        TORSO_GESTURE,

        TORSO_ATTACK,
        TORSO_ATTACK2,

        TORSO_DROP,
        TORSO_RAISE,

        TORSO_STAND,
        TORSO_STAND2,

        LEGS_WALKCR,
        LEGS_WALK,
        LEGS_RUN,
        LEGS_BACK,
        LEGS_SWIM,

        LEGS_JUMP,
        LEGS_LAND,

        LEGS_JUMPB,
        LEGS_LANDB,

        LEGS_IDLE,
        LEGS_IDLECR,

        LEGS_TURN,

        TORSO_GETFLAG,
        TORSO_GUARDBASE,
        TORSO_PATROL,
        TORSO_FOLLOWME,
        TORSO_AFFIRMATIVE,
        TORSO_NEGATIVE,

        MAX_ANIMATIONS,

        LEGS_BACKCR,
        LEGS_BACKWALK,
        FLAG_RUN,
        FLAG_STAND,
        FLAG_STAND2RUN,

        MAX_TOTALANIMATIONS
} animNumber_t;


typedef struct animation_s {
        int             firstFrame;
        int             numFrames;
        int             loopFrames;                     // 0 to numFrames
        int             frameLerp;                      // msec between frames
        int             initialLerp;            // msec to get to first frame
        int             reversed;                       // true if animation is reversed
        int             flipflop;                       // true if animation should flipflop back to base
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT          128


typedef enum {
        TEAM_FREE,
        TEAM_RED,
        TEAM_BLUE,
// STONELANCE
        TEAM_GREEN,
        TEAM_YELLOW,
// END
        TEAM_SPECTATOR,

        TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME               1000

// How many players on the overlay
#define TEAM_MAXOVERLAY         32

//team task
typedef enum {
        TEAMTASK_NONE,
        TEAMTASK_OFFENSE,
        TEAMTASK_DEFENSE,
        TEAMTASK_PATROL,
        TEAMTASK_FOLLOW,
        TEAMTASK_RETRIEVE,
        TEAMTASK_ESCORT,
        TEAMTASK_CAMP
} teamtask_t;

// means of death
typedef enum {
        MOD_UNKNOWN,
        MOD_SHOTGUN,
        MOD_GAUNTLET,
        MOD_MACHINEGUN,
        MOD_GRENADE,
        MOD_GRENADE_SPLASH,
        MOD_ROCKET,
        MOD_ROCKET_SPLASH,
        MOD_PLASMA,
        MOD_PLASMA_SPLASH,
        MOD_FLAME_THROWER,
        MOD_RAILGUN,
        MOD_LIGHTNING,
        MOD_BFG,
        MOD_BFG_SPLASH,
        MOD_WATER,
        MOD_SLIME,
        MOD_LAVA,
        MOD_CRUSH,
        MOD_TELEFRAG,
        MOD_FALLING,
        MOD_SUICIDE,
        MOD_TARGET_LASER,
        MOD_TRIGGER_HURT,
#ifdef MISSIONPACK
        MOD_NAIL,
        MOD_CHAINGUN,
        MOD_PROXIMITY_MINE,
        MOD_KAMIKAZE,
        MOD_JUICED,
#endif
        MOD_UPSIDEDOWN,
        MOD_BO_SHOCKS,
        MOD_CAR_COLLISION,
        MOD_WORLD_COLLISION,
        MOD_HIGH_FORCES,

        MOD_MINE_SPLASH,
        MOD_BIOHAZARD,
        MOD_MINE,
        MOD_POISON,
        MOD_FIRE,
        MOD_GRAPPLE,
        MOD_BREAKABLE_SPLASH
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum {
        IT_BAD,
        IT_WEAPON,                              // EFX: rotate + upscale + minlight
        IT_AMMO,                                // EFX: rotate
        IT_ARMOR,                               // EFX: rotate + minlight
        IT_HEALTH,                              // EFX: static external sphere + rotating internal
        IT_POWERUP,                             // instant on, timer based
                                                        // EFX: rotate + external ring that rotates
        IT_HOLDABLE,                    // single use, holdable item
                                                        // EFX: rotate + bob
        IT_PERSISTANT_POWERUP,
        IT_TEAM,
// Q3Rally Code Start
        IT_RFWEAPON,
        IT_SIGIL
// Q3Rally Code END
} itemType_t;

// Q3Rally Code Start
typedef enum {
        HT_NOTHAZARD,

        HT_BIO,
        HT_POISON,
        HT_EXPLOSIVE,
        HT_OIL,
        HT_SMOKE,
        HT_FIRE,

        NUM_HAZARD_TYPES
} hazard_t;
// Q3Rally Code END

#define MAX_ITEM_MODELS 4

typedef struct gitem_s {
        char            *classname;     // spawning name
        char            *pickup_sound;
        char            *world_model[MAX_ITEM_MODELS];

        char            *icon;
        char            *pickup_name;   // for printing on pickup

        int                     quantity;               // for ammo how much, or duration of powerup
        itemType_t  giType;                     // IT_* flags


        int                     giTag;

        char            *precaches;             // string of all models and images this item will use
        char            *sounds;                // string of all sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern  gitem_t bg_itemlist[];
extern  int             bg_numItems;

gitem_t *BG_FindItem( const char *pickupName );
gitem_t *BG_FindItemForWeapon( weapon_t weapon );
gitem_t *BG_FindItemForPowerup( powerup_t pw );
gitem_t *BG_FindItemForHoldable( holdable_t pw );
#define ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean        BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps );


// g_dmflags->integer flags
#define DF_NO_FALLING                   8
#define DF_FIXED_FOV                    16
#define DF_NO_FOOTSTEPS                 32

// content masks
#define MASK_ALL                                (-1)
#define MASK_SOLID                              (CONTENTS_SOLID)
#define MASK_PLAYERSOLID                (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define MASK_DEADSOLID                  (CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define MASK_WATER                              (CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE                             (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_SHOT                               (CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)


//
// entityState_t->eType
//
typedef enum {
        ET_GENERAL,
        ET_PLAYER,
        ET_ITEM,
        ET_MISSILE,
        ET_MOVER,
        ET_BEAM,
        ET_PORTAL,
        ET_SPEAKER,
        ET_PUSH_TRIGGER,
        ET_TELEPORT_TRIGGER,
        ET_INVISIBLE,
// STONELANCE - removed grapple
//      ET_GRAPPLE,                             // grapple hooked on wall
// END
        ET_TEAM,
        ET_BREAKGLASS,
	      ET_BREAKWOOD,
	      ET_BREAKMETAL,

// STONELANCE
        ET_WEATHER,                             // used to specify per area weather
        ET_CHECKPOINT,
        ET_SCRIPTED,                    // used for our new scripted map objects
        ET_AUXENT,                              // extra ent for passing car info to client
// END
        ET_EVENTS                               // any of the EV_* events can be added freestanding
                                                        // by setting eType to ET_EVENTS + eventNum
                                                        // this avoids having to set eFlags and eventNum
} entityType_t;



void    BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void    BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void    BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

// STONELANCE
// void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad );
void    BG_TouchJumpPad( car_t *car, playerState_t *ps, entityState_t *jumppad );
// END

void    BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );
// STONELANCE
// void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap );
void    BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, int sv_fps, qboolean snap );
// END

qboolean        BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );


#define ARENAS_PER_TIER         4
#define MAX_ARENAS                      1024
#define MAX_ARENAS_TEXT         8192

#define MAX_BOTS                        1024
#define MAX_BOTS_TEXT           8192


// Kamikaze

// 1st shockwave times
#define KAMI_SHOCKWAVE_STARTTIME                0
#define KAMI_SHOCKWAVEFADE_STARTTIME    1500
#define KAMI_SHOCKWAVE_ENDTIME                  2000
// explosion/implosion times
#define KAMI_EXPLODE_STARTTIME                  250
#define KAMI_IMPLODE_STARTTIME                  2000
#define KAMI_IMPLODE_ENDTIME                    2250
// 2nd shockwave times
#define KAMI_SHOCKWAVE2_STARTTIME               2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME   2500
#define KAMI_SHOCKWAVE2_ENDTIME                 3000
// radius of the models without scaling
#define KAMI_SHOCKWAVEMODEL_RADIUS              88
#define KAMI_BOOMSPHEREMODEL_RADIUS             72
// maximum radius of the models during the effect
#define KAMI_SHOCKWAVE_MAXRADIUS                1320
#define KAMI_BOOMSPHERE_MAXRADIUS               720
#define KAMI_SHOCKWAVE2_MAXRADIUS               704

