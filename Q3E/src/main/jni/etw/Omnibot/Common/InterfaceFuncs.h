#ifndef __INTERFACEFUNCS_H__
#define __INTERFACEFUNCS_H__

obResult InterfaceMsg(const MessageHelper &_data, const GameEntity _ent = GameEntity());

namespace InterfaceFuncs
{
	int Addbot(Msg_Addbot &_addbot);
	void Kickbot(Msg_Kickbot &_kickbot);

	bool IsAlive(const GameEntity _ent);
	bool IsAllied(const GameEntity _ent1, const GameEntity _ent2);
	bool GetHealthAndArmor(const GameEntity _ent, Msg_HealthArmor &_out);
	bool GetMaxSpeed(const GameEntity _ent, Msg_PlayerMaxSpeed &_out);
	int GetEntityTeam(const GameEntity _ent);
	int GetEntityClass(const GameEntity _ent);
	bool GetEntityCategory(const GameEntity _ent, BitFlag32 &_category);
	bool GetEntityFlags(const GameEntity _ent, BitFlag64 &_flags);
	bool GetEntityPowerUps(const GameEntity _ent, BitFlag64 &_powerups);
	WeaponStatus GetEquippedWeapon(const GameEntity _ent);
	WeaponStatus GetMountedWeapon(Client *_bot);
	bool GetWeaponLimits(Client *_bot, int _weapon, WeaponLimits &_limits);
	bool IsReadyToFire(const GameEntity _ent);
	bool IsReloading(const GameEntity _ent);
	bool GetFlagState(const GameEntity _ent, FlagState &_outFlagState, GameEntity &_outEntity);
	int GetControllingTeam(const GameEntity _ent);
	GameState GetGameState();
	float GetGameTimeLeft();
	const char *GetGameState(GameState _state);

	obUserData GetEntityStat(const GameEntity _ent, const char *_statname);
	obUserData GetTeamStat(int _team, const char *_statname);

	bool IsWeaponCharged(Client *_bot, int _weapon, FireMode _mode = Primary);
	bool IsEntWeaponCharged(GameEntity _ent, int _weapon);
	obReal WeaponHeat(Client *_bot, FireMode _mode, float &_current, float &_max);
	bool IsOutSide(const Vector3f &_pos);
	void ChangeName(Client *_bot, const char *_newname);

	bool EntityKill(GameEntity _ent);
	bool ServerCommand(const char *_cmd);

	bool PlaySound(Client *_bot, const char *_sound);
	bool StopSound(Client *_bot, const char *_sound);
	bool ScriptEvent(const char *_func, const char *_entname, const char *_p1, const char *_p2, const char *_p3);

	bool GotoWaypoint(const char *_param, const Vector3f &_pos);

	bool GetVehicleInfo(Client *_bot, VehicleInfo &_vi);

	bool IsMoverAt(const Vector3f &_pos1, const Vector3f &_pos2);

	template <typename T>
	bool SetLoadOut(GameEntity _ent, T &_info)
	{
		MessageHelper msg(GEN_MSG_SETLOADOUT, &_info, sizeof(T));
		return SUCCESS(InterfaceMsg(msg,_ent));
	}
}

#endif
