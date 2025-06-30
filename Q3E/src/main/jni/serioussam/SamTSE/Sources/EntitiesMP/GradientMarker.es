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

230
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Marker";
uses "EntitiesMP/WorldBase";

class CGradientMarker: CMarker {
name      "Gradient Marker";
thumbnail "Thumbnails\\GradientMarker.tbn";
features "IsImportant";

properties:
  1 FLOAT m_fHeight        "Height" 'H' = -10.0f,
  2 BOOL m_bDarkLight      "Dark light" 'D' = TRUE,
  3 COLOR m_colColor0      "Color 0" 'C' = (C_GRAY|CT_OPAQUE),
  4 COLOR m_colColor1      "Color 1" 'A' = (C_WHITE|CT_OPAQUE),

components:
  1 model   MODEL_MARKER     "Models\\Editor\\GradientMarker.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\GradientMarker.tex"

functions:

  /* Get gradient type name, return empty string if not used. */
  const CTString &GetGradientName(void)
  {
    return m_strName;
  }
  /* Get gradient. */
  BOOL GetGradient(INDEX iGradient, class CGradientParameters &gpGradient)
  {
    const FLOATmatrix3D &m = GetRotationMatrix();
    gpGradient.gp_vGradientDir(1) = m(1,2);
    gpGradient.gp_vGradientDir(2) = m(2,2);
    gpGradient.gp_vGradientDir(3) = m(3,2);
    FLOAT fPos = gpGradient.gp_vGradientDir%GetPlacement().pl_PositionVector;
    gpGradient.gp_bDark = m_bDarkLight;
    if( m_fHeight>=0 && m_fHeight<+0.001f) { m_fHeight = +0.001f; }
    if( m_fHeight<=0 && m_fHeight>-0.001f) { m_fHeight = -0.001f; }
    gpGradient.gp_fH0 = fPos;
    gpGradient.gp_fH1 = fPos+m_fHeight;
    gpGradient.gp_col0 = m_colColor0;
    gpGradient.gp_col1 = m_colColor1;
    return TRUE;
  }

  void UncacheShadowsForGradient(void)
  {
    // for all entities in world
    FOREACHINDYNAMICCONTAINER(GetWorld()->wo_cenEntities, CEntity, iten) {
      // if it is world base entity
      if( IsOfClass(&*iten, "WorldBase")) {
        // uncache shadows for gradient
        ((CWorldBase *)&*iten)->UncacheShadowsForGradient(this);
      }
    }
  }

  void SetPlacement_internal(const CPlacement3D &plNew, const FLOATmatrix3D &mRotation, BOOL bNear)
  {
    CEntity::SetPlacement_internal(plNew, mRotation, bNear);
    UncacheShadowsForGradient();
  }

  void OnEnd(void)
  {
    UncacheShadowsForGradient();
    CEntity::OnEnd();
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
    if( m_strName=="Marker") {
      m_strName = "Gradient marker";
    }

    UncacheShadowsForGradient();
    return;
  }
};
