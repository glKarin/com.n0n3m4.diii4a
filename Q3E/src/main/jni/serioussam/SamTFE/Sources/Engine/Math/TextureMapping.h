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

#ifndef SE_INCL_TEXTUREMAPPING_H
#define SE_INCL_TEXTUREMAPPING_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Vector.h>

/* General coordinate naming notes:
 * s,t are texture coordinates in default mapping on a plane
 *   +s=right
 *   +t=down
 * u,v are texture coordinates in mapping on a polygon
 *   +u=right
 *   +v=down
 * i,j are coordinates on screen
 *   +i=right
 *   +j=down
 * k is distance from screen normalized to be in interval [1,+inf>
 *   +1= near clip plane
 *   +inf= far away (infinity) (not normalized to far clip plane!)
 * x,y,z are coordinates in 3D
 *   +x=right
 *   +y=up
 *   -z=front
 */

/*
 * Gradients for a variable lineary interpolated along the screen.
 */
class CPlanarGradients {
public:
  FLOAT pg_f00;       // value at upper left corner
  FLOAT pg_fDOverDI;  // horizontal gradient
  FLOAT pg_fDOverDJ;  // vertical gradient
public:
  // calculate value at some point on screen (i,j)
  inline FLOAT GetValue(FLOAT fI, FLOAT fJ) {
    return pg_f00 + pg_fDOverDI*fI + pg_fDOverDJ*fJ;
  };
  // set constant gradients
  inline void Constant(FLOAT fConst) {
    pg_f00      = fConst;
    pg_fDOverDI = 0;
    pg_fDOverDJ = 0;
  }
  // multiply gradients by given factor
  inline void Multiply(FLOAT fMul) {
    pg_f00      *= fMul;
    pg_fDOverDI *= fMul;
    pg_fDOverDJ *= fMul;
  };
  // add given constant to gradients
  inline void Add(FLOAT fAdd) {
    pg_f00 += fAdd;
  };
  // translate gradients in screen space
  inline void TranslateOnScreen(FLOAT fI, FLOAT fJ) {
    pg_f00 += pg_fDOverDI*fI + pg_fDOverDJ*fJ;
  };
};

/*
 * Description of texture mapping on a polygon relative to default mapping for its plane,
 * version used to communicating information to user.
 */
class ENGINE_API CMappingDefinitionUI {
public:
  ANGLE mdui_aURotation;// rotation angle of u axis from s axis
  ANGLE mdui_aVRotation;// rotation angle of v axis from t axis
  FLOAT mdui_fUStretch; // stretch in u direction
  FLOAT mdui_fVStretch; // stretch in v direction
  FLOAT mdui_fUOffset;  // offset in u direction
  FLOAT mdui_fVOffset;  // offset in v direction
};

/*
 * Vectors defining a transformed mapping on a plane.
 */
class ENGINE_API CMappingVectors {
public:
  FLOAT3D mv_vO;  // origin
  FLOAT3D mv_vU;  // u axis direction
  FLOAT3D mv_vV;  // v axis direction

  // calculate default texture mapping control points from a plane
  void FromPlane(const FLOATplane3D &plPlane);
  void FromPlane_DOUBLE(const DOUBLEplane3D &plPlane);
  // calculate plane from default mapping vectors
  void ToPlane(FLOATplane3D &plPlane) const;
};


#ifdef NETSTRUCTS_PACKED
#pragma pack(1)
#endif

/*
 * Description of texture mapping on a polygon relative to default mapping for its plane.
 */
class ENGINE_API CMappingDefinition {
public:
  FLOAT md_fUoS; // u vector relative to default mapping
  FLOAT md_fUoT;
  FLOAT md_fVoS; // v vector relative to default mapping
  FLOAT md_fVoT;
  FLOAT md_fUOffset;  // offset in u direction
  FLOAT md_fVOffset;  // offset in v direction

  // default constructor
  inline CMappingDefinition(void) :
    md_fUoS(1), md_fUoT(0),
    md_fVoS(0), md_fVoT(1),
    md_fUOffset(0), md_fVOffset(0) {};

  // convert to human-friendly format
  void ToUI(CMappingDefinitionUI &mdui) const;
  // convert from human-friendly format
  void FromUI(const CMappingDefinitionUI &mdui);

  // convert to mapping vectors
  void ToMappingVectors(const CMappingVectors &mvDefault, CMappingVectors &mvVectors) const;
  // convert from mapping vectors
  void FromMappingVectors(const CMappingVectors &mvDefault, const CMappingVectors &mvVectors);
  // make mapping vectors for this mapping
  void MakeMappingVectors(const CMappingVectors &mvSrc, CMappingVectors &mvDst) const;
  // transform default mapping vectors to mapping vectors for this mapping
  void TransformMappingVectors(const CMappingVectors &mvSrc, CMappingVectors &mvDst) const;

  // project another mapping on this one
  void ProjectMapping(const FLOATplane3D &plOriginal, const CMappingDefinition &mdOriginal,
    const FLOATplane3D &pl);

  // translate mapping in 3D
  void Translate(const CMappingVectors &mvDefault, const FLOAT3D &vTranslation);
  // rotate mapping in 3D
  void Rotate(const CMappingVectors &mvDefault, const FLOAT3D &vRotationOrigin, ANGLE aRotation);
  // center mapping to given point in 3D
  void Center(const CMappingVectors &mvDefault, const FLOAT3D &vNewOrigin);
  // transform mapping from one placement to another
  void Transform(const FLOATplane3D &plPlane,
    const class CPlacement3D &plSource, const class CPlacement3D &plTarget);

  // convert from old version of texture mapping defintion
  void ReadOld_t(CTStream &strm); // throw char *
  // find texture coordinates for an object-space point
  void GetTextureCoordinates(
    const CMappingVectors &mvDefault, const FLOAT3D &vSpace, MEX2D &vTexture) const;
  // find object-space coordinates for a texture point
  void GetSpaceCoordinates(
    const CMappingVectors &mvDefault, const MEX2D &vTexture, FLOAT3D &vSpace) const;
};

#ifdef NETSTRUCTS_PACKED
#pragma pack()
#endif

// stream operations
ENGINE_API CTStream &operator>>(CTStream &strm, CMappingDefinition &md);  // throw char *
ENGINE_API CTStream &operator<<(CTStream &strm, const CMappingDefinition &md);  // throw char *


#endif  /* include-once check. */

