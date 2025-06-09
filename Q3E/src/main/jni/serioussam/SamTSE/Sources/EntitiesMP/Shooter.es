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

345
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/ModelHolder2";
uses "EntitiesMP/Projectile";
uses "EntitiesMP/SoundHolder";
uses "EntitiesMP/BloodSpray";
uses "EntitiesMP/CannonBall";

enum FireType {
  0 SFT_WOODEN_DART "Wooden dart",
  1 SFT_FIRE        "Fire",
  2 SFT_GAS         "-none-",
  3 SFT_IRONBALL    "Ironball",
  4 SFT_FIREBALL    "Fireball",
};

class CShooter: CModelHolder2 {
name      "Shooter";
thumbnail "Thumbnails\\Shooter.tbn";
features  "HasName", "IsTargetable";

properties:
  
  2  FLOAT m_fShootingPeriod                "Shooting Period" = 1.0f,
  5  enum FireType m_sftType                "Type" 'Y' = SFT_WOODEN_DART,
  7  FLOAT m_fHealth                        "Health" = 0.0f,
  8  FLOAT m_fCannonBallSize                "Cannon/fire ball size" = 1.0f,
  9  FLOAT m_fCannonBallPower               "Cannon/fire ball power" = 10.0f, 
 10 ANIMATION m_iModelPreFireAnimation     "Model pre-fire animation" = 0,
 11 ANIMATION m_iTexturePreFireAnimation   "Texture pre-fire animation" = 0,
 12 ANIMATION m_iModelPostFireAnimation    "Model post-fire animation" = 0,
 13 ANIMATION m_iTexturePostFireAnimation  "Texture post-fire animation" = 0,
 14 FLOAT m_fFlameBurstDuration            "Flame burst duration" = 1.0f,
 15 FLOAT m_fRndBeginWait                  "Random begin wait time" = 0.0f,

 20 CEntityPointer m_penSoundLaunch        "Sound launch",   // sound when firing
 21 CSoundObject m_soLaunch,  
  
 30 CEntityPointer m_penFlame,
 //internal:

 50 BOOL m_bFiring = FALSE,
 51 BOOL m_bIndestructable = FALSE,

 60 FLOAT m_tmFlameStart = 0.0f,

components:
  1 class CLASS_PROJECTILE    "Classes\\Projectile.ecl",
  2 class CLASS_BLOOD_SPRAY   "Classes\\BloodSpray.ecl",
  3 class CLASS_CANNONBALL    "Classes\\CannonBall.ecl",

functions:                                        
  
  void Precache(void) {
    CModelHolder2::Precache();
    PrecacheClass(CLASS_PROJECTILE, PRT_SHOOTER_WOODEN_DART);
    PrecacheClass(CLASS_PROJECTILE, PRT_SHOOTER_FIREBALL);
    PrecacheClass(CLASS_CANNONBALL);
  };

  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // receive damage if not indestructable, and shooter can't hurt another shooter
    if (!m_bIndestructable && !IsOfClass(penInflictor, "Shooter")) {
      if (m_tmSpraySpawned<=_pTimer->CurrentTick()-_pTimer->TickQuantum*8 
          && m_penDestruction!=NULL) {
        
        CModelDestruction *penDestruction = GetDestruction();
        
        // spawn blood spray
        CPlacement3D plSpray = CPlacement3D( vHitPoint, ANGLE3D(0, 0, 0));
        m_penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
        m_penSpray->SetParent(this);
        ESpawnSpray eSpawnSpray;
        eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
        
        // adjust spray power
        if( fDamageAmmount > 50.0f) {
          eSpawnSpray.fDamagePower = 3.0f;
        } else if(fDamageAmmount > 25.0f ) {
          eSpawnSpray.fDamagePower = 2.0f;
        } else {
          eSpawnSpray.fDamagePower = 1.0f;
        }
        
        // remember spray type
        eSpawnSpray.sptType = penDestruction->m_sptType;
        eSpawnSpray.fSizeMultiplier = 1.0f;
        
        // get your down vector (simulates gravity)
        FLOAT3D vDn(-en_mRotation(1,2), -en_mRotation(2,2), -en_mRotation(3,2));
        
        // setup direction of spray
        FLOAT3D vHitPointRelative = vHitPoint - GetPlacement().pl_PositionVector;
        FLOAT3D vReflectingNormal;
        GetNormalComponent( vHitPointRelative, vDn, vReflectingNormal);
        vReflectingNormal.Normalize();
        
        vReflectingNormal(1)/=5.0f;
        
        FLOAT3D vProjectedComponent = vReflectingNormal*(vDirection%vReflectingNormal);
        FLOAT3D vSpilDirection = vDirection-vProjectedComponent*2.0f-vDn*0.5f;
        
        eSpawnSpray.vDirection = vSpilDirection;
        eSpawnSpray.penOwner = this;
        
        // initialize spray
        m_penSpray->Initialize( eSpawnSpray);
        m_tmSpraySpawned = _pTimer->CurrentTick();
      }
  
      CRationalEntity::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    }
  }

  // render particles
  void RenderParticles(void)
  {
    // fire particles
    if (m_sftType==SFT_FIRE) {
    }
    CModelHolder2::RenderParticles();
  }

  /* Get anim data for given animation property - return NULL for none. */
  CAnimData *GetAnimData(SLONG slPropertyOffset) 
  {
    if (slPropertyOffset==_offsetof(CShooter, m_iModelPreFireAnimation) ||
        slPropertyOffset==_offsetof(CShooter, m_iModelPostFireAnimation)) {
      return GetModelObject()->GetData();
    } else if (slPropertyOffset==_offsetof(CShooter, m_iTexturePreFireAnimation) ||
               slPropertyOffset==_offsetof(CShooter, m_iTexturePostFireAnimation)) {
      return GetModelObject()->mo_toTexture.GetData();
    } else {
      return CModelHolder2::GetAnimData(slPropertyOffset);
    }
  }

  // shoot projectile on enemy
  CEntity *ShootProjectile(enum ProjectileType pt, const FLOAT3D &vOffset, const ANGLE3D &aOffset) {
    // launch
    CPlacement3D pl;
    pl = GetPlacement();
    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = pt;
    penProjectile->Initialize(eLaunch);

    return penProjectile;
  };

  // fire flame
  void FireFlame(void) {
    // flame start position
    CPlacement3D plFlame;
    plFlame = GetPlacement();
    
    FLOAT3D vNormDir;
    AnglesToDirectionVector(plFlame.pl_OrientationAngle, vNormDir);
    plFlame.pl_PositionVector += vNormDir*0.1f;
    
    // create flame
    CEntityPointer penFlame = CreateEntity(plFlame, CLASS_PROJECTILE);
    // init and launch flame
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_SHOOTER_FLAME;
    penFlame->Initialize(eLaunch);
    // link last flame with this one (if not NULL or deleted)
    if (m_penFlame!=NULL && !(m_penFlame->GetFlags()&ENF_DELETED)) {
      ((CProjectile&)*m_penFlame).m_penParticles = penFlame;
    }
    // link to this
    ((CProjectile&)*penFlame).m_penParticles = this;
    // store last flame
    m_penFlame = penFlame;
  };

  void StopFlame(void) {
    ((CProjectile&)*m_penFlame).m_penParticles = NULL;
    //m_penFlame = NULL;
  }

  void PlayFireSound(void) {
    // if sound entity exists
    if (m_penSoundLaunch!=NULL) {
      CSoundHolder &sh = (CSoundHolder&)*m_penSoundLaunch;
      m_soLaunch.Set3DParameters(FLOAT(sh.m_rFallOffRange), FLOAT(sh.m_rHotSpotRange), sh.m_fVolume, 1.0f);
      PlaySound(m_soLaunch, sh.m_fnSound, sh.m_iPlayType);
    }
  };

  void ShootCannonball()
  {
    // cannon ball start position
    CPlacement3D plBall = GetPlacement();
    // create cannon ball
    CEntityPointer penBall = CreateEntity(plBall, CLASS_CANNONBALL);
    // init and launch cannon ball
    ELaunchCannonBall eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.fLaunchPower = 10.0f+m_fCannonBallPower; // ranges from 50-150 (since iPower can be max 100)
    eLaunch.cbtType = CBT_IRON;
    eLaunch.fSize = m_fCannonBallSize;
    penBall->Initialize(eLaunch);
  };

  void ShootFireball()
  {
    // cannon ball start position
    CPlacement3D plBall = GetPlacement();
    // create cannon ball
    CEntityPointer penBall = CreateEntity(plBall, CLASS_CANNONBALL);
    // init and launch cannon ball
    ELaunchCannonBall eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.fLaunchPower = 10.0f+m_fCannonBallPower; // ranges from 50-150 (since iPower can be max 100)
    eLaunch.cbtType = CBT_IRON;
    eLaunch.fSize = m_fCannonBallSize;
    penBall->Initialize(eLaunch);
  };

procedures:
  
  FireOnce()
  {
    if (m_sftType==SFT_FIRE) { jump FlameBurst(); }
    
    PlayFireSound();

    GetModelObject()->PlayAnim(m_iModelPreFireAnimation, 0);
    GetModelObject()->mo_toTexture.PlayAnim(m_iTexturePreFireAnimation, 0);
    autowait(GetModelObject()->GetAnimLength(m_iModelPreFireAnimation));
    
    switch (m_sftType) {
      case SFT_WOODEN_DART:
        ShootProjectile(PRT_SHOOTER_WOODEN_DART, FLOAT3D (0.0f, 0.0f, 0.0f), ANGLE3D (0.0f, 0.0f, 0.0f));
        break;
      case SFT_GAS:
        break;
      case SFT_IRONBALL:
        ShootCannonball();
        break;
      case SFT_FIREBALL:
        ShootProjectile(PRT_SHOOTER_FIREBALL, FLOAT3D (0.0f, 0.0f, 0.0f), ANGLE3D (0.0f, 0.0f, 0.0f));
        break;
    }
    
    GetModelObject()->PlayAnim(m_iModelPostFireAnimation, 0);
    GetModelObject()->mo_toTexture.PlayAnim(m_iTexturePostFireAnimation, 0);
    autowait(GetModelObject()->GetAnimLength(m_iModelPostFireAnimation));
    
    return EEnd();
  }
  
  FireContinuous() {
    
    // possible random wait
    if (m_fRndBeginWait>0.0f)
    {
      FLOAT fRndWait = FRnd()*m_fRndBeginWait+0.05f;
      autowait(fRndWait);
    }

    while(m_bFiring) {
      autocall FireOnce() EEnd;
      autowait(m_fShootingPeriod);
    }
    return EReturn();
  };

  FlameBurst() {
    PlayFireSound();
    m_penFlame = NULL;
    m_tmFlameStart = _pTimer->CurrentTick();
    while(_pTimer->CurrentTick( ) < m_tmFlameStart + m_fFlameBurstDuration)
    {
      // wait a bit and fire 
      autowait(0.05f);
      FireFlame();
    }
    StopFlame();
    return EEnd();
  };

  MainLoop() {
    //main loop
    wait() {
      on (EBegin) : {
        resume;
      }
      on (ETrigger) : {
        if (!m_bFiring) {
          call FireOnce();
        } else {
          resume;
        }
      }
      on (EActivate) : {
        m_bFiring = TRUE;
        call FireContinuous();
      }
      on (EDeactivate) : {
        m_bFiring = FALSE;
        resume;
      }
      on (EDeath) : {
        if (m_penDestruction!=NULL) {
          jump CModelHolder2::Die();
        } else {
          Destroy();
          return;
        }
      }
      on (EReturn) : {
        resume;
      }
    }
  };

  Main() {
    // init as model
    CModelHolder2::InitModelHolder();

    if (m_fHealth>0.0f) {
      SetHealth(m_fHealth);
      m_bIndestructable = FALSE;
    } else {
      SetHealth(10000.0f);
      m_bIndestructable = TRUE;
    }

    ClampUp(m_fCannonBallSize, 10.0f);
    ClampDn(m_fCannonBallSize, 0.1f);
    ClampUp(m_fCannonBallPower, 100.0f);
    ClampDn(m_fCannonBallPower, 0.0f);
    
    if (m_penSoundLaunch!=NULL && !IsOfClass(m_penSoundLaunch, "SoundHolder")) {
      WarningMessage( "Entity '%s' is not of class SoundHolder!", (const char *) m_penSoundLaunch->GetName());
      m_penSoundLaunch=NULL;
    }
    if (m_penDestruction!=NULL && !IsOfClass(m_penDestruction, "ModelDestruction")) {
      WarningMessage( "Entity '%s' is not of class ModelDestruction!", (const char *) m_penDestruction->GetName());
      m_penDestruction=NULL;
    }

    autowait(_pTimer->TickQuantum);

    jump MainLoop();
    
    return;
  };
};
