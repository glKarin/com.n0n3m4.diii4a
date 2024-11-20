////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompJA.h"
#include "JA_InterfaceFuncs.h"

namespace InterfaceFuncs
{
	int GetForce(Client *_bot)
	{
		JA_ForceData data = { 0 };
		MessageHelper msg(JA_MSG_FORCEDATA, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		if (data.m_Force < 0)
			return 0;
		if (data.m_Force > 100)
			return 100;
		return data.m_Force;
	}
	int GetForceLevel(Client *_bot, int _force)
	{
		JA_ForceData data = { 0 };
		MessageHelper msg(JA_MSG_FORCEDATA, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		if (_force < JA_FP_HEAL || _force > JA_FP_SABERTHROW)
			return 0;
		return data.m_Levels[_force];
	}
	int GetForceKnown(Client *_bot)
	{
		JA_ForceData data = { 0 };
		MessageHelper msg(JA_MSG_FORCEDATA, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_KnownBits;
	}
	int GetForceActive(Client *_bot)
	{
		JA_ForceData data = { 0 };
		MessageHelper msg(JA_MSG_FORCEDATA, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_ActiveBits;
	}

	bool HasFlag(Client *_bot)
	{
		JA_HasFlag data = { False };
		MessageHelper msg(JA_MSG_GHASFLAG, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_HasFlag == True;
	}

	bool HasMeMindTricked(Client *_bot, const GameEntity _ent)
	{
		JA_Mindtricked data = { _ent, False };
		MessageHelper msg(JA_MSG_MINDTRICKED, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_IsMindtricked == True;
	}

	bool ItemCanBeGrabbed(Client *_bot, const GameEntity _ent)
	{
		JA_CanBeGrabbed data = { _ent, False };
		MessageHelper msg(JA_MSG_GCANBEGRABBED, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_CanBeGrabbed == True;
	}

	void NumTeamMines(Client *_bot, int &_current, int &_max)
	{
		JA_TeamMines data = { 0, 0 };
		MessageHelper msg(JA_MSG_GNUMTEAMMINES, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		_current = data.m_Current;
		_max = data.m_Max;
	}

	void NumTeamDetpacks(Client *_bot, int &_current, int &_max)
	{
		JA_TeamDetpacks data = { 0, 0 };
		MessageHelper msg(JA_MSG_GNUMTEAMDETPACKS, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		_current = data.m_Current;
		_max = data.m_Max;
	}
	/*bool IsWeaponOverheated(Client *_bot, ET_Weapon _weapon)
	{
		ET_WeaponOverheated data = { _weapon, False };
		MessageHelper msg(ET_MSG_WPOVERHEATED, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_IsOverheated == True;
	}

	void GetMountedGunHeat(Client *_bot, const GameEntity _gun, int &_cur, int &_max)
	{
		ET_WeaponHeatLevel data = { _gun, 0, 0 };
		MessageHelper msg(ET_MSG_GETGUNHEAT, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		_cur = data.m_Current;
		_max = data.m_Max;
	}

	ExplosiveState GetExplosiveState(Client *_bot, const GameEntity _dynamite)
	{
		ET_ExplosiveState data = { _dynamite, XPLO_INVALID };
		MessageHelper msg(ET_MSG_GEXPLOSIVESTATE, &data, sizeof(data));
		InterfaceMsg(msg, _bot ? _bot->GetGameEntity() : NULL);
		return data.m_State;
	}

	ConstructableState GetConstructableState(Client *_bot, const GameEntity _constructable)
	{
		ET_ConstructionState data = { _constructable, CONST_INVALID };
		MessageHelper msg(ET_MSG_GCONSTRUCTABLE, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_State;
	}

	ConstructableState IsDestroyable(Client *_bot, const GameEntity _ent)
	{
		ET_Destroyable data = { _ent, CONST_INVALID };
		MessageHelper msg(ET_MSG_GDYNDESTROYABLE, &data, sizeof(data));
		InterfaceMsg(msg, _bot ? _bot->GetGameEntity() : NULL);
		return data.m_State;
	}

	bool HasFlag(Client *_bot)
	{
		ET_HasFlag data = { False };
		MessageHelper msg(ET_MSG_GHASFLAG, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_HasFlag == True;
	}

	bool ItemCanBeGrabbed(Client *_bot, const GameEntity _ent)
	{
		ET_CanBeGrabbed data = { _ent, False };
		MessageHelper msg(ET_MSG_GCANBEGRABBED, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_CanBeGrabbed == True;
	}

	void NumTeamMines(Client *_bot, int &_current, int &_max)
	{
		ET_TeamMines data = { 0, 0 };
		MessageHelper msg(ET_MSG_GNUMTEAMMINES, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		_current = data.m_Current;
		_max = data.m_Max;
	}

	obBool IsWaitingForMedic(Client *_bot, const GameEntity _ent)
	{
		ET_WaitingForMedic data = { False };
		MessageHelper msg(ET_MSG_ISWAITINGFORMEDIC, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
		return data.m_WaitingForMedic;
	}

	bool SelectPrimaryWeapon(Client *_bot, ET_Weapon _weapon)
	{
		ET_SelectWeapon data = { _weapon };
		MessageHelper msg(ET_MSG_PICKWEAPON, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_Good == True;
	}

	bool SelectSecondaryWeapon(Client *_bot, ET_Weapon _weapon)
	{
		ET_SelectWeapon data = { _weapon };
		MessageHelper msg(ET_MSG_PICKWEAPON2, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_Good == True;
	}

	float GetReinforceTime(Client *_bot)
	{
		ET_ReinforceTime data = { 0 };
		MessageHelper msg(ET_MSG_REINFORCETIME, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return (float)data.m_ReinforceTime / 1000.0f;
	}

	bool IsMedicNear(Client *_bot)
	{
		ET_MedicNear data = { False };
		MessageHelper msg(ET_MSG_ISMEDICNEAR, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_MedicNear == True;
	}

	bool GoToLimbo(Client *_bot)
	{
		ET_GoLimbo data = { False };
		MessageHelper msg(ET_MSG_GOTOLIMBO, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_GoLimbo == True;
	}

	GameEntity GetMountedPlayerOnMG42(Client *_bot, GameEntity _gun)
	{
		ET_MG42MountedPlayer data = { _gun, NULL };
		MessageHelper msg(ET_MSG_ISGUNMOUNTED, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_MountedEntity;
	}

	bool IsMountableGunRepairable(Client *_bot, GameEntity _gun)
	{
		ET_MG42MountedRepairable data = { _gun, False };
		MessageHelper msg(ET_MSG_ISGUNREPAIRABLE, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_Repairable == True;
	}

	int GetGunHealth(Client *_bot, const GameEntity _gun)
	{
		ET_MG42Health data = { _gun, 0 };
		MessageHelper msg(ET_MSG_GETGUNHEALTH, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_Health;
	}

	void GetCurrentCursorHint(Client *_bot, int &_type, int &_val)
	{
		ET_CursorHint data = { 0, 0 };
		MessageHelper msg(ET_MSG_GETHINT, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());

		_type = data.m_Type;
		_val = data.m_Value;
	}

	bool GetCheckPointTeam(GameEntity _ent, int &_owningteam)
	{
		ET_CheckpointTeam data = { 0 };
		MessageHelper msg(ET_MSG_CHECKPOINTTEAM, &data, sizeof(data));
		if(SUCCESS(InterfaceMsg(msg, _ent)))
		{
			_owningteam = data.m_OwningTeam;
			return true;
		}
		return false;		
	}

	void ChangeSpawnPoint(Client *_bot, int _spawnpoint)
	{
		ET_SpawnPoint data = { _spawnpoint };
		MessageHelper msg(ET_MSG_CHANGESPAWNPOINT, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
	}

	bool CanSnipe(Client *_bot)
	{
		if(_bot->GetPlayerClass() && _bot->GetPlayerClass()->GetClassID() == ET_CLASS_COVERTOPS)
		{
			// Make sure we have a sniping weapon.
			if (_bot->GetWeaponSystem()->HasAmmo(ET_WP_FG42_SCOPE) || 
				_bot->GetWeaponSystem()->HasAmmo(ET_WP_K43_SCOPE) ||
				_bot->GetWeaponSystem()->HasAmmo(ET_WP_GARAND_SCOPE))
				return true;
		}
		return false;
	}*/
};