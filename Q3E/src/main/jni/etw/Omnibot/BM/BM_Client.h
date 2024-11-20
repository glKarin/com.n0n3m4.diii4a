////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __BM_CLIENT_H__
#define __BM_CLIENT_H__

#include "Client.h"
#include "BM_Config.h"
#include "BM_Messages.h"

// class: BM_Client
//		Client for Half-life 2 Deathmatch
class BM_Client : public Client
{
public:

	NavFlags GetTeamFlag();
	NavFlags GetTeamFlag(int _team);

	void SendVoiceMacro(int _macroId) {};

	float GetGameVar(GameVar _var) const;
	float GetAvoidRadius(int _class) const;

	bool DoesBotHaveFlag(MapGoalPtr _mapgoal);

	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

	BM_Client();
	virtual ~BM_Client();
protected:
};

#endif
