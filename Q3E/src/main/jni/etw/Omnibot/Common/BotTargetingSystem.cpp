#include "PrecompCommon.h"
#include "Client.h"

namespace AiState
{
	TargetingSystem::TargetingSystem() : StateChild("TargetingSystem")
	{
	}

	TargetingSystem::~TargetingSystem()
	{
	}

	void TargetingSystem::RenderDebug()
	{
		if(m_CurrentTarget.IsValid())
		{
			Vector3f vPos;
			EngineFuncs::EntityPosition(m_CurrentTarget, vPos);
			Utils::DrawLine(GetClient()->GetEyePosition(), vPos, COLOR::RED, IGame::GetDeltaTimeSecs());
		}
		if(m_LastTarget.IsValid())
		{
			Vector3f vPos;
			EngineFuncs::EntityPosition(m_LastTarget, vPos);
			Utils::DrawLine(GetClient()->GetEyePosition(), vPos, COLOR::ORANGE, IGame::GetDeltaTimeSecs());
		}
	}

	void TargetingSystem::ForceTarget(GameEntity _ent)
	{
		m_ForceTarget = _ent;
	}

	const MemoryRecord *TargetingSystem::GetCurrentTargetRecord() const
	{
		// todo: cache the record
		return HasTarget() ? GetClient()->GetSensoryMemory()->GetMemoryRecord(GetCurrentTarget(), false, true) : 0;
	}

	//////////////////////////////////////////////////////////////////////////
	// State stuff

	void TargetingSystem::Initialize()
	{
		// Give the bot a default targeting filter.
		FilterPtr filter(new FilterClosest(GetClient(), SensoryMemory::EntEnemy));
		filter->AddCategory(ENT_CAT_SHOOTABLE);
		SetDefaultTargetingFilter(filter);
	}

	void TargetingSystem::Exit()
	{
		m_CurrentTarget.Reset();
	}

	State::StateStatus TargetingSystem::Update(float fDt)
	{
		Prof(TargetingSystem);
		{
			Prof(UpdateTargeting);

			GameEntity newtarget;

			if(m_ForceTarget.IsValid())
			{
				const MemoryRecord *pRec = GetClient()->GetSensoryMemory()->GetMemoryRecord(m_ForceTarget);
				if(!pRec || pRec->m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DEAD))
					m_ForceTarget.Reset();
				else
					newtarget = m_ForceTarget;
			}

			if(!newtarget.IsValid())
			{
				m_DefaultFilter->Reset();
				GetClient()->GetSensoryMemory()->QueryMemory(*m_DefaultFilter.get());
				newtarget = m_DefaultFilter->GetBestEntity();
			}

			// Update the last target.
			if(newtarget.IsValid() && m_CurrentTarget.IsValid())
			{
				if(newtarget != m_CurrentTarget)
					m_LastTarget = m_CurrentTarget;
			}

			m_CurrentTarget = newtarget;
		}
		return State_Busy;
	}

}
