#include "PrecompCommon.h"
#include "FilterMostHurt.h"

FilterMostHurt::FilterMostHurt(Client *_client, AiState::SensoryMemory::Type _type) :
	FilterSensory(_client, _type),
	m_MostHurtHealthPc(1.0f)
{
}

FilterMostHurt::~FilterMostHurt()
{
}

void FilterMostHurt::Reset()
{
	FilterSensory::Reset();
	m_MostHurtHealthPc = 1.0f;
}

void FilterMostHurt::Check(int _index, const MemoryRecord &_record)
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

		// Only alive targets count for shootable
		if(m_Category.CheckFlag(ENT_CAT_SHOOTABLE) && _record.m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DEAD))
			return;

		// Make sure it isn't disabled.
		if(_record.m_TargetInfo.m_EntityFlags.CheckFlag(ENT_FLAG_DISABLED))
			return;

		// Check the health
		Msg_HealthArmor healthArmor;
		InterfaceFuncs::GetHealthAndArmor(_record.GetEntity(), healthArmor);
		if(healthArmor.m_CurrentHealth > 0)
		{
			float m_HealthPercent = (float)healthArmor.m_CurrentHealth / (float)healthArmor.m_MaxHealth;
			if(m_HealthPercent < m_MostHurtHealthPc)
			{
				m_MostHurtHealthPc = m_HealthPercent;
				m_BestEntity = _record.GetEntity();
			}
		}		
	}
}
