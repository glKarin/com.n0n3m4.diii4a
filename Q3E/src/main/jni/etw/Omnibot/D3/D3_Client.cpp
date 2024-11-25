////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompD3.h"
#include "D3_Client.h"
#include "D3_NavigationFlags.h"
#include "D3_Messages.h"

#include "IGame.h"
#include "IGameManager.h"

namespace AiState
{
	class ShootExplodingBarrel : public StateChild, public AimerUser
	{
	public:

		void RenderDebug()
		{
			if(m_ShootBarrel.IsValid())
			{
				AABB aabb;
				EngineFuncs::EntityWorldAABB(m_ShootBarrel, aabb);
				Utils::OutlineAABB(aabb, COLOR::GREEN, 0.2f);
			}
		}

		obReal GetPriority() 
		{
			const MemoryRecord *pTarget = GetClient()->GetTargetingSystem()->GetCurrentTargetRecord();
			if(!pTarget)
				return 0.f;

			if(m_ShootBarrel.IsValid())
				return 1.f;

			Vector3f vTargetPos = pTarget->PredictPosition(1.f);

			static float fSplashRadius = 256.f;

			SensoryMemory *sensory = GetClient()->GetSensoryMemory();

			// Check for exploding barrels near my target.
			MemoryRecords records;
			Vector3List recordpos;

			FilterAllType filter(GetClient(), AiState::SensoryMemory::EntAny, records);
			filter.MemorySpan(Utils::SecondsToMilliseconds(7.f));
			filter.AddCategory(ENT_CAT_MISC);
			filter.AddClass(ENT_CLASS_EXPLODING_BARREL);
			sensory->QueryMemory(filter);

			sensory->GetRecordInfo(records, &recordpos, NULL);

			for(obuint32 i = 0; i < recordpos.size(); ++i)
			{
				if(SquaredLength(recordpos[i], vTargetPos) < Mathf::Sqr(fSplashRadius))
				{
					MemoryRecord *pRec = sensory->GetMemoryRecord(records[i]);
					if(pRec)
					{
						m_ShootBarrel = pRec->GetEntity();
						return 1.f;
					}
				}
			}
			return 0.f;
		}

		void Enter()
		{
			FINDSTATEIF(Aimer, GetParent(), AddAimRequest(Priority::Medium, this, GetNameHash()));
		}

		void Exit()
		{
			m_ShootBarrel.Reset();
			FINDSTATEIF(Aimer, GetParent(), ReleaseAimRequest(GetNameHash()));
		}

		State::StateStatus Update(float fDt) 
		{
			if(!m_ShootBarrel.IsValid() || !InterfaceFuncs::IsAlive(m_ShootBarrel))
				return State_Finished;
			return State_Busy; 
		}

		bool GetAimPosition(Vector3f &_aimpos)
		{
			AABB aabb;
			if(m_ShootBarrel.IsValid() && EngineFuncs::EntityWorldAABB(m_ShootBarrel, aabb))
			{
				Vector3f vAimPos;				
				aabb.CenterPoint(vAimPos);
				_aimpos = vAimPos;
				return true;
			}
			return false;
		}

		void OnTarget()
		{
			FINDSTATE(wsys, WeaponSystem, GetParent());
			if(wsys)
			{
				WeaponPtr wpn = wsys->GetCurrentWeapon();
				if(wpn)
				{
					if(wsys->ReadyToFire())
					{
						wpn->PreShoot(Primary);
						wpn->Shoot();
					}
					else
					{
						wpn->StopShooting(Primary);
					}
				}
			}
		}

		ShootExplodingBarrel() : StateChild("ShootExplodingBarrel")
		{
		}
	private:
		GameEntity	m_ShootBarrel;
	};
};

//////////////////////////////////////////////////////////////////////////

D3_Client::D3_Client()
{
	m_StepHeight = 15.0f; // it's actually 16
}

D3_Client::~D3_Client()
{
}

NavFlags D3_Client::GetTeamFlag()
{
	return GetTeamFlag(GetTeam());
}

NavFlags D3_Client::GetTeamFlag(int _team)
{
	static const NavFlags defaultTeam = 0;
	switch(_team)
	{
	case D3_TEAM_RED:
		return F_NAV_TEAM1;
	case D3_TEAM_BLUE:
		return F_NAV_TEAM2;
	default:
		return defaultTeam;
	}
}

float D3_Client::GetAvoidRadius(int _class) const
{
	switch(_class)
	{
	case D3_CLASS_PLAYER:
		return 32.0f;
	}
	return 0.0f;
}

float D3_Client::GetGameVar(GameVar _var) const
{
	switch(_var)
	{
	case JumpGapOffset:
		return 0.0f;
	}
	return 0.0f;
}

bool D3_Client::DoesBotHaveFlag(MapGoalPtr _mapgoal)
{
	int iTeamFlags = 0;
	switch(GetTeam())
	{
	case D3_TEAM_RED:
		iTeamFlags = D3_PWR_REDFLAG;
		break;
	case D3_TEAM_BLUE:
		iTeamFlags = D3_PWR_BLUEFLAG;
		break;
	}

	if(HasPowerup(iTeamFlags))
		return true;
	return false;
}

void D3_Client::SetupBehaviorTree()
{
	using namespace AiState;
	FINDSTATEIF(LowLevel,GetStateRoot(),AppendState(new ShootExplodingBarrel))
}
