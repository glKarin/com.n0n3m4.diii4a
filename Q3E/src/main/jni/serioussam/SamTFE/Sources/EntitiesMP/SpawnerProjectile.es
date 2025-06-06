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

507
%{
#include "EntitiesMP/StdH/StdH.h"
  
#define ECF_SPAWNERPROJECTILE ( \
  ((ECBI_BRUSH)<<ECB_TEST) |\
  ((ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_PASS) |\
  ((ECBI_MODEL)<<ECB_IS))
#define EPF_SPAWNERPROJECTILE  ( \
  EPF_ONBLOCK_STOP|EPF_ORIENTEDBYGRAVITY|\
  EPF_TRANSLATEDBYGRAVITY|EPF_MOVABLE)

%}


uses "EntitiesMP/BasicEffects";
uses "EntitiesMP/Summoner";


// input parameter for twister
event ESpawnerProjectile {
  CEntityPointer penOwner,         // entity which owns it
  CEntityPointer penTemplate,      // template of the enemy to be spawned
};

%{
void CSpawnerProjectile_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_CANNON);
  pdec->PrecacheModel(MODEL_INVISIBLE);
};
%}

class CSpawnerProjectile : CMovableModelEntity {
  name      "SpawnerProjectile";
  thumbnail "";
  features "ImplementsOnPrecache";

properties:
  1 CEntityPointer m_penOwner,                  // entity which owns it
  2 CEntityPointer m_penTemplate,               // entity which owns it
  4 FLOAT m_fSize = 0.0f,       // for particle rendering
  5 FLOAT m_fTimeAdjust = 0.0f, // for particle rendering
  6 BOOL  m_bExploding = FALSE,
  7 FLOAT m_fExplosionDuration = 0.25f, // how long to explode
  8 FLOAT m_tmExplosionBegin = 0.0f, // explosion beginning time
  9 FLOAT m_tmSpawn = 0.0f,         // for particle rendering

components:
  
  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",
 10 model   MODEL_INVISIBLE     "ModelsMP\\Enemies\\Summoner\\SpawnerProjectile\\Invisible.mdl",
    
functions:
  
  void SpawnEntity()
  {
    CEntity *pen = NULL;
    // copy template entity
    pen = GetWorld()->CopyEntityInWorld( *m_penTemplate,
      CPlacement3D(FLOAT3D(-32000.0f+FRnd()*200.0f, -32000.0f+FRnd()*200.0f, 0), ANGLE3D(0, 0, 0)) );
    
    // change needed properties
    pen->End();
    
    CEnemyBase *peb = ((CEnemyBase*)pen);
    peb->m_bTemplate = FALSE;
    pen->Initialize();
    
    // adjust circle radii to account for enemy size
    /* unused
    FLOAT fEntityR = 0;
    if (pen->en_pciCollisionInfo!=NULL) {
      fEntityR = pen->en_pciCollisionInfo->GetMaxFloorRadius();
    } */
    
    // teleport back
    pen->Teleport(GetPlacement(), FALSE);
    
    // initialize tactics
    CEnemyBase &penMonster = (CEnemyBase &)*pen;

    if (penMonster.m_penTacticsHolder != NULL) {
      if (IsOfClass(penMonster.m_penTacticsHolder, "TacticsHolder")) {
        penMonster.StartTacticsNow();
      }
    }
 
  };
  
  void Explode(void)
  {
    // spawn explosion
    CPlacement3D plExplosion = GetPlacement();
    CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_BOMB;
    eSpawnEffect.vStretch = FLOAT3D(1.0f,1.0f,1.0f);
    penExplosion->Initialize(eSpawnEffect);

    // explosion debris
    eSpawnEffect.betType = BET_EXPLOSION_DEBRIS;
    CEntityPointer penExplosionDebris = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosionDebris->Initialize(eSpawnEffect);

    // explosion smoke
    eSpawnEffect.betType = BET_EXPLOSION_SMOKE;
    CEntityPointer penExplosionSmoke = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    penExplosionSmoke->Initialize(eSpawnEffect);
    
    /*
    // spawn smoke effect
    ESpawnEffect ese;
    ese.colMuliplier = C_WHITE|CT_OPAQUE;
    ese.betType = BET_CANNON;
    ese.vStretch = FLOAT3D(1.0f, 1.0f, 1.0f);
    CEntityPointer penEffect = CreateEntity(this->GetPlacement(), CLASS_BASIC_EFFECT);
    penEffect->Initialize(ese);
    */
  };
  
  void RenderParticles(void) {
    Particles_AfterBurner( this, m_tmSpawn, 1.0f, 1);
    if (m_bExploding)
    {
      //Particles_SummonerProjectileExplode( this, m_fSize, m_tmExplosionBegin, m_fExplosionDuration, m_fTimeAdjust );
    }
  }
  
 /************************************************************
  *                   P R O C E D U R E S                    *
  ************************************************************/
procedures:
  
  // --->>> MAIN
  Main(ESpawnerProjectile esp) {
    // remember the initial parameters
    ASSERT(esp.penOwner!=NULL);
    ASSERT(esp.penTemplate!=NULL);
    ASSERT(IsDerivedFromClass(esp.penTemplate, "Enemy Base"));
    m_penOwner = esp.penOwner;
    m_penTemplate = esp.penTemplate;

    m_fTimeAdjust = FRnd()*5.0f;
    EntityInfo *pei = (EntityInfo*) (m_penTemplate->GetEntityInfo());
    m_fSize = pei->vSourceCenter[1]*0.2f;

    m_tmSpawn=_pTimer->CurrentTick();

    // initialization
    InitAsModel();
    SetPhysicsFlags(EPF_SPAWNERPROJECTILE);
    SetCollisionFlags(ECF_SPAWNERPROJECTILE);
    SetFlags(GetFlags() | ENF_SEETHROUGH);
    SetModel(MODEL_INVISIBLE);
    
    Particles_AfterBurner_Prepare(this);

    // loop untill touched something
    wait() {
      on (EBegin) : { resume; }
      on (ETouch et) : { stop; }
      otherwise (): { resume; }
    }
    
    m_bExploding = TRUE;
    m_tmExplosionBegin = _pTimer->CurrentTick();
    
    // wait for explosion to end and spawn the monster in the middle
    SpawnEntity();

    Explode();

    SwitchToEditorModel();
    autowait(4.0f);
    Destroy();
    
    return;
  }
};
