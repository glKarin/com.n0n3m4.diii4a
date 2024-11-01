////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __HL2DM_CLIENT_H__
#define __HL2DM_CLIENT_H__

#include "Client.h"
#include "HL2DM_Config.h"

// class: HL2DM_Client
//		Client for Half-life 2 Deathmatch
class HL2DM_Client : public Client
{
public:

	NavFlags GetTeamFlag();
	NavFlags GetTeamFlag(int _team);

	void SendVoiceMacro(int _macroId) {};

	float GetGameVar(GameVar _var) const;
	float GetAvoidRadius(int _class) const;

	bool DoesBotHaveFlag(MapGoalPtr _mapgoal);

	HL2DM_Client();
	virtual ~HL2DM_Client();
protected:

};

#endif
