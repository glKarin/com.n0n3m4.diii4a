#include "PrecompCommon.h"
#include "FilterBias.h"

FilterBiasEntityFlag::FilterBiasEntityFlag(const int _entflags, const float _multiplier) :
	m_EntityFlag			(_entflags),
	m_DistanceMultiplier	(_multiplier)
{
}

float FilterBiasEntityFlag::CheckBias(const AiState::SensoryMemory::MemoryMap::const_iterator &_it)
{
	if(_it->second.m_TargetInfo.m_EntityFlags.CheckFlag(m_EntityFlag))
		return m_DistanceMultiplier;	
	return 1.0f;
}

//////////////////////////////////////////////////////////////////////////
