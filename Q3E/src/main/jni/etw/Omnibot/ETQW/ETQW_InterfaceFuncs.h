////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ETQW_INTERFACEFUNCS_H__
#define __ETQW_INTERFACEFUNCS_H__

#include "BotWeaponSystem.h"

namespace InterfaceFuncs
{	
	bool IsWeaponOverheated(Client *_bot, ETQW_Weapon _weapon);
	void GetMountedGunHeat(Client *_bot, const GameEntity _gun, int &_cur, int &_max);
	ExplosiveState GetExplosiveState(Client *_bot, const GameEntity _dynamite);
	ConstructableState GetConstructableState(Client *_bot, const GameEntity _constructable);
	ConstructableState IsDestroyable(Client *_bot, const GameEntity _ent);
	bool HasFlag(Client *_bot);
	bool ItemCanBeGrabbed(Client *_bot, const GameEntity _ent);
	void NumTeamMines(Client *_bot, int &_current, int &_max);
	obBool IsWaitingForMedic(Client *_bot, const GameEntity _ent);
	bool SelectPrimaryWeapon(Client *_bot, ETQW_Weapon _weapon);
	bool SelectSecondaryWeapon(Client *_bot, ETQW_Weapon _weapon);
	float GetReinforceTime(Client *_bot);
	bool IsMedicNear(Client *_bot);
	bool GoToLimbo(Client *_bot);
	GameEntity GetMountedPlayerOnMG42(Client *_client, const GameEntity _gun);
	bool IsMountableGunRepairable(Client *_bot, GameEntity _gun);
	int GetGunHealth(Client *_client, const GameEntity _gun);
	void GetCurrentCursorHint(Client *_bot, int &_type, int &_val);
	void ChangeSpawnPoint(Client *_bot, int _spawnpoint);
	bool GetMg42Properties(Client *_bot, ETQW_MG42Info &data);	
};

#endif
