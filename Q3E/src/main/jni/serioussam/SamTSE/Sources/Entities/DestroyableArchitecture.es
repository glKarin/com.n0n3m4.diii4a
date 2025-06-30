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

107
%{
#include "Entities/StdH/StdH.h"
#include "Entities/Effector.h"
#include "Entities/MovingBrush.h"
%}
uses "Entities/Devil";
uses "Entities/Debris";
uses "Entities/GradientMarker";
uses "Entities/Effector";

%{
struct DebrisInfo {
  ULONG ulModelID;
  ULONG ulTextureID;
  FLOAT vOffset[3];
};

static struct DebrisInfo _ObeliskDebrisInfo[] =
{
  { MODEL_OBELISK01, TEXTURE_OBELISK, 0.0f, 114.4989f, 0.0f},
  { MODEL_OBELISK02, TEXTURE_OBELISK, 0.035f, 106.8628f, 0.0f},
	{ MODEL_OBELISK03, TEXTURE_OBELISK, 0.0f, 98.628f, 0.0f},
	{ MODEL_OBELISK04, TEXTURE_OBELISK, 0.0f, 90.4996f, 0.0f},
	{ MODEL_OBELISK05, TEXTURE_OBELISK, 0.0f, 82.174f, 0.0f},
	{ MODEL_OBELISK06, TEXTURE_OBELISK, 0.0f, 71.0425f, 0.0f},
	{ MODEL_OBELISK07, TEXTURE_OBELISK, 0.0f, 59.2f, 0.0f},
	{ MODEL_OBELISK08, TEXTURE_OBELISK, 0.0f, 46.65f, 0.0f},
	{ MODEL_OBELISK09, TEXTURE_OBELISK, 0.0f, 36.6f, 0.0f},
};

static struct DebrisInfo _PylonDebrisInfo[] =
{
  { MODEL_PYLON01, TEXTURE_PYLON, -17.3379f, 55.92f, 0},
	{ MODEL_PYLON02, TEXTURE_PYLON, -10.525f, 58.045f, 0},
	{ MODEL_PYLON03, TEXTURE_PYLON, -17.66f, 42.32f, 0},
	{ MODEL_PYLON04, TEXTURE_PYLON, -0.815000f, 54.69f, 0	},
	{ MODEL_PYLON05, TEXTURE_PYLON, 14.795f, 51.65f, 0},
	{ MODEL_PYLON06, TEXTURE_PYLON, 0.02f, 36.18f, 0},
	{ MODEL_PYLON07, TEXTURE_PYLON, -10.289f, 33.982f, 0},
	{ MODEL_PYLON08, TEXTURE_PYLON, -22.9152f, 28.6205f, 0},
	{ MODEL_PYLON09, TEXTURE_PYLON, 21.932f, 47.2453f, 0},
};
%}

class CDestroyableArchitecture: CMovableBrushEntity {
name      "DestroyableArchitecture";
thumbnail "Thumbnails\\DestroyableArchitecture.tbn";
features  "HasName", "IsTargetable";
properties:
  1 CTString m_strName                  "Name" 'N' = "DestroyableArchitecture",              // name
  2 FLOAT m_fHealth                     "Health" 'H' = -1.0f,                                // health
  3 enum EffectorEffectType m_etType    "Type" 'Y' = ET_DESTROY_OBELISK,                     // name
  4 FLOAT3D m_vDamageDir = FLOAT3D(0,0,0),                                                   // direction of damage
  5 FLOAT m_fStretch                    "Stretch" 'S' = 1.0f,                                // debris stretch
  6 CEntityPointer m_penGradient        "Gradient" 'R',

 10 COLOR m_colDebrises         "Color of debrises" = C_WHITE,
 11 INDEX m_ctDebrises          "Debris count" = 12,
 12 FLOAT m_fCandyEffect        "Debris blow power" = 0.0f,
 13 FLOAT m_fCubeFactor         "Cube factor" = 1.0f,
 14 BOOL m_bBlowupByDamager     "Blowup by Damager" = FALSE,   // if only damager can destroy brush

components:
// ************** DEBRIS PARTS **************
 10 texture   TEXTURE_OBELISK        "Models\\CutSequences\\Obelisk\\Obelisk.tex",
 11 model     MODEL_OBELISK01        "Models\\CutSequences\\Obelisk\\Part01.mdl",
 12 model     MODEL_OBELISK02        "Models\\CutSequences\\Obelisk\\Part02.mdl",
 13 model     MODEL_OBELISK03        "Models\\CutSequences\\Obelisk\\Part03.mdl",
 14 model     MODEL_OBELISK04        "Models\\CutSequences\\Obelisk\\Part04.mdl",
 15 model     MODEL_OBELISK05        "Models\\CutSequences\\Obelisk\\Part05.mdl",
 16 model     MODEL_OBELISK06        "Models\\CutSequences\\Obelisk\\Part06.mdl",
 17 model     MODEL_OBELISK07        "Models\\CutSequences\\Obelisk\\Part07.mdl",
 18 model     MODEL_OBELISK08        "Models\\CutSequences\\Obelisk\\Part08.mdl",
 19 model     MODEL_OBELISK09        "Models\\CutSequences\\Obelisk\\Part09.mdl",
 
 20 texture   TEXTURE_PYLON          "Models\\CutSequences\\Pylon\\Pylon.tex",
 21 model     MODEL_PYLON01          "Models\\CutSequences\\Pylon\\Part01.mdl",
 22 model     MODEL_PYLON02          "Models\\CutSequences\\Pylon\\Part02.mdl",
 23 model     MODEL_PYLON03          "Models\\CutSequences\\Pylon\\Part03.mdl",
 24 model     MODEL_PYLON04          "Models\\CutSequences\\Pylon\\Part04.mdl",
 25 model     MODEL_PYLON05          "Models\\CutSequences\\Pylon\\Part05.mdl",
 26 model     MODEL_PYLON06          "Models\\CutSequences\\Pylon\\Part06.mdl",
 27 model     MODEL_PYLON07          "Models\\CutSequences\\Pylon\\Part07.mdl",
 28 model     MODEL_PYLON08          "Models\\CutSequences\\Pylon\\Part08.mdl",
 29 model     MODEL_PYLON09          "Models\\CutSequences\\Pylon\\Part09.mdl",
 
// ************** NEEDED CLASSES **************
 30 class     CLASS_DEBRIS       "Classes\\Debris.ecl",
 31 class     CLASS_EFFECTOR     "Classes\\Effector.ecl",

// ************** STONE PARTS **************
 32 model     MODEL_STONE        "Models\\Effects\\Debris\\Stone\\Stone.mdl",
 33 texture   TEXTURE_STONE      "Models\\Effects\\Debris\\Stone\\Stone.tex",

functions:
  
  void Precache(void)
  {
    PrecacheClass   (CLASS_DEBRIS);
    PrecacheModel   (MODEL_STONE);
    PrecacheTexture (TEXTURE_STONE);

    // precache acording to destroying architecture
    switch( m_etType)
    {
    case ET_DESTROY_OBELISK:
      PrecacheClass   (CLASS_EFFECTOR,ET_DESTROY_OBELISK);
      PrecacheTexture (TEXTURE_OBELISK);
      PrecacheModel   (MODEL_OBELISK01);
      PrecacheModel   (MODEL_OBELISK02);
      PrecacheModel   (MODEL_OBELISK03);
      PrecacheModel   (MODEL_OBELISK04);
      PrecacheModel   (MODEL_OBELISK05);
      PrecacheModel   (MODEL_OBELISK06);
      PrecacheModel   (MODEL_OBELISK07);
      PrecacheModel   (MODEL_OBELISK08);
      PrecacheModel   (MODEL_OBELISK09);
      break;
    case ET_DESTROY_PYLON:
      PrecacheClass   (CLASS_EFFECTOR,ET_DESTROY_PYLON);
      PrecacheTexture (TEXTURE_PYLON);
      PrecacheModel   (MODEL_PYLON01);
      PrecacheModel   (MODEL_PYLON02);
      PrecacheModel   (MODEL_PYLON03);
      PrecacheModel   (MODEL_PYLON04);
      PrecacheModel   (MODEL_PYLON05);
      PrecacheModel   (MODEL_PYLON06);
      PrecacheModel   (MODEL_PYLON07);
      PrecacheModel   (MODEL_PYLON08);
      PrecacheModel   (MODEL_PYLON09);
      break;
    }
  }

  // Validate offered target for one property
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if(penTarget==NULL)
    {
      return FALSE;
    }
    
    // if gradient marker
    if( slPropertyOffset==offsetof(CDestroyableArchitecture, m_penGradient) )
    {
      return (IsDerivedFromClass(penTarget, "Gradient Marker"));
    }
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }

  /* Get gradient type name, return empty string if not used. */
  const CTString &GetGradientName(INDEX iGradient)
  {
    static const CTString strDummyName("");
    static const CTString strMarkerUnused("Marker not set");
    if (iGradient==1)
    {
      CGradientMarker *pgm = (CGradientMarker *) m_penGradient.ep_pen;
      if (pgm != NULL) {
        return pgm->GetGradientName();
      } else {
        return strMarkerUnused;
      }
    }
    return strDummyName;
  }
  /* Uncache shadows for given gradient */
  void UncacheShadowsForGradient(class CGradientMarker *penDiscard)
  {
    CGradientMarker *pgm = (CGradientMarker *) m_penGradient.ep_pen;
    if(pgm == penDiscard)
    {
      CEntity::UncacheShadowsForGradient(1);
    }
  }

  /* Get gradient, return FALSE for none. */
  BOOL GetGradient(INDEX iGradient, class CGradientParameters &fpGradient)
  {
    if ( iGradient==1)
    {
      CGradientMarker *pgm = (CGradientMarker *) m_penGradient.ep_pen;
      if (pgm != NULL) {
        return pgm->GetGradient(0, fpGradient);
      }
    }
    return FALSE;
  }
  
  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // if not destroyable
    if(m_fHealth<0) {
      // ignore damages
      return;
    }
    
    if(m_bBlowupByDamager)
    {
      if( dmtType == DMT_DAMAGER)
      {
        CMovableBrushEntity::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
      }
    }
    else
    {
      // react only on explosions
      if( (dmtType == DMT_EXPLOSION) ||
          (dmtType == DMT_PROJECTILE) ||
          (dmtType == DMT_CANNONBALL) )
      {
        CMovableBrushEntity::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
      }
    }
  }

  void DestroyObelisk()
  {
    for( INDEX iDebris=0; iDebris < static_cast<INDEX>(ARRAYCOUNT(_ObeliskDebrisInfo)); iDebris++)
    {
      DebrisInfo &di = _ObeliskDebrisInfo[iDebris];
      FLOAT3D vOffset = FLOAT3D( di.vOffset[0], di.vOffset[1], di.vOffset[2])*m_fStretch;
      FLOAT3D vPos = GetPlacement().pl_PositionVector+vOffset;
      CEntityPointer penDebris = GetWorld()->CreateEntity_t(
        CPlacement3D(vPos, ANGLE3D(0,0,0)), CTFILENAME("Classes\\Debris.ecl"));
      // prepare parameters
      ESpawnDebris eSpawn;
      eSpawn.colDebris = C_WHITE|CT_OPAQUE;
      eSpawn.Eeibt = EIBT_ROCK;
      eSpawn.dptParticles = DPT_NONE;
      eSpawn.betStain = BET_NONE;
      eSpawn.pmd = GetModelDataForComponent(di.ulModelID);
      eSpawn.ptd = GetTextureDataForComponent(di.ulTextureID);
      eSpawn.ptdRefl = NULL;
      eSpawn.ptdSpec = NULL;
      eSpawn.ptdBump = NULL;
      eSpawn.iModelAnim = 0;
      eSpawn.fSize = m_fStretch;
      // initialize it
      penDebris->Initialize(eSpawn);

      // speed it up
      FLOAT fHeightRatio = di.vOffset[1]*m_fStretch/120.0f;
      FLOAT3D vSpeed = FLOAT3D( FRnd()-0.5f, 0.0f, FRnd()-0.5f)*(1.0f-fHeightRatio)*160.0f;
      FLOAT3D vRot   = FLOAT3D( FRnd()-0.5f, (FRnd()-0.5f)*(1.0f-fHeightRatio), FRnd()-0.5f)*200.0f;
      /*
      vSpeed = FLOAT3D( 0,0,0);
      vRot   = FLOAT3D( 0,0,0);*/
      ((CMovableEntity&)*penDebris).LaunchAsFreeProjectile( vSpeed, NULL);
      ((CMovableEntity&)*penDebris).SetDesiredRotation( vRot);
    }

    // notify children
    FOREACHINLIST( CEntity, en_lnInParent, en_lhChildren, iten) {
      iten->SendEvent( EBrushDestroyed());
    }
    m_fHealth = -1;
    ForceFullStop();
    SetDefaultProperties();
    
    CPlacement3D plObelisk = GetPlacement();

    // notify engine to kickstart entities that are cached in stationary position,
    // before we turn off, so they can fall
    NotifyCollisionChanged();
    SetFlags( GetFlags()|ENF_HIDDEN);
    SetCollisionFlags(ECF_IMMATERIAL);
    
    // spawn spray spray
    CEntity *penEffector = CreateEntity( plObelisk, CLASS_EFFECTOR);
    // set spawn parameters
    ESpawnEffector eSpawnEffector;
    eSpawnEffector.tmLifeTime = 6.0f;
    eSpawnEffector.eetType = ET_DESTROY_OBELISK;
    // initialize spray
    penEffector->Initialize( eSpawnEffector);
  }

  void DestroyPylon()
  {
    for( INDEX iDebris=0; iDebris < static_cast<INDEX>((ARRAYCOUNT(_PylonDebrisInfo))); iDebris++)
    {
      DebrisInfo &di = _PylonDebrisInfo[iDebris];
      FLOAT3D vOffset = FLOAT3D( di.vOffset[0], di.vOffset[1], di.vOffset[2])*m_fStretch;
      FLOAT3D vPos = GetPlacement().pl_PositionVector+vOffset;
      CEntityPointer penDebris = GetWorld()->CreateEntity_t(
        CPlacement3D(vPos, ANGLE3D(0,0,0)), CTFILENAME("Classes\\Debris.ecl"));
      // prepare parameters
      ESpawnDebris eSpawn;
      eSpawn.colDebris = C_WHITE|CT_OPAQUE;
      eSpawn.Eeibt = EIBT_ROCK;
      eSpawn.dptParticles = DPT_NONE;
      eSpawn.betStain = BET_NONE;
      eSpawn.pmd = GetModelDataForComponent(di.ulModelID);
      eSpawn.ptd = GetTextureDataForComponent(di.ulTextureID);
      eSpawn.ptdRefl = NULL;
      eSpawn.ptdSpec = NULL;
      eSpawn.ptdBump = NULL;
      eSpawn.iModelAnim = 0;
      eSpawn.fSize = m_fStretch;
      // initialize it
      penDebris->Initialize(eSpawn);

      // speed it up
      FLOAT fHeightRatio = di.vOffset[1]*m_fStretch/120.0f;
      FLOAT3D vSpeed = (m_vDamageDir*2.0f+FLOAT3D( FRnd()-0.5f, 0.0f, FRnd()))*fHeightRatio*160.0f;
      FLOAT3D vRot   = FLOAT3D( FRnd()-0.5f, (FRnd()-0.5f)*fHeightRatio, FRnd()-0.5f)*300.0f;
      ((CMovableEntity&)*penDebris).LaunchAsFreeProjectile( vSpeed, NULL);
      ((CMovableEntity&)*penDebris).SetDesiredRotation( vRot);
    }

    // notify children
    FOREACHINLIST( CEntity, en_lnInParent, en_lhChildren, iten) {
      iten->SendEvent( EBrushDestroyed());
    }
    m_fHealth = -1;
    CPlacement3D plObelisk = GetPlacement();
    // spawn spray spray
    CEntity *penEffector = CreateEntity( plObelisk, CLASS_EFFECTOR);
    // set spawn parameters
    ESpawnEffector eSpawnEffector;
    eSpawnEffector.eetType = ET_DESTROY_PYLON;
    eSpawnEffector.tmLifeTime = 6.0f;
    eSpawnEffector.vDamageDir = m_vDamageDir;
    // initialize spray
    penEffector->Initialize( eSpawnEffector);

    ForceFullStop();
    SetDefaultProperties();
    // notify engine to kickstart entities that are cached in stationary position,
    // before we turn off, so they can fall
    NotifyCollisionChanged();
    SetFlags( GetFlags()|ENF_HIDDEN);
    SetCollisionFlags(ECF_IMMATERIAL);    
  }
procedures:

  Main() {
    // declare yourself as a brush
    InitAsBrush();
    SetPhysicsFlags(EPF_BRUSH_MOVING);
    SetCollisionFlags(ECF_BRUSH);
    // non-zoning brush
    SetFlags(GetFlags()&~ENF_ZONING);
    SetHealth(m_fHealth);

    // start moving
    wait() {
      on (EBegin) : {
        resume;
      }
      on (EBrushDestroyedByDevil ebdbd) :
      {
        m_vDamageDir = ebdbd.vDamageDir;
        switch( m_etType)
        {
        case ET_DESTROY_OBELISK:
          DestroyObelisk();
          break;
        case ET_DESTROY_PYLON:
          DestroyPylon();
          break;
        }
        stop;
      }
      on (EDeath eDeath) : {
        // get your size
        FLOATaabbox3D box;
        GetSize(box);
        if( m_ctDebrises<=0)
        {
          m_ctDebrises=1;
        }
        FLOAT fEntitySize = pow(box.Size()(1)*box.Size()(2)*box.Size()(3)/m_ctDebrises, 1.0f/3.0f)*m_fCubeFactor;
        
        Debris_Begin(EIBT_ROCK, DPT_NONE, BET_NONE, fEntitySize, FLOAT3D(1.0f,2.0f,3.0f),
          FLOAT3D(0,0,0), 1.0f+m_fCandyEffect/2.0f, m_fCandyEffect, m_colDebrises);
        for(INDEX iDebris = 0; iDebris<m_ctDebrises; iDebris++) {
          Debris_Spawn(this, this, MODEL_STONE, TEXTURE_STONE, 0, 0, 0, IRnd()%4, 1.0f,
            FLOAT3D(FRnd()*0.8f+0.1f, FRnd()*0.8f+0.1f, FRnd()*0.8f+0.1f));
        }

        // notify children
        FOREACHINLIST( CEntity, en_lnInParent, en_lhChildren, iten) {
          iten->SendEvent( EBrushDestroyed());
        }

        m_fHealth = -1;
        ForceFullStop();
        SetDefaultProperties();
        // notify engine to kickstart entities that are cached in stationary position,
        // before we turn off, so they can fall
        NotifyCollisionChanged();
        SetFlags( GetFlags()|ENF_HIDDEN);
        SetCollisionFlags(ECF_IMMATERIAL);    
        
        // for each child of this entity
        {FOREACHINLIST(CEntity, en_lnInParent, en_lhChildren, itenChild) {
          // send it destruction event
          itenChild->SendEvent(ERangeModelDestruction());
        }}

        stop;
      }
      on (EReturn) :
      {
        stop;
      }
    }
    return;
  }
};
