////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __Q4_CLIENT_H__
#define __Q4_CLIENT_H__

#include "Client.h"
#include "Q4_Config.h"

// class: Q4_Client
//		Q4 Bot Class
class Q4_Client : public Client
{
public:

	NavFlags GetTeamFlag();
	NavFlags GetTeamFlag(int _team);

	void SendVoiceMacro(int _macroId) {};

	float GetGameVar(GameVar _var) const;
	float GetAvoidRadius(int _class) const;

	bool DoesBotHaveFlag(MapGoalPtr _mapgoal);

	void SetupBehaviorTree();

	Q4_Client();
	virtual ~Q4_Client();
protected:

};

#endif
