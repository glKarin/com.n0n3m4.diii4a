/*
 * This file is generated by Entity Class Compiler, (c) CroTeam 1997-98
 */

#ifndef _Entities_Twister_INCLUDED
#define _Entities_Twister_INCLUDED 1
#include <Entities/Elemental.h>
extern DECL_DLL CEntityPropertyEnumType TwisterSize_enum;
enum TwisterSize {
  TWS_SMALL = 0,
  TWS_BIG = 1,
  TWS_LARGE = 2,
};
DECL_DLL inline void ClearToDefault(TwisterSize &e) { e = (TwisterSize)0; } ;
#define EVENTCODE_ETwister 0x01fb0000
class DECL_DLL ETwister : public CEntityEvent {
public:
ETwister();
CEntityEvent *MakeCopy(void);
CEntityPointer penOwner;
enum TwisterSize EtsSize;
};
DECL_DLL inline void ClearToDefault(ETwister &e) { e = ETwister(); } ;
extern "C" DECL_DLL CDLLEntityClass CTwister_DLLClass;
class CTwister : public CMovableModelEntity {
public:
  DECL_DLL virtual void SetDefaultProperties(void);
  CEntityPointer m_penOwner;
  enum TwisterSize m_EtsSize;
  FLOAT3D m_vStartPosition;
  FLOAT3D m_vDesiredPosition;
  FLOAT3D m_vDesiredAngle;
  FLOAT m_fStopTime;
  FLOAT m_fActionRadius;
  FLOAT m_fActionTime;
  FLOAT m_fDiffMultiply;
  FLOAT m_fUpMultiply;
  BOOL m_bFadeOut;
  FLOAT m_fFadeStartTime;
  FLOAT m_fFadeTime;
   
#line 88 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void * GetEntityInfo(void);
   
#line 93 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void ReceiveDamage(CEntity * penInflictor,enum DamageType dmtType,
#line 94 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
FLOAT fDamageAmmount,const FLOAT3D & vHitPoint,const FLOAT3D & vDirection);
   
#line 104 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
BOOL AdjustShadingParameters(FLOAT3D & vLightDirection,COLOR & colLight,COLOR & colAmbient);
   
#line 121 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void CalcHeadingRotation(ANGLE aWantedHeadingRelative,ANGLE & aRotation);
   
#line 140 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void CalcAngleFromPosition();
   
#line 146 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void RotateToAngle();
   
#line 156 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void MoveInDirection();
   
#line 168 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void MoveToPosition();
   
#line 174 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void RotateToPosition();
   
#line 180 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void StopMoving();
   
#line 186 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void StopRotating();
   
#line 191 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void StopTranslating();
   
#line 200 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
void SpinEntity(CEntity * pen);
#define  STATE_CTwister_Main 1
  BOOL 
#line 228 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/Twister.es"
Main(const CEntityEvent &__eeInput);
  BOOL H0x01fb0001_Main_01(const CEntityEvent &__eeInput);
  BOOL H0x01fb0002_Main_02(const CEntityEvent &__eeInput);
  BOOL H0x01fb0003_Main_03(const CEntityEvent &__eeInput);
  BOOL H0x01fb0004_Main_04(const CEntityEvent &__eeInput);
  BOOL H0x01fb0005_Main_05(const CEntityEvent &__eeInput);
  BOOL H0x01fb0006_Main_06(const CEntityEvent &__eeInput);
  BOOL H0x01fb0007_Main_07(const CEntityEvent &__eeInput);
  BOOL H0x01fb0008_Main_08(const CEntityEvent &__eeInput);
};
#endif // _Entities_Twister_INCLUDED
