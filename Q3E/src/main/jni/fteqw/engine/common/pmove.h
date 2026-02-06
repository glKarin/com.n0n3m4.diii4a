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

#define BUTTON_ATTACK 1
#define BUTTON_JUMP 2

typedef enum {
	PM_NORMAL,			// normal ground movement
	PM_OLD_SPECTATOR,	// fly, no clip to world (QW bug)
	PM_SPECTATOR,		// fly, no clip to world
	PM_DEAD,			// no acceleration
	PM_FLY,				// fly, bump into walls
	PM_NONE,			// can't move
	PM_FREEZE,			// can't move or look around (TODO)
	PM_WALLWALK,		// sticks to walls. on ground while near one
	PM_6DOF				// spaceship mode
} pmtype_t;

#define PMF_JUMP_HELD			1
#define PMF_LADDER				2	//pmove flags. seperate from flags

#define	MAX_PHYSENTS	2048
typedef struct
{
	vec3_t	origin;
	vec3_t	angles;
	model_t	*model;		// only for bsp models
	vec3_t	mins, maxs;	// only for non-bsp models
	unsigned int	info;		// for client or server to identify
	qbyte		nonsolid;		//contributes to contents, but does not block. FIXME: why not just use the contentsmask directly?
	qbyte		notouch;		//don't trigger touch events. FIXME: why are these entities even in the list?
	qbyte		isportal;		//special portal traversion required
	unsigned int forcecontentsmask;
//	framestate_t framestate;
#define PE_FRAMESTATE NULLFRAMESTATE	//remove this once we start wanting players to interact with ents in different frames.
} physent_t;

typedef struct
{
	// player state
	vec3_t		origin;
	vec3_t		safeorigin;	//valid when safeorigin_known. needed for extrasr4's ladders otherwise they bug out.
	vec3_t		angles;
	vec3_t		velocity;
	vec3_t		basevelocity;
	vec3_t		gravitydir;
	qboolean		jump_held;
	int			jump_msec;	// msec since last jump
	float		waterjumptime;
	int			pm_type;
	vec3_t		player_mins;
	vec3_t		player_maxs;
	qboolean	capsule;

	// world state
	int			numphysent;
	physent_t	physents[MAX_PHYSENTS];	// 0 should be the world

	// input
	usercmd_t	cmd;

	qboolean onladder;
	qboolean safeorigin_known;

	// results
	int			skipent;
	int			numtouch;
	int			touchindex[MAX_PHYSENTS];
	vec3_t		touchvel[MAX_PHYSENTS];
	qboolean		onground;
	int			groundent;		// index in physents array, only valid
								// when onground is true
	int			waterlevel;
	int			watertype;

	struct world_s		*world;
} playermove_t;

typedef struct {
	//standard quakeworld
	float gravity;
	float stopspeed;
	float maxspeed;
	float spectatormaxspeed;
	float accelerate;
	float airaccelerate;
	float wateraccelerate;
	float friction;
	float waterfriction;
	float flyfriction;
	float entgravity;

	//extended stuff, sent via serverinfo
	float bunnyspeedcap;
	float watersinkspeed;
	float ktjump;
	float edgefriction; //default 2
	int	walljump;
	qboolean slidefix;
	qboolean airstep;
	qboolean pground;
	qboolean stepdown;
	qboolean slidyslopes;
	qboolean autobunny;
	qboolean bunnyfriction;	//force at least one frame of friction when bunnying.
	int stepheight;

	qbyte coordtype;	//FIXME: EZPEXT1_FLOATENTCOORDS should mean 4, but the result does not match ezquake/mvdsv's round-towards-origin which would result in inconsistencies. so player coords are rounded inconsistently.

	unsigned int	flags;
} movevars_t;

#define MOVEFLAG_VALID							0x80000000	//to signal that these are actually known. otherwise reserved.
//#define MOVEFLAG_Q2AIRACCELERATE				0x00000001
#define MOVEFLAG_NOGRAVITYONGROUND				0x00000002	//no slope sliding
//#define MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE	0x00000004	//apply half-gravity both before AND after the move, which better matches the curve
#define MOVEFLAG_QWEDGEBOX						0x00010000	//calculate edgefriction using tracebox and a buggy start pos
#define MOVEFLAG_QWCOMPAT						(MOVEFLAG_NOGRAVITYONGROUND|MOVEFLAG_QWEDGEBOX)

extern	movevars_t		movevars;
extern	playermove_t	pmove;

void PM_PlayerMove (float gamespeed);
void PM_Init (void);
void PM_InitBoxHull (void);

void PM_CategorizePosition (void);
int PM_HullPointContents (hull_t *hull, int num, vec3_t p);

int PM_ExtraBoxContents (vec3_t p);	//Peeks for HL-style water.
int PM_PointContents (vec3_t point);
qboolean PM_TestPlayerPosition (vec3_t point, qboolean ignoreportals);
#ifndef __cplusplus
struct trace_s PM_PlayerTrace (vec3_t start, vec3_t stop, unsigned int solidmask);
#endif

