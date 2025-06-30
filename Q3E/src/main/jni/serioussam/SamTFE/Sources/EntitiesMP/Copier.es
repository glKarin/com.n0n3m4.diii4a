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

225
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/BasicEffects";

class CCopier : CRationalEntity {
name      "Copier";
thumbnail "Thumbnails\\Copier.tbn";
features  "HasName", "HasTarget", "IsTargetable";

properties:

  1 CTString m_strName          "Name" 'N' = "Copier",
  3 CTString m_strDescription = "",
  2 CEntityPointer m_penTarget  "Target" 'T' COLOR(C_BROWN|0xFF),
  4 BOOL m_bSpawnEffect "Spawn Effect" 'X' = TRUE,
  5 BOOL m_bTelefrag "Telefrag" 'F' = TRUE,


components:

  1 model   MODEL_TELEPORT     "Models\\Editor\\Copier.mdl",
  2 texture TEXTURE_TELEPORT   "Models\\Editor\\Copier.tex",
  3 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",


functions:

  const CTString &GetDescription(void) const
  {
    ((CTString&)m_strDescription).PrintF("-><none>");
    if (m_penTarget!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", (const char *) m_penTarget->GetName());
    }
    return m_strDescription;
  }

  void TeleportEntity()
  {
    // if the target doesn't exist, or is destroyed
    if (m_penTarget==NULL || (m_penTarget->GetFlags()&ENF_DELETED)) {
      // do nothing
      return;
    }

    CEntity *pen = GetWorld()->CopyEntityInWorld( *m_penTarget,
      CPlacement3D(FLOAT3D(-32000.0f+FRnd()*200.0f, -32000.0f+FRnd()*200.0f, 0), ANGLE3D(0, 0, 0)) );

    // teleport back
    CPlacement3D pl = GetPlacement();
    pl.pl_PositionVector += GetRotationMatrix().GetColumn(2)*0.05f;
    pen->Teleport(pl, m_bTelefrag);

    // spawn teleport effect
    if (m_bSpawnEffect) {
      ESpawnEffect ese;
      ese.colMuliplier = C_WHITE|CT_OPAQUE;
      ese.betType = BET_TELEPORT;
      ese.vNormal = FLOAT3D(0,1,0);
      FLOATaabbox3D box;
      pen->GetBoundingBox(box);
      FLOAT fEntitySize = box.Size().MaxNorm()*2;
      ese.vStretch = FLOAT3D(fEntitySize, fEntitySize, fEntitySize);
      CEntityPointer penEffect = CreateEntity(GetPlacement(), CLASS_BASIC_EFFECT);
      penEffect->Initialize(ese);
    }
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CCopier) - sizeof(CRationalEntity) + CRationalEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strDescription.Length();
    slUsedMemory += m_strName.Length();
    return slUsedMemory;
  }



procedures:


  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_TOUCHMODEL);

    // set appearance
    SetModel(MODEL_TELEPORT);
    SetModelMainTexture(TEXTURE_TELEPORT);

    while (TRUE) {
      // wait to someone enter and teleport it
      wait() {
        on (ETrigger eTrigger) : {
          if (m_penTarget!=NULL) {
            TeleportEntity();
          }
          stop;
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

