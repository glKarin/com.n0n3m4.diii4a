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

216
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";
uses "EntitiesMP/FogMarker";


class CHazeMarker: CMarker {
name      "Haze Marker";
thumbnail "Thumbnails\\HazeMarker.tbn";
features "IsImportant";

properties:
 10 enum FogAttenuationType m_faType     "Attenuation Type" 'A' =FA_EXP,
 11 FLOAT m_fDensity      "Density"  'D' = 0.1f,
 12 FLOAT m_fNear         "Near" = 100.0f,
 13 FLOAT m_fFar          "Far"  = 1000.0f,
 14 BOOL m_bVisibleFromOutside    "Visible from outside"  = FALSE,

 22 INDEX m_iSize         "Size" = 32,
 23 COLOR m_colBase        "Base Color" 'C' = (C_WHITE|CT_OPAQUE),
 24 COLOR m_colUp          "Color (up)"     = (C_BLACK|CT_TRANSPARENT),
 25 COLOR m_colDown        "Color (down)"   = (C_BLACK|CT_TRANSPARENT),
 26 COLOR m_colNorth       "Color (north)"  = (C_BLACK|CT_TRANSPARENT),
 27 COLOR m_colSouth       "Color (south)"  = (C_BLACK|CT_TRANSPARENT),
 28 COLOR m_colEast        "Color (east)"   = (C_BLACK|CT_TRANSPARENT),
 29 COLOR m_colWest        "Color (west)"   = (C_BLACK|CT_TRANSPARENT),

components:
  1 model   MODEL_MARKER     "Models\\Editor\\Haze.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\Haze.tex"

functions:

  /* Get haze type name, return empty string if not used. */
  const CTString &GetHazeName(void)
  {
    return m_strName;
  }

  /* Get haze. */
  void GetHaze(class CHazeParameters &hpHaze, FLOAT3D &vViewDir)
  {
    // calculate directional haze color
    COLOR colDir=C_BLACK, colMul; 
    FLOAT fR=0.0f, fG=0.0f, fB=0.0f, fA=0.0f;
    FLOAT fSum = 255.0f / (Abs(vViewDir(1))+Abs(vViewDir(2))+Abs(vViewDir(3)));

    if( vViewDir(1) < 0.0f) {
      colMul = (COLOR)(-vViewDir(1)*fSum);
      colMul = (colMul<<CT_RSHIFT) | (colMul<<CT_GSHIFT) | (colMul<<CT_BSHIFT) | (colMul<<CT_ASHIFT);
      colDir = AddColors( colDir, MulColors( m_colWest, colMul));
    }
    if( vViewDir(1) > 0.0f) {
      colMul = (COLOR)(+vViewDir(1)*fSum);
      colMul = (colMul<<CT_RSHIFT) | (colMul<<CT_GSHIFT) | (colMul<<CT_BSHIFT) | (colMul<<CT_ASHIFT);
      colDir = AddColors( colDir, MulColors( m_colEast, colMul));
    }
    if( vViewDir(2) < 0.0f) {
      colMul = (COLOR)(-vViewDir(2)*fSum);
      colMul = (colMul<<CT_RSHIFT) | (colMul<<CT_GSHIFT) | (colMul<<CT_BSHIFT) | (colMul<<CT_ASHIFT);
      colDir = AddColors( colDir, MulColors( m_colDown, colMul));
    }
    if( vViewDir(2) > 0.0f) {
      colMul = (COLOR)(+vViewDir(2)*fSum);
      colMul = (colMul<<CT_RSHIFT) | (colMul<<CT_GSHIFT) | (colMul<<CT_BSHIFT) | (colMul<<CT_ASHIFT);
      colDir = AddColors( colDir, MulColors( m_colUp, colMul));
    }
    if( vViewDir(3) < 0.0f) {
      colMul = (COLOR)(-vViewDir(3)*fSum);
      colMul = (colMul<<CT_RSHIFT) | (colMul<<CT_GSHIFT) | (colMul<<CT_BSHIFT) | (colMul<<CT_ASHIFT);
      colDir = AddColors( colDir, MulColors( m_colNorth, colMul));
    }
    if( vViewDir(3) > 0.0f) {
      colMul = (COLOR)(+vViewDir(3)*fSum);
      colMul = (colMul<<CT_RSHIFT) | (colMul<<CT_GSHIFT) | (colMul<<CT_BSHIFT) | (colMul<<CT_ASHIFT);
      colDir = AddColors( colDir, MulColors( m_colSouth, colMul));
    }

    // blend base and direction haze color
    colDir = AddColors( colDir, m_colBase);

    // assign parameters
    hpHaze.hp_colColor = colDir;
    hpHaze.hp_atType = (AttenuationType) m_faType;
    hpHaze.hp_fDensity = m_fDensity;
    hpHaze.hp_fNear = m_fNear;
    hpHaze.hp_fFar  = m_fFar;
    hpHaze.hp_iSize = m_iSize;
    hpHaze.hp_ulFlags = 0;
    if (m_bVisibleFromOutside) {
      hpHaze.hp_ulFlags |= HPF_VISIBLEFROMOUTSIDE;
    }
  }

procedures:

  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    // set name
    if (m_strName=="Marker") {
      m_strName = "Haze marker";
    }

    // clamp values to valid ranges
    m_fDensity = ClampDn(m_fDensity, 1E-6f);
    m_fFar = ClampDn(m_fFar, 0.001f);
    m_fNear = Clamp(m_fNear, 0.0f, m_fFar-0.0005f);
    ASSERT(m_fNear>=0 && m_fNear<m_fFar);

    m_iSize = 1<<INDEX(Log2(m_iSize));
    m_iSize= Clamp(m_iSize, INDEX(2), INDEX(256));

    return;
  }
};
