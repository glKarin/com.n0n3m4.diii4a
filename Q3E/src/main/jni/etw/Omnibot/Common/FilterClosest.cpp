#include "PrecompCommon.h"
#include "FilterClosest.h"

FilterClosest::FilterClosest(Client *_client, AiState::SensoryMemory::Type _type) :
	FilterSensory(_client, _type),
	m_BestDistanceSq(Utils::FloatMax)
{
}

FilterClosest::~FilterClosest()
{
}

void FilterClosest::Reset()
{
	FilterSensory::Reset();
	m_BestDistanceSq = Utils::FloatMax;
}

void FilterClosest::Check(int _index, const MemoryRecord &_record)
{
	if(m_MemorySpan==0) 
		m_MemorySpan = m_Client->GetSensoryMemory()->GetMemorySpan();

	const bool bIsStatic = _record.m_TargetInfo.m_EntityCategory.CheckFlag(ENT_CAT_STATIC);
	if(bIsStatic || (IGame::GetTime() - _record.GetTimeLastSensed()) <= m_MemorySpan)
	{
		switch(m_Type)
		{
		case AiState::SensoryMemory::EntAlly:
			if(!_record.IsAllied())
				return;
			break;
		case AiState::SensoryMemory::EntEnemy:
			if(_record.IsAllied())
				return;
			break;
		case AiState::SensoryMemory::EntAny:
			break;
		}
		
		if(IsBeingIgnored(_record.GetEntity()))
			return;

		Vector3f vSensoryPos;
		if(!m_NumPositions)
		{
			m_ClosestPosition = 0;
			vSensoryPos = m_Client->GetPosition();
		}
		else
		{
			//////////////////////////////////////////////////////////////////////////
			// Find the closest position
			float fClosest = Utils::FloatMax;
			for(int p = 0; p < m_NumPositions; ++p)
			{
				float fDistSq = (m_Position[p] - _record.GetLastSensedPosition()).SquaredLength();
				if(fDistSq < fClosest)
				{
					fClosest = fDistSq;
					vSensoryPos = m_Position[p];
					m_ClosestPosition = p;
				}
			}
		}		

		//////////////////////////////////////////////////////////////////////////
		// Distance
		float fDistanceSq = (vSensoryPos - _record.GetLastSensedPosition()).SquaredLength();

		// Early out if this wouldn't have been closer anyhow even if it passed.
		if(fDistanceSq >= m_BestDistanceSq)
			return;

		if(m_MaxDistance > 0.f)
		{
			if(fDistanceSq > Mathf::Sqr(m_MaxDistance))
				return;
		}
		//////////////////////////////////////////////////////////////////////////

		// Make sure the class matches.
		if(m_AnyPlayerClass)
		{
			if(_record.m_TargetInfo.m_EntityClass >= ANYPLAYERCLASS)
				return;
		} 
		else if(!PassesFilter(_record.m_TargetInfo.m_EntityClass))
			return;

		// Make sure the category matches.
		if(m_Category.AnyFlagSet() && !(m_Category & _record.m_TargetInfo.m_EntityCategory).AnyFlagSet())
			return;

		// Make sure it isn't disabled.
		if(_record.m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DISABLED))
			return;

		if(_record.ShouldIgnore())
			return;

		// Only alive targets count for shootable
		if(m_Category.CheckFlag(ENT_CAT_SHOOTABLE))
		{
			if(bIsStatic && !_record.IsShootable())
				return;

			if(_record.m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DEAD))
				return;

			if(!m_Client->GetWeaponSystem()->CanShoot(_record))
				return;
		}

		// Get the distance to this person.		
		if(CheckEx(_record))
		{
			m_BestDistanceSq = fDistanceSq;
			m_BestEntity = _record.GetEntity();
		}			
	}
}
