#ifndef __RTCW_INTERFACEFUNCS_H__
#define __RTCW_INTERFACEFUNCS_H__

#include "BotWeaponSystem.h"

namespace InterfaceFuncs
{
	bool SendPrivateMessage(Client *_bot, char *_targName, char *_message);
	bool SetCvar(char *_cvar, char *_value);
	int GetCvar(const char *_cvar);
	bool IsWeaponOverheated(Client *_bot, RTCW_Weapon _weapon);
	void GetMountedGunHeat(Client *_bot, const GameEntity _gun, int &_cur, int &_max);
	ExplosiveState GetExplosiveState(Client *_bot, const GameEntity _dynamite);
	ConstructableState IsDestroyable(Client *_bot, const GameEntity _ent);
	bool HasFlag(Client *_bot);
	bool ItemCanBeGrabbed(Client *_bot, const GameEntity _ent);
	void NumTeamMines(Client *_bot, int &_current, int &_max);
	bool SelectPrimaryWeapon(Client *_bot, RTCW_Weapon _weapon);
	bool SelectSecondaryWeapon(Client *_bot, RTCW_Weapon _weapon);
	float GetReinforceTime(Client *_bot);
	bool IsMedicNear(Client *_bot);
	bool GoToLimbo(Client *_bot);
	GameEntity GetMountedPlayerOnMG42(Client *_client, const GameEntity _gun);
	bool IsMountableGunRepairable(Client *_bot, GameEntity _gun);
	int GetGunHealth(Client *_client, const GameEntity _gun);
	void GetCurrentCursorHint(Client *_bot, int &_type, int &_val);
	bool CanSnipe(Client *_bot);
	void ChangeSpawnPoint(GameEntity _ent, int _spawnpoint);
	void ChangeSpawnPoint(Client *_bot, int _spawnpoint);
	bool GetMg42Properties(Client *_bot, RTCW_MG42Info &data);
	int GetGameType();
	int GetSpawnPoint(Client *_bot);
	void SetSuicide(Client *_bot, int _sui, int _pers);
	void DisableBotPush(Client *_bot, int _push);
	int GetPlayerClass(GameEntity _ent);
}

#endif
