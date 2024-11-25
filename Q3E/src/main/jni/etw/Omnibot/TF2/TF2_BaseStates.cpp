////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompTF2.h"
#include "TF2_BaseStates.h"
#include "TF2_NavigationFlags.h"
#include "TF2_Game.h"
#include "TF_InterfaceFuncs.h"
#include "ScriptManager.h"
#include "gmScriptGoal.h"

namespace AiState
{
	const float IdealMedigunRange = 512.f;
	const float MaxMedigunRange = 512.f;

	bool MediGun::Ent::_ByDistance(const Ent &ent1, const Ent &ent2)
	{
		const bool _inrange_1 = ent1.m_Distance < 1024.f;
		const bool _inrange_2 = ent2.m_Distance < 1024.f;

		if(_inrange_1 && !_inrange_2)
			return true;
		if(!_inrange_1 && _inrange_2)
			return false;

		const bool _full_1 = ent1.m_Health >= ent1.m_MaxHealth;
		const bool _full_2 = ent2.m_Health >= ent2.m_MaxHealth;

		const bool _over_1 = ent1.m_Health >= ent1.m_MaxHealth * 1.3f;
		const bool _over_2 = ent2.m_Health >= ent2.m_MaxHealth * 1.3f;

		if(!_full_1 && _full_2)
			return true;
		if(_full_1 && !_full_2)
			return false;

		if(!_over_1 && _over_2)
			return true;
		if(_over_1 && !_over_2)
			return false;

		return ent1.m_Health < ent2.m_Health;
	}

	MediGun::MediGun() 
		: StateChild("MediGun")
		, FollowPathUser("MediGun")
	{
		LimitToClass().SetFlag(TF_CLASS_MEDIC);
	}
	
	MediGun::Ent MediGun::NeedsHealing(GameEntity _ent)
	{
		Ent e;

		if(_ent.IsValid() && InterfaceFuncs::IsAlive(_ent))
		{
			Msg_HealthArmor h;
			if(InterfaceFuncs::GetHealthAndArmor(_ent,h))
			{
				e.m_Ent = _ent;

				e.m_Health = h.m_CurrentHealth;
				e.m_MaxHealth = h.m_MaxHealth;
			}
		}
		return e;
	}	
	GameEntity MediGun::FindHealerTarget()
	{
		//////////////////////////////////////////////////////////////////////////
		enum { MaxHealEnts = 32 };
		Ent HealEnts[MaxHealEnts];
		int NumHealEnts = 0;
		//////////////////////////////////////////////////////////////////////////

		SensoryMemory *sensory = GetClient()->GetSensoryMemory();

		MemoryRecords records;
		Vector3List recordpos;

		FilterAllType filter(GetClient(), AiState::SensoryMemory::EntAlly, records);
		filter.MemorySpan(Utils::SecondsToMilliseconds(7.f));
		filter.AddClass(TF_CLASS_ANY);
		sensory->QueryMemory(filter);
		sensory->GetRecordInfo(records, &recordpos, NULL);

		const Vector3f vMyPos = GetClient()->GetPosition();

		for(obuint32 i = 0; i < recordpos.size(); ++i)
		{
			MemoryRecord *rec = sensory->GetMemoryRecord(records[i]);
			if(rec)
			{
				HealEnts[NumHealEnts] = NeedsHealing(rec->GetEntity());
				if(HealEnts[NumHealEnts].m_Ent.IsValid())
				{
					HealEnts[NumHealEnts].m_Distance = Length(rec->GetLastSensedPosition(), vMyPos);
					++NumHealEnts;
				}
			}
		}

		if(NumHealEnts > 0)
		{
			std::sort(HealEnts,HealEnts+NumHealEnts, Ent::_ByDistance);
			return HealEnts[0].m_Ent;
		}
		return GameEntity();
	}
	//////////////////////////////////////////////////////////////////////////
	bool MediGun::GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt)
	{
		Vector3f vTargetPos;
		if(EngineFuncs::EntityPosition(m_HealEnt,vTargetPos))
		{
			_desination.push_back(Destination(vTargetPos, 64.f));
			_final = true;
			_skiplastpt = false;
			return true;
		}
		_desination.push_back(Destination(GetClient()->GetPosition(), 64.f));
		return false;
	}
	bool MediGun::GetAimPosition(Vector3f &_aimpos)
	{
		if(EngineFuncs::EntityPosition(m_HealEnt,_aimpos))
		{
			return true;
		}
		return false;
	}
	void MediGun::OnTarget()
	{
		FINDSTATE(ws, WeaponSystem, GetRootState());
		if(ws && ws->CurrentWeaponIs(TF2_WP_MEDIGUN))
			ws->FireWeapon();
	}	
	obReal MediGun::GetPriority()
	{
		if(IsActive())
			return GetLastPriority();

		GameEntity targEnt = FindHealerTarget();
		if(targEnt.IsValid())
			m_HealEnt = targEnt;

		return m_HealEnt.IsValid() ? 1.f : 0.f;
	}
	void MediGun::Enter()
	{
		FINDSTATEIF(FollowPath,GetRootState(),Goto(this));
	}
	void MediGun::Exit()
	{
		FINDSTATEIF(Aimer,GetRootState(),ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), ReleaseWeaponRequest(GetNameHash()));
	}
	State::StateStatus MediGun::Update(float fDt) 
	{ 
		FINDSTATE(fp,FollowPath,GetRootState());

		// Keep firing when healing.
		TF_HealTarget healTarget = InterfaceFuncs::GetHealTargetInfo(GetClient());
		if(healTarget.m_Healing && healTarget.m_Target == m_HealEnt)
		{
			FINDSTATE(ws, WeaponSystem, GetRootState());
			if(ws && ws->CurrentWeaponIs(TF2_WP_MEDIGUN))
				ws->FireWeapon();
		}

		Vector3f vTargetPos;
		if(!EngineFuncs::EntityPosition(m_HealEnt,vTargetPos))
			return State_Finished;
		
		const float DistToHealTargetSq = SquaredLength(GetClient()->GetPosition(),vTargetPos);

		Path::PathPoint pt;
		fp->GetCurrentPath().GetLastPt(pt);

		const float DistGoalToTargetSq = SquaredLength(pt.m_Pt,vTargetPos);

		bool Repath = false;
		if(fp->IsMoving())
		{
			if(DistGoalToTargetSq > Mathf::Sqr(256.f))
			{
				Repath = true;
			}
		}
		else
		{
			if(DistToHealTargetSq > Mathf::Sqr(256.f))
			{
				Repath = true;
			}
		}

		if(Repath)
		{
			FINDSTATEIF(FollowPath,GetRootState(),Goto(this));
		}		

		FINDSTATEIF(Aimer,GetRootState(),AddAimRequest(Priority::High,this,GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetRootState(), AddWeaponRequest(Priority::High, GetNameHash(), TF2_WP_MEDIGUN));
		
		return State_Busy; 
	}
	void MediGun::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
	{
		switch(_message.GetMessageId())
		{
			HANDLER(PERCEPT_FEEL_PAIN)
			{
				const Event_TakeDamage *m = _message.Get<Event_TakeDamage>();
				if(m->m_Inflictor.IsValid())
				{
					
				}
				break;
			}	
		}
	}
};
