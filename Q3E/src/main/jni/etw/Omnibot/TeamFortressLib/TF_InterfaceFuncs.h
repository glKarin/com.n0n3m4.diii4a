////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TF_INTERFACEFUNCS_H__
#define __TF_INTERFACEFUNCS_H__

#include "BotWeaponSystem.h"

namespace InterfaceFuncs
{
	int GetPlayerPipeCount(Client *_bot);
	void DisguiseOptions(Client *_bot, TF_DisguiseOptions &_disguiseoptions);
	void Disguise(Client *_bot, obint32 _team, obint32 _class);
	void GetDisguiseInfo(GameEntity _ent, int &_team, int &_class);
	void GetDisguiseInfo(const BitFlag64 &_flags, int &_team, int &_class);
	void Cloak(Client *_bot, bool _silent);

	TF_BuildInfo GetBuildInfo(Client *_bot);
	TF_HealTarget GetHealTargetInfo(Client *_bot);

	bool LockPlayerPosition(GameEntity _ent, obBool _lock);

	void ShowHudHint(GameEntity _player, obint32 _id, const char *_msg);
	void ShowHudMenu(TF_HudMenu &_data);
	void ShowHudText(TF_HudText &_data);
};

#endif
