////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompHL2DM.h"
#include "HL2DM_Client.h"
#include "HL2DM_NavigationFlags.h"

#include "IGame.h"
#include "IGameManager.h"

HL2DM_Client::HL2DM_Client()
{
}

HL2DM_Client::~HL2DM_Client()
{
}

NavFlags HL2DM_Client::GetTeamFlag()
{
	return GetTeamFlag(GetTeam());
}

NavFlags HL2DM_Client::GetTeamFlag(int _team)
{
	static const NavFlags defaultTeam = 0;
	switch(_team)
	{
	case HL2DM_TEAM_1:
		return F_NAV_TEAM1;
	case HL2DM_TEAM_2:
		return F_NAV_TEAM2;
	default:
		return defaultTeam;
	}
}

float HL2DM_Client::GetGameVar(GameVar _var) const
{
	switch(_var)
	{
	case JumpGapOffset:
		return 0.0f;
	}
	return 0.0f;
}

float HL2DM_Client::GetAvoidRadius(int _class) const
{
	return 0.0f;
}

bool HL2DM_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
{
	return false;
}
