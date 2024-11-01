////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __JA_CLIENT_H__
#define __JA_CLIENT_H__

#include "Client.h"
#include "JA_Config.h"

// class: JA_Client
//		Extended client class for Jedi-Academy.
class JA_Client : public Client
{
public:
	//friend void gmBindJABotLibrary(gmMachine *_machine);

	void Init(int _gameid);

	NavFlags GetTeamFlag();
	NavFlags GetTeamFlag(int _team);

	void SendVoiceMacro(int _macroId);

	void ProcessGotoNode(const Path &_path);

	float GetGameVar(GameVar _var) const;
	float GetAvoidRadius(int _class) const;

	bool DoesBotHaveFlag(MapGoalPtr _mapgoal);

	bool CanBotSnipe();
	bool GetSniperWeapon(int &nonscoped, int &scoped);

	float GetBreakableTargetDist() const { return m_BreakableTargetDistance; }

	float NavCallback(const NavFlags &_flag, Waypoint *from, Waypoint *to);

	void SetupBehaviorTree();

	JA_Client();
	virtual ~JA_Client();
protected:
	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);
	int HandleVoiceMacroEvent(const MessageHelper &_message);

	float		m_BreakableTargetDistance;
};

#endif
