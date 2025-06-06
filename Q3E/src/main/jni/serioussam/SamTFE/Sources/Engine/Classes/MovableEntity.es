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

/*
 * Entity that can move and obey physics.
 */
1
%{
#include <Engine/StdH.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/World/PhysicsProfile.h>
#include <Engine/Math/Geometry.inl>
#include <Engine/Math/Float.h>
#include <Engine/Base/Stream.h>
#include <Engine/World/World.h>
#include <Engine/Network/Network.h>
#include <Engine/Entities/EntityCollision.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/BSP.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/World/WorldSettings.h>
#include <Engine/World/WorldCollision.h>
#include <Engine/Math/Clipping.inl>
#include <Engine/Light/LightSource.h>
#include <Engine/Entities/LastPositions.h>
#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Base/Console.h>
#include <Engine/Base/CRC.h>
#include <Engine/Network/SessionState.h>
#include <Engine/Terrain/TerrainMisc.h>
#define CLEARMEM(var) memset(&var, 0, sizeof(var))

%}


%{

#define ANYEXCEPTION  ...
template class CStaticStackArray<CBrushPolygon*>;

#define MAXCOLLISIONRETRIES 4*4
extern FLOAT phy_fCollisionCacheAhead;
extern FLOAT phy_fCollisionCacheAround;
extern FLOAT cli_fPredictionFilter;

// force breakpoint (debug)
extern INDEX dbg_bBreak;
// must be in separate function to disable stupid optimizer
extern void Breakpoint(void); 

CEntity *GetPredictedSafe(CEntity *pen)
{
  if ((pen->en_ulFlags&(ENF_PREDICTOR|ENF_TEMPPREDICTOR)) == ENF_PREDICTOR) {
    return pen->GetPredicted();
  } else {
    return pen;
  }
}

// add acceleration to velocity
static inline void AddAcceleration(
  FLOAT3D &vCurrentVelocity, const FLOAT3D &vDesiredVelocity, 
  FLOAT fAcceleration, FLOAT fDecceleration)
{
  // if desired velocity is smaller than current velocity
  if (vDesiredVelocity.Length()<vCurrentVelocity.Length()) {
    fAcceleration=fDecceleration;
  }
  // find difference between current and desired velocities
  FLOAT3D vDelta = vDesiredVelocity-vCurrentVelocity;
  // accelerate in the direction of the difference with given maximum acceleration
  FLOAT fDelta = vDelta.Length();
  if (fDelta>fAcceleration) {
    vCurrentVelocity += vDelta*(fAcceleration/fDelta);
  } else {
    vCurrentVelocity = vDesiredVelocity;
  }
}

// add gravity acceleration to velocity along an axis
static inline void AddGAcceleration(
  FLOAT3D &vCurrentVelocity, const FLOAT3D &vGDir, 
  FLOAT fGA, FLOAT fGV)
{
  // disassemble speed
  FLOAT3D vCurrentParallel, vCurrentOrthogonal;
  GetParallelAndNormalComponents(vCurrentVelocity, vGDir, vCurrentOrthogonal, vCurrentParallel);

/* 
IMPORTANT:
This is how this piece of code should look like:

  // if not already going down at max speed
  if (! (vCurrentOrthogonal%vGDir>=fGV)) {
    // add accelleration to parallel speed
    vCurrentOrthogonal+=vGDir*fGA;

    // if going down at max speed
    if (vCurrentOrthogonal%vGDir>=fGV) {
      // clamp
      vCurrentOrthogonal = vGDir*fGV;
    }
  }

But, due to need for compatibility with older versions and bad VC code generator, we use this kludge:
  */
  
// KLUDGE_BEGIN

  if (_pNetwork->ga_ulDemoMinorVersion<=2) {
    Swap(vCurrentOrthogonal, vCurrentParallel);
  }

  FLOAT3D vCurrentOrthogonalOrg=vCurrentOrthogonal;
  // add accelleration to parallel speed
  vCurrentOrthogonal+=vGDir*fGA;

  // if going down at max speed
  if (vCurrentOrthogonal%vGDir>=fGV) {
    // clamp
    vCurrentOrthogonal = vGDir*fGV;
  } else {
    vCurrentOrthogonalOrg = vCurrentOrthogonal;
  }

  if (_pNetwork->ga_ulDemoMinorVersion>2) {
    vCurrentOrthogonal=vCurrentOrthogonalOrg;
  }
// KLUDGE_END

  // assemble speed back
  vCurrentVelocity = vCurrentParallel+vCurrentOrthogonal;
}

// NOTE:
// this is pulled out into a separate function because, otherwise, VC6 generates
// invalid code when optimizing this. no clue why is that so.

#pragma inline_depth(0)
static void CheckAndAddGAcceleration(CMovableEntity *pen, FLOAT3D &vTranslationAbsolute, FLOAT fTickQuantum)
{
  // if there is forcefield involved
  if (pen->en_fForceA>0.01f) {
    // add force acceleration
    FLOAT fGV=pen->en_fForceV*fTickQuantum;
    FLOAT fGA=pen->en_fForceA*fTickQuantum*fTickQuantum;
    AddGAcceleration(vTranslationAbsolute, pen->en_vForceDir, fGA, fGV);
  }
}
#pragma inline_depth()  // see important note above


// add acceleration to velocity, but only along a plane
static inline void AddAccelerationOnPlane(
  FLOAT3D &vCurrentVelocity, const FLOAT3D &vDesiredVelocity, 
  FLOAT fAcceleration, FLOAT fDecceleration,
  const FLOAT3D &vPlaneNormal)
{
  FLOAT3D vCurrentParallel, vCurrentOrthogonal;
  GetParallelAndNormalComponents(vCurrentVelocity, vPlaneNormal, vCurrentOrthogonal, vCurrentParallel);
  FLOAT3D vDesiredParallel;
  GetNormalComponent(vDesiredVelocity, vPlaneNormal, vDesiredParallel);
  AddAcceleration(vCurrentParallel, vDesiredParallel, fAcceleration, fDecceleration);
  vCurrentVelocity = vCurrentParallel+vCurrentOrthogonal;
}

// add acceleration to velocity, for roller-coaster slope -- slow!
static inline void AddAccelerationOnPlane2(
  FLOAT3D &vCurrentVelocity, const FLOAT3D &vDesiredVelocity, 
  FLOAT fAcceleration, FLOAT fDecceleration,
  const FLOAT3D &vPlaneNormal, const FLOAT3D &vGravity)
{
  // get down and horizontal direction
  FLOAT3D vDn;
  GetNormalComponent(vGravity, vPlaneNormal, vDn);
  vDn.Normalize();
  FLOAT3D vRt = vPlaneNormal*vDn;
  vRt.Normalize();

  // add only horizontal acceleration
  FLOAT3D vCurrentParallel, vCurrentOrthogonal;
  GetParallelAndNormalComponents(vCurrentVelocity, vRt, vCurrentParallel, vCurrentOrthogonal);
  FLOAT3D vDesiredParallel;
  GetParallelComponent(vDesiredVelocity, vRt, vDesiredParallel);
  AddAcceleration(vCurrentParallel, vDesiredParallel, fAcceleration, fDecceleration);
  vCurrentVelocity = vCurrentParallel+vCurrentOrthogonal;
}

// max number of retries during movement
static INDEX _ctTryToMoveCheckCounter;
static INDEX _ctSliding;
static FLOAT3D _vSlideOffDir;   // move away direction for sliding
static FLOAT3D _vSlideDir;
static void InitTryToMove(void)
{
  _ctTryToMoveCheckCounter = MAXCOLLISIONRETRIES;
  _ctSliding = 0;
  _vSlideOffDir = FLOAT3D(0,0,0);
  _vSlideDir = FLOAT3D(0,0,0);
}

// array of forces for current entity
class CEntityForce {
public:
  CEntityPointer ef_penEntity;
  INDEX ef_iForceType;
  FLOAT ef_fRatio;    // how much of entity this force gets [0-1]
  inline void Clear(void) {
    ef_penEntity = NULL;
  };
  ~CEntityForce(void) {
    Clear();
  };
};
static CStaticStackArray<CEntityForce> _aefForces;

void ClearMovableEntityCaches(void)
{
  _aefForces.Clear();
}

%}

class export CMovableEntity : CRationalEntity {
name      "MovableEntity";
thumbnail "";

properties:

  // NOTE: all properties that are not marked as 'adjustable' should be threated read-only

  // translation and rotation speed that this entity would like to have (in relative system)
  1 FLOAT3D en_vDesiredTranslationRelative = FLOAT3D(0.0f,0.0f,0.0f),
  2 ANGLE3D en_aDesiredRotationRelative = ANGLE3D(0,0,0),

  // translation and rotation speed that this entity currently has in absolute system
  3 FLOAT3D en_vCurrentTranslationAbsolute = FLOAT3D(0.0f,0.0f,0.0f),
  4 ANGLE3D en_aCurrentRotationAbsolute = ANGLE3D(0,0,0),

  6 CEntityPointer en_penReference, // reference entity (for standing on)
  7 FLOAT3D en_vReferencePlane = FLOAT3D(0.0f,0.0f,0.0f),   // reference plane (only for standing on)
  8 INDEX en_iReferenceSurface = 0,     // surface on reference entity
  9 CEntityPointer en_penLastValidReference,  // last valid reference entity (for impact damage)
 14 FLOAT en_tmLastSignificantVerticalMovement = 0.0f,   // last time entity moved significantly up/down
  // swimming parameters
 10 FLOAT en_tmLastBreathed = 0,        // last time when entity took some air
 11 FLOAT en_tmMaxHoldBreath = 5.0f,    // how long can entity be without air (adjustable)
 12 FLOAT en_fDensity = 5000.0f,        // density of the body [kg/m3] - defines buoyancy (adjustable)
 13 FLOAT en_tmLastSwimDamage = 0,      // last time when entity was damaged by swimming
 // content immersion parameters
 20 INDEX en_iUpContent = 0,
 21 INDEX en_iDnContent = 0,
 22 FLOAT en_fImmersionFactor = 1.0f,
 // force parameters
 25 FLOAT3D en_vGravityDir = FLOAT3D(0,-1,0),
 26 FLOAT en_fGravityA = 0.0f,
 27 FLOAT en_fGravityV = 0.0f,
 66 FLOAT3D en_vForceDir = FLOAT3D(1,0,0),
 67 FLOAT en_fForceA = 0.0f,
 68 FLOAT en_fForceV = 0.0f,
 // jumping parameters
 30 FLOAT en_tmJumped = 0,            // time when entity jumped
 31 FLOAT en_tmMaxJumpControl = 0.5f,  // how long after jump can have control in the air [s] (adjustable)
 32 FLOAT en_fJumpControlMultiplier = 0.5f,  // how good is control when jumping (adjustable)
 // movement parameters
 35 FLOAT en_fAcceleration = 200.0f,  // acc/decc [m/s2] in ideal situation (adjustable)
 36 FLOAT en_fDeceleration = 40.0f,

 37 FLOAT en_fStepUpHeight = 1.0f,        // how high can entity step upstairs (adjustable)
 42 FLOAT en_fStepDnHeight = -1.0f,       // how low can entity step (negative means don't check) (adjustable)
 38 FLOAT en_fBounceDampParallel = 0.5f,  // damping parallel to plane at each bounce (adjustable)
 39 FLOAT en_fBounceDampNormal   = 0.5f,  // damping normal to plane damping at each bounce (adjustable)
 // collision damage control
 40 FLOAT en_fCollisionSpeedLimit = 20.0f,      // max. collision speed without damage (adjustable)
 41 FLOAT en_fCollisionDamageFactor = 20.0f,    // collision damage ammount multiplier (adjustable)

 51 FLOATaabbox3D en_boxMovingEstimate = FLOATaabbox3D(FLOAT3D(0,0,0), 0.01f), // overestimate of movement in next few ticks
 52 FLOATaabbox3D en_boxNearCached = FLOATaabbox3D(FLOAT3D(0,0,0), 0.01f),     // box in which the polygons are cached

 // intended movement in this tick
 64  FLOAT3D en_vIntendedTranslation = FLOAT3D(0,0,0),  // can be read on receiving a touch event, holds last velocity before touch
 65  FLOATmatrix3D en_mIntendedRotation = FLOATmatrix3D(0),

{
// these are not saved via the property system
  
  CPlacement3D en_plLastPlacement;  // placement in last tick (used for lerping) (not saved)
  CListNode en_lnInMovers;          // node in list of moving entities  (saved as bool)

  CBrushPolygon *en_pbpoStandOn; // cached last polygon standing on, just for optimization
  // used for caching near polygons of zoning brushes for fast collision detection
  CStaticStackArray<CBrushPolygon *> en_apbpoNearPolygons;  // cached polygons

  FLOAT en_tmLastPredictionHead;
  FLOAT3D en_vLastHead;
  FLOAT3D en_vPredError;
  FLOAT3D en_vPredErrorLast;

  // these are really temporary - should never be used across ticks
  // next placement for collision detection
  FLOAT3D en_vNextPosition;
  FLOATmatrix3D en_mNextRotation;

  // delta for this movement
  FLOAT3D en_vMoveTranslation;
  FLOATmatrix3D en_mMoveRotation;
  // aplied movement in this tick
  FLOAT3D en_vAppliedTranslation;
  FLOATmatrix3D en_mAppliedRotation;
}


components:


functions:


  void ResetPredictionFilter(void)
  {
    en_tmLastPredictionHead = -2;
    en_vLastHead = en_plPlacement.pl_PositionVector;
    en_vPredError = en_vPredErrorLast = FLOAT3D(0,0,0);
  }

  /* Constructor. */
  export void CMovableEntity(void)
  {
    en_pbpoStandOn = NULL;
    en_apbpoNearPolygons.SetAllocationStep(5);
    ResetPredictionFilter();
  }
  export void ~CMovableEntity(void)
  {
  }

  /* Initialization. */
  export void OnInitialize(const CEntityEvent &eeInput)
  {
    CRationalEntity::OnInitialize(eeInput);
    ClearTemporaryData();
    en_vIntendedTranslation = FLOAT3D(0,0,0);
    en_mIntendedRotation.Diagonal(1.0f);
    en_boxNearCached = FLOATaabbox3D();
    en_boxMovingEstimate = FLOATaabbox3D();
    en_pbpoStandOn = NULL;
  }
  /* Called before releasing entity. */
  export void OnEnd(void)
  {
    // remove from movers if active
    if (en_lnInMovers.IsLinked()) {
      en_lnInMovers.Remove();
    }
    ClearTemporaryData();
    en_boxNearCached = FLOATaabbox3D();
    en_boxMovingEstimate = FLOATaabbox3D();
    CRationalEntity::OnEnd();
  }
  export void Copy(CEntity &enOther, ULONG ulFlags)
  {
    CRationalEntity::Copy(enOther, ulFlags);
    CMovableEntity *pmenOther = (CMovableEntity *)(&enOther);

    if (ulFlags&COPY_PREDICTOR) {
      en_plLastPlacement      = pmenOther->en_plLastPlacement      ;
      en_vNextPosition        = pmenOther->en_vNextPosition        ;
      en_mNextRotation        = pmenOther->en_mNextRotation        ;
///*!*/      en_vIntendedTranslation = pmenOther->en_vIntendedTranslation ;
///*!*/      en_mIntendedRotation    = pmenOther->en_mIntendedRotation    ;
      en_vAppliedTranslation  = pmenOther->en_vAppliedTranslation  ;
      en_mAppliedRotation     = pmenOther->en_mAppliedRotation     ;
      en_boxNearCached        = pmenOther->en_boxNearCached        ;
      en_boxMovingEstimate    = pmenOther->en_boxMovingEstimate    ;
      en_pbpoStandOn          = pmenOther->en_pbpoStandOn          ;
      en_apbpoNearPolygons    = pmenOther->en_apbpoNearPolygons    ;
    } else {
      ClearTemporaryData();
      en_boxNearCached = FLOATaabbox3D();
      en_boxMovingEstimate = FLOATaabbox3D();
      en_pbpoStandOn = NULL;
    }

    ResetPredictionFilter();
    en_plLastPlacement = pmenOther->en_plLastPlacement;
    if (pmenOther->en_lnInMovers.IsLinked()) {
      AddToMovers();
    }
  }

  void ClearTemporaryData(void)
  {
    en_plLastPlacement = en_plPlacement;
    // init moving parameters so that they are valid for collision if entity is not moving
    en_vNextPosition = en_plPlacement.pl_PositionVector;
    en_mNextRotation = en_mRotation;
///*!*/    en_vIntendedTranslation = FLOAT3D(0,0,0);
///*!*/    en_mIntendedRotation.Diagonal(1.0f);
    en_vAppliedTranslation = FLOAT3D(0,0,0);
    en_mAppliedRotation.Diagonal(1.0f);
    ResetPredictionFilter();

// !!!! is this ok?
//    en_pbpoStandOn = NULL;
//    en_apbpoNearPolygons.Clear();
//    en_apbpoNearPolygons.SetAllocationStep(5);
  }

  // create a checksum value for sync-check
  export void ChecksumForSync(ULONG &ulCRC, INDEX iExtensiveSyncCheck)
  {
    CRationalEntity::ChecksumForSync(ulCRC, iExtensiveSyncCheck);
    if (iExtensiveSyncCheck>0) {
      if (en_pbpoStandOn!=NULL) {
        CRC_AddLONG(ulCRC, en_pbpoStandOn->bpo_iInWorld);
      }
      CRC_AddFLOAT(ulCRC, en_apbpoNearPolygons.Count());
      if (iExtensiveSyncCheck>2) {
        for (INDEX i=0; i<en_apbpoNearPolygons.Count(); i++) {
          CRC_AddLONG(ulCRC, en_apbpoNearPolygons[i]->bpo_iInWorld);
        }
      }
      CRC_AddBlock(ulCRC, (UBYTE*)&en_vReferencePlane, sizeof(en_vReferencePlane));
      CRC_AddBlock(ulCRC, (UBYTE*)&en_vDesiredTranslationRelative, sizeof(en_vDesiredTranslationRelative));
      CRC_AddBlock(ulCRC, (UBYTE*)&en_aDesiredRotationRelative, sizeof(en_aDesiredRotationRelative));
      CRC_AddBlock(ulCRC, (UBYTE*)&en_vCurrentTranslationAbsolute, sizeof(en_vCurrentTranslationAbsolute));
      CRC_AddBlock(ulCRC, (UBYTE*)&en_aCurrentRotationAbsolute, sizeof(en_aCurrentRotationAbsolute));
    }
  }

  // dump sync data to text file
  export void DumpSync_t(CTStream &strm, INDEX iExtensiveSyncCheck)  // throw char *
  {
    CRationalEntity::DumpSync_t(strm, iExtensiveSyncCheck);
    if (iExtensiveSyncCheck>0) {
      strm.FPrintF_t("standon polygon: ");
      if (en_pbpoStandOn!=NULL) {
        strm.FPrintF_t("%d\n", en_pbpoStandOn->bpo_iInWorld);
      } else {
        strm.FPrintF_t("<none>\n");
      }
      strm.FPrintF_t("near polygons: %d - ", en_apbpoNearPolygons.Count());
      if (iExtensiveSyncCheck>2) {
        for (INDEX i=0; i<en_apbpoNearPolygons.Count(); i++) {
          strm.FPrintF_t("%d, ", en_apbpoNearPolygons[i]->bpo_iInWorld);
        }
      }
      strm.FPrintF_t("\n");
      strm.FPrintF_t("desired translation: %g, %g, %g (%08X %08X %08X)\n",
        en_vDesiredTranslationRelative(1),
        en_vDesiredTranslationRelative(2),
        en_vDesiredTranslationRelative(3),
        (ULONG&)en_vDesiredTranslationRelative(1),
        (ULONG&)en_vDesiredTranslationRelative(2),
        (ULONG&)en_vDesiredTranslationRelative(3));
      strm.FPrintF_t("desired rotation: %g, %g, %g (%08X %08X %08X)\n",
        en_aDesiredRotationRelative(1),
        en_aDesiredRotationRelative(2),
        en_aDesiredRotationRelative(3),
        (ULONG&)en_aDesiredRotationRelative(1),
        (ULONG&)en_aDesiredRotationRelative(2),
        (ULONG&)en_aDesiredRotationRelative(3));
      strm.FPrintF_t("current translation: %g, %g, %g (%08X %08X %08X)\n",
        en_vCurrentTranslationAbsolute(1),
        en_vCurrentTranslationAbsolute(2),
        en_vCurrentTranslationAbsolute(3),
        (ULONG&)en_vCurrentTranslationAbsolute(1),
        (ULONG&)en_vCurrentTranslationAbsolute(2),
        (ULONG&)en_vCurrentTranslationAbsolute(3));
      strm.FPrintF_t("current rotation: %g, %g, %g (%08X %08X %08X)\n",
        en_aCurrentRotationAbsolute(1),
        en_aCurrentRotationAbsolute(2),
        en_aCurrentRotationAbsolute(3),
        (ULONG&)en_aCurrentRotationAbsolute(1),
        (ULONG&)en_aCurrentRotationAbsolute(2),
        (ULONG&)en_aCurrentRotationAbsolute(3));
      strm.FPrintF_t("reference plane: %g, %g, %g (%08X %08X %08X)\n",
        en_vReferencePlane(1),
        en_vReferencePlane(2),
        en_vReferencePlane(3),
        (ULONG&)en_vReferencePlane(1),
        (ULONG&)en_vReferencePlane(2),
        (ULONG&)en_vReferencePlane(3));
      strm.FPrintF_t("reference surface: %d\n", en_iReferenceSurface);
      strm.FPrintF_t("reference entity: ");
      if (en_penReference!=NULL) {
        strm.FPrintF_t("id: %08X\n", en_penReference->en_ulID);
      } else {
        strm.FPrintF_t("none\n");
      }
    }
  }

  /* Read from stream. */
  export void Read_t( CTStream *istr) // throw char *
  {
    CRationalEntity::Read_t(istr);
    // last placement is not saved to disk - it is not neccessary!
    ClearTemporaryData();

    // old version didn't load this
    if (istr->PeekID_t()==CChunkID("MENT")) { // 'movable entity'
      istr->ExpectID_t("MENT"); // 'movable entity'

      INDEX ibpo;
      (*istr)>>ibpo;
      en_pbpoStandOn = GetWorldPolygonPointer(ibpo);

      BOOL bAnyNULLs = FALSE;
      INDEX ctbpoNear;
      (*istr)>>ctbpoNear;
      if (ctbpoNear>0) {
        en_apbpoNearPolygons.PopAll();
        en_apbpoNearPolygons.Push(ctbpoNear);
        for(INDEX i=0; i<ctbpoNear; i++) {
          INDEX ibpo;
          (*istr)>>ibpo;
           en_apbpoNearPolygons[i] = GetWorldPolygonPointer(ibpo);
           if (en_apbpoNearPolygons[i]==NULL) {
            bAnyNULLs = TRUE;
           }
        }
        if (bAnyNULLs) {
          CPrintF("NULL saved for near polygon!\n");
          en_apbpoNearPolygons.PopAll();
        }
      }
    }

    // if entity was in list of movers when saving
    BOOL bWasMoving;
    (*istr)>>bWasMoving;
    if (bWasMoving) {
      // add it to movers
      AddToMovers();
    }
  }
  /* Write to stream. */
  export void Write_t( CTStream *ostr) // throw char *
  {
    CRationalEntity::Write_t(ostr);

    ostr->WriteID_t("MENT"); // 'movable entity'

    INDEX ibpo;
    ibpo = GetWorldPolygonIndex(en_pbpoStandOn);
    (*ostr)<<ibpo;

    INDEX ctbpoNear = en_apbpoNearPolygons.Count();
    (*ostr)<<ctbpoNear;
    for(INDEX i=0; i<ctbpoNear; i++) {
      INDEX ibpo;
      ibpo = GetWorldPolygonIndex(en_apbpoNearPolygons[i]);
      (*ostr)<<ibpo;
    }

    // last placement is not saved to disk - it is not neccessary!
    // save linked state in list of movers
    (*ostr)<<en_lnInMovers.IsLinked();
  }

  // this one is used in rendering - gets lerped placement between ticks 
  export CPlacement3D GetLerpedPlacement(void) const
  {
    // get the lerping factor
    FLOAT fLerpFactor;
    if (IsPredictor()) {
      fLerpFactor = _pTimer->GetLerpFactor();
    } else {
      fLerpFactor = _pTimer->GetLerpFactor2();
    }
    CPlacement3D plLerped;
    plLerped.Lerp(en_plLastPlacement, en_plPlacement, fLerpFactor);
    CMovableEntity *penTail = (CMovableEntity *)GetPredictedSafe((CEntity*)this);
    // if should filter predictions
    extern BOOL _bPredictionActive;
    if (_bPredictionActive) {
      // add the smoothed error
      FLOAT3D vError = penTail->en_vPredError;
      vError*=pow(cli_fPredictionFilter, fLerpFactor);
//      FLOAT fErrLen = vError.Length();
//      if (fErrLen>0) {
//        vError = vError/fErrLen*ClampDn(fErrLen-cli_fPredictionCorrection*fLerpFactor, 0.0f);
//      }
      plLerped.pl_PositionVector -= vError;
    }
    return plLerped;
  }
  /* Add yourself to list of movers. */
  export void AddToMovers(void)
  {
    if (!en_lnInMovers.IsLinked()) {
      en_pwoWorld->wo_lhMovers.AddTail(en_lnInMovers);
    }
  }

  export void AddToMoversDuringMoving(void) // used for recursive adding
  {
    // if already added
    if (en_lnInMovers.IsLinked()) {
      // do nothing
      return;
    }
    // add it
    AddToMovers();
    // mark that it was forced to add
    en_ulPhysicsFlags|=EPF_FORCEADDED;
  }

  /* Set desired rotation speed of movable entity. */
  export void SetDesiredRotation(const ANGLE3D &aRotation) 
  {
    en_aDesiredRotationRelative = aRotation;
    AddToMovers();
  }
  export const ANGLE3D &GetDesiredRotation(void) const { return en_aDesiredRotationRelative; };

  /* Set desired translation speed of movable entity. */
  export void SetDesiredTranslation(const FLOAT3D &vTranslation) 
  {
    en_vDesiredTranslationRelative = vTranslation;
    AddToMovers();
  }
  export const FLOAT3D &GetDesiredTranslation(void) const { return en_vDesiredTranslationRelative; };

  /* Add an impulse to the current speed of the entity (used for instantaneous launching). */
  export void GiveImpulseTranslationRelative(const FLOAT3D &vImpulseSpeedRelative)
  {
    CPlacement3D plImpulseSpeedAbsolute( vImpulseSpeedRelative, ANGLE3D(0,0,0)); 
    plImpulseSpeedAbsolute.RelativeToAbsolute(
      CPlacement3D(FLOAT3D(0.0f,0.0f,0.0f), en_plPlacement.pl_OrientationAngle));
    en_vCurrentTranslationAbsolute += plImpulseSpeedAbsolute.pl_PositionVector;
    AddToMovers();
  }
  export void GiveImpulseTranslationAbsolute(const FLOAT3D &vImpulseSpeed)
  {
    en_vCurrentTranslationAbsolute += vImpulseSpeed;
    AddToMovers();
  }

  export void LaunchAsPropelledProjectile(const FLOAT3D &vImpulseSpeedRelative, 
    CMovableEntity *penLauncher)
  {
    en_vDesiredTranslationRelative = vImpulseSpeedRelative;
    en_vCurrentTranslationAbsolute += vImpulseSpeedRelative*en_mRotation;
//    en_vCurrentTranslationAbsolute += penLauncher->en_vCurrentTranslationAbsolute;
    AddToMovers();
  }
  export void LaunchAsFreeProjectile(const FLOAT3D &vImpulseSpeedRelative, 
    CMovableEntity *penLauncher)
  {
    en_vCurrentTranslationAbsolute += vImpulseSpeedRelative*en_mRotation;
//    en_vCurrentTranslationAbsolute += penLauncher->en_vCurrentTranslationAbsolute;
//    en_fAcceleration = en_fDeceleration = 0.0f;
    AddToMovers();
  }

  /* Stop all translation */
  export void ForceStopTranslation(void) {
    en_vDesiredTranslationRelative = FLOAT3D(0.0f,0.0f,0.0f);
    en_vCurrentTranslationAbsolute = FLOAT3D(0.0f,0.0f,0.0f);
    en_vAppliedTranslation = FLOAT3D(0.0f,0.0f,0.0f);
  }

  /* Stop all rotation */
  export void ForceStopRotation(void) {
    en_aDesiredRotationRelative = ANGLE3D(0,0,0);
    en_aCurrentRotationAbsolute = ANGLE3D(0,0,0);
    en_mAppliedRotation.Diagonal(1.0f);
  }

  /* Stop at once in place */
  export void ForceFullStop(void) {
    ForceStopTranslation();
    ForceStopRotation();
  }

  /* Fake that the entity jumped (for jumppads) */
  export void FakeJump(const FLOAT3D &vOrgSpeed, const FLOAT3D &vDirection, FLOAT fStrength,
    FLOAT fParallelMultiplier, FLOAT fNormalMultiplier, FLOAT fMaxExitSpeed, TIME tmControl)
  {
    // fixup jump time for right control
    en_tmJumped = _pTimer->CurrentTick()-en_tmMaxJumpControl+tmControl;

    // apply parallel and normal component multipliers
    FLOAT3D vCurrentNormal;
    FLOAT3D vCurrentParallel;
    GetParallelAndNormalComponents(vOrgSpeed, vDirection, vCurrentParallel, vCurrentNormal);

    /*
    CPrintF( "\nCurrent translation absolute before: %g, %g, %g\n", 
      vOrgSpeed(1),
      vOrgSpeed(2),
      vOrgSpeed(3));*/

    // compile translation vector
    en_vCurrentTranslationAbsolute = 
      vCurrentParallel*fParallelMultiplier + 
      vCurrentNormal*fNormalMultiplier +
      vDirection*fStrength;

    // clamp translation speed
    FLOAT fLength = en_vCurrentTranslationAbsolute.Length();
    if( fLength > fMaxExitSpeed)
    {
      en_vCurrentTranslationAbsolute = 
        en_vCurrentTranslationAbsolute/fLength*fMaxExitSpeed;
    }

    /*CPrintF( "Current translation absolute after: %g, %g, %g\n\n", 
      en_vCurrentTranslationAbsolute(1),
      en_vCurrentTranslationAbsolute(2),
      en_vCurrentTranslationAbsolute(3));*/

    // no reference while bouncing
    en_penReference = NULL;
    en_pbpoStandOn = NULL;
    en_vReferencePlane = FLOAT3D(0.0f, 0.0f, 0.0f);
    en_iReferenceSurface = 0;

    // add to movers
    AddToMovers();
  }

  /* Get relative angles from direction angles. */
  export ANGLE GetRelativeHeading(const FLOAT3D &vDirection) {
    ASSERT(Abs(vDirection.Length()-1)<0.001f); // must be normalized!
    // get front component of vector
    FLOAT fFront = 
      -vDirection(1)*en_mRotation(1,3)
      -vDirection(2)*en_mRotation(2,3)
      -vDirection(3)*en_mRotation(3,3);
    // get left component of vector
    FLOAT fLeft = 
      -vDirection(1)*en_mRotation(1,1)
      -vDirection(2)*en_mRotation(2,1)
      -vDirection(3)*en_mRotation(3,1);
    // relative heading is arctan of angle between front and left
    return ATan2(fLeft, fFront);
  }
  export ANGLE GetRelativePitch(const FLOAT3D &vDirection) {
    ASSERT(Abs(vDirection.Length()-1)<0.001f); // must be normalized!
    // get front component of vector
    FLOAT fFront = 
      -vDirection(1)*en_mRotation(1,3)
      -vDirection(2)*en_mRotation(2,3)
      -vDirection(3)*en_mRotation(3,3);
    // get up component of vector
    FLOAT fUp = 
      +vDirection(1)*en_mRotation(1,2)
      +vDirection(2)*en_mRotation(2,2)
      +vDirection(3)*en_mRotation(3,2);
    // relative pitch is arctan of angle between front and up
    return ATan2(fUp, fFront);
  }

  /* Get absolute direction for a heading relative to another direction. */
  export void GetReferenceHeadingDirection(const FLOAT3D &vReference, ANGLE aH, FLOAT3D &vDirection) {
    ASSERT(Abs(vReference.Length()-1)<0.001f); // must be normalized!
    FLOAT3D vY(en_mRotation(1,2), en_mRotation(2,2), en_mRotation(3,2));
    FLOAT3D vX = (vY*vReference).Normalize();
    FLOAT3D vMZ = vY*vX;
    vDirection = -vX*Sin(aH)+vMZ*Cos(aH);
  }

  /* Get absolute direction for a heading relative to current direction. */
  export void GetHeadingDirection(ANGLE aH, FLOAT3D &vDirection) {
    FLOAT3D vX(en_mRotation(1,1), en_mRotation(2,1), en_mRotation(3,1));
    FLOAT3D vZ(en_mRotation(1,3), en_mRotation(2,3), en_mRotation(3,3));
    vDirection = -vX*Sin(aH)-vZ*Cos(aH);
  }

  /* Get absolute direction for a pitch relative to current direction. */
  export void GetPitchDirection(ANGLE aH, FLOAT3D &vDirection) {
    FLOAT3D vY(en_mRotation(1,2), en_mRotation(2,2), en_mRotation(3,2));
    FLOAT3D vZ(en_mRotation(1,3), en_mRotation(2,3), en_mRotation(3,3));
    vDirection = -vZ*Cos(aH)+vY*Sin(aH);
  }

  // get a valid inflictor for misc damage (last brush or this)
  CEntity *MiscDamageInflictor(void)
  {
    // NOTE: must be damaged by some brush if possible, because enemies are set up so
    // that they cannot harm themselves.
    if (en_penLastValidReference!=NULL) {
      return en_penLastValidReference;
    } else {
      CBrushSector *pbsc = GetFirstSector();
      if (pbsc==NULL) {
        return this;
      } else {
        return pbsc->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
      }
    }
  }

  // add the sector force
  void UpdateOneSectorForce(CBrushSector &bsc, FLOAT fRatio)
  {
    // if not significantly
    if (fRatio<0.01f) {
      // just ignore it
      return;
    }
    INDEX iForceType = bsc.GetForceType();
    CEntity *penEntity = bsc.bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;

/*
    BOOL bAnyIn = FALSE;
    CStaticArray<CMovingSphere> &absSpheres = en_pciCollisionInfo->ci_absSpheres;
    // for each sphere
    for(INDEX iSphere=0; iSphere<absSpheres.Count(); iSphere++) {
      CMovingSphere &ms = absSpheres[iSphere];
      // if the sphere is in field sector
      if (bsc.bsc_bspBSPTree.TestSphere(
        FLOATtoDOUBLE(ms.ms_vRelativeCenter0), ms.ms_fR)>=0) {
        bAnyIn = TRUE;
        break;
      }
    }
    if (!bAnyIn) {
      return;
    }*/

    // try to find the force in container
    CEntityForce *pef = NULL;
    for(INDEX iForce=0; iForce<_aefForces.Count(); iForce++) {
      if (penEntity ==_aefForces[iForce].ef_penEntity
        &&iForceType==_aefForces[iForce].ef_iForceType) {
        pef = &_aefForces[iForce];
        break;
      }
    }
   
    // if field is not found
    if (pef==NULL) {
      // add a new one
      pef = _aefForces.Push(1);
      pef->ef_penEntity = penEntity;
      pef->ef_iForceType = iForceType;
      pef->ef_fRatio = 0.0f;
    }
    pef->ef_fRatio+=fRatio;
  }

  // test for field containment
  void TestFields(INDEX &iUpContent, INDEX &iDnContent, FLOAT &fImmersionFactor)
  {
    // this works only for models
    ASSERT(en_RenderType==RT_MODEL || en_RenderType==RT_EDITORMODEL || en_RenderType==RT_SKAMODEL || en_RenderType==RT_SKAEDITORMODEL);
    iUpContent = 0;
    iDnContent = 0;
    FLOAT fUp = 0.0f;
    FLOAT fDn = 0.0f;

    FLOAT3D &vOffset = en_plPlacement.pl_PositionVector;
    FLOATmatrix3D &mRotation = en_mRotation;
    // project height min/max in the entity to absolute space
    FLOAT3D vMin = FLOAT3D(0, en_pciCollisionInfo->ci_fMinHeight, 0);
    FLOAT3D vMax = FLOAT3D(0, en_pciCollisionInfo->ci_fMaxHeight, 0);
    vMin = vMin*mRotation+vOffset;
    vMax = vMax*mRotation+vOffset;
    // project all spheres in the entity to absolute space (for touch field testing)
    CStaticArray<CMovingSphere> &absSpheres = en_pciCollisionInfo->ci_absSpheres;
    FOREACHINSTATICARRAY(absSpheres, CMovingSphere, itms) {
      itms->ms_vRelativeCenter0 = itms->ms_vCenter*mRotation+vOffset;
    }

    // clear forces
    _aefForces.PopAll();
    // for each sector that this entity is in
    {FOREACHSRCOFDST(en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
      CBrushSector &bsc = *pbsc;
      // if this sector is not in first mip
      if (!bsc.bsc_pbmBrushMip->IsFirstMip()) {
        // skip it
        continue;
      }
      // get entity of the sector
      CEntity *penSector = bsc.bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;

      // if not real brush
      if (penSector->en_RenderType!=RT_BRUSH) {
        // skip it
        continue;
      }

      // get min/max parameters of entity inside sector
      double dMin, dMax;
      bsc.bsc_bspBSPTree.FindLineMinMax(FLOATtoDOUBLE(vMin), FLOATtoDOUBLE(vMax), dMin, dMax);

      // if sector content is not default
      INDEX iContent = bsc.GetContentType();
      if (iContent!=0) {
        // if inside sector at all
        if (dMax>0.0f && dMin<1.0f) {
          //CPrintF("%s: %lf %lf    ", bsc.bsc_strName, dMin, dMax);
          // if minimum is small
          if (dMin<0.01f) {
            // update down content
            iDnContent = iContent;
            fDn = Max(fDn, FLOAT(dMax));
          }
          // if maximum is large
          if (dMax>0.99f) {
            // update up content
            iUpContent = iContent;
            fUp = Max(fUp, 1-FLOAT(dMin));
          }
        }
      }

      // add the sector force
      UpdateOneSectorForce(bsc, dMax-dMin);

    ENDFOR;}
    //CPrintF("%f %d %f %d\n", fDn, iDnContent, fUp, iUpContent);

    // if same contents
    if (iUpContent == iDnContent) {
      // trivial case
      fImmersionFactor = 1.0f;
    // if different contents
    } else {
      // calculate immersion factor
      if (iUpContent==0) {
        fImmersionFactor = fDn;
      } else if (iDnContent==0) {
        fImmersionFactor = 1-fUp;
      } else {
        fImmersionFactor = Max(fDn, 1-fUp);
      }
      // eliminate degenerate cases
      if (fImmersionFactor<0.01f) {
        fImmersionFactor = 1.0f;
        iDnContent = iUpContent;
      } else if (fImmersionFactor>0.99f) {
        fImmersionFactor = 1.0f;
        iUpContent = iDnContent;
      }
    }

    // clear force container and calculate average forces
    FLOAT3D vGravityA(0,0,0);
    FLOAT3D vGravityV(0,0,0);
    FLOAT3D vForceA(0,0,0);
    FLOAT3D vForceV(0,0,0);
    FLOAT fRatioSum = 0.0f;

    {for(INDEX iForce=0; iForce<_aefForces.Count(); iForce++) {
      CForceStrength fsGravity;
      CForceStrength fsField;
      _aefForces[iForce].ef_penEntity->GetForce(
        _aefForces[iForce].ef_iForceType, en_plPlacement.pl_PositionVector, 
        fsGravity, fsField);
      FLOAT fRatio = _aefForces[iForce].ef_fRatio;
      fRatioSum+=fRatio;
      vGravityA+=fsGravity.fs_vDirection*fsGravity.fs_fAcceleration*fRatio;
      vGravityV+=fsGravity.fs_vDirection*fsGravity.fs_fVelocity*fRatio;
      if (fsField.fs_fAcceleration>0) {
        vForceA+=fsField.fs_vDirection*fsField.fs_fAcceleration*fRatio;
        vForceV+=fsField.fs_vDirection*fsField.fs_fVelocity*fRatio;
      }
      _aefForces[iForce].Clear();
    }}
    if (fRatioSum>0) {
      vGravityA/=fRatioSum;
      vGravityV/=fRatioSum;
      vForceA/=fRatioSum;
      vForceV/=fRatioSum;
    }
    en_fGravityA = vGravityA.Length();
    if (en_fGravityA<0.01f) {
     en_fGravityA = 0;
    } else {
     en_fGravityV = vGravityV.Length();
     en_vGravityDir = vGravityA/en_fGravityA;
    }
    en_fForceA = vForceA.Length();
    if (en_fForceA<0.01f) {
     en_fForceA = 0;
    } else {
     en_fForceV = vForceV.Length();
     en_vForceDir = vForceA/en_fForceA;
    }
    _aefForces.PopAll();
  }

  // test entity breathing
  void TestBreathing(CContentType &ctUp) 
  {
    // if this entity doesn't breathe
    if (!(en_ulPhysicsFlags&(EPF_HASLUNGS|EPF_HASGILLS))) {
      // do nothing
      return;
    }
    // find current breathing parameters
    BOOL bCanBreathe = 
      ((ctUp.ct_ulFlags&CTF_BREATHABLE_LUNGS) && (en_ulPhysicsFlags&EPF_HASLUNGS)) ||
      ((ctUp.ct_ulFlags&CTF_BREATHABLE_GILLS) && (en_ulPhysicsFlags&EPF_HASGILLS));
    TIME tmNow = _pTimer->CurrentTick();
    TIME tmBreathDelay = tmNow-en_tmLastBreathed;
    // if entity can breathe now
    if (bCanBreathe) {
      // update breathing time
      en_tmLastBreathed = tmNow;
      // if it was without air for some time
      if (tmBreathDelay>_pTimer->TickQuantum*2) {
        // notify entity that it has take air now
        ETakingBreath eTakingBreath;
        eTakingBreath.fBreathDelay = tmBreathDelay/en_tmMaxHoldBreath;
        SendEvent(eTakingBreath);
      }
    // if entity can not breathe now
    } else {
      // if it was without air for too long
      if (tmBreathDelay>en_tmMaxHoldBreath) {
        // inflict drowning damage 
        InflictDirectDamage(this, MiscDamageInflictor(), DMT_DROWNING, ctUp.ct_fDrowningDamageAmount, 
          en_plPlacement.pl_PositionVector, -en_vGravityDir);
        // prolongue breathing a bit, so not to come here every frame
        en_tmLastBreathed = tmNow-en_tmMaxHoldBreath+ctUp.ct_tmDrowningDamageDelay;
      }
    }
  }
  void TestContentDamage(CContentType &ctDn, FLOAT fImmersion)
  {
    // if the content can damage by swimming
    if (ctDn.ct_fSwimDamageAmount>0) {
      TIME tmNow = _pTimer->CurrentTick();
      // if there is a delay
      if (ctDn.ct_tmSwimDamageDelay>0) {
        // if not yet delayed
        if (tmNow-en_tmLastSwimDamage>ctDn.ct_tmSwimDamageDelay+_pTimer->TickQuantum) {
          // delay
          en_tmLastSwimDamage = tmNow+ctDn.ct_tmSwimDamageDelay;
          return;
        }
      }

      if (tmNow-en_tmLastSwimDamage>ctDn.ct_tmSwimDamageFrequency) {
        // inflict drowning damage 
        InflictDirectDamage(this, MiscDamageInflictor(),
          (DamageType)ctDn.ct_iSwimDamageType, ctDn.ct_fSwimDamageAmount*fImmersion, 
          en_plPlacement.pl_PositionVector, -en_vGravityDir);
        en_tmLastSwimDamage = tmNow;
      }
    }
    // if the content kills
    if (ctDn.ct_fKillImmersion>0 && fImmersion>=ctDn.ct_fKillImmersion
      &&(en_ulFlags&ENF_ALIVE)) {
      // inflict killing damage 
      InflictDirectDamage(this, MiscDamageInflictor(),
        (DamageType)ctDn.ct_iKillDamageType, GetHealth()*10.0f, 
        en_plPlacement.pl_PositionVector, -en_vGravityDir);
    }
  }

  void TestSurfaceDamage(CSurfaceType &stDn)
  {
    // if the surface can damage by walking
    if (stDn.st_fWalkDamageAmount>0) {
      TIME tmNow = _pTimer->CurrentTick();
      // if there is a delay
      if (stDn.st_tmWalkDamageDelay>0) {
        // if not yet delayed
        if (tmNow-en_tmLastSwimDamage>stDn.st_tmWalkDamageDelay+_pTimer->TickQuantum) {
          // delay
          en_tmLastSwimDamage = tmNow+stDn.st_tmWalkDamageDelay;
          return;
        }
      }

      if (tmNow-en_tmLastSwimDamage>stDn.st_tmWalkDamageFrequency) {
        // inflict walking damage 
        InflictDirectDamage(this, MiscDamageInflictor(),
          (DamageType)stDn.st_iWalkDamageType, stDn.st_fWalkDamageAmount, 
          en_plPlacement.pl_PositionVector, -en_vGravityDir);
        en_tmLastSwimDamage = tmNow;
      }
    }
  }

  // send touch event to this entity and touched entity
  void SendTouchEvent(const CClipMove &cmMove)
  {
    ETouch etouchThis;
    ETouch etouchOther;
    etouchThis.penOther = cmMove.cm_penHit;
    etouchThis.bThisMoved = FALSE;
    etouchThis.plCollision = cmMove.cm_plClippedPlane;
    etouchOther.penOther = this;
    etouchOther.bThisMoved = TRUE;
    etouchOther.plCollision = cmMove.cm_plClippedPlane;
    SendEvent(etouchThis);
    cmMove.cm_penHit->SendEvent(etouchOther);
  }

  // send block event to this entity
  void SendBlockEvent(CClipMove &cmMove)
  {
    EBlock eBlock;
    eBlock.penOther = cmMove.cm_penHit;
    eBlock.plCollision = cmMove.cm_plClippedPlane;
    SendEvent(eBlock);
  }

  BOOL IsStandingOnPolygon(CBrushPolygon *pbpo)
  {
    _pfPhysicsProfile.StartTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
    // if cannot optimize for standing on handle
    if (en_pciCollisionInfo==NULL 
      ||!(en_pciCollisionInfo->ci_ulFlags&CIF_CANSTANDONHANDLE)) {
      // not standing on polygon
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }

    // if polygon is not valid for standing on any more (brush turned off collision)
    if (en_pbpoStandOn->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->en_ulCollisionFlags==0) {
      // not standing on polygon
      return FALSE;
    }

    const FLOATplane3D &plPolygon = pbpo->bpo_pbplPlane->bpl_plAbsolute;
    // get stand-on handle
    FLOAT3D vHandle = en_plPlacement.pl_PositionVector;
    vHandle(1)+=en_pciCollisionInfo->ci_fHandleY*en_mRotation(1,2);
    vHandle(2)+=en_pciCollisionInfo->ci_fHandleY*en_mRotation(2,2);
    vHandle(3)+=en_pciCollisionInfo->ci_fHandleY*en_mRotation(3,2);
    vHandle-=((FLOAT3D&)plPolygon)*en_pciCollisionInfo->ci_fHandleR;

    // if handle is not on the plane
    if (plPolygon.PointDistance(vHandle)>0.01f) {
      // not standing on polygon
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }

    // find major axes of the polygon plane
    INDEX iMajorAxis1, iMajorAxis2;
    GetMajorAxesForPlane(plPolygon, iMajorAxis1, iMajorAxis2);

    // create an intersector
    CIntersector isIntersector(vHandle(iMajorAxis1), vHandle(iMajorAxis2));
    // for all edges in the polygon
    FOREACHINSTATICARRAY(pbpo->bpo_abpePolygonEdges, CBrushPolygonEdge, itbpePolygonEdge) {
      // get edge vertices (edge direction is irrelevant here!)
      const FLOAT3D &vVertex0 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
      const FLOAT3D &vVertex1 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
      // pass the edge to the intersector
      isIntersector.AddEdge(
        vVertex0(iMajorAxis1), vVertex0(iMajorAxis2),
        vVertex1(iMajorAxis1), vVertex1(iMajorAxis2));
    }

    // if the point is inside polygon
    if (isIntersector.IsIntersecting()) {
      // entity is standing on polygon
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return TRUE;
    // if the point is outside polygon
    } else {
      // entity is not standing on polygon
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }
  }

  // check whether a polygon is below given point, but not too far away
  BOOL IsPolygonBelowPoint(CBrushPolygon *pbpo, const FLOAT3D &vPoint, FLOAT fMaxDist)
  {
    _pfPhysicsProfile.StartTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);

    // if passable or not allowed as ground
    if ((pbpo->bpo_ulFlags&BPOF_PASSABLE)
      ||!AllowForGroundPolygon(pbpo)) {
      // it cannot be below
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }

    // get polygon plane
    const FLOATplane3D &plPolygon = pbpo->bpo_pbplPlane->bpl_plAbsolute;

    // determine polygon orientation relative to gravity
    FLOAT fCos = ((const FLOAT3D &)plPolygon)%en_vGravityDir;
    // if polygon is vertical or upside down
    if (fCos>-0.01f) {
      // it cannot be below
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }

    // if polygon's steepness is too high
    CSurfaceType &stReference = en_pwoWorld->wo_astSurfaceTypes[pbpo->bpo_bppProperties.bpp_ubSurfaceType];
    if ((fCos >= -stReference.st_fClimbSlopeCos && fCos<0)
      || stReference.st_ulFlags&STF_SLIDEDOWNSLOPE) {
      // it cannot be below
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }

    // get distance from point to the plane
    FLOAT fD = plPolygon.PointDistance(vPoint);
    // if the point is behind the plane
    if (fD<-0.01f) {
      // it cannot be below
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }

    // find distance of point from the polygon along gravity vector
    FLOAT fDistance = -fD/fCos;
    // if too far away
    if (fDistance > fMaxDist) {
      // it cannot be below
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }
    // project point to the polygon along gravity vector
    FLOAT3D vProjected = vPoint + en_vGravityDir*fDistance;

    // find major axes of the polygon plane
    INDEX iMajorAxis1, iMajorAxis2;
    GetMajorAxesForPlane(plPolygon, iMajorAxis1, iMajorAxis2);

    // create an intersector
    CIntersector isIntersector(vProjected(iMajorAxis1), vProjected(iMajorAxis2));
    // for all edges in the polygon
    FOREACHINSTATICARRAY(pbpo->bpo_abpePolygonEdges, CBrushPolygonEdge, itbpePolygonEdge) {
      // get edge vertices (edge direction is irrelevant here!)
      const FLOAT3D &vVertex0 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
      const FLOAT3D &vVertex1 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
      // pass the edge to the intersector
      isIntersector.AddEdge(
        vVertex0(iMajorAxis1), vVertex0(iMajorAxis2),
        vVertex1(iMajorAxis1), vVertex1(iMajorAxis2));
    }

    // if the point is inside polygon
    if (isIntersector.IsIntersecting()) {
      // it is below
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return TRUE;
    // if the point is outside polygon
    } else {
      // it is not below
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_ISSTANDINGONPOLYGON);
      return FALSE;
    }
  }

  // override this to make filtering for what can entity stand on
  export virtual BOOL AllowForGroundPolygon(CBrushPolygon *pbpo)
  {
    return TRUE;
  }

  // check whether any cached near polygon is below given point
  BOOL IsSomeNearPolygonBelowPoint(const FLOAT3D &vPoint, FLOAT fMaxDist)
  {
    // otherwise, there is none
    return FALSE;
  }

  // check whether any polygon in a sector is below given point
  BOOL IsSomeSectorPolygonBelowPoint(CBrushSector *pbsc, const FLOAT3D &vPoint, FLOAT fMaxDist)
  {
    // for each polygon in the sector
    FOREACHINSTATICARRAY(pbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
      CBrushPolygon *pbpo = itbpo;
      // if it is below
      if (IsPolygonBelowPoint(pbpo, vPoint, fMaxDist)) {
        // there is some
        return TRUE;
      }
    }
    // otherwise, there is none
    return FALSE;
  }

  // check whether entity would fall if standing on next position
  BOOL WouldFallInNextPosition(void)
  {
    // if entity doesn't care for falling
    if (en_fStepDnHeight<0) {
      // don't check
      return FALSE;
    }
  
    // if the stand-on polygon is near below
    if (en_pbpoStandOn!=NULL &&
      IsPolygonBelowPoint(en_pbpoStandOn, en_vNextPosition, en_fStepDnHeight)) {
      // it won't fall
      return FALSE;
    }

    // make empty list of extra sectors to check
    CListHead lhActiveSectors;

    CStaticStackArray<CBrushPolygon *> &apbpo = en_apbpoNearPolygons;
    // for each cached near polygon
    for(INDEX iPolygon=0; iPolygon<apbpo.Count(); iPolygon++) {
      CBrushPolygon *pbpo = apbpo[iPolygon];
      // if it is below
      if (IsPolygonBelowPoint(pbpo, en_vNextPosition, en_fStepDnHeight)) {
        // it won't fall
        lhActiveSectors.RemAll();
        return FALSE;
      }
      // if the polygon's sector is not added yet
      if (!pbpo->bpo_pbscSector->bsc_lnInActiveSectors.IsLinked()) {
        // add it
        lhActiveSectors.AddTail(pbpo->bpo_pbscSector->bsc_lnInActiveSectors);
      }
    }

    // NOTE: We add non-zoning reference first (if existing),
    // to speed up cases when standing on moving brushes.
    // if the reference is a non-zoning brush
    if (en_penReference!=NULL && en_penReference->en_RenderType==RT_BRUSH
      &&!(en_penReference->en_ulFlags&ENF_ZONING)
      && en_penReference->en_pbrBrush!=NULL) {
      // get first mip of the brush
      CBrushMip *pbmMip = en_penReference->en_pbrBrush->GetFirstMip();
      // for each sector in the brush mip
      FOREACHINDYNAMICARRAY(pbmMip->bm_abscSectors, CBrushSector, itbsc) {
        // if it is not added yet
        if (!itbsc->bsc_lnInActiveSectors.IsLinked()) {
          // add it
          lhActiveSectors.AddTail(itbsc->bsc_lnInActiveSectors);
        }
      }
    }

    // for each zoning sector that this entity is in
    {FOREACHSRCOFDST(en_rdSectors, CBrushSector, bsc_rsEntities, pbsc);
      // if it is not added yet
      if (!pbsc->bsc_lnInActiveSectors.IsLinked()) {
        // add it
        lhActiveSectors.AddTail(pbsc->bsc_lnInActiveSectors);
      }
    ENDFOR;}

    // for each active sector
    BOOL bSupportFound = FALSE;
    FOREACHINLIST(CBrushSector, bsc_lnInActiveSectors, lhActiveSectors, itbsc) {
      CBrushSector *pbsc = itbsc;
      // if the sector is zoning
      if (pbsc->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->en_ulFlags&ENF_ZONING) {
        // for non-zoning brush entities in the sector
        {FOREACHDSTOFSRC(pbsc->bsc_rsEntities, CEntity, en_rdSectors, pen);
          if (pen->en_RenderType==CEntity::RT_TERRAIN) {
            if (IsTerrainBelowPoint(pen->en_ptrTerrain, en_vNextPosition, en_fStepDnHeight, en_vGravityDir)) {
              bSupportFound = TRUE;
              goto out;
            }
            continue;
          }
          if (pen->en_RenderType!=CEntity::RT_BRUSH&&
              pen->en_RenderType!=CEntity::RT_FIELDBRUSH) {
            break;  // brushes are sorted first in list
          }
          // get first mip of the brush
          CBrushMip *pbmMip = pen->en_pbrBrush->GetFirstMip();
          // for each sector in the brush mip
          FOREACHINDYNAMICARRAY(pbmMip->bm_abscSectors, CBrushSector, itbscInMip) {
            // if it is not added yet
            if (!itbscInMip->bsc_lnInActiveSectors.IsLinked()) {
              // add it
              lhActiveSectors.AddTail(itbscInMip->bsc_lnInActiveSectors);
            }
          }
        ENDFOR;}
      }
      // if there is a polygon below in that sector
      if (IsSomeSectorPolygonBelowPoint(itbsc, en_vNextPosition, en_fStepDnHeight)) {
        // it won't fall
        bSupportFound = TRUE;
        break;
      }
    }
out:;

    // clear list of active sectors
    lhActiveSectors.RemAll();

    // if no support, it surely would fall
    return !bSupportFound;
  }

  // clear next position to current placement
  void ClearNextPosition(void)
  {
    en_vNextPosition = en_plPlacement.pl_PositionVector;
    en_mNextRotation = en_mRotation;
  }
  // set current placement from next position
  void SetPlacementFromNextPosition(void)
  {
    _pfPhysicsProfile.StartTimer((INDEX) CPhysicsProfile::PTI_SETPLACEMENTFROMNEXTPOSITION);

    _pfPhysicsProfile.IncrementTimerAveragingCounter((INDEX) CPhysicsProfile::PTI_SETPLACEMENTFROMNEXTPOSITION);
    CPlacement3D plNew;
    plNew.pl_PositionVector = en_vNextPosition;
    DecomposeRotationMatrixNoSnap(plNew.pl_OrientationAngle, en_mNextRotation);
    FLOATmatrix3D mRotation;
    MakeRotationMatrixFast(mRotation, plNew.pl_OrientationAngle);
    SetPlacement_internal(plNew, mRotation, TRUE /* try to optimize for small movements */);

    // use this for collision code debugging only not useful in real conditions
/*
    // check that we have not ended inside a wall
    if (GetRenderType()==RT_MODEL) {
      extern BOOL CanEntityChangeCollisionBox(CEntity *pen, INDEX iNewCollisionBox, CEntity **ppenObstacle);
      CEntity *penObstacle;
      if (!CanEntityChangeCollisionBox(this, GetCollisionBoxIndex(), &penObstacle)) {
          CPrintF("*** COLLISION!!!!\n");
      }
    }
    */

    _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_SETPLACEMENTFROMNEXTPOSITION);
  }

  BOOL TryToGoUpstairs(const FLOAT3D &vTranslationAbsolute, const CSurfaceType &stHit,
    BOOL bHitStairsOrg)
  {
    _pfPhysicsProfile.StartTimer((INDEX) CPhysicsProfile::PTI_TRYTOGOUPSTAIRS);
    _pfPhysicsProfile.IncrementTimerAveragingCounter((INDEX) CPhysicsProfile::PTI_TRYTOGOUPSTAIRS);

    // use only horizontal components of the movement
    FLOAT3D vTranslationHorizontal;
    GetNormalComponent(vTranslationAbsolute, en_vGravityDir, vTranslationHorizontal);

    //CPrintF("Trying: (%g) ", vTranslationHorizontal(3));
    // if the movement has no substantial value
    if(vTranslationHorizontal.Length()<0.001f) {
      //CPrintF("no value\n");
      // don't do it
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOGOUPSTAIRS);
      return FALSE;
    }
    FLOAT3D vTranslationHorizontalOrg = vTranslationHorizontal;
    // if the surface that is climbed on is not really stairs
    if (!bHitStairsOrg) {
      // keep minimum speed
      vTranslationHorizontal.Normalize();
      vTranslationHorizontal*=0.5f;
    }

    // remember original placement
    CPlacement3D plOriginal = en_plPlacement;

    // take stairs height
    FLOAT fStairsHeight = 0;
    if (stHit.st_fStairsHeight>0) {
      fStairsHeight = Max(stHit.st_fStairsHeight, en_fStepUpHeight);
    } else if (stHit.st_fStairsHeight<0) {
      fStairsHeight = Min(stHit.st_fStairsHeight, en_fStepUpHeight);
    }

    CContentType &ctDn = en_pwoWorld->wo_actContentTypes[en_iDnContent];
    CContentType &ctUp = en_pwoWorld->wo_actContentTypes[en_iUpContent];

    // if in partially in water
    BOOL bGettingOutOfWater = FALSE;
    if ((ctDn.ct_ulFlags&CTF_SWIMABLE) && !(ctUp.ct_ulFlags&CTF_SWIMABLE)
      && en_fImmersionFactor>0.3f) {
      // add immersion height to up step
      if (en_pciCollisionInfo!=NULL) {
        fStairsHeight=fStairsHeight*2+en_fImmersionFactor*
          (en_pciCollisionInfo->ci_fMaxHeight-en_pciCollisionInfo->ci_fMinHeight);
        // remember that we are trying to get out of water
        bGettingOutOfWater = TRUE;
      }
    }

    // calculate the 3 translation directions (up, forward and down)
    FLOAT3D avTranslation[3];
    avTranslation[0] = en_vGravityDir*-fStairsHeight;
    avTranslation[1] = vTranslationHorizontal;
    avTranslation[2] = en_vGravityDir*fStairsHeight;

    // for each translation step
    for(INDEX iStep=0; iStep<3; iStep++) {
      BOOL bStepOK = TRUE;
      // create new placement with the translation step
      en_vNextPosition = en_plPlacement.pl_PositionVector+avTranslation[iStep];
      en_mNextRotation = en_mRotation;
      // clip the movement to the entity's world
      CClipMove cm(this);
      en_pwoWorld->ClipMove(cm);

      // if not passed
      if (cm.cm_fMovementFraction<1.0f) {
        // find hit surface
        INDEX iSurfaceHit = 0;
        BOOL bHitStairsNow = FALSE;
        if (cm.cm_pbpoHit!=NULL) {
          bHitStairsNow = cm.cm_pbpoHit->bpo_ulFlags&BPOF_STAIRS;
          iSurfaceHit = cm.cm_pbpoHit->bpo_bppProperties.bpp_ubSurfaceType;
        }
        CSurfaceType &stHit = en_pwoWorld->wo_astSurfaceTypes[iSurfaceHit];


        // check if hit a slope while climbing stairs
        const FLOAT3D &vHitPlane = cm.cm_plClippedPlane;
        FLOAT fPlaneDotG = vHitPlane%en_vGravityDir;
        FLOAT fPlaneDotGAbs = Abs(fPlaneDotG);

        BOOL bSlidingAllowed = (fPlaneDotGAbs>-0.01f && fPlaneDotGAbs<0.99f)&&bHitStairsOrg;

        BOOL bEarlyClipAllowed = 
          // going up or
          iStep==0 || 
          // going forward and hit stairs or
          (iStep==1 && bHitStairsNow) || 
          // going down and ends on something that is not high slope
          (iStep==2 && 
            (vHitPlane%en_vGravityDir<-stHit.st_fClimbSlopeCos ||
             bHitStairsNow));

        // if early clip is allowed
        if (bEarlyClipAllowed || bSlidingAllowed) {
          // try to go to where it is clipped (little bit before)
          en_vNextPosition = en_plPlacement.pl_PositionVector +
            avTranslation[iStep]*(cm.cm_fMovementFraction*0.98f);
          if (bSlidingAllowed && iStep!=2) {
            FLOAT3D vSliding = cm.cm_plClippedPlane.ProjectDirection(
                  avTranslation[iStep]*(1.0f-cm.cm_fMovementFraction))+
            vHitPlane*(ClampUp(avTranslation[iStep].Length(), 0.5f)/100.0f);
            en_vNextPosition += vSliding;
          }
          CClipMove cm(this);
          en_pwoWorld->ClipMove(cm);
          // if it failed
          if (cm.cm_fMovementFraction<=1.0f) {
            // mark that this step is unsuccessful
            bStepOK = FALSE;
          }
        // if early clip is not allowed
        } else {
          // mark that this step is unsuccessful
          bStepOK = FALSE;
        }
      }

      // if the step is successful
      if (bStepOK) {
        // use that position
        SetPlacementFromNextPosition();
      // if the step failed
      } else {
        // restore original placement
        en_vNextPosition = plOriginal.pl_PositionVector;
        SetPlacementFromNextPosition();
        // move is unsuccessful
        _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOGOUPSTAIRS);
        //CPrintF("FAILED\n");
        return FALSE;
      }

    } // end of steps loop

    // all steps passed, use the final position

    // NOTE: must not keep the speed when getting out of water,
    // or the player gets launched out too fast
    if (!bGettingOutOfWater) {
      en_vAppliedTranslation += vTranslationHorizontalOrg;
    }
    // move is successful
    _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOGOUPSTAIRS);
    //CPrintF("done\n");
    return TRUE;
  }

  /* Try to translate the entity. Slide, climb or push others if needed. */
  BOOL TryToMove(CMovableEntity *penPusher, BOOL bTranslate, BOOL bRotate)
  {
    //CPrintF("TryToMove(%d)\n", _ctTryToMoveCheckCounter);
    // decrement the recursion counter
    if (penPusher!=NULL) {
      _ctTryToMoveCheckCounter--;
    } else {
      _ctTryToMoveCheckCounter-=4;
    }
    // if recursing too deep
    if (_ctTryToMoveCheckCounter<0) {
      // fail the move
      return FALSE;
    }
    _pfPhysicsProfile.StartTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
    _pfPhysicsProfile.IncrementTimerAveragingCounter((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
    _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_TRYTOMOVE);

    // create new placement with movement
    if (bTranslate) {
      en_vNextPosition = en_plPlacement.pl_PositionVector+en_vMoveTranslation;
/* STREAMDUMP START
       if(GetRenderType()==RT_MODEL)
        {
          try
          {
            CTMemoryStream &strm = *GetDumpStream();
            DUMPVECTOR2("Move transl:", en_vMoveTranslation);
            DUMPVECTOR2("Next pos:", en_vNextPosition);      
          }
          catch( char *strError)
          {
            strError;
          }
        }
      STREAMDUMP END */

    } else {
      en_vNextPosition = en_plPlacement.pl_PositionVector;
    }
    if (bRotate) {
//      CPrintF("  r:???\n");
      en_mNextRotation = en_mMoveRotation*en_mRotation;
    } else {
      en_mNextRotation = en_mRotation;
    }

    // test if rotation can be ignored
    ULONG ulCIFlags = en_pciCollisionInfo->ci_ulFlags;
    BOOL bIgnoreRotation = !bRotate ||
      ((ulCIFlags&CIF_IGNOREROTATION)|| 
      ( (ulCIFlags&CIF_IGNOREHEADING) && 
        (en_mMoveRotation(1,2)==0&&en_mMoveRotation(2,2)==1&&en_mMoveRotation(3,2)==0) ));

    // create movement towards the new placement
    CClipMove cmMove(this);
    // clip the movement to the entity's world
    if (!bTranslate && bIgnoreRotation) {
      cmMove.cm_fMovementFraction = 2.0f;
      _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_TRYTOMOVE_FAST);
    } else {
      en_pwoWorld->ClipMove(cmMove);
    }

    // if the move passes
    if (cmMove.cm_fMovementFraction>1.0f) {
//      CPrintF("  passed\n");
      //CPrintF("%d\n", MAXCOLLISIONRETRIES-(_ctTryToMoveCheckCounter+1));
      // if entity is in walking control now, but it might fall of an edge
      if (bTranslate && en_penReference!=NULL && 
         (en_ulPhysicsFlags&EPF_TRANSLATEDBYGRAVITY) &&
         !(en_ulPhysicsFlags&(EPF_ONSTEEPSLOPE|EPF_ORIENTINGTOGRAVITY|EPF_FLOATING)) &&
         penPusher==NULL && WouldFallInNextPosition()) {
        // fail the movement
        SendEvent(EWouldFall());
        //CPrintF("  wouldfall\n");
        return FALSE;
      }
      // make entity use its new placement
      SetPlacementFromNextPosition();
      if (bTranslate) {
        en_vAppliedTranslation += en_vMoveTranslation;
      }
      if (bRotate) {
        en_mAppliedRotation = en_mMoveRotation*en_mAppliedRotation;
      }
      // move is successful
      _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_TRYTOMOVE_PASS);
      _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
      //CPrintF("  successful\n");
      return TRUE;

    // if the move is clipped
    } else {
      _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_TRYTOMOVE_CLIP);

      /* STREAMDUMP START
        if(GetRenderType()==RT_MODEL)
        {
          try
          {
            CTMemoryStream &strm = *GetDumpStream();
            strm.FPrintF_t( "Movm Fract: %g\n", cmMove.cm_fMovementFraction);
            strm.FPrintF_t( "En : ID:%08x %s\n", cmMove.cm_penHit->en_ulID, (const char*) GetName() );
            DUMPVECTOR2("Pl col:", cmMove.cm_plClippedPlane);
          }
          catch( char *strError)
          {
            strError;
          }
        }
      STREAMDUMP END */

      // if must not retry
      if (_ctTryToMoveCheckCounter<=0) {
        // fail
        _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
        return FALSE;
      }

      // if hit brush
      if (cmMove.cm_pbpoHit!=NULL) {
        // if polygon is stairs, and the entity can climb stairs
        if ((cmMove.cm_pbpoHit->bpo_ulFlags&BPOF_STAIRS)
          &&((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_CLIMBORSLIDE)) {
          // adjust against sliding upwards
          cmMove.cm_plClippedPlane = FLOATplane3D(-en_vGravityDir, 0);
        }
        // if cannot be damaged by impact
        INDEX iSurface = cmMove.cm_pbpoHit->bpo_bppProperties.bpp_ubSurfaceType;
        if (en_pwoWorld->wo_astSurfaceTypes[iSurface].st_ulFlags&STF_NOIMPACT) {
          // remember that
          en_ulPhysicsFlags|=EPF_NOIMPACTTHISTICK;
        }
      }

      // if entity is translated by gravity and 
      // the hit plane is more orthogonal to the gravity than the last one found
      if ((en_ulPhysicsFlags&EPF_TRANSLATEDBYGRAVITY) && !(en_ulPhysicsFlags&EPF_FLOATING)
       && (
       ((en_vGravityDir%(FLOAT3D&)cmMove.cm_plClippedPlane)
       <(en_vGravityDir%en_vReferencePlane)) ) ) {
        // remember touched entity as stand-on reference
        en_penReference = cmMove.cm_penHit;
//        CPrintF("    newreference id%08x\n", en_penReference->en_ulID);
        en_vReferencePlane = (FLOAT3D&)cmMove.cm_plClippedPlane;
        en_pbpoStandOn = cmMove.cm_pbpoHit;  // is NULL if not hit a brush
        if (cmMove.cm_pbpoHit==NULL) {
          en_iReferenceSurface = 0;
        } else {
          en_iReferenceSurface = cmMove.cm_pbpoHit->bpo_bppProperties.bpp_ubSurfaceType;
        }
      }

      // send touch event to this entity and to touched entity
      SendTouchEvent(cmMove);

      // if cannot be damaged by impact
      if (cmMove.cm_penHit->en_ulPhysicsFlags&EPF_NOIMPACT) {
        // remember that
        en_ulPhysicsFlags|=EPF_NOIMPACTTHISTICK;
      }

      // if entity bounces when blocked
      FLOAT3D vBounce;
      BOOL bBounce = FALSE;
      if ( ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_BOUNCE) && bTranslate) {
        // create translation speed for bouncing off clipping plane
        FLOAT3D vParallel, vNormal;
        GetParallelAndNormalComponents(en_vMoveTranslation, cmMove.cm_plClippedPlane, 
          vNormal, vParallel);
        vNormal   *= -en_fBounceDampNormal;
        vParallel *= +en_fBounceDampParallel;
        vBounce = vNormal+vParallel;
        // if not too small bounce
        if (vNormal.Length()>0.1f) {
          // do bounce
          bBounce = TRUE;
        }
        // rotate slower
        en_aDesiredRotationRelative *= en_fBounceDampParallel;
        if (en_aDesiredRotationRelative.Length()<10) {
          en_aDesiredRotationRelative = ANGLE3D(0,0,0);
        }
      }
      
      // if entity pushes when blocked and the blocking entity is pushable
      if (penPusher!=NULL&&(cmMove.cm_penHit->en_ulPhysicsFlags&EPF_PUSHABLE)) {
        CMovableModelEntity *penBlocking = ((CMovableModelEntity *)cmMove.cm_penHit);
        // create push translation to account for rotating radius
        FLOAT3D vRadius = cmMove.cm_penHit->en_plPlacement.pl_PositionVector-
                                 penPusher->en_plPlacement.pl_PositionVector;
        FLOAT3D vPush=(vRadius*penPusher->en_mMoveRotation-vRadius);
          //*(1.01f-cmMove.cm_fMovementFraction);
        vPush += penPusher->en_vMoveTranslation;
          //*(1.01f-cmMove.cm_fMovementFraction);

        penBlocking->en_vMoveTranslation = vPush;
        penBlocking->en_mMoveRotation = penPusher->en_mMoveRotation;

        // make sure it is added to the movers list
        penBlocking->AddToMoversDuringMoving();
        // push the blocking entity
        _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
        BOOL bUnblocked = penBlocking->TryToMove(penPusher, bTranslate, bRotate);
        _pfPhysicsProfile.StartTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
        // if it has removed itself
        if (bUnblocked) {
          // retry the movement
          ClearNextPosition();
          _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
          return TryToMove(penPusher, bTranslate, bRotate);
        } else {
          // move is unsuccessful
          SendBlockEvent(cmMove);
          ClearNextPosition();
          _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
          return FALSE;
        }
      // if entity slides if blocked
      } else if (
        ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_SLIDE)||
        ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_BOUNCE)||
        ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_CLIMBORSLIDE)||
        ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_STOPEXACT) ){

        // if translating
        if (bTranslate) {
  
          // create translation for sliding along clipping plane
          FLOAT3D vSliding;
          // if sliding along one plane
          if (_ctSliding==0) {
            // remember sliding parameters from the plane
            _vSlideOffDir = cmMove.cm_plClippedPlane;
            // get sliding velocity
            vSliding = cmMove.cm_plClippedPlane.ProjectDirection(
                en_vMoveTranslation*(1.0f-cmMove.cm_fMovementFraction));
            _ctSliding++;
          // if second plane
          } else if (_ctSliding==1) {
            // off direction is away from both planes
            _vSlideOffDir+=cmMove.cm_plClippedPlane;
            // sliding direction is along both planes
            _vSlideDir = _vSlideOffDir*(FLOAT3D&)cmMove.cm_plClippedPlane;
            if (_vSlideDir.Length()>0.001f) {
              _vSlideDir.Normalize();
            }
            _ctSliding++;
            // get sliding velocity
            GetParallelComponent(en_vMoveTranslation*(1.0f-cmMove.cm_fMovementFraction),
              _vSlideDir, vSliding);
          // if more than two planes
          } else {
            // off direction is away from all planes
            _vSlideOffDir+=cmMove.cm_plClippedPlane;
            // sliding direction is along all planes
            _vSlideDir = cmMove.cm_plClippedPlane.ProjectDirection(_vSlideDir);
            _ctSliding++;
            // get sliding velocity
            GetParallelComponent(en_vMoveTranslation*(1.0f-cmMove.cm_fMovementFraction),
              _vSlideDir, vSliding);
          }
          ASSERT(IsValidFloat(vSliding(1)));
          ASSERT(IsValidFloat(_vSlideDir(1)));
          ASSERT(IsValidFloat(_vSlideOffDir(1)));

          // if entity hit a brush polygon
          if (cmMove.cm_pbpoHit!=NULL) {
            CSurfaceType &stHit = en_pwoWorld->wo_astSurfaceTypes[
              cmMove.cm_pbpoHit->bpo_bppProperties.bpp_ubSurfaceType];
            // if it is not being pushed, and it can climb stairs
            if (penPusher==NULL
             &&(en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_CLIMBORSLIDE) {
              // NOTE: originally, the polygon's plane was considered here.
              // due to sphere-polygon collision algo, it is possible for the collision
              // plane to be even orthogonal to the polygon plane.
              // considering polygon's plane prevented climbing up the stairs.
              // so now, the collision plane is considered.
              // if there are any further problems, i recommend choosing
              // the plane that is more orthogonal to the movement direction.
              FLOAT3D &vHitPlane = (FLOAT3D&)cmMove.cm_plClippedPlane;//cmMove.cm_pbpoHit->bpo_pbplPlane->bpl_plAbsolute;
              BOOL bHitStairs = cmMove.cm_pbpoHit->bpo_ulFlags&BPOF_STAIRS;
              // if the plane hit is steep enough to climb on it 
              // (cannot climb low slopes as if those were stairs)
              if ((vHitPlane%en_vGravityDir>-stHit.st_fClimbSlopeCos)
                ||bHitStairs) {
                // if sliding along it would be mostly horizontal 
                // (i.e. cannot climb up the slope)
                FLOAT fSlidingVertical2 = en_vMoveTranslation%en_vGravityDir;
                fSlidingVertical2*=fSlidingVertical2;
                FLOAT fSliding2 = en_vMoveTranslation%en_vMoveTranslation;
                if ((2*fSlidingVertical2<=fSliding2)
                // if can go upstairs
                  && TryToGoUpstairs(en_vMoveTranslation, stHit, bHitStairs)) {
                  // movement is ok
                  _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
                  return FALSE;
                }
              }
            }
          }
          // entity shouldn't really slide
          if ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_STOPEXACT) {
            // kill sliding
            vSliding = FLOAT3D(0,0,0);
          }

          ASSERT(IsValidFloat(vSliding(1)));

          // add a component perpendicular to the sliding plane
          vSliding += _vSlideOffDir*
            (ClampUp(en_vMoveTranslation.Length(), 0.5f)/100.0f);

          // if initial movement has some substantial value
          if(en_vMoveTranslation.Length()>0.001f && cmMove.cm_fMovementFraction>0.002f) {
            // go to where it is clipped (little bit before)
            vSliding += en_vMoveTranslation*(cmMove.cm_fMovementFraction*0.98f);
          }

          // ignore extremely small sliding
          if (vSliding.ManhattanNorm()<0.001f) {
            _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
            return FALSE;
          }

          // recurse
          en_vMoveTranslation = vSliding;
          ClearNextPosition();
          _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
          TryToMove(penPusher, bTranslate, bRotate);
          // if bouncer
          if (bBounce) {
            // remember bouncing speed for next tick
            en_vAppliedTranslation = vBounce;
            // no reference while bouncing
            en_penReference = NULL;
            en_vReferencePlane = FLOAT3D(0.0f, 0.0f, 0.0f);
            en_iReferenceSurface = 0;
          }

          // move is not entirely successful
          return FALSE;

        // if rotating
        } else if (bRotate) {
          // if bouncing entity
          if ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_BOUNCE) {
            // rotate slower
            en_aDesiredRotationRelative *= en_fBounceDampParallel;
            if (en_aDesiredRotationRelative.Length()<10) {
              en_aDesiredRotationRelative = ANGLE3D(0,0,0);
            }
            _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
            // move is not successful
            return FALSE;
          }
          // create movement getting away from the collision point
          en_vMoveTranslation = cmMove.cm_vClippedLine*-1.2f;
          // recurse
          ClearNextPosition();
          _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
          TryToMove(penPusher, TRUE, bRotate);
          // move is not entirely successful
          return FALSE;
        }
        // not translating and not rotating? -  move is unsuccessful
        _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
        return FALSE;

      // if entity has some other behaviour when blocked
      } else {
        // move is unsuccessful (EPF_ONBLOCK_STOP is assumed)
        SendBlockEvent(cmMove);
        ClearNextPosition();
        _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_TRYTOTRANSLATE);
        return FALSE;
      }
    }
  }


  /* STREAMDUMP START
    void ExportEntityPlacementAndSpeed(CMovableEntity &en, CTString strDes)
    {
      try
      {
        CTMemoryStream &strm = *GetDumpStream();
        strm.FPrintF_t("%s : %s .................\n", strDes, (const char*) GetName());
        DUMPPLACEMENT("En pl:", en.en_plPlacement);
        DUMPVECTOR2("DesTraRel:", en.en_vDesiredTranslationRelative);
        DUMPVECTOR2("CurTraAbs:", en.en_vCurrentTranslationAbsolute);
        DUMPVECTOR2("IntendTra:", en.en_vIntendedTranslation);
      }
      catch( char *strError)
      {
        strError;
      }
    }
  STREAMDUMP END */

  // clear eventual temporary variables that are not persistent
  export void ClearMovingTemp(void)
  {
//    return;
//    CLEARMEM(en_vIntendedTranslation);
//    CLEARMEM(en_mIntendedRotation);
      ClearNextPosition();
    CLEARMEM(en_vMoveTranslation);
    CLEARMEM(en_mMoveRotation);
    CLEARMEM(en_vAppliedTranslation);
    CLEARMEM(en_mAppliedRotation);
  }

  // prepare parameters for moving in this tick
  export void PreMoving(void)
  {
    //
    // NOTE:
    // This is one more cludge for bad VC6 code generator creating sync errors in older demos.
    // We are getting rid of the whole virtual actions system ASAP when the network is recoded.
    //
    // FOR LICENSEES: Since you don't need to replay old demos from SSam, you may remove the whole
    // PreMovingOld() function.
    //
    if (_pNetwork->ga_ulDemoMinorVersion<=5) {
      PreMovingOld();
    } else {
      PreMovingNew();
    }
  }
  void PreMovingNew(void)
  {
    if (en_pciCollisionInfo==NULL) {
      return;
    }

    /* STREAMDUMP START
      ExportEntityPlacementAndSpeed( *(CMovableEntity *)this, "Pre moving (start of function)");
    STREAMDUMP END */

    _pfPhysicsProfile.StartTimer((INDEX) CPhysicsProfile::PTI_PREMOVING);
    _pfPhysicsProfile.IncrementTimerAveragingCounter((INDEX) CPhysicsProfile::PTI_PREMOVING);

    // remember old placement for lerping
    en_plLastPlacement = en_plPlacement;

    // for each child of the mover
    {FOREACHINLIST(CEntity, en_lnInParent, en_lhChildren, itenChild) {
      // if the child is movable, yet not in movers list
      if ((itenChild->en_ulPhysicsFlags&EPF_MOVABLE)
        &&!((CMovableEntity*)&*itenChild)->en_lnInMovers.IsLinked()) {
        CMovableEntity *penChild = ((CMovableEntity*)&*itenChild);
        // remember old placement for lerping
        penChild->en_plLastPlacement = penChild->en_plPlacement;  
      }
    }}

    FLOAT fTickQuantum=_pTimer->TickQuantum; // used for normalizing from SI units to game ticks

    // trig break point if required
    if( dbg_bBreak) {
      dbg_bBreak = FALSE; // auto turn off breakpoint when triggered
      try { 
        Breakpoint();
      } catch (ANYEXCEPTION) {
        CPrintF("Breakpoint!\n");
      };
    }

    // NOTE: this limits maximum velocity of any entity in game.
    // it is absolutely neccessary in order to prevent extreme slowdowns in physics.
    // if you plan to increase this one radically, consider decreasing 
    // collision grid cell size!
    // currently limited to a bit less than speed of sound (not that it is any specificaly
    // relevant constant, but it is just handy)
    const FLOAT fMaxSpeed = 300.0f;
    en_vCurrentTranslationAbsolute(1)=Clamp(en_vCurrentTranslationAbsolute(1), -fMaxSpeed, +fMaxSpeed);
    en_vCurrentTranslationAbsolute(2)=Clamp(en_vCurrentTranslationAbsolute(2), -fMaxSpeed, +fMaxSpeed);
    en_vCurrentTranslationAbsolute(3)=Clamp(en_vCurrentTranslationAbsolute(3), -fMaxSpeed, +fMaxSpeed);

    // if the entity is a model
    if (en_RenderType==RT_MODEL || en_RenderType==RT_EDITORMODEL ||
        en_RenderType==RT_SKAMODEL || en_RenderType==RT_SKAEDITORMODEL) {
      // test for field containment
      TestFields(en_iUpContent, en_iDnContent, en_fImmersionFactor);
      // if entity has sticky feet
      if (en_ulPhysicsFlags & EPF_STICKYFEET) {
        // find gravity towards nearest polygon
        FLOAT3D vPoint;
        FLOATplane3D plPlane;
        FLOAT fDistanceToEdge;
        if (GetNearestPolygon(vPoint, plPlane, fDistanceToEdge)) {
          en_vGravityDir = -(FLOAT3D&)plPlane;
        }
      }
    }
    CContentType &ctDn = en_pwoWorld->wo_actContentTypes[en_iDnContent];
    CContentType &ctUp = en_pwoWorld->wo_actContentTypes[en_iUpContent];

    // test entity breathing
    TestBreathing(ctUp);
    // test content damage
    TestContentDamage(ctDn, en_fImmersionFactor);
    // test surface damage
    if (en_penReference!=NULL) {
      CSurfaceType &stReference = en_pwoWorld->wo_astSurfaceTypes[en_iReferenceSurface];
      TestSurfaceDamage(stReference);
    }
   
    // calculate content fluid factors
    FLOAT fBouyancy = (1-
      (ctDn.ct_fDensity/en_fDensity)*en_fImmersionFactor-
      (ctUp.ct_fDensity/en_fDensity)*(1-en_fImmersionFactor));
    FLOAT fSpeedModifier = 
      ctDn.ct_fSpeedMultiplier*en_fImmersionFactor+
      ctUp.ct_fSpeedMultiplier*(1-en_fImmersionFactor);
    FLOAT fFluidFriction =
      ctDn.ct_fFluidFriction*en_fImmersionFactor+
      ctUp.ct_fFluidFriction*(1-en_fImmersionFactor);
    FLOAT fControlMultiplier =
      ctDn.ct_fControlMultiplier*en_fImmersionFactor+
      ctUp.ct_fControlMultiplier*(1-en_fImmersionFactor);

    // transform relative desired translation into absolute
    FLOAT3D vDesiredTranslationAbsolute = en_vDesiredTranslationRelative;
    // relative absolute
    if (!(en_ulPhysicsFlags & EPF_ABSOLUTETRANSLATE)) {
      vDesiredTranslationAbsolute *= en_mRotation;
    }
    // transform translation and rotation into tick time units
    vDesiredTranslationAbsolute*=fTickQuantum;
    ANGLE3D aRotationRelative;
    aRotationRelative(1) = en_aDesiredRotationRelative(1)*fTickQuantum;
    aRotationRelative(2) = en_aDesiredRotationRelative(2)*fTickQuantum;
    aRotationRelative(3) = en_aDesiredRotationRelative(3)*fTickQuantum;
    // make absolute matrix rotation from relative angle rotation
    FLOATmatrix3D mRotationAbsolute;

    if ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_PUSH) {
      FLOATmatrix3D mNewRotation;
      MakeRotationMatrixFast(mNewRotation, en_plPlacement.pl_OrientationAngle+aRotationRelative);
      mRotationAbsolute = mNewRotation*!en_mRotation;

    } else {
      MakeRotationMatrixFast(mRotationAbsolute, aRotationRelative);
      mRotationAbsolute = en_mRotation*(mRotationAbsolute*!en_mRotation);
    }

    // modify desired speed for fluid parameters
    vDesiredTranslationAbsolute*=fSpeedModifier;

    // remember jumping strength (if any)
    FLOAT fJump = -en_mRotation.GetColumn(2)%vDesiredTranslationAbsolute;

    BOOL bReferenceMovingInY = FALSE;
    BOOL bReferenceRotatingNonY = FALSE;
    // if we have a CMovableEntity for a reference entity
    if (en_penReference!=NULL && (en_penReference->en_ulPhysicsFlags&EPF_MOVABLE)) {
      CMovableEntity *penReference = (CMovableEntity *)(CEntity*)en_penReference;
      // get reference deltas for this tick
      const FLOAT3D &vReferenceTranslation = penReference->en_vIntendedTranslation;
      const FLOATmatrix3D &mReferenceRotation = penReference->en_mIntendedRotation;
      // calculate radius of this entity relative to reference
      FLOAT3D vRadius = en_plPlacement.pl_PositionVector
          -penReference->en_plPlacement.pl_PositionVector;
      FLOAT3D vReferenceDelta = vReferenceTranslation + vRadius*mReferenceRotation - vRadius;
      // add the deltas to this entity
      vDesiredTranslationAbsolute += vReferenceDelta;
      mRotationAbsolute = mReferenceRotation*mRotationAbsolute;

      // remember if reference is moving in y
      bReferenceMovingInY = (vReferenceDelta%en_vGravityDir != 0.0f);
      bReferenceRotatingNonY = ((en_vGravityDir*mReferenceRotation)%en_vGravityDir)>0.01f;
    }

    FLOAT3D vTranslationAbsolute = en_vCurrentTranslationAbsolute*fTickQuantum;

    // initially not orienting
    en_ulPhysicsFlags&=~EPF_ORIENTINGTOGRAVITY;
    // if the entity is rotated by gravity
    if (en_ulPhysicsFlags&EPF_ORIENTEDBYGRAVITY) {
      // find entity's down vector
      FLOAT3D vDown;
      vDown(1) = -en_mRotation(1,2);
      vDown(2) = -en_mRotation(2,2);
      vDown(3) = -en_mRotation(3,2);

      // find angle entities down and gravity down
      FLOAT fCos = vDown%en_vGravityDir;
      // if substantial
      if (fCos<0.99999f) {
        // mark
        en_ulPhysicsFlags|=EPF_ORIENTINGTOGRAVITY;

        // limit the angle rotation
        ANGLE a = ACos(fCos);
        if (Abs(a)>20) {
          a = 20*Sgn(a);
        }
        FLOAT fRad =RadAngle(a);

        // make rotation axis
        FLOAT3D vAxis = vDown*en_vGravityDir;
        FLOAT fLen = vAxis.Length();
        if (fLen<0.01f) {
          vAxis(1) = en_mRotation(1,3);
          vAxis(2) = en_mRotation(2,3);
          vAxis(3) = en_mRotation(3,3);
        // NOTE: must have this patch for smooth rocking on moving brushes
        // (should infact do fRad/=fLen always)
        } else if (!bReferenceRotatingNonY) {
          fRad/=fLen;
        }
        vAxis*=fRad;

        // make rotation matrix
        FLOATmatrix3D mGRotation;
        mGRotation(1,1) =  1;        mGRotation(1,2) = -vAxis(3); mGRotation(1,3) =  vAxis(2);
        mGRotation(2,1) =  vAxis(3); mGRotation(2,2) =  1;        mGRotation(2,3) = -vAxis(1);
        mGRotation(3,1) = -vAxis(2); mGRotation(3,2) =  vAxis(1); mGRotation(3,3) = 1;
        OrthonormalizeRotationMatrix(mGRotation);

        // add the gravity rotation
        mRotationAbsolute = mGRotation*mRotationAbsolute;
      }
    }

    // initially not floating
    en_ulPhysicsFlags&=~EPF_FLOATING;

    FLOAT ACC=en_fAcceleration*fTickQuantum*fTickQuantum;
    FLOAT DEC=en_fDeceleration*fTickQuantum*fTickQuantum;
    // if the entity is not affected by gravity
    if (!(en_ulPhysicsFlags&EPF_TRANSLATEDBYGRAVITY)) {
      // accellerate towards desired absolute translation
      if (en_ulPhysicsFlags&EPF_NOACCELERATION) {
        vTranslationAbsolute = vDesiredTranslationAbsolute;
      } else {
        AddAcceleration(vTranslationAbsolute, vDesiredTranslationAbsolute, 
          ACC*fControlMultiplier,
          DEC*fControlMultiplier);
      }
    // if swimming
    } else if ((fBouyancy*en_fGravityA<0.5f && (ctDn.ct_ulFlags&(CTF_SWIMABLE|CTF_FLYABLE)))) {
      // mark that
      en_ulPhysicsFlags|=EPF_FLOATING;
      // accellerate towards desired absolute translation
      if (en_ulPhysicsFlags&EPF_NOACCELERATION) {
        vTranslationAbsolute = vDesiredTranslationAbsolute;
      } else {
        AddAcceleration(vTranslationAbsolute, vDesiredTranslationAbsolute, 
          ACC*fControlMultiplier,
          DEC*fControlMultiplier);
      }

      // add gravity acceleration
      if (fBouyancy<-0.1f) {
        FLOAT fGV=en_fGravityV*fTickQuantum*fSpeedModifier;
        FLOAT fGA=(en_fGravityA*-fBouyancy)*fTickQuantum*fTickQuantum;
        AddAcceleration(vTranslationAbsolute, en_vGravityDir*-fGV, fGA, fGA);
      } else if (fBouyancy>+0.1f) {
        FLOAT fGV=en_fGravityV*fTickQuantum*fSpeedModifier;
        FLOAT fGA=(en_fGravityA*fBouyancy)*fTickQuantum*fTickQuantum;
        AddAcceleration(vTranslationAbsolute, en_vGravityDir*fGV, fGA, fGA);
      }

    // if the entity is affected by gravity
    } else {
      BOOL bGravityAlongPolygon = TRUE;
      // if there is no fixed remembered stand-on polygon or the entity is not on it anymore
      if (en_pbpoStandOn==NULL || !IsStandingOnPolygon(en_pbpoStandOn) || bReferenceMovingInY
        || (en_ulPhysicsFlags&EPF_ORIENTINGTOGRAVITY)) {
        // clear the stand on polygon
        en_pbpoStandOn=NULL;
        if (en_penReference == NULL || bReferenceMovingInY) {
          bGravityAlongPolygon = FALSE;
        }
      }

      // if gravity can cause the entity to fall
      if (!bGravityAlongPolygon) {
        _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_GRAVITY_NONTRIVIAL);

        // add gravity acceleration
        FLOAT fGV=en_fGravityV*fTickQuantum*fSpeedModifier;
        FLOAT fGA=(en_fGravityA*fBouyancy)*fTickQuantum*fTickQuantum;
        AddGAcceleration(vTranslationAbsolute, en_vGravityDir, fGA, fGV);
      // if entity can only slide down its stand-on polygon
      } else {
        _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_GRAVITY_TRIVIAL);

        // disassemble gravity to parts parallel and normal to plane
        FLOAT3D vPolygonDir = -en_vReferencePlane;
        // NOTE: normal to plane=paralel to plane normal vector!
        FLOAT3D vGParallel, vGNormal;
        GetParallelAndNormalComponents(en_vGravityDir, vPolygonDir, vGNormal, vGParallel);
        // add gravity part parallel to plane
        FLOAT fFactor = vGParallel.Length();

        if (fFactor>0.001f) {
          FLOAT fGV=en_fGravityV*fTickQuantum*fSpeedModifier;
          FLOAT fGA=(en_fGravityA*fBouyancy)*fTickQuantum*fTickQuantum;
          AddGAcceleration(vTranslationAbsolute, vGParallel/fFactor, fGA*fFactor, fGV*fFactor);
        }

        // kill your normal-to-polygon speed if towards polygon and small
        FLOAT fPolyGA = (vPolygonDir%en_vGravityDir)*en_fGravityA;
        FLOAT fYSpeed = vPolygonDir%vTranslationAbsolute;
        if (fYSpeed>0 && fYSpeed < fPolyGA) {
          vTranslationAbsolute -= vPolygonDir*fYSpeed;
        }

        // if a bouncer
        if ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_BOUNCE) {
          // rotate slower
          en_aDesiredRotationRelative *= en_fJumpControlMultiplier;
          if (en_aDesiredRotationRelative.Length()<10) {
            en_aDesiredRotationRelative = ANGLE3D(0,0,0);
          }
        }
      }

      CSurfaceType &stReference = en_pwoWorld->wo_astSurfaceTypes[en_iReferenceSurface];

      // if it has a reference entity
      if (en_penReference!=NULL) {
        FLOAT fPlaneY = (en_vGravityDir%en_vReferencePlane);
        FLOAT fPlaneYAbs = Abs(fPlaneY);
        FLOAT fFriction = stReference.st_fFriction;
        // if on a steep slope
        if ((fPlaneY>=-stReference.st_fClimbSlopeCos&&fPlaneY<0)
          ||((stReference.st_ulFlags&STF_SLIDEDOWNSLOPE)&&fPlaneY>-0.99f)) {
          en_ulPhysicsFlags|=EPF_ONSTEEPSLOPE;
          // accellerate horizontaly towards desired absolute translation
          AddAccelerationOnPlane2(
            vTranslationAbsolute, 
            vDesiredTranslationAbsolute,
            ACC*fPlaneYAbs*fPlaneYAbs*fFriction*fControlMultiplier,
            DEC*fPlaneYAbs*fPlaneYAbs*fFriction*fControlMultiplier,
            en_vReferencePlane,
            en_vGravityDir);
        // if not on a steep slope
        } else {
          en_ulPhysicsFlags&=~EPF_ONSTEEPSLOPE;
          // accellerate on plane towards desired absolute translation
          AddAccelerationOnPlane(
            vTranslationAbsolute, 
            vDesiredTranslationAbsolute,
            ACC*fPlaneYAbs*fPlaneYAbs*fFriction*fControlMultiplier,
            DEC*fPlaneYAbs*fPlaneYAbs*fFriction*fControlMultiplier,
            en_vReferencePlane);
        }
        // if wants to jump and can jump
        if (fJump<-0.01f && (fPlaneY<-stReference.st_fJumpSlopeCos
          || _pTimer->CurrentTick()>en_tmLastSignificantVerticalMovement+0.25f) ) {
          // jump
          vTranslationAbsolute += en_vGravityDir*fJump;
          en_tmJumped = _pTimer->CurrentTick();
          en_pbpoStandOn = NULL;
        }

      // if it doesn't have a reference entity
      } else {//if (en_penReference==NULL) 
        // if can control after jump
        if (_pTimer->CurrentTick()-en_tmJumped<en_tmMaxJumpControl) {
          // accellerate horizontaly, but slower
          AddAccelerationOnPlane(
            vTranslationAbsolute, 
            vDesiredTranslationAbsolute,
            ACC*fControlMultiplier*en_fJumpControlMultiplier,
            DEC*fControlMultiplier*en_fJumpControlMultiplier,
            FLOATplane3D(en_vGravityDir, 0));
        }

        // if wants to jump and can jump
        if (fJump<-0.01f && 
          _pTimer->CurrentTick()>en_tmLastSignificantVerticalMovement+0.25f) {
          // jump
          vTranslationAbsolute += en_vGravityDir*fJump;
          en_tmJumped = _pTimer->CurrentTick();
          en_pbpoStandOn = NULL;
        }
      }
    }

    // check for force-field acceleration
    // NOTE: pulled out because of a bug in VC code generator, see function comments above
    CheckAndAddGAcceleration(this, vTranslationAbsolute, fTickQuantum);

    // if there is fluid friction involved
    if (fFluidFriction>0.01f) {
      // slow down
      AddAcceleration(vTranslationAbsolute, FLOAT3D(0.0f, 0.0f, 0.0f),
        0.0f, DEC*fFluidFriction);
    }

    // if may slow down spinning
    if ( (en_ulPhysicsFlags& EPF_CANFADESPINNING) &&
      ( (ctDn.ct_ulFlags&CTF_FADESPINNING) || (ctUp.ct_ulFlags&CTF_FADESPINNING) ) ) {
      // reduce desired rotation
      en_aDesiredRotationRelative *= (1-fSpeedModifier*0.05f);
      if (en_aDesiredRotationRelative.Length()<10) {
        en_aDesiredRotationRelative = ANGLE3D(0,0,0);
      }
    }

    // discard reference entity (will be recalculated)
    if (en_pbpoStandOn==NULL && (vTranslationAbsolute.ManhattanNorm()>1E-5f || 
      en_vReferencePlane%en_vGravityDir<0.0f)) {
      en_penReference = NULL;
      en_vReferencePlane = FLOAT3D(0.0f, 0.0f, 0.0f);
      en_iReferenceSurface = 0;
    }

    en_vIntendedTranslation = vTranslationAbsolute;
    en_mIntendedRotation = mRotationAbsolute;

    //-- estimate future movements for collision caching

    // make box of the entity for its current rotation
    FLOATaabbox3D box;
    en_pciCollisionInfo->MakeBoxAtPlacement(FLOAT3D(0,0,0), en_mRotation, box);
    // if it is a light source
    {CLightSource *pls = GetLightSource();
    if (pls!=NULL && !(pls->ls_ulFlags&LSF_LENSFLAREONLY)) {
      // expand the box to be sure that it contains light range
      ASSERT(!(pls->ls_ulFlags&LSF_DIRECTIONAL));
      box |= FLOATaabbox3D(FLOAT3D(0,0,0), pls->ls_rFallOff);
    }}
    // add a bit around it
    box.ExpandByFactor( phy_fCollisionCacheAround-1.0f);
    // make box go few ticks ahead of the entity
    box += en_plPlacement.pl_PositionVector;
    en_boxMovingEstimate  = box;
    box += en_vIntendedTranslation*phy_fCollisionCacheAhead;
    en_boxMovingEstimate |= box;

    // clear applied movement to be updated during movement
    en_vAppliedTranslation = FLOAT3D(0.0f, 0.0f, 0.0f);
    en_mAppliedRotation.Diagonal(1.0f);
    _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_PREMOVING);
// STREAMDUMP     ExportEntityPlacementAndSpeed( *(CMovableEntity *)this, "Pre moving (end of function)");
  }

  // VC6 KLUDGE - do not change this function
  // See note above in PreMoving()
/* old */  void PreMovingOld(void)
/* old */  {
/* old */    if (en_pciCollisionInfo==NULL) {
/* old */      return;
/* old */    }
/* old */
/* old */    //STREAMDUMP START
/* old */    //  ExportEntityPlacementAndSpeed( *(CMovableEntity *)this, "Pre moving (start of function)");
/* old */    //STREAMDUMP END
/* old */
/* old */    _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_PREMOVING);
/* old */    _pfPhysicsProfile.IncrementTimerAveragingCounter(CPhysicsProfile::PTI_PREMOVING);
/* old */
/* old */    // remember old placement for lerping
/* old */    en_plLastPlacement = en_plPlacement;
/* old */
/* old */    // for each child of the mover
/* old */    {FOREACHINLIST(CEntity, en_lnInParent, en_lhChildren, itenChild) {
/* old */      // if the child is movable, yet not in movers list
/* old */      if ((itenChild->en_ulPhysicsFlags&EPF_MOVABLE)
/* old */        &&!((CMovableEntity*)&*itenChild)->en_lnInMovers.IsLinked()) {
/* old */        CMovableEntity *penChild = ((CMovableEntity*)&*itenChild);
/* old */        // remember old placement for lerping
/* old */        penChild->en_plLastPlacement = penChild->en_plPlacement;  
/* old */      }
/* old */    }}
/* old */
/* old */    FLOAT fTickQuantum=_pTimer->TickQuantum; // used for normalizing from SI units to game ticks
/* old */
/* old */    // trig break point if required
/* old */    if( dbg_bBreak) {
/* old */      dbg_bBreak = FALSE; // auto turn off breakpoint when triggered
/* old */      try { 
/* old */        Breakpoint();
/* old */      } catch (ANYEXCEPTION) {
/* old */        CPrintF("Breakpoint!\n");
/* old */      };
/* old */    }
/* old */
/* old */    // NOTE: this limits maximum velocity of any entity in game.
/* old */    // it is absolutely neccessary in order to prevent extreme slowdowns in physics.
/* old */    // if you plan to increase this one radically, consider decreasing 
/* old */    // collision grid cell size!
/* old */    // currently limited to a bit less than speed of sound (not that it is any specificaly
/* old */    // relevant constant, but it is just handy)
/* old */    const FLOAT fMaxSpeed = 300.0f;
/* old */    en_vCurrentTranslationAbsolute(1)=Clamp(en_vCurrentTranslationAbsolute(1), -fMaxSpeed, +fMaxSpeed);
/* old */    en_vCurrentTranslationAbsolute(2)=Clamp(en_vCurrentTranslationAbsolute(2), -fMaxSpeed, +fMaxSpeed);
/* old */    en_vCurrentTranslationAbsolute(3)=Clamp(en_vCurrentTranslationAbsolute(3), -fMaxSpeed, +fMaxSpeed);
/* old */
/* old */    // if the entity is a model
/* old */    if (en_RenderType==RT_MODEL || en_RenderType==RT_EDITORMODEL) {
/* old */      // test for field containment
/* old */      TestFields(en_iUpContent, en_iDnContent, en_fImmersionFactor);
/* old */      // if entity has sticky feet
/* old */      if (en_ulPhysicsFlags & EPF_STICKYFEET) {
/* old */        // find gravity towards nearest polygon
/* old */        FLOAT3D vPoint;
/* old */        FLOATplane3D plPlane;
/* old */        FLOAT fDistanceToEdge;
/* old */        if (GetNearestPolygon(vPoint, plPlane, fDistanceToEdge)) {
/* old */          en_vGravityDir = -(FLOAT3D&)plPlane;
/* old */        }
/* old */      }
/* old */    }
/* old */    CContentType &ctDn = en_pwoWorld->wo_actContentTypes[en_iDnContent];
/* old */    CContentType &ctUp = en_pwoWorld->wo_actContentTypes[en_iUpContent];
/* old */
/* old */    // test entity breathing
/* old */    TestBreathing(ctUp);
/* old */    // test content damage
/* old */    TestContentDamage(ctDn, en_fImmersionFactor);
/* old */    // test surface damage
/* old */    if (en_penReference!=NULL) {
/* old */      CSurfaceType &stReference = en_pwoWorld->wo_astSurfaceTypes[en_iReferenceSurface];
/* old */      TestSurfaceDamage(stReference);
/* old */    }
/* old */   
/* old */    // calculate content fluid factors
/* old */    FLOAT fBouyancy = (1-
/* old */      (ctDn.ct_fDensity/en_fDensity)*en_fImmersionFactor-
/* old */      (ctUp.ct_fDensity/en_fDensity)*(1-en_fImmersionFactor));
/* old */    FLOAT fSpeedModifier = 
/* old */      ctDn.ct_fSpeedMultiplier*en_fImmersionFactor+
/* old */      ctUp.ct_fSpeedMultiplier*(1-en_fImmersionFactor);
/* old */    FLOAT fFluidFriction =
/* old */      ctDn.ct_fFluidFriction*en_fImmersionFactor+
/* old */      ctUp.ct_fFluidFriction*(1-en_fImmersionFactor);
/* old */    FLOAT fControlMultiplier =
/* old */      ctDn.ct_fControlMultiplier*en_fImmersionFactor+
/* old */      ctUp.ct_fControlMultiplier*(1-en_fImmersionFactor);
/* old */
/* old */    // transform relative desired translation into absolute
/* old */    FLOAT3D vDesiredTranslationAbsolute = en_vDesiredTranslationRelative;
/* old */    // relative absolute
/* old */    if (!(en_ulPhysicsFlags & EPF_ABSOLUTETRANSLATE)) {
/* old */      vDesiredTranslationAbsolute *= en_mRotation;
/* old */    }
/* old */    // transform translation and rotation into tick time units
/* old */    vDesiredTranslationAbsolute*=fTickQuantum;
/* old */    ANGLE3D aRotationRelative;
/* old */    aRotationRelative(1) = en_aDesiredRotationRelative(1)*fTickQuantum;
/* old */    aRotationRelative(2) = en_aDesiredRotationRelative(2)*fTickQuantum;
/* old */    aRotationRelative(3) = en_aDesiredRotationRelative(3)*fTickQuantum;
/* old */    // make absolute matrix rotation from relative angle rotation
/* old */    FLOATmatrix3D mRotationAbsolute;
/* old */
/* old */    if ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_PUSH) {
/* old */      FLOATmatrix3D mNewRotation;
/* old */      MakeRotationMatrixFast(mNewRotation, en_plPlacement.pl_OrientationAngle+aRotationRelative);
/* old */      mRotationAbsolute = mNewRotation*!en_mRotation;
/* old */
/* old */    } else {
/* old */      MakeRotationMatrixFast(mRotationAbsolute, aRotationRelative);
/* old */      mRotationAbsolute = en_mRotation*(mRotationAbsolute*!en_mRotation);
/* old */    }
/* old */
/* old */    // modify desired speed for fluid parameters
/* old */    vDesiredTranslationAbsolute*=fSpeedModifier;
/* old */
/* old */    // remember jumping strength (if any)
/* old */    FLOAT fJump = -en_mRotation.GetColumn(2)%vDesiredTranslationAbsolute;
/* old */
/* old */    BOOL bReferenceMovingInY = FALSE;
/* old */    BOOL bReferenceRotatingNonY = FALSE;
/* old */    // if we have a CMovableEntity for a reference entity
/* old */    if (en_penReference!=NULL && (en_penReference->en_ulPhysicsFlags&EPF_MOVABLE)) {
/* old */      CMovableEntity *penReference = (CMovableEntity *)(CEntity*)en_penReference;
/* old */      // get reference deltas for this tick
/* old */      const FLOAT3D &vReferenceTranslation = penReference->en_vIntendedTranslation;
/* old */      const FLOATmatrix3D &mReferenceRotation = penReference->en_mIntendedRotation;
/* old */      // calculate radius of this entity relative to reference
/* old */      FLOAT3D vRadius = en_plPlacement.pl_PositionVector
/* old */          -penReference->en_plPlacement.pl_PositionVector;
/* old */      FLOAT3D vReferenceDelta = vReferenceTranslation + vRadius*mReferenceRotation - vRadius;
/* old */      // add the deltas to this entity
/* old */      vDesiredTranslationAbsolute += vReferenceDelta;
/* old */      mRotationAbsolute = mReferenceRotation*mRotationAbsolute;
/* old */
/* old */      // remember if reference is moving in y
/* old */      bReferenceMovingInY = (vReferenceDelta%en_vGravityDir != 0.0f);
/* old */      bReferenceRotatingNonY = ((en_vGravityDir*mReferenceRotation)%en_vGravityDir)>0.01f;
/* old */    }
/* old */
/* old */    FLOAT3D vTranslationAbsolute = en_vCurrentTranslationAbsolute*fTickQuantum;
/* old */
/* old */    // initially not orienting
/* old */    en_ulPhysicsFlags&=~EPF_ORIENTINGTOGRAVITY;
/* old */    // if the entity is rotated by gravity
/* old */    if (en_ulPhysicsFlags&EPF_ORIENTEDBYGRAVITY) {
/* old */      // find entity's down vector
/* old */      FLOAT3D vDown;
/* old */      vDown(1) = -en_mRotation(1,2);
/* old */      vDown(2) = -en_mRotation(2,2);
/* old */      vDown(3) = -en_mRotation(3,2);
/* old */
/* old */      // find angle entities down and gravity down
/* old */      FLOAT fCos = vDown%en_vGravityDir;
/* old */      // if substantial
/* old */      if (fCos<0.99999f) {
/* old */        // mark
/* old */        en_ulPhysicsFlags|=EPF_ORIENTINGTOGRAVITY;
/* old */
/* old */        // limit the angle rotation
/* old */        ANGLE a = ACos(fCos);
/* old */        if (Abs(a)>20) {
/* old */          a = 20*Sgn(a);
/* old */        }
/* old */        FLOAT fRad =RadAngle(a);
/* old */
/* old */        // make rotation axis
/* old */        FLOAT3D vAxis = vDown*en_vGravityDir;
/* old */        FLOAT fLen = vAxis.Length();
/* old */        if (fLen<0.01f) {
/* old */          vAxis(1) = en_mRotation(1,3);
/* old */          vAxis(2) = en_mRotation(2,3);
/* old */          vAxis(3) = en_mRotation(3,3);
/* old */        // NOTE: must have this patch for smooth rocking on moving brushes
/* old */        // (should infact do fRad/=fLen always)
/* old */        } else if (!bReferenceRotatingNonY) {
/* old */          fRad/=fLen;
/* old */        }
/* old */        vAxis*=fRad;
/* old */
/* old */        // make rotation matrix
/* old */        FLOATmatrix3D mGRotation;
/* old */        mGRotation(1,1) =  1;        mGRotation(1,2) = -vAxis(3); mGRotation(1,3) =  vAxis(2);
/* old */        mGRotation(2,1) =  vAxis(3); mGRotation(2,2) =  1;        mGRotation(2,3) = -vAxis(1);
/* old */        mGRotation(3,1) = -vAxis(2); mGRotation(3,2) =  vAxis(1); mGRotation(3,3) = 1;
/* old */        OrthonormalizeRotationMatrix(mGRotation);
/* old */
/* old */        // add the gravity rotation
/* old */        mRotationAbsolute = mGRotation*mRotationAbsolute;
/* old */      }
/* old */    }
/* old */
/* old */    // initially not floating
/* old */    en_ulPhysicsFlags&=~EPF_FLOATING;
/* old */
/* old */    FLOAT ACC=en_fAcceleration*fTickQuantum*fTickQuantum;
/* old */    FLOAT DEC=en_fDeceleration*fTickQuantum*fTickQuantum;
/* old */    // if the entity is not affected by gravity
/* old */    if (!(en_ulPhysicsFlags&EPF_TRANSLATEDBYGRAVITY)) {
/* old */      // accellerate towards desired absolute translation
/* old */      if (en_ulPhysicsFlags&EPF_NOACCELERATION) {
/* old */        vTranslationAbsolute = vDesiredTranslationAbsolute;
/* old */      } else {
/* old */        AddAcceleration(vTranslationAbsolute, vDesiredTranslationAbsolute, 
/* old */          ACC*fControlMultiplier,
/* old */          DEC*fControlMultiplier);
/* old */      }
/* old */    // if swimming
/* old */    } else if ((fBouyancy*en_fGravityA<0.5f && (ctDn.ct_ulFlags&(CTF_SWIMABLE|CTF_FLYABLE)))) {
/* old */      // mark that
/* old */      en_ulPhysicsFlags|=EPF_FLOATING;
/* old */      // accellerate towards desired absolute translation
/* old */      if (en_ulPhysicsFlags&EPF_NOACCELERATION) {
/* old */        vTranslationAbsolute = vDesiredTranslationAbsolute;
/* old */      } else {
/* old */        AddAcceleration(vTranslationAbsolute, vDesiredTranslationAbsolute, 
/* old */          ACC*fControlMultiplier,
/* old */          DEC*fControlMultiplier);
/* old */      }
/* old */
/* old */      // add gravity acceleration
/* old */      if (fBouyancy<-0.1f) {
/* old */        FLOAT fGV=en_fGravityV*fTickQuantum*fSpeedModifier;
/* old */        FLOAT fGA=(en_fGravityA*-fBouyancy)*fTickQuantum*fTickQuantum;
/* old */        AddAcceleration(vTranslationAbsolute, en_vGravityDir*-fGV, fGA, fGA);
/* old */      } else if (fBouyancy>+0.1f) {
/* old */        FLOAT fGV=en_fGravityV*fTickQuantum*fSpeedModifier;
/* old */        FLOAT fGA=(en_fGravityA*fBouyancy)*fTickQuantum*fTickQuantum;
/* old */        AddAcceleration(vTranslationAbsolute, en_vGravityDir*fGV, fGA, fGA);
/* old */      }
/* old */
/* old */    // if the entity is affected by gravity
/* old */    } else {
/* old */      BOOL bGravityAlongPolygon = TRUE;
/* old */      // if there is no fixed remembered stand-on polygon or the entity is not on it anymore
/* old */      if (en_pbpoStandOn==NULL || !IsStandingOnPolygon(en_pbpoStandOn) || bReferenceMovingInY
/* old */        || (en_ulPhysicsFlags&EPF_ORIENTINGTOGRAVITY)) {
/* old */        // clear the stand on polygon
/* old */        en_pbpoStandOn=NULL;
/* old */        if (en_penReference == NULL || bReferenceMovingInY) {
/* old */          bGravityAlongPolygon = FALSE;
/* old */        }
/* old */      }
/* old */
/* old */      // if gravity can cause the entity to fall
/* old */      if (!bGravityAlongPolygon) {
/* old */        _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_GRAVITY_NONTRIVIAL);
/* old */
/* old */        // add gravity acceleration
/* old */        FLOAT fGV=en_fGravityV*fTickQuantum*fSpeedModifier;
/* old */        FLOAT fGA=(en_fGravityA*fBouyancy)*fTickQuantum*fTickQuantum;
/* old */        AddGAcceleration(vTranslationAbsolute, en_vGravityDir, fGA, fGV);
/* old */      // if entity can only slide down its stand-on polygon
/* old */      } else {
/* old */        _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_GRAVITY_TRIVIAL);
/* old */
/* old */        // disassemble gravity to parts parallel and normal to plane
/* old */        FLOAT3D vPolygonDir = -en_vReferencePlane;
/* old */        // NOTE: normal to plane=paralel to plane normal vector!
/* old */        FLOAT3D vGParallel, vGNormal;
/* old */        GetParallelAndNormalComponents(en_vGravityDir, vPolygonDir, vGNormal, vGParallel);
/* old */        // add gravity part parallel to plane
/* old */        FLOAT fFactor = vGParallel.Length();
/* old */
/* old */        if (fFactor>0.001f) {
/* old */          FLOAT fGV=en_fGravityV*fTickQuantum*fSpeedModifier;
/* old */          FLOAT fGA=(en_fGravityA*fBouyancy)*fTickQuantum*fTickQuantum;
/* old */          AddGAcceleration(vTranslationAbsolute, vGParallel/fFactor, fGA*fFactor, fGV*fFactor);
/* old */        }
/* old */
/* old */        // kill your normal-to-polygon speed if towards polygon and small
/* old */        FLOAT fPolyGA = (vPolygonDir%en_vGravityDir)*en_fGravityA;
/* old */        FLOAT fYSpeed = vPolygonDir%vTranslationAbsolute;
/* old */        if (fYSpeed>0 && fYSpeed < fPolyGA) {
/* old */          vTranslationAbsolute -= vPolygonDir*fYSpeed;
/* old */        }
/* old */
/* old */        // if a bouncer
/* old */        if ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_BOUNCE) {
/* old */          // rotate slower
/* old */          en_aDesiredRotationRelative *= en_fJumpControlMultiplier;
/* old */          if (en_aDesiredRotationRelative.Length()<10) {
/* old */            en_aDesiredRotationRelative = ANGLE3D(0,0,0);
/* old */          }
/* old */        }
/* old */      }
/* old */
/* old */      CSurfaceType &stReference = en_pwoWorld->wo_astSurfaceTypes[en_iReferenceSurface];
/* old */
/* old */      // if it has a reference entity
/* old */      if (en_penReference!=NULL) {
/* old */        FLOAT fPlaneY = (en_vGravityDir%en_vReferencePlane);
/* old */        FLOAT fPlaneYAbs = Abs(fPlaneY);
/* old */        FLOAT fFriction = stReference.st_fFriction;
/* old */        // if on a steep slope
/* old */        if ((fPlaneY>=-stReference.st_fClimbSlopeCos&&fPlaneY<0)
/* old */          ||((stReference.st_ulFlags&STF_SLIDEDOWNSLOPE)&&fPlaneY>-0.99f)) {
/* old */          en_ulPhysicsFlags|=EPF_ONSTEEPSLOPE;
/* old */          // accellerate horizontaly towards desired absolute translation
/* old */          AddAccelerationOnPlane2(
/* old */            vTranslationAbsolute, 
/* old */            vDesiredTranslationAbsolute,
/* old */            ACC*fPlaneYAbs*fPlaneYAbs*fFriction*fControlMultiplier,
/* old */            DEC*fPlaneYAbs*fPlaneYAbs*fFriction*fControlMultiplier,
/* old */            en_vReferencePlane,
/* old */            en_vGravityDir);
/* old */        // if not on a steep slope
/* old */        } else {
/* old */          en_ulPhysicsFlags&=~EPF_ONSTEEPSLOPE;
/* old */          // accellerate on plane towards desired absolute translation
/* old */          AddAccelerationOnPlane(
/* old */            vTranslationAbsolute, 
/* old */            vDesiredTranslationAbsolute,
/* old */            ACC*fPlaneYAbs*fPlaneYAbs*fFriction*fControlMultiplier,
/* old */            DEC*fPlaneYAbs*fPlaneYAbs*fFriction*fControlMultiplier,
/* old */            en_vReferencePlane);
/* old */        }
/* old */        // if wants to jump and can jump
/* old */        if (fJump<-0.01f && (fPlaneY<-stReference.st_fJumpSlopeCos
/* old */          || _pTimer->CurrentTick()>en_tmLastSignificantVerticalMovement+0.25f) ) {
/* old */          // jump
/* old */          vTranslationAbsolute += en_vGravityDir*fJump;
/* old */          en_tmJumped = _pTimer->CurrentTick();
/* old */          en_pbpoStandOn = NULL;
/* old */        }
/* old */
/* old */      // if it doesn't have a reference entity
/* old */      } else {//if (en_penReference==NULL) 
/* old */        // if can control after jump
/* old */        if (_pTimer->CurrentTick()-en_tmJumped<en_tmMaxJumpControl) {
/* old */          // accellerate horizontaly, but slower
/* old */          AddAccelerationOnPlane(
/* old */            vTranslationAbsolute, 
/* old */            vDesiredTranslationAbsolute,
/* old */            ACC*fControlMultiplier*en_fJumpControlMultiplier,
/* old */            DEC*fControlMultiplier*en_fJumpControlMultiplier,
/* old */            FLOATplane3D(en_vGravityDir, 0));
/* old */        }
/* old */
/* old */        // if wants to jump and can jump
/* old */        if (fJump<-0.01f && 
/* old */          _pTimer->CurrentTick()>en_tmLastSignificantVerticalMovement+0.25f) {
/* old */          // jump
/* old */          vTranslationAbsolute += en_vGravityDir*fJump;
/* old */          en_tmJumped = _pTimer->CurrentTick();
/* old */          en_pbpoStandOn = NULL;
/* old */        }
/* old */      }
/* old */    }
/* old */
/* old */    // check for force-field acceleration
/* old */    // NOTE: pulled out because of a bug in VC code generator, see function comments above
/* old */    CheckAndAddGAcceleration(this, vTranslationAbsolute, fTickQuantum);
/* old */
/* old */    // if there is fluid friction involved
/* old */    if (fFluidFriction>0.01f) {
/* old */      // slow down
/* old */      AddAcceleration(vTranslationAbsolute, FLOAT3D(0.0f, 0.0f, 0.0f),
/* old */        0.0f, DEC*fFluidFriction);
/* old */    }
/* old */
/* old */    // if may slow down spinning
/* old */    if ( (en_ulPhysicsFlags& EPF_CANFADESPINNING) &&
/* old */      ( (ctDn.ct_ulFlags&CTF_FADESPINNING) || (ctUp.ct_ulFlags&CTF_FADESPINNING) ) ) {
/* old */      // reduce desired rotation
/* old */      en_aDesiredRotationRelative *= (1-fSpeedModifier*0.05f);
/* old */      if (en_aDesiredRotationRelative.Length()<10) {
/* old */        en_aDesiredRotationRelative = ANGLE3D(0,0,0);
/* old */      }
/* old */    }
/* old */
/* old */    // discard reference entity (will be recalculated)
/* old */    if (en_pbpoStandOn==NULL) {
/* old */      en_penReference = NULL;
/* old */      en_vReferencePlane = FLOAT3D(0.0f, 0.0f, 0.0f);
/* old */      en_iReferenceSurface = 0;
/* old */    }
/* old */
/* old */    en_vIntendedTranslation = vTranslationAbsolute;
/* old */    en_mIntendedRotation = mRotationAbsolute;
/* old */
/* old */    //-- estimate future movements for collision caching
/* old */
/* old */    // make box of the entity for its current rotation
/* old */    FLOATaabbox3D box;
/* old */    en_pciCollisionInfo->MakeBoxAtPlacement(FLOAT3D(0,0,0), en_mRotation, box);
/* old */    // if it is a light source
/* old */    {CLightSource *pls = GetLightSource();
/* old */    if (pls!=NULL && !(pls->ls_ulFlags&LSF_LENSFLAREONLY)) {
/* old */      // expand the box to be sure that it contains light range
/* old */      ASSERT(!(pls->ls_ulFlags&LSF_DIRECTIONAL));
/* old */      box |= FLOATaabbox3D(FLOAT3D(0,0,0), pls->ls_rFallOff);
/* old */    }}
/* old */    // add a bit around it
/* old */    box.ExpandByFactor( phy_fCollisionCacheAround-1.0f);
/* old */    // make box go few ticks ahead of the entity
/* old */    box += en_plPlacement.pl_PositionVector;
/* old */    en_boxMovingEstimate  = box;
/* old */    box += en_vIntendedTranslation*phy_fCollisionCacheAhead;
/* old */    en_boxMovingEstimate |= box;
/* old */
/* old */    // clear applied movement to be updated during movement
/* old */    en_vAppliedTranslation = FLOAT3D(0.0f, 0.0f, 0.0f);
/* old */    en_mAppliedRotation.Diagonal(1.0f);
/* old */    _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_PREMOVING);
/* old */// STREAMDUMP     ExportEntityPlacementAndSpeed( *(CMovableEntity *)this, "Pre moving (end of function)");
/* old */  }

  /* Calculate physics for moving. */
  export void DoMoving(void)
  {
    if (en_pciCollisionInfo==NULL || (en_ulPhysicsFlags&EPF_FORCEADDED)) {
      return;
    }
// STREAMDUMP     ExportEntityPlacementAndSpeed(*(CMovableEntity *)this, "Do moving (start of function)");

    _pfPhysicsProfile.StartTimer((INDEX) CPhysicsProfile::PTI_DOMOVING);
    _pfPhysicsProfile.IncrementTimerAveragingCounter((INDEX) CPhysicsProfile::PTI_DOMOVING);

    _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_DOMOVING);

    //FLOAT fTickQuantum=_pTimer->TickQuantum; // used for normalizing from SI units to game ticks

    // if rotation and translation are synchronized
    if (en_ulPhysicsFlags&EPF_RT_SYNCHRONIZED) {
      _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_DOMOVING_SYNC);

      // move both in translation and rotation
      en_vMoveTranslation = en_vIntendedTranslation-en_vAppliedTranslation;
      en_mMoveRotation = en_mIntendedRotation*!en_mAppliedRotation;

      InitTryToMove();
      CMovableEntity *penPusher = NULL;
      if ((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)==EPF_ONBLOCK_PUSH) {
        penPusher = this;
      }
      /* BOOL bMoveSuccessfull = */ TryToMove(penPusher, TRUE, TRUE);

    // if rotation and translation are asynchronious
    } else {
      ASSERT((en_ulPhysicsFlags&EPF_ONBLOCK_MASK)!=EPF_ONBLOCK_PUSH);
      _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_DOMOVING_ASYNC);

      // if there is no reference
      if (en_penReference == NULL) {
        _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_DOMOVING_ASYNC_SYNCTRY);

        // try to do simple move both in translation and rotation
        en_vMoveTranslation = en_vIntendedTranslation-en_vAppliedTranslation;
        en_mMoveRotation = en_mIntendedRotation*!en_mAppliedRotation;
        InitTryToMove();
        _ctTryToMoveCheckCounter = 4; // no retries
        BOOL bMoveSuccessfull = TryToMove(NULL, TRUE, TRUE);
        // if it passes
        if (bMoveSuccessfull) {
          // finish
          _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_DOMOVING_ASYNC_SYNCPASS);
          _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_DOMOVING);
// STREAMDUMP           ExportEntityPlacementAndSpeed(*(CMovableEntity *)this, "Do moving (return: if it passes)");
          return;
        }
      }

      _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_DOMOVING_ASYNC_TRANSLATE);
      // translate
      en_vMoveTranslation = en_vIntendedTranslation-en_vAppliedTranslation;
      InitTryToMove();
      TryToMove(NULL, TRUE, FALSE);

      // rotate
      en_mMoveRotation = en_mIntendedRotation*!en_mAppliedRotation;
      if (
          en_mMoveRotation(1,1)!=1 || en_mMoveRotation(1,2)!=0 || en_mMoveRotation(1,3)!=0 ||
          en_mMoveRotation(2,1)!=0 || en_mMoveRotation(2,2)!=1 || en_mMoveRotation(2,3)!=0 ||
          en_mMoveRotation(3,1)!=0 || en_mMoveRotation(3,2)!=0 || en_mMoveRotation(3,3)!=1) {
        _pfPhysicsProfile.IncrementCounter((INDEX) CPhysicsProfile::PCI_DOMOVING_ASYNC_ROTATE);
        InitTryToMove();
        TryToMove(NULL, FALSE, TRUE);
      }
    }

    _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_DOMOVING);
// STREAMDUMP     ExportEntityPlacementAndSpeed(*(CMovableEntity *)this, "Do moving (end of function)");
  }

  // calculate consequences of moving/not moving in this tick
  export void PostMoving(void) 
  {
    if (en_pciCollisionInfo==NULL) {
      // mark for removing from list of movers
      en_ulFlags |= ENF_INRENDERING;
      return;
    }

    if (en_ulPhysicsFlags&EPF_FORCEADDED) {
      en_ulPhysicsFlags&=~EPF_FORCEADDED;
      return;
    }

// STREAMDUMP     ExportEntityPlacementAndSpeed(*(CMovableEntity *)this, "Post moving (start of function)");

    _pfPhysicsProfile.StartTimer((INDEX) CPhysicsProfile::PTI_POSTMOVING);
    _pfPhysicsProfile.IncrementTimerAveragingCounter((INDEX) CPhysicsProfile::PTI_POSTMOVING);

    // remember valid reference if valid
    if (en_penReference!=NULL) {
      en_penLastValidReference = en_penReference;
    }

    // remember original translation
    FLOAT3D vOldTranslation = en_vCurrentTranslationAbsolute;
    FLOAT fTickQuantum=_pTimer->TickQuantum; // used for normalizing from SI units to game ticks
    // calculate current velocity from movements applied in this tick
    en_vCurrentTranslationAbsolute = en_vAppliedTranslation/fTickQuantum;

    // remember significant movements
    if (Abs(en_vCurrentTranslationAbsolute%en_vGravityDir)>0.1f) {
      en_tmLastSignificantVerticalMovement = _pTimer->CurrentTick();
    }

    ClearNextPosition();

    // calculate speed change between needed and possible (in m/s)
    FLOAT3D vSpeedDelta = en_vIntendedTranslation - en_vAppliedTranslation;
    FLOAT fSpeedDelta = vSpeedDelta.Length()/fTickQuantum;

    // if it is large change and can be damaged by impact
    if (fSpeedDelta>en_fCollisionSpeedLimit &&
       !(en_ulPhysicsFlags&EPF_NOIMPACTTHISTICK)) {
      // inflict impact damage 
      FLOAT fDamage = ((fSpeedDelta-en_fCollisionSpeedLimit)/en_fCollisionSpeedLimit)*en_fCollisionDamageFactor;
      InflictDirectDamage(this, MiscDamageInflictor(), DMT_IMPACT, fDamage, 
        en_plPlacement.pl_PositionVector, -vSpeedDelta.Normalize());
    }
    en_ulPhysicsFlags&=~EPF_NOIMPACTTHISTICK;

    // remember old speed for Touch reactions
    en_vIntendedTranslation = vOldTranslation;

    // if not moving anymore
    if (en_vCurrentTranslationAbsolute.ManhattanNorm()<0.001f
      &&(en_vDesiredTranslationRelative.ManhattanNorm()==0 || en_fAcceleration==0)
      &&en_aDesiredRotationRelative.ManhattanNorm()==0) {

      // if there is a reference
      if (en_penReference!=NULL) {
        // it the reference is movable
        if (en_penReference->en_ulPhysicsFlags&EPF_MOVABLE) {
          CMovableEntity *penReference = (CMovableEntity *)(CEntity*)en_penReference;
          // if the reference is not in the list of movers
          if (!penReference->en_lnInMovers.IsLinked()) {
            // mark for removing from list of movers
            en_ulFlags |= ENF_INRENDERING;
          }
        // if the reference is not movable
        } else {
          // mark for removing from list of movers
          en_ulFlags |= ENF_INRENDERING;
        }

      // if there is no reference
      } else {
        // if no gravity and no forces can affect this entity
        if (
          (!(en_ulPhysicsFlags&(EPF_TRANSLATEDBYGRAVITY|EPF_ORIENTEDBYGRAVITY))
            || en_fGravityA==0.0f)) { // !!!! test for forces also when implemented
          // mark for removing from list of movers
          en_ulFlags |= ENF_INRENDERING;
        }
      }

      // if should remove from movers list
      if (en_ulFlags&ENF_INRENDERING) {
        // clear last placement
        en_plLastPlacement = en_plPlacement;
      }
    }

    // remember new position for particles
    if (en_plpLastPositions!=NULL) {
      en_plpLastPositions->AddPosition(en_vNextPosition);
    }


    // if should filter predictions
    extern BOOL _bPredictionActive;
    if (_bPredictionActive && (IsPredictable() || IsPredictor())) {
      CMovableEntity *penTail = (CMovableEntity *)GetPredictedSafe(this);
      TIME tmNow = _pTimer->CurrentTick();
 
      if (penTail->en_tmLastPredictionHead<-1) {
        penTail->en_vLastHead = en_plPlacement.pl_PositionVector;
        penTail->en_vPredError = FLOAT3D(0,0,0);
        penTail->en_vPredErrorLast = FLOAT3D(0,0,0);
      }

      // if this is a predictor
      if (IsPredictor()) {
        // if a new prediction of old prediction head, or just started prediction
        if (penTail->en_tmLastPredictionHead==tmNow || penTail->en_tmLastPredictionHead<0) {
          // remember error
          penTail->en_vPredErrorLast = penTail->en_vPredError;
          penTail->en_vPredError += 
            en_plPlacement.pl_PositionVector-penTail->en_vLastHead;
          // remember last head
          penTail->en_vLastHead = en_plPlacement.pl_PositionVector;
          // if this is really head of prediction chain
          if (IsPredictionHead()) {
            // remember the time
            penTail->en_tmLastPredictionHead = tmNow;
          }

        // if newer than last prediction head
        } else if (tmNow>penTail->en_tmLastPredictionHead) {
          // just remember head and time
          penTail->en_vLastHead = en_plPlacement.pl_PositionVector;
          penTail->en_tmLastPredictionHead = tmNow;
        }

      // if prediction is of for this entity
      } else if (!(en_ulFlags&ENF_WILLBEPREDICTED)) {
        // if it was on before
        if (penTail->en_tmLastPredictionHead>0) {
          // remember error
          penTail->en_vPredErrorLast = penTail->en_vPredError;
          penTail->en_vPredError += 
            en_plPlacement.pl_PositionVector-penTail->en_vLastHead;
        }
        // remember this as head
        penTail->en_vLastHead = en_plPlacement.pl_PositionVector;
        penTail->en_tmLastPredictionHead = -1;
      }
      // if this is head of chain
      if (IsPredictionHead()) {
        // fade error
        penTail->en_vPredErrorLast = penTail->en_vPredError;
        penTail->en_vPredError *= cli_fPredictionFilter;
//        FLOAT fErrLen = penTail->en_vPredError.Length();
//        if (fErrLen>0) {
//          penTail->en_vPredError=penTail->en_vPredError/fErrLen*ClampDn(fErrLen-cli_fPredictionCorrection, 0.0f);
//        }
      }
    }

    //CPrintF("\n%f", _pTimer->CurrentTick());
    _pfPhysicsProfile.StopTimer((INDEX) CPhysicsProfile::PTI_POSTMOVING);

// STREAMDUMP     ExportEntityPlacementAndSpeed(*(CMovableEntity *)this, "Post moving (end of function)");
  }

  // call this if you move without collision
  export void CacheNearPolygons(void) 
  {
    CClipMove cm(this);
    cm.CacheNearPolygons();
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // init
    SLONG slUsedMemory = sizeof(CMovableEntity) - sizeof(CRationalEntity) + CRationalEntity::GetUsedMemory();
    // add some more
    slUsedMemory += en_apbpoNearPolygons.sa_Count * sizeof(CBrushPolygon*);
    return slUsedMemory;
  }


procedures:


  // must have at least one procedure per class
  Dummy() {};
};
