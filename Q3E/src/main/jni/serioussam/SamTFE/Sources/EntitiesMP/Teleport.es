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

219
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/BasicEffects";

class CTeleport : CRationalEntity {
name      "Teleport";
thumbnail "Thumbnails\\Teleport.tbn";
features  "HasName", "HasTarget", "IsTargetable", "IsImportant";


properties:

  1 CTString m_strName          "Name" 'N' = "Teleport",
  3 CTString m_strDescription = "",
  2 CEntityPointer m_penTarget  "Target" 'T' COLOR(C_BROWN|0xFF),
  4 FLOAT m_fWidth              "Width"  'W' = 2.0f,
  5 FLOAT m_fHeight             "Height" 'H' = 3.0f,
  6 BOOL m_bActive              "Active" 'A' = TRUE,
  7 BOOL m_bPlayersOnly         "Players only" 'P' = TRUE,
  8 BOOL m_bForceStop           "Force stop" 'F' = FALSE,


components:

  1 model   MODEL_TELEPORT     "Models\\Editor\\Teleport.mdl",
  2 texture TEXTURE_TELEPORT   "Models\\Editor\\Teleport.tex",
  3 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",


functions:

  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("-><none>");
    if (m_penTarget!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", (const char *) m_penTarget->GetName());
    }
    return m_strDescription;
  }

  void TeleportEntity(CEntity *pen, const CPlacement3D &pl)
  {
    // teleport back
    pen->Teleport(pl);

    // spawn teleport effect
    ESpawnEffect ese;
    ese.colMuliplier = C_WHITE|CT_OPAQUE;
    ese.betType = BET_TELEPORT;
    ese.vNormal = FLOAT3D(0,1,0);
    FLOATaabbox3D box;
    pen->GetBoundingBox(box);
    FLOAT fEntitySize = box.Size().MaxNorm()*2;
    ese.vStretch = FLOAT3D(fEntitySize, fEntitySize, fEntitySize);
    CEntityPointer penEffect = CreateEntity(pl, CLASS_BASIC_EFFECT);
    penEffect->Initialize(ese);
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CTeleport) - sizeof(CRationalEntity) + CRationalEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strName.Length();
    slUsedMemory += m_strDescription.Length();
    return slUsedMemory;
  }


procedures:


  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_TOUCHMODEL);

    // correct height so teleport could collide as sphere
    if(m_fHeight<m_fWidth)
    {
      m_fHeight=m_fWidth;
    }

    // set appearance
    GetModelObject()->StretchModel(FLOAT3D(m_fWidth, m_fHeight, m_fWidth));
    SetModel(MODEL_TELEPORT);
    ModelChangeNotify();
    SetModelMainTexture(TEXTURE_TELEPORT);

    while (TRUE) {
      // wait to someone enter and teleport it
      wait() {
        on (EPass ePass) : {
          if (m_penTarget!=NULL && m_bActive) {
            if (m_bPlayersOnly && !IsOfClass(ePass.penOther, "Player")) {
            resume;
            }
            TeleportEntity(ePass.penOther, m_penTarget->GetPlacement());
            if (m_bForceStop && (ePass.penOther->GetPhysicsFlags()&EPF_MOVABLE) ) {
              ((CMovableEntity*)&*ePass.penOther)->ForceFullStop();
            }
            stop;
          }
          resume;
        }
        on (EActivate) : {
          m_bActive = TRUE;
          resume;
        }
        on (EDeactivate) : {
          m_bActive = FALSE;
          resume;
        }
        otherwise() : {
          resume;
        };
      };
      
      // wait a bit to recover
      autowait(0.1f);
    }
  }
};

