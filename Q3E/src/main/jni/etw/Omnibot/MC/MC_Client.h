////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MC_CLIENT_H__
#define __MC_CLIENT_H__

#include "Client.h"
#include "MC_Config.h"
#include "MC_Messages.h"

// class: MC_Client
//		Client for Half-life 2 Deathmatch
class MC_Client : public Client
{
public:

	NavFlags GetTeamFlag();
	NavFlags GetTeamFlag(int _team);

	const MC_PlayerStats &GetPlayerStats();
	const MC_ModuleStats &GetModuleStats();
	const MC_WeaponUpgradeStats &GetUpgradeStats();

	void SendVoiceMacro(int _macroId) {};

	float GetGameVar(GameVar _var) const;
	float GetAvoidRadius(int _class) const;

	bool DoesBotHaveFlag(MapGoalPtr _mapgoal);

	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

	MC_Client();
	virtual ~MC_Client();
protected:
	MC_PlayerStats			m_PlayerStats;
	int						m_PlayerTimeStamp;

	MC_ModuleStats			m_ModuleStats;
	int						m_ModuleTimeStamp;

	MC_WeaponUpgradeStats	m_UpgradeStats;
	int						m_UpgradeTimeStamp;
};

#endif
