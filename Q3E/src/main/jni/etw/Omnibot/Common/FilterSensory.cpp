#include "PrecompCommon.h"

int FilterSensory::ANYPLAYERCLASS = 0;

FilterSensory::FilterSensory(Client *_client, AiState::SensoryMemory::Type _type) 
	: m_Client			(_client)
	, m_NumPositions	(0)
	, m_MaxDistance		(-1.f)
	, m_Type			(_type)
	, m_MemorySpan		(0)
	, m_SortType		(Sort_None)
	, m_AnyPlayerClass	(false)
{
	ClearPositions();
	ResetClasses();
	ResetCategory();
}

void FilterSensory::MaxMemorySpan()
{
	m_MemorySpan = std::numeric_limits<int>::max();
}

void FilterSensory::AddPosition(const Vector3f &_pos)
{
	if(m_NumPositions < MaxPositions-1)
		m_Position[m_NumPositions++] = _pos;
}

void FilterSensory::ClearPositions()
{
	m_ClosestPosition = 0;
	m_NumPositions = 0;
	for(int i = 0; i < MaxPositions; ++i)
		m_Position[i] = Vector3f::ZERO;
}

void FilterSensory::AddClass(int _class)
{
	if(_class == ANYPLAYERCLASS)
		m_AnyPlayerClass = true;

	for(int i = 0; i < MaxClassType; ++i)
	{
		if(m_ClassType[i] == 0)
		{
			m_ClassType[i] = _class;
			break;
		}
	}
}

void FilterSensory::AddCategory(int _category)
{
	m_Category.SetFlag(_category, true);
}

void FilterSensory::AddIgnoreEntity(GameEntity _ent)
{
	for(int i = 0; i < MaxIgnoreEntity; ++i)
	{
		if(!m_IgnoreEntity[i].IsValid())
		{
			m_IgnoreEntity[i] = _ent;
			break;
		}
	}
}

bool FilterSensory::IsBeingIgnored(GameEntity _ent)
{
	for(int i = 0; i < MaxIgnoreEntity; ++i)
	{
		if(m_IgnoreEntity[i].IsValid() && m_IgnoreEntity[i] == _ent)
		{
			return true;
		}
	}
	return false;
}

void FilterSensory::ResetClasses()
{
	m_AnyPlayerClass = false;
	for(int i = 0; i < MaxClassType; ++i)
		m_ClassType[i] = 0;
}

void FilterSensory::ResetCategory()
{
	m_Category.ClearAll();
}

void FilterSensory::ResetIgnoreEntity()
{
	for(int i = 0; i < MaxIgnoreEntity; ++i)
		m_IgnoreEntity[i].Reset();
}