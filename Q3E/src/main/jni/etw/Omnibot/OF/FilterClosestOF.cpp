////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompOF.h"
#include "OF_Config.h"
#include "FilterClosestOF.h"

// class: FilterClosestTF
//		This filter is specific to team fortress, and should take into account additional sensory considerations,
//		such as spy disguises, spy feigning

FilterClosestOF::FilterClosestOF(Client *_client, AiState::SensoryMemory::Type _type) :
	FilterClosestTF	(_client, _type)
{
}

bool FilterClosestOF::CheckEx(const GameEntity _ent, const MemoryRecord &_record)
{
	if(m_Client->GetEntityFlags().CheckFlag(OF_ENT_FLAG_BLIND))
		return false;

	return FilterClosestTF::CheckEx(_ent, _record);
}
