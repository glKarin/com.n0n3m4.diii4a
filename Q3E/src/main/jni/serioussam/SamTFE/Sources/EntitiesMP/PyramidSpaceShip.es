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

609
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/CutSequences/SpaceShip/SpaceShip.h"
#include "Models/CutSequences/SpaceShip/Door.h"
#include "Models/CutSequences/SpaceShip/LightBeam.h"
#include "EntitiesMP/Effector.h"
#include "EntitiesMP/Light.h"
%}

uses "EntitiesMP/PyramidSpaceShipMarker";

enum PSSState {
  0 PSSS_IDLE                  "Idle",                      // idle
  1 PSSS_MOVING                "Moving",                    // process of moving trough markers
  2 PSSS_REACHED_DESTINATION   "Reached destination",       // process of turning on
  3 PSSS_KILLING_BEAM_FIREING  "Killing beam fireing",      // killing beam fireing
  4 PSSS_BEAM_DEACTIVATED      "Killing beam deactivated",  // killing beam gas been deactivated, wait to pick up Sam
  5 PSSS_DOORS_CLOSED          "Doors closed",              // doors closed
};

event EForcePathMarker {
  CEntityPointer penForcedPathMarker,
};

%{
#define STRETCH_X (200.0f*m_fStretch)
#define STRETCH_Y (100.0f*m_fStretch)
#define STRETCH_Z (200.0f*m_fStretch)
#define PSS_STRETCH (FLOAT3D(STRETCH_X, STRETCH_Y, STRETCH_Z)*m_fStretch)
#define SND_FALLOFF 1000.0f
#define SND_HOTSPOT 250.0f
#define SND_VOLUME 2.0f

#define BIG_LIGHT_BEAM_LIFE_TIME (8.0f)
#define SMALL_FLARE_WAIT 2.0f
#define SMALL_FLARES_LIFE_TIME (BIG_LIGHT_BEAM_LIFE_TIME+SMALL_FLARE_WAIT)
#define SMALL_LIGHTNING_WAIT 1.5f
#define SMALL_LIGHTININGS_LIFE_TIME (SMALL_FLARES_LIFE_TIME+SMALL_LIGHTNING_WAIT)
#define BIG_FLARE_WAIT 1.0f
#define BIG_FLARE_LIFE_TIME (SMALL_LIGHTININGS_LIFE_TIME+BIG_FLARE_WAIT)

#define BM_DX (0.414657f*STRETCH_X)
#define BM_DY (-1.72731f*STRETCH_Y)
#define BM_DZ (0.414657f*STRETCH_Z)
#define BM_FLARE_DY (-0.25f*STRETCH_Y)

#define BM_MASTER_Y (-1.76648f*STRETCH_Y)
%}

class CPyramidSpaceShip: CMovableModelEntity {
name      "PyramidSpaceShip";
thumbnail "Thumbnails\\PyramidSpaceShip.tbn";
features  "HasName", "IsTargetable";
properties:
  1 CTString m_strName            "Name" 'N' = "Pyramid Space Ship",  // name
  3 FLOAT m_fMovingSpeed = 0.0f,                                      // current speed
  5 CEntityPointer m_penBeamHit "Beam hit target marker" 'T',         // target point for light beam
  6 CEntityPointer m_penLightBeam "Beam model holder" 'B',            // light beam model holder
  7 FLOAT m_tmBeamTime =-1.0f,                                        // light beam time var
  8 CEntityPointer m_penHitPlaceFlare "Hit place flare" 'H',          // hit place model holder
  9 FLOAT m_tmHitFlareTime =-1.0f,                                    // light beam hit flare time var
 10 FLOAT m_iRingCounter = 0,                                         // ring counter
 11 FLOAT m_fRatio =0.0f,                                             // misc ratio
 12 CTString m_strDescription = "",                                   // description
 13 enum PSSState m_epssState = PSSS_IDLE,                            // current state
 14 FLOAT m_fStretch "Stretch" 'S' = 1.0f,                            // stretch factor
 // path properties
 20 BOOL m_bStopMoving = FALSE,                                       // stop moving on next marker
 21 CEntityPointer m_penTarget "Target" 'T' COLOR(C_lBLUE|0xFF),      // next path target
 29 CEntityPointer m_penFlyAwayTarget "Fly away path marker" COLOR(C_lBLUE|0xFF), // fly away path marker
 22 CEntityPointer m_penLast,                                         // previous marker
 23 BOOL m_bMoving = FALSE,                                           // set while moving
 24 FLOAT m_fRot = 0.0f,                                              // current rotation
 25 FLOAT m_fLastRotSpeed = 0.0f,                                     // last speed rotation
 26 FLOAT m_fRotSpeed = 0.0f,                                         // current speed rotation
 27 BOOL m_bApplyDamageToHitted = TRUE,                               // if damage should be applied
 28 FLOAT m_tmTemp = 0.0f,                                            // temporary time var

 30 FLOAT m_tmAtMarker = 0.0f,                                        // time when current marker was reached
 31 FLOAT m_tmDelta = 0.0f,                                           // time to reach next marker
 32 FLOAT3D m_vPNp0 = FLOAT3D(0,0,0),
 33 FLOAT3D m_vPNp1 = FLOAT3D(0,0,0),
 34 FLOAT3D m_vTNp0 = FLOAT3D(0,0,0),
 35 FLOAT3D m_vTNp1 = FLOAT3D(0,0,0),
 36 FLOATquat3D m_qPNp0 = FLOATquat3D(0,0,0,0),
 37 FLOATquat3D m_qPNp1 = FLOATquat3D(0,0,0,0),
 38 FLOATquat3D m_qANp0 = FLOATquat3D(0,0,0,0),
 39 FLOATquat3D m_qANp1 = FLOATquat3D(0,0,0,0),
 40 FLOAT m_fRotSpeedp0 = 0.0f,
 41 FLOAT m_fRotSpeedp1 = 0.0f,
 42 FLOAT m_fTRotSpeedp0 = 0.0f,
 43 FLOAT m_fTRotSpeedp1 = 0.0f,

 50 CSoundObject m_soPlates,
 51 CSoundObject m_soBeamMachine,
 52 CSoundObject m_soBeam,
 53 CSoundObject m_soFlaresFX,
 54 BOOL m_bFireingDeactivatedBeam=FALSE,
 55 BOOL m_bImmediateAnimations=FALSE,
 56 FLOAT m_fWaitAfterKillingBeam "Wait after auto killing beam" 'W' = 1.0f,
 
 60 BOOL m_bInvisible "Invisible" 'I' = FALSE,

components:
  1 model   MODEL_SPACESHIP     "Models\\CutSequences\\SpaceShip\\SpaceShip.mdl",
  2 model   MODEL_BODY          "Models\\CutSequences\\SpaceShip\\Body.mdl",
  3 texture TEXTURE_BODY        "Models\\CutSequences\\SpaceShip\\Body.tex",
  4 model   MODEL_DOOR          "Models\\CutSequences\\SpaceShip\\Door.mdl",
  5 texture TEXTURE_DOOR        "Models\\CutSequences\\SpaceShip\\Door.tex",
  6 model   MODEL_BEAMMACHINE   "Models\\CutSequences\\SpaceShip\\BeamMachine.mdl",
  7 texture TEXTURE_BEAMMACHINE "Models\\CutSequences\\SpaceShip\\BeamMachine.tex",
  8 model   MODEL_BEAMRIM       "Models\\CutSequences\\SpaceShip\\BeamMachineRim.mdl",
  9 texture TEXTURE_BEAMRIM     "Models\\CutSequences\\SpaceShip\\BeamMachineRim.tex",
 10 class   CLASS_EFFECTOR      "Classes\\Effector.ecl",
 11 model   MODEL_SHIP_INSIDE   "Models\\CutSequences\\SpaceShip\\Fillin.mdl",
 20 sound   SOUND_PLATES        "Sounds\\CutSequences\\SpaceShip\\SSPlates.wav",
 21 sound   SOUND_BEAMMACHINE   "Sounds\\CutSequences\\SpaceShip\\SSProbe.wav",
 22 sound   SOUND_BEAM          "Sounds\\CutSequences\\SpaceShip\\LaserBeam.wav",
 23 sound   SOUND_WARMUP        "Sounds\\CutSequences\\SpaceShip\\Warmup.wav",

functions:
  void Precache(void) {
    PrecacheModel   (MODEL_SPACESHIP     );
    PrecacheModel   (MODEL_BODY          );
    PrecacheTexture (TEXTURE_BODY        );
    PrecacheModel   (MODEL_DOOR          );
    PrecacheTexture (TEXTURE_DOOR        );
    PrecacheModel   (MODEL_BEAMMACHINE   );
    PrecacheTexture (TEXTURE_BEAMMACHINE );
    PrecacheModel   (MODEL_BEAMRIM       );
    PrecacheModel   (MODEL_SHIP_INSIDE   );
    PrecacheTexture (TEXTURE_BEAMRIM     );
    PrecacheClass   (CLASS_EFFECTOR, ET_SIZING_RING_FLARE);
    PrecacheClass   (CLASS_EFFECTOR, ET_SIZING_BIG_BLUE_FLARE);
    PrecacheClass   (CLASS_EFFECTOR, ET_LIGHTNING);
    PrecacheClass   (CLASS_EFFECTOR, ET_MOVING_RING);
    PrecacheSound   (SOUND_PLATES        );
    PrecacheSound   (SOUND_BEAMMACHINE   );
    PrecacheSound   (SOUND_BEAM          );
    PrecacheSound   (SOUND_WARMUP        );
  }

  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if(penTarget==NULL)
    {
      return FALSE;
    }
    if( slPropertyOffset==_offsetof(CPyramidSpaceShip, m_penTarget) ||
        slPropertyOffset==_offsetof(CPyramidSpaceShip, m_penFlyAwayTarget))
    {
      return( IsDerivedFromClass(penTarget, "Pyramid Space Ship Marker"));
    }
    return TRUE;
  }

  // Check if entity is moved on a route set up by its targets
  BOOL MovesByTargetedRoute( CTString &strTargetProperty) const
  {
    strTargetProperty = "Target";
    return TRUE;
  }

  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker( CTFileName &fnmMarkerClass, CTString &strTargetProperty) const
  {
    fnmMarkerClass = CTFILENAME("Classes\\PyramidSpaceShipMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }

  // returns description
  const CTString &GetDescription(void) const
  {
    if (m_penTarget!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", (const char *) m_penTarget->GetName());
    } else {
      ((CTString&)m_strDescription).PrintF("-><none>");
    }
    return m_strDescription;
  }


  CPlacement3D GetLerpedPlacement(void) const
  {
    return CMovableEntity::GetLerpedPlacement();
  }

  void PreMoving()
  {
    // remember old placement for lerping
    en_plLastPlacement = en_plPlacement;  
  }

  void HideBeamMachine(void)
  {
    if(GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_BEAM_RIM) != NULL)
    {
      RemoveAttachment(SPACESHIP_ATTACHMENT_BEAM_RIM);
    }
    if(GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_BEAM) != NULL)
    {
      RemoveAttachment(SPACESHIP_ATTACHMENT_BEAM);
    }
  }

  void ShowBeamMachine(void)
  {
    AddAttachment(SPACESHIP_ATTACHMENT_BEAM_RIM, MODEL_BEAMRIM, TEXTURE_BEAMRIM);
    AddAttachment(SPACESHIP_ATTACHMENT_BEAM, MODEL_BEAMMACHINE, TEXTURE_BEAMMACHINE);
    GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_BEAM_RIM)->amo_moModelObject.StretchModel(PSS_STRETCH);
    GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_BEAM)->amo_moModelObject.StretchModel(PSS_STRETCH);
  }

  void InitializePathMoving( CPyramidSpaceShipMarker *penStartMarker)
  {
    // set as current
    m_penTarget = penStartMarker;

    m_epssState = PSSS_MOVING;
    // check all markers for correct type and numbers
    INDEX ctMarkers=1;
    CPyramidSpaceShipMarker *pcm0 = (CPyramidSpaceShipMarker*) m_penTarget.ep_pen;
    if( pcm0 == NULL)
    {
      return;
    }
    CPyramidSpaceShipMarker *pcm  = (CPyramidSpaceShipMarker*) pcm0->m_penTarget.ep_pen;
    // loop thru markers
    while( pcm!=NULL && pcm->m_penTarget!=pcm0)
    {
      pcm = (CPyramidSpaceShipMarker*) pcm->m_penTarget.ep_pen;
      if (pcm==NULL) {
        WarningMessage( "Space ship path - broken link!");
        return;
      }
      ctMarkers++;
      if (ctMarkers>500) {
        WarningMessage( "Space ship path - invalid marker loop!");
        return;
      }
    }
    // check if we have enough markers to do smooth interpolation
    if( ctMarkers<2) {
      WarningMessage( "Space ship path requires at least 2 markers in order to work!");
      return;
    }

    // prepare internal variables
    FLOAT tmCurrent = _pTimer->CurrentTick();
    m_tmAtMarker = tmCurrent;
    m_tmDelta = 0.0f;
    m_bStopMoving = FALSE;
    m_penLast = pcm; // keep last marker
    ASSERT( pcm->m_penTarget == m_penTarget);
    m_bMoving = TRUE;
    AddToMovers();
  }

  void DoMoving()  
  {
    if (!m_bMoving) {
      return;
    }
    // read current tick
    FLOAT tmCurrent = _pTimer->CurrentTick();
    // lerping is initially enabled
    BOOL bLerping = TRUE;

    // if we hit a marker
    if( tmCurrent > (m_tmAtMarker+m_tmDelta - _pTimer->TickQuantum*3/2)) 
    {
      // get markers
      CPyramidSpaceShipMarker *pcmNm1 = &(CPyramidSpaceShipMarker&)*m_penLast;
      CPyramidSpaceShipMarker *pcmNp0 = &(CPyramidSpaceShipMarker&)*m_penTarget;
      CPyramidSpaceShipMarker *pcmNp1 = &(CPyramidSpaceShipMarker&)*pcmNp0->m_penTarget;
      CPyramidSpaceShipMarker *pcmNp2 = &(CPyramidSpaceShipMarker&)*pcmNp1->m_penTarget;

      // if there is a trigger at the hit marker
      if (pcmNp0->m_penTrigger!=NULL) {
        // trigger it
        SendToTarget(pcmNp0->m_penTrigger, EET_TRIGGER, NULL);
      }

      // update markers for next interval
      m_penTarget = pcmNp1;
      m_penLast   = pcmNp0;

      // get markers
      CPyramidSpaceShipMarker &cmNm1 = *pcmNm1;
      CPyramidSpaceShipMarker &cmNp0 = *pcmNp0;
      CPyramidSpaceShipMarker &cmNp1 = *pcmNp1;
      CPyramidSpaceShipMarker &cmNp2 = *pcmNp2;

      // get positions from four markers
      const FLOAT3D &vPNm1 = cmNm1.GetPlacement().pl_PositionVector;
      const FLOAT3D &vPNp0 = cmNp0.GetPlacement().pl_PositionVector;
      const FLOAT3D &vPNp1 = cmNp1.GetPlacement().pl_PositionVector;
      const FLOAT3D &vPNp2 = cmNp2.GetPlacement().pl_PositionVector;
      ANGLE3D aPNm1 = cmNm1.GetPlacement().pl_OrientationAngle;
      ANGLE3D aPNp0 = cmNp0.GetPlacement().pl_OrientationAngle;
      ANGLE3D aPNp1 = cmNp1.GetPlacement().pl_OrientationAngle;
      ANGLE3D aPNp2 = cmNp2.GetPlacement().pl_OrientationAngle;
      FLOAT fRotSpeedm1 = cmNm1.m_fRotSpeed;
      FLOAT fRotSpeedp0 = cmNp0.m_fRotSpeed;
      FLOAT fRotSpeedp1 = cmNp1.m_fRotSpeed;
      FLOAT fRotSpeedp2 = cmNp2.m_fRotSpeed;

      // find quaternions for rotations
      FLOATquat3D qPNm1; qPNm1.FromEuler(aPNm1);
      FLOATquat3D qPNp0; qPNp0.FromEuler(aPNp0);
      FLOATquat3D qPNp1; qPNp1.FromEuler(aPNp1);
      FLOATquat3D qPNp2; qPNp2.FromEuler(aPNp2);

      // make all angles between quaternion pairs acute
      if( qPNm1%qPNp0<0 ) {
        qPNp0 = -qPNp0;
      }
      if( qPNp0%qPNp1<0 ) {
        qPNp1 = -qPNp1;
      }
      if( qPNp1%qPNp2<0 ) {
        qPNp2 = -qPNp2;
      }

      // update time and position
      m_tmAtMarker = m_tmAtMarker+m_tmDelta;
      m_tmDelta    = cmNp0.m_fDeltaTime;
      m_vPNp0 = vPNp0;
      m_vPNp1 = vPNp1;
      m_fRotSpeedp0 = fRotSpeedp0;
      m_fRotSpeedp1 = fRotSpeedp1;
      m_qPNp0 = qPNp0;
      m_qPNp1 = qPNp1;

      // determine delta time multipliers
      FLOAT tmDNm1 = cmNm1.m_fDeltaTime;
      FLOAT tmDNp0 = cmNp0.m_fDeltaTime;
      FLOAT tmDNp1 = cmNp1.m_fDeltaTime;
      FLOAT fD0 = 2*tmDNp0 / (tmDNm1+tmDNp0);
      FLOAT fD1 = 2*tmDNp0 / (tmDNp0+tmDNp1);

      // determine biases, tensions and continuities
      FLOAT fBNp0 = cmNp0.m_fBias;
      FLOAT fTNp0 = cmNp0.m_fTension;
      FLOAT fCNp0 = cmNp0.m_fContinuity;
      FLOAT fBNp1 = cmNp1.m_fBias;
      FLOAT fTNp1 = cmNp1.m_fTension;
      FLOAT fCNp1 = cmNp1.m_fContinuity;

      FLOAT fF00 = (1-fTNp0)*(1-fCNp0)*(1-fBNp0) / 2;
      FLOAT fF01 = (1-fTNp0)*(1+fCNp0)*(1+fBNp0) / 2;
      FLOAT fF10 = (1-fTNp1)*(1+fCNp1)*(1-fBNp1) / 2;
      FLOAT fF11 = (1-fTNp1)*(1-fCNp1)*(1+fBNp1) / 2;

      // find tangents for translation
      m_vTNp0 = ( (vPNp1-vPNp0) * fF00 + (vPNp0-vPNm1) * fF01) * fD0;
      m_vTNp1 = ( (vPNp2-vPNp1) * fF10 + (vPNp1-vPNp0) * fF11) * fD1;

      // find tangents for moving speed
      m_fTRotSpeedp0 = ( (fRotSpeedp1-fRotSpeedp0) * fF00 + (fRotSpeedp0-fRotSpeedm1) * fF01) * fD0;
      m_fTRotSpeedp1 = ( (fRotSpeedp2-fRotSpeedp1) * fF10 + (fRotSpeedp1-fRotSpeedp0) * fF11) * fD1;

      // find tangents for rotation
      FLOATquat3D qTNp0, qTNp1;
      qTNp0 = ( Log(qPNp0.Inv()*qPNp1) * fF00 + Log(qPNm1.Inv()*qPNp0) * fF01) * fD0;
      qTNp1 = ( Log(qPNp1.Inv()*qPNp2) * fF10 + Log(qPNp0.Inv()*qPNp1) * fF11) * fD1;

      // find squad parameters
      m_qANp0 = qPNp0*Exp( (qTNp0 - Log(qPNp0.Inv()*qPNp1))/2 );
      m_qANp1 = qPNp1*Exp( (Log(qPNp0.Inv()*qPNp1) - qTNp1)/2 );

      // check for stop moving
      if( cmNp0.m_bStopMoving && m_fRotSpeed==0.0f) {
        m_bStopMoving = TRUE;
      }
    }

    // calculate the parameter value and hermit basis
    FLOAT fT  = (tmCurrent - m_tmAtMarker) / m_tmDelta;
    FLOAT fH0 =  2*fT*fT*fT - 3*fT*fT + 1;
    FLOAT fH1 = -2*fT*fT*fT + 3*fT*fT;
    FLOAT fH2 =    fT*fT*fT - 2*fT*fT + fT;
    FLOAT fH3 =    fT*fT*fT -   fT*fT;

    // interpolate position, rotation and fov
    FLOAT3D vPos = m_vPNp0*fH0 + m_vPNp1*fH1 + m_vTNp0*fH2 + m_vTNp1*fH3;
    FLOAT fRotSpeed = m_fRotSpeedp0*fH0 + m_fRotSpeedp1*fH1 + m_fTRotSpeedp0*fH2 + m_fTRotSpeedp1*fH3;
    FLOATquat3D qRot = Squad(fT, m_qPNp0, m_qPNp1, m_qANp0, m_qANp1);
    FLOATmatrix3D mRotLocal;
    MakeRotationMatrixFast(mRotLocal, ANGLE3D(m_fRot,0,0));
    FLOATmatrix3D mRot;
    qRot.ToMatrix(mRot);
    mRot = mRotLocal*mRot;

    // just cache near polygons for various engine needs
    en_vNextPosition = vPos;
    en_mNextRotation = mRot;
    CacheNearPolygons();

    // set new placement
    CPlacement3D plNew;
    plNew.pl_PositionVector = vPos;
    DecomposeRotationMatrixNoSnap(plNew.pl_OrientationAngle, mRot);
    SetPlacement_internal(plNew, mRot, TRUE);
    // if lerping is disabled
    if (!bLerping) {
      // make last placement same as this one
      en_plLastPlacement = en_plPlacement;  
    }
    // set new speed
    m_fLastRotSpeed = m_fRotSpeed;
    m_fRotSpeed = fRotSpeed;
    m_fRot += m_fRotSpeed;
  }


  void PostMoving()  
  {
    if (!m_bMoving) {
      return;
    }

    // remember new position for particles
    if (en_plpLastPositions!=NULL) {
      en_plpLastPositions->AddPosition(en_vNextPosition);
    }

    //
    if( m_bStopMoving) {
      m_bMoving = FALSE;
      // mark for removing from list of movers
      en_ulFlags |= ENF_INRENDERING;
      m_epssState = PSSS_REACHED_DESTINATION;
      // remember old placement for lerping
      en_plLastPlacement = en_plPlacement;  
    }
  }

  void SpawnBeamMachineFlares(void)
  {
    // spawn small beam machine flares
    CPlacement3D plSpaceShip = GetPlacement();
    CPlacement3D plFlare1 = CPlacement3D( FLOAT3D(  BM_DX, BM_DY+BM_FLARE_DY,      0), ANGLE3D(0,0,0));
    CPlacement3D plFlare2 = CPlacement3D( FLOAT3D(      0, BM_DY+BM_FLARE_DY, -BM_DZ), ANGLE3D(0,0,0));
    CPlacement3D plFlare3 = CPlacement3D( FLOAT3D( -BM_DX, BM_DY+BM_FLARE_DY,      0), ANGLE3D(0,0,0));
    CPlacement3D plFlare4 = CPlacement3D( FLOAT3D(      0, BM_DY+BM_FLARE_DY,  BM_DZ), ANGLE3D(0,0,0));
    
    plFlare1.RelativeToAbsolute(plSpaceShip);
    plFlare2.RelativeToAbsolute(plSpaceShip);
    plFlare3.RelativeToAbsolute(plSpaceShip);
    plFlare4.RelativeToAbsolute(plSpaceShip);

    CEntity *penFlare1 = CreateEntity( plFlare1, CLASS_EFFECTOR);
    CEntity *penFlare2 = CreateEntity( plFlare2, CLASS_EFFECTOR);
    CEntity *penFlare3 = CreateEntity( plFlare3, CLASS_EFFECTOR);
    CEntity *penFlare4 = CreateEntity( plFlare4, CLASS_EFFECTOR);

    ESpawnEffector eSpawnFlare;
    eSpawnFlare.tmLifeTime = SMALL_FLARES_LIFE_TIME;
    eSpawnFlare.tmLifeTime = 10.5f;
    eSpawnFlare.eetType = ET_SIZING_RING_FLARE;

    penFlare1->Initialize( eSpawnFlare);
    penFlare2->Initialize( eSpawnFlare);
    penFlare3->Initialize( eSpawnFlare);
    penFlare4->Initialize( eSpawnFlare);
  }

  void SpawnBeamMachineMainFlare(void)
  {
    // spawn main flare
    //CPlacement3D plSpaceShip = GetPlacement();
    CPlacement3D plFlare = CPlacement3D( FLOAT3D(0, BM_MASTER_Y+BM_FLARE_DY, 0), ANGLE3D(0,0,0));
    plFlare.RelativeToAbsolute(GetPlacement());
    CEntity *penFlare = CreateEntity( plFlare, CLASS_EFFECTOR);
    ESpawnEffector eSpawnFlare;
    eSpawnFlare.tmLifeTime = 20.0f;
    eSpawnFlare.fSize = 1.0f;
    eSpawnFlare.eetType = ET_SIZING_BIG_BLUE_FLARE;
    penFlare->Initialize( eSpawnFlare);
  }

  void ShowBeamMachineHitFlare(void)
  {
    if( m_penHitPlaceFlare!=NULL && IsOfClass(m_penHitPlaceFlare, "ModelHolder2") )
    {
      CModelObject *pmo = m_penHitPlaceFlare->GetModelObject();
      if( pmo != NULL) 
      {
        m_penHitPlaceFlare->SwitchToModel();
      }
    }
  }

  void HideBeamMachineHitFlare(void)
  {
    m_tmHitFlareTime = -1;
    if( m_penHitPlaceFlare!=NULL && IsOfClass(m_penHitPlaceFlare, "ModelHolder2") )
    {
      CModelObject *pmo = m_penHitPlaceFlare->GetModelObject();
      if( pmo != NULL) 
      {
        m_penHitPlaceFlare->SwitchToEditorModel();
        pmo->mo_colBlendColor = C_WHITE|CT_OPAQUE;
      }
    }
  }

  void SpawnBeamMachineLightnings(void)
  {
    // spawn beam lightnings
    CPlacement3D plLightning1 = CPlacement3D( FLOAT3D(BM_DX, BM_DY, 0), ANGLE3D(0,0,0));
    CPlacement3D plLightning2 = CPlacement3D( FLOAT3D(0, BM_DY, -BM_DZ), ANGLE3D(0,0,0));
    CPlacement3D plLightning3 = CPlacement3D( FLOAT3D(-BM_DX, BM_DY, 0), ANGLE3D(0,0,0));
    CPlacement3D plLightning4 = CPlacement3D( FLOAT3D(0, BM_DY, BM_DZ), ANGLE3D(0,0,0));
    
    CPlacement3D plLightningDest = CPlacement3D( FLOAT3D(0, BM_MASTER_Y, 0), ANGLE3D(0,0,0));
    CPlacement3D plSpaceShip = GetPlacement();
    plLightningDest.RelativeToAbsolute(plSpaceShip);
    
    plLightning1.RelativeToAbsolute(plSpaceShip);
    plLightning2.RelativeToAbsolute(plSpaceShip);
    plLightning3.RelativeToAbsolute(plSpaceShip);
    plLightning4.RelativeToAbsolute(plSpaceShip);

    CEntity *penLightning1 = CreateEntity( plLightning1, CLASS_EFFECTOR);
    CEntity *penLightning2 = CreateEntity( plLightning2, CLASS_EFFECTOR);
    CEntity *penLightning3 = CreateEntity( plLightning3, CLASS_EFFECTOR);
    CEntity *penLightning4 = CreateEntity( plLightning4, CLASS_EFFECTOR);

    ESpawnEffector eSpawnLightning;
    eSpawnLightning.eetType = ET_LIGHTNING;
    eSpawnLightning.tmLifeTime = SMALL_LIGHTININGS_LIFE_TIME;
    eSpawnLightning.vDestination = plLightningDest.pl_PositionVector; 
    eSpawnLightning.fSize = 16.0f;
    eSpawnLightning.ctCount = 16;

    penLightning1->Initialize( eSpawnLightning);
    penLightning2->Initialize( eSpawnLightning);
    penLightning3->Initialize( eSpawnLightning);
    penLightning4->Initialize( eSpawnLightning);
  }

  void SpawnBeamMachineMainLightning(void)
  {
    // spawn main lightning
    FLOAT3D vDestination = GetPlacement().pl_PositionVector + FLOAT3D( 0, BM_MASTER_Y, 0);
    CPlacement3D plSource = CPlacement3D( vDestination, ANGLE3D(0,0,0));
    if( m_penBeamHit != NULL)
    {
      plSource.pl_PositionVector = m_penBeamHit->GetPlacement().pl_PositionVector;
      CEntity *penEffector = CreateEntity( plSource, CLASS_EFFECTOR);
      ESpawnEffector eSpawnEffector;
      eSpawnEffector.eetType = ET_LIGHTNING;
      eSpawnEffector.tmLifeTime = BIG_LIGHT_BEAM_LIFE_TIME;
      eSpawnEffector.vDestination = vDestination;
      eSpawnEffector.fSize = 32.0f;
      eSpawnEffector.ctCount = 32;
      penEffector->Initialize( eSpawnEffector);
    }
  }

  void SpawnMovingRing(void)
  {
    if( m_penBeamHit != NULL)
    {
      FLOAT3D vStart = GetPlacement().pl_PositionVector + FLOAT3D( 0, BM_MASTER_Y, 0);
      CPlacement3D plSource = CPlacement3D( vStart, ANGLE3D(0,0,0));
      FLOAT3D vHitPlace = m_penBeamHit->GetPlacement().pl_PositionVector;
      CEntity *penEffector = CreateEntity( plSource, CLASS_EFFECTOR);
      ESpawnEffector eSpawnEffector;
      eSpawnEffector.eetType = ET_MOVING_RING;
      eSpawnEffector.tmLifeTime = BIG_LIGHT_BEAM_LIFE_TIME;
      eSpawnEffector.vDestination = vHitPlace+FLOAT3D(0.0f, 0.0f, 0.0f);
      eSpawnEffector.fSize = 16.0f;
      eSpawnEffector.ctCount = 2;
      penEffector->Initialize( eSpawnEffector);
    }
  }

  void TurnOnLightBeam(void)
  {
    if( m_penLightBeam!=NULL && IsOfClass(m_penLightBeam, "ModelHolder2") )
    {
      CModelObject *pmo = m_penLightBeam->GetModelObject();
      m_penLightBeam->SwitchToModel();
      pmo->mo_colBlendColor = C_WHITE|CT_OPAQUE;
    }
  }

  void TurnOffLightBeam(void)
  {
    m_tmBeamTime=-1.0f;

    if( m_penLightBeam!=NULL && IsOfClass(m_penLightBeam, "ModelHolder2") )
    {
      m_penLightBeam->SwitchToEditorModel();
      CModelObject *pmo = m_penLightBeam->GetModelObject();
    }
  }

  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    FLOAT fBeamRatio = 1.0f;
    
    // light beam
    if( m_penLightBeam!=NULL && IsOfClass(m_penLightBeam, "ModelHolder2") )
    {
      CModelObject *pmo = m_penLightBeam->GetModelObject();
      if( pmo != NULL) 
      {
        if( m_tmBeamTime>-1.0f)
        {
          FLOAT fT = _pTimer->CurrentTick()-m_tmBeamTime;
          fBeamRatio = 1.0f-ClampUp(fT/2.0f, 1.0f);
          UBYTE ub = UBYTE (255.0f*fBeamRatio);
          COLOR col = RGBAToColor(ub,ub,ub,ub);
          pmo->mo_colBlendColor = col;
        }
      }
    }

    // hit flare
    if( m_penHitPlaceFlare!=NULL && IsOfClass(m_penHitPlaceFlare, "ModelHolder2") )
    {
      CModelObject *pmo = m_penHitPlaceFlare->GetModelObject();
      if( pmo != NULL) 
      {
        if( m_tmHitFlareTime>-1.0f)
        {
          FLOAT fT = _pTimer->CurrentTick()-m_tmHitFlareTime;
          FLOAT fRatio = (Sin(fT*2000)*0.5f+0.5f)*(Sin(fT*1333)*0.5f+0.5f);
          /*if(fRatio>0.5f)
          {
            fRatio=0.0f;
          }
          else
          {
            fRatio=1.0f;
          }*/

          UBYTE ub = UBYTE((200+55*fRatio)*fBeamRatio);
          //ub = 255;
          COLOR col = RGBAToColor(ub,ub,ub,ub);
          pmo->mo_colBlendColor = col;
        }
      }
    }
    return FALSE;
  };

procedures:

  MPIntro()
  {
    SwitchToModel();
    m_bImmediateAnimations=TRUE;
    autocall OpenDoors() EReturn;
    autocall FireLightBeam() EReturn;
    m_epssState = PSSS_BEAM_DEACTIVATED;
    autowait(m_fWaitAfterKillingBeam);
    autocall FireLightBeam() EReturn;
    m_bImmediateAnimations=FALSE;
    autocall CloseDoors() EReturn;
    return EReturn();
  }


  OpenDoors()
  {
    // if ship inside not yet added
    if( GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_SHIPINSIDE) == NULL)
    {
      // add it
      AddAttachment( SPACESHIP_ATTACHMENT_SHIPINSIDE, MODEL_SHIP_INSIDE, TEXTURE_BODY);
      GetModelObject()->StretchModel(PSS_STRETCH);
    }
    ShowBeamMachine();

    if( !m_bImmediateAnimations)
    {
      PlaySound( m_soPlates, SOUND_PLATES, SOF_3D);
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR1)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPENING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR2)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPENING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR3)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPENING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR4)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPENING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR5)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPENING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR6)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPENING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR7)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPENING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR8)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPENING, 0); 
    }
    else
    {
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR1)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPEN, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR2)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPEN, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR3)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPEN, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR4)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPEN, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR5)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPEN, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR6)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPEN, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR7)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPEN, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR8)->amo_moModelObject.PlayAnim(DOOR_ANIM_OPEN, 0); 
    }

    // all children lights named pulsating should pulsate
    FOREACHINLIST( CEntity, en_lnInParent, en_lhChildren, iten)
    {
      if( IsOfClass(iten, "Light"))
      {
        if( iten->GetName() == "Pulsating")
        {
          CLight *penLight = (CLight *) &*iten;
          EChangeAnim eChange;
          eChange.iLightAnim=3;
          eChange.bLightLoop=TRUE;
          penLight->SendEvent(eChange);
        }
        else if( iten->GetName() == "Motors")
        {
          CLight *penLight = (CLight *) &*iten;
          EChangeAnim eChange;
          eChange.iLightAnim=4;
          eChange.bLightLoop=TRUE;
          penLight->SendEvent(eChange);
        }
      }
    }

    m_epssState = PSSS_KILLING_BEAM_FIREING;
    return EReturn();
  }

  CloseDoors()
  {
    m_epssState=PSSS_DOORS_CLOSED;
    // if ship inside attachment added
    if( GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_SHIPINSIDE) != NULL)
    {
      PlaySound( m_soPlates, SOUND_PLATES, SOF_3D);
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR1)->amo_moModelObject.PlayAnim(DOOR_ANIM_CLOSING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR2)->amo_moModelObject.PlayAnim(DOOR_ANIM_CLOSING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR3)->amo_moModelObject.PlayAnim(DOOR_ANIM_CLOSING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR4)->amo_moModelObject.PlayAnim(DOOR_ANIM_CLOSING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR5)->amo_moModelObject.PlayAnim(DOOR_ANIM_CLOSING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR6)->amo_moModelObject.PlayAnim(DOOR_ANIM_CLOSING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR7)->amo_moModelObject.PlayAnim(DOOR_ANIM_CLOSING, 0); 
      GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR8)->amo_moModelObject.PlayAnim(DOOR_ANIM_CLOSING, 0); 
    
      autowait( GetModelObject()->GetAttachmentModel( SPACESHIP_ATTACHMENT_DOOR1)->amo_moModelObject.GetAnimLength(DOOR_ANIM_CLOSING));
      // remove ship inside attachment
      RemoveAttachment( SPACESHIP_ATTACHMENT_SHIPINSIDE);
    }

    HideBeamMachine();
    InitializePathMoving( (CPyramidSpaceShipMarker*) m_penFlyAwayTarget.ep_pen);
    return EReturn();
  }

  FireLightBeam()
  {
    if(m_epssState==PSSS_DOORS_CLOSED)
    {
      return;
    }

    if(m_epssState==PSSS_BEAM_DEACTIVATED)
    {
      m_bFireingDeactivatedBeam=TRUE;
    }

    if( !m_bImmediateAnimations)
    {
      PlaySound( m_soBeamMachine, SOUND_BEAMMACHINE, SOF_3D);
      GetModelObject()->PlayAnim(SPACESHIP_ANIM_OPENING, 0); 
      autowait( GetModelObject()->GetAnimLength(SPACESHIP_ANIM_OPENING));
    }
    else
    {
      GetModelObject()->PlayAnim(SPACESHIP_ANIM_OPEN, 0); 
    }

    if( !m_bImmediateAnimations)
    {
      PlaySound( m_soBeamMachine, SOUND_WARMUP, SOF_3D);
      SpawnBeamMachineFlares();
      autowait( SMALL_FLARE_WAIT);
    }
    else
    {
      SpawnBeamMachineFlares();
    }

    SpawnBeamMachineLightnings();
    if( !m_bImmediateAnimations)
    {
      autowait( SMALL_LIGHTNING_WAIT);
    }

    SpawnBeamMachineMainFlare();
    if( !m_bImmediateAnimations)
    {
      autowait( BIG_FLARE_WAIT);
    }

    // turn on light beam
    TurnOnLightBeam();
    if(!m_bFireingDeactivatedBeam)
    {
      SpawnBeamMachineMainLightning();
    }

    m_soBeam.Set3DParameters(SND_FALLOFF, SND_HOTSPOT, SND_VOLUME, 1.0f);
    PlaySound( m_soBeam, SOUND_BEAM, SOF_3D|SOF_LOOP);
    ShowBeamMachineHitFlare();
    m_tmHitFlareTime = _pTimer->CurrentTick();
    
    m_iRingCounter = 0;
    while(_pTimer->CurrentTick()<m_tmHitFlareTime+BIG_LIGHT_BEAM_LIFE_TIME)
    {
      // spawn one moving ring
      if( m_iRingCounter < 16)
      {
        SpawnMovingRing();
        m_iRingCounter++;
      }

      // apply beam damage
      m_tmTemp = _pTimer->CurrentTick();
      while( _pTimer->CurrentTick() < m_tmTemp+0.49f)
      {
        autowait(_pTimer->TickQuantum);
        // cast ray for possible damage
        if( m_penBeamHit != NULL && !m_bFireingDeactivatedBeam)
        {
          // cast ray
          FLOAT3D vSource = GetPlacement().pl_PositionVector + FLOAT3D( 0, BM_MASTER_Y, 0);
          FLOAT3D vDestination = m_penBeamHit->GetPlacement().pl_PositionVector;
          CCastRay crRay( this, vSource, vDestination);
          crRay.cr_bHitTranslucentPortals = FALSE;
          crRay.cr_bPhysical = FALSE;
          crRay.cr_ttHitModels = CCastRay::TT_COLLISIONBOX;
          crRay.cr_fTestR = 16.0f;
          GetWorld()->CastRay(crRay);
        
          // if entity is hit
          if( crRay.cr_penHit != NULL)
          {
            InflictDirectDamage( crRay.cr_penHit, this, DMT_BULLET, 
              10000.0f/GetGameDamageMultiplier()*_pTimer->TickQuantum/0.5f/16.0f,
              FLOAT3D(0, 0, 0), (vSource-vDestination).Normalize());
            crRay.cr_penHit->SendEvent( EHitBySpaceShipBeam());
          }
        }
      }
    }

    m_tmBeamTime = _pTimer->CurrentTick();
    while(_pTimer->CurrentTick()<m_tmBeamTime+2.0f)
    {
      autowait(_pTimer->TickQuantum);
      FLOAT tmNow = _pTimer->CurrentTick();
      FLOAT fRatio = CalculateRatio(tmNow, m_tmBeamTime, m_tmBeamTime+2.0f, 0, 1.0f);
      m_soBeam.Set3DParameters(SND_FALLOFF, SND_HOTSPOT, fRatio*SND_VOLUME, 1.0f);
    }
    
    // turn off light beam
    TurnOffLightBeam();
    HideBeamMachineHitFlare();
    
    // little pause
    autowait( 2.0f);

    GetModelObject()->PlayAnim(SPACESHIP_ANIM_CLOSING, 0); 
    PlaySound( m_soBeamMachine, SOUND_BEAMMACHINE, SOF_3D);
    autowait( GetModelObject()->GetAnimLength(SPACESHIP_ANIM_CLOSING));

    m_tmHitFlareTime = -1.0f;
    m_tmBeamTime = -1.0f;

    if(m_bFireingDeactivatedBeam)
    {
      jump CloseDoors();
    }
    return EReturn();
  }

  Main() {
    // declare yourself as a model
    InitAsEditorModel();
    //InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL|EPF_MOVABLE);
    SetCollisionFlags(ECF_MODEL_HOLDER);

    m_bImmediateAnimations=FALSE;
    en_fAcceleration = 1e6f;
    en_fDeceleration = 1e6f;

    m_soBeam.Set3DParameters(SND_FALLOFF, SND_HOTSPOT, SND_VOLUME, 1.0f);
    m_soBeamMachine.Set3DParameters(SND_FALLOFF, SND_HOTSPOT, SND_VOLUME/2.0f, 1.0f);
    m_soPlates.Set3DParameters(SND_FALLOFF, SND_HOTSPOT, SND_VOLUME/2.0f, 1.0f);
    m_soFlaresFX.Set3DParameters(SND_FALLOFF, SND_HOTSPOT, SND_VOLUME, 1.0f);

    // set appearance
    SetModel(MODEL_SPACESHIP);
    SetModelMainTexture(TEXTURE_BODY);
    AddAttachment(SPACESHIP_ATTACHMENT_BODY, MODEL_BODY, TEXTURE_BODY);
    AddAttachment(SPACESHIP_ATTACHMENT_DOOR1, MODEL_DOOR, TEXTURE_DOOR);
    AddAttachment(SPACESHIP_ATTACHMENT_DOOR2, MODEL_DOOR, TEXTURE_DOOR);
    AddAttachment(SPACESHIP_ATTACHMENT_DOOR3, MODEL_DOOR, TEXTURE_DOOR);
    AddAttachment(SPACESHIP_ATTACHMENT_DOOR4, MODEL_DOOR, TEXTURE_DOOR);
    AddAttachment(SPACESHIP_ATTACHMENT_DOOR5, MODEL_DOOR, TEXTURE_DOOR);
    AddAttachment(SPACESHIP_ATTACHMENT_DOOR6, MODEL_DOOR, TEXTURE_DOOR);
    AddAttachment(SPACESHIP_ATTACHMENT_DOOR7, MODEL_DOOR, TEXTURE_DOOR);
    AddAttachment(SPACESHIP_ATTACHMENT_DOOR8, MODEL_DOOR, TEXTURE_DOOR);

    GetModelObject()->StretchModel(PSS_STRETCH);
    ModelChangeNotify();
    m_bMoving = FALSE;
    m_epssState = PSSS_IDLE;
    m_bFireingDeactivatedBeam=FALSE;
    
    autowait( 0.25f);

    // turn off light beam
    TurnOffLightBeam();
    // turn off light beam hit flare
    HideBeamMachineHitFlare();

    // start moving
    wait() {
      on( EActivate):
      {
        if( !m_bInvisible)
        {
          SwitchToModel();
        }
        InitializePathMoving((CPyramidSpaceShipMarker*) m_penTarget.ep_pen);
        resume;
      }
      on( ETrigger):
      {
        if(m_epssState == PSSS_IDLE)
        {
          // ignore all triggs
        }
        else if( m_epssState==PSSS_KILLING_BEAM_FIREING)
        {
          call FireLightBeam();
        }
        else if(m_epssState==PSSS_BEAM_DEACTIVATED)
        {
          call FireLightBeam();
        }
        else if(m_epssState == PSSS_REACHED_DESTINATION)
        {
          call OpenDoors();
        }
        resume;
      }
      on (EForcePathMarker eForcePathMarker):
      {
        if(m_epssState != PSSS_IDLE)
        {
          m_penTarget = eForcePathMarker.penForcedPathMarker;
          InitializePathMoving((CPyramidSpaceShipMarker*) m_penTarget.ep_pen);
        }
        resume;
      }
      on( EEnvironmentStart):
      {
        call MPIntro();
        resume;
      }
      on( EEnvironmentStop):
      {
        m_bMoving = FALSE;
        PostMoving();
        resume;
      }
      on( EDeactivate):
      {
        m_epssState = PSSS_BEAM_DEACTIVATED;
        resume;
      }
      on( EReturn):
      {
        resume;
      }
    }

    Destroy();
    return;
  }
};
