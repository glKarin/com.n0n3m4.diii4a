/*
 * This file is generated by Entity Class Compiler, (c) CroTeam 1997-98
 */

#line 17 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"

#include "Entities/StdH/StdH.h"

#include <Entities/RollingStone.h>
#include <Entities/RollingStone_tables.h>
void CRollingStone::SetDefaultProperties(void) {
  m_fBounce = 0.5f;
  m_fHealth = 400.0f;
  m_fDamage = 1000.0f;
  m_bFixedDamage = FALSE ;
  m_fStretch = 1.0f;
  m_fDeceleration = 0.9f;
  m_fStartSpeed = 50.0f;
  m_vStartDir = ANGLE3D(0 , 0 , 0);
  m_soBounce0.SetOwner(this);
m_soBounce0.Stop_internal();
  m_soBounce1.SetOwner(this);
m_soBounce1.Stop_internal();
  m_soBounce2.SetOwner(this);
m_soBounce2.Stop_internal();
  m_soBounce3.SetOwner(this);
m_soBounce3.Stop_internal();
  m_soBounce4.SetOwner(this);
m_soBounce4.Stop_internal();
  m_iNextChannel = 0;
  m_soRoll.SetOwner(this);
m_soRoll.Stop_internal();
  m_bRollPlaying = FALSE ;
  m_qA = FLOATquat3D(0 , 1 , 0 , 0);
  m_qALast = FLOATquat3D(0 , 1 , 0 , 0);
  m_fASpeed = 0.0f;
  m_vR = FLOAT3D(0 , 0 , 1);
  CMovableModelEntity::SetDefaultProperties();
}
  
#line 67 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
void CRollingStone::Precache(void) 
#line 68 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 69 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
PrecacheClass  (CLASS_DEBRIS );
#line 70 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
PrecacheModel  (MODEL_STONE );
#line 71 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
PrecacheTexture  (TEXTURE_STONE );
#line 72 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
PrecacheSound  (SOUND_BOUNCE );
#line 73 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
PrecacheSound  (SOUND_ROLL );
#line 74 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
  
#line 75 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
void CRollingStone::PostMoving() {
#line 76 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
CMovableModelEntity  :: PostMoving  ();
#line 79 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(en_penReference  != NULL ){
#line 81 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
AdjustSpeeds  (en_vReferencePlane );
#line 83 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}else {
#line 85 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 88 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_qALast  = m_qA ;
#line 90 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOATquat3D qRot ;
#line 91 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
qRot  . FromAxisAngle  (m_vR  , m_fASpeed  * _pTimer  -> TickQuantum  * PI  / 180);
#line 92 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOATmatrix3D mRot ;
#line 93 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
qRot  . ToMatrix  (mRot );
#line 94 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_qA  = qRot  * m_qA ;
#line 95 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(en_ulFlags  & ENF_INRENDERING ){
#line 96 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_qALast  = m_qA ;
#line 97 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 98 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
  
#line 101 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
void CRollingStone::AdjustMipFactor(FLOAT & fMipFactor) 
#line 102 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 103 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
fMipFactor  = 0;
#line 105 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOATquat3D qA ;
#line 106 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
qA  = Slerp  (_pTimer  -> GetLerpFactor  () , m_qALast  , m_qA );
#line 108 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOATmatrix3D mA ;
#line 109 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
qA  . ToMatrix  (mA );
#line 110 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
ANGLE3D vA ;
#line 111 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
DecomposeRotationMatrixNoSnap  (vA  , mA );
#line 113 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
CAttachmentModelObject  * amo  = GetModelObject  () -> GetAttachmentModel  (0);
#line 114 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
amo  -> amo_plRelative  . pl_OrientationAngle  = vA ;
#line 115 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
  
#line 117 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
void CRollingStone::AdjustSpeedOnOneAxis(FLOAT & fTraNow,FLOAT & aRotNow,BOOL bRolling) 
#line 118 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 125 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fR  = 4.0f * m_fStretch ;
#line 127 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fTraNew  = (2 * aRotNow  * fR  + 5 * fTraNow ) / 7;
#line 128 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT aRotNew  = fTraNew  / fR ;
#line 130 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
fTraNow  = fTraNew ;
#line 131 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
aRotNow  = aRotNew ;
#line 132 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
  
#line 135 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
void CRollingStone::AdjustSpeeds(const FLOAT3D & vPlane) 
#line 136 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 138 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(en_vCurrentTranslationAbsolute  . Length  () < 1.0f && m_fASpeed  < 1.0f){
#line 140 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
en_vCurrentTranslationAbsolute  = FLOAT3D (0 , 0 , 0);
#line 141 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_fASpeed  = 0.0f;
#line 142 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
RollSound  (0.0f);
#line 143 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
return ;
#line 144 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 147 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D vTranslationNormal ;
#line 148 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D vTranslationParallel ;
#line 149 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
GetParallelAndNormalComponents  (en_vCurrentTranslationAbsolute  , vPlane  , vTranslationNormal  , vTranslationParallel );
#line 152 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
BOOL bRolling  = vTranslationNormal  . Length  () < 0.1f;
#line 154 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(bRolling ){
#line 156 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fSpeedTra  = vTranslationParallel  . Length  ();
#line 160 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
RollSound  (fSpeedTra );
#line 161 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}else {
#line 162 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
RollSound  (0);
#line 163 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 169 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D vRotFromRot  = m_vR ;
#line 170 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D vTraFromRot  = vPlane  * vRotFromRot ;
#line 171 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
vTraFromRot  . Normalize  ();
#line 173 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fTraFromRot  = 0;
#line 174 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fRotFromRot  = m_fASpeed  * PI  / 180.0f;
#line 177 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D vTraFromTra  = vTranslationParallel ;
#line 178 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fTraFromTra  = vTraFromTra  . Length  ();
#line 179 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D vRotFromTra  = FLOAT3D (1 , 0 , 0);
#line 180 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fRotFromTra  = 0;
#line 181 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(fTraFromTra  > 0.001f){
#line 182 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
vTraFromTra  /= fTraFromTra ;
#line 183 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
vRotFromTra  = vTraFromTra  * vPlane ;
#line 184 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
vRotFromTra  . Normalize  ();
#line 185 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 188 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(Abs  (fRotFromRot ) > 0.01f){
#line 190 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
AdjustSpeedOnOneAxis  (fTraFromRot  , fRotFromRot  , bRolling );
#line 191 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 193 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(Abs  (fTraFromTra ) > 0.01f){
#line 195 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
AdjustSpeedOnOneAxis  (fTraFromTra  , fRotFromTra  , bRolling );
#line 196 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 199 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOATquat3D qTra ;
#line 200 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
qTra  . FromAxisAngle  (vRotFromTra  , fRotFromTra );
#line 201 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOATquat3D qRot ;
#line 202 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
qRot  . FromAxisAngle  (vRotFromRot  , fRotFromRot );
#line 203 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOATquat3D q  = qRot  * qTra ;
#line 204 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D vSpeed  = vTraFromTra  * fTraFromTra  + vTraFromRot  * fTraFromRot ;
#line 207 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
en_vCurrentTranslationAbsolute  = vTranslationNormal  + vSpeed ;
#line 208 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
q  . ToAxisAngle  (m_vR  , m_fASpeed );
#line 209 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_fASpeed  *= 180 / PI ;
#line 210 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
  
#line 215 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
void CRollingStone::BounceSound(FLOAT fSpeed) {
#line 216 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fHitStrength  = fSpeed  * fSpeed ;
#line 218 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fVolume  = fHitStrength  / 20.0f;
#line 220 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
fVolume  = Clamp  (fVolume  , 0.0f , 2.0f);
#line 222 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fPitch  = Lerp  (0.2f , 1.0f , Clamp  (fHitStrength  / 100 , 0.0f , 1.0f));
#line 223 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(fVolume  < 0.1f){
#line 224 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
return ;
#line 225 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 226 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
CSoundObject & so  = (& m_soBounce0 ) [ m_iNextChannel  ];
#line 227 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_iNextChannel  = (m_iNextChannel  + 1) % 5;
#line 228 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
so  . Set3DParameters  (200.0f * m_fStretch  , 100.0f * m_fStretch  , fVolume  , fPitch );
#line 229 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
PlaySound  (so  , SOUND_BOUNCE  , SOF_3D );
#line 230 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
  
#line 232 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
void CRollingStone::RollSound(FLOAT fSpeed) 
#line 233 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 234 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fHitStrength  = fSpeed  * fSpeed  * m_fStretch  * m_fStretch  * m_fStretch ;
#line 236 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fVolume  = fHitStrength  / 20.0f;
#line 237 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
fVolume  = Clamp  (fVolume  , 0.0f , 1.0f);
#line 238 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fPitch  = Lerp  (0.2f , 1.0f , Clamp  (fHitStrength  / 100 , 0.0f , 1.0f));
#line 239 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(fVolume  < 0.1f){
#line 240 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(m_bRollPlaying ){
#line 241 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_soRoll  . Stop  ();
#line 242 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_bRollPlaying  = FALSE ;
#line 243 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 244 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
return ;
#line 245 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 246 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_soRoll  . Set3DParameters  (200.0f * m_fStretch  , 100.0f * m_fStretch  , fVolume  , fPitch );
#line 248 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(! m_bRollPlaying ){
#line 249 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
PlaySound  (m_soRoll  , SOUND_ROLL  , SOF_3D  | SOF_LOOP );
#line 250 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_bRollPlaying  = TRUE ;
#line 251 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 252 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
BOOL CRollingStone::
#line 256 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
Main(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CRollingStone_Main
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EVoid, "CRollingStone::Main expects 'EVoid' as input!");  const EVoid &e = (const EVoid &)__eeInput;
#line 259 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
InitAsModel  ();
#line 260 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
SetPhysicsFlags  (EPF_ONBLOCK_BOUNCE  | EPF_PUSHABLE  | EPF_MOVABLE  | EPF_TRANSLATEDBYGRAVITY );
#line 261 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
SetCollisionFlags  (ECF_MODEL );
#line 262 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
SetModel  (MODEL_ROLLINGSTONE );
#line 263 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
SetModelMainTexture  (TEXTURE_ROLLINGSTONE );
#line 264 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
AddAttachmentToModel  (this  , * GetModelObject  () , 0 , MODEL_STONESPHERE  , TEXTURE_ROLLINGSTONE  , 0 , 0 , TEXTURE_DETAIL );
#line 266 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
GetModelObject  () -> StretchModel  (FLOAT3D (m_fStretch  , m_fStretch  , m_fStretch ));
#line 267 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
ModelChangeNotify  ();
#line 269 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
en_fBounceDampNormal  = m_fBounce ;
#line 270 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
en_fBounceDampParallel  = m_fBounce ;
#line 271 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
en_fAcceleration  = en_fDeceleration  = m_fDeceleration ;
#line 272 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
en_fCollisionSpeedLimit  = 45.0f;
#line 273 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
en_fCollisionDamageFactor  = 10.0f;
#line 275 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
SetPlacement  (CPlacement3D (GetPlacement  () . pl_PositionVector  , ANGLE3D (0 , 0 , 0)));
#line 276 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_qA  = FLOATquat3D (0 , 1 , 0 , 0);
#line 277 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
m_qALast  = FLOATquat3D (0 , 1 , 0 , 0);
#line 279 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
SetTimerAfter(0.1f);
Jump(STATE_CURRENT, 0x025c0000, FALSE, EBegin());return TRUE;}BOOL CRollingStone::H0x025c0000_Main_01(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT 0x025c0000
switch(__eeInput.ee_slEvent) {case EVENTCODE_EBegin: return TRUE;case EVENTCODE_ETimer: Jump(STATE_CURRENT,0x025c0001, FALSE, EInternal()); return TRUE;default: return FALSE; }}BOOL CRollingStone::H0x025c0001_Main_02(const CEntityEvent &__eeInput){
ASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);
#undef STATE_CURRENT
#define STATE_CURRENT 0x025c0001
;
#line 281 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
SetHealth  (m_fHealth );
#line 282 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
AddToMovers  ();
#line 284 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
SetTimerAt(THINKTIME_NEVER);
Jump(STATE_CURRENT, 0x025c0002, FALSE, EBegin());return TRUE;}BOOL CRollingStone::H0x025c0002_Main_03(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT 0x025c0002
switch(__eeInput.ee_slEvent){case(EVENTCODE_ETrigger):{const ETrigger&e= (ETrigger&)__eeInput;

#line 286 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D v ;
#line 287 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
AnglesToDirectionVector  (m_vStartDir  , v );
#line 288 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
GiveImpulseTranslationAbsolute  (v  * m_fStartSpeed );
#line 290 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
return TRUE;
#line 291 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}ASSERT(FALSE);break;case(EVENTCODE_ETouch):
#line 293 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{const ETouch&eTouch= (ETouch&)__eeInput;

#line 297 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(! m_bFixedDamage )
#line 298 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 299 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fDamageFactor  = en_vCurrentTranslationAbsolute  . Length  () / 10.0f;
#line 300 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fAppliedDamage  = fDamageFactor  * m_fDamage ;
#line 302 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
InflictDirectDamage  (eTouch  . penOther  , this  , DMT_CANNONBALL  , fAppliedDamage  , 
#line 303 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
eTouch  . penOther  -> GetPlacement  () . pl_PositionVector  , eTouch  . plCollision );
#line 304 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 305 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
else 
#line 306 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 307 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(en_vCurrentTranslationAbsolute  . Length  () != 0.0f)
#line 308 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 310 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
InflictDirectDamage  (eTouch  . penOther  , this  , DMT_CANNONBALL  , m_fDamage  , 
#line 311 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
eTouch  . penOther  -> GetPlacement  () . pl_PositionVector  , eTouch  . plCollision );
#line 312 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 313 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 316 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
AdjustSpeeds  (eTouch  . plCollision );
#line 319 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(eTouch  . penOther  -> GetRenderType  () & RT_BRUSH )
#line 320 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 321 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
BounceSound  (((FLOAT3D &) eTouch  . plCollision ) % en_vCurrentTranslationAbsolute );
#line 323 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fImpactSpeed  = en_vCurrentTranslationAbsolute  % (- (FLOAT3D &) eTouch  . plCollision );
#line 326 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
if(fImpactSpeed  > 1000)
#line 327 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
{
#line 329 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
ReceiveDamage  (eTouch  . penOther  , DMT_IMPACT  , m_fHealth  * 2.0f , 
#line 330 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D (0 , 0 , 0) , FLOAT3D (0 , 0 , 0));
#line 331 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 332 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 333 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
return TRUE;
#line 334 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}ASSERT(FALSE);break;case(EVENTCODE_EDeath):{const EDeath&e= (EDeath&)__eeInput;

#line 337 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOATaabbox3D box ;
#line 338 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
GetBoundingBox  (box );
#line 339 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT fEntitySize  = box  . Size  () . MaxNorm  ();
#line 341 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
Debris_Begin  (EIBT_ROCK  , DPT_NONE  , BET_NONE  , fEntitySize  , FLOAT3D (1.0f , 2.0f , 3.0f) , FLOAT3D (0 , 0 , 0) , 1.0f , 0.0f);
#line 342 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
for(INDEX iDebris  = 0;iDebris  < 12;iDebris  ++){
#line 343 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
Debris_Spawn  (this  , this  , MODEL_STONE  , TEXTURE_STONE  , 0 , 0 , 0 , IRnd  () % 4 , 0.15f , 
#line 344 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
FLOAT3D (FRnd  () * 0.8f + 0.1f , FRnd  () * 0.8f + 0.1f , FRnd  () * 0.8f + 0.1f));
#line 345 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}
#line 346 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
Destroy  ();
#line 347 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
UnsetTimer();Jump(STATE_CURRENT,0x025c0003, FALSE, EInternal());return TRUE;
#line 348 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}ASSERT(FALSE);break;default: return FALSE; break;
#line 349 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
}return TRUE;}BOOL CRollingStone::H0x025c0003_Main_04(const CEntityEvent &__eeInput){
ASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);
#undef STATE_CURRENT
#define STATE_CURRENT 0x025c0003

#line 352 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
Destroy  ();
#line 354 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
Return(STATE_CURRENT,EVoid());
#line 354 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/RollingStone.es"
return TRUE; ASSERT(FALSE); return TRUE;};