////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompDOD.h"
#include "DOD_Client.h"
#include "DOD_NavigationFlags.h"
#include "DOD_Messages.h"
#include "DOD_BaseStates.h"

#include "IGame.h"
#include "IGameManager.h"
#include "ScriptManager.h"

namespace AiState
{
	
};

//////////////////////////////////////////////////////////////////////////

DOD_Client::DOD_Client()
{
	m_StepHeight = 15.0f; // it's actually 16
}

DOD_Client::~DOD_Client()
{
}

NavFlags DOD_Client::GetTeamFlag()
{
	return GetTeamFlag(GetTeam());
}

NavFlags DOD_Client::GetTeamFlag(int _team)
{
	static const NavFlags defaultTeam = 0;
	switch(_team)
	{
	case DOD_TEAM_AXIS:
		return F_NAV_TEAM1;
	case DOD_TEAM_ALLIES:
		return F_NAV_TEAM2;
	default:
		return defaultTeam;
	}
}

float DOD_Client::GetAvoidRadius(int _class) const
{
	switch(_class)
	{
	case DOD_CLASS_RIFLEMAN:
	case DOD_CLASS_ASSAULT:
	case DOD_CLASS_SUPPORT:
	case DOD_CLASS_SNIPER:
	case DOD_CLASS_MACHINEGUNNER:
	case DOD_CLASS_ROCKET:
		return 24.0f;
	}
	return 0.0f;
}

float DOD_Client::GetGameVar(GameVar _var) const
{
	switch(_var)
	{
	case JumpGapOffset:
		return 0.0f;
	}
	return 0.0f;
}

bool DOD_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
{
	return false;
}

bool DOD_Client::CanBotSnipe() 
{
	if(GetClass() == DOD_CLASS_SNIPER)
	{
		// Make sure we have a sniping weapon.
		int iCurrentTeam = g_EngineFuncs->GetEntityTeam(GetGameEntity());
		if(iCurrentTeam == DOD_TEAM_AXIS || GetWeaponSystem()->HasAmmo(DOD_WP_K98S))
			return true;
		if(iCurrentTeam == DOD_TEAM_ALLIES || GetWeaponSystem()->HasAmmo(DOD_WP_SPRING))
			return true;
	}
	return false;
}

bool DOD_Client::GetSniperWeapon(int &nonscoped, int &scoped)
{
	nonscoped = 0;
	scoped = 0;

	if(GetClass() == DOD_CLASS_SNIPER)
	{
		int iCurrentTeam = g_EngineFuncs->GetEntityTeam(GetGameEntity());
		if(iCurrentTeam == DOD_TEAM_AXIS || GetWeaponSystem()->HasAmmo(DOD_WP_K98S))
		{
			nonscoped = DOD_WP_K98S;
			scoped = DOD_WP_K98S;
			return true;
		}
		if(iCurrentTeam == DOD_TEAM_ALLIES || GetWeaponSystem()->HasAmmo(DOD_WP_SPRING))
		{
			nonscoped = DOD_WP_SPRING;
			scoped = DOD_WP_SPRING;
			return true;
		}
	}
	return false;
}

void DOD_Client::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
{
	switch(_message.GetMessageId())
	{
		HANDLER(MESSAGE_CHANGECLASS)
		{
			AiState::WeaponSystem *ws = GetWeaponSystem();
			ws->ClearWeapons();
			int iCurrentTeam = g_EngineFuncs->GetEntityTeam(GetGameEntity());
			const Event_ChangeClass *m = _message.Get<Event_ChangeClass>();
			if(m)
			{
				if(iCurrentTeam == DOD_TEAM_AXIS)
				{
					switch(m->m_NewClass)
					{
					case DOD_CLASS_RIFLEMAN:
						ws->AddWeaponToInventory(DOD_WP_K98);
						ws->AddWeaponToInventory(DOD_WP_SPADE);
						ws->AddWeaponToInventory(DOD_WP_RIFLEGREN_GER);
						break;
					case DOD_CLASS_ASSAULT:
						ws->AddWeaponToInventory(DOD_WP_MP40);
						ws->AddWeaponToInventory(DOD_WP_P38);
						ws->AddWeaponToInventory(DOD_WP_SMOKE_GER);
						ws->AddWeaponToInventory(DOD_WP_FRAG_GER);
						break;
					case DOD_CLASS_SUPPORT:
						ws->AddWeaponToInventory(DOD_WP_MP44);
						ws->AddWeaponToInventory(DOD_WP_SPADE);
						ws->AddWeaponToInventory(DOD_WP_FRAG_GER);
						break;
					case DOD_CLASS_SNIPER:
						ws->AddWeaponToInventory(DOD_WP_K98S);
						ws->AddWeaponToInventory(DOD_WP_P38);
						ws->AddWeaponToInventory(DOD_WP_SPADE);
						break;
					case DOD_CLASS_MACHINEGUNNER:
						ws->AddWeaponToInventory(DOD_WP_MG42);
						ws->AddWeaponToInventory(DOD_WP_P38);
						ws->AddWeaponToInventory(DOD_WP_SPADE);
						break;
					case DOD_CLASS_ROCKET:
						ws->AddWeaponToInventory(DOD_WP_PSCHRECK);
						ws->AddWeaponToInventory(DOD_WP_C96);
						ws->AddWeaponToInventory(DOD_WP_SPADE);
						break;
					}
					break;
				}

				if(iCurrentTeam == DOD_TEAM_ALLIES)
				{
					switch(m->m_NewClass)
					{
					case DOD_CLASS_RIFLEMAN:
						ws->AddWeaponToInventory(DOD_WP_GARAND);
						ws->AddWeaponToInventory(DOD_WP_AMERKNIFE);
						ws->AddWeaponToInventory(DOD_WP_RIFLEGREN_US);
						break;
					case DOD_CLASS_ASSAULT:
						ws->AddWeaponToInventory(DOD_WP_THOMPSON);
						ws->AddWeaponToInventory(DOD_WP_COLT);
						ws->AddWeaponToInventory(DOD_WP_SMOKE_US);
						ws->AddWeaponToInventory(DOD_WP_FRAG_US);
						break;
					case DOD_CLASS_SUPPORT:
						ws->AddWeaponToInventory(DOD_WP_BAR);
						ws->AddWeaponToInventory(DOD_WP_AMERKNIFE);
						ws->AddWeaponToInventory(DOD_WP_FRAG_US);
						break;
					case DOD_CLASS_SNIPER:
						ws->AddWeaponToInventory(DOD_WP_SPRING);
						ws->AddWeaponToInventory(DOD_WP_COLT);
						ws->AddWeaponToInventory(DOD_WP_AMERKNIFE);
						break;
					case DOD_CLASS_MACHINEGUNNER:
						ws->AddWeaponToInventory(DOD_WP_30CAL);
						ws->AddWeaponToInventory(DOD_WP_COLT);
						ws->AddWeaponToInventory(DOD_WP_AMERKNIFE);
						break;
					case DOD_CLASS_ROCKET:
						ws->AddWeaponToInventory(DOD_WP_BAZOOKA);
						ws->AddWeaponToInventory(DOD_WP_M1CARBINE);
						ws->AddWeaponToInventory(DOD_WP_AMERKNIFE);
						break;
					}
					break;
				}
			}			
			break;
		}
		HANDLER(MESSAGE_DEATH)
		{
			const Event_Death_DOD *m = _message.Get<Event_Death_DOD>();
			_cb.CallScript();
			_cb.AddEntity("inflictor", m->m_WhoKilledMe);				
			_cb.AddString("meansofdeath", m->m_MeansOfDeath);
			return; // early out since we are overriding a shared event
			//break;
		}
		HANDLER(MESSAGE_KILLEDSOMEONE)
		{
			const Event_KilledSomeone_DOD *m = _message.Get<Event_KilledSomeone_DOD>();
			_cb.CallScript();
			_cb.AddEntity("victim", m->m_WhoIKilled);
			_cb.AddString("meansofdeath", m->m_MeansOfDeath);
			return; // early out since we are overriding a shared event
			//break;
		}	
	}

	Client::ProcessEvent(_message, _cb);
}

void DOD_Client::SetupBehaviorTree()
{
	using namespace AiState;
	
}
