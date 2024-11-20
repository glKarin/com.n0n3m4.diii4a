////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompWOLF.h"
#include "WOLF_Client.h"

#include "IGame.h"
#include "IGameManager.h"

WOLF_Client::WOLF_Client()
{
}

WOLF_Client::~WOLF_Client()
{
}

NavFlags WOLF_Client::GetTeamFlag()
{
	return GetTeamFlag(GetTeam());
}

NavFlags WOLF_Client::GetTeamFlag(int _team)
{
	static const NavFlags defaultTeam = 0;
	switch(_team)
	{
	case WOLF_ALLIES:
		return F_NAV_TEAM1;
	case WOLF_AXIS:
		return F_NAV_TEAM2;
	default:
		return defaultTeam;
	}
}

float WOLF_Client::GetGameVar(GameVar _var) const
{
	switch(_var)
	{
	case JumpGapOffset:
		return 0.0f;
	}
	return 0.0f;
}

float WOLF_Client::GetAvoidRadius(int _class) const
{
	return 0.0f;
}

bool WOLF_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
{
	return false;
}
