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

304
%{
#include "Entities/StdH/StdH.h"
%}


uses "Entities/EnemyBase";
uses "Entities/BasicEffects";

enum EnemySpawnerType {
  0 EST_SIMPLE      "Simple",       // spawns on trigger
  1 EST_RESPAWNER   "Respawner",    // respawn after death
  2 EST_DESTROYABLE "Destroyable",  // spawns untill killed
  3 EST_TRIGGERED   "Triggered",    // spawn one group on each trigger
  4 EST_TELEPORTER  "Teleporter",   // teleport the target instead copying it - usable only once
  5 EST_RESPAWNERBYONE   "RespawnerbyOne",    // respawn only one (not entire group) after death
};


class CEnemySpawner: CRationalEntity {
name      "Enemy Spawner";
thumbnail "Thumbnails\\EnemySpawner.tbn";
features  "HasName", "HasTarget", "IsTargetable";

properties:
  1 CEntityPointer m_penTarget  "Template Target" 'T'  COLOR(C_BLUE|0x20),        // template entity to duplicate
  2 CTString m_strDescription = "",
  3 CTString m_strName          "Name" 'N' = "Enemy spawner",
  
  6 RANGE m_fInnerCircle        "Circle inner" 'V' = 0.0f,    // inner circle for creation
  7 RANGE m_fOuterCircle        "Circle outer" 'B' = 0.0f,    // outer circle for creation
  9 FLOAT m_tmDelay             "Wait delay" 'W' = 0.0f,      // how long to delay before spawning
 16 FLOAT m_tmSingleWait        "Single delay" 'O' = 0.1f,    // delay inside one group
  5 FLOAT m_tmGroupWait         "Group delay" 'G' = 0.1f,     // delay between two groups
 17 INDEX m_ctGroupSize         "Group size"  = 1,
  8 INDEX m_ctTotal             "Total count" 'C' = 1,        // max. number of spawned enemies
 13 CEntityPointer m_penPatrol  "Patrol target" 'P'  COLOR(C_lGREEN|0xFF),          // for spawning patrolling 
 15 enum EnemySpawnerType m_estType "Type"  'Y' = EST_SIMPLE,      // type of spawner
 18 BOOL m_bTelefrag "Telefrag" 'F' = FALSE,                  // telefrag when spawning
 19 BOOL m_bSpawnEffect "SpawnEffect" 'S' = TRUE, // show effect and play sound
 20 BOOL m_bDoubleInSerious "Double in serious mode" = FALSE,
 21 CEntityPointer m_penSeriousTarget  "Template for Serious"  COLOR(C_RED|0x20),
 
 50 CSoundObject m_soSpawn,    // sound channel
 51 INDEX m_iInGroup=0,          // in group counter for loops

components:
  1 model   MODEL_ENEMYSPAWNER     "Models\\Editor\\EnemySpawner.mdl",
  2 texture TEXTURE_ENEMYSPAWNER   "Models\\Editor\\EnemySpawner.tex",
  3 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",

functions:
  void Precache(void) {
    PrecacheClass(CLASS_BASIC_EFFECT, BET_TELEPORT);
  }

  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("-><none>");
    if (m_penTarget!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s",(const char*) m_penTarget->GetName());
      if (m_penSeriousTarget!=NULL) {
        ((CTString&)m_strDescription).PrintF("->%s, %s", 
          (const char*) m_penTarget->GetName(),(const char*) m_penSeriousTarget->GetName());
      }
    }
    ((CTString&)m_strDescription) = EnemySpawnerType_enum.NameForValue(INDEX(m_estType))
      + m_strDescription;
    return m_strDescription;
  }

  // check if one template is valid for this spawner
  BOOL CheckTemplateValid(CEntity *pen)
  {
    if (pen==NULL || !IsDerivedFromClass(pen, "Enemy Base")) {
      return FALSE;
    }
    if (m_estType==EST_TELEPORTER) {
      return !(((CEnemyBase&)*pen).m_bTemplate);
    } else {
      return ((CEnemyBase&)*pen).m_bTemplate;
    }
  }

  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if( slPropertyOffset == offsetof(CEnemySpawner, m_penTarget))
    {
      return CheckTemplateValid(penTarget);
    }
    else if( slPropertyOffset == offsetof(CEnemySpawner, m_penPatrol))
    {
      return (penTarget!=NULL && IsDerivedFromClass(penTarget, "Enemy Marker"));
    }
    else if( slPropertyOffset == offsetof(CEnemySpawner, m_penSeriousTarget))
    {
      return CheckTemplateValid(penTarget);
    }   
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }

  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    if (m_penTarget==NULL) { return FALSE; }
    m_penTarget->FillEntityStatistics(pes);
    pes->es_ctCount = m_ctTotal;
    pes->es_strName += " (spawned)";
    if (m_penSeriousTarget!=NULL) {
      pes->es_strName += " (has serious)";
    }
    return TRUE;
  }

  // spawn new entity
  void SpawnEntity(BOOL bCopy) {
    // spawn new entity if of class basic enemy
    if (CheckTemplateValid(m_penTarget)) {

      CEntity *pen = NULL;
      if (bCopy) {
        // copy template entity
        pen = GetWorld()->CopyEntityInWorld( *m_penTarget,
          CPlacement3D(FLOAT3D(-32000.0f+FRnd()*200.0f, -32000.0f+FRnd()*200.0f, 0), ANGLE3D(0, 0, 0)) );

        // change needed properties
        pen->End();
        CEnemyBase *peb = ((CEnemyBase*)pen);
        peb->m_bTemplate = FALSE;
        if (m_estType==EST_RESPAWNER || m_estType==EST_RESPAWNERBYONE) {
          peb->m_penSpawnerTarget = this;
        }
        if (m_penPatrol!=NULL) {
          peb->m_penMarker = m_penPatrol;
        }
        pen->Initialize();
      } else {
        pen = m_penTarget;
        m_penTarget = NULL;
      }

      // adjust circle radii to account for enemy size
      FLOAT fEntityR = 0;
      if (pen->en_pciCollisionInfo!=NULL) {
        fEntityR = pen->en_pciCollisionInfo->GetMaxFloorRadius();
      }
      FLOAT fOuterCircle = ClampDn(m_fOuterCircle-fEntityR, 0.0f);
      FLOAT fInnerCircle = ClampUp(m_fInnerCircle+fEntityR, fOuterCircle);
      // calculate new position
      FLOAT fR = fInnerCircle + FRnd()*(fOuterCircle-fInnerCircle);
      FLOAT fA = FRnd()*360.0f;
      CPlacement3D pl(FLOAT3D(CosFast(fA)*fR, 0.05f, SinFast(fA)*fR), ANGLE3D(0, 0, 0));
      pl.RelativeToAbsolute(GetPlacement());

      // teleport back
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
        CEntityPointer penEffect = CreateEntity(pl, CLASS_BASIC_EFFECT);
        penEffect->Initialize(ese);
      }
    }
  };


procedures:
  // spawn one group of entities
  SpawnGroup() 
  {
    // no enemies in group yet
    m_iInGroup = 0;
    // repeat forever
    while(TRUE) {

      // spawn one enemy
      SpawnEntity(TRUE);

      // count total enemies spawned
      m_ctTotal--;
      // if no more left
      if (m_ctTotal<=0) {
        // finish entire spawner
        return EEnd();
      }

      // count enemies in group
      m_iInGroup++;
      // if entire group spawned
      if (m_iInGroup>=m_ctGroupSize) {
        // finish
        return EReturn();
      }

      // wait between two entities in group
      wait(m_tmSingleWait) {
        on (EBegin) : { resume; }
        on (ETimer) : { stop; }
        otherwise() : { pass; }
      }
    }
  }

  // simple spawner
  Simple()
  {
    // wait to be triggered
    wait() {
      on (EBegin) : { resume; }
      on (ETrigger) : { stop; };
      on (EStart) : { stop; };
      otherwise() : { pass; }
    }

    // if should delay
    if (m_tmDelay>0) {
      // wait delay
      autowait(m_tmDelay);
    }

    // repeat
    while(TRUE) {
      // spawn one group
      autocall SpawnGroup() EReturn;
      // delay between groups
      autowait(m_tmGroupWait);
    }
  }

  // teleports the template
  Teleporter()
  {
    // wait to be triggered
    wait() {
      on (EBegin) : { resume; }
      on (ETrigger) : { stop; };
      on (EStart) : { stop; };
      otherwise() : { pass; }
    }

    // if should delay
    if (m_tmDelay>0) {
      // wait delay
      autowait(m_tmDelay);
    }

    // teleport it
    SpawnEntity(FALSE);

    // end the spawner
    return EEnd();
  }

  // respawn enemies when killed
  Respawner()
  {
    // repeat
    while(TRUE) {
      // wait to be triggered
      wait() {
        on (EBegin) : { resume; }
        on (ETrigger) : { stop; };
        on (EStart) : { stop; };
        otherwise() : { pass; }
      }
      // if should delay
      if (m_tmDelay>0) {
        // wait delay
        autowait(m_tmDelay);
      }

      // spawn one group
      autocall SpawnGroup() EReturn;
      // if should continue respawning by one
      if (m_estType==EST_RESPAWNERBYONE) {
        // set group size to 1
        m_ctGroupSize = 1;
      }
      // wait a bit to recover
      autowait(0.1f);
    }
  }

  DestroyableInactive()
  {
    waitevent() EActivate;
    jump DestroyableActive();
  }

  DestroyableActiveSpawning()
  {
    // repeat
    while(TRUE) {
      // spawn one group
      autocall SpawnGroup() EReturn;
      // delay between groups
      autowait(m_tmGroupWait);
    }
  }
  DestroyableActive()
  {
    autocall DestroyableActiveSpawning() EDeactivate;
    jump DestroyableInactive();
  }

  // spawn new entities until you are stopped
  Destroyable()
  {
    // start in inactive state and do until stopped
    autocall DestroyableInactive() EStop;
    // finish
    return EEnd();
  }

  Main() {
    // init as nothing
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_ENEMYSPAWNER);
    SetModelMainTexture(TEXTURE_ENEMYSPAWNER);

    // set range
    if (m_fInnerCircle > m_fOuterCircle) {
      m_fInnerCircle = m_fOuterCircle;
    }

    // check target
    if (m_penTarget!=NULL) {
      if (!IsDerivedFromClass(m_penTarget, "Enemy Base")) {
        WarningMessage("Target '%s' is of wrong class!", (const char*)m_penTarget->GetName());
        m_penTarget = NULL;
      }
    }
    if (m_penSeriousTarget!=NULL) {
      if (!IsDerivedFromClass(m_penSeriousTarget, "Enemy Base")) {
        WarningMessage("Target '%s' is of wrong class!", (const char*)m_penSeriousTarget->GetName());
        m_penSeriousTarget = NULL;
      }
    }

    // never start ai in wed
    autowait(_pTimer->TickQuantum);

    if (m_bDoubleInSerious && GetSP()->sp_gdGameDifficulty==CSessionProperties::GD_EXTREME) {
      m_ctGroupSize*=2;
      m_ctTotal*=2;
    }
    if (m_penSeriousTarget!=NULL && GetSP()->sp_gdGameDifficulty==CSessionProperties::GD_EXTREME) {
      m_penTarget = m_penSeriousTarget;
    }

    wait() {
      on(EBegin) : {
        if(m_estType==EST_SIMPLE) {
          call Simple();
        } else if(m_estType==EST_TELEPORTER) {
          call Teleporter();
        } else if(m_estType==EST_RESPAWNER || m_estType==EST_RESPAWNERBYONE || m_estType==EST_TRIGGERED) {
          call Respawner();
        } else if(m_estType==EST_DESTROYABLE) {
          call Destroyable();
        }
      }
      on(EDeactivate) : {
        stop;
      }
      on(EStop) : {
        stop;
      }
      on(EEnd) : {
        stop;
      }
    }

    Destroy();

    return;
  };
};
