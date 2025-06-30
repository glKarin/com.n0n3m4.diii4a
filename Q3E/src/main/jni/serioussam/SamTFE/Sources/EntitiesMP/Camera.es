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

220
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/WorldLink";
uses "EntitiesMP/Player";
uses "EntitiesMP/CameraMarker";

class CCamera : CMovableModelEntity
{
name      "Camera";
thumbnail "Thumbnails\\Camera.tbn";
features  "HasName", "IsTargetable", "IsImportant";

properties:

  1 FLOAT m_tmTime             "Time" 'E' = 5.0f,       // how long to show the scene
  2 FLOAT m_fFOV               "FOV" 'F' = 90.0f,       // camera fov
  5 FLOAT m_fLastFOV = 90.0f,
  3 CEntityPointer m_penTarget "Target" 'T' COLOR(C_lBLUE|0xFF),
  4 CTString m_strName         "Name" 'N' = "Camera",
  6 CEntityPointer m_penOnBreak "OnBreak" 'B' COLOR(C_lRED|0xFF),
  7 BOOL m_bWideScreen "WideScreen" 'W' = TRUE,

 10 FLOAT m_tmAtMarker = 0.0f, // time when current marker was reached
 11 FLOAT m_tmDelta = 0.0f, // time to reach next marker
 13 FLOAT3D m_vPNp0 = FLOAT3D(0,0,0),
 14 FLOAT3D m_vPNp1 = FLOAT3D(0,0,0),
 15 FLOAT3D m_vTNp0 = FLOAT3D(0,0,0),
 16 FLOAT3D m_vTNp1 = FLOAT3D(0,0,0),
 17 FLOAT m_fFOVp0 = 0.0f,
 18 FLOAT m_fFOVp1 = 0.0f,
 19 FLOAT m_fTFOVp0 = 0.0f,
 20 FLOAT m_fTFOVp1 = 0.0f,
 31 FLOATquat3D m_qPNp0 = FLOATquat3D(0,0,0,0),
 32 FLOATquat3D m_qPNp1 = FLOATquat3D(0,0,0,0),
 33 FLOATquat3D m_qANp0 = FLOATquat3D(0,0,0,0),
 34 FLOATquat3D m_qANp1 = FLOATquat3D(0,0,0,0),
 
 40 CEntityPointer m_penLast,    // previous marker
 41 CEntityPointer m_penPlayer,  // player viewing this camera
 42 CTString m_strDescription = "",
 43 BOOL m_bStopMoving = FALSE,       // stop moving camera on next target

 50 COLOR m_colFade0 = 0,     // camera fading color
 51 COLOR m_colFade1 = 0,
 52 BOOL m_bMoving = FALSE,   // set while moving
 53 CEntityPointer m_penViewTarget0,
 54 CEntityPointer m_penViewTarget1,

 55 FLOAT3D m_vPosRatio0 = FLOAT3D(0,0,0),
 56 FLOAT3D m_vPosRatio1 = FLOAT3D(0,0,0),

 60 FLOAT m_fMyTimer = 0.0f,
 61 FLOAT m_fMyTimerLast = 0.0f,
 62 BOOL m_bIgnoreTimeStretch "Ignore time stretch" = FALSE,
 63 BOOL m_bAutoRotation "Auto rotate (AR)" 'A' = FALSE,
 64 FLOAT m_fStartHdg "AR start heading" 'D' = 0.0f,
 65 FLOAT m_fRotateSpeed "AR Rotate speed" 'S' = 180.0f,
 66 FLOAT m_fRotateTime "AR Rotate time" 'I' = 8.0f,
 67 FLOAT m_fRadX "AR Radius X" 'X' = 8.0f,
 68 FLOAT m_fHeight "AR Height (controlls pitch)" 'H' = 4.0f,
 69 FLOAT m_fRadZ "AR Radius Z" 'Z' = 8.0f,

 70 CEntityPointer m_penAutoCameraEndTarget "Auto camera end target",
 71 enum EventEType m_eetAutoCameraEndEvent "Auto camera end event" = EET_STOP,
 72 FLOAT3D m_vRelTargetOffset = FLOAT3D(0,0,0),

components:

  1 model   MODEL_CAMERA     "Models\\Editor\\Camera.mdl",
  2 texture TEXTURE_CAMERA   "Models\\Editor\\Camera.tex"


functions:

  // render particles
  void RenderParticles(void)
  {
    if (Particle_GetViewer()==this) {
      Particles_ViewerLocal(this);
    }
  }

  // Check if entity is moved on a route set up by its targets
  BOOL MovesByTargetedRoute( CTString &strTargetProperty) const
  {
    strTargetProperty = "Target";
    return TRUE;
  }

  // Check if entity can drop marker for making linked route
  BOOL DropsMarker( CTFileName &fnmMarkerClass, CTString &strTargetProperty) const
  {
    fnmMarkerClass = CTFILENAME( "Classes\\CameraMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  }

  // returns camera description
  const CTString &GetDescription(void) const
  {
    if (m_penTarget!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", (const char *) m_penTarget->GetName());
    } else {
      ((CTString&)m_strDescription).PrintF("-><none>");
    }
    return m_strDescription;
  }

  void GetAutoRotatePlacement( FLOAT tmCurrent, FLOAT3D &vPos, FLOATmatrix3D &mRot,
    CPlacement3D &plNew, FLOAT3D vTarget) const
  {
    // create new pos and angle
    FLOAT fT=m_fStartHdg+m_fRotateSpeed*tmCurrent;
    FLOAT fX=m_fRadX*Sin(fT);
    FLOAT fZ=m_fRadZ*Cos(fT);
    vPos = FLOAT3D(fX, -m_fHeight, fZ);
    FLOAT3D vDir=vPos;
    vDir.Normalize();
    ANGLE3D ang;
    DirectionVectorToAngles(vDir, ang);
    plNew.pl_PositionVector = vTarget-vPos;
    plNew.pl_OrientationAngle = ang;

    MakeRotationMatrix( mRot, ang);
    return;
  }

  CPlacement3D GetLerpedPlacement(void) const
  {
    FLOAT fLerpFactor;
    if (IsPredictor()) {
      fLerpFactor = _pTimer->GetLerpFactor();
    } else {
      fLerpFactor = _pTimer->GetLerpFactor2();
    }
    
    if( m_bAutoRotation && m_bMoving)
    {
      FLOAT fTime=Lerp(m_fMyTimerLast, m_fMyTimer, fLerpFactor);

      FLOAT3D vPos;
      FLOATmatrix3D mRot;
      CPlacement3D plNew;
      // get target placement
      FLOAT3D vTarget=FLOAT3D(0,0,0);
      if( m_penTarget!=NULL)
      {
        CCameraMarker *pcm = (CCameraMarker*) m_penTarget.ep_pen;
        if( pcm->m_penViewTarget!=NULL)
        {
          vTarget=pcm->m_penViewTarget->GetLerpedPlacement().pl_PositionVector+m_vRelTargetOffset;
        }
      }

      GetAutoRotatePlacement( fTime, vPos, mRot, plNew, vTarget);
      return plNew;
    }
    else if(m_penViewTarget0!=NULL)
    {
      CPlacement3D plNew=LerpPlacementsPrecise(en_plLastPlacement, en_plPlacement, fLerpFactor);
      FLOATmatrix3D mRot;
      CalcTargetedRotation(plNew.pl_PositionVector, m_penViewTarget0, m_vPosRatio0, mRot, TRUE);
      DecomposeRotationMatrixNoSnap(plNew.pl_OrientationAngle, mRot);
      return plNew;
    }
    else
    {
      return LerpPlacementsPrecise(en_plLastPlacement, en_plPlacement, fLerpFactor);
    }
    //return CMovableEntity::GetLerpedPlacement();
  }

  // calculate rotation matrix that points in direction of a target entity
  void CalcTargetedRotation(const FLOAT3D &vMyPos, CEntity *penViewTarget,
    FLOAT3D vPosRatio, FLOATmatrix3D &mRotTarget, BOOL bLerping) const
  {
    FLOAT3D vAbsPos;
    penViewTarget->GetEntityPointRatio(vPosRatio, vAbsPos, bLerping);
    FLOAT3D vDir;
    vDir=vAbsPos-vMyPos;
    vDir.Normalize();
    ANGLE3D aDir;
    DirectionVectorToAnglesNoSnap(vDir, aDir);
    MakeRotationMatrixFast(mRotTarget, aDir);
  }

  void PreMoving()
  {
    // remember old placement for lerping
    en_plLastPlacement = en_plPlacement;  
  }


  void DoMoving()  
  {
    if (!m_bMoving) {
      return;
    }
    FLOAT tmCurrent;
    if( !m_bIgnoreTimeStretch)
    {
      tmCurrent= _pTimer->CurrentTick();
    }
    else
    {
      m_fMyTimerLast = m_fMyTimer;
      m_fMyTimer+=_pTimer->TickQuantum/_pNetwork->GetRealTimeFactor();
      tmCurrent = m_fMyTimer;
    }

    if( m_bAutoRotation)
    {
      // if we're finished with auto rotating camera
      if( tmCurrent>m_fRotateTime)
      {
        m_bStopMoving=TRUE;
        return;
      }
      FLOAT3D vPos;
      FLOATmatrix3D mRot;
      CPlacement3D plNew;
      // get target placement
      FLOAT3D vTarget=FLOAT3D(0,0,0);
      if( m_penTarget!=NULL)
      {
        CCameraMarker *pcm = (CCameraMarker*) m_penTarget.ep_pen;
        if( pcm->m_penViewTarget!=NULL)
        {
          vTarget=pcm->m_penViewTarget->GetPlacement().pl_PositionVector+m_vRelTargetOffset;
        }
      }
      GetAutoRotatePlacement( tmCurrent, vPos, mRot, plNew, vTarget);
      en_vNextPosition = vPos;
      en_mNextRotation = mRot;
      CacheNearPolygons();
      SetPlacement_internal(plNew, mRot, TRUE);
      return;
    }

    // lerping is initially enabled
    BOOL bLerping = TRUE;

    // if we hit a marker
    if( tmCurrent > (m_tmAtMarker+m_tmDelta - _pTimer->TickQuantum*3/2)) 
    {
      // get markers
      CCameraMarker *pcmNm1 = &(CCameraMarker&)*m_penLast;
      CCameraMarker *pcmNp0 = &(CCameraMarker&)*m_penTarget;
      CCameraMarker *pcmNp1 = &(CCameraMarker&)*pcmNp0->m_penTarget;
      CCameraMarker *pcmNp2 = &(CCameraMarker&)*pcmNp1->m_penTarget;

      // repeat
      FOREVER {
        // if there is a trigger at the hit marker
        if (pcmNp0->m_penTrigger!=NULL) {
          // trigger it
          SendToTarget(pcmNp0->m_penTrigger, EET_TRIGGER, m_penPlayer);
        }
        
        // if the marker should not be skipped
        if (!pcmNp0->m_bSkipToNext) {
          // stop skipping
          break;
        }

        // go to next marker immediately
        pcmNm1 = pcmNp0;
        pcmNp0 = pcmNp1;
        pcmNp1 = pcmNp2;
        pcmNp2 = (CCameraMarker*) pcmNp2->m_penTarget.ep_pen;

        // disable lerping
        bLerping = FALSE;
      }

      // update markers for next interval
      m_penTarget = pcmNp1;
      m_penLast   = pcmNp0;

      // get markers
      CCameraMarker &cmNm1 = *pcmNm1;
      CCameraMarker &cmNp0 = *pcmNp0;
      CCameraMarker &cmNp1 = *pcmNp1;
      CCameraMarker &cmNp2 = *pcmNp2;

      // get positions from four markers
      const FLOAT3D &vPNm1 = cmNm1.GetPlacement().pl_PositionVector;
      const FLOAT3D &vPNp0 = cmNp0.GetPlacement().pl_PositionVector;
      const FLOAT3D &vPNp1 = cmNp1.GetPlacement().pl_PositionVector;
      const FLOAT3D &vPNp2 = cmNp2.GetPlacement().pl_PositionVector;
      ANGLE3D aPNm1 = cmNm1.GetPlacement().pl_OrientationAngle;
      ANGLE3D aPNp0 = cmNp0.GetPlacement().pl_OrientationAngle;
      ANGLE3D aPNp1 = cmNp1.GetPlacement().pl_OrientationAngle;
      ANGLE3D aPNp2 = cmNp2.GetPlacement().pl_OrientationAngle;
      FLOAT fFOVm1 = cmNm1.m_fFOV;
      FLOAT fFOVp0 = cmNp0.m_fFOV;
      FLOAT fFOVp1 = cmNp1.m_fFOV;
      FLOAT fFOVp2 = cmNp2.m_fFOV;
      m_colFade0 = cmNp0.m_colFade;
      m_colFade1 = cmNp1.m_colFade;
      m_penViewTarget0 = cmNp0.m_penViewTarget;
      m_penViewTarget1 = cmNp1.m_penViewTarget;
      m_vPosRatio0=FLOAT3D(0,0,0);
      m_vPosRatio1=FLOAT3D(0,0,0);
      if( m_penViewTarget0!=NULL)
      {
        m_vPosRatio0=cmNp0.m_vPosRatio;
      }
      if( m_penViewTarget1!=NULL)
      {
        m_vPosRatio1=cmNp1.m_vPosRatio;
      }

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
      m_fFOVp0 = fFOVp0;
      m_fFOVp1 = fFOVp1;
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

      // find tangents for FOV
      m_fTFOVp0 = ( (fFOVp1-fFOVp0) * fF00 + (fFOVp0-fFOVm1) * fF01) * fD0;
      m_fTFOVp1 = ( (fFOVp2-fFOVp1) * fF10 + (fFOVp1-fFOVp0) * fF11) * fD1;

      // find tangents for rotation
      FLOATquat3D qTNp0, qTNp1;
      qTNp0 = ( Log(qPNp0.Inv()*qPNp1) * fF00 + Log(qPNm1.Inv()*qPNp0) * fF01) * fD0;
      qTNp1 = ( Log(qPNp1.Inv()*qPNp2) * fF10 + Log(qPNp0.Inv()*qPNp1) * fF11) * fD1;

      // find squad parameters
      m_qANp0 = qPNp0*Exp( (qTNp0 - Log(qPNp0.Inv()*qPNp1))/2 );
      m_qANp1 = qPNp1*Exp( (Log(qPNp0.Inv()*qPNp1) - qTNp1)/2 );

      // check for stop moving
      if( cmNp0.m_bStopMoving) {
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
    FLOAT fFOV = m_fFOVp0*fH0 + m_fFOVp1*fH1 + m_fTFOVp0*fH2 + m_fTFOVp1*fH3;
    FLOATquat3D qRot = Squad(fT, m_qPNp0, m_qPNp1, m_qANp0, m_qANp1);
    FLOATmatrix3D mRot;
    qRot.ToMatrix(mRot);

    // calculate targeted matrices
    FLOATmatrix3D mRotTarget0 = mRot;
    FLOATmatrix3D mRotTarget1 = mRot;
    BOOL bDoTargeting = FALSE;
    if (m_penViewTarget0!=NULL) {
      CalcTargetedRotation(vPos, m_penViewTarget0, m_vPosRatio0, mRotTarget0, FALSE);
      bDoTargeting = TRUE;
    }
    if (m_penViewTarget1!=NULL) {
      CalcTargetedRotation(vPos, m_penViewTarget1, m_vPosRatio1, mRotTarget1, FALSE);
      bDoTargeting = TRUE;
    }
    
    // if any targeting involved
    if (bDoTargeting) {
      // lerp between two matrices
      FLOATquat3D qRot0; qRot0.FromMatrix(mRotTarget0);
      FLOATquat3D qRot1; qRot1.FromMatrix(mRotTarget1);
      FLOATquat3D qRot = Slerp(Clamp(fT, 0.0f, 1.0f), qRot0, qRot1);
      qRot.ToMatrix(mRot);
    }

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
    // set new fov
    m_fLastFOV = m_fFOV;
    m_fFOV = fFOV;
  }


  void PostMoving()  
  {
    if (!m_bMoving) {
      return;
    }
    //
    if( m_bStopMoving) {
      m_bMoving = FALSE;
      // mark for removing from list of movers
      en_ulFlags |= ENF_INRENDERING;
      SendEvent( EStop());
    }
  }


procedures:

  // routine for playing static camera
  PlayStaticCamera()
  {
    m_bMoving = FALSE;
    ECameraStart eStart;
    eStart.penCamera = this;
    m_penPlayer->SendEvent(eStart);
    autowait(m_tmTime);
    ECameraStop eStop;
    eStop.penCamera=this;
    m_penPlayer->SendEvent(eStop);
    return;
  }

  // routine for playing autorotating camera
  PlayAutoRotatingCamera()
  {
    // register camera as movable entity
    AddToMovers();
    m_bMoving = TRUE;
    ECameraStart eStart;
    eStart.penCamera = this;
    m_penPlayer->SendEvent(eStart);
    // roll, baby, roll ...
    wait() {
      on( EStop) : {
        ECameraStop eStop;
        eStop.penCamera=this;
        m_penPlayer->SendEvent(eStop);
        if( m_penAutoCameraEndTarget!=NULL)
        {
          SendToTarget(m_penAutoCameraEndTarget, m_eetAutoCameraEndEvent, m_penPlayer);
        }
        return;
      }
      otherwise() : {
        resume;
      }
    }
    return;
  }

  // routine for playing movable camera
  PlayMovingCamera()
  {
    // init camera
    ECameraStart eStart;
    eStart.penCamera = this;
    m_penPlayer->SendEvent(eStart);

    // check all markers for correct type and numbers
    INDEX ctMarkers=1;
    INDEX ctNonSkipped=0;
    CCameraMarker *pcm0 = (CCameraMarker*) m_penTarget.ep_pen;
    CCameraMarker *pcm  = (CCameraMarker*) pcm0->m_penTarget.ep_pen;
    // loop thru markers
    while( pcm!=NULL && pcm->m_penTarget!=pcm0)
    {
      pcm = (CCameraMarker*) pcm->m_penTarget.ep_pen;
      if (pcm==NULL) {
        WarningMessage( "Movable camera - broken link!");
        return;
      }
      if (!pcm->m_bSkipToNext) {
        ctNonSkipped++;
      }
      ctMarkers++;
      if (ctMarkers>500) {
        WarningMessage( "Movable camera - invalid marker loop!");
        return;
      }
    }
    // check if we have enough markers to do smooth interpolation
    if( ctMarkers<2) {
      WarningMessage( "Movable camera requires at least 2 markers in order to work!");
      return;
    }
    // check if we have enough markers to do smooth interpolation
    if( ctNonSkipped<1) {
      WarningMessage( "Movable camera requires at least 1 non-skipped marker!");
      return;
    }

    // prepare internal variables
    FLOAT tmCurrent;
    if( !m_bIgnoreTimeStretch)
    {
      tmCurrent= _pTimer->CurrentTick();
    }
    else
    {
      tmCurrent = m_fMyTimer;
    }

    m_tmAtMarker = tmCurrent;
    m_tmDelta = 0.0f;
    m_bStopMoving = FALSE;
    m_penLast = pcm; // keep last marker
    ASSERT( pcm->m_penTarget == m_penTarget);
    pcm  = (CCameraMarker*) m_penTarget.ep_pen;
    m_colFade0 = m_colFade1 = pcm->m_colFade;

    // register camera as movable entity
    AddToMovers();
    m_bMoving = TRUE;

    // roll, baby, roll ...
    wait() {
      on( EStop) : {
        ECameraStop eStop;
        eStop.penCamera=this;
        m_penPlayer->SendEvent(eStop);
        return;
      }
      otherwise() : {
        resume;
      }
    }

    // all done for now
    return;
  }


  // determine camera type and jump to corresponding routine
  PlayCamera()
  {
    // eventually add to movers list
    CCameraMarker *cm = (CCameraMarker*) m_penTarget.ep_pen;
    // if auto rotating
    if( m_bAutoRotation)
    {
      jump PlayAutoRotatingCamera();
    }
    // if there is at least one marker
    else if( cm!=NULL)
    {
      // treat camera as movable
      jump PlayMovingCamera();
    }
    else 
    {
      // treat camera as fixed
      jump PlayStaticCamera();
    }
  }


  Main()
  {
    // init as model
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MOVABLE);
    SetCollisionFlags(ECF_CAMERA);
    // set appearance
    FLOAT fSize = 5.0f;
    GetModelObject()->mo_Stretch = FLOAT3D(fSize, fSize, fSize);
    SetModel(MODEL_CAMERA);
    SetModelMainTexture(TEXTURE_CAMERA);
    m_fLastFOV = m_fFOV;

    if( m_penTarget!=NULL && !IsOfClass( m_penTarget, "Camera Marker")) {
      WarningMessage( "Entity '%s' is not of Camera Marker class!", (const char *) (m_penTarget->GetName()));
      m_penTarget = NULL;
    }

    if( m_bAutoRotation || m_penTarget!=NULL)
    {
      autowait(0.1f);
    }

    m_vRelTargetOffset=FLOAT3D(0,0,0);
    if( m_penTarget!=NULL)
    {
      CCameraMarker *pcm = &(CCameraMarker&)*m_penTarget;
      if( pcm->m_penViewTarget!=NULL)
      {
        FLOAT3D vAbsTarget=FLOAT3D(0,0,0);
        pcm->m_penViewTarget->GetEntityPointRatio(pcm->m_vPosRatio, vAbsTarget, FALSE);
        m_vRelTargetOffset=vAbsTarget-pcm->m_penViewTarget->GetPlacement().pl_PositionVector;
      }
    }

    while(TRUE)
    {
      wait() {
        on (ETrigger eTrigger) : {
          CEntity *penCaused;
          penCaused = FixupCausedToPlayer(this, eTrigger.penCaused, FALSE);
          if( IsDerivedFromClass(penCaused, "Player")) {
            m_penPlayer = penCaused;
            call PlayCamera();
          }
          resume;
        }
      }
    };

    // cease to exist
    Destroy();
    return;
  };
};

