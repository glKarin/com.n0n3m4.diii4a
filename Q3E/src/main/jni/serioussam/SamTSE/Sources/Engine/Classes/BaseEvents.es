/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

5
%{
#include <Engine/StdH.h>
#define DECL_DLL ENGINE_API
#include <Engine/Entities/EntityEvent.h>
#include <Engine/Entities/EntityPointer.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Plane.h>
#include <Engine/Entities/EntityProperties.h>
%}

/*
 * These events are used globally
 */
event EInternal {     // internal jumping event - do not use
};                    
event EVoid {         // general void type
};                    
event EReturn {       // sub procedure return notify
};
event EBegin {        // sent to wait handler when started
};                    
event ETimer {        // timer elapsed
};                    
event ETouch {        // one entity touched another while moving
  CEntityPointer penOther,    // other entity
  BOOL bThisMoved,           // if this entity has touched other entity
  FLOATplane3D   plCollision, // plane of collision
};
event EPass {         // one entity passed through another while moving
  CEntityPointer penOther,   // other entity
  BOOL bThisMoved,           // if this entity has touched other entity
};
event EBlock {        // ONBLOCK_PUSH or ONBLOCK_STOP entity is blocked
  CEntityPointer penOther,    // other entity
  FLOATplane3D   plCollision, // plane of collision
};
event EWouldFall {    // entity cannot move or it would fall over an edge
};
event ETeleport {     // teleport has been activated in your vicinity
};
event EPreLevelChange { // notifying an entity that a level is about to change
  INDEX iUserData,
};
event EPostLevelChange { // notifying an entity that a level has just changed
  INDEX iUserData,
};
event EFirstWorldBase { // notifying an entity that it is the first worldbase in the world
};

enum DamageType {
   1 DMT_EXPLOSION  "Explosion",    // caused by dynamites, rockets and other ordinary explosives
   2 DMT_PROJECTILE "Projectile",   // caused by projectile (non exploding)
   3 DMT_CLOSERANGE "Close range",  // caused by close range weapon (chainsaw, head-saw, ...)
   4 DMT_BULLET     "Bullets",      // caused by ordinary bullets from pistols, rifles etc.
   5 DMT_DROWNING   "Drowning",     // caused by being without air for too long
   6 DMT_IMPACT     "Impact",       // caused by impact with some object at high relative velocity
   7 DMT_BRUSH      "Brush",        // caused by moving brush
   8 DMT_BURNING    "Burning",      // caused by being burned by fire or lava
   9 DMT_ACID       "Acid",         // caused by being burned by acid
  10 DMT_TELEPORT   "Teleport",     // applied to entities in teleport destination
  11 DMT_FREEZING   "Freezing",     // caused by freezing in cold water
  12 DMT_CANNONBALL "Cannon ball",  // caused by cannon ball
  13 DMT_CANNONBALL_EXPLOSION "Cannon ball explosion",   // when cannonball explodes
  14 DMT_SPIKESTAB  "Spike stab",   // stabbed by spikes (usually content type)
  15 DMT_ABYSS      "Abyss",        // when someone falls off a high ledge into the void
  16 DMT_HEAT       "Heat",         // walking under open sun too long
  17 DMT_DAMAGER    "Damager",      // caused by damager
  18 DMT_CHAINSAW   "Chain saw",    // caused by chainsaw
9999 DMT_NONE       "no damage",    // internal
};

event EDamage {  // entity has been damaged
  CEntityPointer penInflictor,    // entity that inflicted the damage
  FLOAT3D vDirection,             // where the damage came from (in absolute space)
  FLOAT3D vHitPoint,              // where the damage hit the entity (in absolute space)
  FLOAT fAmount,                  // amount of damage done
  enum DamageType dmtType,        // type of damage
};

event EDeath { // when this entity dies (health reaches zero)
  EDamage eLastDamage,  // the damage event that caused the death
};

event ETakingBreath { // when this entity takes air after being without it for some time
  FLOAT fBreathDelay,  // how long it was without air (0=little, 1=drowning)
};
