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
#include "ModelsMP/Enemies/AirElemental/Twister.h"

#define ECF_TWISTER ( \
  ((ECBI_BRUSH|ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_TEST) |\
  ((ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_PASS) |\
  ((ECBI_MODEL)<<ECB_IS))
#define EPF_TWISTER ( \
  EPF_ONBLOCK_CLIMBORSLIDE|EPF_ORIENTEDBYGRAVITY|\
  EPF_TRANSLATEDBYGRAVITY|EPF_MOVABLE|EPF_ABSOLUTETRANSLATE)
%}


uses "EntitiesMP/AirElemental";
uses "EntitiesMP/Elemental";
uses "EntitiesMP/Spinner";


// input parameter for twister
event ETwister {
  CEntityPointer penOwner,        // entity which owns it
  FLOAT fSize,                    // twister size
  FLOAT fDuration,                // twister duration
  INDEX sgnSpinDir,               // spin direction
  BOOL  bGrow,                    // grow from miniature twister to full size?
  BOOL  bMovingAllowed,           // if moving is allowed
};


%{
static EntityInfo eiTwister = {
  EIBT_AIR, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 0.75f, 0.0f,
};


#define MOVE_FREQUENCY 0.1f
#define ROTATE_SPEED 10000.0f
#define MOVE_SPEED 7.5f

void CTwister_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->PrecacheClass(CLASS_SPINNER);
  pdec->PrecacheModel(MODEL_TWISTER);
  pdec->PrecacheTexture(TEXTURE_TWISTER);
  pdec->PrecacheSound(SOUND_SPIN);
}
%}

class CTwister : CMovableModelEntity {
name      "Twister";
thumbnail "";
features  "ImplementsOnPrecache";

properties:
  1 CEntityPointer m_penOwner,                  // entity which owns it
  2 FLOAT   m_fSize = 1.0f,                     // size
  3 FLOAT3D m_vSpeed = FLOAT3D(0,0,0),          // current speed
  4 INDEX   m_sgnSpinDir = 1,                   // spin clockwise
  5 BOOL    m_bGrow = TRUE,                     // grow to full size?
  6 FLOAT   m_tmLastMove = 0.0f,                // when moving has started
  7 FLOAT3D m_aSpeedRotation = FLOAT3D(0,0,0),
  8 BOOL    m_bMoving = FALSE,
  9 BOOL    m_bMovingAllowed = TRUE,
  
 // internal -> do not use
 10 FLOAT3D m_vDesiredPosition = FLOAT3D(0,0,0),
 11 FLOAT3D m_vDesiredAngle = FLOAT3D(0,0,0),
 12 FLOAT m_fStopTime = 0.0f,
 13 FLOAT m_fActionRadius = 0.0f,
 14 FLOAT m_fActionTime = 0.0f,
 15 FLOAT m_fDiffMultiply = 0.0f,
 16 FLOAT m_fUpMultiply = 0.0f,
 20 BOOL  m_bFadeOut = FALSE,
 21 FLOAT m_fFadeStartTime = 1e6,
 22 FLOAT m_fFadeTime = 2.0f,
 23 FLOAT m_fStartTime = 0.0f,

 50 CSoundObject m_soSpin,  // sound channel for spinning

components:

  1 class   CLASS_SPINNER         "Classes\\Spinner.ecl",
 
 10 model   MODEL_TWISTER         "ModelsMP\\Enemies\\AirElemental\\Twister.mdl",
 11 texture TEXTURE_TWISTER       "ModelsMP\\Enemies\\AirElemental\\Twister.tex",

200 sound   SOUND_SPIN            "ModelsMP\\Enemies\\AirElemental\\Sounds\\TwisterSpin.wav",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiTwister;
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    return;
  };

  // render burning particles
  void RenderParticles(void)
  {
    if(m_bMovingAllowed)
    {
      Particles_Twister(this, m_fSize/15.0f, m_fStartTime, m_fFadeStartTime, 1.0f);
    }
    else
    {
      CEntity *penParent=GetParent();
      FLOAT fStretch=1.0f;
      if(penParent!=NULL)
      {
        CAirElemental *penAir=(CAirElemental *)penParent;
        FLOAT fStretchRatio=penAir->GetCurrentStretchRatio();
        fStretch=1.0f+(fStretchRatio)*6.0f;
      }
      Particles_Twister(this, m_fSize/15.0f*fStretch, m_fStartTime, m_fFadeStartTime, 0.5f*fStretch);
    }
  }

/************************************************************
 *                   FADE OUT & MOVING                      *
 ************************************************************/
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient) {
    // fading out
    if (m_bFadeOut) {
      FLOAT fTimeRemain = m_fFadeStartTime + m_fFadeTime - _pTimer->CurrentTick();
      if (fTimeRemain < 0.0f) { fTimeRemain = 0.0f; }
      COLOR colAlpha = GetModelObject()->mo_colBlendColor;
      colAlpha = (colAlpha&0xffffff00) + (COLOR(fTimeRemain/m_fFadeTime*0xff)&0xff);
      GetModelObject()->mo_colBlendColor = colAlpha;
    }
    return CMovableModelEntity::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  };


/************************************************************
 *                   ATTACK SPECIFIC                        *
 ************************************************************/
  
  void SpinEntity(CEntity *pen) {
    
    // don't spin air elemental and other twisters and any items
    if (IsOfClass(pen, "AirElemental") || IsOfClass(pen, "Twister")
        || IsDerivedFromClass(pen, "Item")) {
      return;
    }
    // don't spin air elementals wind blast
    if (IsOfClass(pen, "Projectile")) {
      if (((CProjectile *)&*pen)->m_prtType==PRT_AIRELEMENTAL_WIND)
      {
        return; 
      }
    }
    


    if (pen->GetPhysicsFlags()&EPF_MOVABLE) {
      // if any other spinner affects the target, skip this spinner
      BOOL bNoSpinner = TRUE;
      {FOREACHINLIST( CEntity, en_lnInParent, pen->en_lhChildren, iten) {
        if (IsOfClass(iten, "Spinner"))
        {
          bNoSpinner = FALSE;
          return;      
        }
      }}
      if (bNoSpinner) {    
        ESpinnerInit esi;
        CEntityPointer penSpinner;
        esi.penParent = pen;
        esi.penTwister = this;
        esi.bImpulse = FALSE;

        // spin projectiles a bit longer but not so high
        if (IsOfClass(pen, "Projectile"))
        {
          switch(((CProjectile &)*pen).m_prtType) {
          case PRT_GRENADE:
          case PRT_HEADMAN_BOMBERMAN:
          case PRT_DEMON_FIREBALL:
          case PRT_SHOOTER_FIREBALL:
          case PRT_BEAST_PROJECTILE:
          case PRT_BEAST_BIG_PROJECTILE:
          case PRT_LAVA_COMET:
            esi.tmSpinTime = 2.5f;
            esi.vRotationAngle = ANGLE3D(-m_sgnSpinDir*250.0f, 0, 0);
            esi.fUpSpeed = m_fDiffMultiply*0.75;
            break;
          default:
            esi.tmSpinTime = 1.5f;
            esi.vRotationAngle = ANGLE3D(-m_sgnSpinDir*180.0f, 0, 0);
            esi.fUpSpeed = m_fDiffMultiply/5.0f;
            break;
          }
        // cannon ball - short but powerfull
        } else if (IsOfClass(pen, "Cannon ball")){
          esi.tmSpinTime = 0.2f;
          esi.vRotationAngle = ANGLE3D(-m_sgnSpinDir*500.0f, 0, 0);
          esi.fUpSpeed = m_fDiffMultiply*3.0f;          
        // don't take it easy with players
        } else if (IsOfClass(pen, "Player")){
          esi.tmSpinTime = 3.0f;
          esi.vRotationAngle = ANGLE3D(-m_sgnSpinDir*220.0f, 0, 0);
          esi.bImpulse = TRUE;
          esi.fUpSpeed = m_fDiffMultiply*(0.4f + FRnd()*0.4f);
          esi.tmImpulseDuration = 1.4f + FRnd()*0.5f;
        // everything else
        } else {
          esi.tmSpinTime = 0.5f;
          esi.vRotationAngle = ANGLE3D(-m_sgnSpinDir*180.0f, 0, 0);
          esi.fUpSpeed = m_fDiffMultiply;
        }
        penSpinner = CreateEntity(pen->GetPlacement(), CLASS_SPINNER);
        penSpinner->Initialize(esi);
        penSpinner->SetParent(pen);
      }
      // damage
      FLOAT3D vDirection;
      AnglesToDirectionVector(GetPlacement().pl_OrientationAngle, vDirection);
      InflictDirectDamage(pen, m_penOwner, DMT_IMPACT, 2.0f, GetPlacement().pl_PositionVector, vDirection);
    }
    
  };
  
  void PreMoving(void) {
    // moving - rotate speed direction
    if (m_bMoving) {
      FLOATmatrix3D m;
      ANGLE3D aRotation;
      aRotation = m_aSpeedRotation*(_pTimer->CurrentTick()-m_tmLastMove);
      MakeRotationMatrix(m, aRotation);
      m_vSpeed = m_vSpeed*m;
      SetDesiredTranslation(m_vSpeed);
      m_tmLastMove = _pTimer->CurrentTick();
    }
    CMovableModelEntity::PreMoving();
  };
 
  
/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  // --->>> MAIN
  Main(ETwister et) {
    // remember the initial parameters
    ASSERT(et.penOwner!=NULL);
    m_penOwner = et.penOwner;
    m_sgnSpinDir = et.sgnSpinDir;
    if (m_sgnSpinDir==0) { m_sgnSpinDir=1; };
    m_fSize = et.fSize;
    m_fStopTime = _pTimer->CurrentTick() + et.fDuration;
    m_bGrow = et.bGrow;
    m_bMovingAllowed = et.bMovingAllowed;
    
    // initialization
    InitAsEditorModel();
    SetPhysicsFlags(EPF_TWISTER);
    SetCollisionFlags(ECF_TWISTER);
    SetFlags(GetFlags() | ENF_SEETHROUGH);
    SetModel(MODEL_TWISTER);
    SetModelMainTexture(TEXTURE_TWISTER);

    // some twister parameters
    m_fActionRadius = pow(m_fSize, 0.33333f)*10.0f;
    m_fActionTime = m_fActionRadius;
    m_fUpMultiply = m_fActionRadius/2.0f;
    m_fDiffMultiply = sqrt(m_fSize);
    GetModelObject()->StretchModel(FLOAT3D(m_fSize, m_fSize, m_fSize));
    ModelChangeNotify();

    m_fStartTime=_pTimer->CurrentTick();
    
    //wait for some randome time
    autowait(FRnd()*0.25f);
 
    m_soSpin.Set3DParameters(50.0f, 10.0f, 1.0f, 1.0f);
    PlaySound(m_soSpin, SOUND_SPIN, SOF_3D|SOF_LOOP);
    
    // immediately rotate
    SetDesiredRotation(ANGLE3D(m_sgnSpinDir*(FRnd()*50.0f+50.0f), 0.0f, 0.0f));
    
    if (m_bGrow) {
      StartModelAnim(TWISTER_ANIM_GROWING, AOF_SMOOTHCHANGE|AOF_NORESTART);
    }
    autowait(GetModelObject()->GetAnimLength(TWISTER_ANIM_GROWING));
    
    // beginning random speed
    FLOAT fR = FRndIn(5.0f, 10.0f);
    FLOAT fA = FRnd()*360.0f;
    m_vSpeed = FLOAT3D(CosFast(fA)*fR, 0, SinFast(fA)*fR);
    m_bMoving = m_bMovingAllowed;

    // move in range
    while(_pTimer->CurrentTick() < m_fStopTime) {
      FLOAT fMoveTime = FRndIn(2.0f, 4.0f);
      m_aSpeedRotation = FLOAT3D(FRndIn(8.0f, 16.0f), 0.0f, 0.0f);
      m_tmLastMove = _pTimer->CurrentTick();
      // NOTE: fMoveTime will cause the twister to stop existing
      // a bit later then the m_fStopTime, but what the heck?
      wait(fMoveTime) {
        on (EBegin) : { resume; }
        on (ETimer) : { stop; }
        on (EPass ep) : {
          if (ep.penOther->GetRenderType()&RT_MODEL &&
            ep.penOther->GetPhysicsFlags()&EPF_MOVABLE &&
            !IsOfClass(ep.penOther, "Twister")) {
            SpinEntity(ep.penOther);
          }
          resume;
        }
      }     
    }

    // fade out
    m_fFadeStartTime = _pTimer->CurrentTick();
    m_bFadeOut = TRUE;
    m_fFadeTime = 2.0f;
    autowait(m_fFadeTime);

    // cease to exist
    Destroy();

    return;
  }
};
