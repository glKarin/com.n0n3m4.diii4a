////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __DOD_CLIENT_H__
#define __DOD_CLIENT_H__

#include "Client.h"
#include "DOD_Config.h"

// class: DOD_Client
//		DOD Bot Class
class DOD_Client : public Client
{
public:

	NavFlags GetTeamFlag();
	NavFlags GetTeamFlag(int _team);

	void SendVoiceMacro(int _macroId) {};

	float GetGameVar(GameVar _var) const;
	float GetAvoidRadius(int _class) const;

	bool DoesBotHaveFlag(MapGoalPtr _mapgoal);

	bool CanBotSnipe();
	bool GetSniperWeapon(int &nonscoped, int &scoped);

	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

	void SetupBehaviorTree();

	DOD_Client();
	virtual ~DOD_Client();
protected:

};

#endif
