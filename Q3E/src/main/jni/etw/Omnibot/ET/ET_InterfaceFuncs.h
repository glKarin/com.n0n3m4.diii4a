#ifndef __ET_INTERFACEFUNCS_H__
#define __ET_INTERFACEFUNCS_H__

#include "BotWeaponSystem.h"

namespace InterfaceFuncs
{	
	bool IsWeaponOverheated(Client *_bot, ET_Weapon _weapon);
	void GetMountedGunHeat(Client *_bot, const GameEntity _gun, int &_cur, int &_max);
	ExplosiveState GetExplosiveState(Client *_bot, const GameEntity _dynamite);
	ConstructableState GetConstructableState(Client *_bot, const GameEntity _constructable);
	ConstructableState IsDestroyable(Client *_bot, const GameEntity _ent);
	bool HasFlag(Client *_bot);
	bool ItemCanBeGrabbed(Client *_bot, const GameEntity _ent);
	void NumTeamMines(Client *_bot, int &_current, int &_max);
	obBool IsWaitingForMedic(const GameEntity _ent);
	bool SelectPrimaryWeapon(Client *_bot, ET_Weapon _weapon);
	bool SelectSecondaryWeapon(Client *_bot, ET_Weapon _weapon);
	float GetReinforceTime(Client *_bot);
	bool IsMedicNear(Client *_bot);
	bool GoToLimbo(Client *_bot);
	GameEntity GetMountedPlayerOnMG42(Client *_client, const GameEntity _gun);
	bool IsMountableGunRepairable(Client *_bot, GameEntity _gun);
	int GetGunHealth(Client *_client, const GameEntity _gun);
	void GetCurrentCursorHint(Client *_bot, int &_type, int &_val);
	void ChangeSpawnPoint(GameEntity _ent, int _spawnpoint);
	void ChangeSpawnPoint(Client *_bot, int _spawnpoint);
	bool GetMg42Properties(Client *_bot, ET_MG42Info &data);
	bool GetCabinetData(GameEntity _ent, ET_CabinetData &data);
	
	bool FireTeamCreate(Client *_bot);
	bool FireTeamDisband(Client *_bot);
	bool FireTeamLeave(Client *_bot);
	bool FireTeamApply(Client *_bot, int _fireteamnum);
	bool FireTeamInvite(Client *_bot, GameEntity _target);
	bool FireTeamWarn(Client *_bot, GameEntity _target);
	bool FireTeamKick(Client *_bot, GameEntity _target);
	bool FireTeamPropose(Client *_bot, GameEntity _target);
	bool FireTeamGetInfo(Client *_bot, ET_FireTeamInfo&data);

	bool SetCvar(char *_cvar, char *_value);
	int GetCvar(const char *_cvar);
	int GetGameType();
	void DisableBotPush(Client *_bot, int _push);
}

#endif
