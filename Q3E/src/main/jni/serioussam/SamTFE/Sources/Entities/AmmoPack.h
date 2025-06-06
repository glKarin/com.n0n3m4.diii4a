/*
 * This file is generated by Entity Class Compiler, (c) CroTeam 1997-98
 */

#ifndef _Entities_AmmoPack_INCLUDED
#define _Entities_AmmoPack_INCLUDED 1
#include <Entities/Item.h>
extern DECL_DLL CEntityPropertyEnumType AmmoPackType_enum;
enum AmmoPackType {
  APT_CUSTOM = 1,
  APT_SERIOUS = 2,
};
DECL_DLL inline void ClearToDefault(AmmoPackType &e) { e = (AmmoPackType)0; } ;
#define EVENTCODE_EAmmoPackItem 0x03260000
class DECL_DLL EAmmoPackItem : public CEntityEvent {
public:
EAmmoPackItem();
CEntityEvent *MakeCopy(void);
INDEX iShells;
INDEX iBullets;
INDEX iRockets;
INDEX iGrenades;
INDEX iElectricity;
INDEX iIronBalls;
};
DECL_DLL inline void ClearToDefault(EAmmoPackItem &e) { e = EAmmoPackItem(); } ;
extern "C" DECL_DLL CDLLEntityClass CAmmoPack_DLLClass;
class CAmmoPack : public CItem {
public:
  DECL_DLL virtual void SetDefaultProperties(void);
  enum AmmoPackType m_aptPackType;
  INDEX m_iShells;
  INDEX m_iBullets;
  INDEX m_iRockets;
  INDEX m_iGrenades;
  INDEX m_iElectricity;
  INDEX m_iIronBalls;
   
#line 77 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/AmmoPack.es"
void Precache(void);
   
#line 82 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/AmmoPack.es"
void RenderParticles(void);
   
#line 95 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/AmmoPack.es"
BOOL FillEntityStatistics(EntityStats * pes);
   
#line 121 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/AmmoPack.es"
void SetProperties(void);
   
#line 154 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/AmmoPack.es"
void AdjustDifficulty(void);
#define  STATE_CAmmoPack_ItemCollected 0x03260001
  BOOL 
#line 164 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/AmmoPack.es"
ItemCollected(const CEntityEvent &__eeInput);
#define  STATE_CAmmoPack_Main 1
  BOOL 
#line 201 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/AmmoPack.es"
Main(const CEntityEvent &__eeInput);
};
#endif // _Entities_AmmoPack_INCLUDED
