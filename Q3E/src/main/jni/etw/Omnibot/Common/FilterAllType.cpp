#include "PrecompCommon.h"
#include "FilterAllType.h"

FilterAllType::FilterAllType(Client *_client, AiState::SensoryMemory::Type _type, MemoryRecords &_list) :
	FilterSensory	(_client, _type),
	m_List			(_list)
{
}

void FilterAllType::Check(int _index, const MemoryRecord &_record)
{
	if(m_MemorySpan==0) 
		m_MemorySpan = m_Client->GetSensoryMemory()->GetMemorySpan();

	if(_record.m_TargetInfo.m_EntityCategory.CheckFlag(ENT_CAT_STATIC) ||
		(IGame::GetTime() - _record.GetTimeLastSensed()) <= m_MemorySpan)
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

		if(IsBeingIgnored(_record.GetEntity()))
			return;

		float fDistanceSq = (vSensoryPos - _record.GetLastSensedPosition()).SquaredLength();
		if(m_MaxDistance > 0.f)
		{
			if(fDistanceSq > Mathf::Sqr(m_MaxDistance))
				return;
		}

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
		
		if(!CheckEx(_record))
			return;

		if(_record.ShouldIgnore())
			return;

		// Only alive targets count for shootable
		if(m_Category.CheckFlag(ENT_CAT_SHOOTABLE))
		{
			if(_record.m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DEAD))
				return;

			if(!m_Client->GetWeaponSystem()->CanShoot(_record))
				return;
		}

		if(m_SortType == Sort_None)
		{
			m_List.push_back(RecordHandle((obint16)_index, _record.GetSerial()));
		}
		else
		{
			OBASSERT(0, "Not Implemented yet!");

			// Get the distance to this person.
			/*float fCurDistanceToSq = (vSensoryPos - _record.m_TargetInfo.m_LastPosition).SquaredLength();
			fCurDistanceToSq;
			for(obuint32 i = 0; i < m_List.size(); ++i)
			{
				switch(m_SortType)
				{
				case Sort_NearToFar:
					{
						const MemoryRecord *pRec = 
						float fDistSq = (m_Position - m_List[i]->GetLastSensedPosition()).SquaredLength();
						if(fCurDistanceToSq < fDistSq)
						{
							m_List.insert(i, &_record);
							break;
						}
						break;
					}
				case Sort_FarToNear:
					{
						float fDistSq = (m_Position - m_List[i]->GetLastSensedPosition()).SquaredLength();
						if(fCurDistanceToSq > fDistSq)
						{

						}
						break;
					}
				case Sort_None:
				default:
					break;
				}
			}*/
		}
	}
}
