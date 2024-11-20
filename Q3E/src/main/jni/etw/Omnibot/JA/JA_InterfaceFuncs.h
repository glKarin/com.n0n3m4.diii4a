////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __JA_INTERFACEFUNCS_H__
#define __JA_INTERFACEFUNCS_H__

#include "BotWeaponSystem.h"

namespace InterfaceFuncs
{	
	int GetForce(Client *_bot);
	int GetForceLevel(Client *_bot, int _force);
	int GetForceKnown(Client *_bot);
	int GetForceActive(Client *_bot);
	bool HasMeMindTricked(Client *_bot, const GameEntity _ent);
	bool HasFlag(Client *_bot);
	bool ItemCanBeGrabbed(Client *_bot, const GameEntity _ent);
	void NumTeamMines(Client *_bot, int &_current, int &_max);
	void NumTeamDetpacks(Client *_bot, int &_current, int &_max);
	/*bool IsWeaponOverheated(Client *_bot, ET_Weapon _weapon);
	void GetMountedGunHeat(Client *_bot, const GameEntity _gun, int &_cur, int &_max);
	ExplosiveState GetExplosiveState(Client *_bot, const GameEntity _dynamite);
	ConstructableState GetConstructableState(Client *_bot, const GameEntity _constructable);
	ConstructableState IsDestroyable(Client *_bot, const GameEntity _ent);
	bool HasFlag(Client *_bot);
	bool ItemCanBeGrabbed(Client *_bot, const GameEntity _ent);
	void NumTeamMines(Client *_bot, int &_current, int &_max);
	obBool IsWaitingForMedic(Client *_bot, const GameEntity _ent);
	bool SelectPrimaryWeapon(Client *_bot, ET_Weapon _weapon);
	bool SelectSecondaryWeapon(Client *_bot, ET_Weapon _weapon);
	float GetReinforceTime(Client *_bot);
	bool IsMedicNear(Client *_bot);
	bool GoToLimbo(Client *_bot);
	GameEntity GetMountedPlayerOnMG42(Client *_client, const GameEntity _gun);
	bool IsMountableGunRepairable(Client *_bot, GameEntity _gun);
	int GetGunHealth(Client *_client, const GameEntity _gun);
	void GetCurrentCursorHint(Client *_bot, int &_type, int &_val);
	bool CanSnipe(Client *_bot);
	bool GetCheckPointTeam(GameEntity _ent, int &_owningteam);
	void ChangeSpawnPoint(Client *_bot, int _spawnpoint);*/
	
};

#endif
