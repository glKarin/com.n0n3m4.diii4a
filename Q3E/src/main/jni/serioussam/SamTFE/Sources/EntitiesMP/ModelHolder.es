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

203
%{
#include "EntitiesMP/StdH/StdH.h"
%}

class CModelHolder : CEntity {
name      "ModelHolder";
thumbnail "";
features "HasName", "HasDescription";

properties:
  1 CTFileName m_fnModel    "Model" 'M' =CTFILENAME("Models\\Editor\\Axis.mdl"),
  2 CTFileName m_fnTexture  "Texture" 'T' =CTFILENAME("Models\\Editor\\Vector.tex"),
  3 FLOAT m_fStretchAll     "StretchAll" 'S' = 1.0f,
  4 FLOAT m_fStretchX       "StretchX"   'X' = 1.0f,
  5 FLOAT m_fStretchY       "StretchY"   'Y' = 1.0f,
  6 FLOAT m_fStretchZ       "StretchZ"   'Z' = 1.0f,
  7 CTString m_strName      "Name" 'N' ="",
 12 CTString m_strDescription = "",
  8 BOOL m_bColliding       "Colliding" 'C' = FALSE,    // set if model is not immatierial
  9 ANIMATION m_iModelAnimation   "Model animation" 'A' = 0,
 10 ANIMATION m_iTextureAnimation "Texture animation" = 0,
 11 BOOL m_bClusterShadows "Cluster shadows" = FALSE,   // set if model uses cluster shadows
 13 BOOL m_bBackground     "Background" = FALSE,   // set if model is rendered in background

 // parameters for custom shading of a model (overrides automatic shading calculation)
 14 BOOL m_bCustomShading       "Custom shading" 'H' = FALSE,
 15 ANGLE3D m_aShadingDirection "Light direction" = ANGLE3D( AngleDeg(45.0f),AngleDeg(45.0f),AngleDeg(45.0f)),
 16 COLOR m_colLight            "Light color" = C_WHITE,
 17 COLOR m_colAmbient          "Ambient color" = C_BLACK,
 18 CTFileName m_fnmLightAnimation "Light animation file" = CTString(""),
 19 ANIMATION m_iLightAnimation "Light animation" = 0,
 20 CAnimObject m_aoLightAnimation,
{
  CTFileName m_fnOldModel;  // used for remembering last selected model (not saved at all)
}

components:

functions:
  /* Get anim data for given animation property - return NULL for none. */
  CAnimData *GetAnimData(SLONG slPropertyOffset) 
  {
    if (slPropertyOffset==_offsetof(CModelHolder, m_iModelAnimation)) {
      return GetModelObject()->GetData();
    } else if (slPropertyOffset==_offsetof(CModelHolder, m_iTextureAnimation)) {
      return GetModelObject()->mo_toTexture.GetData();
    } else if (slPropertyOffset==_offsetof(CModelHolder, m_iLightAnimation)) {
      return m_aoLightAnimation.GetData();
    } else {
      return CEntity::GetAnimData(slPropertyOffset);
    }
  };

  /* Adjust model shading parameters if needed. */
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    if (m_bCustomShading) {
      // if there is color animation
      if (m_aoLightAnimation.GetData()!=NULL) {
        // get lerping info
        SLONG colFrame0, colFrame1; FLOAT fRatio;
        m_aoLightAnimation.GetFrame( colFrame0, colFrame1, fRatio);
        UBYTE ubAnimR0, ubAnimG0, ubAnimB0;
        UBYTE ubAnimR1, ubAnimG1, ubAnimB1;
        ColorToRGB( colFrame0, ubAnimR0, ubAnimG0, ubAnimB0);
        ColorToRGB( colFrame1, ubAnimR1, ubAnimG1, ubAnimB1);

        // calculate current animation color
        FLOAT fAnimR = NormByteToFloat( Lerp( ubAnimR0, ubAnimR1, fRatio));
        FLOAT fAnimG = NormByteToFloat( Lerp( ubAnimG0, ubAnimG1, fRatio));
        FLOAT fAnimB = NormByteToFloat( Lerp( ubAnimB0, ubAnimB1, fRatio));
        
        // decompose constant colors
        UBYTE ubLightR, ubLightG, ubLightB;
        UBYTE ubAmbientR, ubAmbientG, ubAmbientB;
        ColorToRGB( m_colLight,   ubLightR,   ubLightG,   ubLightB);
        ColorToRGB( m_colAmbient, ubAmbientR, ubAmbientG, ubAmbientB);
        colLight   = RGBToColor( (UBYTE) (ubLightR  *fAnimR), (UBYTE) (ubLightG  *fAnimG), (UBYTE) (ubLightB  *fAnimB));
        colAmbient = RGBToColor( (UBYTE) (ubAmbientR*fAnimR), (UBYTE) (ubAmbientG*fAnimG), (UBYTE) (ubAmbientB*fAnimB));

      // if there is no color animation
      } else {
        colLight   = m_colLight;
        colAmbient = m_colAmbient;
      }

      AnglesToDirectionVector(m_aShadingDirection, vLightDirection);
      vLightDirection = -vLightDirection;
    }
    return TRUE;
  };

  /* Init model holder*/
  void InitModelHolder(void) {
    // stretch factors must not have extreme values
    if (m_fStretchX  < 0.01f) { m_fStretchX   = 0.01f;  }
    if (m_fStretchY  < 0.01f) { m_fStretchY   = 0.01f;  }
    if (m_fStretchZ  < 0.01f) { m_fStretchZ   = 0.01f;  }
    if (m_fStretchAll< 0.01f) { m_fStretchAll = 0.01f;  }
    if (m_fStretchX  >100.0f) { m_fStretchX   = 100.0f; }
    if (m_fStretchY  >100.0f) { m_fStretchY   = 100.0f; }
    if (m_fStretchZ  >100.0f) { m_fStretchZ   = 100.0f; }
    if (m_fStretchAll>100.0f) { m_fStretchAll = 100.0f; }

    // if initialized for the first time
    if (m_fnOldModel=="") {
      // just remember the model filename
      m_fnOldModel = m_fnModel;
    // if re-initialized
    } else {
      // if the model filename has changed
      if (m_fnOldModel != m_fnModel) {
        // set texture filename to same as the model filename with texture extension
        m_fnTexture = m_fnModel.FileDir()+m_fnModel.FileName()+CTString(".tex");
        // remember the model filename
        m_fnOldModel = m_fnModel;
      }
    }

    InitAsModel();
    if (m_bColliding) {
      SetPhysicsFlags(EPF_MODEL_FIXED);
      SetCollisionFlags(ECF_MODEL_HOLDER);
    } else {
      SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
      SetCollisionFlags(ECF_IMMATERIAL);
    }

    if (m_bClusterShadows) {
      SetFlags(GetFlags()|ENF_CLUSTERSHADOWS);
    } else {
      SetFlags(GetFlags()&~ENF_CLUSTERSHADOWS);
    }

    if (m_bBackground) {
      SetFlags(GetFlags()|ENF_BACKGROUND);
    } else {
      SetFlags(GetFlags()&~ENF_BACKGROUND);
    }

    // set model stretch -- MUST BE DONE BEFORE SETTING MODEL!
    GetModelObject()->mo_Stretch = FLOAT3D(
      m_fStretchAll*m_fStretchX,
      m_fStretchAll*m_fStretchY,
      m_fStretchAll*m_fStretchZ);

    // set appearance
    SetModel(m_fnModel);
    SetModelMainTexture(m_fnTexture);

    GetModelObject()->PlayAnim(m_iModelAnimation, AOF_LOOPING);
    GetModelObject()->mo_toTexture.PlayAnim(m_iTextureAnimation, AOF_LOOPING);

    try {
      m_aoLightAnimation.SetData_t(m_fnmLightAnimation);
    } catch ( const char *strError) {
      WarningMessage(TRANS("Cannot load '%s': %s"), (const char *) (CTString&)m_fnmLightAnimation, strError);
      m_fnmLightAnimation = "";
    }
    if (m_aoLightAnimation.GetData()!=NULL) {
      m_aoLightAnimation.PlayAnim(m_iLightAnimation, AOF_LOOPING);
    }

    m_strDescription.PrintF("%s,%s", (const char *) m_fnModel.FileName(), (const char *) m_fnTexture.FileName());

    return;
  };

procedures:
  Main()
  {
    InitModelHolder();
    return;
  }
};
