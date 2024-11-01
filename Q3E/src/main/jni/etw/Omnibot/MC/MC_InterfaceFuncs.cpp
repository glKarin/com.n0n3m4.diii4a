////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompMC.h"
#include "MC_InterfaceFuncs.h"

namespace InterfaceFuncs
{
	bool GetPlayerStats(GameEntity _ent, MC_PlayerStats &stats)
	{
		MessageHelper msg(MC_MSG_GET_PLAYER_STATS, &stats, sizeof(stats));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}
	bool GetModuleStats(GameEntity _ent, MC_ModuleStats &stats)
	{
		MessageHelper msg(MC_MSG_GET_MODULE_STATS, &stats, sizeof(stats));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}
	bool UpgradeModule(GameEntity _ent, int moduleId)
	{
		MC_UpgradeModule data = { moduleId };
		MessageHelper msg(MC_MSG_UPGRADE_MODULE, &data, sizeof(data));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}
	bool GetWeaponStats(GameEntity _ent, MC_WeaponUpgradeStats &stats)
	{
		MessageHelper msg(MC_MSG_GET_WEAPON_UPGRADE_STATS, &stats, sizeof(stats));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}
	bool UpgradeWeapon(GameEntity _ent, int upgradeId)
	{
		MC_UpgradeWeapon data = { upgradeId };
		MessageHelper msg(MC_MSG_UPGRADE_WEAPON, &data, sizeof(data));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}
	bool CanPhysPickup(GameEntity _ent, GameEntity _pickup)
	{
		MC_CanPhysPickup data = { _pickup, False };
		MessageHelper msg(MC_MSG_CAN_PHYSPICKUP, &data, sizeof(data));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}
	bool GetPhysGunInfo(GameEntity _ent, MC_PhysGunInfo &info)
	{
		MessageHelper msg(MC_MSG_PHYSGUNINFO, &info, sizeof(info));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}
	bool GetChargerStatus(GameEntity _ent, MC_ChargerStatus &status)
	{
		MessageHelper msg(MC_MSG_CHARGER_STATUS, &status, sizeof(status));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}
};
