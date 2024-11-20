#ifndef __NAVIGATIONFLAGS_H__
#define __NAVIGATIONFLAGS_H__

#include "Omni-Bot_Types.h"

// bot: NavFlags
//		All the flags representing a state or property of a navigation point
static const NavFlags F_NAV_NONE		= (NavFlags(0));
static const NavFlags F_NAV_TEAM1		= (NavFlags(1)<<0);
static const NavFlags F_NAV_TEAM2		= (NavFlags(1)<<1);
static const NavFlags F_NAV_TEAM3		= (NavFlags(1)<<2);
static const NavFlags F_NAV_TEAM4		= (NavFlags(1)<<3);
static const NavFlags F_NAV_TEAMONLY	= (NavFlags(1)<<4);
static const NavFlags F_NAV_CLOSED		= (NavFlags(1)<<5);
static const NavFlags F_NAV_CROUCH		= (NavFlags(1)<<6);
static const NavFlags F_NAV_DOOR		= (NavFlags(1)<<7);
static const NavFlags F_NAV_JUMPGAP		= (NavFlags(1)<<8);
static const NavFlags F_NAV_CLIMB		= (NavFlags(1)<<9);
static const NavFlags F_NAV_SNEAK		= (NavFlags(1)<<10);
static const NavFlags F_NAV_ELEVATOR	= (NavFlags(1)<<11);
static const NavFlags F_NAV_TELEPORT	= (NavFlags(1)<<12);
static const NavFlags F_NAV_SNIPE		= (NavFlags(1)<<13);
static const NavFlags F_NAV_HEALTH		= (NavFlags(1)<<14);
static const NavFlags F_NAV_ARMOR		= (NavFlags(1)<<15);
static const NavFlags F_NAV_AMMO		= (NavFlags(1)<<16);
//static const NavFlags F_NAV_BLOCKABLE	= (NavFlags(1)<<17); // deprecated
static const NavFlags F_NAV_JUMPLOW		= (NavFlags(1)<<18);
static const NavFlags F_NAV_DYNAMIC		= (NavFlags(1)<<19);
static const NavFlags F_NAV_PRONE		= (NavFlags(1)<<20);
static const NavFlags F_NAV_INWATER		= (NavFlags(1)<<21);
static const NavFlags F_NAV_UNDERWATER	= (NavFlags(1)<<22);
//static const NavFlags F_NAV_MOVABLE		= (NavFlags(1)<<23); // deprecated
static const NavFlags F_NAV_DEFEND		= (NavFlags(1)<<24);
static const NavFlags F_NAV_ATTACK		= (NavFlags(1)<<25);
static const NavFlags F_NAV_SCRIPT		= (NavFlags(1)<<26);
static const NavFlags F_NAV_JUMP		= (NavFlags(1)<<27);
static const NavFlags F_NAV_ROUTEPT		= (NavFlags(1)<<28);
static const NavFlags F_NAV_DONTSAVE	= (NavFlags(1)<<29);
static const NavFlags F_NAV_INFILTRATOR	= (NavFlags(1)<<30);

// We skip a few so we can leave a few slots for default flags, so adding them later
// won't invalidate previously saved waypoint flags.
static const NavFlags F_NAV_NEXT			= (NavFlags(1)<<32);
// Mod/game defined waypoint tags should start at F_NAV_NEXT

static const NavFlags F_NAV_TEAM_ALL		= F_NAV_TEAM1|F_NAV_TEAM2|F_NAV_TEAM3|F_NAV_TEAM4;

// typedef: LinkFlags
//		All the flags representing a state or property of a link
typedef enum
{
	F_LNK_CLOSED	= (1<<0),
	F_LNK_CROUCH	= (1<<1),
	F_LNK_DOOR		= (1<<2),
	F_LNK_JUMPGAP	= (1<<3),
	F_LNK_CLIMB		= (1<<4),
	F_LNK_SNEAK		= (1<<5),
	F_LNK_NEAREDGE	= (1<<7),
	F_LNK_RIDE_ELEV = (1<<8),
	F_LNK_TELEPORT	= (1<<9),
	F_LNK_NOSHORT	= (1<<10),
	F_LNK_OUTOFRANGE= (1<<11),
	F_LNK_DONTSAVE	= (1<<12),

	F_LNK_NEXT		= (1<<16),
	
} LinkFlags;

// NEW RECAST STUFF

enum NavigationAreas {
	NAV_AREA_GROUND,
	NAV_AREA_WATER,
	NAV_AREA_CROUCH,
	NAV_AREA_HARMFUL,
	NAV_AREA_AVOID,
	NAV_AREA_LADDER,
};

enum NavigationFlags {
	NAV_FLAG_TEAM1		= (1<<0),
	NAV_FLAG_TEAM2		= (1<<1),
	NAV_FLAG_TEAM3		= (1<<2),
	NAV_FLAG_TEAM4		= (1<<3),
	NAV_FLAG_WALK		= (1<<4),
	NAV_FLAG_JUMP		= (1<<5),
	NAV_FLAG_LADDER		= (1<<6),

	NAV_FLAG_MOD		= (1<<10),
	//NAV_FLAG_TELEPORT	= (1<<2),
	//NAV_FLAG_ALLTEAMS	= NAV_FLAG_TEAM1|NAV_FLAG_TEAM2|NAV_FLAG_TEAM3|NAV_FLAG_TEAM4
};

#endif
