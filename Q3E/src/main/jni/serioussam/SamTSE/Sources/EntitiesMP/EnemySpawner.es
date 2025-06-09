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
#include "EntitiesMP/StdH/StdH.h"
%}


uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/BasicEffects";

enum EnemySpawnerType {
  0 EST_SIMPLE          "Simple",           // spawns on trigger
  1 EST_RESPAWNER       "Respawner",        // respawn after death
  2 EST_DESTROYABLE     "Destroyable",      // spawns untill killed
  3 EST_TRIGGERED       "Triggered",        // spawn one group on each trigger
  4 EST_TELEPORTER      "Teleporter",       // teleport the target instead copying it - usable only once
  5 EST_RESPAWNERBYONE  "OBSOLETE - Don't use!",  // respawn only one (not entire group) after death
  6 EST_MAINTAINGROUP   "MaintainGroup",    // respawn by need to maintain the number of active enemies
  7 EST_RESPAWNGROUP    "RespawnerByGroup", // respawn the whole group when it's destroyed
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
  9 FLOAT m_tmDelay             "Delay initial" 'W' = 0.0f,      // how long to delay before spawning
 16 FLOAT m_tmSingleWait        "Delay single" 'O' = 0.1f,    // delay inside one group
  5 FLOAT m_tmGroupWait         "Delay group" 'G' = 0.1f,     // delay between two groups
 17 INDEX m_ctGroupSize         "Count group"  = 1,
  8 INDEX m_ctTotal             "Count total" 'C' = 1,        // max. number of spawned enemies
 13 CEntityPointer m_penPatrol  "Patrol target" 'P'  COLOR(C_lGREEN|0xFF),          // for spawning patrolling 
 15 enum EnemySpawnerType m_estType "Type"  'Y' = EST_SIMPLE,      // type of spawner
 18 BOOL m_bTelefrag "Telefrag" 'F' = FALSE,                  // telefrag when spawning
 19 BOOL m_bSpawnEffect "SpawnEffect" 'S' = TRUE, // show effect and play sound
 20 BOOL m_bDoubleInSerious "Double in serious mode" = FALSE,
 21 CEntityPointer m_penSeriousTarget  "Template for Serious"  COLOR(C_RED|0x20),
 22 BOOL m_bFirstPass = TRUE,
 
 50 CSoundObject m_soSpawn,    // sound channel
 51 INDEX m_iInGroup=0,        // in group counter for loops
 52 INDEX m_iEnemiesTriggered=0,  // number of times enemies triggered the spawner on death

 60 CEntityPointer m_penTacticsHolder  "Tactics Holder",
 61 BOOL m_bTacticsAutostart           "Tactics autostart" = TRUE,

 

components:

  1 model   MODEL_ENEMYSPAWNER     "Models\\Editor\\EnemySpawner.mdl",
  2 texture TEXTURE_ENEMYSPAWNER   "Models\\Editor\\EnemySpawner.tex",
  3 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",


functions:

  void Precache(void)
  {
    PrecacheClass(CLASS_BASIC_EFFECT, BET_TELEPORT);
  }


  const CTString &GetDescription(void) const
  {
    ((CTString&)m_strDescription).PrintF("-><none>");
    if (m_penTarget!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", (const char *) m_penTarget->GetName());
      if (m_penSeriousTarget!=NULL) {
        ((CTString&)m_strDescription).PrintF("->%s, %s", 
          (const char *) m_penTarget->GetName(), (const char *) m_penSeriousTarget->GetName());
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
    if( slPropertyOffset == _offsetof(CEnemySpawner, m_penTarget))
    {
      return CheckTemplateValid(penTarget);
    }
    else if( slPropertyOffset == _offsetof(CEnemySpawner, m_penPatrol))
    {
      return (penTarget!=NULL && IsDerivedFromClass(penTarget, "Enemy Marker"));
    }
    else if( slPropertyOffset == _offsetof(CEnemySpawner, m_penSeriousTarget))
    {
      return CheckTemplateValid(penTarget);
    }   
    else if( slPropertyOffset == _offsetof(CEnemySpawner, m_penTacticsHolder))
    {
      if (IsOfClass(penTarget, "TacticsHolder")) { return TRUE; }
      else { return FALSE; }
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
        if (m_estType==EST_RESPAWNER /*|| m_estType==EST_RESPAWNERBYONE*/
         || m_estType==EST_MAINTAINGROUP || m_estType==EST_RESPAWNGROUP) {
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

      // This line needs to be split up, since it triggers a bug in gcc 2.95.3.  --ryan.
      //CPlacement3D pl(FLOAT3D(CosFast(fA)*fR, 0.05f, SinFast(fA)*fR), ANGLE3D(0, 0, 0));

      FLOAT3D f3d(CosFast(fA)*fR, 0.05f, SinFast(fA)*fR);
      ANGLE3D a3d(0, 0, 0);
      CPlacement3D pl(f3d, a3d);

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

      // initialize tactics
      if (m_penTacticsHolder!=NULL) {
        if (IsOfClass(m_penTacticsHolder, "TacticsHolder")) {
          CEnemyBase *peb = ((CEnemyBase*)pen);
          peb->m_penTacticsHolder = m_penTacticsHolder;
          if (m_bTacticsAutostart) {
            // start tactics
            peb->StartTacticsNow();
          }
        }
      }
      
    }
  };

  // Handle an event, return false if the event is not handled
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    if (ee.ee_slEvent==EVENTCODE_ETrigger)
    {
      ETrigger eTrigger = ((ETrigger &) ee);
      if(IsDerivedFromClass(eTrigger.penCaused, "Enemy Base")
        && (m_estType==EST_MAINTAINGROUP || m_estType==EST_RESPAWNGROUP)) {
        m_iEnemiesTriggered++;
      }
    }
    return CRationalEntity::HandleEvent(ee);
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CEnemySpawner) - sizeof(CRationalEntity) + CRationalEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strDescription.Length();
    slUsedMemory += m_strName.Length();
    slUsedMemory += 1* sizeof(CSoundObject);
    return slUsedMemory;
  }

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
      // decrease the needed count
      if (m_iEnemiesTriggered>0 && m_estType==EST_RESPAWNGROUP) {
        if (!m_bFirstPass) {
          m_iEnemiesTriggered--;
        }
      } else if (m_iEnemiesTriggered>0) {
         m_iEnemiesTriggered--;
      }

      // if entire group spawned
      if (m_iInGroup>=m_ctGroupSize) {
        if (!(m_estType==EST_MAINTAINGROUP && m_iEnemiesTriggered>0)) {
          // finish
          return EReturn();
        }
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
        on (EBegin) : { 
          if (!m_bFirstPass && m_iEnemiesTriggered>0) {
            stop;
          }
          resume;
        }
        on (ETrigger) : { stop; };
        on (EStart) : { stop; };
        otherwise() : { pass; }
      }
     
      // if should delay - only once, on beginning
      if (m_tmDelay>0 && m_bFirstPass) {
        // initial delay
        autowait(m_tmDelay);
      }

      if (m_estType==EST_RESPAWNGROUP) {
        if (m_bFirstPass) {
          autocall SpawnGroup() EReturn;
        } else if (m_iEnemiesTriggered>=m_ctGroupSize) {
          if (m_tmGroupWait>0) { autowait(m_tmGroupWait); }
          autocall SpawnGroup() EReturn;
        }
      } else if (TRUE) {
        // spawn one group
        if (m_tmGroupWait>0 && !m_bFirstPass) { autowait(m_tmGroupWait); }
        autocall SpawnGroup() EReturn;
      }

      // if should continue respawning by one
      /*if (m_estType==EST_RESPAWNERBYONE) {
        // set group size to 1
        if (m_tmGroupWait>0 && !m_bFirstPass) { autowait(m_tmGroupWait); }
        m_ctGroupSize = 1;
      }*/

      // if should continue maintaining group
      if (m_estType==EST_MAINTAINGROUP) {
        // set group size to 1
        m_ctGroupSize = 1;
      }

      // never do an initial delay again - set FirstPass to FALSE
      m_bFirstPass = FALSE;

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

    if (m_tmSingleWait<=0.0f) { m_tmSingleWait=0.05f; }
    if (m_tmGroupWait<=0.0f) { m_tmGroupWait=0.05f; }
    
    // set range
    if (m_fInnerCircle > m_fOuterCircle) {
      m_fInnerCircle = m_fOuterCircle;
    }

    if (m_estType==EST_RESPAWNERBYONE) {
      m_estType=EST_MAINTAINGROUP;
    }

    // check target
    if (m_penTarget!=NULL) {
      if (!IsDerivedFromClass(m_penTarget, "Enemy Base")) {
        WarningMessage("Target '%s' is of wrong class!", (const char *) m_penTarget->GetName());
        m_penTarget = NULL;
      }
    }
    if (m_penSeriousTarget!=NULL) {
      if (!IsDerivedFromClass(m_penSeriousTarget, "Enemy Base")) {
        WarningMessage("Target '%s' is of wrong class!", (const char *) m_penSeriousTarget->GetName());
        m_penSeriousTarget = NULL;
      }
    }

    // never start ai in wed
    autowait(_pTimer->TickQuantum);

    // destroy self if this is a multiplayer-only spawner, and flags indicate no extra enemies
    if ( !GetSP()->sp_bUseExtraEnemies && !GetSP()->sp_bSinglePlayer 
      && !(GetSpawnFlags()&SPF_SINGLEPLAYER)) {
      Destroy();
      return;
    }

    if (m_bDoubleInSerious && GetSP()->sp_gdGameDifficulty==CSessionProperties::GD_EXTREME) {
      m_ctGroupSize*=2;
      m_ctTotal*=2;
    }
    if (m_penSeriousTarget!=NULL && GetSP()->sp_gdGameDifficulty==CSessionProperties::GD_EXTREME) {
      m_penTarget = m_penSeriousTarget;
    }

    if (m_estType==EST_MAINTAINGROUP) {
      m_iEnemiesTriggered = m_ctGroupSize;
    }

    m_bFirstPass = TRUE;

    wait() {
      on(EBegin) : {
        if(m_estType==EST_SIMPLE) {
          call Simple();
        } else if(m_estType==EST_TELEPORTER) {
          call Teleporter();
        } else if(m_estType==EST_RESPAWNER /*|| m_estType==EST_RESPAWNERBYONE*/
               || m_estType==EST_TRIGGERED || m_estType==EST_RESPAWNGROUP) {
          call Respawner();
        } else if(m_estType==EST_MAINTAINGROUP) {
          m_ctGroupSize = 1;
          call Respawner();
        }
        else if(m_estType==EST_DESTROYABLE) {
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
