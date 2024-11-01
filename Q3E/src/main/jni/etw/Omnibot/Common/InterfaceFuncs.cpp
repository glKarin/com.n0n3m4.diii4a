#include "PrecompCommon.h"
#include "InterfaceFuncs.h"

extern float g_fTopWaypointOffset;
extern float g_fBottomWaypointOffset;

obResult InterfaceMsg(const MessageHelper &_data, const GameEntity _ent)
{
	obResult res = g_EngineFuncs->InterfaceSendMessage(_data, _ent);
	OBASSERT(res != UnknownMessageType, "Unknown Interface Message: %d", _data.GetMessageId());
	return res;
}

namespace InterfaceFuncs
{
	int Addbot(Msg_Addbot &_addbot)
	{
		OBASSERT(_addbot.m_Name[0], "Invalid Name!");
		MessageHelper msg(GEN_MSG_ADDBOT, &_addbot, sizeof(_addbot));
		return g_EngineFuncs->AddBot(msg);
	}

	void Kickbot(Msg_Kickbot &_kickbot)
	{
		MessageHelper msg(GEN_MSG_KICKBOT, &_kickbot, sizeof(_kickbot));
		g_EngineFuncs->RemoveBot(msg);
	}

	bool IsAlive(const GameEntity _ent)
	{
		Msg_IsAlive data;
		MessageHelper msg(GEN_MSG_ISALIVE, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
		return (data.m_IsAlive == True);
	}

	bool IsAllied(const GameEntity _ent1, const GameEntity _ent2)
	{
		// Ally check.
		Msg_IsAllied data(_ent2);
		MessageHelper msg(GEN_MSG_ISALLIED, &data, sizeof(data));
		InterfaceMsg(msg, _ent1);
		return (data.m_IsAllied == True);
	}

	bool IsOutSide(const Vector3f &_pos)
	{
		Msg_IsOutside data;
		data.m_Position[0] = _pos[0];
		data.m_Position[1] = _pos[1];
		data.m_Position[2] = _pos[2];
		data.m_IsOutside = False;
		MessageHelper msg(GEN_MSG_ISOUTSIDE, &data, sizeof(data));
		InterfaceMsg(msg, GameEntity());
		return data.m_IsOutside == True;
	}

	bool GetHealthAndArmor(const GameEntity _ent, Msg_HealthArmor &_out)
	{
		MessageHelper msg(GEN_MSG_GETHEALTHARMOR, &_out, sizeof(Msg_HealthArmor));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}

	bool GetMaxSpeed(const GameEntity _ent, Msg_PlayerMaxSpeed &_out)
	{
		MessageHelper msg(GEN_MSG_GETMAXSPEED, &_out, sizeof(Msg_PlayerMaxSpeed));
		return SUCCESS(InterfaceMsg(msg, _ent));
	}

	int GetEntityTeam(const GameEntity _ent)
	{
		return g_EngineFuncs->GetEntityTeam(_ent);
	}

	int GetEntityClass(const GameEntity _ent)
	{
		return g_EngineFuncs->GetEntityClass(_ent);
	}

	bool GetEntityCategory(const GameEntity _ent, BitFlag32 &_category)
	{
		return SUCCESS(g_EngineFuncs->GetEntityCategory(_ent, _category));
	}

	bool GetEntityFlags(const GameEntity _ent, BitFlag64 &_flags)
	{
		return SUCCESS(g_EngineFuncs->GetEntityFlags(_ent, _flags));
	}

	bool GetEntityPowerUps(const GameEntity _ent, BitFlag64 &_powerups)
	{
		return SUCCESS(g_EngineFuncs->GetEntityPowerups(_ent, _powerups));
	}

	WeaponStatus GetEquippedWeapon(const GameEntity _ent)
	{
		WeaponStatus data;
		MessageHelper msg(GEN_MSG_GETEQUIPPEDWEAPON, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
		data.m_WeaponId = IGameManager::GetInstance()->GetGame()->ConvertWeaponId(data.m_WeaponId);
		return data;
	}

	WeaponStatus GetMountedWeapon(Client *_bot)
	{
		WeaponStatus data;
		MessageHelper msg(GEN_MSG_GETMOUNTEDWEAPON, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data;
	}

	bool GetWeaponLimits(Client *_bot, int _weapon, WeaponLimits &_limits)
	{
		_limits.m_WeaponId = _weapon;
		MessageHelper msg(GEN_MSG_GETWEAPONLIMITS, &_limits, sizeof(_limits));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return true;
	}

	bool IsReadyToFire(const GameEntity _ent)
	{
		Msg_ReadyToFire data;
		MessageHelper msg(GEN_MSG_ISREADYTOFIRE, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
		return data.m_Ready == True;
	}

	bool IsReloading(const GameEntity _ent)
	{
		Msg_Reloading data;
		MessageHelper msg(GEN_MSG_ISRELOADING, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
		return data.m_Reloading == True;
	}

	bool GetFlagState(const GameEntity _ent, FlagState &_outFlagState, GameEntity &_outEntity)
	{
		Msg_FlagState data;
		data.m_Owner = GameEntity();
		data.m_FlagState = S_FLAG_NOT_A_FLAG;
		MessageHelper msg(GEN_MSG_GETFLAGSTATE, &data, sizeof(data));
		bool bOk = SUCCESS(InterfaceMsg(msg, _ent));
		if(bOk)
		{
			_outFlagState = data.m_FlagState;
			_outEntity = data.m_Owner;
		}
		return bOk;
	}

	int GetControllingTeam(const GameEntity _ent)
	{
		ControllingTeam data;
		MessageHelper msg(GEN_MSG_GETCONTROLLINGTEAM, &data, sizeof(data));
		if(SUCCESS(InterfaceMsg(msg, _ent)))
			return data.m_ControllingTeam;
		return 0;
	}

	GameState GetGameState()
	{
		Msg_GameState data;
		MessageHelper msg(GEN_MSG_GAMESTATE, &data, sizeof(data));
		InterfaceMsg(msg);
		return data.m_GameState;
	}

	float GetGameTimeLeft()
	{
		Msg_GameState data;
		MessageHelper msg(GEN_MSG_GAMESTATE, &data, sizeof(data));
		InterfaceMsg(msg);
		return data.m_TimeLeft;
	}

	const char *GetGameState(GameState _state)
	{
		switch(_state)
		{
		case GAME_STATE_PLAYING:
			return "Playing";
		case GAME_STATE_WARMUP:
			return "Warm-up";
		case GAME_STATE_WARMUP_COUNTDOWN:
			return "Warm-up Countdown";
		case GAME_STATE_INTERMISSION:
			return "Intermission";
		case GAME_STATE_WAITINGFORPLAYERS:
			return "Waiting for Players";
		case GAME_STATE_PAUSED:
			return "Paused";
		default:
			return "Invalid";
		}	
	}

	obUserData GetEntityStat(const GameEntity _ent, const char *_statname)
	{
		Msg_EntityStat data;
		Utils::StringCopy(data.m_StatName, _statname ? _statname : "", sizeof(data.m_StatName));
		
		MessageHelper msg(GEN_MSG_ENTITYSTAT, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
		return data.m_Result;
	}

	obUserData GetTeamStat(int _team, const char *_statname)
	{
		Msg_TeamStat data;
		data.m_Team = _team;
		Utils::StringCopy(data.m_StatName, _statname ? _statname : "", sizeof(data.m_StatName));

		MessageHelper msg(GEN_MSG_TEAMSTAT, &data, sizeof(data));
		InterfaceMsg(msg);
		return data.m_Result;
	}

	bool IsWeaponCharged(Client *_bot, int _weapon, FireMode _mode)
	{
		WeaponCharged data(_weapon, _mode);
		MessageHelper msg(GEN_MSG_WPCHARGED, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		return data.m_IsCharged == True;
	}

	bool IsEntWeaponCharged(GameEntity _ent, int _weapon)
	{
		WeaponCharged data(_weapon);
		MessageHelper msg(GEN_MSG_WPCHARGED, &data, sizeof(data));
		InterfaceMsg(msg, _ent);
		return data.m_IsCharged == True;
	}

	obReal WeaponHeat(Client *_bot, FireMode _mode, float &_current, float &_max)
	{
		WeaponHeatLevel data(_mode);
		MessageHelper msg(GEN_MSG_WPHEATLEVEL, &data, sizeof(data));
		InterfaceMsg(msg, _bot->GetGameEntity());
		_current = data.m_CurrentHeat;
		_max = data.m_MaxHeat;
		return data.m_MaxHeat != 0.f ? (obReal)data.m_CurrentHeat / (obReal)data.m_MaxHeat : 0.f;
	}

	void ChangeName(Client *_bot, const char *_newname)
	{
		if(_newname)
		{
			Msg_ChangeName data = { };
			Utils::StringCopy(data.m_NewName, _newname, sizeof(data.m_NewName) / sizeof(data.m_NewName[0]));
			MessageHelper msg(GEN_MSG_CHANGENAME, &data, sizeof(data));
			InterfaceMsg(msg, _bot->GetGameEntity());
		}
	}

	bool EntityKill(GameEntity _ent)
	{
		Msg_KillEntity data = { _ent };
		MessageHelper msg(GEN_MSG_ENTITYKILL, &data, sizeof(data));
		return InterfaceMsg(msg) == Success;
	}

	bool ServerCommand(const char *_cmd)
	{
		Msg_ServerCommand data;
		Utils::StringCopy(data.m_Command, _cmd, sizeof(data.m_Command));
		MessageHelper msg(GEN_MSG_SERVERCOMMAND, &data, sizeof(data));
		return InterfaceMsg(msg) == Success;
	}

	bool PlaySound(Client *_bot, const char *_sound)
	{
		Event_PlaySound data = { };
		Utils::StringCopy(data.m_SoundName, _sound, sizeof(data.m_SoundName));
		MessageHelper msg(GEN_MSG_PLAYSOUND, &data, sizeof(data));
		return InterfaceMsg(msg, _bot->GetGameEntity()) == Success;
	}

	bool StopSound(Client *_bot, const char *_sound)
	{
		Event_StopSound data = { };
		Utils::StringCopy(data.m_SoundName, _sound, sizeof(data.m_SoundName));
		MessageHelper msg(GEN_MSG_STOPSOUND, &data, sizeof(data));
		return InterfaceMsg(msg, _bot->GetGameEntity()) == Success;
	}

	bool ScriptEvent(const char *_func, const char *_entname, const char *_p1, const char *_p2, const char *_p3)
	{
		if(!_func)
			return false;

		Event_ScriptEvent data = { };
		Utils::StringCopy(data.m_FunctionName, _func, sizeof(data.m_FunctionName));
		Utils::StringCopy(data.m_EntityName, _entname, sizeof(data.m_EntityName));
		Utils::StringCopy(data.m_Param1, _p1?_p1:"", sizeof(data.m_Param1));
		Utils::StringCopy(data.m_Param2, _p2?_p2:"", sizeof(data.m_Param2));
		Utils::StringCopy(data.m_Param3, _p3?_p3:"", sizeof(data.m_Param3));
		MessageHelper msg(GEN_MSG_SCRIPTEVENT, &data, sizeof(data));
		return InterfaceMsg(msg) == Success;
	}

	bool GotoWaypoint(const char *_param, const Vector3f &_pos)
	{
		Msg_GotoWaypoint data;
		Utils::StringCopy(data.m_WaypointName, _param, sizeof(data.m_WaypointName));
		data.m_Origin[0] = _pos[0];
		data.m_Origin[1] = _pos[1];
		data.m_Origin[2] = _pos[2];
		MessageHelper msg(GEN_MSG_GOTOWAYPOINT, &data, sizeof(data));
		return InterfaceMsg(msg, Utils::GetLocalEntity()) == Success;
	}

	bool GetVehicleInfo(Client *_bot, VehicleInfo &_vi)
	{
		MessageHelper msg(GEN_MSG_VEHICLEINFO, &_vi, sizeof(_vi));
		return InterfaceMsg(msg) == Success;
	}

	bool IsMoverAt(const Vector3f &_pos1, const Vector3f &_pos2)
	{
		Msg_MoverAt data;
		data.m_Position[0] = _pos1.x;
		data.m_Position[1] = _pos1.y;
		data.m_Position[2] = _pos1.z;
		
		data.m_Under[0] = _pos2.x;
		data.m_Under[1] = _pos2.y;
		data.m_Under[2] = _pos2.z;

		MessageHelper msg(GEN_MSG_MOVERAT, &data, sizeof(data));
		return SUCCESS(InterfaceMsg(msg)) && data.m_Entity.IsValid();
	}


}
