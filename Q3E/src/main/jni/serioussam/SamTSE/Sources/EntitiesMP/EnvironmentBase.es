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

900
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/EnvironmentMarker";
uses "EntitiesMP/WatchPlayers";

class CEnvironmentBase: CMovableEntity {
name      "Environment Base";
thumbnail "Thumbnails\\EnvironmentBase.tbn";
features  "HasName", "IsTargetable";

properties:
  1 CTString m_strName            "Name" 'N' = "Base Environment",
  2 CTString m_strDescription = "",
  3 RANGE m_fDistance             "Range" 'R' = 100.0f,   // distance when player is seen
  4 FLOAT m_fStretch              "Stretch" 'S' = 1.0f,

  5 CEntityPointer m_penTarget    "Target" 'T',
  6 CEntityPointer m_penWatcher,
  7 FLOAT m_fWatcherFrequency     "Watcher frequency" = 2.0f,   // watcher will look every x seconds for players
  8 FLOAT3D m_vDesiredPosition = FLOAT3D(0, 0, 0),              // desired position for moving

 10 FLOAT m_fMoveSpeed            "Move speed" 'V' = 2.0f,
 11 FLOAT m_fRotateSpeed          "Rotate speed" 'B' = 60.0f,
 12 FLOAT m_fMoveFrequency        "Move frequency" = 0.5f,
 13 BOOL m_bUseWatcher            "Use watcher" = FALSE,    // use individual watcher
 14 BOOL m_bFlying                "Flying" 'F' = FALSE,     // flying model
 16 FLOAT m_fWaitTime = 0.0f,

 20 CTFileName m_fnMdl           "Model" 'M' = CTFILENAME("Models\\Editor\\Axis.mdl"),
 21 CTFileName m_fnTex           "Texture" 'X' = CTString(""),
 22 ANIMATION m_iAnim            "Animation" =0,

 25 CTFileName m_fnAtt1Mdl       "Attachment 1 Model" = CTString(""),
 26 CTFileName m_fnAtt1Tex       "Attachment 1 Texture" = CTString(""),
 27 INDEX m_iAtt1Position        "Attachment 1 position"=0,
 28 ANIMATION m_iAtt1Anim        "Attachment 1 animation"=0,

 30 CTFileName m_fnAtt2Mdl       "Attachment 2 Model" = CTString(""),
 31 CTFileName m_fnAtt2Tex       "Attachment 2 Texture" = CTString(""),
 32 INDEX m_iAtt2Position        "Attachment 2 position"=1,
 33 ANIMATION m_iAtt2Anim        "Attachment 2 animation"=0,

 35 CTFileName m_fnAtt3Mdl       "Attachment 3 Model" = CTString(""),
 36 CTFileName m_fnAtt3Tex       "Attachment 3 Texture" = CTString(""),
 37 INDEX m_iAtt3Position        "Attachment 3 position"=1,
 38 ANIMATION m_iAtt3Anim        "Attachment 3 animation"=0,

components:
  1 class   CLASS_WATCHPLAYERS    "Classes\\WatchPlayers.ecl",

functions:
  /* Check if entity is moved on a route set up by its targets. */
  BOOL MovesByTargetedRoute(CTString &strTargetProperty) const {
    strTargetProperty = "Target";
    return TRUE;
  };
  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker(CTFileName &fnmMarkerClass, CTString &strTargetProperty) const {
    fnmMarkerClass = CTFILENAME("Classes\\EnvironmentMarker.ecl");
    strTargetProperty = "Target";
    return TRUE;
  };
  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("-><none>");
    if (m_penTarget!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", (const char *) m_penTarget->GetName());
    }
    return m_strDescription;
  };
  /* Get anim data for given animation property - return NULL for none. */
  CAnimData *GetAnimData(SLONG slPropertyOffset) {
    if(slPropertyOffset==_offsetof(CEnvironmentBase, m_iAnim)) {
      return GetModelObject()->GetData();

    } else if(slPropertyOffset==_offsetof(CEnvironmentBase, m_iAtt1Anim)) {
      CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel(m_iAtt1Position);
      if( pamo != NULL) { return pamo->amo_moModelObject.GetData(); }
      return CEntity::GetAnimData(slPropertyOffset);

    } else if(slPropertyOffset==_offsetof(CEnvironmentBase, m_iAtt2Anim)) {
      CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel(m_iAtt2Position);
      if( pamo != NULL) { return pamo->amo_moModelObject.GetData(); }
      return CEntity::GetAnimData(slPropertyOffset);

    } else if(slPropertyOffset==_offsetof(CEnvironmentBase, m_iAtt3Anim)) {
      CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel(m_iAtt3Position);
      if( pamo != NULL) { return pamo->amo_moModelObject.GetData(); }
      return CEntity::GetAnimData(slPropertyOffset);

    } else {
      return CEntity::GetAnimData(slPropertyOffset);
    }
  };



/************************************************************
 *                      MOVE FUNCTIONS                      *
 ************************************************************/
  // switch to next marker
  BOOL NextMarker(void) {
    if (m_penTarget==NULL) {
      return FALSE;
    }

    // assure valid target
    if (m_penTarget!=NULL && !IsOfClass(m_penTarget, "Environment Marker")) {
      WarningMessage("Target '%s' is not of Environment Marker class!", (const char *) m_penTarget->GetName());
      m_penTarget = NULL;
      return FALSE;
    }

    // get next marker
    CMarker *penTarget = (CMarker *)(CEntity*)m_penTarget;
    CMarker *penNextTarget = (CMarker *)(CEntity*)penTarget->m_penTarget;

    // if got to end
    if (penNextTarget==NULL) {
      return FALSE;
    }

    // remember next marker as current target
    m_penTarget = penNextTarget;

    return TRUE;
  };

  // calculate rotation
  void CalcRotation(ANGLE aWantedHeadingRelative, ANGLE3D &aRotation) {
    // normalize it to [-180,+180] degrees
    aWantedHeadingRelative = NormalizeAngle(aWantedHeadingRelative);

    // if desired position is left
    if (aWantedHeadingRelative<-m_fRotateSpeed*m_fMoveFrequency) {
      // start turning left
      aRotation(1) = -m_fRotateSpeed;
    // if desired position is right
    } else if (aWantedHeadingRelative>m_fRotateSpeed*m_fMoveFrequency) {
      // start turning right
      aRotation(1) = +m_fRotateSpeed;
    // if desired position is more-less ahead
    } else {
      aRotation(1) = aWantedHeadingRelative/m_fMoveFrequency;
    }
  };

  // stop moving
  void StopMoving(void) {
    SetDesiredRotation(ANGLE3D(0, 0, 0));
    SetDesiredTranslation(FLOAT3D(0.0f, 0.0f, 0.0f));
  };

  // move to position
  void MoveToPosition(void) {
    FLOAT3D vDesiredAngle;

    // desired angle vector
    vDesiredAngle = (m_vDesiredPosition - GetPlacement().pl_PositionVector).Normalize();
    // find relative heading towards the desired angle
    ANGLE3D aRotation(0,0,0);
    CalcRotation(GetRelativeHeading(vDesiredAngle), aRotation);

    // determine translation speed
    FLOAT3D vTranslation(0.0f, 0.0f, 0.0f);
    vTranslation(3) = -m_fMoveSpeed;

    // if flying set y axis translation speed
    if (m_bFlying) {
      vTranslation(2) = Sgn(vDesiredAngle(2)) * m_fMoveSpeed/10;
    }

    // start moving
    SetDesiredRotation(aRotation);
    SetDesiredTranslation(vTranslation);
  };

  // calc destination
  void CalcDestination() {
    // new position to walk to
    FLOAT fR = FRnd()*((CEnvironmentMarker&)*m_penTarget).m_fMarkerRange;
    FLOAT fA = FRnd()*360.0f;
    m_vDesiredPosition = m_penTarget->GetPlacement().pl_PositionVector + 
                          FLOAT3D(CosFast(fA)*fR, 0, SinFast(fA)*fR);
  };

  // marker parameters
  void MarkerParameters() {
    if (m_penTarget != NULL) {
      CEnvironmentMarker &em = (CEnvironmentMarker&)*m_penTarget;
      if (em.m_fMoveSpeed > 0.0f) {
        m_fMoveSpeed = em.m_fMoveSpeed;
      }
      if (em.m_fRotateSpeed > 0.0f) {
        m_fRotateSpeed = em.m_fRotateSpeed;
      }
    }
  };



/************************************************************
 *                  INITIALIZE FUNCTIONS                    *
 ************************************************************/
  void Initialize(void) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING&~(EPF_ORIENTEDBYGRAVITY|EPF_TRANSLATEDBYGRAVITY));
    SetCollisionFlags(ECF_MODEL);

    // set model stretch -- MUST BE DONE BEFORE SETTING MODEL!
    GetModelObject()->mo_Stretch = FLOAT3D( m_fStretch, m_fStretch, m_fStretch);

    // set appearance
    SetModel(m_fnMdl);
    GetModelObject()->PlayAnim(m_iAnim, AOF_LOOPING);
    if( m_fnTex != CTString("")) {
      GetModelObject()->mo_toTexture.SetData_t(m_fnTex);
    }

    GetModelObject()->RemoveAllAttachmentModels();
    
    AddAttachment( m_iAtt1Position, m_fnAtt1Mdl, m_fnAtt1Tex);
    CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel( m_iAtt1Position);
    if( pamo != NULL) {
      pamo->amo_moModelObject.StartAnim( m_iAtt1Anim);
    }

    if( (m_iAtt2Position != m_iAtt1Position) && (m_fnAtt1Mdl != m_fnAtt2Mdl) ) {
      AddAttachment( m_iAtt2Position, m_fnAtt2Mdl, m_fnAtt2Tex);
      CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel( m_iAtt2Position);
      if( pamo != NULL) {
        pamo->amo_moModelObject.StartAnim( m_iAtt2Anim);
      }
    }

    if( (m_iAtt3Position != m_iAtt1Position) && (m_fnAtt1Mdl != m_fnAtt3Mdl) && 
        (m_iAtt3Position != m_iAtt2Position) && (m_fnAtt2Mdl != m_fnAtt3Mdl) ) {
      AddAttachment( m_iAtt3Position, m_fnAtt3Mdl, m_fnAtt3Tex);
      CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel( m_iAtt3Position);
      if( pamo != NULL) {
        pamo->amo_moModelObject.StartAnim( m_iAtt3Anim);
      }
    }

    // assure valid target
    if (m_penTarget!=NULL && !IsOfClass(m_penTarget, "Environment Marker")) {
      WarningMessage("Target '%s' is not of Environment Marker class!", (const char *) m_penTarget->GetName());
      m_penTarget = NULL;
    }
};


/************************************************************
 *                    WATCHER FUNCTIONS                     *
 ************************************************************/
  void InitializeWatcher(FLOAT fWaitTime) {
    // spawn player watcher
    m_penWatcher = CreateEntity(GetPlacement(), CLASS_WATCHPLAYERS);
    m_penWatcher->Initialize(EVoid());

    // setup player watcher
    CWatchPlayers &pw = (CWatchPlayers&)*m_penWatcher;
    pw.m_penOwner = this;
    pw.m_fWaitTime = 2.0f;
    pw.m_fDistance = m_fDistance;
    pw.m_bRangeWatcher = FALSE;
    pw.m_eetEventClose = EET_ENVIRONMENTSTART;
    pw.m_eetEventFar = EET_ENVIRONMENTSTOP;
  };



/************************************************************
 *                    ANIMATION FUCNTIONS                   *
 ************************************************************/
  // play default anim
  void PlayDefaultAnim(void) {
    GetModelObject()->PlayAnim(m_iAnim, AOF_LOOPING|AOF_NORESTART);
  };

  // play marker animation
  void PlayMarkerAnimation(void) {
    if (m_penTarget != NULL) {
      GetModelObject()->PlayAnim(((CEnvironmentMarker&)*m_penTarget).m_iAnim, AOF_LOOPING|AOF_NORESTART);
    }
  };

  // change default anim
  void ChangeDefaultAnim(void) {
    if (m_penTarget != NULL && ((CEnvironmentMarker&)*m_penTarget).m_bChangeDefaultAnim) {
      m_iAnim = ((CEnvironmentMarker&)*m_penTarget).m_iAnim;
    }
  };

  // wait on marker
  void WaitOnMarker(void) {
    if (m_penTarget != NULL) {
      CEnvironmentMarker &em = (CEnvironmentMarker&)*m_penTarget;
      m_fWaitTime = em.m_fWaitTime;               // wait time
      m_fWaitTime += FRnd() * em.m_fRandomTime;   // random wait time
      // fixed anim length
      if (em.m_bFixedAnimLength) {
        m_fWaitTime = floor(m_fWaitTime + 0.5f);
      }
    }
  };



procedures:
/************************************************************
 *                    SUPPORT PROCEDURES                    *
 ************************************************************/
  // move to marker
  MoveToMarker(EVoid) {
    // if next marker exist
    if (NextMarker()) {
      // destination
      CalcDestination();
      // move to marker
      while ((m_vDesiredPosition-GetPlacement().pl_PositionVector).Length() > 5.0f) {
        wait(m_fMoveFrequency) {
          on (EBegin) : {
            MoveToPosition();
            resume;
          }
          on (ETimer) : { stop; }
        }
      }
    }
    // stop moving
    StopMoving();
    return EEnd();
  };



/************************************************************
 *                    A C T I O N S                         *
 ************************************************************/
  // activate
  Activate(EVoid) {
    wait() {
      on (EBegin) : { call DoAction(); }
      on (EEnvironmentStop) : { jump Stop(); }
    }
  };

  // just wait
  Stop(EVoid) {
    StopMoving();
    wait() {
      on (EBegin) : { resume; }
      on (EEnvironmentStart) : { jump Activate(); }
    }
  };

  // do actions
  DoAction(EVoid) {
    while (TRUE) {
      WaitOnMarker();
      if (m_fWaitTime > 0.0f) {
        PlayMarkerAnimation();
        autowait(m_fWaitTime);
      }
      ChangeDefaultAnim();

      MarkerParameters();
      PlayDefaultAnim();
      autocall MoveToMarker() EEnd;

      // if no more targets wait forever in last anim (if marker anim exist otherwise in default anim)
      if (m_penTarget==NULL || ((CEnvironmentMarker&)*m_penTarget).m_penTarget==NULL) {
        autowait();
      }
    }
  };



/************************************************************
 *                M  A  I  N    L  O  O  P                  *
 ************************************************************/
  // main loop
  MainLoop(EVoid) {
    autocall Stop() EEnd;

    // destroy player watcher
    m_penWatcher->SendEvent(EEnd());

    // cease to exist
    Destroy();

    return;
  };

  Main() {
    // initialize
    Initialize();

    // wait until game starts
    autowait(FRnd()*2.0f+1.0f);

    // initialize watcher
    if (m_bUseWatcher) {
      InitializeWatcher(m_fWatcherFrequency);
    }

    m_strDescription = "Environment base";

    jump MainLoop();
  };
};
