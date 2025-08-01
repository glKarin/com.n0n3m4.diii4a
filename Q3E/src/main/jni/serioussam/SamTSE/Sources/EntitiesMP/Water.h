/*
 * This file is generated by Entity Class Compiler, (c) CroTeam 1997-98
 */

#ifndef _EntitiesMP_Water_INCLUDED
#define _EntitiesMP_Water_INCLUDED 1
#include <EntitiesMP/Light.h>
extern DECL_DLL CEntityPropertyEnumType WaterSize_enum;
enum WaterSize {
  WTS_SMALL = 0,
  WTS_BIG = 1,
  WTS_LARGE = 2,
};
DECL_DLL inline void ClearToDefault(WaterSize &e) { e = (WaterSize)0; } ;
#define EVENTCODE_EWater 0x01fc0000
class DECL_DLL EWater : public CEntityEvent {
public:
EWater();
CEntityEvent *MakeCopy(void);
CEntityPointer penLauncher;
enum WaterSize EwsSize;
};
DECL_DLL inline void ClearToDefault(EWater &e) { e = EWater(); } ;
extern "C" DECL_DLL CDLLEntityClass CWater_DLLClass;
class CWater : public CMovableModelEntity {
public:
  DECL_DLL virtual void SetDefaultProperties(void);
  CEntityPointer m_penLauncher;
  enum WaterSize m_EwsSize;
  FLOAT m_fDamageAmount;
  FLOAT m_fIgnoreTime;
  FLOAT m_fPushAwayFactor;
CLightSource m_lsLightSource;
   
#line 75 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Water.es"
void Read_t(CTStream * istr);
   
#line 82 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Water.es"
CLightSource * GetLightSource(void);
   
#line 92 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Water.es"
void SetupLightSource(void);
   
#line 109 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Water.es"
void RenderParticles(void);
   
#line 114 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Water.es"
void WaterTouch(CEntityPointer penHit);
#define  STATE_CWater_WaterFly 0x01fc0001
  BOOL 
#line 134 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Water.es"
WaterFly(const CEntityEvent &__eeInput);
  BOOL H0x01fc0002_WaterFly_01(const CEntityEvent &__eeInput);
  BOOL H0x01fc0003_WaterFly_02(const CEntityEvent &__eeInput);
#define  STATE_CWater_Main 1
  BOOL 
#line 176 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/Water.es"
Main(const CEntityEvent &__eeInput);
  BOOL H0x01fc0004_Main_01(const CEntityEvent &__eeInput);
  BOOL H0x01fc0005_Main_02(const CEntityEvent &__eeInput);
};
#endif // _EntitiesMP_Water_INCLUDED
