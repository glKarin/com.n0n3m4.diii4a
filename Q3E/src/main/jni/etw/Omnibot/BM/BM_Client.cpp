////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompBM.h"
#include "BM_Client.h"
#include "BM_Messages.h"
#include "BM_InterfaceFuncs.h"

#include "IGame.h"
#include "IGameManager.h"

BM_Client::BM_Client()
{
}

BM_Client::~BM_Client()
{
}

NavFlags BM_Client::GetTeamFlag()
{
	return GetTeamFlag(GetTeam());
}

NavFlags BM_Client::GetTeamFlag(int _team)
{
	static const NavFlags defaultTeam = 0;
	switch(_team)
	{
	case BM_TEAM_RED:
		return F_NAV_TEAM1;
	case BM_TEAM_BLUE:
		return F_NAV_TEAM2;
	default:
		return defaultTeam;
	}
}

float BM_Client::GetGameVar(GameVar _var) const
{
	switch(_var)
	{
	case JumpGapOffset:
		return 0.0f;
	}
	return 0.0f;
}

float BM_Client::GetAvoidRadius(int _class) const
{
	return 0.0f;
}

bool BM_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
{
	return false;
}

void BM_Client::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
{
	Client::ProcessEvent(_message, _cb);
}
