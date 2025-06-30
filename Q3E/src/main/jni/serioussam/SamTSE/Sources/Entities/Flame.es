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

504
%{
#include "Entities/StdH/StdH.h"
%}

uses "Entities/Light";

// input parameter for flame
event EFlame {
  CEntityPointer penOwner,        // entity which owns it
  CEntityPointer penAttach,       // entity on which flame is attached (his parent)
};

%{
void CFlame_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->PrecacheModel(MODEL_FLAME);
  pdec->PrecacheTexture(TEXTURE_FLAME);
  pdec->PrecacheSound(SOUND_FLAME);
}
%}

class CFlame : CMovableModelEntity {
name      "Flame";
thumbnail "";
features "ImplementsOnPrecache", "CanBePredictable";

properties:
  1 CEntityPointer m_penOwner,    // entity which owns it
  2 CEntityPointer m_penAttach,   // entity on which flame is attached (his parent)
  5 BOOL m_bLoop = FALSE,                 // internal for loops

 10 CSoundObject m_soEffect,      // sound channel

{
  CLightSource m_lsLightSource;
}

components:
  1 class   CLASS_LIGHT         "Classes\\Light.ecl",

// ********* FLAME *********
 10 model   MODEL_FLAME         "Models\\Effects\\Flame\\Flame.mdl",
 //"Models\\Weapons\\Flamer\\Projectile\\Invisible.mdl",
 11 texture TEXTURE_FLAME       "Models\\Effects\\Flame\\Flame.tex",
 12 sound   SOUND_FLAME         "Sounds\\Fire\\Fire4.wav",

functions:
  // add to prediction any entities that this entity depends on
  void AddDependentsToPrediction(void)
  {
    m_penOwner->AddToPrediction();
  }
  // postmoving
  void PostMoving(void) {
    CMovableModelEntity::PostMoving();

    // if no air
    CContentType &ctDn = GetWorld()->wo_actContentTypes[en_iDnContent];
    // stop existing
    if (!(ctDn.ct_ulFlags&CTF_BREATHABLE_LUNGS)) {
      SendEvent(EEnd());
    }

    // never remove from list of movers
    en_ulFlags &= ~ENF_INRENDERING;
    // not moving in fact, only moving with its parent
    en_plLastPlacement = en_plPlacement;
  };

  /* Read from stream. */
  void Read_t( CTStream *istr) // throw char *
  {
    CRationalEntity::Read_t(istr);
    SetupLightSource();
  }

  /* Get static light source information. */
  CLightSource *GetLightSource(void)
  {
    if (!IsPredictor()) {
      return &m_lsLightSource;
    } else {
      return NULL;
    }
  }

  // Setup light source
  void SetupLightSource(void)
  {
    // setup light source
    CLightSource lsNew;
    lsNew.ls_ulFlags = LSF_NONPERSISTENT|LSF_DYNAMIC;
    lsNew.ls_colColor = C_dYELLOW;
    lsNew.ls_rFallOff = 2.0f;
    lsNew.ls_rHotSpot = 0.2f;
    lsNew.ls_plftLensFlare = NULL;
    lsNew.ls_ubPolygonalMask = 0;
    lsNew.ls_paoLightAnimation = NULL;

    m_lsLightSource.ls_penEntity = this;
    m_lsLightSource.SetLightSource(lsNew);
  }

/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  // --->>> MAIN
  Main(EFlame ef) {
    // attach to parent (another entity)
    ASSERT(ef.penOwner!=NULL);
    ASSERT(ef.penAttach!=NULL);
    m_penOwner = ef.penOwner;
    m_penAttach = ef.penAttach;
    SetParent(ef.penAttach);

    // initialization
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_FLYING);
    SetCollisionFlags(ECF_FLAME);
    SetFlags(GetFlags() | ENF_SEETHROUGH);

    // if parent is model
    if (m_penAttach->GetRenderType()==CEntity::RT_MODEL) {
      // stretch to its size
      FLOATaabbox3D box;
      m_penAttach->GetBoundingBox(box);
      GetModelObject()->StretchModel(box.Size());
    }
    ModelChangeNotify();

    SetModel(MODEL_FLAME);
    SetModelMainTexture(TEXTURE_FLAME);
    // play the burning sound
    m_soEffect.Set3DParameters(5.0f, 1.0f, 1.0f, 1.0f);
    PlaySound(m_soEffect, SOUND_FLAME, SOF_3D|SOF_LOOP);

    // setup light source
    SetupLightSource();

    // must always be in movers, to find sector content type
    AddToMovers();

    // burning damage
    SpawnReminder(this, 7.5f, 0);
    m_bLoop = TRUE;
    while(m_bLoop) {
      wait(0.25f) {
        // damage to parent
        on (EBegin) : {
          // if parent does not exist anymore
          if (m_penAttach==NULL || (m_penAttach->GetFlags()&ENF_DELETED)) {
            // stop existing
            m_bLoop = FALSE;
            stop;
          }
          // inflict damage to parent
          m_penAttach->InflictDirectDamage(m_penAttach, m_penOwner, DMT_BURNING, 1.0f, FLOAT3D(0, 0, 0), -en_vGravityDir);
          resume;
        }
        on (EFlame ef) : {
          m_penOwner = ef.penOwner;
          resume;
        };
        on (ETimer) : { stop; }
        on (EReminder) : {
          m_bLoop = FALSE;
          stop;
        }
        on (EEnd) : {
          m_bLoop = FALSE;
          stop;
        }
      }
    }

    // cease to exist
    Destroy();
    return;
  }
};
