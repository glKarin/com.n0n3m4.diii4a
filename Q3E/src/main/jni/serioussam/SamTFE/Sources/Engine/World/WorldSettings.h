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

#ifndef SE_INCL_WORLDSETTINGS_H
#define SE_INCL_WORLDSETTINGS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/CTString.h>
#include <Engine/Math/TextureMapping.h>

// type of dynamic texture mapping transformation
class ENGINE_API CTextureTransformation {
public:
  CTString tt_strName;
  CMappingDefinition tt_mdTransformation;

  /* Constructor. */
  CTextureTransformation(void);
};

// type of texture blending used for a texture on a polygon
class ENGINE_API CTextureBlending {
public:
  CTString tb_strName;
  UBYTE tb_ubBlendingType;    // uses gfx flags for texture blending (STX_BLEND_xxxx)
  COLOR tb_colMultiply;       // original polygon color is multiplied by this

  /* Constructor. */
  CTextureBlending(void);
};

// type of surface of a polygon (its physical properties)
#define STF_SLIDEDOWNSLOPE  (1UL<<0)    // can accelerate only across slope, not up/down
#define STF_NOIMPACT        (1UL<<1)    // cannot damage because of impact with this polygon
class ENGINE_API CSurfaceType {
public:
  FLOAT st_fFriction;       // friction coefficient on the surface
  FLOAT st_fStairsHeight;   // stairs step-up height modifier (=0-no step, >0 use max, <0 use min)
  FLOAT st_fClimbSlopeCos;  // cosine of max. climbable slope
  FLOAT st_fJumpSlopeCos;   // cosine of max. jump-from slope
  INDEX st_iWalkDamageType;         // type of damage inflicted when walking on surface
  FLOAT st_fWalkDamageAmount;       // how much to damage when inside
  FLOAT st_tmWalkDamageDelay;       // how much to delay before first damage
  FLOAT st_tmWalkDamageFrequency;   // how much to delay between two damages
  ULONG st_ulFlags;
  CTString st_strName;      // name of surface type
  /* Default constructor. */
  CSurfaceType(void) :
    st_fFriction(1.0f),
    st_fStairsHeight(1.0f),
    st_fClimbSlopeCos( 0.7071067811865f/*cos(45.0f)*/),
    st_fJumpSlopeCos( 0.7071067811865f/*cos(45.0f)*/),
    st_iWalkDamageType(0),
    st_fWalkDamageAmount(0),
    st_tmWalkDamageDelay(0),
    st_tmWalkDamageFrequency(100),
    st_ulFlags(0),
    st_strName("") {}
};

// type of content of a sector
#define CTF_BREATHABLE_LUNGS  (1UL<<0)    // breathable for creatures with lungs
#define CTF_BREATHABLE_GILLS  (1UL<<1)    // breathable for creatures with gills
#define CTF_FLYABLE           (1UL<<2)    // flyable in if can fly
#define CTF_SWIMABLE          (1UL<<3)    // swimmable in if enough density
#define CTF_FADESPINNING      (1UL<<4)    // spinning objects may stop spinning

class ENGINE_API CContentType {
public:
  CTString ct_strName;  // name of surface type
  ULONG ct_ulFlags;     // various flags
  FLOAT ct_fDensity;    // density of the fluid inside content kg/m3 - defines buoyancy

  FLOAT ct_fFluidFriction;      // friction inside the fluid (stopping movements)
  FLOAT ct_fControlMultiplier;  // defines voluntary acceleration/decceleration
  FLOAT ct_fSpeedMultiplier;    // max. speed modifier inside the fluid
  INDEX ct_iSwimDamageType;         // type of damage inflicted when swimming
  FLOAT ct_fSwimDamageAmount;       // how much to damage when inside
  FLOAT ct_tmSwimDamageDelay;       // how much to delay before first damage
  FLOAT ct_tmSwimDamageFrequency;   // how much to delay between two damages
  FLOAT ct_fDrowningDamageAmount;   // how much to damage when drowning
  FLOAT ct_tmDrowningDamageDelay;   // how much to delay between two damages
  FLOAT ct_fKillImmersion;          // contents kills anything alive that gets in deeper than this
  INDEX ct_iKillDamageType;         // type of killing damage
  /* Default constructor. */
  CContentType(void) :
    ct_strName(""),
    ct_ulFlags(CTF_BREATHABLE_LUNGS|CTF_FLYABLE),
    ct_fDensity(0),
    ct_fFluidFriction    (0),
    ct_fControlMultiplier(1),
    ct_fSpeedMultiplier(1),
    ct_iSwimDamageType(0),
    ct_fSwimDamageAmount(0),
    ct_tmSwimDamageDelay(0),
    ct_tmSwimDamageFrequency(100),
    ct_fDrowningDamageAmount(10),
    ct_tmDrowningDamageDelay(1),
    ct_fKillImmersion(0),
    ct_iKillDamageType(0)
    {}
};

class ENGINE_API CEnvironmentType {
public:
  CTString et_strName;  // name of environment type
  INDEX et_iType;
  FLOAT et_fSize;
  /* Default constructor. */
  CEnvironmentType(void) :
    et_strName(""),
    et_iType(1),
    et_fSize(7.5)
    {}
};

/*
 * One type of illuminating polygon.
 */
class ENGINE_API CIlluminationType {
public:
  CTString it_strName;  // name of illumination type

  /* Default constructor. */
  CIlluminationType(void) : it_strName("") {};
};

/*
 * One type of mirroring (or warping) polygon.
 */
#define MPF_WARP  (1UL<<0)    // warp portal
class CMirrorParameters {
public:
  ULONG mp_ulFlags;
  // for warps
  CPlacement3D mp_plWarpIn;   // warp entry
  CPlacement3D mp_plWarpOut;  // warp exit
  CEntity *mp_penWarpViewer;  // which entity to view from
  FLOAT mp_fWarpFOV;        // FOV, -1 for no change
};


#endif  /* include-once check. */

