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

217
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Debris";
uses "EntitiesMP/ModelHolder2";
uses "EntitiesMP/BasicEffects";
uses "EntitiesMP/BloodSpray";
uses "EntitiesMP/SoundHolder";

// event sent to entities in range of model destroy
// (e.g light can turn off)
event ERangeModelDestruction {
};

// type of debris
enum DestructionDebrisType {
  1 DDT_STONE "Stone",
  2 DDT_WOOD  "Wood",
  3 DDT_PALM  "Palm",
  4 DDT_CHILDREN_CUSTOM "Custom (children)",
};

class CModelDestruction : CEntity {
name      "ModelDestruction";
thumbnail "Thumbnails\\ModelDestruction.tbn";
features  "HasName", "IsTargetable", "IsImportant";

properties:
  1 CTString m_strName          "Name" 'N' = "ModelDestruction",
  2 CTString m_strDescription = "",

 10 CEntityPointer m_penModel0  "Model 0" 'M' COLOR(C_RED|0x80),
 11 CEntityPointer m_penModel1  "Model 1" COLOR(C_RED|0x80),
 12 CEntityPointer m_penModel2  "Model 2" COLOR(C_RED|0x80),
 13 CEntityPointer m_penModel3  "Model 3" COLOR(C_RED|0x80),
 14 CEntityPointer m_penModel4  "Model 4" COLOR(C_RED|0x80),

 20 FLOAT m_fHealth "Health" 'H' = 50.0f,   // health of the model pointing to this
 22 enum DestructionDebrisType m_ddtDebris "Debris" 'D' = DDT_STONE,  // type of debris
 23 INDEX m_ctDebris "Debris Count" = 3,
 24 FLOAT m_fDebrisSize "Debris Size" = 1.0f,
 25 enum EntityInfoBodyType m_eibtBodyType "Body Type" = EIBT_ROCK,
 26 enum SprayParticlesType m_sptType "Particle Type" = SPT_NONE, // type of particles
 27 FLOAT m_fParticleSize "Particle Size" 'Z' = 1.0f, // size of particles
 28 BOOL m_bRequireExplosion "Requires Explosion" = FALSE,
 29 FLOAT m_fDebrisLaunchPower "CC: Debris Launch Power" 'L' = 1.0f, // launch power of debris
 30 INDEX /*enum DebrisParticlesType*/ m_dptParticles "CC: Trail particles" = DPT_NONE,
 31 INDEX /*enum BasicEffectType*/ m_betStain "CC: Leave stain" = BET_NONE,
 32 FLOAT m_fLaunchCone "CC: Launch cone" = 45.0f,
 33 FLOAT m_fRndRotH "CC: Rotation heading" = 720.0f,
 34 FLOAT m_fRndRotP "CC: Rotation pitch" = 720.0f,
 35 FLOAT m_fRndRotB "CC: Rotation banking" = 720.0f,
 36 FLOAT m_fParticleLaunchPower "Particle Launch Power" 'P' = 1.0f, // launch power of particles
 37 COLOR m_colParticles "Central Particle Color" 'C' = COLOR(C_WHITE|CT_OPAQUE),
 40 ANIMATION m_iStartAnim "Start anim" = -1,
 41 BOOL m_bDebrisImmaterialASAP "Immaterial ASAP" = TRUE,

 50 INDEX m_ctDustFall "Dusts Count" = 1,                                   // count of spawned dust falls
 51 FLOAT m_fMinDustFallHeightRatio "Dust Min Height Ratio" = 0.1f,         // min ratio of model height for dust
 52 FLOAT m_fMaxDustFallHeightRatio "Dust Max Height Ratio" = 0.6f,         // max ratio of model height for dust
 53 FLOAT m_fDustStretch "Dust Stretch" = 1.0f,                             // dust stretch
 54 FLOAT m_fDebrisDustRandom "Dust Debris Random" = 0.25f,                 // random for spawning dusts on debris fall
 55 FLOAT m_fDebrisDustStretch "Dust Debris Stretch" = 1.0f,                // size of spawned dust
 56 CEntityPointer m_penShake "Shake marker" 'A',

components:
  1 model   MODEL_MODELDESTRUCTION     "Models\\Editor\\ModelDestruction.mdl",
  2 texture TEXTURE_MODELDESTRUCTION   "Models\\Editor\\ModelDestruction.tex",
  3 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",

// ************** WOOD PARTS **************
 10 model     MODEL_WOOD        "Models\\Effects\\Debris\\Wood01\\Wood.mdl",
 11 texture   TEXTURE_WOOD      "Models\\Effects\\Debris\\Wood01\\Wood.tex",

 12 model     MODEL_BRANCH        "ModelsMP\\Effects\\Debris\\Tree\\Tree.mdl",
 13 texture   TEXTURE_BRANCH      "ModelsMP\\Plants\\Tree01\\Tree01.tex",

// ************** STONE PARTS **************
 14 model     MODEL_STONE        "Models\\Effects\\Debris\\Stone\\Stone.mdl",
 15 texture   TEXTURE_STONE      "Models\\Effects\\Debris\\Stone\\Stone.tex",

functions:
  void Precache(void) {
    PrecacheClass(CLASS_BASIC_EFFECT, BET_EXPLOSIONSTAIN);
    switch(m_ddtDebris) {
    case DDT_STONE: { 
      PrecacheModel(MODEL_STONE);
      PrecacheTexture(TEXTURE_STONE);
                          } break;
    case DDT_WOOD:   {  
      PrecacheModel(MODEL_WOOD);
      PrecacheTexture(TEXTURE_WOOD);
                          } break;
    case DDT_PALM:   {  
      PrecacheModel(MODEL_WOOD);
      PrecacheTexture(TEXTURE_WOOD);
                          } break;
    }
  };

  /* Get anim data for given animation property - return NULL for none. */
  CAnimData *GetAnimData(SLONG slPropertyOffset)
  {
    if(slPropertyOffset==_offsetof(CModelDestruction, m_iStartAnim)) 
    {
      CModelHolder2 *pmh=GetModel(0);
      if(pmh!=NULL)
      {
        return pmh->GetModelObject()->GetData();
      }
    }
    return CEntity::GetAnimData(slPropertyOffset);
  }

  const CTString &GetDescription(void) const {
    INDEX ct = GetModelsCount();
    if(ct==0) {
      ((CTString&)m_strDescription).PrintF("(%g): no more", m_fHealth);
    } else if(ct==1) {
      ((CTString&)m_strDescription).PrintF("(%g): %s", m_fHealth, (const char *) m_penModel0->GetName());
    } else if (TRUE) {
      ((CTString&)m_strDescription).PrintF("(%g): %s,...(%d)", m_fHealth, (const char *) m_penModel0->GetName(), ct);
    }
    return m_strDescription;
  }

  // check if one model target is valid 
  void CheckOneModelTarget(CEntityPointer &pen)
  {
    if (pen!=NULL && !IsOfClass(pen, "ModelHolder2")) {
      WarningMessage("Model '%s' is not ModelHolder2!", (const char *) pen->GetName());
      pen=NULL;
    }
  }

  // get next phase in destruction
  class CModelHolder2 *GetNextPhase(void)
  {
    INDEX ct = GetModelsCount();
    // if not more models
    if (ct==0) {
      // return none
      return NULL;
    // if there are some
    } else {
      // choose by random
      return GetModel(IRnd()%ct);
    }
  }

  // get number of models set by user
  INDEX GetModelsCount(void) const
  {
    // note: only first N that are no NULL are used
    if (m_penModel0==NULL) { return 0; };
    if (m_penModel1==NULL) { return 1; };
    if (m_penModel2==NULL) { return 2; };
    if (m_penModel3==NULL) { return 3; };
    if (m_penModel4==NULL) { return 4; };
    return 5;
  }
  // get model by its index
  class CModelHolder2 *GetModel(INDEX iModel)
  {
    ASSERT(iModel<=GetModelsCount());
    iModel = Clamp(iModel, INDEX(0), GetModelsCount());
    return (CModelHolder2 *) (&m_penModel0)[iModel].ep_pen;
  }
  // spawn debris for given model
  void SpawnDebris(CModelHolder2 *penmhDestroyed)
  {
    FLOATaabbox3D box;
    penmhDestroyed->GetBoundingBox(box);
    FLOAT fEntitySize = box.Size().MaxNorm();
    switch(m_ddtDebris) {
    case DDT_STONE: {
      Debris_Begin(EIBT_ROCK, DPT_NONE, BET_NONE, fEntitySize, FLOAT3D(0,0,0), FLOAT3D(0,0,0), 1.0f, 0.0f);
      for(INDEX iDebris = 0; iDebris<m_ctDebris; iDebris++) {
        Debris_Spawn(penmhDestroyed, this, MODEL_STONE, TEXTURE_STONE, 0, 0, 0, IRnd()%4, m_fDebrisSize,
          FLOAT3D(FRnd()*0.8f+0.1f, FRnd()*0.8f+0.1f, FRnd()*0.8f+0.1f));
      }
                    } break;
    case DDT_WOOD:
    {
      Debris_Begin(EIBT_WOOD, DPT_NONE, BET_NONE, fEntitySize, FLOAT3D(0,0,0), FLOAT3D(0,0,0), 1.0f, 0.0f);
      for(INDEX iDebris = 0; iDebris<m_ctDebris; iDebris++)
      {
        Debris_Spawn(penmhDestroyed, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 0, m_fDebrisSize,
          FLOAT3D(0.5f, 0.5f, 0.5f));
      }
      break;
    }
    case DDT_CHILDREN_CUSTOM:
    {
      Debris_Begin(EIBT_WOOD, DPT_NONE, BET_NONE, 1.0f, FLOAT3D(10,10,10), FLOAT3D(0,0,0), 5.0f, 2.0f);
      // launch all children of model holder type
      FOREACHINLIST( CEntity, en_lnInParent, en_lhChildren, iten)
      {
        if( IsOfClass(&*iten, "ModelHolder2"))
        {
          CModelHolder2 &mhTemplate=(CModelHolder2 &)*iten;
          if( mhTemplate.GetModelObject()==NULL || penmhDestroyed->GetModelObject()==NULL)
          {
            continue;
          }
          CModelObject &moNew=*mhTemplate.GetModelObject();
          CModelObject &moOld=*penmhDestroyed->GetModelObject();
          CPlacement3D plRel=mhTemplate.GetPlacement();
          plRel.AbsoluteToRelative(this->GetPlacement());
          CPlacement3D plLaunch=plRel;
          FLOAT3D vStretch=moOld.mo_Stretch;
          plLaunch.pl_PositionVector(1)=plLaunch.pl_PositionVector(1)*vStretch(1);
          plLaunch.pl_PositionVector(2)=plLaunch.pl_PositionVector(2)*vStretch(2);
          plLaunch.pl_PositionVector(3)=plLaunch.pl_PositionVector(3)*vStretch(3);
          plLaunch.RelativeToAbsolute(penmhDestroyed->GetPlacement());
          ANGLE3D angLaunch=ANGLE3D(FRnd()*360.0f,90.0f+m_fLaunchCone*(FRnd()-0.5f),0);
          FLOAT3D vLaunchDir;
          FLOAT3D vStretchTemplate=FLOAT3D(
            moOld.mo_Stretch(1)*moNew.mo_Stretch(1),
            moOld.mo_Stretch(2)*moNew.mo_Stretch(2),
            moOld.mo_Stretch(3)*moNew.mo_Stretch(3));
          AnglesToDirectionVector(angLaunch, vLaunchDir);
          vLaunchDir.Normalize();
          vLaunchDir=vLaunchDir*m_fDebrisLaunchPower;
          ANGLE3D angRotSpeed=ANGLE3D(m_fRndRotH*2.0f*(FRnd()-0.5f),m_fRndRotP*(FRnd()-0.5f),m_fRndRotB*(FRnd()-0.5f));
          
          FLOAT fDustSize=0.0f;
          if( FRnd()<m_fDebrisDustRandom)
          {
            fDustSize=m_fDebrisDustStretch;
          }
          
          Debris_Spawn_Template( m_eibtBodyType, m_dptParticles, m_betStain,
            penmhDestroyed, this, &mhTemplate, vStretchTemplate, mhTemplate.m_fStretchAll, plLaunch,
            vLaunchDir, angRotSpeed, m_bDebrisImmaterialASAP, fDustSize, penmhDestroyed->m_colBurning);
        }
        if( IsOfClass(&*iten, "SoundHolder"))
        {
          CSoundHolder &ensh=(CSoundHolder &)*iten;
          // copy it at the placement of destroyed model
          CEntity *penNewSH = GetWorld()->CopyEntityInWorld( ensh, penmhDestroyed->GetPlacement());
          penNewSH->SetParent(NULL);
          penNewSH->SendEvent(EStart());
        }
      }
      break;
    }
    case DDT_PALM: {
      Debris_Begin(EIBT_WOOD, DPT_NONE, BET_NONE, fEntitySize, penmhDestroyed->m_vDamage*0.3f, FLOAT3D(0,0,0), 1.0f, 0.0f);
      Debris_Spawn(penmhDestroyed, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 0, m_fDebrisSize,
        FLOAT3D(0.5f, 0.2f, 0.5f));
      Debris_Spawn(penmhDestroyed, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 1, m_fDebrisSize,
        FLOAT3D(0.5f, 0.3f, 0.5f));
      Debris_Spawn(penmhDestroyed, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 2, m_fDebrisSize,
        FLOAT3D(0.5f, 0.4f, 0.5f));
      Debris_Spawn(penmhDestroyed, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 3, m_fDebrisSize,
        FLOAT3D(0.5f, 0.5f, 0.5f));
      Debris_Spawn(penmhDestroyed, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 1, m_fDebrisSize,
        FLOAT3D(0.5f, 0.6f, 0.5f));
      Debris_Spawn(penmhDestroyed, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 2, m_fDebrisSize,
        FLOAT3D(0.5f, 0.8f, 0.5f));
      Debris_Spawn(penmhDestroyed, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 1, m_fDebrisSize,
        FLOAT3D(0.5f, 0.9f, 0.5f));
                    } break;
    default: {} break;
    };
    
    if( m_ctDustFall>0)
    {
      FLOAT fHeight=box.Size()(2);
      FLOAT fMinHeight=fHeight*m_fMinDustFallHeightRatio;
      FLOAT fMaxHeight=fHeight*m_fMaxDustFallHeightRatio;
      FLOAT fHeightSteep=(fMaxHeight-fMinHeight)/m_ctDustFall;
      for(INDEX iDust=0; iDust<m_ctDustFall; iDust++)
      {
        FLOAT fY=fMinHeight+iDust*fHeightSteep;
        CPlacement3D plDust=penmhDestroyed->GetPlacement();
        plDust.pl_PositionVector=plDust.pl_PositionVector+FLOAT3D(0,fY,0);
        // spawn dust effect
        ESpawnEffect ese;
        ese.colMuliplier = C_WHITE|CT_OPAQUE;
        ese.vStretch = FLOAT3D(m_fDustStretch,m_fDustStretch,m_fDustStretch);
        ese.vNormal = FLOAT3D(0,1,0);
        ese.betType = BET_DUST_FALL;
        CEntityPointer penFX = CreateEntity(plDust, CLASS_BASIC_EFFECT);
        penFX->Initialize(ese);
      }
    }
  }

procedures:
  Main()
  {
    // must not allow invalid classes
    CheckOneModelTarget(m_penModel0);
    CheckOneModelTarget(m_penModel1);
    CheckOneModelTarget(m_penModel2);
    CheckOneModelTarget(m_penModel3);
    CheckOneModelTarget(m_penModel4);

    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MODELDESTRUCTION);
    SetModelMainTexture(TEXTURE_MODELDESTRUCTION);

    return;
  }
};

