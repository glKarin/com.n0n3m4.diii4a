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

604
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/Debris";

class CRollingStone: CMovableModelEntity {
name      "RollingStone";
thumbnail "Thumbnails\\RollingStone.tbn";
features "IsTargetable";
properties:
  1 FLOAT m_fBounce "Bounce" 'B' = 0.5f,
  2 FLOAT m_fHealth "Health" 'H' = 400.0f,
  3 FLOAT m_fDamage "Damage" 'D' = 1000.0f,
  4 BOOL m_bFixedDamage "Fixed damage" 'F' = FALSE,
  5 FLOAT m_fStretch "Stretch" 'S' = 1.0f,
  6 FLOAT m_fDeceleration "Deceleration" = 0.9f,
  7 FLOAT m_fStartSpeed "Start Speed" 'Z' = 50.0f,
  8 ANGLE3D m_vStartDir "Start Direction" 'A' = ANGLE3D(0,0,0),
  9 CEntityPointer m_penDeathTarget "Death target" 'T',

 // sound channels for bouncing sound
 20 CSoundObject m_soBounce0,
 21 CSoundObject m_soBounce1,
 22 CSoundObject m_soBounce2,
 23 CSoundObject m_soBounce3,
 24 CSoundObject m_soBounce4,
 30 INDEX m_iNextChannel = 0,         // next channel to play sound on
 31 CSoundObject m_soRoll,
 32 BOOL m_bRollPlaying = FALSE,

 // internal vars
 40 FLOATquat3D m_qA = FLOATquat3D(0, 1, 0, 0),
 41 FLOATquat3D m_qALast = FLOATquat3D(0, 1, 0, 0),
 42 FLOAT m_fASpeed = 0.0f,
 43 FLOAT3D m_vR = FLOAT3D(0,0,1),

components:
 1 model   MODEL_ROLLINGSTONE      "Models\\Ages\\Egypt\\Traps\\RollingStone\\RollingStone.mdl",
 2 model   MODEL_STONESPHERE       "Models\\Ages\\Egypt\\Traps\\RollingStone\\Stone.mdl",
 3 texture TEXTURE_ROLLINGSTONE    "Models\\Ages\\Egypt\\Traps\\RollingStone\\Stone.tex",
 5 texture TEXTURE_DETAIL          "Models\\Ages\\Egypt\\Traps\\RollingStone\\Detail.tex",
// ************** STONE PARTS **************
 14 model     MODEL_STONE        "Models\\Effects\\Debris\\Stone\\Stone.mdl",
 15 texture   TEXTURE_STONE      "Models\\Effects\\Debris\\Stone\\Stone.tex",
 16 class     CLASS_DEBRIS       "Classes\\Debris.ecl",
  4 class     CLASS_BASIC_EFFECT "Classes\\BasicEffect.ecl",
 20 sound   SOUND_BOUNCE         "Sounds\\Misc\\RollingStone.wav",
 21 sound   SOUND_ROLL           "Sounds\\Misc\\RollingStoneEnvironment.wav",

functions:
  void Precache(void)
  {
    PrecacheClass(CLASS_DEBRIS);
    PrecacheModel(MODEL_STONE);
    PrecacheTexture(TEXTURE_STONE);
    PrecacheSound(SOUND_BOUNCE);
    PrecacheSound(SOUND_ROLL);
  }
  void PostMoving() {
    CMovableModelEntity::PostMoving();

    // if touching floor
    if (en_penReference!=NULL) {
      // adjust rotation and translation speeds
      AdjustSpeeds(en_vReferencePlane);
      //CPrintF("adjusting\n");
    } else {
      //CPrintF("not adjusting\n");
    }
//    m_fASpeed *= m_fDeceleration;

    m_qALast = m_qA;

    FLOATquat3D qRot;
    qRot.FromAxisAngle(m_vR, m_fASpeed*_pTimer->TickQuantum*PI/180);
    FLOATmatrix3D mRot;
    qRot.ToMatrix(mRot);
    m_qA = qRot*m_qA;
    if (en_ulFlags&ENF_INRENDERING) {
      m_qALast = m_qA;
    }
  }

  /* Adjust model mip factor if needed. */
  void AdjustMipFactor(FLOAT &fMipFactor)
  {
    fMipFactor = 0;// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    FLOATquat3D qA;
    qA = Slerp(_pTimer->GetLerpFactor(), m_qALast, m_qA);
    
    FLOATmatrix3D mA;
    qA.ToMatrix(mA);
    ANGLE3D vA;
    DecomposeRotationMatrixNoSnap(vA, mA);

    CAttachmentModelObject *amo = GetModelObject()->GetAttachmentModel(0);
    amo->amo_plRelative.pl_OrientationAngle = vA;
  }

  void AdjustSpeedOnOneAxis(FLOAT &fTraNow, FLOAT &aRotNow, BOOL bRolling)
  {
    // calculate new rotation and translation to make them synchronized
    // NOTE: formulae used:
    //   momentum of the ball : I = 2*m*r^2/5
    //   velocity and rotation syncronized: w*r = v
    //   sum of impulses is constant: w1/r*I+v1*m = w2/r*I+v2*m
    // this yields: v2 = (2*r*w1+5*v1)/7
    FLOAT fR = 4.0f*m_fStretch; // size of original sphere model (4m) times stretch

    FLOAT fTraNew = (2*aRotNow*fR+5*fTraNow)/7;
    FLOAT aRotNew = fTraNew/fR;

    fTraNow = fTraNew;
    aRotNow = aRotNew;
  }

  // adjust rotation and translation speeds
  void AdjustSpeeds(const FLOAT3D &vPlane) 
  {
    // if going too slow in translation and rotation
    if (en_vCurrentTranslationAbsolute.Length()<1.0f && m_fASpeed<1.0f) {
      // just stop
      en_vCurrentTranslationAbsolute = FLOAT3D(0,0,0);
      m_fASpeed = 0.0f;
      RollSound(0.0f);
      return;
    }

    // decompose speed to components regarding the plane
    FLOAT3D vTranslationNormal;
    FLOAT3D vTranslationParallel;
    GetParallelAndNormalComponents(en_vCurrentTranslationAbsolute, vPlane, vTranslationNormal, vTranslationParallel);

    // check if rolling
    BOOL bRolling = vTranslationNormal.Length()<0.1f;
    // if rolling
    if (bRolling) {
      // get rotation direction from speed, if possible
      FLOAT fSpeedTra = vTranslationParallel.Length();
/*      if (fSpeedTra>0.01f) {
        m_vR = (vTranslationParallel/fSpeedTra)*vPlane;
      }*/
      RollSound(fSpeedTra);
    } else {
      RollSound(0);
    }


    // --- find original axes and values

    // what is caused by rotation
    FLOAT3D vRotFromRot = m_vR;
    FLOAT3D vTraFromRot = vPlane*vRotFromRot;
    vTraFromRot.Normalize();

    FLOAT fTraFromRot = 0;
    FLOAT fRotFromRot = m_fASpeed*PI/180.0f;

    // what is caused by translation
    FLOAT3D vTraFromTra = vTranslationParallel;
    FLOAT fTraFromTra = vTraFromTra.Length();
    FLOAT3D vRotFromTra = FLOAT3D(1,0,0);
    FLOAT fRotFromTra = 0;
    if (fTraFromTra>0.001f) {
      vTraFromTra/=fTraFromTra;
      vRotFromTra = vTraFromTra*vPlane;
      vRotFromTra.Normalize();
    }

    // if there is any rotation
    if (Abs(fRotFromRot)>0.01f) {
      // adjust on rotation axis
      AdjustSpeedOnOneAxis(fTraFromRot, fRotFromRot, bRolling);
    }
    // if there is any translation
    if (Abs(fTraFromTra)>0.01f) {
      // adjust on translation axis
      AdjustSpeedOnOneAxis(fTraFromTra, fRotFromTra, bRolling);
    }

    // put the speeds back together
    FLOATquat3D qTra;
    qTra.FromAxisAngle(vRotFromTra, fRotFromTra);
    FLOATquat3D qRot;
    qRot.FromAxisAngle(vRotFromRot, fRotFromRot);
    FLOATquat3D q = qRot*qTra;
    FLOAT3D vSpeed = vTraFromTra*fTraFromTra + vTraFromRot*fTraFromRot;

    // set the new speeds
    en_vCurrentTranslationAbsolute = vTranslationNormal+vSpeed;
    q.ToAxisAngle(m_vR, m_fASpeed);
    m_fASpeed *= 180/PI;
  }

/************************************************************
 *                      S O U N D S                         *
 ************************************************************/
void BounceSound(FLOAT fSpeed) {
  FLOAT fHitStrength = fSpeed*fSpeed;

  FLOAT fVolume = fHitStrength/20.0f; 
  //CPrintF("bounce %g->%g\n", fHitStrength, fVolume);
  fVolume = Clamp( fVolume, 0.0f, 2.0f);
  //FLOAT fVolume = Clamp(fHitStrength*5E-3f, 0.0f, 2.0f);
  FLOAT fPitch = Lerp(0.2f, 1.0f, Clamp(fHitStrength/100, 0.0f, 1.0f));
  if (fVolume<0.1f) {
    return;
  }
  CSoundObject &so = (&m_soBounce0)[m_iNextChannel];
  m_iNextChannel = (m_iNextChannel+1)%5;
  so.Set3DParameters(200.0f*m_fStretch, 100.0f*m_fStretch, fVolume, fPitch);
  PlaySound(so, SOUND_BOUNCE, SOF_3D);
};

void RollSound(FLOAT fSpeed) 
{
  FLOAT fHitStrength = fSpeed*fSpeed*m_fStretch*m_fStretch*m_fStretch;

  FLOAT fVolume = fHitStrength/20.0f; 
  fVolume = Clamp( fVolume, 0.0f, 1.0f);
  FLOAT fPitch = Lerp(0.2f, 1.0f, Clamp(fHitStrength/100, 0.0f, 1.0f));
  if (fVolume<0.1f) {
    if (m_bRollPlaying) {
      m_soRoll.Stop();
      m_bRollPlaying = FALSE;
    }
    return;
  }
  m_soRoll.Set3DParameters(200.0f*m_fStretch, 100.0f*m_fStretch, fVolume, fPitch);

  if (!m_bRollPlaying) {
    PlaySound(m_soRoll, SOUND_ROLL, SOF_3D|SOF_LOOP);
    m_bRollPlaying = TRUE;
  }
}

procedures:

  Main()
  {
    // set appearance
    InitAsModel();
    SetPhysicsFlags(EPF_ONBLOCK_BOUNCE|EPF_PUSHABLE|EPF_MOVABLE|EPF_TRANSLATEDBYGRAVITY);
    SetCollisionFlags(ECF_MODEL);
    SetModel(MODEL_ROLLINGSTONE);
    SetModelMainTexture(TEXTURE_ROLLINGSTONE);
    AddAttachmentToModel(this, *GetModelObject(), 0, MODEL_STONESPHERE, TEXTURE_ROLLINGSTONE, 0, 0, TEXTURE_DETAIL);

    GetModelObject()->StretchModel( FLOAT3D(m_fStretch, m_fStretch, m_fStretch));
    ModelChangeNotify();

    en_fBounceDampNormal = m_fBounce;
    en_fBounceDampParallel = m_fBounce;
    en_fAcceleration = en_fDeceleration = m_fDeceleration;
    en_fCollisionSpeedLimit = 45.0f;
    en_fCollisionDamageFactor = 10.0f;

    SetPlacement(CPlacement3D(GetPlacement().pl_PositionVector, ANGLE3D(0,0,0)));
    m_qA = FLOATquat3D(0, 1, 0, 0);
    m_qALast= FLOATquat3D(0, 1, 0, 0);

    autowait(0.1f);

    SetHealth( m_fHealth);
    AddToMovers();

    wait() {
      on (ETrigger) : {
        FLOAT3D v;
        AnglesToDirectionVector(m_vStartDir, v);
        GiveImpulseTranslationAbsolute(v*m_fStartSpeed);
        //CPrintF("triggered\n");
        resume;
      }
      on (ETouch eTouch) :
      {

        //CPrintF("touched\n");

        if( !m_bFixedDamage)
        {
          FLOAT fDamageFactor = en_vCurrentTranslationAbsolute.Length()/10.0f;
          FLOAT fAppliedDamage = fDamageFactor*m_fDamage;
          // inflict damage
          InflictDirectDamage( eTouch.penOther, this, DMT_CANNONBALL, fAppliedDamage,
                     eTouch.penOther->GetPlacement().pl_PositionVector, eTouch.plCollision);
        }
        else
        {
          if(en_vCurrentTranslationAbsolute.Length() != 0.0f)
          {
            // inflict damage
            InflictDirectDamage( eTouch.penOther, this, DMT_CANNONBALL, m_fDamage,
                       eTouch.penOther->GetPlacement().pl_PositionVector, eTouch.plCollision);
          }
        }
    
        // adjust rotation and translation speeds
        AdjustSpeeds(eTouch.plCollision);

        // if touched a brush
        if (eTouch.penOther->GetRenderType() & RT_BRUSH)
        {
          BounceSound(((FLOAT3D&)eTouch.plCollision) % en_vCurrentTranslationAbsolute);
          // calculate speed along impact normal
          FLOAT fImpactSpeed = en_vCurrentTranslationAbsolute% (-(FLOAT3D&)eTouch.plCollision);

          // if strong collision
          if( fImpactSpeed > 1000)
          {
            // receive artificial impact damage
            ReceiveDamage(eTouch.penOther, DMT_IMPACT, m_fHealth*2.0f,
              FLOAT3D(0,0,0), FLOAT3D(0,0,0));
          }
        }
        resume;
      }
      on (EDeath eDeath) : {
        SendToTarget(m_penDeathTarget, EET_TRIGGER, eDeath.eLastDamage.penInflictor);
        // get your size
        FLOATaabbox3D box;
        GetBoundingBox(box);
        FLOAT fEntitySize = box.Size().MaxNorm();
        
        Debris_Begin(EIBT_ROCK, DPT_NONE, BET_NONE, fEntitySize, FLOAT3D(1.0f,2.0f,3.0f), FLOAT3D(0,0,0), 1.0f, 0.0f);
        for(INDEX iDebris = 0; iDebris<12; iDebris++) {
          Debris_Spawn(this, this, MODEL_STONE, TEXTURE_STONE, 0, 0, 0, IRnd()%4, 0.15f,
            FLOAT3D(FRnd()*0.8f+0.1f, FRnd()*0.8f+0.1f, FRnd()*0.8f+0.1f));
        }
        Destroy();
        stop;
      }
    }

    // cease to exist
    Destroy();

    return;
  }
};
