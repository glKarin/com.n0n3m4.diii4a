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

343
%{
#include "EntitiesMP/StdH/StdH.h"
//#include "ModelsMP/Enemies/Grunt/Grunt.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/BasicEffects";

enum GruntSkaType {
  0 GT_SOLDIER    "Grunt soldier",
  1 GT_COMMANDER  "Grunt commander",
};

%{
#define STRETCH_SOLDIER   1.2f
#define STRETCH_COMMANDER 1.4f
  
// info structure
static EntityInfo eiGruntSoldier = {
  EIBT_FLESH, 200.0f,
  0.0f, 1.9f*STRETCH_SOLDIER, 0.0f,     // source (eyes)
  0.0f, 1.3f*STRETCH_SOLDIER, 0.0f,     // target (body)
};

static EntityInfo eiGruntCommander = {
  EIBT_FLESH, 250.0f,
  0.0f, 1.9f*STRETCH_COMMANDER, 0.0f,     // source (eyes)
  0.0f, 1.3f*STRETCH_COMMANDER, 0.0f,     // target (body)
};

#define FIREPOS_SOLDIER       FLOAT3D(0.07f, 1.36f, -0.78f)*STRETCH_SOLDIER
#define FIREPOS_COMMANDER_UP  FLOAT3D(0.09f, 1.45f, -0.62f)*STRETCH_COMMANDER
#define FIREPOS_COMMANDER_DN  FLOAT3D(0.10f, 1.30f, -0.60f)*STRETCH_COMMANDER

#define COMANDER_SMC_MODEL "ModelsSKA\\Enemies\\Grunt\\Commander.smc"
#define SOLIDER_SMC_MODEL "ModelsSKA\\Enemies\\Grunt\\Grunt.smc"

#define GRUNT_MESH      
#define GRUNT_SKELETON  
#define GRUNT_ANIMSET   
#define GRUNT_TEXTURE   

#define CLEAR_ANIM_TIME   0.2f

static INDEX idGrunt_Wound          = -1;
static INDEX idGrunt_Run            = -1;
static INDEX idGrunt_IdlePatrol     = -1;
static INDEX idGrunt_IdleAttack     = -1;
static INDEX idGrunt_Fire           = -1;
static INDEX idGrunt_Default        = -1;
static INDEX idGrunt_DeathForward   = -1;
static INDEX idGrunt_DeathBackward  = -1;
static INDEX idGrunt_GunModel       = -1;

static INDEX idGrund_NormalBox      = -1;
static INDEX idGrund_DeathBox       = -1;

static CTextureObject _toStar01;

#define SHP_BASE_TEXTURE 0

%}


class CGruntSka: CEnemyBase {
name      "GruntSka";
thumbnail "Thumbnails\\Grunt.tbn";

properties:
  1 enum GruntSkaType m_gtType "Type" 'Y' = GT_SOLDIER,

  10 CSoundObject m_soFire1,
  11 CSoundObject m_soFire2,
  20 FLOAT        m_fMidBoneRot = 0.0f,
  30 CModelInstance m_miTest,


// class internal
    
components:
  1 class   CLASS_BASE            "Classes\\EnemyBase.ecl",
  3 class   CLASS_PROJECTILE      "Classes\\Projectile.ecl",

// ************** SOUNDS **************
 50 sound   SOUND_IDLE            "ModelsMP\\Enemies\\Grunt\\Sounds\\Idle.wav",
 52 sound   SOUND_SIGHT           "ModelsMP\\Enemies\\Grunt\\Sounds\\Sight.wav",
 53 sound   SOUND_WOUND           "ModelsMP\\Enemies\\Grunt\\Sounds\\Wound.wav",
 57 sound   SOUND_FIRE            "ModelsMP\\Enemies\\Grunt\\Sounds\\Fire.wav",
 58 sound   SOUND_DEATH           "ModelsMP\\Enemies\\Grunt\\Sounds\\Death.wav",

functions:

  void CGruntSka(void)
  {
    // Get animation id's
    idGrunt_Wound          = ska_GetIDFromStringTable("Grunt_Wound");
    idGrunt_Run            = ska_GetIDFromStringTable("Grunt_Run");
    idGrunt_IdlePatrol     = ska_GetIDFromStringTable("Grunt_IdlePatrol");
    idGrunt_IdleAttack     = ska_GetIDFromStringTable("Grunt_IdleAttack");
    idGrunt_Fire           = ska_GetIDFromStringTable("Grunt_Fire");
    idGrunt_Default        = ska_GetIDFromStringTable("Grunt_Default");
    idGrunt_DeathForward   = ska_GetIDFromStringTable("Grunt_DeathForward");
    idGrunt_DeathBackward  = ska_GetIDFromStringTable("Grunt_DeathBackward");
    idGrunt_GunModel       = ska_GetIDFromStringTable("Flamer");
    
    // Get colision box id's
    idGrund_NormalBox      = ska_GetIDFromStringTable("Normal");
    idGrund_DeathBox       = ska_GetIDFromStringTable("Death");
  };

  void CreateTestModelInstance()
  {
    try {
      m_miTest.AddMesh_t((CTString)"ModelsSKA\\Test\\Arm\\Arm.bm");
      m_miTest.AddSkeleton_t((CTString)"ModelsSKA\\Test\\Arm\\Arm.bs");
      m_miTest.AddAnimSet_t((CTString)"ModelsSKA\\Test\\Arm\\Arm.ba");
      m_miTest.AddTexture_t((CTString)"ModelsSKA\\Test\\Arm\\Objects\\Arm.tex","Arm",NULL);
      m_miTest.AddColisionBox("Default",FLOAT3D(-0.5f,0.0f,-0.5f),FLOAT3D(0.5f,2.0f,0.5f));
    } catch ( const char *strErr) {
      FatalError(strErr);
    }
  }

  void BuildGruntModel()
  {
    // CreateTestModelInstance();

    en_pmiModelInstance = CreateModelInstance("GruntSka");
    CModelInstance *pmi = GetModelInstance();
    try{
      // setup grunt solider
      pmi->AddMesh_t((CTString)"ModelsSKA\\Enemies\\Grunt\\Grunt.bm");
      pmi->AddSkeleton_t((CTString)"ModelsSKA\\Enemies\\Grunt\\Grunt.bs");
      pmi->AddAnimSet_t((CTString)"ModelsSKA\\Enemies\\Grunt\\Grunt.ba");
      pmi->AddTexture_t((CTString)"ModelsSKA\\Enemies\\Grunt\\Soldier.tex","Grunt",NULL);
      pmi->AddColisionBox("Default",FLOAT3D(-0.5f,0.0f,-0.5f),FLOAT3D(0.5f,2.0f,0.5f));
      
      // setup weapon
      CModelInstance *pmiFlamer = CreateModelInstance("Flamer");
      pmiFlamer->AddMesh_t((CTString)"ModelsSKA\\Weapons\\Flamer\\Flamer.bm");
      pmiFlamer->AddSkeleton_t((CTString)"ModelsSKA\\Weapons\\Flamer\\Flamer.bs");
      pmiFlamer->AddAnimSet_t((CTString)"ModelsSKA\\Weapons\\Flamer\\Flamer.ba");
      pmiFlamer->AddTexture_t((CTString)"ModelsSKA\\Weapons\\Flamer\\Flamer.tex","Flamer",NULL);
      pmiFlamer->AddTexture_t((CTString)"ModelsSKA\\Enemies\\Grunt\\Lava04FX.tex","Lava04FX",NULL);
      // Set flamer offset
      pmiFlamer->SetOffsetRot(ANGLE3D(0,0,180));
      // Attach flamer to grunt
      pmi->AddChild(pmiFlamer);

      // Set flamer parent bone
      INDEX iParenBoneID = ska_GetIDFromStringTable("R_Hand");
      pmiFlamer->SetParentBone(iParenBoneID);

      // Set colision info
      SetSkaColisionInfo();
    } catch ( const char *strErr) {
      FatalError(strErr);
    }
  };

  void BuildCommanderModel(CEntity *penGrunt)
  {
    SetSkaModel("ModelsSKA\\Enemies\\Grunt\\CommanderNoGun.smc");
    CModelInstance *pmiFlamer = NULL;
    try{
      pmiFlamer = ParseSmcFile_t("ModelsSKA\\Weapons\\Flamer\\Flamer.smc");
    } catch ( const char *strErr) {
      FatalError(strErr);
    }
    // Set flamer parent bone
    INDEX iParenBoneID = ska_GetIDFromStringTable("R_Hand");
    CModelInstance *pmi = GetModelInstance();
    pmi->AddChild(pmiFlamer);
    pmiFlamer->SetParentBone(iParenBoneID);
    // Set flamer offset
    pmiFlamer->SetOffsetRot(ANGLE3D(0,0,180));
    // Set colision info
    SetSkaColisionInfo();
  };
/*
  void Particles_OneParticle( FLOAT3D vPos )
  {
    Particle_PrepareTexture(&_toStar01, PBT_ADDALPHA);
    Particle_SetTexturePart( 512, 512, 0, 0);
         
    COLOR col = RGBAToColor(128, 128, 128, 128);
    Particle_RenderSquare( vPos, 1.0f, 0.0f, col);
    Particle_Flush();
  }

  void RenderParticles(void) {
    INDEX iBoneID = ska_GetIDFromStringTable("R_Hand");
    FLOAT3D vStartPoint;
    FLOAT3D vEndPoint;
    if(GetBoneAbsPosition(iBoneID,vStartPoint,vEndPoint)) {
      Particles_OneParticle(vStartPoint);
    }
  };
  */
  /*
  void AdjustBones()
  {
    INDEX iBoneID = ska_GetIDFromStringTable("MidTorso");
    RenBone *rb = RM_FindRenBone(iBoneID);
    if(rb!=NULL) {
      FLOATquat3D quat;
      quat.FromEuler(ANGLE3D(0,0,AngleRad(Sin(m_fMidBoneRot)/3.0f)));
      rb->rb_arRot.ar_qRot = quat;
    }
    m_fMidBoneRot+=1;
  };
*/
  /*
  void AdjustBones()
  {
    INDEX ctrb = 0;
    RenBone *pRenBones=RM_GetRenBoneArray(ctrb);
    // for each t ren bones after first dummy one
    for(INDEX irb=1;irb<ctrb-3;irb+=3) {
      RenBone &rb = pRenBones[irb];
      FLOATquat3D quat;
      quat.FromEuler(ANGLE3D(0,0,AngleRad(Sin(m_fMidBoneRot)/3.0f)));
      rb.rb_arRot.ar_qRot = quat;
    }
    m_fMidBoneRot+=1;
  }
*/
  /*
  void AdjustShaderParams(INDEX iSurfaceID,CShader *pShader,ShaderParams &spParams)
  {
    INDEX iFlamerMeshID = ska_GetIDFromStringTable("Top");


    if(iSurfaceID == iFlamerMeshID) {
      if(pShader != NULL) {
        ShaderDesc sdDesc;
        pShader->GetShaderDesc(sdDesc);
        if(sdDesc.sd_astrTextureNames.Count() > SHP_BASE_TEXTURE) {
          spParams.sp_aiTextureIDs[SHP_BASE_TEXTURE] = ska_GetIDFromStringTable("Lava04FX");
        }
      }
    }
  }
  */
/*
  CModelInstance *GetModelInstanceForRendering()
  {
    return &m_miTest;
  };
*/

  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("A Grunt sent %s into the halls of Valhalla"), (const char *) strPlayerName);
    return str;
  }

  /* Entity info */
  void *GetEntityInfo(void) {
    if (m_gtType==GT_SOLDIER) {
      return &eiGruntSoldier;
    } else if (m_gtType==GT_COMMANDER) {
      return &eiGruntSoldier;
    } else {
      ASSERTALWAYS("Unknown grunt type!");
      return NULL;
    }
  };

  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnmSoldier,     "DataMP\\Messages\\Enemies\\GruntSoldier.txt");
    static DECLARE_CTFILENAME(fnmCommander,   "DataMP\\Messages\\Enemies\\GruntCommander.txt");
    switch(m_gtType) {
    default: ASSERT(FALSE);
    case GT_SOLDIER:  return fnmSoldier;
    case GT_COMMANDER: return fnmCommander;
    }
  };

  void Precache(void) {
    CEnemyBase::Precache();
    
   if (m_gtType==GT_SOLDIER) {
      PrecacheClass(CLASS_PROJECTILE, PRT_GRUNT_PROJECTILE_SOL);
    }
    if (m_gtType==GT_COMMANDER) {
      PrecacheClass(CLASS_PROJECTILE, PRT_GRUNT_PROJECTILE_COM);
    }

    PrecacheSound(SOUND_IDLE);
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_FIRE);
    PrecacheSound(SOUND_DEATH);
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
  };

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    GetModelInstance()->AddAnimation(idGrunt_Wound,AN_CLEAR,1,0);
    return idGrunt_Wound;
  };

  // death
  INDEX AnimForDeath(void) {
    INDEX idAnimDeath;
    FLOAT3D vFront;
    GetHeadingDirection(0, vFront);
    FLOAT fDamageDir = m_vDamage%vFront;
    if (fDamageDir<0) {
      idAnimDeath = idGrunt_DeathBackward;
    } else {
      idAnimDeath = idGrunt_DeathForward;
    }

    GetModelInstance()->AddAnimation(idAnimDeath,AN_CLEAR,1,0);

    return idAnimDeath;
  };

  FLOAT WaitForDust(FLOAT3D &vStretch) {

    vStretch=FLOAT3D(1,1,2);
    if(GetModelInstance()->IsAnimationPlaying(idGrunt_DeathBackward)) {
      return 0.5f;
    } else if(GetModelInstance()->IsAnimationPlaying(idGrunt_DeathForward)) {
      return 1.0f;
    }
    return -1.0f;
  };

  void DeathNotify(void) {
    INDEX iBoxIndex = GetModelInstance()->GetColisionBoxIndex(idGrund_DeathBox);
    ASSERT(iBoxIndex>=0);
    ChangeCollisionBoxIndexWhenPossible(iBoxIndex);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    GetModelInstance()->AddAnimation(idGrunt_IdleAttack,AN_LOOPING|AN_NORESTART|AN_CLEAR,1,0);
  };
  /*void StandingAnimFight(void)
  {
    StartModelAnim(HEADMAN_ANIM_IDLE_FIGHT, AOF_LOOPING|AOF_NORESTART);
  }*/
  void RunningAnim(void) {
    GetModelInstance()->AddAnimation(idGrunt_Run,AN_LOOPING|AN_NORESTART|AN_CLEAR,1,0);
  };
    void WalkingAnim(void) {
    RunningAnim();
  };
  void RotatingAnim(void) {
    RunningAnim();
  };

  // virtual sound functions
  void IdleSound(void) {
    PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };
  void SightSound(void) {
    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D);
  };
  void WoundSound(void) {
    PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
  };
  void DeathSound(void) {
    PlaySound(m_soSound, SOUND_DEATH, SOF_3D);
  };

  // adjust sound and watcher parameters here if needed
  void EnemyPostInit(void) 
  {
    // set sound default parameters
    m_soFire1.Set3DParameters(160.0f, 50.0f, 1.0f, 1.0f);
    m_soFire2.Set3DParameters(160.0f, 50.0f, 1.0f, 1.0f);
  };

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  Fire(EVoid) : CEnemyBase::Fire {
    // soldier
    if (m_gtType == GT_SOLDIER) {
      autocall SoldierAttack() EEnd;
    // commander
    } else if (m_gtType == GT_COMMANDER) {
      autocall CommanderAttack() EEnd;
    // should never get here
    } else{
      ASSERT(FALSE);
    }
    return EReturn();
  };
    
  // Soldier attack
  SoldierAttack(EVoid) {

    StandingAnimFight();
    autowait(0.2f + FRnd()*0.25f);

    GetModelInstance()->AddAnimation(idGrunt_Fire,AN_CLEAR,1.0f,0);

    ShootProjectile(PRT_GRUNT_PROJECTILE_SOL, FIREPOS_SOLDIER, ANGLE3D(0, 0, 0));
    PlaySound(m_soFire1, SOUND_FIRE, SOF_3D);

    autowait(0.15f + FRnd()*0.1f);
    GetModelInstance()->AddAnimation(idGrunt_Fire,AN_CLEAR,1.0f,0);
    ShootProjectile(PRT_GRUNT_PROJECTILE_SOL, FIREPOS_SOLDIER, ANGLE3D(0, 0, 0));
    PlaySound(m_soFire2, SOUND_FIRE, SOF_3D);
    

    autowait(FRnd()*0.333f);
    return EEnd();
  };

  // Commander attack (predicted firing on moving player)
  CommanderAttack(EVoid) {
    StandingAnimFight();
    autowait(0.2f + FRnd()*0.25f);

    /*FLOAT3D vGunPosAbs   = GetPlacement().pl_PositionVector + FLOAT3D(0.0f, 1.0f, 0.0f)*GetRotationMatrix();
    FLOAT3D vEnemySpeed  = ((CMovableEntity&) *m_penEnemy).en_vCurrentTranslationAbsolute;
    FLOAT3D vEnemyPos    = ((CMovableEntity&) *m_penEnemy).GetPlacement().pl_PositionVector;
    FLOAT   fLaserSpeed  = 45.0f; // m/s
    FLOAT3D vPredictedEnemyPosition = CalculatePredictedPosition(vGunPosAbs,
      vEnemyPos, fLaserSpeed, vEnemySpeed, GetPlacement().pl_PositionVector(2) );
    ShootPredictedProjectile(PRT_GRUNT_LASER, vPredictedEnemyPosition, FLOAT3D(0.0f, 1.0f, 0.0f), ANGLE3D(0, 0, 0));*/

    GetModelInstance()->AddAnimation(idGrunt_Fire,AN_CLEAR,1,0);
    ShootProjectile(PRT_GRUNT_PROJECTILE_COM, FIREPOS_COMMANDER_DN, ANGLE3D(-20, 0, 0));
    PlaySound(m_soFire1, SOUND_FIRE, SOF_3D);

    autowait(0.035f);
    GetModelInstance()->AddAnimation(idGrunt_Fire,AN_CLEAR,1,0);
    ShootProjectile(PRT_GRUNT_PROJECTILE_COM, FIREPOS_COMMANDER_DN, ANGLE3D(-10, 0, 0));
    PlaySound(m_soFire2, SOUND_FIRE, SOF_3D);

    autowait(0.035f);
    GetModelInstance()->AddAnimation(idGrunt_Fire,AN_CLEAR,1,0);
    ShootProjectile(PRT_GRUNT_PROJECTILE_COM, FIREPOS_COMMANDER_DN, ANGLE3D(0, 0, 0));
    PlaySound(m_soFire1, SOUND_FIRE, SOF_3D);

    autowait(0.035f);
    GetModelInstance()->AddAnimation(idGrunt_Fire,AN_CLEAR,1,0);
    ShootProjectile(PRT_GRUNT_PROJECTILE_COM, FIREPOS_COMMANDER_DN, ANGLE3D(10, 0, 0));
    PlaySound(m_soFire2, SOUND_FIRE, SOF_3D);

    autowait(0.035f);
    GetModelInstance()->AddAnimation(idGrunt_Fire,AN_CLEAR,1,0);
    ShootProjectile(PRT_GRUNT_PROJECTILE_COM, FIREPOS_COMMANDER_DN, ANGLE3D(20, 0, 0));
    PlaySound(m_soFire2, SOUND_FIRE, SOF_3D);

    autowait(FRnd()*0.5f);
    return EEnd();
  };

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsSkaModel();
    SetPhysicsFlags(EPF_MODEL_WALKING|EPF_HASLUNGS);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    en_tmMaxHoldBreath = 5.0f;
    en_fDensity = 2000.0f;
    //m_fBlowUpSize = 2.0f;

    //_toStar01.SetData_t(CTFILENAME("Models\\Items\\Particles\\Star01.tex"));


    // set your appearance
    switch (m_gtType) {
      case GT_SOLDIER:
        SetSkaModel(SOLIDER_SMC_MODEL);
        // BuildGruntModel();

        // setup moving speed
        m_fWalkSpeed = FRnd() + 2.5f;
        m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 500.0f);
        m_fAttackRunSpeed = FRnd() + 6.5f;
        m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
        m_fCloseRunSpeed = FRnd() + 6.5f;
        m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
        // setup attack distances
        m_fAttackDistance = 80.0f;
        m_fCloseDistance = 0.0f;
        m_fStopDistance = 8.0f;
        m_fAttackFireTime = 2.0f;
        m_fCloseFireTime = 1.0f;
        m_fIgnoreRange = 200.0f;
        //m_fBlowUpAmount = 65.0f;
        m_fBlowUpAmount = 80.0f;
        m_fBodyParts = 4;
        m_fDamageWounded = 0.0f;
        m_iScore = 500;
        SetHealth(40.0f);
        m_fMaxHealth = 40.0f;
        // set stretch factors for height and width
        GetModelInstance()->StretchModel(FLOAT3D(STRETCH_SOLDIER, STRETCH_SOLDIER, STRETCH_SOLDIER));
        break;
  
      case GT_COMMANDER:

        SetSkaModel(COMANDER_SMC_MODEL);
        // BuildCommanderModel();

        m_fWalkSpeed = FRnd() + 2.5f;
        m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 500.0f);
        m_fAttackRunSpeed = FRnd() + 8.0f;
        m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
        m_fCloseRunSpeed = FRnd() + 8.0f;
        m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
        // setup attack distances
        m_fAttackDistance = 90.0f;
        m_fCloseDistance = 0.0f;
        m_fStopDistance = 15.0f;
        m_fAttackFireTime = 4.0f;
        m_fCloseFireTime = 2.0f;
        //m_fBlowUpAmount = 180.0f;
        m_fIgnoreRange = 200.0f;
        // damage/explode properties
        m_fBodyParts = 5;
        m_fDamageWounded = 0.0f;
        m_iScore = 800;
        SetHealth(60.0f);
        m_fMaxHealth = 60.0f;
        // set stretch factors for height and width
        GetModelInstance()->StretchModel(FLOAT3D(STRETCH_COMMANDER, STRETCH_COMMANDER, STRETCH_COMMANDER));
        break;
    }

    ModelChangeNotify();
    StandingAnim();

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
