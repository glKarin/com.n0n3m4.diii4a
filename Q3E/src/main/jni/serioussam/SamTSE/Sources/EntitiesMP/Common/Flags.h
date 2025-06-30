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

#ifndef SE_INCL_FLAGS_H
#define SE_INCL_FLAGS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// collision flags
#define ECBI_BRUSH              (1UL<<0)
#define ECBI_MODEL              (1UL<<1)
#define ECBI_PROJECTILE_MAGIC   (1UL<<2)
#define ECBI_PROJECTILE_SOLID   (1UL<<3)
#define ECBI_ITEM               (1UL<<4)
#define ECBI_CORPSE             (1UL<<5)
#define ECBI_MODEL_HOLDER       (1UL<<6)
#define ECBI_CORPSE_SOLID       (1UL<<7)
#define ECBI_PLAYER             (1UL<<8)

// standard flag combinations:

/*
 *  COLLISION COMBINATIONS
 */
#define ECF_IMMATERIAL (0UL)

// brush
#define ECF_BRUSH ( \
  ((ECBI_MODEL|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID|ECBI_ITEM|ECBI_CORPSE|ECBI_CORPSE_SOLID)<<ECB_TEST) |\
  ((ECBI_BRUSH)<<ECB_IS))

// model
#define ECF_MODEL ( \
  ((ECBI_MODEL|ECBI_BRUSH|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID|ECBI_ITEM|ECBI_MODEL_HOLDER|ECBI_CORPSE_SOLID)<<ECB_TEST) |\
  ((ECBI_MODEL)<<ECB_IS))

// projectile magic
#define ECF_PROJECTILE_MAGIC ( \
  ((ECBI_MODEL|ECBI_BRUSH|ECBI_CORPSE|ECBI_CORPSE_SOLID|ECBI_MODEL_HOLDER)<<ECB_TEST) |\
  ((ECBI_PROJECTILE_MAGIC)<<ECB_IS) |\
  ((ECBI_MODEL)<<ECB_PASS) )

// projectile solid
#define ECF_PROJECTILE_SOLID ( \
  ((ECBI_MODEL|ECBI_BRUSH|ECBI_PROJECTILE_SOLID|ECBI_CORPSE|ECBI_CORPSE_SOLID|ECBI_MODEL_HOLDER)<<ECB_TEST) |\
  ((ECBI_PROJECTILE_SOLID)<<ECB_IS) |\
  ((ECBI_MODEL|ECBI_PROJECTILE_SOLID)<<ECB_PASS) )

// item
#define ECF_ITEM ( \
  ((ECBI_MODEL|ECBI_BRUSH)<<ECB_TEST) |\
  ((ECBI_MODEL)<<ECB_PASS) |\
  ((ECBI_ITEM)<<ECB_IS))

// touch model
#define ECF_TOUCHMODEL ( \
  ((ECBI_MODEL)<<ECB_TEST) |\
  ((ECBI_MODEL)<<ECB_PASS) |\
  ((ECBI_ITEM)<<ECB_IS))

// corpse
#define ECF_CORPSE ( \
  ((ECBI_BRUSH|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_TEST) |\
  ((ECBI_CORPSE)<<ECB_IS))

// large corpse that is not passable, but doesn't collide with itself
#define ECF_CORPSE_SOLID ( \
  ((ECBI_BRUSH|ECBI_MODEL|ECBI_MODEL_HOLDER|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_TEST) |\
  ((ECBI_CORPSE_SOLID)<<ECB_IS))

// model holder
#define ECF_MODEL_HOLDER ( \
  ((ECBI_MODEL|ECBI_CORPSE|ECBI_CORPSE_SOLID|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID|ECBI_ITEM|ECBI_MODEL_HOLDER)<<ECB_TEST) |\
  ((ECBI_MODEL_HOLDER)<<ECB_IS))

// debris
#define ECF_DEBRIS ( \
  ((ECBI_BRUSH)<<ECB_TEST) |\
  ((ECBI_CORPSE)<<ECB_IS))

// flame
#define ECF_FLAME ( \
  ((ECBI_MODEL|ECBI_CORPSE_SOLID)<<ECB_TEST) |\
  ((ECBI_PROJECTILE_MAGIC)<<ECB_IS) |\
  ((ECBI_MODEL|ECBI_CORPSE_SOLID)<<ECB_PASS) )

// camera
#define ECF_CAMERA ( \
  ((ECBI_BRUSH)<<ECB_TEST) |\
  ((ECBI_MODEL)<<ECB_IS) |\
  ((ECBI_BRUSH)<<ECB_PASS) )

/*
 *  PHYSIC COMBINATIONS
 */
// model that walks around on feet (CMovableModelEntity)
#define EPF_MODEL_WALKING ( \
  EPF_ONBLOCK_CLIMBORSLIDE|EPF_ORIENTEDBYGRAVITY|\
  EPF_TRANSLATEDBYGRAVITY|EPF_PUSHABLE|EPF_MOVABLE)

// model that flies around with wings or similar (CMovableModelEntity)
#define EPF_MODEL_FLYING ( \
  EPF_ONBLOCK_SLIDE|EPF_ORIENTEDBYGRAVITY|EPF_PUSHABLE|EPF_MOVABLE)

// model that flies around with no gravity orientation (CMovableModelEntity)
#define EPF_MODEL_FREE_FLYING ( \
  EPF_ONBLOCK_SLIDE|EPF_PUSHABLE|EPF_MOVABLE)

// model that bounce around (CMovableModelEntity)
#define EPF_MODEL_BOUNCING ( \
  EPF_ONBLOCK_BOUNCE|EPF_PUSHABLE|EPF_MOVABLE|EPF_TRANSLATEDBYGRAVITY)

// projectile that flies around with no gravity orientation (CMovableModelEntity)
#define EPF_PROJECTILE_FLYING ( \
  EPF_ONBLOCK_STOPEXACT|EPF_PUSHABLE|EPF_MOVABLE)

// model that slides on brushes (CMovableModelEntity)
#define EPF_MODEL_SLIDING (\
  EPF_ONBLOCK_SLIDE|EPF_ORIENTEDBYGRAVITY|EPF_TRANSLATEDBYGRAVITY|\
  EPF_PUSHABLE|EPF_MOVABLE)

// model that fall (CMovableModelEntity)
#define EPF_MODEL_FALL ( \
  EPF_ONBLOCK_SLIDE|EPF_PUSHABLE|EPF_MOVABLE|EPF_TRANSLATEDBYGRAVITY)

// model for items
#define EPF_MODEL_ITEM ( \
  EPF_ONBLOCK_BOUNCE|EPF_PUSHABLE|EPF_MOVABLE|EPF_TRANSLATEDBYGRAVITY|EPF_ORIENTEDBYGRAVITY)

// model that is fixed in one place and cannot be moved (CEntity)
#define EPF_MODEL_FIXED (0UL)

// model that can be pushed around by others (CEntity)
#define EPF_MODEL_PUSHAROUND (\
  EPF_ONBLOCK_SLIDE|EPF_ORIENTEDBYGRAVITY|EPF_TRANSLATEDBYGRAVITY|EPF_PUSHABLE|EPF_MOVABLE)

// model that walks around switches to this when dead (CMovableModelEntity)
#define EPF_MODEL_CORPSE ( \
  EPF_ONBLOCK_SLIDE|EPF_ORIENTEDBYGRAVITY|EPF_TRANSLATEDBYGRAVITY|\
  EPF_PUSHABLE|EPF_MOVABLE)

// model that is not physically present - just a decoration (CEntity)
#define EPF_MODEL_IMMATERIAL (0UL)

// brush that moves around absolute (CMovableBrushEntity)
#define EPF_BRUSH_MOVING (\
  EPF_ONBLOCK_PUSH|EPF_RT_SYNCHRONIZED|\
  EPF_ABSOLUTETRANSLATE|EPF_NOACCELERATION|EPF_MOVABLE)

// brush that is fixed in one place and cannot be moved (CEntity)
#define EPF_BRUSH_FIXED  (0UL)

// brush that is not physically present - just a decoration (CEntity)
#define EPF_BRUSH_IMMATERIAL (0UL)



#endif  /* include-once check. */

