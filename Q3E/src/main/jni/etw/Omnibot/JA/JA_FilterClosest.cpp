////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompJA.h"
#include "JA_FilterClosest.h"

#include "ScriptManager.h"

JA_FilterClosest::JA_FilterClosest(Client *_client, AiState::SensoryMemory::Type _type) :
	FilterClosest				(_client, _type)
{
}

bool JA_FilterClosest::CheckEx(const GameEntity _ent, const MemoryRecord &_record)
{
	if (_record.m_TargetInfo.m_EntityClass == JA_CLASSEX_CORPSE)
		return false;

	if (_record.m_TargetInfo.m_EntityFlags.CheckFlag(JA_ENT_FLAG_CLOAKED) && !_record.m_TargetInfo.m_EntityFlags.CheckFlag(JA_ENT_FLAG_MUTANT))
		return false;

	if (InterfaceFuncs::HasMeMindTricked(m_Client, m_Client->GetTargetingSystem()->GetLastTarget()))
		return false;

	return true;
}
