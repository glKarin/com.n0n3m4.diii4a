////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompTF2.h"
#include "TF2_Client.h"
#include "TF2_NavigationFlags.h"
#include "TF2_Messages.h"
#include "TF2_BaseStates.h"

#include "IGame.h"
#include "IGameManager.h"

namespace AiState
{
	
};

//////////////////////////////////////////////////////////////////////////

TF2_Client::TF2_Client()
{
	m_StepHeight = 15.0f; // it's actually 16
}

TF2_Client::~TF2_Client()
{
}

//NavFlags TF2_Client::GetTeamFlag()
//{
//	return GetTeamFlag(GetTeam());
//}
//
//NavFlags TF2_Client::GetTeamFlag(int _team)
//{
//	static const NavFlags defaultTeam = 0;
//	switch(_team)
//	{
//	case TF_TEAM_RED:
//		return F_NAV_TEAM1;
//	case TF_TEAM_BLUE:
//		return F_NAV_TEAM2;
//	default:
//		return defaultTeam;
//	}
//}
//
////float TF2_Client::GetAvoidRadius(int _class) const
////{
////	switch(_class)
////	{
////	case TF_CLASS_PLAYER:
////		return 32.0f;
////	}
////	return 0.0f;
////}
//
//float TF2_Client::GetGameVar(GameVar _var) const
//{
//	switch(_var)
//	{
//	case JumpGapOffset:
//		return 0.0f;
//	}
//	return 0.0f;
//}
//
//bool TF2_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
//{
//	return false;
//}
//

void TF2_Client::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
{
	switch(_message.GetMessageId())
	{
		HANDLER(MESSAGE_CHANGECLASS)
		{
			AiState::WeaponSystem *ws = GetWeaponSystem();
			ws->ClearWeapons();
			const Event_ChangeClass *m = _message.Get<Event_ChangeClass>();
			if(m)
			{
				ws->AddWeaponToInventory(TF2_WP_MELEE);
				switch(m->m_NewClass)
				{
				case TF_CLASS_SCOUT:
					ws->AddWeaponToInventory(TF2_WP_SCATTERGUN);
					ws->AddWeaponToInventory(TF2_WP_PISTOL);
					break;
				case TF_CLASS_SNIPER:
					ws->AddWeaponToInventory(TF2_WP_SNIPER_RIFLE);
					ws->AddWeaponToInventory(TF2_WP_SMG);
					break;
				case TF_CLASS_SOLDIER:
					ws->AddWeaponToInventory(TF2_WP_ROCKET_LAUNCHER);
					ws->AddWeaponToInventory(TF2_WP_SHOTGUN);
					break;
				case TF_CLASS_DEMOMAN:
					ws->AddWeaponToInventory(TF2_WP_GRENADE_LAUNCHER);
					ws->AddWeaponToInventory(TF2_WP_PIPE_LAUNCHER);
					break;
				case TF_CLASS_MEDIC:
					ws->AddWeaponToInventory(TF2_WP_SYRINGE_GUN);
					ws->AddWeaponToInventory(TF2_WP_MEDIGUN);
					break;
				case TF_CLASS_HWGUY:
					ws->AddWeaponToInventory(TF2_WP_MINIGUN);
					ws->AddWeaponToInventory(TF2_WP_SHOTGUN);
					break;
				case TF_CLASS_PYRO:
					ws->AddWeaponToInventory(TF2_WP_FLAMETHROWER);
					ws->AddWeaponToInventory(TF2_WP_SHOTGUN);
					break;
				case TF_CLASS_SPY:
					ws->AddWeaponToInventory(TF2_WP_REVOLVER);
					//ws->AddWeaponToInventory(TF2_WP_ELECTRO_SAPPER);
					break;
				case TF_CLASS_ENGINEER:
					ws->AddWeaponToInventory(TF2_WP_SHOTGUN);
					ws->AddWeaponToInventory(TF2_WP_PISTOL);

					//ws->AddWeaponToInventory(TF2_WP_ENGINEER_BUILD);
					//ws->AddWeaponToInventory(TF2_WP_ENGINEER_DESTROY);
					break;
				}
			}			
			break;
		}
		HANDLER(MESSAGE_DEATH)
		{
			const Event_Death_TF2 *m = _message.Get<Event_Death_TF2>();
			_cb.CallScript();
			_cb.AddEntity("inflictor", m->m_WhoKilledMe);				
			_cb.AddString("meansofdeath", m->m_MeansOfDeath);
			_cb.AddInt("dominated",m->m_Dominated);
			_cb.AddInt("dominated_assist",m->m_Dominated_Assister);
			_cb.AddInt("revenge",m->m_Revenge);
			_cb.AddInt("revenge_assist",m->m_Revenge_Assister);
			return; // early out since we are overriding a shared event
			//break;
		}
		HANDLER(MESSAGE_KILLEDSOMEONE)
		{
			const Event_KilledSomeone_TF2 *m = _message.Get<Event_KilledSomeone_TF2>();
			_cb.CallScript();
			_cb.AddEntity("victim", m->m_WhoIKilled);
			_cb.AddString("meansofdeath", m->m_MeansOfDeath);
			_cb.AddInt("dominated",m->m_Dominated);
			_cb.AddInt("dominated_assist",m->m_Dominated_Assister);
			_cb.AddInt("revenge",m->m_Revenge);
			_cb.AddInt("revenge_assist",m->m_Revenge_Assister);
			return; // early out since we are overriding a shared event
			//break;
		}	
	}

	TF_Client::ProcessEvent(_message,_cb);
}

void TF2_Client::SetupBehaviorTree()
{
	TF_Client::SetupBehaviorTree();

	using namespace AiState;

	delete GetStateRoot()->RemoveState("ThrowGrenade");
	delete GetStateRoot()->RemoveState("Detpack");
	delete GetStateRoot()->RemoveState("ConcussionJump");

	GetStateRoot()->AppendTo("HighLevel", new Teleporter);
	GetStateRoot()->AppendTo("HighLevel", new MediGun);
}
