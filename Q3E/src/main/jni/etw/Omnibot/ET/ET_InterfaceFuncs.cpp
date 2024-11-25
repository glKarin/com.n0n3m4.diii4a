#include "PrecompET.h"
#include "ET_InterfaceFuncs.h"

namespace InterfaceFuncs
{
	bool IsWeaponOverheated(Client *_bot, ET_Weapon _weapon)
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
		InterfaceMsg(msg, _bot ? _bot->GetGameEntity() : GameEntity());
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
		InterfaceMsg(msg, _bot ? _bot->GetGameEntity() : GameEntity());
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

	obBool IsWaitingForMedic(const GameEntity _ent)
	{
		ET_WaitingForMedic data = { False };
		MessageHelper msg(ET_MSG_ISWAITINGFORMEDIC, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
		return data.m_WaitingForMedic==True ? True : False;
	}

	bool SelectPrimaryWeapon(Client *_bot, ET_Weapon _weapon)
	{
		ET_SelectWeapon data ={(ET_Weapon)_bot->ConvertWeaponIdToMod(_weapon)};
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
		ET_MG42MountedPlayer data = { _gun, GameEntity() };
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

	void ChangeSpawnPoint(GameEntity _ent, int _spawnpoint)
	{
		ET_SpawnPoint data = { _spawnpoint };
		MessageHelper msg(ET_MSG_CHANGESPAWNPOINT, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
	}

	void ChangeSpawnPoint(Client *_bot, int _spawnpoint)
	{
		ChangeSpawnPoint(_bot->GetGameEntity(), _spawnpoint);
	}

	bool GetMg42Properties(Client *_bot, ET_MG42Info &_data)
	{
		memset(&_data, 0, sizeof(_data));
		MessageHelper msg(ET_MSG_MOUNTEDMG42INFO, &_data, sizeof(ET_MG42Info));
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool GetCabinetData(GameEntity _ent, ET_CabinetData &data)
	{
		MessageHelper msg(ET_MSG_CABINETDATA, &data, sizeof(ET_CabinetData));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}

	bool FireTeamCreate(Client *_bot)
	{
		MessageHelper msg(ET_MSG_FIRETEAM_CREATE);
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool FireTeamDisband(Client *_bot)
	{
		MessageHelper msg(ET_MSG_FIRETEAM_DISBAND);
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool FireTeamLeave(Client *_bot)
	{
		MessageHelper msg(ET_MSG_FIRETEAM_LEAVE);
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool FireTeamApply(Client *_bot, int _fireteamnum)
	{
		ET_FireTeamApply data = { _fireteamnum };
		MessageHelper msg(ET_MSG_FIRETEAM_APPLY, &data, sizeof(ET_FireTeamApply));
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool FireTeamInvite(Client *_bot, GameEntity _target)
	{
		ET_FireTeam data = { _target };
		MessageHelper msg(ET_MSG_FIRETEAM_INVITE, &data, sizeof(ET_FireTeam));
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool FireTeamWarn(Client *_bot, GameEntity _target)
	{
		ET_FireTeam data = { _target };
		MessageHelper msg(ET_MSG_FIRETEAM_WARN, &data, sizeof(ET_FireTeam));
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool FireTeamKick(Client *_bot, GameEntity _target)
	{
		ET_FireTeam data = { _target };
		MessageHelper msg(ET_MSG_FIRETEAM_KICK, &data, sizeof(ET_FireTeam));
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool FireTeamPropose(Client *_bot, GameEntity _target)
	{
		ET_FireTeam data = { _target };
		MessageHelper msg(ET_MSG_FIRETEAM_PROPOSE, &data, sizeof(ET_FireTeam));
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool FireTeamGetInfo(Client *_bot, ET_FireTeamInfo &_data)
	{
		MessageHelper msg(ET_MSG_FIRETEAM_INFO, &_data, sizeof(ET_FireTeamInfo));
		return SUCCESS(InterfaceMsg(msg, _bot->GetGameEntity()));
	}

	bool SetCvar(char *_cvar, char *_value)
	{
		if (_cvar && _value)
		{
			ET_CvarSet data;
			data.m_Cvar = _cvar;
			data.m_Value = _value;
			MessageHelper msg(ET_MSG_SETCVAR, &data, sizeof(data));
			InterfaceMsg(msg);
		}
		return true;
	}

	int GetCvar(const char *_cvar)
	{
		if (_cvar)
		{
			ET_CvarGet data;
			data.m_Cvar = _cvar;
			data.m_Value = 0;
			MessageHelper msg(ET_MSG_GETCVAR, &data, sizeof(data));
			InterfaceMsg(msg);
			return data.m_Value;
		}
		return 0;
	}

	int GetGameType()
	{
		ET_GameType data = { 0 };
		MessageHelper msg(ET_MSG_GETGAMETYPE, &data, sizeof(data));
		InterfaceMsg(msg);
		return data.m_GameType;
	}

	void DisableBotPush(Client *_bot, int _push )
	{
		ET_DisableBotPush data = { _push };
		MessageHelper msg(ET_MSG_DISABLEBOTPUSH, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
	}
}
