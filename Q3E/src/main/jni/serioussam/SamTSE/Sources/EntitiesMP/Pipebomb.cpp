/*
 * This file is generated by Entity Class Compiler, (c) CroTeam 1997-98
 */

#line 17 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"

#include "EntitiesMP/StdH/StdH.h"

#include <EntitiesMP/Pipebomb.h>
#include <EntitiesMP/Pipebomb_tables.h>
CEntityEvent *EDropPipebomb::MakeCopy(void) { CEntityEvent *peeCopy = new EDropPipebomb(*this); return peeCopy;}
EDropPipebomb::EDropPipebomb() : CEntityEvent(EVENTCODE_EDropPipebomb) {;
 ClearToDefault(penLauncher);
 ClearToDefault(fSpeed);
};
#line 31 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"

#define ECF_PIPEBOMB ( \
  ((ECBI_MODEL|ECBI_BRUSH|ECBI_PROJECTILE_SOLID|ECBI_MODEL_HOLDER)<<ECB_TEST) |\
  ((ECBI_PROJECTILE_SOLID)<<ECB_IS) )

void CPipebomb_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) 
{
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_GRENADE);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_EXPLOSIONSTAIN);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_SHOCKWAVE);
  pdec->PrecacheClass(CLASS_BASIC_EFFECT, BET_GRENADE_PLANE);
  pdec->PrecacheModel(MODEL_PIPEBOMB);
  pdec->PrecacheTexture(TEXTURE_PIPEBOMB);
  pdec->PrecacheSound(SOUND_LAUNCH);
}

void CPipebomb::SetDefaultProperties(void) {
  m_penLauncher = NULL;
  m_fIgnoreTime = 0.0f;
  m_fSpeed = 0.0f;
  m_bCollected = FALSE ;
  m_penPrediction = NULL;
  CMovableModelEntity::SetDefaultProperties();
}
  
#line 74 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
void CPipebomb::Read_t(CTStream * istr) 
#line 75 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
{
#line 76 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
CMovableModelEntity  :: Read_t  (istr );
#line 77 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SetupLightSource  ();
#line 78 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
  
#line 81 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
CLightSource * CPipebomb::GetLightSource(void) 
#line 82 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
{
#line 83 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
if(! IsPredictor  ()){
#line 84 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
return & m_lsLightSource ;
#line 85 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}else {
#line 86 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
return NULL ;
#line 87 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
#line 88 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
  
#line 91 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
void CPipebomb::SetupLightSource(void) 
#line 92 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
{
#line 94 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
CLightSource  lsNew ;
#line 95 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
lsNew  . ls_ulFlags  = LSF_NONPERSISTENT  | LSF_DYNAMIC ;
#line 96 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
lsNew  . ls_colColor  = C_vdRED ;
#line 97 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
lsNew  . ls_rFallOff  = 1.0f;
#line 98 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
lsNew  . ls_rHotSpot  = 0.1f;
#line 99 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
lsNew  . ls_plftLensFlare  = & _lftYellowStarRedRingFar ;
#line 100 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
lsNew  . ls_ubPolygonalMask  = 0;
#line 101 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
lsNew  . ls_paoLightAnimation  = NULL ;
#line 103 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
m_lsLightSource  . ls_penEntity  = this ;
#line 104 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
m_lsLightSource  . SetLightSource  (lsNew );
#line 105 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
  
#line 108 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
void CPipebomb::RenderParticles(void) {
#line 109 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
Particles_GrenadeTrail  (this );
#line 110 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
  
#line 115 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
void CPipebomb::Pipebomb(void) {
#line 117 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
InitAsModel  ();
#line 118 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SetPhysicsFlags  (EPF_MODEL_BOUNCING );
#line 119 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SetCollisionFlags  (ECF_PIPEBOMB );
#line 122 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SetModel  (MODEL_PIPEBOMB );
#line 123 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SetModelMainTexture  (TEXTURE_PIPEBOMB );
#line 125 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
LaunchAsFreeProjectile  (FLOAT3D (0.0f , 0.0f , - m_fSpeed ) , (CMovableEntity  *) m_penLauncher  . ep_pen );
#line 126 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SetDesiredRotation  (ANGLE3D (0 , FRnd  () * 120.0f + 120.0f , FRnd  () * 250.0f - 125.0f));
#line 127 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
en_fBounceDampNormal  = 0.7f;
#line 128 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
en_fBounceDampParallel  = 0.7f;
#line 129 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
en_fJumpControlMultiplier  = 0.0f;
#line 130 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
en_fCollisionSpeedLimit  = 45.0f;
#line 131 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
en_fCollisionDamageFactor  = 10.0f;
#line 132 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SetHealth  (20.0f);
#line 133 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
  
#line 135 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
void CPipebomb::PipebombExplosion(void) {
#line 136 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ESpawnEffect  ese ;
#line 137 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
FLOAT3D vPoint ;
#line 138 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
FLOATplane3D vPlaneNormal ;
#line 139 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
FLOAT fDistanceToEdge ;
#line 142 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ese  . colMuliplier  = C_WHITE  | CT_OPAQUE ;
#line 143 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ese  . betType  = BET_GRENADE ;
#line 144 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ese  . vStretch  = FLOAT3D (1 , 1 , 1);
#line 145 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SpawnEffect  (GetPlacement  () , ese );
#line 147 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
if(IsDerivedFromClass  (m_penLauncher  , "Player")){
#line 148 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SpawnRangeSound  (m_penLauncher  , this  , SNDT_PLAYER  , 50.0f);
#line 149 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
#line 152 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
if(GetNearestPolygon  (vPoint  , vPlaneNormal  , fDistanceToEdge )){
#line 153 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
if((vPoint  - GetPlacement  () . pl_PositionVector ) . Length  () < 3.5f){
#line 155 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ese  . betType  = BET_EXPLOSIONSTAIN ;
#line 156 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ese  . vNormal  = FLOAT3D (vPlaneNormal );
#line 157 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SpawnEffect  (CPlacement3D (vPoint  , ANGLE3D (0 , 0 , 0)) , ese );
#line 159 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ese  . betType  = BET_SHOCKWAVE ;
#line 160 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ese  . vNormal  = FLOAT3D (vPlaneNormal );
#line 161 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SpawnEffect  (CPlacement3D (vPoint  , ANGLE3D (0 , 0 , 0)) , ese );
#line 163 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ese  . betType  = BET_GRENADE_PLANE ;
#line 164 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ese  . vNormal  = FLOAT3D (vPlaneNormal );
#line 165 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SpawnEffect  (CPlacement3D (vPoint  + ese  . vNormal  / 50.0f , ANGLE3D (0 , 0 , 0)) , ese );
#line 166 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
#line 167 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
#line 168 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
  
#line 177 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
void CPipebomb::ProjectileHit(void) {
#line 179 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
InflictRangeDamage  (m_penLauncher  , DMT_EXPLOSION  , 100.0f , 
#line 180 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
GetPlacement  () . pl_PositionVector  , 4.0f , 8.0f);
#line 182 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ESound  eSound ;
#line 183 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
eSound  . EsndtSound  = SNDT_EXPLOSION ;
#line 184 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
eSound  . penTarget  = m_penLauncher ;
#line 185 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SendEventInRange  (eSound  , FLOATaabbox3D (GetPlacement  () . pl_PositionVector  , 50.0f));
#line 186 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
  
#line 190 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
void CPipebomb::SpawnEffect(const CPlacement3D & plEffect,const ESpawnEffect & eSpawnEffect) {
#line 191 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
CEntityPointer penEffect  = CreateEntity  (plEffect  , CLASS_BASIC_EFFECT );
#line 192 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
penEffect  -> Initialize  (eSpawnEffect );
#line 193 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
BOOL CPipebomb::
#line 203 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ProjectileSlide(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CPipebomb_ProjectileSlide
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EVoid, "CPipebomb::ProjectileSlide expects 'EVoid' as input!");  const EVoid &e = (const EVoid &)__eeInput;
#line 204 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
m_bCollected  = FALSE ;
#line 206 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SetTimerAt(THINKTIME_NEVER);
Jump(STATE_CURRENT, 0x01f70002, FALSE, EBegin());return TRUE;}BOOL CPipebomb::H0x01f70002_ProjectileSlide_01(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT 0x01f70002
switch(__eeInput.ee_slEvent){case(EVENTCODE_EBegin):{const EBegin&e= (EBegin&)__eeInput;
return TRUE;}ASSERT(FALSE);break;case(EVENTCODE_ETouch):{const ETouch&etouch= (ETouch&)__eeInput;

#line 211 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
if(etouch  . penOther  -> GetRenderType  () & RT_BRUSH ){
#line 212 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
m_fIgnoreTime  = 0.0f;
#line 213 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
return TRUE;
#line 214 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
#line 216 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
BOOL bCollect ;
#line 218 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
bCollect  = etouch  . penOther  != m_penLauncher  || _pTimer  -> CurrentTick  () > m_fIgnoreTime ;
#line 220 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
bCollect  &= (en_vCurrentTranslationAbsolute  . Length  () < 0.25f);
#line 222 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
EAmmoItem  eai ;
#line 223 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
eai  . EaitType  = AIT_GRENADES ;
#line 224 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
eai  . iQuantity  = 1;
#line 225 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
if(bCollect  && etouch  . penOther  -> ReceiveItem  (eai )){
#line 226 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
m_bCollected  = TRUE ;
#line 227 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
UnsetTimer();Jump(STATE_CURRENT,0x01f70003, FALSE, EInternal());return TRUE;
#line 228 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
#line 229 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
return TRUE;
#line 230 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}ASSERT(FALSE);break;case(EVENTCODE_EDeath):{const EDeath&e= (EDeath&)__eeInput;

#line 233 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ProjectileHit  ();
#line 234 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
UnsetTimer();Jump(STATE_CURRENT,0x01f70003, FALSE, EInternal());return TRUE;
#line 235 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}ASSERT(FALSE);break;case(EVENTCODE_EStart):{const EStart&e= (EStart&)__eeInput;

#line 238 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ProjectileHit  ();
#line 239 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
UnsetTimer();Jump(STATE_CURRENT,0x01f70003, FALSE, EInternal());return TRUE;
#line 240 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}ASSERT(FALSE);break;default: return FALSE; break;
#line 241 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}return TRUE;}BOOL CPipebomb::H0x01f70003_ProjectileSlide_02(const CEntityEvent &__eeInput){
ASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);
#undef STATE_CURRENT
#define STATE_CURRENT 0x01f70003

#line 242 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
Return(STATE_CURRENT,EEnd  ());
#line 242 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
return TRUE; ASSERT(FALSE); return TRUE;};BOOL CPipebomb::
#line 246 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
Main(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CPipebomb_Main
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EDropPipebomb, "CPipebomb::Main expects 'EDropPipebomb' as input!");  const EDropPipebomb &edrop = (const EDropPipebomb &)__eeInput;
#line 248 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
ASSERT  (edrop  . penLauncher  != NULL );
#line 249 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
m_penLauncher  = edrop  . penLauncher ;
#line 250 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
m_fSpeed  = edrop  . fSpeed ;
#line 253 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
Pipebomb  ();
#line 256 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
SetupLightSource  ();
#line 259 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
m_fIgnoreTime  = _pTimer  -> CurrentTick  () + 0.5f;
#line 262 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
STATE_CPipebomb_ProjectileSlide, TRUE;
Jump(STATE_CURRENT, 0x01f70004, FALSE, EBegin());return TRUE;}BOOL CPipebomb::H0x01f70004_Main_01(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT 0x01f70004
switch(__eeInput.ee_slEvent) {case EVENTCODE_EBegin: Call(STATE_CURRENT, STATE_CPipebomb_ProjectileSlide, TRUE, EVoid());return TRUE;case EVENTCODE_EEnd: Jump(STATE_CURRENT,0x01f70005, FALSE, __eeInput); return TRUE;default: return FALSE; }}BOOL CPipebomb::H0x01f70005_Main_02(const CEntityEvent &__eeInput){
#undef STATE_CURRENT
#define STATE_CURRENT 0x01f70005
const EEnd&__e= (EEnd&)__eeInput;
;
#line 265 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
if(! m_bCollected ){
#line 266 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
PipebombExplosion  ();
#line 267 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
}
#line 269 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
Destroy  ();
#line 271 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
Return(STATE_CURRENT,EVoid());
#line 271 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Pipebomb.es"
return TRUE; ASSERT(FALSE); return TRUE;};