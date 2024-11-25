////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __WOLF_CLIENT_H__
#define __WOLF_CLIENT_H__

#include "Client.h"
#include "WOLF_Config.h"

// class: WOLF_Client
//		Client for Half-life 2 Deathmatch
class WOLF_Client : public Client
{
public:

	NavFlags GetTeamFlag();
	NavFlags GetTeamFlag(int _team);

	void SendVoiceMacro(int _macroId) {};

	float GetGameVar(GameVar _var) const;
	float GetAvoidRadius(int _class) const;

	bool DoesBotHaveFlag(MapGoalPtr _mapgoal);

	WOLF_Client();
	virtual ~WOLF_Client();
protected:

};

#endif
