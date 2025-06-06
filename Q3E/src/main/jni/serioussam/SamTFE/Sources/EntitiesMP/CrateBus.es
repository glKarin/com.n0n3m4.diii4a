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

352
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/CutSequences/CrateBus/CrateBus.h"
#include "ModelsMP/Enemies/Mental/Mental.h"
%}

uses "EntitiesMP/SpawnerProjectile";
uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/BasicEffects";

%{
INDEX _aiLeftAnimations[] = {
  MENTAL_ANIM_LEFTWAVE01,
  MENTAL_ANIM_LEFTWAVE02,
  MENTAL_ANIM_LEFTWAVE03,
  MENTAL_ANIM_LEFTWAVE04,
  MENTAL_ANIM_LEFTWAVE05,
  MENTAL_ANIM_LEFTWAVE06
};

INDEX _aiRightAnimations[] = {
  MENTAL_ANIM_RIGHTWAVE01,
  MENTAL_ANIM_RIGHTWAVE02,
  MENTAL_ANIM_RIGHTWAVE03,
  MENTAL_ANIM_RIGHTWAVE04,
  MENTAL_ANIM_RIGHTWAVE05,
  MENTAL_ANIM_RIGHTWAVE06,
  MENTAL_ANIM_RIGHTWAVE07,
  MENTAL_ANIM_RIGHTWAVE08
};

  CAutoPrecacheSound m_aps;
  CAutoPrecacheTexture m_apt;

#define CT_BIGHEADS 24
%}

class CCrateBus: CMovableModelEntity {
name      "CrateBus";
thumbnail "Thumbnails\\Mental.tbn";
features  "IsTargetable", "HasName", "IsImportant";

properties:
  1 BOOL m_bActive "Active" = TRUE,
  2 FLOAT m_fExplosionStretch "Explosion Stretch" 'E' = 1.0f,
  3 FLOAT m_tmDeath = 0.0f,
  4 CTString m_strName  "Name" 'N' = "Crate bus",
  5 INDEX m_ctMentals=0,
  6 BOOL m_bShowTrail=FALSE,
  7 FLOAT m_fStretch "Stretch" 'S' = 1.0f,
  11 CTFileName m_fnmHeadTex01 "Head texture 01" 'H' = CTString(""),
  12 CTFileName m_fnmHeadTex02 "Head texture 02"     = CTString(""),
  13 CTFileName m_fnmHeadTex03 "Head texture 03"     = CTString(""),
  14 CTFileName m_fnmHeadTex04 "Head texture 04"     = CTString(""),
  15 CTFileName m_fnmHeadTex05 "Head texture 05"     = CTString(""),
  16 CTFileName m_fnmHeadTex06 "Head texture 06"     = CTString(""),
  17 CTFileName m_fnmHeadTex07 "Head texture 07"     = CTString(""),
  18 CTFileName m_fnmHeadTex08 "Head texture 08"     = CTString(""),
  19 CTFileName m_fnmHeadTex09 "Head texture 09"     = CTString(""),
  20 CTFileName m_fnmHeadTex10 "Head texture 10"     = CTString(""),
  21 CTFileName m_fnmHeadTex11 "Head texture 11"     = CTString(""),
  22 CTFileName m_fnmHeadTex12 "Head texture 12"     = CTString(""),
  23 CTFileName m_fnmHeadTex13 "Head texture 13"     = CTString(""),
  24 CTFileName m_fnmHeadTex14 "Head texture 14"     = CTString(""),
  25 CTFileName m_fnmHeadTex15 "Head texture 15"     = CTString(""),
  26 CTFileName m_fnmHeadTex16 "Head texture 16"     = CTString(""),
  27 CTFileName m_fnmHeadTex17 "Head texture 17"     = CTString(""),
  28 CTFileName m_fnmHeadTex18 "Head texture 18"     = CTString(""),
  29 CTFileName m_fnmHeadTex19 "Head texture 19"     = CTString(""),
  30 CTFileName m_fnmHeadTex20 "Head texture 20"     = CTString(""),
  31 CTFileName m_fnmHeadTex21 "Head texture 21"     = CTString(""),
  32 CTFileName m_fnmHeadTex22 "Head texture 22"     = CTString(""),
  33 CTFileName m_fnmHeadTex23 "Head texture 23"     = CTString(""),
  34 CTFileName m_fnmHeadTex24 "Head texture 24"     = CTString(""),

components:
  1 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",
  2 class   CLASS_PROJECTILE    "Classes\\Projectile.ecl",
  3 class   CLASS_SPAWNER_PROJECTILE "Classes\\SpawnerProjectile.ecl",

// ************** DATA **************
  10 model   MODEL_MENTAL           "ModelsMP\\Enemies\\Mental\\Mental.mdl",
  11 texture TEXTURE_MENTAL         "ModelsMP\\Enemies\\Mental\\Mental.tex",
  12 model   MODEL_CRATE_BUS        "ModelsMP\\CutSequences\\CrateBus\\CrateBus.mdl",
  13 texture TEXTURE_CRATE_BUS      "ModelsMP\\CutSequences\\CrateBus\\CrateBus.tex",
  14 model   MODEL_HEAD             "ModelsMP\\Enemies\\Mental\\Head.mdl",
  15 texture TEXTURE_HEAD           "ModelsMP\\Enemies\\Mental\\Head.tex",

functions:

  void Precache(void)
  {
    PrecacheClass(CLASS_BASIC_EFFECT, BET_BOMB);
    PrecacheTexture(TEXTURE_HEAD);
  };

  void AddRiders()
  {
    GetModelObject()->RemoveAllAttachmentModels();
    for( INDEX i=0; i<CT_BIGHEADS; i++)
    {
      AddAttachment(CRATEBUS_ATTACHMENT_1+i, MODEL_MENTAL, TEXTURE_MENTAL);
      CAttachmentModelObject *pamoMental = GetModelObject()->GetAttachmentModel(CRATEBUS_ATTACHMENT_1+i);
      if( pamoMental==NULL) { continue; }
      CModelObject &moMental=pamoMental->amo_moModelObject;
      AddAttachmentToModel(this, moMental, MENTAL_ATTACHMENT_HEAD, MODEL_HEAD, TEXTURE_HEAD, 0, 0, 0);
      CAttachmentModelObject *pamoHead = moMental.GetAttachmentModel(MENTAL_ATTACHMENT_HEAD);
      if (pamoHead==NULL) { continue; }
      CTFileName fnm=(&m_fnmHeadTex01)[i];
      if (fnm!="")
      {
        // try to
        try
        {
          pamoHead->amo_moModelObject.mo_toTexture.SetData_t(fnm);
        }
        // if anything failed
        catch ( const char *strError)
        {
          // report error
          CPrintF("%s\n", (const char *)strError);
        }
      }
      INDEX iRndLeft=IRnd()%(sizeof(_aiLeftAnimations)/sizeof(INDEX));
      INDEX iRndRight=IRnd()%(sizeof(_aiRightAnimations)/sizeof(INDEX));
      if(i&1)
      {
        moMental.PlayAnim(_aiRightAnimations[iRndRight], AOF_LOOPING);
      }
      else    
      {
        moMental.PlayAnim(_aiLeftAnimations[iRndLeft], AOF_LOOPING);
      }
      FLOAT tmOffsetPhase=-FRnd()*10.0f;
      moMental.OffsetPhase(tmOffsetPhase);
    }
  }

  // particles
  void RenderParticles(void)
  {
    CEntity *penParent=GetParent();
    if( m_bShowTrail && penParent!=NULL)
    {
      Particles_AfterBurner( penParent, 0.0f, 0.5f);
      //Particles_RocketTrail(penParent, 25.0f);
    }
  }

  void SpawnExplosion(INDEX iCharacter, FLOAT fAddY, FLOAT fSize)
  {
    FLOAT3D vOffset=FLOAT3D(0,0,0);
    // spawn explosion
    if(iCharacter>=0)
    {
      CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel(iCharacter);
      if( pamo==NULL && fAddY>=0) {return;}
      GetModelObject()->RemoveAttachmentModel(iCharacter);
      m_ctMentals--;
      // character pos
      INDEX iX=iCharacter%2;
      INDEX iZ=iCharacter/2;
      vOffset=FLOAT3D(-1.0f+iX*2.0f, 3.0f+(FRnd())*1.0f+fAddY, -14.5f+iZ*2.8f)*m_fStretch;
    }
    else
    {
      // rnd pos
      vOffset=FLOAT3D( (FRnd()-0.5f)*4.0f, 3.0f+(FRnd())*1.0f+fAddY, (FRnd()-0.5f)*36.0f)*m_fStretch;
    }

    CPlacement3D plExplosion = GetPlacement();
    plExplosion.pl_PositionVector=plExplosion.pl_PositionVector+vOffset;
    CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_BOMB;
    eSpawnEffect.vStretch = FLOAT3D(m_fExplosionStretch,m_fExplosionStretch,m_fExplosionStretch);
    penExplosion->Initialize(eSpawnEffect);

    // explosion debris
    eSpawnEffect.betType = BET_EXPLOSION_DEBRIS;
    CEntityPointer penExplosionDebris = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosionDebris->Initialize(eSpawnEffect);

    // explosion smoke
    eSpawnEffect.betType = BET_EXPLOSION_SMOKE;
    CEntityPointer penExplosionSmoke = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosionSmoke->Initialize(eSpawnEffect);
  }

  CPlacement3D GetLerpedPlacement(void) const
  {
    return CEntity::GetLerpedPlacement(); // we never move anyway, so let's be able to be parented
  }

procedures:
  Die()
  {
    // for each child of this entity
    {FOREACHINLIST(CEntity, en_lnInParent, en_lhChildren, itenChild) {
      // send it destruction event
      itenChild->SendEvent(ERangeModelDestruction());
    }}

    /*
    m_tmDeath=_pTimer->CurrentTick();
    m_ctMentals=TEXTURE_HEAD20-TEXTURE_HEAD01;
    while((_pTimer->CurrentTick()<m_tmDeath+12.0f) && (m_ctMentals>0))
    {
      autowait(_pTimer->TickQuantum);
      SpawnExplosion(IRnd()%(TEXTURE_HEAD20-TEXTURE_HEAD01),0,1);
      if(IRnd()%1) { SpawnExplosion(-1,0,1);}
      if(IRnd()%1) {SpawnExplosion(IRnd()%(TEXTURE_HEAD20-TEXTURE_HEAD01),0,1);}
      if(IRnd()%1) { SpawnExplosion(-1,0,1);}
    }
    */

    for(INDEX iChar=0; iChar<CT_BIGHEADS; iChar+=1)
    {
      // character pos
      INDEX iX=iChar%2;
      INDEX iZ=iChar/2;
      FLOAT fAddY=1.0f*m_fStretch;
      FLOAT3D vOffset=FLOAT3D(-1.0f+iX*2.0f, 3.0f+(FRnd())*1.0f+fAddY, -14.5f+iZ*2.8f)*m_fStretch;
      FLOAT3D vPos = GetPlacement().pl_PositionVector+vOffset;
      CEntityPointer penDebris = GetWorld()->CreateEntity_t( CPlacement3D(vPos, ANGLE3D(0,0,0)),
        CTFILENAME("Classes\\Debris.ecl"));
      // prepare parameters
      ESpawnDebris eSpawn;
      eSpawn.bImmaterialASAP=FALSE;
      eSpawn.fDustStretch=4.0f;
      eSpawn.bCustomShading=FALSE;
      eSpawn.colDebris = C_WHITE|CT_OPAQUE;
      eSpawn.Eeibt = EIBT_FLESH;
      eSpawn.dptParticles = DPT_AFTERBURNER;
      eSpawn.betStain = BET_BLOODSTAINGROW;
      eSpawn.pmd = GetModelDataForComponent(MODEL_HEAD);
      eSpawn.ptd = GetTextureDataForComponent(TEXTURE_HEAD);
      eSpawn.ptdRefl = NULL;
      eSpawn.ptdSpec = NULL;
      eSpawn.ptdBump = NULL;
      eSpawn.iModelAnim = 0;
      eSpawn.fSize = m_fStretch;
      eSpawn.vStretch = FLOAT3D(1,1,1);
      eSpawn.penFallFXPapa=NULL;
      // initialize it
      penDebris->Initialize(eSpawn);

      // speed it up
      FLOAT3D vSpeed = FLOAT3D( FRnd()-0.5f, 0.25f+FRnd()*0.75f, FRnd()-0.5f)*60.0f;
      FLOAT3D vRot   = FLOAT3D( FRnd()-0.5f, FRnd()-0.5f, FRnd()-0.5f)*200.0f;
      ((CMovableEntity&)*penDebris).LaunchAsFreeProjectile( vSpeed, NULL);
      ((CMovableEntity&)*penDebris).SetDesiredRotation( vRot);

      GetModelObject()->RemoveAttachmentModel(iChar);
    }
    
    {for(INDEX iChar=0; iChar<CT_BIGHEADS; iChar+=3)
    {
      SpawnExplosion(iChar, -2.0f, 4.0f);
    }}

    SwitchToEditorModel();
    autowait(2.0f);
    // destroy yourself
    Destroy();
    return;
  }

 /************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    if( m_bActive)
    {
      InitAsModel();
    }
    else
    {
      InitAsEditorModel();
    }

    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set your appearance
    SetModel(MODEL_CRATE_BUS);
    SetModelMainTexture(TEXTURE_CRATE_BUS);

    m_bShowTrail=FALSE;
    AddRiders();
    GetModelObject()->StretchModel( FLOAT3D(m_fStretch,m_fStretch,m_fStretch));
    ModelChangeNotify();

    autowait(0.1f);

    CEntity *penParent=GetParent();
    if( penParent!=NULL)
    {
      //Particles_RocketTrail_Prepare(penParent);
      Particles_AfterBurner_Prepare(penParent);
    }

    wait()
    {
      // on the beginning
      on(EBegin): {
        resume;
      }
      // activate/deactivate shows/hides model
      on (EActivate): {
        SwitchToModel();
        m_bActive = TRUE;
        resume;
      }
      on (EDeactivate): {
        SwitchToEditorModel();
        m_bActive = FALSE;
        resume;
      }
      on (EEnvironmentStart): {
        m_bShowTrail=TRUE;
        resume;
      }
      // when dead
      on(EStop): {
        jump Die();
        resume;
      }
      otherwise(): {
        resume;
      }
    };
  };
};

