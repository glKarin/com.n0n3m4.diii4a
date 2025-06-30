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

215
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";

enum FogAttenuationType {
  0 FA_LINEAR   "Linear",
  1 FA_EXP      "Exp",
  2 FA_EXP2     "Exp2",
};
enum FogGraduationType2 {
  0 FG_CONSTANT   "Constant",
  1 FG_LINEAR     "Linear",
  2 FG_EXP        "Exp",
};

class CFogMarker: CMarker {
name      "Fog Marker";
thumbnail "Thumbnails\\FogMarker.tbn";
features "IsImportant";

properties:
  1 FLOAT m_fDepth        "Depth" 'E' = 10.0f,
  2 FLOAT m_fAbove        "Above" 'O' = 20.0f,
  3 FLOAT m_fBelow        "Below" 'B' = 20.0f,
  4 FLOAT m_fFar          "Far"   'F' = 100.0f,

 10 enum FogAttenuationType m_faType     "Attenuation Type" 'A' =FA_EXP,
 11 FLOAT m_fDensity      "Density"  'D' = 0.1f,
 12 enum FogGraduationType2 m_fgType     "Graduation Type" 'G' =FG_CONSTANT,
 13 FLOAT m_fGraduation   "Graduation"  'R' = 0.1f,

 // for indirect density calculation
 14 BOOL m_bDensityDirect "Density Direct" = TRUE,
 15 FLOAT m_fDensityPercentage "DensityPercentage" = 0.95f,
 16 FLOAT m_fDensityDistance "DensityDistance" = 10.0f,

 // for indirect graduation calculation
 17 BOOL m_bGraduationDirect "Graduation Direct" = TRUE,
 18 FLOAT m_fGraduationPercentage "GraduationPercentage" = 0.95f,
 19 FLOAT m_fGraduationDistance "GraduationDistance" = 10.0f,

 22 INDEX m_iSizeL       "Size Distance" 'S' = 32,
 23 INDEX m_iSizeH       "Size Depth" 'I' = 16,
 24 COLOR m_colColor     "Color" 'C' = (C_WHITE|CT_OPAQUE),

components:
  1 model   MODEL_MARKER     "Models\\Editor\\Fog.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\Fog.tex"

functions:

  /* Get fog type name, return empty string if not used. */
  const CTString &GetFogName(void)
  {
    return m_strName;
  }
  /* Get fog. */
  void GetFog(class CFogParameters &fpFog)
  {
    const FLOATmatrix3D &m = GetRotationMatrix();
    fpFog.fp_vFogDir(1) = m(1,2);
    fpFog.fp_vFogDir(2) = m(2,2);
    fpFog.fp_vFogDir(3) = m(3,2);
    FLOAT fPos = fpFog.fp_vFogDir%GetPlacement().pl_PositionVector;
    fpFog.fp_colColor = m_colColor;
    fpFog.fp_atType = (AttenuationType) m_faType;
    fpFog.fp_fDensity = m_fDensity;
    fpFog.fp_fgtType = (FogGraduationType) m_fgType;
    fpFog.fp_fGraduation = m_fGraduation;
    fpFog.fp_fH0 = fPos-m_fDepth-m_fBelow;
    fpFog.fp_fH1 = fPos-m_fDepth;
    fpFog.fp_fH2 = fPos;
    fpFog.fp_fH3 = fPos+m_fAbove;
    fpFog.fp_fFar = m_fFar;
    fpFog.fp_iSizeH = m_iSizeH;
    fpFog.fp_iSizeL = m_iSizeL;
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
      m_strName = "Fog marker";
    }

    // if density is calculated indirectly
    if (!m_bDensityDirect) {
      // calculate density to have given percentage at given distance
      switch(m_faType) {
      case FA_LINEAR: 
        m_fDensity = m_fDensityPercentage/m_fDensityDistance;
        break;
      case FA_EXP: 
        m_fDensity = -log(1-m_fDensityPercentage)/m_fDensityDistance;
        break;
      case FA_EXP2: 
        m_fDensity = Sqrt(-log(1-m_fDensityPercentage))/m_fDensityDistance;
        break;
      }
    }

    // if graduation is calculated indirectly
    if (!m_bGraduationDirect) {
      // calculate graduation to have given percentage at given depth
      switch(m_fgType) {
      case FG_LINEAR: 
        m_fGraduation = m_fGraduationPercentage/m_fGraduationDistance;
        break;
      case FG_EXP: 
        m_fGraduation = -log(1-m_fGraduationPercentage)/m_fGraduationDistance;
        break;
      }
    }

    
    // clamp values to valid ranges
    m_fDensity = ClampDn(m_fDensity, 1E-6f);

    m_fDepth = ClampDn(m_fDepth , 0.001f);
    m_fAbove = ClampDn(m_fAbove , 0.001f);
    m_fBelow = ClampDn(m_fBelow , 0.001f);
    m_fFar   = ClampDn(m_fFar,    0.001f);

    m_iSizeL = 1<<INDEX(Log2(m_iSizeL));
    m_iSizeH = 1<<INDEX(Log2(m_iSizeH));
    m_iSizeL= Clamp(m_iSizeL, INDEX(2), INDEX(256));
    m_iSizeH= Clamp(m_iSizeH, INDEX(2), INDEX(256));

    return;
  }
};
