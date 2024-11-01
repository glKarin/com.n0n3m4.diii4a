////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompQ4.h"
#include "Q4_Client.h"
#include "Q4_NavigationFlags.h"
#include "Q4_Messages.h"

#include "IGame.h"
#include "IGameManager.h"

//////////////////////////////////////////////////////////////////////////

Q4_Client::Q4_Client()
{
	m_StepHeight = 15.0f; // it's actually 16
}

Q4_Client::~Q4_Client()
{
}

NavFlags Q4_Client::GetTeamFlag()
{
	return GetTeamFlag(GetTeam());
}

NavFlags Q4_Client::GetTeamFlag(int _team)
{
	static const NavFlags defaultTeam = 0;
	switch(_team)
	{
	case Q4_TEAM_MARINE:
		return F_NAV_TEAM1;
	case Q4_TEAM_STROGG:
		return F_NAV_TEAM2;
	default:
		return defaultTeam;
	}
}

float Q4_Client::GetAvoidRadius(int _class) const
{
	switch(_class)
	{
	case Q4_CLASS_PLAYER:
		return 64.0f;
	}
	return 0.0f;
}

float Q4_Client::GetGameVar(GameVar _var) const
{
	switch(_var)
	{
	case JumpGapOffset:
		return 0.0f;
	}
	return 0.0f;
}

bool Q4_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
{
	int iTeamFlags = 0;
	switch(GetTeam())
	{
	case Q4_TEAM_MARINE:
		iTeamFlags = Q4_PWR_STROGGFLAG;
		break;
	case Q4_TEAM_STROGG:
		iTeamFlags = Q4_PWR_MARINEFLAG;
		break;
	}

	if(HasPowerup(iTeamFlags) || HasPowerup(Q4_PWR_ONEFLAG) || HasPowerup(Q4_PWR_DEADZONE))
		return true;
	return false;
}

void Q4_Client::SetupBehaviorTree()
{
	using namespace AiState;
	//FINDSTATEIF(HighLevel,GetStateRoot(),AppendState(new Deadzone))
}
