////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MC_INTERFACEFUNCS_H__
#define __MC_INTERFACEFUNCS_H__

#include "BotWeaponSystem.h"

namespace InterfaceFuncs
{	
	bool GetPlayerStats(GameEntity _ent, MC_PlayerStats &stats);
	bool GetModuleStats(GameEntity _ent, MC_ModuleStats &stats);
	bool UpgradeModule(GameEntity _ent, int moduleId);
	bool GetWeaponStats(GameEntity _ent, MC_WeaponUpgradeStats &stats);
	bool UpgradeWeapon(GameEntity _ent, int upgradeId);
	bool CanPhysPickup(GameEntity _ent, GameEntity _pickup);
	bool GetPhysGunInfo(GameEntity _ent, MC_PhysGunInfo &info);
	bool GetChargerStatus(GameEntity _ent, MC_ChargerStatus &status);
};

#endif
