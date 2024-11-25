////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompETQW.h"
#include "ETQW_InterfaceFuncs.h"

namespace InterfaceFuncs
{
	bool IsWeaponOverheated(Client *_bot, ETQW_Weapon _weapon)
	{
		ETQW_WeaponOverheated data = { _weapon, False };
		MessageHelper msg(ETQW_MSG_WPOVERHEATED, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_IsOverheated == True;
	}

	void GetMountedGunHeat(Client *_bot, const GameEntity _gun, int &_cur, int &_max)
	{
		ETQW_WeaponHeatLevel data = { _gun, 0, 0 };
		MessageHelper msg(ETQW_MSG_GETGUNHEAT, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		_cur = data.m_Current;
		_max = data.m_Max;
	}

	ExplosiveState GetExplosiveState(Client *_bot, const GameEntity _dynamite)
	{
		ETQW_ExplosiveState data = { _dynamite, XPLO_INVALID };
		MessageHelper msg(ETQW_MSG_GEXPLOSIVESTATE, &data, sizeof(data));
		InterfaceMsg(msg, _bot ? _bot->GetGameEntity() : GameEntity());
		return data.m_State;
	}

	ConstructableState GetConstructableState(Client *_bot, const GameEntity _constructable)
	{
		ETQW_ConstructionState data = { _constructable, CONST_INVALID };
		MessageHelper msg(ETQW_MSG_GCONSTRUCTABLE, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_State;
	}

	ConstructableState IsDestroyable(Client *_bot, const GameEntity _ent)
	{
		ETQW_Destroyable data = { _ent, CONST_INVALID };
		MessageHelper msg(ETQW_MSG_GDYNDESTROYABLE, &data, sizeof(data));
		InterfaceMsg(msg, _bot ? _bot->GetGameEntity() : GameEntity());
		return data.m_State;
	}

	bool HasFlag(Client *_bot)
	{
		ETQW_HasFlag data = { False };
		MessageHelper msg(ETQW_MSG_GHASFLAG, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_HasFlag == True;
	}

	bool ItemCanBeGrabbed(Client *_bot, const GameEntity _ent)
	{
		ETQW_CanBeGrabbed data = { _ent, False };
		MessageHelper msg(ETQW_MSG_GCANBEGRABBED, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_CanBeGrabbed == True;
	}

	void NumTeamMines(Client *_bot, int &_current, int &_max)
	{
		ETQW_TeamMines data = { 0, 0 };
		MessageHelper msg(ETQW_MSG_GNUMTEAMMINES, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		_current = data.m_Current;
		_max = data.m_Max;
	}

	obBool IsWaitingForMedic(Client *_bot, const GameEntity _ent)
	{
		ETQW_WaitingForMedic data = { False };
		MessageHelper msg(ETQW_MSG_ISWAITINGFORMEDIC, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
		return data.m_WaitingForMedic;
	}

	bool SelectPrimaryWeapon(Client *_bot, ETQW_Weapon _weapon)
	{
		ETQW_SelectWeapon data = { _weapon };
		MessageHelper msg(ETQW_MSG_PICKWEAPON, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_Good == True;
	}

	bool SelectSecondaryWeapon(Client *_bot, ETQW_Weapon _weapon)
	{
		ETQW_SelectWeapon data = { _weapon };
		MessageHelper msg(ETQW_MSG_PICKWEAPON2, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_Good == True;
	}

	float GetReinforceTime(Client *_bot)
	{
		ETQW_ReinforceTime data = { 0 };
		MessageHelper msg(ETQW_MSG_REINFORCETIME, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return (float)data.m_ReinforceTime / 1000.0f;
	}

	bool IsMedicNear(Client *_bot)
	{
		ETQW_MedicNear data = { False };
		MessageHelper msg(ETQW_MSG_ISMEDICNEAR, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_MedicNear == True;
	}

	bool GoToLimbo(Client *_bot)
	{
		ETQW_GoLimbo data = { False };
		MessageHelper msg(ETQW_MSG_GOTOLIMBO, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_GoLimbo == True;
	}

	GameEntity GetMountedPlayerOnMG42(Client *_bot, GameEntity _gun)
	{
		ETQW_MG42MountedPlayer data = { _gun, GameEntity() };
		MessageHelper msg(ETQW_MSG_ISGUNMOUNTED, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_MountedEntity;
	}

	bool IsMountableGunRepairable(Client *_bot, GameEntity _gun)
	{
		ETQW_MG42MountedRepairable data = { _gun, False };
		MessageHelper msg(ETQW_MSG_ISGUNREPAIRABLE, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_Repairable == True;
	}

	int GetGunHealth(Client *_bot, const GameEntity _gun)
	{
		ETQW_MG42Health data = { _gun, 0 };
		MessageHelper msg(ETQW_MSG_GETGUNHEALTH, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_Health;
	}

	void GetCurrentCursorHint(Client *_bot, int &_type, int &_val)
	{
		ETQW_CursorHint data = { 0, 0 };
		MessageHelper msg(ETQW_MSG_GETHINT, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());

		_type = data.m_Type;
		_val = data.m_Value;
	}

	void ChangeSpawnPoint(Client *_bot, int _spawnpoint)
	{
		ETQW_SpawnPoint data = { _spawnpoint };
		MessageHelper msg(ETQW_MSG_CHANGESPAWNPOINT, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
	}

	bool GetMg42Properties(Client *_bot, ETQW_MG42Info &_data)
	{
		memset(&_data, 0, sizeof(_data));
		MessageHelper msg(ETQW_MSG_MOUNTEDMG42INFO, &_data, sizeof(ETQW_MG42Info));
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}
};