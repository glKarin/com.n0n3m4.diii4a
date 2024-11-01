////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ETQW_CLIENT_H__
#define __ETQW_CLIENT_H__

#include "Client.h"
#include "ETQW_Config.h"

// class: ETQW_Client
//		Extended client class for Enemy-Territory.
class ETQW_Client : public Client
{
public:	
	friend void gmBindETQWBotLibrary(gmMachine *_machine);

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

	bool GetSkills(gmMachine *machine, gmTableObject *tbl);

	float GetBreakableTargetDist() const { return m_BreakableTargetDistance; }

	float NavCallback(const NavFlags &_flag, Waypoint *from, Waypoint *to) ;

	void SetupBehaviorTree();

	ETQW_Client();
	virtual ~ETQW_Client();
protected:
	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);
	int HandleVoiceMacroEvent(const MessageHelper &_message);

	float		m_BreakableTargetDistance;
};

#endif
