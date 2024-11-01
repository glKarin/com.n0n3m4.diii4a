#ifndef __RTCW_CLIENT_H__
#define __RTCW_CLIENT_H__

#include "Client.h"
#include "RTCW_Config.h"

// class: RTCW_Client
//		Extended client class for Return to Castle Wolfenstein.
class RTCW_Client : public Client
{
public:	
	friend void gmBindRTCWBotLibrary(gmMachine *_machine);

	void Init(int _gameid);

	NavFlags GetTeamFlag();
	NavFlags GetTeamFlag(int _team);

	void SendVoiceMacro(int _macroId);

	void ProcessGotoNode(const Path &_path);

	float GetGameVar(GameVar _var) const;
	float GetAvoidRadius(int _class) const;

	bool DoesBotHaveFlag(MapGoalPtr _mapgoal);
	bool IsFlagGrabbable(MapGoalPtr _mapgoal);
	bool IsItemGrabbable(GameEntity _ent);

	bool CanBotSnipe();
	bool GetSniperWeapon(int &nonscoped, int &scoped);

	float GetBreakableTargetDist() const { return m_BreakableTargetDistance; }
	float GetHealthEntityDist() const { return m_HealthEntityDistance; }
	float GetAmmoEntityDist() const { return m_AmmoEntityDistance; }
	float GetWeaponEntityDist() const { return m_WeaponEntityDistance; }
	float GetProjectileEntityDist() const { return m_ProjectileEntityDistance; }

	float NavCallback(const NavFlags &_flag, Waypoint *from, Waypoint *to) ;

	void SetupBehaviorTree();
	
	RTCW_Client();
	virtual ~RTCW_Client();
protected:
	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);
	int HandleVoiceMacroEvent(const MessageHelper &_message);

	float		m_BreakableTargetDistance;
	float		m_HealthEntityDistance;
	float		m_AmmoEntityDistance;
	float		m_WeaponEntityDistance;
	float		m_ProjectileEntityDistance;
	int		m_StrafeJump;
};

#endif
