/*
 * This file is generated by Entity Class Compiler, (c) CroTeam 1997-98
 */

#line 17 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"

#include "Entities/StdH/StdH.h"

#include <Entities/RobotFlying.h>
#include <Entities/RobotFlying_tables.h>
#line 28 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"

static EntityInfo eiRobotFlying = {
  EIBT_ROBOT, 100.0f, // mass[kg]
  0.0f, 0.0f, 0.0f,  // source  
  0.0f, 0.0f, 0.0f,  // target  
};

#define FIRE_POS      FLOAT3D(0.0f, 0.0f, 0.0f)

void CRobotFlying::SetDefaultProperties(void) {
  m_rfcChar = RFC_FIGHTER ;
  CEnemyFly::SetDefaultProperties();
}
  
#line 64 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void * CRobotFlying::GetEntityInfo(void) {
#line 65 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
return & eiRobotFlying ;
#line 66 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
  
#line 69 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void CRobotFlying::ReceiveDamage(CEntity * penInflictor,enum DamageType dmtType,
#line 70 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
FLOAT fDamageAmmount,const FLOAT3D & vHitPoint,const FLOAT3D & vDirection) 
#line 71 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
{
#line 72 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
if(! IsOfSameClass  (penInflictor  , this )){
#line 73 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
CEnemyFly  :: ReceiveDamage  (penInflictor  , dmtType  , fDamageAmmount  , vHitPoint  , vDirection );
#line 74 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
#line 75 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
  
#line 78 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void CRobotFlying::StandingAnim(void) {
#line 80 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
  
#line 81 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void CRobotFlying::WalkingAnim(void) {
#line 83 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
  
#line 84 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void CRobotFlying::RunningAnim(void) {
#line 86 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
  
#line 87 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void CRobotFlying::RotatingAnim(void) {
#line 89 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
  
#line 92 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void CRobotFlying::IdleSound(void) {
#line 93 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
PlaySound  (m_soSound  , SOUND_IDLE  , SOF_3D );
#line 94 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
  
#line 95 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void CRobotFlying::SightSound(void) {
#line 96 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
PlaySound  (m_soSound  , SOUND_SIGHT  , SOF_3D );
#line 97 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
  
#line 98 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void CRobotFlying::WoundSound(void) {
#line 99 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
PlaySound  (m_soSound  , SOUND_WOUND  , SOF_3D );
#line 100 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
  
#line 101 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
void CRobotFlying::DeathSound(void) {
#line 102 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
PlaySound  (m_soSound  , SOUND_DEATH  , SOF_3D );
#line 103 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
BOOL CRobotFlying::
#line 110 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
FlyHit(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CRobotFlying_FlyHit
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EVoid, "CRobotFlying::FlyHit expects 'EVoid' as input!");  const EVoid &e = (const EVoid &)__eeInput;
#line 111 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
if(m_rfcChar  == RFC_FIGHTER ){
#line 112 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
Jump(STATE_CURRENT, STATE_CRobotFlying_FlyFire, TRUE, EVoid());return TRUE;
#line 115 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
#line 118 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
if(CalcDist  (m_penEnemy ) <= 3.0f){
#line 120 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetHealth  (- 45.0f);
#line 121 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
ReceiveDamage  (NULL  , DMT_EXPLOSION  , 10.0f , FLOAT3D (0 , 0 , 0) , FLOAT3D (0 , 1 , 0));
#line 122 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
InflictRangeDamage  (this  , DMT_EXPLOSION  , 20.0f , GetPlacement  () . pl_PositionVector  , 
#line 123 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
2.75f , 8.0f);
#line 124 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
#line 127 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fShootTime  = _pTimer  -> CurrentTick  () + 0.1f;
#line 128 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
Return(STATE_CURRENT,EReturn  ());
#line 128 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
return TRUE; ASSERT(FALSE); return TRUE;};BOOL CRobotFlying::
#line 131 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
FlyFire(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CRobotFlying_FlyFire
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EVoid, "CRobotFlying::FlyFire expects 'EVoid' as input!");  const EVoid &e = (const EVoid &)__eeInput;
#line 132 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
if(m_rfcChar  == RFC_KAMIKAZE ){
#line 133 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fShootTime  = _pTimer  -> CurrentTick  () + 1.0f;
#line 134 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
Return(STATE_CURRENT,EReturn  ());
#line 134 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
return TRUE;
#line 135 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
#line 137 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
ShootProjectile  (PRT_CYBORG_LASER  , FIRE_POS  , ANGLE3D (0 , 0 , 0));
#line 138 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
PlaySound  (m_soSound  , SOUND_FIRE  , SOF_3D );
#line 140 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
Return(STATE_CURRENT,EReturn  ());
#line 140 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
return TRUE; ASSERT(FALSE); return TRUE;};BOOL CRobotFlying::
#line 143 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
Death(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CRobotFlying_Death
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EVoid, "CRobotFlying::Death expects 'EVoid' as input!");  const EVoid &e = (const EVoid &)__eeInput;
#line 145 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
StopMoving  ();
#line 146 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
DeathSound  ();
#line 149 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetPhysicsFlags  (EPF_MODEL_CORPSE );
#line 150 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetCollisionFlags  (ECF_CORPSE );
#line 153 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
Return(STATE_CURRENT,EEnd  ());
#line 153 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
return TRUE; ASSERT(FALSE); return TRUE;};BOOL CRobotFlying::
#line 159 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
Main(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CRobotFlying_Main
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EVoid, "CRobotFlying::Main expects 'EVoid' as input!");  const EVoid &e = (const EVoid &)__eeInput;
#line 161 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_EeftType  = EFT_FLY_ONLY ;
#line 164 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
InitAsModel  ();
#line 165 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetPhysicsFlags  (EPF_MODEL_WALKING );
#line 166 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetCollisionFlags  (ECF_MODEL );
#line 167 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetFlags  (GetFlags  () | ENF_ALIVE );
#line 168 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetHealth  (20.0f);
#line 169 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fMaxHealth  = 20.0f;
#line 170 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
en_fDensity  = 13000.0f;
#line 172 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fBlowUpAmount  = 0.0f;
#line 173 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fBodyParts  = 4;
#line 174 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_bRobotBlowup  = TRUE ;
#line 175 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fDamageWounded  = 100000.0f;
#line 178 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
switch(m_rfcChar ){
#line 179 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
case RFC_KAMIKAZE : {
#line 180 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetModel  (MODEL_KAMIKAZE );
#line 181 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetModelMainTexture  (TEXTURE_KAMIKAZE );
#line 183 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyWalkSpeed  = FRnd  () / 2 + 1.0f;
#line 184 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_aFlyWalkRotateSpeed  = FRnd  () * 10.0f + 25.0f;
#line 185 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyAttackRunSpeed  = FRnd  () * 2.0f + 8.0f;
#line 186 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_aFlyAttackRotateSpeed  = FRnd  () * 25 + 150.0f;
#line 187 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyCloseRunSpeed  = FRnd  () * 2.0f + 6.0f;
#line 188 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_aFlyCloseRotateSpeed  = FRnd  () * 50 + 500.0f;
#line 190 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyAttackDistance  = 50.0f;
#line 191 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyCloseDistance  = 12.5f;
#line 192 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyStopDistance  = 0.0f;
#line 193 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyAttackFireTime  = 2.0f;
#line 194 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyCloseFireTime  = 0.1f;
#line 195 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyIgnoreRange  = 200.0f;
#line 196 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyHeight  = 1.0f;
#line 197 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_iScore  = 1000;
#line 198 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}break ;
#line 199 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
case RFC_FIGHTER : {
#line 200 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetModel  (MODEL_FIGHTER );
#line 201 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
SetModelMainTexture  (TEXTURE_FIGHTER );
#line 203 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyWalkSpeed  = FRnd  () / 2 + 1.0f;
#line 204 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_aFlyWalkRotateSpeed  = FRnd  () * 10.0f + 25.0f;
#line 205 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyAttackRunSpeed  = FRnd  () * 2.0f + 7.0f;
#line 206 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_aFlyAttackRotateSpeed  = FRnd  () * 25 + 150.0f;
#line 207 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyCloseRunSpeed  = FRnd  () * 2.0f + 20.0f;
#line 208 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_aFlyCloseRotateSpeed  = 150.0f;
#line 210 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyAttackDistance  = 50.0f;
#line 211 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyCloseDistance  = 10.0f;
#line 212 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyStopDistance  = 0.1f;
#line 213 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyAttackFireTime  = 3.0f;
#line 214 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyCloseFireTime  = 0.2f;
#line 215 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyIgnoreRange  = 200.0f;
#line 216 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
m_fFlyHeight  = 2.5f;
#line 217 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}break ;
#line 218 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
default  : ASSERT  (FALSE );
#line 219 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
}
#line 222 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RobotFlying.es"
Jump(STATE_CURRENT, STATE_CEnemyFly_MainLoop, FALSE, EVoid());return TRUE; ASSERT(FALSE); return TRUE;};